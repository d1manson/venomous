
#include <iostream>
#include <string>
#include <memory>
#include <cassert>


/*
axona_file_name is an input, so is just a wrapper around some const data.
*/
class axona_file_name {
public:
	axona_file_name(std::string value_in) : value(value_in) { }
	std::string get_value() const{
		return value;
	}
	using value_t = std::string;
private:
	const std::string value;
};


/*
	The boxed<T> provides a zero-indirection wrapper around 
	a constant, or wraps a shared_ptr to the actual value,
	which could itself be mutable or immutable.
*/
template<typename T>
class boxed{
public:
	boxed() : is_mutable(true),
	          is_ptr(true) {
		value_ptr = nullptr;
	}
	boxed(std::shared_ptr<T> value_in) : is_mutable(false),
										 is_ptr(true) { 
		value_ptr = value_in;
	}
	boxed(typename T::value_t value_in) : is_mutable(false),
									      is_ptr(false),
									      value_raw(value_in) {	}
	~boxed(){
		if(!is_ptr){
			// because of the union we need to explicity call the destructor of value_raw if it was in use
			value_raw.~T();
		}
	}
	bool get_is_mutable() const{
		return is_mutable; 
	}
	typename T::value_t get_value() const {
		assert(!is_ptr || value_ptr != nullptr);
		return is_ptr ? value_ptr->get_value() : value_raw.get_value();
	}
	void set_value(std::shared_ptr<T> value_in){
		assert(is_mutable && is_ptr);
		value_ptr = value_in;
	}
	void set_value(typename T::value_t value_in){
		assert(is_mutable &&  is_ptr);
		value_ptr = std::make_shared<T>(value_in);
	}
private:
	const bool is_mutable;
	const bool is_ptr; // this gaurds the union
	union{
		std::shared_ptr<T> value_ptr = nullptr;
		T value_raw; // note that we are assuming T is itself not that large, e.g. it holds a vector, or string would, but not an array or large structure
	};
};


template<typename T>
using boxed_shared_ptr = std::shared_ptr<boxed<T>>;

template<typename T, typename ...Args>
boxed_shared_ptr<T> make_shared_boxed(Args ...a){
	return std::make_shared<boxed<T>>(a...);
}

class axona_file{
public:
	axona_file(boxed_shared_ptr<axona_file_name> fn_in) : fn(fn_in){
	}
	std::string get_value() const{
		return std::string("[ contents of '") + fn->get_value() + "' ]";
	}
private:
	boxed_shared_ptr<axona_file_name> fn;
};




// Main loop
int main(int argc, char **argv)
{
	auto a1 = make_shared_boxed<axona_file_name>("my_file.txt");
	auto a2 = make_shared_boxed<axona_file_name>();

	axona_file b1(a1);
	axona_file b2(a2);

	std::cout << b1.get_value() << std::endl;
	a2->set_value("something.txt");
	std::cout << a2->get_value() << std::endl;
	std::cout << b2.get_value() << std::endl;
	a2->set_value("newer.txt");
	std::cout << a2->get_value() << std::endl;
	std::cout << b2.get_value() << std::endl;
	
	return 0;
}