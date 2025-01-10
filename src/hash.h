#ifndef HASH_H
#define HASH_H

#include <cstdint>
#include <string>

uint32_t PersistentHash(const void* data, size_t length);
uint32_t PersistentHash(const std::string& str);
uint32_t Hash(const void* data, size_t length);
uint32_t Hash(const std::string& str);
size_t HashInts32(uint32_t value1, uint32_t value2);
size_t HashInts64(uint64_t value1, uint64_t value2);

#endif // HASH_H
