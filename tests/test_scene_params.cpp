#include "doctest.h"
#include "scene_params.h"

#include <map>
#include <string>

using namespace kestrel;

TEST_CASE("substitute_params replaces $name and ${name}") {
  std::map<std::string, std::string> p{{"spp", "64"}, {"width", "683"}};
  CHECK(substitute_params("$spp", p) == "64");
  CHECK(substitute_params("${width}", p) == "683");
  CHECK(substitute_params("a $spp b ${width}", p) == "a 64 b 683");
}

TEST_CASE("substitute_params leaves unknown refs and plain text untouched") {
  std::map<std::string, std::string> p{{"spp", "64"}};
  CHECK(substitute_params("plain", p) == "plain");
  CHECK(substitute_params("$missing", p) == "$missing");
  CHECK(substitute_params("nodollar", {}) == "nodollar");
}

TEST_CASE("substitute_params prefers longer names (no prefix clobber)") {
  std::map<std::string, std::string> p{{"x", "1"}, {"xy", "2"}};
  CHECK(substitute_params("$xy", p) == "2");
  CHECK(substitute_params("$x", p) == "1");
}
