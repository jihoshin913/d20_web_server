cmake_minimum_required(VERSION 3.10.0)
project(d20)

# Make sure developers do not run cmake in the main project directory, to keep
# build artifacts from becoming clutter
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed.
    Please make a new directory (called a build directory) and run CMake from there.
    You may need to remove CMakeCache.txt." )
endif()

# Turn on debug builds if we are building in a devel shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    message(STATUS "Setting debug build type by default in devel shell")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output binaries to a sub directory "bin"
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable cmake testing
include(CTest)
enable_testing()

# Enable GoogleTest
include(GoogleTest)
add_subdirectory(/usr/src/googletest googletest)

# Enable Boost
# Use static libraries so binaries can be deployed without a full boost install
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system filesystem log_setup log regex)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

# TODO(!): Update name and srcs
add_library(logger src/logger.cc)
add_library(config_parser src/config_parser.cc)
add_library(http src/http_response.cc src/http_helper.cc src/http_request.cc src/path_router.cc)
add_library(request_handler src/echo_handler.cc src/file_handler.cc src/handler_factory.cc src/not_found_handler.cc src/crud_handler.cc src/health_handler.cc src/sleep_handler.cc src/mock_filesystem.cc)
add_library(server_lib src/server.cc src/session.cc src/server_config.cc)
target_link_libraries(server_lib http request_handler)
add_library(filesys src/mock_filesystem.cc)
target_link_libraries(filesys
    PUBLIC
        Boost::log
        Boost::log_setup
        Boost::system
        Boost::filesystem
        Boost::regex
)

add_executable(server src/server_main.cc)
target_link_libraries(server logger config_parser server_lib filesys Boost::system Boost::filesystem 
                      Boost::regex Boost::log_setup Boost::log)

add_executable(config_parser_main src/config_parser_main.cc)
target_link_libraries(config_parser_main config_parser)

add_executable(unit_tests 
    tests/config_parser_test.cc
    tests/http_helper_test.cc
    tests/server_config_test.cc
    tests/http_request_test.cc
    tests/http_response_test.cc
    tests/echo_handler_test.cc
    tests/file_handler_test.cc
    tests/path_router_test.cc
    tests/handler_factory_test.cc
    tests/not_found_handler_test.cc
    tests/mock_file_system_test.cc
    tests/crud_handler_test.cc
    tests/health_handler_test.cc
)
target_link_libraries(unit_tests gtest_main config_parser http server_lib filesys)

gtest_discover_tests(unit_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME integration_test COMMAND bash ${CMAKE_SOURCE_DIR}/tests/integration_test.sh)
add_test(NAME multithreading_integration_test COMMAND bash ${CMAKE_SOURCE_DIR}/tests/mit.sh)

# Update with target/test targets
include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(TARGETS config_parser http request_handler server_lib filesys TESTS unit_tests)
