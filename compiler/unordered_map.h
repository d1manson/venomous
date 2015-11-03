/*
	unordered_map class

	A fixed-capactiy hash table, that uses quadratic probing for collisions.
	Indiviudal key-value pairs are stored contiguously in a special class
	called KVP.

	These KVPs are then kept in an array called store. A secondary
	array called extra_storage_info holds some additional details to aid deletion
	cleanup (and to some extent insertion).

	Both the store and the extra_storage_info record whether a slot in the hash 
	table is null/tombstone/valid.  If this data is not kept in sync then we will 
	be generating hard to track down bugs.

	Only the main thread can insert and delete, and only the main thread has any access
	to the extra_storage_info.

	See key_value_pair class for the required interface of KVP.

	Worker threads (i.e. not main thread) are allowed to find(..), but may get a 
	spurious nullptr if there is no thread-specific lock on the given KVP.

	Only the main thread may obtain locks, and only worker threads can relase (their
	respective) locks.

*/

#include <memory>
#include <array>
#include <cstdint>

#include "utils/murmur3.h"
#include "tmp_utils.h"

#define HASH_NAMESPACE murmur3


struct ExtraStorageInfo{
public:
	static const size_t null_kvp = -1;
	static const size_t tombstone_kvp = -2;
	size_t probes_passing_through = 0; // the number of entries in the hash table that probed this kvp in order to reach their final insertion destination	
private:
	size_t flag_or_probes_used = null_kvp; // the original index to which this kvp was hashed, from which any probing would have started
public:
	bool is_null() const {
		return flag_or_probes_used == null_kvp;
	}
	bool is_tombstone() const {
		return flag_or_probes_used == tombstone_kvp;
	}
	bool is_valid_probes_used() const {
		return !(flag_or_probes_used == null_kvp || flag_or_probes_used == tombstone_kvp);
	}
	void set_to_null(){
		flag_or_probes_used = null_kvp;
	}	
	void set_to_tombstone() {
		flag_or_probes_used = tombstone_kvp;
	}
	void set_to_probes_used(size_t probes_used){
		assert(probes_used != null_kvp && probes_used != tombstone_kvp);
		flag_or_probes_used = probes_used;
	}
	size_t probes_used() const{
		assert(is_valid_probes_used());
		return flag_or_probes_used;
	}
};

template<typename KVP, size_t capacity>
class unordered_map{
	static_assert(utils::is_pow_2(capacity), "capacity should be pow 2.");

public:
	using self_t = unordered_map<KVP, capacity>;  //convenience
	using prefix_t = typename KVP::prefix_t_;
	using key_element_t = typename KVP::key_element_t_;
	using prefix_type_id_t = typename prefix_t::type_id_t;
	static const auto capacity_ = capacity;
	static const size_t invalid_index = -1;

private:
	// these large arrays might be better off on the heap, with unique ptrs
	std::array<KVP, capacity> store;
	std::array<ExtraStorageInfo, capacity> extra_storage_info;
	const size_t max_probing = 30; // total number of attempts to read from store before giving up
								   // i.e. if 1, then just read at hashed location with no quadartic probing.
	size_t tombstone_count = 0;
	size_t valid_count = 0;
	const size_t main_thread_id = 0;

	static auto modulo_capacity(size_t x) {
		return (capacity-1) & x;
	}

	static auto probe_i_from(size_t base_idx, size_t i){
		return modulo_capacity(base_idx + i*i);
	}
	static auto base_from_probe_i(size_t probe_idx, size_t i){
		// inverse of probe_i_from
		return modulo_capacity(probe_idx - i*i);
	}
	static auto get_hashed_idx(prefix_type_id_t prefix_type_id,
						     key_element_t const* begin, key_element_t const* end) {
		// returns the index into store.
		// key_element_t must be contiguous
		return modulo_capacity(HASH_NAMESPACE::hash<key_element_t>(
								begin, end-begin, prefix_type_id));
	}

public:

