/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <istream>
#include <ostream>
#include <string_view>
#include <cstdint>

// Note: from Chapter 20 of C++ Crash Course
//       c.f. https://github.com/JLospinoso/ccc/blob/master/chapter_20/web_request.cpp
std::string make_request(std::string_view host, std::string_view port, std::string_view payload, boost::asio::io_context& io_context);
