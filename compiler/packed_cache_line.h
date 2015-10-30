/*

	This class is a special kind of variant/union.
	It stores a prefix followed by a std::array<array_t, N> and
	then some data.  The length of the array is specified by
	a static const on data, data::accompanying_arr_n.  
	You instatiate the template with a list of possible data types
	(each with their associated accompanying_arr_n value), then
	when you construct an instance of the class you provide one of
	the data types, together with an array of matching length.

	When you access the data, with .get<some_type>(), there is an assert
	to check the type of the variant matches, although this can be
	disabled with #NDEBUG. This same check is done when you access the
	array data using .get_array_before<some_type>().

	If you don't know the type being stored you can still get iterators
	 using begin_array() and end_array(), where  the second method
	has to lookup the length of the array for the current type in a
	table. (Note that if you are comparing to an array of known length
	using std::equal then you don't actually need the end iterator at all.)

	You can also call get_array_all_unsafe(), which will give a view of
	the stuff beyond the prefix as an array, even though the array is only
	valid up to a certain length for each of the various types.
	
	the data types must be copy (and ideally move) constructible, so
	that if the variant is copied/moved, the prefix can be used to lookup
	the relevant function in a special vtable.

	The prefix is basically an int, which is large enough to store the number
	N-1, where N is the number of types which can be stored in the variant.
	It exposes the int through a 1-arg constructor, and a const getter method, type_id().
	You can put additional stuff in the prefix if you like, but it will be ignored
	by the machienrary here.  The benefit of this is that you can get better
	utilization of the bits, e.g. using 8 bits for storing the type_id, and then
	using the other bits for cusotm prefixs.

	Note that although we came close to implementing this without
	reinterpret casts, but in the end it seemed impossible to guarnatee
	that the array portition of the variants lined up, because the 
	data types did not statisfy "standardLayout" requirements.  So it
	was not safe to access the array at the  head of the variant, even if
	we knew that all of the types in the variant began with an array.

	The name, packed_cache_line, is not great, but we do have a static 
	assert that verifies everything fits on a "cache line", hard-coded
	as 64bytes.
*/

#include <array>
#include <utility>
#include <cassert>
#include <type_traits>



namespace packed_cache_line_impl{

template <typename Q, typename PCL>
void destroy_member(PCL& pcl){
	pcl. template get<Q>().~Q();
}

template <typename Q, typename PCL>
void move_member(PCL& dest, PCL&& other){
	auto& dest_arr = dest. template get_array_before<Q>();
	auto& other_arr = other. template cget_array_before<Q>();
	std::copy(other_arr.begin(), other_arr.end(), dest_arr.begin());

	auto& destq = dest. template get<Q>();
	auto& otherq = other. template get<Q>();
	new (&destq) Q(std::move(otherq)); // placement new, i.e. move construction into uninitialized union
}

template <typename Q, typename PCL>
void copy_member(PCL& dest, PCL const& other){
	auto& dest_arr = dest. template get_array_before<Q>();
	auto& other_arr = other. template cget_array_before<Q>();
	std::copy(other_arr.begin(), other_arr.end(), dest_arr.begin());

	auto& destq = dest. template get<Q>();
	auto& otherq = other. template cget<Q>();
	new (&destq) Q(otherq); // placement new, i.e. copy construction into uninitialized union
}

// http://stackoverflow.com/a/27650981/2399799
template <typename T, typename U=void, typename... Types>
constexpr size_t index_of_type() {
    return std::is_same<T, U>::value ? 0 : 1 + index_of_type<T, Types...>();
}

// http://stackoverflow.com/a/28253503/2399799
template<bool...> struct bool_pack;
template<bool...values> struct all_of 
    : std::is_same<bool_pack<values..., true>, bool_pack<true, values...>>{};


template<typename prefix_t, typename array_t, typename Q, size_t max>
bool constexpr is_small_enough(){
	return sizeof(prefix_t) + sizeof(Q) + sizeof(std::array<array_t, Q::accompanying_arr_n>) <= max;
}

} //packed_cache_line_impl

template<typename prefix_t, typename array_t, typename ...Qs>
class packed_cache_line{
	static const size_t cache_line_len_bytes = 64;

	static_assert(std::is_pod<array_t>::value, "array should be POD, though could possibly relax this.");

	static_assert(packed_cache_line_impl::all_of<packed_cache_line_impl::is_small_enough<
												prefix_t, array_t, Qs, cache_line_len_bytes>()...>::value, 
												"one or more types do not fit in a cache line");

public:
	using self_t = packed_cache_line<prefix_t, array_t, Qs...>; //convenience
	using prefix_t_ = prefix_t;
	using array_t_ = array_t; //note that this is the type within the array, not the array itself
	using dtor_t = void(*)(self_t&);
	using mvctor_t = void(*)(self_t&, self_t&&);
	using cpctor_t = void(*)(self_t&, self_t const&);

	
	static const size_t max_arr_len = (cache_line_len_bytes - sizeof(prefix_t))/sizeof(array_t);
	
	const static std::array<dtor_t, sizeof...(Qs)> dtor_vtable; //can't compile when this is constexpr, not sure whether there's a difference.
	const static std::array<mvctor_t, sizeof...(Qs)> mvctor_vtable; 
	const static std::array<cpctor_t, sizeof...(Qs)> cpctor_vtable; 
	const static std::array<size_t, sizeof...(Qs)> accompanying_arr_n_table;


