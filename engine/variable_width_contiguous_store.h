
#ifndef _VARIABLE_WIDTH_CONTIGUOUS_STOR_H_
#define _VARIABLE_WIDTH_CONTIGUOUS_STOR_H_

template<typename element_t, element_t padding_el>
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
	/* TODO: I would like to use opaque-typedefs to separate the 
		three kinds of indexing here: 
			idx_ref_t 		 - indexing by ref
			idx_iter_chunk_t - indexing by iteration order, when we iterate over
							   std::array<element_t, n_elements> chunks
			idx_iter_el_t 	 - indexing by iteratrion order, when we iterate over
			  				   individual element_t's.			
	 Currently the user has to be careful not to confuse the three.*/
	using idx_ref_t = uint32_t;
	using idx_iter_chunk_t = uint32_t;
	using idx_iter_el_t = uint32_t;
	using ref_t = idx_ref_t;
	
	const size_t n_elements;

private:
	std::vector<element_t> data; // this is a 2D array with, n_elements on the "inner" dimension/loop
	std::vector<idx_iter_chunk_t> ref_to_data_mapping;
	std::vector<idx_ref_t> data_to_ref_mapping; // this grows/shrinks in synchrony with data, so could in principle combine malloc/dealloc, but who cares?!
	ref_t free_ref = 0;

	void data_push_back(element_t const* begin, element_t const* end){
		assert(std::distance(begin, end) <= n_elements);

		data.insert(data.end(), begin, end);
		size_t pad = n_elements - std::distance(begin, end); 
		std::fill_n(std::back_inserter(data), pad, padding_el);
	}

	void data_pop_back_into(idx_iter_chunk_t idx){
		std::copy(std::make_move_iterator(data.end() - n_elements), 
				  std::make_move_iterator(data.end()), 
									data.begin() + idx*n_elements); 
		data_pop_back();
	}

	void data_pop_back(){
		for(idx_iter_el_t i=0;i<n_elements;i++)
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

	element_t& operator[](idx_iter_chunk_t idx){
		return data[idx*n_elements];
	}

	auto index_to_ref(idx_iter_chunk_t idx) const{
		return data_to_ref_mapping[idx];
	}

	idx_iter_chunk_t size() const{
		return data_to_ref_mapping.size();
	}

	// TODO: these iterators are confusing: delete them
	auto begin(){return data.begin();}
	auto end(){return data.end();}
	auto cbegin() const {return data.cbegin();}
	auto cend() const { return data.cend();}


	friend std::ostream& operator<<(std::ostream& o, const ContiguousStore& cs){
		o << "ContiguousStore<len=" << cs.n_elements << ">[ ";
		if(cs.size() > 0){
			o << cs.data[0];
			for(idx_iter_el_t i=1; i<cs.data.size(); i++){
				o <<  (i % cs.n_elements == 0 ? " | " : ", ");
				if(cs.data[i] == padding_el)
					o << "-";
				else
					o << cs.data[i];   
			}
		}
		o << " ]";
		return o;
	}

};


template<typename element_t, element_t padding_el, typename Extra>
class ContiguousStoreWithExtra 
	: public ContiguousStore<element_t, padding_el>{
		/* This should be "has-a" rather than "is-a", but I wasn't sure how to do
		   that without manually piping through every single method call/data member
		   access..The intention is to hide some of the methods of the base. 
		   Note that the implelmentation of this is VERY TIGHTLY COUPLED to
		   the base class, you could think of them as template specializations
		   of a common signature.
			
		   This version adds the ability to store an Extra class instance
		   with each item.  The instance value can be read/updated using
		   get_extra and update_extra respectively.

		   The insert method now adds an extra paramenter, an Extra
		   instance.

		   The Extra instances are stored in a vector at the index of the reference,
		   so they are not fast to iterate over.

		   This is really just a convenice - this class doesn't need special access
		   to the standard ContiguousStore, but in future we might decide to 
		   restructer stuff a bit as it seems a bit wasteful having pairs of vectors
		   coupled in terms of growing/shrinking access etc...you could coalessce 
		   malloc's/deletes etc.
		*/

	public:

	using base_t = ContiguousStore<element_t, padding_el>;
	using ref_t = typename base_t::ref_t;
	using idx_iter_chunk_t = typename base_t::idx_iter_chunk_t;

	template<typename ...Args>
	ContiguousStoreWithExtra(Args&& ...args)
		: base_t(std::forward<Args...>(args...)){
			ref_to_extra.push_back({});
		}

	std::vector<Extra> ref_to_extra;

	auto insert(element_t const* begin, element_t const* end, Extra ex){
		auto idx = base_t::insert(begin, end);
		if(idx < ref_to_extra.size()){
			new (&ref_to_extra[idx]) Extra(ex); // placement new
		}else{
			assert(ref_to_extra.size() == idx); //we should be staying in-sync
			ref_to_extra.push_back(ex);
		}
		return idx;
	}

	auto delete_(ref_t idx){
		base_t::delete_(idx);
		if(idx == ref_to_extra.size()-1)
			ref_to_extra.pop_back();
		else
			ref_to_extra[idx].~Extra();  // "placement delete"
			
	}

	auto get_extra_with_index(idx_iter_chunk_t idx) const {
		return ref_to_extra[base_t::index_to_ref(idx)];
	}

	void update_extra_with_ref(ref_t idx, Extra ex){
		ref_to_extra[idx] = ex;
	}

};


