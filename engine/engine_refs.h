/* this file is part of engine.h, we just put it here to keep individual files
	a bit easier to navigate for the reader. */


/* Note that engine_p is a global, this means we
	   (a) don't need to store a poitner to it, and..
   	   (b) don't need to worry about the engine itself going out of scope. */
template<typename E, E* engine_p, typename Q>
class KeyRef{
	/* The KeyRef class is for the user to hold a read-lock
	  on a KVP and to provide typed-access to the data.  

	  It is used for holding references to input-Qs, and 
	  as the argument type for callbacks of Q.	*/

	using key_element_t = typename E::key_element_t;
	using self_t = KeyRef<E, engine_p, Q>;
	using key_t = std::array<key_element_t, Q::accompanying_key_n>;
	key_t key;
	static const auto q_prefix = E::template prefix_for<Q>();
public:
	KeyRef(key_t key_in) :
			key(key_in){
		engine_p->template user_ref_counter_delta<+1>(q_prefix, key.cbegin(), key.cend());
	}
	KeyRef(key_element_t const* begin, key_element_t const* end) {
		std::copy(begin, end, key.begin());
		engine_p->template user_ref_counter_delta<+1>(q_prefix, key.cbegin(), key.cend());
	}
	// move constructor
	KeyRef(self_t&& other) :
			key(other.key) {
		other.key[0] = E::invalid_id; // avoid increment/decrement 
	}
	// copy constructor
	KeyRef(self_t const& other) :
			key(other.key) {
		engine_p->template user_ref_counter_delta<+1>(q_prefix, key.cbegin(), key.cend());
	}
	~KeyRef(){
		if(key[0] != E::invalid_id)
			engine_p->template user_ref_counter_delta<-1>(q_prefix, key.cbegin(), key.cend());
	}
	Q const& cget(){
		return engine_p->template cget_value<Q>(key);
	}
};



template<typename E>
class CallbackRefBaseA {
	/* This base class makes it possible to do 
		dynamic dispatch on exec.	*/
protected:
	virtual ~CallbackRefBaseA(){};
public:
	virtual void exec(typename E::key_element_t const* begin,
				 typename E::key_element_t const* end) = 0;
};

template<typename E, E* engine_p>
class CallbackRefBaseB :
public CallbackRefBaseA<E> {
	/* This adds to the first base, now including
	  the ability to hold a reference to somewhere 
	  within a global instance of engine. */
	using self_t = CallbackRefBaseB<E, engine_p>;
public:
	using ref_t = typename E::callback_ref_t; 
private:
	ref_t ref;
protected:
	ref_t const& get_ref() const {return ref;};
	CallbackRefBaseB(ref_t&& ref_in)
				: ref(std::move(ref_in)) {};
	CallbackRefBaseB(self_t&& other) = default;
	~CallbackRefBaseB() override {};
};



template<typename E, E* engine_p, typename Q, typename State, void (*func_p)( KeyRef<E, engine_p, Q>, State)>
class CallbackRef
 : public CallbackRefBaseB<E, engine_p> {
 	/* This  builds on the  two bases, now
 	   accepting a user callback, which we can provide 
 	   type info to as well as holding, and providing state. 	*/
 	using base_t = CallbackRefBaseB<E, engine_p>;
	using self_t = CallbackRef<E, engine_p, Q, State, func_p>;
	static const auto q_prefix = E::template prefix_for<Q>();
	State state;
public:
	CallbackRef(typename base_t::ref_t&& ref_in, State state_in) 
				: base_t(std::move(ref_in)),
				  state(state_in) {
				  	engine_p->callback_now_at(base_t::get_ref(), this);
				  };
	CallbackRef(self_t&& other) 
				: base_t(std::move(other)),
				  state(std::move(other.state)) {
					engine_p->callback_now_at(base_t::get_ref(), this);
				};
	~CallbackRef() override{
		engine_p->callback_now_at(base_t::get_ref(), nullptr);
	};
	void exec(typename E::key_element_t const* begin,
				 typename E::key_element_t const* end) override{
		func_p({begin, end}, state); //provide type info and state to user, yay!
	}
};

