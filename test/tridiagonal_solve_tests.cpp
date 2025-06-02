#include <array>
#include <catch2/catch_template_test_macros.hpp>

#include "linalg/tridiagonal_solve.hpp"

namespace pricer {
constexpr auto eps = 1e-12;

TEST_CASE("solve system", "[tridiagonalSolve]")
{
  auto lower = std::array{ 0.0, -1.0, -1.0 };
  auto diag = std::array{ 2.0, 2.0, 2.0 };
  auto upper = std::array{ -1.0, -1.0, 0.0 };
  auto rhs = std::array{ 1.0, 0.0, 1.0 };

  auto result = tridiagonalSolve({ .lower = lower, .diag = diag, .upper = upper, .rhs = rhs });

  REQUIRE(result.size() == 3);
  REQUIRE(std::abs(result[0] - 1.0) < eps);
  REQUIRE(std::abs(result[1] - 1.0) < eps);
  REQUIRE(std::abs(result[2] - 1.0) < eps);
}
}// namespace pricer
