

template<typename element_t>
class ContiguousStore{
/* 
	This is a container for storing multiple std::array<element_t, len>'s 
	contiguously in memory,  the contiguity is maintined even when you do 
	dynamic insertions and deletes.  This requries moving/copying at most 
	one of the std::arrays on deletion.

	It acts as a store in the sense that when you insert an array into it,
	you are given a key, that can be used for accessing/deleting
	the stored array in future.  Accessing/deleting requires a dereferencing step
	and doesn't have any special contiguity/cache utilization properties, whereas
	iterating is fast because it's contiguous and doesn't involve dereferencing.
	
	The iterating order is undefined.

	The key's are actually indices into ref_to_data_mapping, which in turn
	maintains up-to-date indices for the actual value_t's in the main array.
	data_to_ref_mapping maintains the inverse.

	ref_to_data_mapping operates an (index-based) free-list with
	the head of the list recorded in the variable free_ref.  if free_ref=0,
	then the free list is empty (this means we can never actually utilize index 0 in
	ref_to_data_mapping).

	This is not in any way thread-safe.

	Originally we made the len in array<> one of the template parameters, but
	that caused unneccessary code-bloat.

	*/


public:
	using ref_t = uint32_t;
	const size_t n_elements;

private:
	std::vector<element_t> data; // this is a 2D array with, n_elements on the "inner" dimension/loop
	std::vector<ref_t> ref_to_data_mapping;
	std::vector<ref_t> data_to_ref_mapping; // this grows/shrinks in synchrony with data, so could in principle combine malloc/dealloc, but who cares?!
	ref_t free_ref = 0;

	void data_push_back(element_t const* begin, element_t const* end){
		assert(std::distance(begin, end) <= n_elements);

		data.insert(data.end(), begin, end);
		size_t pad = n_elements - std::distance(begin, end); 
		data.resize(data.size() + pad);
	}

	void data_pop_back_into(size_t idx){
		std::copy(std::make_move_iterator(data.end() - n_elements), 
				  std::make_move_iterator(data.end()), 
									data.begin() + idx*n_elements); 
		data_pop_back();
	}

	void data_pop_back(){
		for(size_t i=0;i<n_elements;i++)
			data.pop_back();
	}


public:
	ref_t insert(element_t const* begin, element_t const* end){
		ref_t ref;
		if(free_ref != 0){
			// pop the ref off the free list...
			ref = free_ref;
			free_ref = ref_to_data_mapping[free_ref];
		}else{
			// create a whole new ref...
			ref_to_data_mapping.push_back(0);
			ref = ref_to_data_mapping.size() -1;
		}

		// store the val, and record its location at ref
		data_push_back(begin, end);
		data_to_ref_mapping.push_back(ref);
		ref_to_data_mapping[ref] = size() -1;
		return ref;
	}

	void delete_(ref_t ref){
		assert(ref != 0);

		auto idx = ref_to_data_mapping[ref];

		// shrink the data array by 1
		if(idx < size()){
			// in the general case we need to swap the last element into the hole
			// and update the index for the ref of that last element to point to
			// its new location.
			data_pop_back_into(idx); 
			data_to_ref_mapping[idx] = data_to_ref_mapping.back();
			data_to_ref_mapping.pop_back();
			ref_to_data_mapping[data_to_ref_mapping[idx]] = idx;
		}else{
			// otherwise we cleanly shrink the data array
			data_pop_back();
			data_to_ref_mapping.pop_back();
		}

		// deal with the ref_to_data_mapping....
		if(ref < ref_to_data_mapping.size()-1){
			// In the general case, push the old ref onto the free-list
			ref_to_data_mapping[ref] = free_ref;
			free_ref = ref;
		}else{
			// otherwise we cleanly shrink the mapping, and don't touch the free-list 
			ref_to_data_mapping.pop_back();
		}

	}

	ContiguousStore(size_t n_elements_in) 
				: n_elements(n_elements_in){
		ref_to_data_mapping.push_back(0);
	}

	auto begin(){
		return data.begin();
	}

	auto end(){
		return data.end();
	}

	auto cbegin() const {
		return data.cbegin();
	}

	auto cend() const {
		return data.cend();
	}

	size_t size() const{
		return data_to_ref_mapping.size();
	}

	friend std::ostream& operator<<(std::ostream& o, const ContiguousStore& cs){
		o << "ContiguousStore<len=" << cs.n_elements << ">[ ";
		if(cs.size() > 0){
			o << cs.data[0];
			for(size_t i=1; i<cs.data.size(); i++)
				o <<  (i % cs.n_elements == 0 ? " | " : ", ") << cs.data[i];   
		}
		o << " ]";
		return o;
	}

};



template<typename key_element_t, size_t ...bucket_key_lens>
class VariableWidthContiguousStore{
	using ref_t = uint32_t;

	class BucketRef{
		// Publically this is only moveable/destructable not construcable/copyable.
		ref_t ref_within_bucket;
		size_t len;
		BucketRef(size_t len_in, ref_t ref_in) {
			len = len_in;
			ref_within_bucket = ref_in;
		}
	public:
		BucketRef(BucketRef&& other){
			len = other.len;
			ref_within_bucket = other.ref_within_bucket;
			other.len = 0;
		}
		~BucketRef(){
			// TODO: call delete_ on parent store.
		}
		friend class VariableWidthContiguousStore<key_element_t, bucket_key_lens...>; // apparently outer class is not a friend by default
	
		friend std::ostream& operator<<(std::ostream& os, BucketRef const& r){
			os << "BucketRef[len=" << r.len << ", ref_within_bucket=" << r.ref_within_bucket <<  "]";
			return os;
		}
	};

	std::array<ContiguousStore<key_element_t>, sizeof...(bucket_key_lens)> bucket_pyramid{bucket_key_lens...};

	public:
	
	auto insert(key_element_t const* begin, key_element_t const* end){
		size_t len = std::distance(begin, end);
		for(auto& b : bucket_pyramid)if(len <= b.n_elements)
			return BucketRef(len, b.insert(begin, end));
		abort();
	}

	auto delete_(BucketRef ref){
		assert(ref.len > 0);
		for(auto& b : bucket_pyramid)if(ref.len <= b.n_elements){
			b.delete_(ref.ref_within_bucket);
			return;
		}
	}

	friend std::ostream& operator<<(std::ostream& o, const VariableWidthContiguousStore& vwcs){
		o << "VariableWidthContiguousStore:\n";
		for(auto& b: vwcs.bucket_pyramid)
			o << b << "\n";
		return o;
	}

};




