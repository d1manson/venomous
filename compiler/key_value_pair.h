/*
	key_value_pair class

	stores: [prefix] [key] [value]
	The total size is asserted to fit within a cache line (hard-coded at 64 bytes).

	prefix is of fixed size, and key is an array of
	key_element_t, with length given by value::accompanying_key_n.

	The prefix has a type_id() method which is considered
	to be part of the key, the other prefix data is bitflags that
	record state of the value and thread-wise locks.

	The class is templated on the list of value types (and the 
	prefix implementation).

	You can move/copy construct key_value_pairs, but generally
	you will default construct, which puts the prefix into a "null"
	state.  From there you can use placement_new_key(...) and then
	placement_new_value<Q>(...) to properly construct in two steps.

	You can also move assign.

	When you want to access the key, if you know the value type at compile-time,
	you can use get_key_before<Q>, otherwise you have to use
	begin_key() and end_key(). (Note that actually only end_key actually
	depends on the value type.)

	When you want to access the value, you need to know what it is
	at compile-time, you access it with get<Q>().

	Copying/moving/destructing key_value_pairs is achieved with custom built 
	vtables - value types must all be copy (and ideally move) constructible. 
	There is no run-time way of accessing an unknown-type value...
	because what use would that be?
	
	Most of the above methods come with a 'c'-prefixed verison for
	const return.

	if you #define NDEBUG, then a bunch of assertions will be disabled,
	meaning you can accidentally interpret the value as the wrong type,
	or copy/move/destruct inappropriately.
	
	The prefix is responsible for holding the state of the key_value_pair:
		is_valid_type/is_tombstone/is_null - only one of these is ever true
		type_id - index into value type list, only allowed when is_valid_type
		is_constructed - when is_valid_type, this says whether the value has
					    been constructed (i.e. is it safe to destruct/move/copy)
		safe_to_read - checks whether a lock bit is set for current thread
						or no-op on main thread
		main_thread_only_ref - checks whether no threads (apart from main) 
						are currently safe to read, i.e. is main safe to
						destruct/move/copy etc.?
	The prefix is probably a 64bit atomic thing, as most of its state is
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

#ifndef _KEY_VALUE_PAIR_H_
#define _KEY_VALUE_PAIR_H_

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

template<typename prefix_t, typename key_element_t, typename Q, size_t max>
bool constexpr is_small_enough(){
	return sizeof(prefix_t) + sizeof(Q) + sizeof(std::array<key_element_t, Q::accompanying_key_n>) <= max;
}

} //key_value_pair_impl

struct KeyPrefix{
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
	void set_to_tombstone() {
		_is_constructed = 0;
		_type_id = tombstone;
	}
	void set_to_null(){
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

// prefix_t basically has to be KeyPrefix class above, or similar
template<typename prefix_t, typename key_element_t, typename ...Qs>
class key_value_pair{
	static const size_t cache_line_len_bytes = 64;

	static_assert(std::is_pod<key_element_t>::value, "array should be POD, though could possibly relax this.");

	static_assert(utils::all_of<key_value_pair_impl::is_small_enough<
								prefix_t, key_element_t, Qs, cache_line_len_bytes>()...>::value, 
								"one or more types do not fit in a cache line");

public:
	using self_t = key_value_pair<prefix_t, key_element_t, Qs...>; //convenience
	using prefix_t_ = prefix_t;
	using key_element_t_ = key_element_t; //note that this is the type within the array, not the array itself
	using dtor_t = void(*)(self_t&);
	using mvctor_t = void(*)(self_t&, self_t&&);
	using cpctor_t = void(*)(self_t&, self_t const&);

	
	static const size_t max_arr_len = (cache_line_len_bytes - sizeof(prefix_t))/sizeof(key_element_t);
	const static std::array<dtor_t, sizeof...(Qs)> dtor_vtable; //can't compile when this is constexpr, not sure whether there's a difference.
	const static std::array<mvctor_t, sizeof...(Qs)> mvctor_vtable; 
	const static std::array<cpctor_t, sizeof...(Qs)> cpctor_vtable; 
	const static std::array<size_t, sizeof...(Qs)> accompanying_key_n_table;


	prefix_t prefix; 
	std::array<uint8_t, cache_line_len_bytes - sizeof(prefix_t)> as_u8_array;

	key_value_pair() : prefix() {
		assert(prefix.is_null() && !prefix.is_constructed());
	}

	// construct from a copy of a key_value_pair
	key_value_pair(self_t const& other) {
		prefix = other.prefix;
		std::copy(other.as_u8_array.begin(), other.as_u8_array.end(), as_u8_array.begin());
		if(prefix.is_valid_type() && prefix.is_constructed())
			cpctor_vtable[prefix.type_id()](*this, other);
	}

	// construct by moving from (i.e. ripping the guts out of) another key_value_pair
	key_value_pair(self_t&& other) {
		prefix = other.prefix;
		std::copy(other.as_u8_array.begin(), other.as_u8_array.end(), as_u8_array.begin());
		if(prefix.is_valid_type() && prefix.is_constructed())
			mvctor_vtable[prefix.type_id()](*this, std::move(other));
	}

	~key_value_pair(){
		if(prefix.is_valid_type() && prefix.is_constructed())
			dtor_vtable[prefix.type_id()](*this);
	}

	// move-assign another key_value_pair to this one
	// we only allow this if this key_value_pair is in a tombstone/null state
	// otherwise this is identical to move-constructor.
	self_t& operator=(self_t&& other){
		assert(!prefix.is_valid_type());
		// TODO: assert(is_on_main_thread)
		prefix = other.prefix;
		std::copy(other.as_u8_array.begin(), other.as_u8_array.end(), as_u8_array.begin());
		if(prefix.is_valid_type() && prefix.is_constructed())
			mvctor_vtable[prefix.type_id()](*this, std::move(other));	
		return *this;
	}

	void destruct_to_tombstone(){
		// TODO: assert(is_on_main_thread);
		assert(prefix.main_thread_only_ref());

		if(prefix.is_valid_type() && prefix.is_constructed())
			dtor_vtable[prefix.type_id()](*this);
		prefix.set_to_tombstone();
	}

	void tombstone_to_null(){
		// TODO: assert(is_on_main_thread)
		assert(prefix.main_thread_only_ref());
		assert(prefix.is_tombstone());
		prefix.set_to_null();
	}

	void placement_new_key(typename prefix_t::type_id_t type_id_in, key_element_t const* begin,
						   key_element_t const* end){
		/* this lets you assing key/prefix without constructing the value, 
		this should only be called if the key_value_pair is in null/tombstone state.*/
		assert(!prefix.is_valid_type()); // otherwise we should destruct first
		assert(end - begin == accompanying_key_n_table[type_id_in]);		

		prefix.set_to_type_id(type_id_in);
		std::copy(begin, end, begin_key());
		assert(!prefix.is_constructed()); // we haven't constructed Q yet, so don't claim we have
	}

	template<typename Q, typename ...Args>
	void placement_new_value(Args&&... args){
		// this lets you create de-novo, copy/move construct from another Q
		// when the key/prefix were previously initialized
		// once you've called this you are then in the realm of copy/move/dtor
		// with vtables, but before you call this prefix.is_constructed() is false,
		// so no copy/move/dtor is invoked.
		assert(is_type<Q>());
		assert(!prefix.is_constructed());

		prefix.set_constructed(true); // this comes first to pass assertion in get<Q>
		new (& (get<Q>())) Q(std::forward<Args...>(args)... );
	}

	template<typename Q>
	bool is_type() const{
		return prefix.type_id() == utils::index_of_type<Q, Qs...>();
	}
	template<typename Q>
	Q& get(){
		assert(is_type<Q>() && prefix.is_constructed());
		size_t offset = sizeof(std::array<key_element_t, Q::accompanying_key_n>);
		return *reinterpret_cast<Q*>(&as_u8_array[offset]);
	}

	template<typename Q>
	Q const& cget() const {
		assert(is_type<Q>() && prefix.is_constructed());
		size_t offset = sizeof(std::array<key_element_t, Q::accompanying_key_n>);
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
		return reinterpret_cast<key_element_t*>(&as_u8_array[accompanying_key_n_table[prefix.type_id()]*sizeof(key_element_t)]);
	}
	key_element_t* begin_key(){
		return reinterpret_cast<key_element_t*>(&as_u8_array[0]);
	}
	key_element_t const* cend_key() const {
		return reinterpret_cast<key_element_t const*>(&as_u8_array[accompanying_key_n_table[prefix.type_id()]*sizeof(key_element_t)]);
	}
	key_element_t const* cbegin_key() const {
		return reinterpret_cast<key_element_t const*>(&as_u8_array[0]);
	}

};

