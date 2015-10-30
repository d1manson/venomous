
#include <iostream>
#include <string>
#include <memory>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <iterator>
#include <type_traits>
#include <algorithm>

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


using id_t = uint32_t;

template<typename T, size_t... width_args>
class PostOffice{ // ignore the name for now, maybe can find something better later
private:

	template<size_t width>
	struct value{
		// TODO: make sure this struct is sensibly laid out, i.e. including the choice of widths and union stuff

		enum union_state : uint8_t{
			EMPTY=0,
			POD=1,
			VECTOR=2, // TODO: this is a problem...vector needs to be specialised properly otherwise we cant destruct contents etc.
			STRING=3 
		};
		const std::array<T, width> key;
		size_t ref_count = 0;
		union_state flag = EMPTY;
		union{

		};
		value(std::array<T, width> key_in) : key(key_in) {};
	};

 
	template<size_t width>
	struct values_equal{
		bool operator()(const value<width>& a, const value<width>& b) const{
			std::cout << "comparing keys " << a.key << " and " << b.key << std::endl;
			return std::equal(a.key.begin(), a.key.end(),
					          b.key.begin(), b.key.end());
		}
	};

	template<size_t width>
	struct hash{
		auto operator()(std::array<T, width> const& key) const{
			
			return 0;
		}
	};

	static constexpr std::array< size_t, 3> map_key_widths{width_args...};
	std::tuple<std::unordered_map<std::array<T, width_args>,
						          value<width_args>,
						          hash<width_args>,
						          values_equal<width_args> > ...> maps; 
	
public:
	template<size_t width>
	auto insert(std::array<T, width> key){
		// TODO: "loop" through width_args until you find width <= width_args[i]
		return insert_helper<0>(key);
	}

	template<size_t width>
	auto find(std::array<T, width> key){
		// TODO: "loop" through width_args until you find width <= width_args[i]
		return find_helper<0>(key);
	}

	PostOffice(){
		
			
	}

private:

	template<size_t map_idx, size_t width>
	auto insert_helper(std::array<T, width> key){
		// copy key into zero-padded array of length map_key_widths[0] or map_key_widths[1] etc.
		// TODO: if width == map_key_widths[map_idx] then we don't need zero padding

		std::array<T, std::get<map_idx>(map_key_widths)> padded_key;
		auto pad_start = std::copy(key.begin(), key.end(), padded_key.begin());
		std::fill(pad_start, padded_key.end(), 0); 

		return std::get<map_idx>(maps).emplace(std::make_pair(padded_key, padded_key));
	}

	template<size_t map_idx, size_t width>
	auto find_helper(std::array<T, width> key){
		// copy key into zero-padded array of length map_key_widths[0] or map_key_widths[1] etc.
		// TODO: if width == map_key_widths[map_idx] then we don't need zero padding

		std::array<T, std::get<map_idx>(map_key_widths)> padded_key;
		auto pad_start = std::copy(key.begin(), key.end(), padded_key.begin());
		std::fill(pad_start, padded_key.end(), 0); 

		return std::get<map_idx>(maps).find(padded_key);
	}

};


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

using std::vector;

struct custom_a : public vector<int>{
	static const auto accompanying_arr_n = 3;
};

struct custom_b : public vector<float>{
	static const auto  accompanying_arr_n = 3;
};

struct custom_c : public vector<double>{
	static const auto  accompanying_arr_n = 1;
};

struct cache_line_flag{
	using type_id_t = uint8_t;
	const type_id_t _type_id; 
	bool is_null() const{
		return _type_id == 0;
	}
	bool safe_to_read() const{
		return true; // TODO: implement thread-wise bit fields indicating read lock reference.
	}
	auto type_id() const{
		return _type_id - 1; // note that we +1 when setting and -1 when getting
						 // this means that we can use 0 for null.
	}
	cache_line_flag(type_id_t _type_id_in) 
					: _type_id(_type_id_in +1) {}
};

//#define NDEBUG
#include "packed_cache_line.h"


using cache_line = packed_cache_line<cache_line_flag, id_t, custom_a, custom_b, custom_c>;


// std::static_assert<sizeof(node)==64, "node should be 64bytes to match x86 cache line length.">

#include <chrono>
#include <random>
#include <atomic>

// Main loop
int main(int argc, char **argv)
{
	
	//std::cout << "sizeof(node): " << sizeof(node) << ", sizeof(vector<float>): " << sizeof(std::vector<float>) << std::endl;
	custom_b a1;
	std::array<id_t,3> ah = {34, 12, 45};
	//a1.push_back(23);
	//a1.push_back(60);
	cache_line cc(ah, std::move(a1));

	//a1.push_back(99);

	auto& cc_a1 = cc.get<custom_b>();
	//cc_a1.push_back(44);

	
	//std::cout << "accompanying_arr_n_table: " << cc.accompanying_arr_n_table << std::endl;


	auto BIG_N = 33554432-1; // 2^25-1
	std::vector<uint32_t> v;
	v.reserve(BIG_N);
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> D(0, BIG_N-1);
	for(int i=0; i< BIG_N; i++)
		v[i] = D(gen);
	std::cout << *std::max_element(v.begin(), v.end()) << " max\n";
	auto start_time = std::chrono::high_resolution_clock::now();
	uint32_t p = 0;
	for(uint32_t i=0; i< BIG_N; i++){
		p += v[i];//(p+i) & BIG_N];
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(
				end_time - start_time).count() / static_cast<float>(BIG_N) 
				<< "ns/iteration,  p=" << p << std::endl;
	/*
	uint32_t h = 0;
	for(int i=0;i<1000000;i++)
		h += cc.hash_for<custom_b>();

	std::cout << "sum=" << h;
	*/
	//ah[2] = 42;
	//auto& d2_vector_int = dummy2.get<wector<int> >();
	//auto& d2_float = dummy2.get<float>();
	
	/*
	std::cout << "dummy2_float: " << d2_float 
			  << ", dummy2_vector_int: " << d2_vector_int <<  std::endl; //float is gibberish, int is 23
	
	*/

	/*
	cache_line cc2( std::move(cc));
	auto& cc_a2 = cc2.get<custom_a>();
	cc_a2.push_back(817);
	cc_a1.push_back(404);
	auto& cc_ah = cc.get_array_before<custom_a>();
	auto& cc2_ah = cc2.get_array_before<custom_a>();

	if (std::equal(cc2_ah.begin(), cc2_ah.end(),
				cc.begin_array(), cc.end_array()))
		std::cout << "arrays are equal" << std::endl;
	
	std::cout << "cc_ah: " << cc_ah << std::endl;
	*/
    
	return 0;
}