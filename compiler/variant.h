/*

	Example usage:

		variant<uint16_t, int, double, char[6], float> dummy(3.2f);

		auto& d_float = dummy.get<float>();
		auto& d_float_other = dummy.get<float>();
		
		std::cout << "dummy_float: " << d_float << std::endl;
		d_float_other = 99.2f;
		std::cout << "dummy_float: " << d_float << std::endl;

		auto& d_int = dummy.get<int>(); // run-time assert fails, if NDEBUG is defined we continue...
		d_int = 23; // modify the value as though it were an int 
		std::cout << "dummy_int: " << d_int 
				  << ", dummy_float: " << d_float <<  std::endl; //float is gibberish, int is 23

	Another example:

		struct custom_t{
			float my_value;
			custom(){}
			~custom(){
				std::cout<< "destroying custom " << my_value <<  std::endl;
			}
		};

		variant<uint16_t, float, custom_t> dummy2( (custom_t()) ); // most vexing parse
		auto& d2_custom_t = dummy2.get<custom_t>();
		d2_custom_t.my_value = 808.333f;

		std::cout << "dummy2_custom_t.my_value: " << d2_custom_t.my_value << std::endl;
		auto& d2_float = dummy2.get<float>(); //run-time assert fails, if NDEBUG is defined we continue...
		d2_float = 45.2f; // modify the value as though it were a raw float (which happens to make sense here)
		std::cout << "dummy2_custom_t.my_value: " << d2_custom_t.my_value 
				  << ", dummy2_float: " << d2_float <<  std::endl; // prints the same float value twice
		// the destructor for custom_t is called when dummy2 goes out of scope

		variant<uint16_t, float, custom_t> dummy3(12.4f);
		// the destructor for float is called (NOP) when dummy3 goes out of scope

	
	And another example showing both kinds of move/copy constructors in action:

		template<typename T>
		struct wector : public std::vector<T>{
			static int counter;
			int my_idx;
			wector(){
				my_idx = counter++;
				std::cout << "constructed wector #" << my_idx << std::endl;
			}
			wector(wector<T>&& other) : std::vector<T>(std::move(other)) {
				my_idx = counter++;
				std::cout << "move-constructed vector #" << my_idx << ": " << *this << ", from vector #" << other.my_idx << ", which is now: " << other << std::endl;
			}
			wector(wector<T> const& other) : std::vector<T>(other) {
				my_idx = counter++;
				std::cout << "copy-constructed vector #" << my_idx << ": " << *this << ", from vector #" << other.my_idx << ", which is now: " << other << std::endl;
			}
			~wector(){
				std::cout << "destructing wector #" << my_idx << ": " << *this << std::endl;
			}
		};
		template<typename T>
		int wector<T>::counter = 0;

		wector<int> w1;
		w1.push_back(23);
		w1.push_back(60);

		// construct a variant from a wector...
		variant<uint16_t, float, wector<int>> dummy( w1 ); // can use std::move(w1) if you don't need w1 any more
		w1.push_back(99);

		// construct another variant from the first variant...
		variant<uint16_t, float, wector<int>> dummy2( dummy ); // can use std::move(dummy) if you don't need it any more 
		auto& d2_vector_int = dummy2.get<wector<int> >();
		auto& d2_float = dummy2.get<float>();
	
		std::cout << "dummy2_float: " << d2_float 
				  << ", dummy2_vector_int: " << d2_vector_int <<  std::endl; 


	Variant is a flag+union, where the flag type is given as the first template argument, and
	the types of the union are listed as the 2nd, 3rd,... arguments to the template.

	The flag is const, i.e. the true type of the variant is fixed at construction.  The flag
	is used to lookup the correct destructor in a vtable when the variant's destructor is called.
	When using variant.get<Q>(), a runtime error is thrown if flag doesn't match Q, but this check
	can be disabled if NDEBUG is #defined.

	Note that non-trivial constructors in a union is new in C++11.

	TODO: check that all the placement new stuff is guarnateed never to be called on memory
	that has been initialised but never destructued...if that happened then you'd leak 
	memory.
*/

#include <utility>
#include <cassert>