	prefix_t prefix; //must have a .type_id() method that returns the value passed to the constructor. Must be POD.
	std::array<uint8_t, cache_line_len_bytes - sizeof(prefix_t)> as_u8_array;

	// construct from copy of a Q
	template<typename Q>
	packed_cache_line(std::array<array_t, Q::accompanying_arr_n>& arr, Q const& q) 
					: prefix(packed_cache_line_impl::index_of_type<Q, Qs...>()) {		
		get_array_before<Q>() = arr; // copy array into first section of bytes (after prefix_t)
		new (& (get<Q>())) Q(q); //copy construct q inplace, in the bytes after the initial array
	}

	// construct from a moved Q
	template<typename Q>
	packed_cache_line(std::array<array_t, Q::accompanying_arr_n>& arr, Q&& q) 
					: prefix(packed_cache_line_impl::index_of_type<Q, Qs...>()) {		
		get_array_before<Q>() = arr; // copy array into first section of bytes (after prefix_t)
		new (& (get<Q>())) Q(std::move(q)); //move construct q inplace, in the bytes after the initial array
	}

	// construct from a copy of a packed_cache_line
	packed_cache_line(self_t const& other) 
					: prefix(other.prefix.type_id()) {
		cpctor_vtable[prefix.type_id()](*this, other);
	}

	// construct by moving from (i.e. ripping the guts out of) another packed_cache_line
	packed_cache_line(self_t&& other) 
					: prefix(other.prefix.type_id()) {
		mvctor_vtable[prefix.type_id()](*this, std::move(other));
	}


	~packed_cache_line(){
		dtor_vtable[prefix.type_id()](*this);
	}

	template<typename Q>
	bool is_type() const{
		return prefix.type_id() == packed_cache_line_impl::index_of_type<Q, Qs...>();
	}
	template<typename Q>
	Q& get(){
		assert(is_type<Q>());
		size_t offset = sizeof(std::array<array_t, Q::accompanying_arr_n>);
		return *reinterpret_cast<Q*>(&as_u8_array[offset]);
	}

	template<typename Q>
	Q const& cget() const {
		assert(is_type<Q>());
		size_t offset = sizeof(std::array<array_t, Q::accompanying_arr_n>);
		return *reinterpret_cast<Q const*>(&as_u8_array[offset]);
	}

	template<typename Q>
	std::array<array_t, Q::accompanying_arr_n>& get_array_before(){
		assert(is_type<Q>());
		return *reinterpret_cast<std::array<array_t, Q::accompanying_arr_n>*>(&as_u8_array[0]);	
	}

	template<typename Q>
	std::array<array_t, Q::accompanying_arr_n> const& cget_array_before() const{
		assert(is_type<Q>());
		return *reinterpret_cast<std::array<array_t, Q::accompanying_arr_n> const*>(&as_u8_array[0]);	
	}

	array_t* end_array(){
		return reinterpret_cast<array_t*>(&as_u8_array[accompanying_arr_n_table[prefix.type_id()]*sizeof(array_t)]);
	}
	array_t* begin_array(){
		return reinterpret_cast<array_t*>(&as_u8_array[0]);
	}
	array_t const* cend_array() const {
		return reinterpret_cast<array_t const*>(&as_u8_array[accompanying_arr_n_table[prefix.type_id()]*sizeof(array_t)]);
	}
	array_t const* cbegin_array() const {
		return reinterpret_cast<array_t const*>(&as_u8_array[0]);
	}


	// returns the whole array. This is useful, but unsafe as
	// you can not read past the end of the length defined by Q::accompanying_arr_n,
	// but the caller is responsible for discovering this value somehow.
	std::array<array_t, max_arr_len> const& get_array_all_unsafe(){
		return *reinterpret_cast<std::array<array_t,max_arr_len>*>(&as_u8_array[0]);	
	}

};

// construct dtor_vtable
template<typename prefix_t, typename array_t, typename ...Qs>
const std::array<typename packed_cache_line<prefix_t, array_t, Qs...>::dtor_t, sizeof...(Qs)> 
packed_cache_line<prefix_t, array_t, Qs...>::dtor_vtable = {
&packed_cache_line_impl::destroy_member<Qs, packed_cache_line<prefix_t, array_t, Qs...>> ...};


// construct cpctor_vtable
template<typename prefix_t, typename array_t, typename ...Qs>
const std::array<typename packed_cache_line<prefix_t, array_t, Qs...>::cpctor_t, sizeof...(Qs)> 
packed_cache_line<prefix_t, array_t, Qs...>::cpctor_vtable = {
&packed_cache_line_impl::copy_member<Qs, packed_cache_line<prefix_t, array_t, Qs...>> ...};

// construct mvctor_vtable
template<typename prefix_t, typename array_t, typename ...Qs>
const std::array<typename packed_cache_line<prefix_t, array_t, Qs...>::mvctor_t, sizeof...(Qs)> 
packed_cache_line<prefix_t, array_t, Qs...>::mvctor_vtable = {
&packed_cache_line_impl::move_member<Qs, packed_cache_line<prefix_t, array_t, Qs...>> ...};

template<typename prefix_t, typename array_t, typename ...Qs>
const std::array<size_t, sizeof...(Qs)>
packed_cache_line<prefix_t, array_t, Qs...>::accompanying_arr_n_table = {Qs::accompanying_arr_n...};