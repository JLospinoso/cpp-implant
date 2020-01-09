/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#define BOOST_TEST_MODULE ImplantTest
#include <boost/test/included/unit_test.hpp>
#include "implant.h"
#include "results.h"
#include "tasks.h"
#include <boost/uuid/uuid.hpp>

BOOST_AUTO_TEST_CASE(results_is_constructible) {
  boost::uuids::uuid id{};
  const std::string expected_contents = "test_content";
  const bool expected_success{true};
  Result results{id, expected_contents, expected_success};

  BOOST_TEST(results.id == id);
  BOOST_TEST(results.contents == expected_contents);
  BOOST_TEST(results.success == expected_success);
}
