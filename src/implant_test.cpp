/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#define BOOST_TEST_MODULE ImplantTest
#include <boost/test/included/unit_test.hpp>
#include "implant.h"

struct MyTestFixture {};

BOOST_FIXTURE_TEST_CASE(MyTestA, MyTestFixture) {
  // Test A here
}
