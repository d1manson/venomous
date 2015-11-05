
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

struct custom_a : public vector<int>{
	static const auto accompanying_key_n = 3;
};

struct custom_b : public vector<float>{
	static const auto accompanying_key_n = 3;
};

struct custom_c : public vector<double>{
	static const auto accompanying_key_n = 1;
};


//#define NDEBUG
#include "key_value_pair.h"


using kvp_t = KeyValuePair<KeyValueHeader, id_t, custom_a, custom_b, custom_c>;
static_assert(sizeof(kvp_t) <= 2* kvp_t::minimum_alignment, "KVP is larger than 2 cache lines");

#include <chrono>
#include <random>
#include <atomic>
#include "unordered_map.h"

unordered_map<kvp_t, 64> map;

int main(int argc, char **argv)
{
	

	//std::cout << "sizeof(node): " << sizeof(node) << ", sizeof(vector<float>): " << sizeof(std::vector<float>) << std::endl;
	custom_a a1;
	a1.push_back(23);
	
	for(uint32_t k=0; k<50; k++){
		std::array<id_t, 3> h = {k, 18, 49};
		kvp_t* cc_p = map.insert(0, h.begin(), h.end());
		assert(cc_p != nullptr);	
		if(k==5){
			cc_p->placement_new_value<custom_a>(std::move(a1));
			cc_p->get<custom_a>().push_back(81);
		}
	}

	std::array<id_t, 3> h_1a = {5, 18, 49};
	std::array<id_t, 1> h_2c = {19};
	std::array<id_t, 1> h_3c = {20};
		
	kvp_t* cc_p = map.find(0, h_1a.begin(), h_1a.end(), 0);
	assert(cc_p != nullptr);
	kvp_t& cc1 = *cc_p;

	std::cout << map;

	cc_p = map.insert(2, h_2c.begin(), h_2c.end());
	assert(cc_p != nullptr);
	kvp_t& cc2 = *cc_p;
	
	cc_p = map.insert(2, h_3c.begin(), h_3c.end());
	assert(cc_p != nullptr);
	kvp_t& cc3 = *cc_p;

	std::cout << map;
	
	std::cout << static_cast<size_t>(cc1.key_prefix()) << "\n";
	a1.push_back(99);
	
	auto& cc1_a = cc1.get<custom_a>();
	cc1_a.push_back(44);


	/*
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
	*/




	if (std::equal(cc1.begin_key(), cc1.end_key(), h_1a.begin(), h_1a.end()))
		std::cout << "keys are equal" << std::endl;
	
	std::cout << "cc1_a: " << cc1_a << std::endl;
	std::cout << "a1: " << a1 << std::endl;
	

	for(uint32_t k=0; k<10; k++){
		std::array<id_t, 3> h = {(k+4)%50, 18, 49};
		map.delete_(0, h.begin(), h.end());
	}
	std::cout << "after minor delete \n" << map << std::flush;

	map.attempt_clear_tombstones();

	std::cout << "after cleanup\n" << map;

	for(uint32_t k=10; k<50; k++){
		std::array<id_t, 3> h = {(k+4)%50, 18, 49};
		map.delete_(0, h.begin(), h.end());
	}

	
	std::cout << "after major delete\n" << map;
	map.attempt_clear_tombstones();
    std::cout << "after cleanup\n" << map;

	return 0;
}