template<typename key_element_t, key_element_t padding_el, typename Extra, size_t ...bucket_key_lens>
class VariableWidthContiguousStore{
public:
	static const bool has_extra = !std::is_void<Extra>::value;
	using store_t = typename std::conditional<has_extra,
									ContiguousStoreWithExtra<key_element_t, padding_el, Extra>,
									ContiguousStore<key_element_t, padding_el> >::type;
	using idx_iter_chunk_t = typename store_t::idx_iter_chunk_t;
	using ref_t = typename store_t::ref_t;
									
	class BucketRef{
		// Publically this is only moveable/destructable not construcable/copyable.
		using parent_t = VariableWidthContiguousStore<key_element_t, padding_el, Extra, bucket_key_lens...>;
		parent_t* parent;
		ref_t ref_within_bucket;
		size_t len;
		BucketRef(parent_t* p, size_t len_in, ref_t ref_in) {
			parent = p;
			len = len_in;
			ref_within_bucket = ref_in;
		}
	public:
		BucketRef(BucketRef&& other){
			parent = other.parent;
			len = other.len;
			ref_within_bucket = other.ref_within_bucket;
			other.len = 0;
		}
		~BucketRef(){
			if (len>0)
				parent->delete_(len, ref_within_bucket);
		}
		friend class VariableWidthContiguousStore<key_element_t, padding_el, Extra, bucket_key_lens...>; // apparently outer class is not a friend by default
	
		friend std::ostream& operator<<(std::ostream& os, BucketRef const& r){
			os << "BucketRef[len=" << r.len << ", ref_within_bucket=" << r.ref_within_bucket <<  "]";
			return os;
		}
	};

private:
	std::array<store_t, sizeof...(bucket_key_lens)> bucket_pyramid{bucket_key_lens...};

	auto delete_(size_t len, ref_t ref_within_bucket){
		assert(len > 0);
		for(auto& b : bucket_pyramid)if(len <= b.n_elements){
			b.delete_(ref_within_bucket);
			return;
		}
	}

public:

	template<typename Foo>
	void for_each(Foo foo){
		for(auto& b : bucket_pyramid){
			for(idx_iter_chunk_t i=0;i<b.size();i++)
				foo(&b[i], &b[i+1]);
		} // for-buckets
	}

	template<typename Predicate, typename FooExtra, typename Return=void, typename Extra_=Extra> 
	typename std::enable_if<has_extra, Return>::type  /* return type= void, if hasextra */
	for_each(Predicate pred, FooExtra foo_extra){
		/* This extends the simple for_each when there is Extra data stored.
		   The function now takes two lambdas: the first is the same as before, but
		   is now treated as a predicate: when true, the second lambda is called,
		   passing the extra instance as the first argument: the 2nd and 3rd arguments
		   are the same as the two arguments passed to the predicate.		*/
		for(auto& b : bucket_pyramid){
			for(idx_iter_chunk_t i=0; i<b.size(); i++){
				if(pred(&b[i], &b[i+1]))
					foo_extra(b.get_extra_with_index(i), &b[i], &b[i+1]);
			} // for-chunks
		} // for-buckets
	}

	template<typename Return=BucketRef> 
	typename std::enable_if<!has_extra, Return>::type  /* return type= BucketRef, if !hasextra */
	insert(key_element_t const* begin, key_element_t const* end){
		size_t len = std::distance(begin, end);
		for(auto& b : bucket_pyramid)if(len <= b.n_elements)
			return BucketRef(this, len, b.insert(begin, end));
		abort();
	}

	template<typename Return=BucketRef, typename Extra_=Extra> 
	typename std::enable_if<has_extra, Return>::type  /* return type= BucketRef, if hasextra */
	insert(key_element_t const* begin, key_element_t const* end, Extra_ ex){
		size_t len = std::distance(begin, end);
		for(auto& b : bucket_pyramid)if(len <= b.n_elements)
			return BucketRef(this, len, b.insert(begin, end, ex));
		abort();
	}

	template<typename Return=void, typename Extra_=Extra> 
	typename std::enable_if<has_extra, Return>::type  /* return type= void, if hasextra */
	set_extra(BucketRef const& ref, Extra_ ex){
		size_t len = ref.len;
		for(auto& b : bucket_pyramid)if(len <= b.n_elements)
			return b.update_extra_with_ref(ref.ref_within_bucket, ex);
	}

	friend std::ostream& operator<<(std::ostream& o, const VariableWidthContiguousStore& vwcs){
		o << "VariableWidthContiguousStore:\n";
		for(auto& b: vwcs.bucket_pyramid)
			o << b << "\n";
		return o;
	}

};



#endif // _VARIABLE_WIDTH_CONTIGUOUS_STOR_H_