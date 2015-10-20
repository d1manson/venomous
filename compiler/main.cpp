
#include <iostream>
#include <string>
#include <memory>
#include <cassert>
#include <unordered_map>
#include <tuple>
#include <iterator>
#include <type_traits>

#include "farmhash.h"

template <class T, std::size_t N>
std::ostream& operator<<(std::ostream& o, const std::array<T, N>& arr){
	// adapted from: http://stackoverflow.com/a/19152438/2399799
	o.put('[');
	if(arr.size() > 0){
	    std::copy(arr.cbegin(), arr.cend()-1, std::ostream_iterator<T>(o, ", "));
	    o << arr.back();	
	}
	o.put(']');
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



// Main loop
int main(int argc, char **argv)
{
	auto po = PostOffice<id_t, 2, 6, 14>();
	std::array<id_t, 2> x{3, 44};
	po.find(x);
	// TODO: check this is reasonably fast
	for(int i=0;i<32000;i++){
		x[0] = i;
		po.find(x);
	}
	std::cout << "hello world" << std::endl;
	return 0;
}