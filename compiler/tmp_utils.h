/*
	The name is a bit misleading now, it's partly TMP utils, and partly misc utils.
*/

#ifndef _TMP_UTILS_H_
#define _TMP_UTILS_H_

template <class T, std::size_t N>
std::ostream& operator<<(std::ostream& o, const std::array<T, N>& arr){
	// adapted from: http://stackoverflow.com/a/19152438/2399799
	o << "arr[";
	if(arr.size() > 0){
	    std::copy(arr.cbegin(), arr.cend()-1, std::ostream_iterator<T>(o, ", "));
	    o << arr.back();	
	}
	o << ']';
	return o;
}

template <class T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& vec){
	// adapted from: http://stackoverflow.com/a/19152438/2399799
	o << "vec[";
	if(vec.size() > 0){
	    std::copy(vec.cbegin(), vec.cend()-1, std::ostream_iterator<T>(o, ", "));
	    o << vec.back();	
	}
	o << ']';
	return o;
}


namespace utils{

/*
	conditional_value usage:

	const bool flag = true;
	std::cout << "The following is an int of 3 if flag is true or float of 2.4 if false:\n";
	std::cout << utils::conditional_value<flag>()(3, 2.4) << std::endl;

	This can be useful when providing a function with explictly templated return type, eg:

	template<return_type=Thing*>
	return_type get_thing(){
		// some complex logic to obtain found_idx
		// ...
		return conditional_value<std::is_same(return_type, Thing*)>()(
											&store[found_idx], found_idx);

		// note that the rough pythonic equivalent is:
		//    return &store[found_idx] if isinstance(return_type, Thing*) else found_idx
		// but that doesn't work as a C++ ternary-if because the expersion doesn't evaluate to a single type.
	}

	See also std::conditional, which returns type not value.
*/
template<bool test=false>
struct conditional_value{
	template<typename T1, typename T2>
	T2 operator()(T1 true_value, T2 false_value){ 
		return false_value;
	}
};

template<>
struct conditional_value<true>{
	template<typename T1, typename T2>
	T1 operator()(T1 true_value, T2 false_value){ 
		return true_value;
	}
};




//========================

bool constexpr is_pow_2(int N){
	return (N && (N & (N - 1)) == 0);
}

// ======================

// http://stackoverflow.com/a/27650981/2399799
template <typename T, typename U=void, typename... Types>
constexpr size_t index_of_type() {
    return std::is_same<T, U>::value ? 0 : 1 + index_of_type<T, Types...>();
}

// http://stackoverflow.com/a/28253503/2399799
template<bool...> struct bool_pack;
template<bool...values> struct all_of 
    : std::is_same<bool_pack<values..., true>, bool_pack<true, values...>>{};

}

#endif // _TMP_UTILS_H_