	KVP* insert(prefix_type_id_t prefix_type_id,
		          key_element_t const* begin, key_element_t const* end){
		// TODO: assert(is_on_main_thread);
		assert(find(prefix_type_id, begin, end, main_thread_id) == nullptr);

		auto base_idx = get_hashed_idx(prefix_type_id, begin, end);

		// do 1 or more probing steps, to find the resting place for this kvp
		for(size_t i=0; i<max_probing; i++){
			size_t idx = probe_i_from(base_idx, i);

			if(extra_storage_info[idx].is_null() || extra_storage_info[idx].is_tombstone()){
				// probe i was successfull
				// note that if it was a tombstone, then its probes_passing_through may be >0.
				assert(!store[idx].prefix.is_valid_type());

				extra_storage_info[idx].set_to_probes_used(i);
				valid_count++;
				KVP& kvp = store[idx];
				kvp.placement_new_key(prefix_type_id, begin, end);
				return &kvp;
			}else{
				// probe i was unsuccessful, do more probing
				extra_storage_info[idx].probes_passing_through++; 
			}

		}

		abort(); // TODO: delete kvps/compact table, or increase max_probing???
		//return nullptr;
	}


	template<typename return_type=KVP*>
	return_type find(prefix_type_id_t prefix_type_id, key_element_t const* begin,
					 key_element_t const* end, size_t thread_id){
		/* Can return and KVP* /nullptr or  storage idx (i.e. size_t)/ExtraStorageInfo::null_kvp
		  Note that only main thread should have any business asking for size_t version, but
		  it's actually safe to use this in principle on other threads. 		 */

		const bool return_as_size_t = std::is_same<size_t, return_type>::value;

		auto base_idx = get_hashed_idx(prefix_type_id, begin, end);

		for(size_t i=0; i<max_probing; i++){
			size_t idx = probe_i_from(base_idx, i);
			KVP& kvp = store[idx];
			if(kvp.prefix.is_null())
				return utils::conditional_value<return_as_size_t>()(invalid_index, nullptr);

			if(!kvp.prefix.safe_to_read(thread_id))
				continue; // this may be a match, but it's not safe to check

			if(kvp.prefix.type_id() != prefix_type_id)
				continue; // this is clearly not a match

			if(std::equal(begin, end, kvp.cbegin_key()))
				return utils::conditional_value<return_as_size_t>()(idx, &store[idx]);
		} // for i

		return utils::conditional_value<return_as_size_t>()(invalid_index, nullptr);
     	// either the kvp doesn't exist (so it needs to be made de novo), 
		// or it's not safe to read it from this thread at the moment (so it needs to be locked)
		// or we gave up after max_probing attempts (so the store is probably too full)

	}

	void delete_(prefix_type_id_t prefix_type_id, key_element_t const* begin,
				 key_element_t const* end){
		//TODO: assert(is_on_main_thread);

		size_t tombstone_idx = find<size_t>(prefix_type_id, begin, end, main_thread_id);
		if(tombstone_idx == invalid_index)
			return;

		decrement_upstream_probes_of(tombstone_idx);
		store[tombstone_idx].destruct_to_tombstone();
		extra_storage_info[tombstone_idx].set_to_tombstone();
		valid_count--;
		tombstone_count++;

		if(extra_storage_info[tombstone_idx].probes_passing_through == 0)
			tombstone_to_null(tombstone_idx);
			
	}

private:
	void decrement_upstream_probes_of(size_t end_idx, size_t start_probe_i){
		/* finds the base_idx for end_idx, using extra_storage.probes_used.
		   It then decrements all probe steps from start_probe_i to end_idx, excluding 
		   end_idx itself.
		   If decrementing probe count gets to zero for a tombstone then we convert the tombstone 
		   to null. */
		
		size_t n_probes = extra_storage_info[end_idx].probes_used();
		size_t base_idx = base_from_probe_i(end_idx, n_probes);

		for(size_t i=start_probe_i; i<n_probes; i++){
			size_t idx = probe_i_from(base_idx, i);
			assert(extra_storage_info[idx].probes_passing_through > 0);
			extra_storage_info[idx].probes_passing_through--; 
			if(extra_storage_info[idx].probes_passing_through == 0 && extra_storage_info[idx].is_tombstone())
				tombstone_to_null(idx);
		} // for i
	}

	void decrement_upstream_probes_of(size_t end_idx){
		decrement_upstream_probes_of(end_idx, 0);
	}

