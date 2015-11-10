
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

#include "tmp_utils.h"


using id_t = uint32_t;


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

#include "engine.h"

struct input_a{
	static const auto accompanying_key_n = 1;

	const std::string value;
	template<typename ...Args>
	input_a(Args&& ...args) : value(std::forward<Args...>(args...)){};
};

struct custom_a : public vector<int>{
	static const auto accompanying_key_n = 3;
};

struct custom_b : public vector<float>{
	static const auto accompanying_key_n = 3;
};

struct custom_c : public vector<double>{
	static const auto accompanying_key_n = 1;

	template<typename KVP>
	static void exec(KVP& kvp){
		kvp.template placement_new_value<custom_c>();
		custom_c& ret = kvp.template get<custom_c>();
		ret.push_back(42);
		ret.push_back(24);
		// TODO: kvp.finalise();
	}

};


//#define NDEBUG
#include <chrono>
#include <random>
#include <atomic>




using engine_t = Engine<64, id_t, input_a, custom_a, custom_b, custom_c>;
engine_t engine;
using dispatcher_t = Dispatcher<engine_t, &engine>;
dispatcher_t dispatcher;

void got_c(typename dispatcher_t::callback_arg<custom_c>::type v){
	std::cout << "got: " << v.cget() << std::endl;
}


#include "variable_width_contiguous_store.h"

int main(int argc, char **argv){
	
	std::array<double, 8> dummy = {0.1, 0.2, 3.3, 4.4, 5.5};
	
	
	VariableWidthContiguousStore<double, 2, 4, 8, 16> cs;


	auto cs1 = cs.insert(dummy.begin(), dummy.begin()+1);
	auto cs2 = cs.insert(dummy.begin(), dummy.begin()+2);
	auto cs3 = cs.insert(dummy.begin(), dummy.begin()+4);

	std::cout << "cs1: " << cs1 << ",   cs2: " << cs2 << ",   cs3: " << cs3;
	std::cout << "\n==========\n";

	std::cout << cs << std::flush;
	cs.delete_(std::move(cs1)); // we don't actually want the user to call delete, it should be RAII on the ref
	std::cout << "\n==========\n";
	std::cout << cs << std::flush;	
	
	auto cs4 = cs.insert(dummy.begin()+3, dummy.begin()+5);
	auto cs5 = cs.insert(dummy.begin()+2, dummy.begin()+6);
	std::cout << "cs4: " << cs4 << ",   cs5: " << cs5 ;
	std::cout << "\n==========\n";
	std::cout << cs << std::flush;	

	cs.delete_(std::move(cs2)); // we don't actually want the user to call delete, it should be RAII on the ref
	std::cout << "\n==========\n";
	std::cout << cs << std::flush;	

	cs.delete_(std::move(cs3)); // we don't actually want the user to call delete, it should be RAII on the ref
	std::cout << "\n==========\n";
	std::cout << cs << std::flush;	
		
	cs.delete_(std::move(cs5)); // we don't actually want the user to call delete, it should be RAII on the ref
	std::cout << "\n==========\n";
	std::cout << cs << std::flush;	
	
	/*
	ContiguousStore<std::array<double,8>> cs;
	cs.insert(dummy);
	std::cout << cs << std::endl;
	*/

	/*
	cs.insert(dummy.begin(), dummy.begin()+5);
	cs.insert(dummy.begin(), dummy.begin()+6);
	cs.insert(dummy.begin(), dummy.begin()+7);
	cs.insert(dummy.begin(), dummy.begin()+8);
	cs.insert(dummy.begin(), dummy.begin()+9);
	cs.insert(dummy.begin(), dummy.begin()+12);
	cs.insert(dummy.begin(), dummy.begin()+16);
	*/
	/*
	auto cs1 = cs.insert(1.11);
	auto cs2 = cs.insert(2.002);
	auto cs3 = cs.insert(303.33);
	std::cout << cs << std::endl;
	cs.delete_(cs1);
	cs.delete_(cs2);
	auto cs4 = cs.insert(404.44);
	std::cout << cs << std::endl;
	*/

	/*
	auto r1 = dispatcher.make_input<input_a>("hello world");
	auto c1 = dispatcher.make_callback<custom_c>(got_c);

	std::cout << engine << std::endl;	
	std::cout << "r1: " << r1.cget().value << std::endl;  
	*/
	return 0;
}