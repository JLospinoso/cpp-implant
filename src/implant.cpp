/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include "implant.h"

// Note: from Chapter 20 of C++ Crash Course
//       c.f. https://github.com/JLospinoso/ccc/blob/master/chapter_20/web_request.cpp
std::string make_request(std::string_view host,
                         std::string_view service,
                         std::string_view payload,
                         boost::asio::io_context& io_context) {
  std::stringstream request_stream;
  request_stream << "POST / HTTP/1.1\r\n"
                    "Host: "
                 << host
                 << "\r\n"
                    "Accept: text/html\r\n"
                    "Accept-Language: en-us\r\n"
                    "Accept-Encoding: identity\r\n"
                    "Connection: close\r\n"
                    "Content-Length: " << payload.size() << "\r\n"
                 << "\r\n"
                 << payload;
  const auto request = request_stream.str();
  boost::asio::ip::tcp::resolver resolver{ io_context };
  const auto endpoints = resolver.resolve(host, service);
  boost::asio::ip::tcp::socket socket{ io_context };
  const auto connected_endpoint = boost::asio::connect(socket, endpoints);
  boost::asio::write(socket, boost::asio::buffer(request));
  std::string response;
  boost::system::error_code ec;
  boost::asio::read(socket, boost::asio::dynamic_buffer(response), ec);
  if(ec && ec.value() != 2)
    throw boost::system::system_error{ ec };
  return response;
}
