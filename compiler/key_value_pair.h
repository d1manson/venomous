/*
	KeyValuePair class

	stores:  [header] [key] [padding] [value]
	        | fixed  |     fixed total       |

	It is cache-line aligned, hard-coded at 64bytes, this means its length is also
	a multiple of cache line in length.  The padding is calculated so as to align
	the value to its required alignment (which must be compatible with the KVP 
	overal alignment, i.e. if KVP is 64B-cache-line aligned, value can be
	1,2,4,8,16,32, or 64 aligned, but nothing larger.) 

	header is of fixed size, and key is an array of
	key_element_t, with length given by value::accompanying_key_n.

	The header has a type_id() method which is aliased to key_prefix(). This
	is considered to be part of the key, the other header data is bitflags that
	record state of the value and thread-wise locks.

	The class is templated on the list of value types (and the 
	header implementation).

	You can move/copy construct KeyValuePairs, but generally
	you will default construct, which puts the header into a "null"
	state.  From there you can use placement_new_key(...) and then
	placement_new_value<Q>(...) to properly construct in two steps.

	You can also move assign.

	When you want to access the key, if you know the value type at compile-time,
	you can use get_key_before<Q>, otherwise you have to use
	begin_key() and end_key(). (Note that actually only end_key actually
	depends on the value type.)

	When you want to access the value, you need to know what it is
	at compile-time, you access it with get<Q>().

	Copying/moving/destructing KeyValuePairs is achieved with custom built 
	vtables - value types must all be copy (and ideally move) constructible. 
	There is no run-time way of accessing an unknown-type value...
	because what use would that be?
	
	Most of the above methods come with a 'c'-prefixed verison for
	const return.

	if you #define NDEBUG, then a bunch of assertions will be disabled,
	meaning you can accidentally interpret the value as the wrong type,
	or copy/move/destruct inappropriately.
	
	The header is responsible for holding the state of the KeyValuePair, and it is
	exposed as the public base class:
		is_valid_type/is_tombstone/is_null - only one of these is ever true
		type_id - index into value type list, only allowed when is_valid_type
				this is aliased to key_prefix in the KVP.
		is_constructed - when is_valid_type, this says whether the value has
					    been constructed (i.e. is it safe to destruct/move/copy)
		safe_to_read - checks whether a lock bit is set for current thread
						or no-op on main thread
		main_thread_only_ref - checks whether no threads (apart from main) 
						are currently safe to read, i.e. is main safe to
						destruct/move/copy etc.?
	The header is probably a 64bit atomic thing, as most of its state is
	used in an atomic-neccessary way (including aquire/release semantics)
	but the total amount of state should fit in 64 bits really.
	
	-----

	Note that although we came close to implementing this without
	reinterpret casts, but in the end it seemed impossible to guarnatee
	that the array portition of the variants lined up, because the 
	data types did not statisfy "standardLayout" requirements.  So it
	was not safe to access the array at the  head of the variant, even if
	we knew that all of the types in the variant began with an array.

*/

#ifndef _KeyValuePair_H_
#define _KeyValuePair_H_

#include <array>
#include <utility>
#include <cassert>
#include <type_traits>

#include "tmp_utils.h"


namespace key_value_pair_impl{

template <typename Q, typename KVP>
void destroy_value(KVP& self){
	self. template get<Q>().~Q();
}

template <typename Q, typename KVP>
void move_value(KVP& dest, KVP&& other){
	auto& destq = dest. template get<Q>();
	auto& otherq = other. template get<Q>();
	new (&destq) Q(std::move(otherq)); // placement new, i.e. move construction into uninitialized union
}

template <typename Q, typename KVP>
void copy_value(KVP& dest, KVP const& other){
	auto& destq = dest. template get<Q>();
	auto& otherq = other. template cget<Q>();
	new (&destq) Q(otherq); // placement new, i.e. copy construction into uninitialized union
}


template<typename Q, typename KVP>
constexpr size_t offset_for_value(){
	static_assert(sizeof(typename KVP::header_t_) % alignof(typename KVP::key_element_t_) == 0,
			      "key_element_t alignment requires padding after header_t"); // can relax this if we are careful below

	return	utils::round_length_to_alignment(
				sizeof(typename KVP::key_element_t_) * Q::accompanying_key_n,
				alignof(Q),
				sizeof(typename KVP::header_t_),
				KVP::minimum_alignment);
}

template<typename Q, typename KVP>
constexpr size_t length_for_key_and_value(){
	return offset_for_value<Q, KVP>() + sizeof(Q);
}

} //key_value_pair_impl

struct alignas(8) KeyValueHeader{
	using type_id_t = uint8_t;
	static const type_id_t null = -1;
	static const type_id_t tombstone = -2;

	type_id_t _type_id = null; 
	uint8_t _is_constructed = 0; // todo this is really supposed to be a bit field
		
