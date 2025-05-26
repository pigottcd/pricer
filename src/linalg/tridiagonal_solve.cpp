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

auto tridiagonalSolve(const TridiagonalSystemView& system) -> DVec
{
  const auto size = system.diag.size();
  auto solution = DVec(size);
  auto modified_upper = DVec(size);
  auto modified_rhs = DVec(size);

  // forward sweep
  auto pivot = system.diag[0];
  modified_upper[0] = system.upper[0] / pivot;
  modified_rhs[0] = system.rhs[0] / pivot;
  for (auto i = std::size_t{ 1 }; i < size; ++i) {
    pivot = system.diag[i] - system.lower[i] * modified_upper[i - 1];
    modified_upper[i] = system.upper[i] / pivot;
    modified_rhs[i] = (system.rhs[i] - system.lower[i] * modified_rhs[i - 1]) / pivot;
  }

  // back substitution
  solution[size - 1] = modified_rhs[size - 1];
  for (auto i = std::size_t{ size - 1 }; i-- > 0;) {
    solution[i] = modified_rhs[i] - modified_upper[i] * solution[i + 1];
  }
  return solution;
}

}// namespace pricer
