
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
	static const auto accompanying_key_n = 4;

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


class NoState{};

void got_c(typename dispatcher_t::callback_arg<custom_c>::type v, NoState ns){
	std::cout << "got: " << v.cget() << std::endl;
	std::cout << engine << std::endl;	
}


int main(int argc, char **argv){
	
	if(true){
		auto r1 = dispatcher.make_input<input_a>("hello world");
		auto c1a = dispatcher.make_callback<custom_c, NoState, got_c>(
											NoState(), 13UL, 89UL, 10UL);
		std::cout << engine << std::endl;	


		std::cout << "r1: " << r1.cget().value << std::endl;  

		engine.run();
	}

	std::cout << engine << std::endl;	
		
	return 0;
}
