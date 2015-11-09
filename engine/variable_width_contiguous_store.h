
namespace utils{
/*
	BucketPyramid stores a kind of tuple of
	containers, where each container is asked to hold a
	std::array<element_t, length_i>
	
	The lengths should be a list of increasing, positive, integers.

	the exec function is designed to be used as follows:

	.exec(len, [&](auto& bucket){ // do something with bucket	});

	It executes the lambda on the appropriate bucket, as specified by len,
	i.e. the bucket_i is provided, such that length_(i-1) < len <= length_i.
*/


template<template<typename> class Bucket, typename element_t, size_t length_i, size_t ...larger_lengths>
struct BucketPyramid{
	static const size_t length_i_ = length_i;

	// TODO: static_assert(length_i < larger_lengths[0], "lengths should be strictly increasing")
	Bucket<std::array<element_t, length_i> > bucket_i;
	BucketPyramid<Bucket, element_t, larger_lengths...> other_buckets;

	template<typename Foo>
	auto exec(element_t const* begin, element_t const* end, Foo foo){
		// use with a lambda
		size_t len = std::distance(begin, end);
		if(len <= length_i){
			std::array<element_t, length_i> padded = {};
			std::copy(begin, end, padded.begin());
			// TODO: pad end with special value...or have the default initialization be special.
			return foo(bucket_i, padded, len);
		}else{
			return other_buckets.exec(begin, end, foo);
		}
	}

	template<typename Foo>
	auto exec(size_t len, Foo foo){
		if(len <= length_i)
			return foo(bucket_i);
		else
			return other_buckets.exec(len, foo);
	}

	friend std::ostream& operator<<(std::ostream& o, const BucketPyramid& bp){
		o << "Bucket[len=" <<  bp.length_i_ << "]:  " << bp.bucket_i << "\n" << bp.other_buckets;
		return o;
	}


};

template<template<typename> class Bucket, typename element_t, size_t largest_length>
struct BucketPyramid<Bucket, element_t, largest_length>{
	static const size_t largest_length_ = largest_length;

	Bucket<std::array<element_t, largest_length> > bucket_i;

	template<typename Foo>
	auto exec(element_t const* begin, element_t const* end, Foo foo){
		std::array<element_t, largest_length> padded = {};
		size_t len = std::distance(begin, end); 
		assert(len <= largest_length);
		std::copy(begin, end, padded.begin());
		// TODO: pad end with special value...or have the default initialization be special.
		return foo(bucket_i, padded, len);
	}

	template<typename Foo>
	auto exec(size_t len, Foo foo){
		assert(len <= largest_length);
		return foo(bucket_i);
	}

	friend std::ostream& operator<<(std::ostream& o, const BucketPyramid& bp){
		o << "Bucket[len=" << bp.largest_length_ << "]:  " << bp.bucket_i << "\n";
		return o;
	}

};

}

template<typename value_t>
class ContiguousStore{
	/*
		This is a container storing an array of value_t's contiguously in memory,
		the contiguity is maintined even when you do dynamic insertions and deletes.
		This requries moving/copying at most one value_t on deletion.

		It acts as a store in the sense that when you insert a value_t into it,
		you are given a key, that can be used for accessing/deleting
		the stored value in future.  Accessing/deleting requires a dereferencing step
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
	*/
public:		
	using value_t_ = value_t;
	using ref_t = uint32_t;

private:
	std::vector<value_t> data;
	std::vector<ref_t> ref_to_data_mapping;
	std::vector<ref_t> data_to_ref_mapping;
	
	ref_t free_ref = 0;

public:
	ref_t insert(value_t val){
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
		data.push_back(val);
		data_to_ref_mapping.push_back(ref);
		ref_to_data_mapping[ref] = data.size() -1;
		return ref;
	}

	void delete_(ref_t ref){
		assert(ref != 0);

		auto idx = ref_to_data_mapping[ref];

		// shrink the data array by 1
		if(idx < data.size() -1){
			// in the general case we need to swap the last element into the hole
			// and update the index for the ref of that last element to point to
			// its new location.
			data[idx] = data.back(); // This should be compiled as a move-assign
			data.pop_back(); 
			data_to_ref_mapping[idx] = data_to_ref_mapping.back();
			data_to_ref_mapping.pop_back();
			ref_to_data_mapping[data_to_ref_mapping[idx]] = idx;
		}else{
			// otherwise we cleanly shrink the data array
			data.pop_back();
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

	ContiguousStore(){
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
		return data.size();
	}


	friend std::ostream& operator<<(std::ostream& o, const ContiguousStore& cs){
		o << "ContiguousStore[";
		if(cs.size() > 0){
			o << *cs.cbegin();
			for(auto v=cs.cbegin() +1; v != cs.cend(); v++)
				o <<  ", " << *v;   
		}
		o << ']';
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
	};

	utils::BucketPyramid<ContiguousStore, key_element_t, bucket_key_lens...> bucket_pyramid;

	public:
	
	auto insert(key_element_t const* begin, key_element_t const* end){

		auto ref = bucket_pyramid.exec(begin, end, [=](auto& bucket, auto& key_padded, size_t len){
			ref_t ref = bucket.insert(key_padded);
			return BucketRef(len, ref);
		});

		return ref;

	}

	auto delete_(BucketRef ref){
		assert(ref.len > 0);
		auto ref_within_bucket = ref.ref_within_bucket;
		bucket_pyramid.exec(ref.len, [=](auto& bucket){
			bucket.delete_(ref_within_bucket);
		});
	}

	friend std::ostream& operator<<(std::ostream& o, const VariableWidthContiguousStore& vwcs){
		o << vwcs.bucket_pyramid;
		return o;
	}

};




