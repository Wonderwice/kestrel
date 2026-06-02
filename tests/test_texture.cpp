#include "doctest.h"
#include "test_helpers.h"
#include "texture.h"

using namespace kestrel;
using namespace kestrel_test;

TEST_CASE("ConstantTexture returns the same color for any UV") {
  ConstantTexture tex(Color(0.2f, 0.4f, 0.6f));
  check_vec_approx(tex.value(0.0f, 0.0f), Color(0.2f, 0.4f, 0.6f));
  check_vec_approx(tex.value(0.5f, 0.5f), Color(0.2f, 0.4f, 0.6f));
  check_vec_approx(tex.value(1.0f, 1.0f), Color(0.2f, 0.4f, 0.6f));
  check_vec_approx(tex.value(-3.0f, 7.0f), Color(0.2f, 0.4f, 0.6f));
}

TEST_CASE("ConstantTexture is usable through the Texture interface") {
  const Texture &t = *new ConstantTexture(Color(1, 1, 1));
  check_vec_approx(t.value(0.3f, 0.7f), Color(1, 1, 1));
  delete &t;
}

TEST_CASE("CheckerboardTexture alternates between the two colors") {
  Color a(1, 0, 0), b(0, 1, 0);
  CheckerboardTexture tex(a, b);  // unit scale
  check_vec_approx(tex.value(0.25f, 0.25f), a);  // cell (0,0) -> color0
  check_vec_approx(tex.value(1.25f, 0.25f), b);  // cell (1,0) -> color1
  check_vec_approx(tex.value(0.25f, 1.25f), b);  // cell (0,1) -> color1
  check_vec_approx(tex.value(1.25f, 1.25f), a);  // cell (1,1) -> color0
}

TEST_CASE("CheckerboardTexture honors the uv scale") {
  Color a(1, 1, 1), b(0, 0, 0);
  CheckerboardTexture tex(a, b, 8.0f, 8.0f);
  check_vec_approx(tex.value(0.05f, 0.05f), a);  // floor(0.4)=0 -> color0
  check_vec_approx(tex.value(0.20f, 0.05f), b);  // floor(1.6)=1 -> color1
}
