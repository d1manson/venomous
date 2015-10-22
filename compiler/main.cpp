
#include <iostream>
#include <string>
#include <memory>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <iterator>
#include <type_traits>


#include "farmhash.h"

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


using id_t = size_t;

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
			
			static_assert(sizeof(char) == 1, "char was expected to be 1 byte");
			auto hashed = util::Hash32(reinterpret_cast<const char*>(key.data()), 
									   key.size() * sizeof(T)); 
			//std::cout << "hashed key " << key << " to " << hashed << std::endl;
			return hashed;
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

#define NDEBUG
#include "variant.h"

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

// Main loop
int main(int argc, char **argv)
{
	
	wector<int> w1;
	w1.push_back(23);
	w1.push_back(60);

	variant<uint16_t, float, wector<int>> dummy( w1 );
	w1.push_back(99);
	variant<uint16_t, float, wector<int>> dummy2( std::move(dummy) );

	auto& d2_vector_int = dummy2.get<wector<int> >();
	auto& d2_float = dummy2.get<float>();
	
	std::cout << "dummy2_float: " << d2_float 
			  << ", dummy2_vector_int: " << d2_vector_int <<  std::endl; //float is gibberish, int is 23
	

	return 0;
}