	void tombstone_to_null(size_t idx){
		assert(extra_storage_info[idx].probes_passing_through == 0
				 && extra_storage_info[idx].is_tombstone()
				 && store[idx].prefix.is_tombstone());
		store[idx].tombstone_to_null();
		extra_storage_info[idx].set_to_null();
		tombstone_count--;
	}



public:
	void attempt_clear_tombstones(){
		/* Iterates over the whole extra_storage_info/store, trying to find
		entries which can be shifted upstream, and deletes tombstones with 
		zero probe count. Note that this is not guaranteed to achieve 
		optimal compaction, indeed running multiple times may improve
		compaction in principle, though in practice that's not really that
		likely or significant. However, finding the optimal compaction is likely
		an NP-hard problem that we don't get close to solving - in practice it
		should be easy to do a pretty good job. */

		//TODO: assert(is_on_main_thread);

		for(size_t idx=0; idx<capacity; idx++){
			//std::cout << idx << ".  " <<  *this << std::endl;
			if(extra_storage_info[idx].is_null()){
				//nothing to see here
			}else if(extra_storage_info[idx].is_tombstone()){
				if(extra_storage_info[idx].probes_passing_through == 0){
					// we've found a tombstone already ripe for deletion
					// TODO: check whether this can ever happen
					tombstone_to_null(idx);
				}
			}else if(extra_storage_info[idx].probes_used() != 0 
				  && store[idx].prefix.main_thread_only_ref()){
				// we've found an actual kvp (not null or tombstone), which is not
				// at probe 0, and it is moveable. Lets try moving it.
				size_t base_idx = base_from_probe_i(idx, extra_storage_info[idx].probes_used());

				for(size_t i=0; i< extra_storage_info[idx].probes_used(); i++){
					size_t new_idx = probe_i_from(base_idx, i);
					assert(new_idx != idx); // if we could insert here previously we would have used the lowest possible probe count to reach this spot
						
					if(extra_storage_info[new_idx].is_tombstone()){
						// we've found an available upstream location
						
						// update probes_used_passing_through upstream tombstones/valid entries
						// and possibly convert some tombstones to null (both in extra and store).
						decrement_upstream_probes_of(idx, i);					

						// keep the tombstone count up to date (undo decrement
						// that occured if above call converted new_idx from tombstone to null).
						if(extra_storage_info[new_idx].is_null())
							tombstone_count++;
						
						// move idx into the new_idx
						extra_storage_info[new_idx].set_to_probes_used(i);	
						store[new_idx] = std::move(store[idx]);

						// clean up the old idx
						store[idx].destruct_to_tombstone();
						extra_storage_info[idx].set_to_tombstone();
						if(extra_storage_info[idx].probes_passing_through == 0)
							tombstone_to_null(idx);
						break; // break out of for-i, continue next idx
					}
				} // for i
			} // if tombstone else-if moveable
		} // for idx

	}


public:

	// Defines a non-member function, and makes it a friend of this class at the same time
	// http://en.cppreference.com/w/cpp/language/friend
	friend std::ostream& operator<<(std::ostream& os, self_t const& map){
		os << "unordered_map with capacity " << map.capacity_ << ":\n"
		   << "\tnull: " << map.capacity_-map.tombstone_count-map.valid_count << "\n"
		   << "\ttombstone: " << map.tombstone_count << "\n"
		   << "\tvalid: " << map.valid_count << "\n"
		   << "\tstore head: ";

		size_t head_size = 64;
		for(size_t i=0; i<map.capacity_ && i<head_size; i++){
			if(map.store[i].prefix.is_null())
				os << "-";
			else if(map.store[i].prefix.is_tombstone())
				os << "t";
			else if(get_hashed_idx(map.store[i].prefix.type_id(),
					map.store[i].cbegin_key(), map.store[i].cend_key()) == i)
				os << "#";
			else 
				os << "<";
		}
		os << (map.capacity_ > head_size ? "..." : "") << "\n";

		os << "\textra head: ";
		for(size_t i=0; i<map.capacity_ && i<head_size; i++){
			if(map.extra_storage_info[i].is_null())
				os << "-";
			else if(map.extra_storage_info[i].is_tombstone())
				os << "t";
			else if(map.extra_storage_info[i].probes_used() == 0)
				os << "#";
			else 
				os << "<";
		}
		os << (map.capacity_ > head_size ? "..." : "") << "\n";
		os << "\t            ";
		for(size_t i=0; i<map.capacity_ && i<head_size; i++){
			if(map.extra_storage_info[i].probes_passing_through==0)
				os << "-";
			else if(map.extra_storage_info[i].probes_passing_through < 10)
				os << map.extra_storage_info[i].probes_passing_through;
			else
				os << "+";
		}
		os << (map.capacity_ > head_size ? "..." : "") << "\n";
		return os;
	}
};


