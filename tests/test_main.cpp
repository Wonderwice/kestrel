// Single translation unit that provides doctest's main(). Every other test
// file just includes "doctest.h" without this define.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
