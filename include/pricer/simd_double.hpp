#ifndef SIMD_DOUBLE_H
#define SIMD_DOUBLE_H

#include <immintrin.h>

namespace pricer {

inline constexpr std::size_t simd_avx512 = 512;
inline constexpr std::size_t simd_avx2 = 256;
inline constexpr std::size_t simd_sse2 = 128;

#ifdef __AVX512F__
inline constexpr bool has_avx512 = true;
#else
inline constexpr bool has_avx512 = false;
#endif

#ifdef __AVX2__
inline constexpr bool has_avx2 = true;
#else
inline constexpr bool has_avx2 = false;
#endif

#ifdef __SSE2__
inline constexpr bool has_sse2 = true;
#else
inline constexpr bool has_sse2 = false;
#endif


constexpr auto simd_level() -> std::size_t
{
  if (has_avx512) { return simd_avx512; }
  if (has_avx2) { return simd_avx2; }
  if (has_sse2) { return simd_sse2; }
  return 0;
}

template<std::size_t Level>
struct simd_type;

template<>
struct simd_type<simd_avx512>
{
  using type = __m512d;
};

template<>
struct simd_type<simd_avx2>
{
  using type = __m256d;
};

template<>
struct simd_type<simd_sse2>
{
  using type = __m128d;
};

template<std::size_t Level>
using simd_type_t = typename simd_type<Level>::type;

using simd_double_t = simd_type_t<simd_level()>;
inline constexpr std::size_t simd_alignment = alignof(simd_double_t);
inline constexpr std::size_t simd_size = sizeof(simd_double_t) / sizeof(double);

}// namespace pricer

#endif
