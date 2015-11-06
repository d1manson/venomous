
#include "key_value_pair.h"
#include "unordered_map.h"


/* Note that engine_p is a global, this means we
	   (a) don't need to store a poitner to it, and..
   	   (b) don't need to worry about the engine itself going out of scope. */
template<typename E, E* engine_p, typename Q>
class KeyRef{
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

template<typename E, E* engine_p, typename Q>
class CallbackRef{
	using self_t = CallbackRef<E, engine_p, Q>;
	static const auto q_prefix = E::template prefix_for<Q>();
};



template<typename E, E* engine_p>
class Dispatcher{
public:
	template<typename Q, typename ...Args>
	auto make_input(Args&& ...args){
		return E::template make_input<Q, engine_p>(std::forward<Args...>(args)...);
	}

	template<typename Q, typename ...Args>
	auto make_callback(void (*func_p)( KeyRef<E, engine_p, Q>),
						Args&& ...args){
		return E::template make_callback<Q, engine_p, Args...>(
								func_p, std::forward<Args...>(args)...);	
	}

	// A convenience template
	template<typename Q>
	struct callback_arg{ using type = KeyRef<E, engine_p, Q>; };

};


template<size_t store_capacity, typename id_t, typename ...Qs>
class Engine{

public:
	using kvp_t = KeyValuePair<KeyValueHeader, id_t, Qs...>;
	static_assert(sizeof(kvp_t) <= 2* kvp_t::minimum_alignment, 
					"KVP is larger than 2 (cache line) units.");

	using store_t = unordered_map<kvp_t, store_capacity>;
	using self_t = Engine<store_capacity, id_t, Qs...>;
	using key_element_t = id_t;
	using key_prefix_t = typename kvp_t::key_prefix_t;

	static const id_t invalid_id = -1;
	std::array<id_t, sizeof...(Qs)> next_id_for_type;

	store_t store;
	template<typename E, E* engine> friend class Dispatcher;
	template<typename E, E* engine_p, typename Q> friend class KeyRef;

private:
	std::array<std::atomic<size_t>, store_capacity> user_ref_count;

	template<typename Q, self_t* engine_p, typename ...Args>
	static auto make_input(Args&& ...args){
		auto prefix = prefix_for<Q>();
		auto id = engine_p->next_id_for_type[prefix]++;
		std::array<id_t, 1> key{id};
		auto p = engine_p->store.insert(prefix, key.cbegin(), key.cend());
		assert(p != nullptr);
		p->template placement_new_value<Q>(std::forward<Args...>(args)...);
		return KeyRef<self_t, engine_p, Q>(key);
	}

	template<typename Q, self_t* engine_p, typename ...Args>
	static auto make_callback(void (*func_p)(KeyRef<self_t, engine_p, Q>),
							  Args& ...args){
		/* Takes a pointer to a callback function, which should accept a
		   single argument of type KeyRef<..., Q>. The Args here give the
		   full befores from which to construct Q, ultimately they will be 
		   allowed to be fixed, variables, or "pointers".  */

		// TODO: store func_p somewhere, count refs for callback and actually do something.

		// --- create dummy value in store and dummily-return it to callback --- //
		std::array<key_element_t, Q::accompanying_key_n> dummy_key;
		auto p = engine_p->store.insert(prefix_for<Q>(), dummy_key.begin(), dummy_key.end());
		assert(p != nullptr);
		Q::exec(*p);
		KeyRef<self_t, engine_p, Q> dummy_ref(dummy_key);
		func_p(dummy_ref);

		return CallbackRef<self_t, engine_p, Q>();
	}

	template<int delta>
	void user_ref_counter_delta(key_prefix_t prefix, key_element_t const* begin,
								 key_element_t const* end){
		/* This can be called both from main-thread and from user-thread(s) */
		static_assert(delta == -1 || delta == +1, "delta should be +-1");

		auto idx = store.template find<size_t>(prefix, begin, end /* TODO: ,
												 user_thread_id or main_thread_id */);
		assert(idx != store_t::invalid_index);

		if(delta == +1){
			user_ref_count[idx]++;
		}else{
			size_t v = user_ref_count[idx]--; // aqr_rel vs seq_const ?
			if(v == 0){
				//TODO: post message to main thread something interesting?
			}
		}
	}

	template<typename Q>
	Q const& cget_value(std::array<key_element_t, Q::accompanying_key_n> key){
		auto p = store.find(prefix_for<Q>(), key.cbegin(), key.cend());
		assert(p != nullptr);
		return p->template cget<Q>();
	}

public:
	template<typename Q>
	constexpr static auto prefix_for(){ //convenience
		return kvp_t::template prefix_for<Q>();
	}

	friend std::ostream& operator<<(std::ostream& os, self_t const& engine){
		os << "Engine with store:\n" << engine.store << "\n"; 
		return os; 
	}


};


