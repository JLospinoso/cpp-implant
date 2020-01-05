/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include "results.h"
Result::Result(const boost::uuids::uuid &id, std::string contents,
               const bool success)
    : id(id), contents{std::move(contents)}, success(success) {}
