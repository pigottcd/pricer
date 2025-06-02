#ifndef DVEC_H
#define DVEC_H

#include <vector>

#include "memory/pool_allocator.hpp"
#include "simd_double.hpp"

namespace pricer {

constexpr inline std::size_t dblock_size = 128;
constexpr inline std::size_t dblock_count = 8142;

using DVec = std::vector<double, PoolAllocator<double, dblock_size, dblock_count, simd_alignment>>;

}// namespace pricer

#endif