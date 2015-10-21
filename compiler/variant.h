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

	
	Variant is a flag+union, where the flag type is given as the first template argument, and
	the types of the union are listed as the 2nd, 3rd,... arguments to the template.

	The flag is const, i.e. the true type of the variant is fixed at construction.  The flag
	is used to lookup the correct destructor in a vtable when the variant's destructor is called.
	When using variant.get<Q>(), a runtime error is thrown if flag doesn't match Q, but this check
	can be disabled if NDEBUG is #defined.

*/

#include <cassert>


namespace variant_impl{


/* using the default param + template specialization trick, 
   you can do an if-branch with just a pair of structs/functions. */
template<typename Q, class VU, 
		  bool stop_recursion_flag = std::is_same<Q, typename VU::local_value_t>::value> 
struct get{ //stop_recursion_flag = true here, because we specialize for false below
 	Q& call(VU& vu){
		return vu.local_value;
	}
};

template<typename Q, class VU>
struct get<Q, VU, false>{
 	Q& call(VU& vu){
		return get<Q, typename VU::deeper_values_t>().call(vu.deeper_values);
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

} // namespace variant_impl



template<typename flag_t, typename ...T>
class variant{
	using union_t = variant_impl::union_<T...>;
	using func_t = void(*)(union_t&);

	union_t union_;

	const static std::array<func_t, sizeof...(T)> dtor_vtable; //can't compile when this is constexpr, not sure whether there's a difference.
	
public:
	const flag_t flag;

	template<typename Q>
	bool flag_matches_type(){
		return flag == variant_impl::index_of_type<Q, T...>();
	}

	template<typename Q>
	Q& get(){
		assert(flag_matches_type<Q>());
		return variant_impl::get<Q, union_t>().call(union_);
	}

	template<typename Q>
	variant(Q q) 
		: flag(variant_impl::index_of_type<Q, T...>()) {
		auto& dest = get<Q>();
		dest = q; // TODO: forwarding/moving etc?
	}

	~variant(){
		dtor_vtable[flag](union_);
	}

	//TODO: copy/move/assign etc.
	
};

// consruct dtor_vtable
template<typename flag_t, typename ...T>
const std::array<typename variant<flag_t, T...>::func_t, sizeof...(T)> 
variant<flag_t, T...>::dtor_vtable = {&variant_impl::destroy_union_member<T, typename variant<flag_t, T...>::union_t>...};