// construct dtor_vtable
template<typename prefix_t, typename key_element_t, typename ...Qs>
const std::array<typename key_value_pair<prefix_t, key_element_t, Qs...>::dtor_t, sizeof...(Qs)> 
key_value_pair<prefix_t, key_element_t, Qs...>::dtor_vtable = {
&key_value_pair_impl::destroy_value<Qs, key_value_pair<prefix_t, key_element_t, Qs...>> ...};


// construct cpctor_vtable
template<typename prefix_t, typename key_element_t, typename ...Qs>
const std::array<typename key_value_pair<prefix_t, key_element_t, Qs...>::cpctor_t, sizeof...(Qs)> 
key_value_pair<prefix_t, key_element_t, Qs...>::cpctor_vtable = {
&key_value_pair_impl::copy_value<Qs, key_value_pair<prefix_t, key_element_t, Qs...>> ...};

// construct mvctor_vtable
template<typename prefix_t, typename key_element_t, typename ...Qs>
const std::array<typename key_value_pair<prefix_t, key_element_t, Qs...>::mvctor_t, sizeof...(Qs)> 
key_value_pair<prefix_t, key_element_t, Qs...>::mvctor_vtable = {
&key_value_pair_impl::move_value<Qs, key_value_pair<prefix_t, key_element_t, Qs...>> ...};

template<typename prefix_t, typename key_element_t, typename ...Qs>
const std::array<size_t, sizeof...(Qs)>
key_value_pair<prefix_t, key_element_t, Qs...>::accompanying_key_n_table = {Qs::accompanying_key_n...};

#endif // _KEY_VALUE_PAIR_H_