namespace variant_impl{


/* using the default param + template specialization trick, 
   you can do an if-branch with just a pair of structs/functions. */
template<typename Q, class VU, 
		  bool stop_recursion_flag = std::is_same<Q, typename VU::local_value_t>::value> 
struct get{ //stop_recursion_flag = true here, because we specialize for false below
 	Q& call(VU& vu) {
		return vu.local_value;
	}
};

template<typename Q, class VU>
struct get<Q, VU, false>{
 	Q& call(VU & vu) {
		return get<Q, typename VU::deeper_values_t>().call(vu.deeper_values);
	}
};

// above again, but now for const version...
template<typename Q, class VU, 
		  bool stop_recursion_flag = std::is_same<Q, typename VU::local_value_t>::value> 
struct cget{ //stop_recursion_flag = true here, because we specialize for false below
 	Q const& call(VU const& vu) const {
		return vu.local_value;
	}
};

template<typename Q, class VU>
struct cget<Q, VU, false>{
 	Q const& call(VU const& vu) const {
		return cget<Q, typename VU::deeper_values_t>().call(vu.deeper_values);
	}
};

template<typename T, typename ...S>
struct union_{
	union_(){};
	~union_(){};
	using local_value_t = T;
	using deeper_values_t = union_<S...>;
	union{
	T local_value;
	union_<S...> deeper_values;
	};
};

template<typename T>
struct union_<T>{
	union_(){};
	~union_(){};
	using local_value_t = T;
	T local_value;
};

// http://stackoverflow.com/a/27650981/2399799
template <typename T, typename U=void, typename... Types>
constexpr size_t index_of_type() {
    return std::is_same<T, U>::value ? 0 : 1 + index_of_type<T, Types...>();
}

// http://stackoverflow.com/a/26307877/2399799
template <typename T>
void destroy(T& t){
    t.~T();
}

template <typename T, std::size_t N>
void destroy(T (&t)[N]){
    for (auto i = N; i-- > 0;)
        destroy(t[i]);
}

template <typename T, typename VU>
void destroy_union_member(VU& vu){
	destroy(get<T, VU>().call(vu));
}


// building on destroy pattern, above, now with move...
// TODO: these functions seem to have the ambiguous call signature that std::forward likes, but
// we want them to accept specifically an rvalue reference. It may not matter since we are 
// inside the impl namespace, and they should only be called via variant.variant(variant&& other),
// which is a true rvalue call signature... so we should be safe?
template <typename T>
void move(T& dest, T&& other){
    new (&dest) T(std::move(other));
}

template <typename T, std::size_t N>
void move(T (&dest)[N], T (&&other)[N]){
    for (auto i = N; i-- > 0;)
        move(dest[i], std::move(other[i]));
}

template <typename T, typename VU>
void move_union_member(VU& dest, VU&& other){	
	move(get<T, VU>().call(dest), std::move(get<T, VU>().call(other)));
}

// as with destroy and move, now for copy...
template <typename T>
void copy(T& dest, T const& other){
    new (&dest) T(other);
}

template <typename T, std::size_t N>
void copy(T (&dest)[N], const T (& other)[N]){
    for (auto i = N; i-- > 0;)
        copy(dest[i], other[i]);
}

template <typename T, typename VU>
void copy_union_member(VU& dest, VU const& other){	
	copy(get<T, VU>().call(dest),
	     cget<T, VU>().call(other));
}

} // namespace variant_impl



template<typename flag_t, typename ...T>
class variant{
	using union_t = variant_impl::union_<T...>;
	using dtor_t = void(*)(union_t&);
	using mvctor_t = void(*)(union_t&, union_t&&);
	using cpctor_t = void(*)(union_t&, union_t const&);

	union_t union_;

	const static std::array<dtor_t, sizeof...(T)> dtor_vtable; //can't compile when this is constexpr, not sure whether there's a difference.
	const static std::array<mvctor_t, sizeof...(T)> mvctor_vtable; 
	const static std::array<cpctor_t, sizeof...(T)> cpctor_vtable; 

public:
	const flag_t flag;

	template<typename Q>
	bool flag_matches_type() const{
		return flag == variant_impl::index_of_type<Q, T...>();
	}

	template<typename Q>
	Q& get(){
		assert(flag_matches_type<Q>());
		return variant_impl::get<Q, union_t>().call(union_);
	}
	// same as above, but now const
	template<typename Q>
	Q const& cget(){
		assert(flag_matches_type<Q>());
		return variant_impl::cget<Q, union_t>().call(union_);
	}

	// construct new variant using either a COPY of a Q...
	template<typename Q>
	variant(Q& q) 
		: flag(variant_impl::index_of_type<Q, T...>()) {
		new (& get<Q>() ) Q(q); // placement new, i.e. copy construction into uninitialized union
	}
	// or by "(carefully) ripping the guts out of" a MOVEABLE Q...
	template<typename Q>
	variant(Q&& q) 
		: flag(variant_impl::index_of_type<Q, T...>()) {
		new (& get<Q>() ) Q(std::move(q)); // placement new, i.e. move construction into uninitialized union
	}

	// or by using a COPY of another **variant**
	variant(variant<flag_t, T...>& other)
		: flag(other.flag) {
			cpctor_vtable[flag](union_, other.union_);
	}

	// or by "(carefully) ripping the guts out of" a MOVEABLE **variant** 
	variant(variant<flag_t, T...>&& other)
		: flag(other.flag) {
			mvctor_vtable[flag](union_, std::move(other.union_));
	}
	~variant(){
		dtor_vtable[flag](union_);
	}
	
};

// consruct dtor_vtable
template<typename flag_t, typename ...T>
const std::array<typename variant<flag_t, T...>::dtor_t, sizeof...(T)> 
variant<flag_t, T...>::dtor_vtable = {&variant_impl::destroy_union_member<T, typename variant<flag_t, T...>::union_t>...};

// consruct mvctor_vtable
template<typename flag_t, typename ...T>
const std::array<typename variant<flag_t, T...>::mvctor_t, sizeof...(T)> 
variant<flag_t, T...>::mvctor_vtable = {&variant_impl::move_union_member<T, typename variant<flag_t, T...>::union_t>...};

// consruct cpctor_vtable
template<typename flag_t, typename ...T>
const std::array<typename variant<flag_t, T...>::cpctor_t, sizeof...(T)> 
variant<flag_t, T...>::cpctor_vtable = {&variant_impl::copy_union_member<T, typename variant<flag_t, T...>::union_t>...};
