// Adapted from: https://github.com/PeterScott/murmur, 30-Oct-2015
//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the
// public domain. The author hereby disclaims copyright to this source
// code.
//-----------------------------------------------------------------------------
// user-facing functions is murmur3::hash<data_t>(data_t*, len_data, seed)
// Note that len is NOT BYTES, it is the number of data_t's in the array.
//-----------------------------------------------------------------------------


#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_

#include <cstdint>

namespace murmur3{

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

static FORCE_INLINE uint32_t ROTL32 (uint32_t x, int8_t r){
  return (x << r) | (x >> (32 - r));
}

static FORCE_INLINE uint32_t fmix32 (uint32_t h){
  // Finalization mix - force all bits of a hash block to avalanche
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}


template<typename data_t>
uint32_t hash(const data_t* data, size_t len, uint32_t seed){
  // This is based on the x86_32 version

  const size_t len_8 = len * sizeof(data_t);
  const uint8_t * data_8 = reinterpret_cast<const uint8_t*>(data);
  const size_t len_32 = len_8 / 4; // gives floor (obviously)

  uint32_t h1 = seed;
  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;

  // body
  const uint32_t * data_32_end = reinterpret_cast<const uint32_t*>(
  											data_8 + len_32*4);
  for(size_t i = -len_32; i; i++){
    uint32_t k1 = data_32_end[i];
    k1 *= c1;
    k1 = ROTL32(k1,15);
    k1 *= c2;
    h1 ^= k1;
    h1 = ROTL32(h1,13); 
    h1 = h1*5+0xe6546b64;
  }

  // tail
  if(sizeof(data_t) % 4 != 0){
	  const uint8_t * tail = (const uint8_t*)(data_32_end);
	  uint32_t k1 = 0;
	  switch(len_8 & 3){
	  case 3: k1 ^= tail[2] << 16;
	  case 2: k1 ^= tail[1] << 8;
	  case 1: k1 ^= tail[0];
	          k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
	  };
  }

  // finalization
  h1 ^= len_8;
  h1 = fmix32(h1);
  return h1;
} 



} // namespace murmur3

#endif // _MURMURHASH3_H_
