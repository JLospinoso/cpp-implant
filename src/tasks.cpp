/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include "tasks.h"
#include <boost/uuid/uuid_io.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <array>

[[nodiscard]] Task
parse_task_from(const boost::property_tree::ptree &task_tree,
                std::function<void(const Configuration &)> setter) {
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
  if (type == DeleteTask::key) {
    return DeleteTask{id, task_tree.get_child("path").get_value<std::string>()};
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
                         std::move(setter)};
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
  std::copy(std::begin(contents), std::end(contents),
            std::ostreambuf_iterator<char>(file));
  return Result{id, "Wrote file.", true};
}

ListTask::ListTask(const boost::uuids::uuid &id, std::string path,
                   uint8_t depth)
    : id(id), path(std::move(path)), depth(depth) {}
Result ListTask::run() const {
  std::stringstream ss;
  auto iterator = std::filesystem::recursive_directory_iterator{
      path, std::filesystem::directory_options::skip_permission_denied};
  if (iterator == std::filesystem::recursive_directory_iterator{})
    return Result{id, "Directory empty or did not exist.", false};
  while (iterator != std::filesystem::recursive_directory_iterator{}) {
    try {
      const auto &entry = *iterator;
      ss << entry.path() << "\n";
      iterator++;
    } catch (const std::exception &e) {
      ss << "(" << e.what() << ")\n";
    }
  }
  return Result{id, ss.str(), true};
}

#ifdef _WIN32
#define PLATFORM_SPECIFIC_POPEN _popen
#define PLATFORM_SPECIFIC_PCLOSE _pclose
#else
#define PLATFORM_SPECIFIC_POPEN popen
#define PLATFORM_SPECIFIC_PCLOSE pclose
#endif

ExecuteTask::ExecuteTask(const boost::uuids::uuid &id, std::string command)
    : id(id), command(std::move(command)) {}
Result ExecuteTask::run() const {
  std::string result;
  try {
    std::array<char, 128> buffer{};
    std::unique_ptr<FILE, decltype(&PLATFORM_SPECIFIC_PCLOSE)> pipe{ PLATFORM_SPECIFIC_POPEN(command.c_str(), "r"), PLATFORM_SPECIFIC_PCLOSE };
    if (!pipe)
      throw std::runtime_error("Failed to open pipe.");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
    return Result{id, std::move(result), true};
  } catch (const std::exception &e) {
    return Result{id, e.what(), false};
  }
}

ConfigureTask::ConfigureTask(const boost::uuids::uuid &id, double mean_dwell,
                             bool is_running,
                             std::function<void(const Configuration &)> setter)
    : id{id}, mean_dwell{mean_dwell}, is_running{is_running}, setter{std::move(
                                                                  setter)} {}
Result ConfigureTask::run() const {
  setter(Configuration{mean_dwell, is_running});
  return Result{id, "Configured.", true};
}
Configuration::Configuration(const double meanDwell, const bool isRunning)
    : mean_dwell(meanDwell), is_running(isRunning) {}

DeleteTask::DeleteTask(const boost::uuids::uuid &id, std::string path)
    : id{id}, path{std::move(path)} {}

Result DeleteTask::run() const {
  try {
    std::filesystem::remove(path);
    return Result{id, "Removed.", true};
  } catch (const std::exception &e) {
    return Result{id, e.what(), false};
  }
}