	bool is_constructed() const{
		return _is_constructed; // probably need an atomic aquire here
	}
	void set_constructed(bool value) {
		assert(is_valid_type());
		_is_constructed = value;
		// TODO: probably need an atomic_release here
	}
	void destruct_to_tombstone() {
		_is_constructed = 0;
		_type_id = tombstone;
	}
	void tombstone_to_null(){
		assert(is_tombstone());
		_type_id = null;
	}
	void set_to_type_id(type_id_t idx){
		assert(!is_valid_type());
		assert(idx != null && idx != tombstone);
		_type_id = idx;
	}
	bool is_null() const{
		return _type_id == null;
	}
	bool is_tombstone() const{
		return _type_id == tombstone;
	}
	bool is_valid_type() const{
		return !(_type_id == null || _type_id == tombstone);
	}
	bool safe_to_read(size_t thread_id) const{
		return true; // TODO: implement thread-wise bit fields indicating read lock reference.
	}
	bool main_thread_only_ref() const{
		return true; // TODO: when we have thread-wise bit fields, check all are zero apart from main thread
	}
	auto type_id() const{
		return _type_id; 
	}
};

const size_t CACHE_LINE_LEN = 64;

// header_t basically has to be KeyValueHeader class above, or similar
template<typename header_t, typename key_element_t, typename ...Qs>
class 
alignas(CACHE_LINE_LEN /*see minimum_alignment below*/) 
KeyValuePair : public header_t {
	public:
	static const size_t minimum_alignment = CACHE_LINE_LEN;

	static_assert(std::is_pod<key_element_t>::value, 
				  "key_element_t should be POD."); // could maybe relax this
	using self_t = KeyValuePair<header_t, key_element_t, Qs...>; //convenience
	using header_t_ = header_t;
	using key_prefix_t = typename header_t::type_id_t;
	using key_element_t_ = key_element_t; //note that this is the type within the array, not the array itself
	using dtor_t = void(*)(self_t&);
	using mvctor_t = void(*)(self_t&, self_t&&);
	using cpctor_t = void(*)(self_t&, self_t const&);
	
	const static std::array<dtor_t, sizeof...(Qs)> dtor_vtable; //can't compile when this is constexpr, not sure whether there's a difference.
	const static std::array<mvctor_t, sizeof...(Qs)> mvctor_vtable; 
	const static std::array<cpctor_t, sizeof...(Qs)> cpctor_vtable; 
	const static std::array<size_t, sizeof...(Qs)> key_length_table; // note this doesn't dictate offset for value due to alignment issues
	
	const static size_t max_len_key_and_value = utils::max_element<key_value_pair_impl::
										length_for_key_and_value<Qs, self_t>()...>();
	std::array<uint8_t, max_len_key_and_value> as_u8_array; 

	KeyValuePair() {
		assert(header_t::is_null() && !header_t::is_constructed());
	}

	// construct from a copy of a KeyValuePair
	KeyValuePair(self_t const& other) 
			: header_t(other) {
		std::copy(other.as_u8_array.begin(), other.as_u8_array.end(), as_u8_array.begin());
		if(header_t::is_valid_type() && header_t::is_constructed())
			cpctor_vtable[header_t::type_id()](*this, other);
	}

	// construct by moving from (i.e. ripping the guts out of) another KeyValuePair
	KeyValuePair(self_t&& other)
			: header_t(other) {
		std::copy(other.as_u8_array.begin(), other.as_u8_array.end(), as_u8_array.begin());
		if(header_t::is_valid_type() && header_t::is_constructed())
			mvctor_vtable[header_t::type_id()](*this, std::move(other));
	}

	~KeyValuePair(){
		if(header_t::is_valid_type() && header_t::is_constructed())
			dtor_vtable[header_t::type_id()](*this);
	}

	// move-assign another KeyValuePair to this one
	// we only allow this if this KeyValuePair is in a tombstone/null state
	// otherwise this is identical to move-constructor.
	self_t& operator=(self_t&& other){
		// TODO: assert(is_on_main_thread)
		assert(!header_t::is_valid_type());
		header_t::operator=(other);

		std::copy(other.as_u8_array.begin(), other.as_u8_array.end(), as_u8_array.begin());
		if(header_t::is_valid_type() && header_t::is_constructed())
			mvctor_vtable[header_t::type_id()](*this, std::move(other));	
		return *this;
	}

	void destruct_to_tombstone(){
		// TODO: assert(is_on_main_thread);
		assert(header_t::main_thread_only_ref());

		if(header_t::is_valid_type() && header_t::is_constructed())
			dtor_vtable[header_t::type_id()](*this);
		header_t::destruct_to_tombstone();
	}

	void tombstone_to_null(){
		// TODO: assert(is_on_main_thread)
		assert(header_t::main_thread_only_ref());
		assert(header_t::is_tombstone());
		header_t::tombstone_to_null();
	}

	void placement_new_key(typename header_t::type_id_t type_id_in, key_element_t const* begin,
						   key_element_t const* end){
		/* this lets you assing key/prefix without constructing the value, 
		this should only be called if the KeyValuePair is in null/tombstone state.*/
		assert(!header_t::is_valid_type()); // otherwise we should destruct first
		assert(end - begin == key_length_table[type_id_in]);		

		header_t::set_to_type_id(type_id_in);
		std::copy(begin, end, begin_key());
		assert(!header_t::is_constructed()); // we haven't constructed Q yet, so don't claim we have
	}

	template<typename Q, typename ...Args>
	void placement_new_value(Args&&... args){
		// this lets you create de-novo, copy/move construct from another Q
		// when the key/prefix were previously initialized
		// once you've called this you are then in the realm of copy/move/dtor
		// with vtables, but before you call this prefix.is_constructed() is false,
		// so no copy/move/dtor is invoked.
		assert(is_type<Q>());
		assert(!header_t::is_constructed());

		header_t::set_constructed(true); // this comes first to pass assertion in get<Q>
		new (& (get<Q>())) Q(std::forward<Args...>(args)... );
	}


	template<typename Q>
	Q& get(){
		assert(is_type<Q>() && header_t::is_constructed());
		size_t offset = key_value_pair_impl::offset_for_value<Q, self_t>();
		return *reinterpret_cast<Q*>(&as_u8_array[offset]);
	}

	template<typename Q>
	Q const& cget() const {
		assert(is_type<Q>() && header_t::is_constructed());
		size_t offset = key_value_pair_impl::offset_for_value<Q, self_t>();
		return *reinterpret_cast<Q const*>(&as_u8_array[offset]);
	}

	template<typename Q>
	std::array<key_element_t, Q::accompanying_key_n>& get_key_before(){
		assert(is_type<Q>());
		return *reinterpret_cast<std::array<key_element_t, Q::accompanying_key_n>*>(&as_u8_array[0]);	
	}

	template<typename Q>
	std::array<key_element_t, Q::accompanying_key_n> const& cget_key_before() const{
		assert(is_type<Q>());
		return *reinterpret_cast<std::array<key_element_t, Q::accompanying_key_n> const*>(&as_u8_array[0]);	
	}

	key_element_t* end_key(){
		return reinterpret_cast<key_element_t*>(&as_u8_array[
							key_length_table[header_t::type_id()]*sizeof(key_element_t)]);
	}
	key_element_t* begin_key(){
		return reinterpret_cast<key_element_t*>(&as_u8_array[0]);
	}
	key_element_t const* cend_key() const {
		return reinterpret_cast<key_element_t const*>(&as_u8_array[
							key_length_table[header_t::type_id()]*sizeof(key_element_t)]);
	}
	key_element_t const* cbegin_key() const {
		return reinterpret_cast<key_element_t const*>(&as_u8_array[0]);
	}

	key_prefix_t key_prefix() const{
		// this is an alias to type_id
		return header_t::type_id();
	}
private:
	template<typename Q>
	bool is_type() const{
		return header_t::type_id() == utils::index_of_type<Q, Qs...>();
	}
};

