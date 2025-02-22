﻿#include <cstdint>
#include <string>
#include <limits>
#include <iostream>
#include "superfasthash.cpp"

uint32_t PersistentHash(const void* data, size_t length) {
  if (length > static_cast<size_t>(std::numeric_limits<int>::max())) {
    std::cerr << "Error: Length exceeds maximum limit.\n";
    return 0;
  }
  return SuperFastHash(reinterpret_cast<const char*>(data), static_cast<int>(length));
}

uint32_t PersistentHash(const std::string& str) {
  return PersistentHash(str.data(), str.size());
}

size_t HashInts32(uint32_t value1, uint32_t value2) {
  uint64_t value1_64 = value1;
  uint64_t hash64 = (value1_64 << 32) | value2;
  if (sizeof(size_t) >= sizeof(uint64_t))
    return static_cast<size_t>(hash64);
  uint64_t odd_random = 481046412LL << 32 | 1025306955LL;
  uint32_t shift_random = 10121U << 16;
  hash64 = hash64 * odd_random + shift_random;
  size_t high_bits = static_cast<size_t>(hash64 >> (8 * (sizeof(uint64_t) - sizeof(size_t))));
  return high_bits;
}

size_t HashInts64(uint64_t value1, uint64_t value2) {
  uint32_t short_random1 = 842304669U;
  uint32_t short_random2 = 619063811U;
  uint32_t short_random3 = 937041849U;
  uint32_t short_random4 = 3309708029U;
  uint32_t value1a = static_cast<uint32_t>(value1 & 0xffffffff);
  uint32_t value1b = static_cast<uint32_t>((value1 >> 32) & 0xffffffff);
  uint32_t value2a = static_cast<uint32_t>(value2 & 0xffffffff);
  uint32_t value2b = static_cast<uint32_t>((value2 >> 32) & 0xffffffff);
  uint64_t product1 = static_cast<uint64_t>(value1a) * short_random1;
  uint64_t product2 = static_cast<uint64_t>(value1b) * short_random2;
  uint64_t product3 = static_cast<uint64_t>(value2a) * short_random3;
  uint64_t product4 = static_cast<uint64_t>(value2b) * short_random4;
  uint64_t hash64 = product1 + product2 + product3 + product4;
  if (sizeof(size_t) >= sizeof(uint64_t))
    return static_cast<size_t>(hash64);
  uint64_t odd_random = 1578233944LL << 32 | 194370989LL;
  uint32_t shift_random = 20591U << 16;
  hash64 = hash64 * odd_random + shift_random;
  size_t high_bits = static_cast<size_t>(hash64 >> (8 * (sizeof(uint64_t) - sizeof(size_t))));
  return high_bits;
}

uint32_t Hash(const void* data, size_t length) {
	return PersistentHash(data, length);
}

uint32_t Hash(const std::string& str) {
	return PersistentHash(str.data(), str.size());
}

