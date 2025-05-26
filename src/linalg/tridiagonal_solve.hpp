#include <span>

#include "core_types/dvec.hpp"

namespace pricer {

// View into a tri-diagnoal system.  Assumes lower[0] and upper[n-1] is unused
// and that all spans are of the same length
struct TridiagonalSystemView
{
  std::span<const double> lower;
  std::span<const double> diag;
  std::span<const double> upper;
  std::span<const double> rhs;
};

auto tridiagonalSolve(const TridiagonalSystemView& system) -> DVec;

}// namespace pricer
