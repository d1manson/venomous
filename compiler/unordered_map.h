
#include <memory>
#include <array>
#include <cstdint>

#include "utils/murmur3.h"
#define HASH_NAMESPACE murmur3

bool constexpr is_pow_2(int N){
	return (N && (N & (N - 1)) == 0);
}

template<typename Entry, size_t capacity>
class unordered_map{
	static_assert(is_pow_2(capacity), "capacity should be pow 2.");

	using prefix_t = Entry::prefix_t_;
	using array_t = Entry::array_t_;
	using prefix_type_id_t = prefix_t::type_id_t;

private:
	const auto store = std::make_unique<std::array<Entry, capacity>>();
	const size_t max_probing = 10; // total number of attempts to read from store before giving up
								   // i.e. if 1, then just read at hashed location with no quadartic probing.

	static auto modulo_capacity(size_t x) {
		return (capacity-1) & x;
	}

	static auto get_hashed_idx(prefix_type_id_t prefix_type_id,
						     array_t const* begin, array_t const* end) const {
		// returns the index into store.
		// array_t must be contiguous
		return modulo_capacity(HASH_NAMESPACE::hash<array_t>(
								begin, end-begin, prefix_type_id));
	}

public:
	Entry* find(prefix_type_id_t prefix_type_id,
		        array_t const* begin, array_t const* end){

		auto base_idx = get_hashed_idx(prefix_type_id, begin, end);

		bool encoutered_lazily_deleted_entry = false;

		for(Entry& current=store[base_idx], size_t i=0; 
			!current.prefix.is_null() && i < max_probing;
			current=store[modulo_capacity(base_idx + (++i)*i)]){

			if(!current.safe_to_read())
				continue; // this may be a match, but it's not safe to check

			encoutered_lazily_deleted_entry |= current.prefix.is_lazily_deleted();

			if(current.prefix.type_id() != prefix_type_id)
				continue; // this is clearly not a match

			if(std::equal(begin, end, current.cbegin_array()){
				if(encoutered_lazily_deleted_entry){
					//TODO: request that this entry be swapped into the lazily deleted spot
				}
				return current; // a safe match has been found!
			}
		}

		return nullptr; // either the entry doesn't exist (so it needs to be made de novo), 
						// or it's not safe to read it from this thread at the moment (so it needs to be locked)
						// or we gave up after max_probing attempts (so the store is probably too full)

	}

}