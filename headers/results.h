/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#pragma once
#include <boost/uuid/uuid.hpp>
#include <string>

struct Result {
  Result(const boost::uuids::uuid &id, std::string contents, bool success);
  const boost::uuids::uuid id;
  const std::string contents;
  const bool success;
};