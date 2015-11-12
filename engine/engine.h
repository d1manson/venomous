
#include "key_value_pair.h"
#include "unordered_map.h"
#include "variable_width_contiguous_store.h"

/* engien_refs.h is really a part of this file, we just split it up to 
   keep individual files a bit easier to navigate for the reader. */
#include "engine_refs.h" 


template<typename E, E* engine_p>
class Dispatcher{
public:
	template<typename Q, typename ...Args>
	auto make_input(Args&& ...args){
		return E::template make_input<Q, engine_p>(std::forward<Args...>(args)...);
	}

	template<typename Q, typename State, void (*func_p)( KeyRef<E, engine_p, Q>, State), typename ...Args>
	auto make_callback(State state, Args ...args){
		return E::template make_callback<Q, engine_p, State, func_p, Args...>(state, args...);
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

	template<typename E, E* engine> friend class Dispatcher;
	template<typename E, E* engine_p, typename Q> friend class KeyRef;

	using callback_p_t = CallbackRefBaseA<self_t>*;

private:
	/*
		There are a whole bunch of different containers storing stuff.

		store - a hash-map of sorts, uses full-befores as keys, and actual
			    data as values. Keys and data are packed together into a
			    KVP which is aligned on cache-lines.  Provides some level
			    of thread-safe usage, but most interesting stuff has to
			    be done on main thread.

		user_ref_count - indices match up to store, counts number of
				 references on the user-side of the planet.  TODO: make
				 threading guarnaees.

		callbacks - holds the full-befores for callbacks. The user holds
				an RAII-ref to individual callbacks in here, i.e. whne 
				the user no longer cares about a given callback it will
				be removed from this container.  It is very fast to iterate
				over this container, and fairly fast to insert/delete.
				There are no special threading guarantees.
	*/
	store_t store;
	std::array<std::atomic<size_t>, store_capacity> user_ref_count; 
	VariableWidthContiguousStore<id_t, invalid_id, callback_p_t, 2, 4, 8, 16> callbacks;
public:
	using callback_ref_t = typename decltype(callbacks)::BucketRef;
	
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

private:

	void callback_now_at(callback_ref_t const& ref, callback_p_t cb_p){
		callbacks.set_extra(ref, cb_p);
	}

	template<typename Q, self_t* engine_p, typename State, void (*func_p)(KeyRef<self_t, engine_p, Q>, State), typename ...Args>
	static auto make_callback(State state, Args ...args){
		static_assert(sizeof...(Args) == Q::accompanying_key_n -1, 
					  "full-befores list is not the correct length");

		// TODO: accept refs rather than raw id_t's, and check they match the proper type for Q

		// add callback's full-befores to callbacks store...
		std::array<id_t, sizeof...(Args)> full_befores{args...};
		auto ref = engine_p->callbacks.insert(full_befores.cbegin(), full_befores.cend(), nullptr /*,
												, prefix_for<Q>() */);


		/* callbackRef will register a pointer to itself in engine_p->callbacks, and if it
			subsequently moves it's location will be updated. It is a subclass of CallbackRefBaseA
			and we can use dynamic dispatch to exec the callback on it properly.		*/
		return CallbackRef<self_t, engine_p, Q, State, func_p>(std::move(ref), state); 
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
			size_t v = --user_ref_count[idx]; // aqr_rel vs seq_const ?
			if(v == 0){
				//TODO: make this thread-safe, i.e. if not on main, post request to main
				store.delete_(prefix, begin, end); // TODO: strictly we could reuse idx here
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

	void run(){
		// eventually this will be called once, and start up the main
		// engine thread loop, but for now the user has to repeatedly
		// call it manually.
		std::cout << "------ RUN -----------------------------"  << std::endl;

		callbacks.for_each(
		[](id_t* begin, id_t* end){
			std::vector<id_t> v(begin, end);
			std::cout << "found callback registered for full-befores: " << v << std::endl;
			return true;
		},
		[](callback_p_t cb_p, auto begin, auto end){
			/*		
			// --- create dummy value in store and dummily-return it to callback --- //
			std::array<key_element_t, Q::accompanying_key_n> dummy_key;
			auto p = engine_p->store.insert(prefix_for<Q>(), dummy_key.begin(), dummy_key.end());
			assert(p != nullptr);
			Q::exec(*p);
			KeyRef<self_t, engine_p, Q> dummy_ref(dummy_key);
			func_p(dummy_ref);
			*/
			std::cout << "I'd love to call the callback pointed to: " << cb_p << "\n";
		});

		std::cout << "----------------------------------------"  << std::endl;

	}


	template<typename E, E* engine_p, typename Q, typename State, void (*func_p)( KeyRef<E, engine_p, Q>, State)>
	friend class CallbackRef;

	friend std::ostream& operator<<(std::ostream& os, self_t const& engine){
		os << "###### Engine #########\n\nWith store:\n============\n" 
		   << engine.store << "\n\nWith callbacks:\n===============\n"
		   << engine.callbacks << "\n##################\n"; 
		return os; 
	}

};