// construct dtor_vtable
template<typename header_t, typename key_element_t, typename ...Qs>
const std::array<typename KeyValuePair<header_t, key_element_t, Qs...>::dtor_t, sizeof...(Qs)> 
KeyValuePair<header_t, key_element_t, Qs...>::dtor_vtable = {
&key_value_pair_impl::destroy_value<Qs, KeyValuePair<header_t, key_element_t, Qs...>> ...};


// construct cpctor_vtable
template<typename header_t, typename key_element_t, typename ...Qs>
const std::array<typename KeyValuePair<header_t, key_element_t, Qs...>::cpctor_t, sizeof...(Qs)> 
KeyValuePair<header_t, key_element_t, Qs...>::cpctor_vtable = {
&key_value_pair_impl::copy_value<Qs, KeyValuePair<header_t, key_element_t, Qs...>> ...};

// construct mvctor_vtable
template<typename header_t, typename key_element_t, typename ...Qs>
const std::array<typename KeyValuePair<header_t, key_element_t, Qs...>::mvctor_t, sizeof...(Qs)> 
KeyValuePair<header_t, key_element_t, Qs...>::mvctor_vtable = {
&key_value_pair_impl::move_value<Qs, KeyValuePair<header_t, key_element_t, Qs...>> ...};

template<typename header_t, typename key_element_t, typename ...Qs>
const std::array<size_t, sizeof...(Qs)>
KeyValuePair<header_t, key_element_t, Qs...>::key_length_table = {Qs::accompanying_key_n...};

#endif // _KeyValuePair_H_