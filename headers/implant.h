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

std::string make_request(std::string_view host, std::string_view port, boost::asio::io_context& io_context);
