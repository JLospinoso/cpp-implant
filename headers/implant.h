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
#include <variant>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid.hpp>

struct GetTask{
  constexpr static std::string_view key{ "get" };
  GetTask(const boost::uuids::uuid &id, std::string path);
private:
  boost::uuids::uuid id;
  std::string path;
};

struct PutTask{
  constexpr static std::string_view key{ "put" };
  PutTask(const boost::uuids::uuid &id, std::string path,
          std::string contents);

private:
  boost::uuids::uuid id;
  std::string path;
  std::string contents;
};
struct ListTask{
  constexpr static std::string_view key{ "list" };
  ListTask(const boost::uuids::uuid &id, std::string path,
           uint8_t depth);

private:
  boost::uuids::uuid id;
  std::string path;
  uint8_t depth;
};
struct ExecuteTask {
  constexpr static std::string_view key{ "execute" };
  ExecuteTask(const boost::uuids::uuid &id, std::string command);

private:
  boost::uuids::uuid id;
  std::string command;
};

using Task = std::variant<GetTask, PutTask, ListTask, ExecuteTask>;

// Note: from Chapter 20 of C++ Crash Course
//       c.f. https://github.com/JLospinoso/ccc/blob/master/chapter_20/web_request.cpp
std::string make_request(std::string_view host, std::string_view port, std::string_view payload, boost::asio::io_context& io_context);

Task parse_task_from(const boost::property_tree::ptree& task_tree);