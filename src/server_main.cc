//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "server.h"
#include "server_config.h"
#include "path_router.h"
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include "server.h"
#include "logger.h"

int main(int argc, char* argv[])
{
  try
  {
    Logger *logger = Logger::getLogger();
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <config_file>\n";
      logger->logErrorFile("wrong usage server config is needed");
      return 1;
    }

    // Parse nginx config
    NginxConfigParser parser;
    NginxConfig config;

    if (!parser.Parse(argv[1], &config)) {
      logger->logErrorFile("Failed to parse config file");
      return -1;
    }

    // Load server configuration
    ServerConfig server_config;
    if (!server_config.load_from_nginx_config(config)) {
      logger->logErrorFile("Invalid server configuration");
      return -1;
    }

    // Initialize router with config
    auto router = std::make_shared<PathRouter>(server_config);

    // Start server
    boost::asio::io_service io_service;

    using namespace std; // For atoi.

    Server s(io_service, server_config.get_port(), router);
    logger->logServerInitialization();
    logger->logTraceFile("Starting server on port: " + std::to_string(server_config.get_port()));

    size_t detected = std::thread::hardware_concurrency();
    if (detected == 0) detected = 1;

    const size_t num_threads = std::max<size_t>(4, detected);

    logger->logTraceFile("Starting " + std::to_string(num_threads) + " worker threads");
    
    std::vector<std::thread> thread_pool;
    
    // Create worker threads that all call io_service.run()
    // Boost.Asio will automatically distribute work among them
    for (size_t i = 0; i < num_threads; ++i) {
      thread_pool.emplace_back([&io_service]() {
        io_service.run();
      });
    }
    
    // Wait for all threads to complete
    for (auto& thread : thread_pool) {
      thread.join();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}