# Contributor Documentation
####  Team d20: Jiho Shin, Tony Yu, Marco, Nikolas

## Build, Test, Run
### Build
```bash
mkdir build
cd build
cmake ..
make
```
### Test
```bash
cd build
make test
```
For unit tests, run
```bash
cd tests
../build/bin/config_parser_test
```
For the integration test, run
```bash
./tests/integration_test.sh
```

For the multi-threading integration test, run
```bash
./tests/mit.sh
```

### Run
To run locally, use
```bash
cd build
./bin/server ../configs/example_config 
```
then start a new terminal within the same docker environment

## Architecture Overview

### Multi-threaded Server Design

The server implements true concurrent request handling using Boost.Asio's thread pool architecture:

- **Thread Pool**: Multiple worker threads (default: hardware concurrency) all call `io_service.run()`
- **Automatic Load Balancing**: Boost.Asio distributes incoming connections across available threads
- **Thread Safety**: Session objects use `std::shared_ptr` and `enable_shared_from_this` to ensure safe lifetime management across threads
- **Non-blocking I/O**: All network operations are asynchronous, allowing efficient handling of many concurrent connections

#### Key Implementation Details

**Server Initialization (main.cc)**:
```cpp
// Create thread pool based on hardware concurrency
const size_t num_threads = std::thread::hardware_concurrency() > 0 
                            ? std::thread::hardware_concurrency() 
                            : 4;

std::vector<std::thread> thread_pool;
for (size_t i = 0; i < num_threads; ++i) {
    thread_pool.emplace_back([&io_service]() {
        io_service.run();  // Each thread processes async operations
    });
}
```

**Thread-Safe Session Management (session.h/cc)**:
- Sessions inherit from `std::enable_shared_from_this<Session>` 
- All async callbacks capture `shared_from_this()` to extend session lifetime
- No raw `delete this` - `shared_ptr` reference counting handles cleanup automatically
- Prevents race conditions and double-delete bugs in multi-threaded environment

**Why This Design**:
- Leverages Boost.Asio's proven thread-safety guarantees
- No manual mutex/lock management needed for session lifetime
- Scales with available CPU cores
- Handles blocking operations (like file I/O) without blocking other requests

## File Explanation
### config_parser.h
Implements parsing logic for Nginx-style configuration files.
Defines NginxConfigStatement to represent individual config statements, NginxConfig to represent the full parsed configuration, and NginxConfigParser to read and validate configuration files from streams or filenames.
Used by the server and handler factory to interpret routing rules, ports, and handler parameters.

### echo_handler.h
Defines the EchoHandler class, a concrete implementation of RequestHandler that returns the incoming HTTP request as the response body.
Used primarily for testing and debugging to verify that requests are correctly received and parsed by the server.

### file_handler.h
Defines the FileHandler class, a RequestHandler implementation that serves static files from a specified root directory.
Supports customizable route prefixes and file type filtering via allowed extensions.
Includes helper methods for MIME type detection, path sanitization, and file type validation to ensure secure and correct file delivery.

### handler_factory.h
Defines the HandlerFactory class responsible for creating instances of RequestHandler subclasses based on configuration data.
Provides create_handler() to construct the appropriate handler for a given path and configuration, and parse_extensions() to process comma-separated file extension strings used by file-related handlers.

### logger.h
Defines the Logger class, a centralized logging utility built on the Boost.Log framework.
Provides severity-based logging methods (Trace, Debug, Warning, Error, etc.) and helper functions for server initialization and HTTP request tracing.
Implements a singleton pattern (getLogger()) to ensure consistent logging across all components of the server.

### not_found_handler.h
Defines the NotFoundHandler class, a RequestHandler implementation that generates a 404 Not Found response when a requested resource or route does not exist.
Used as a fallback handler to ensure that all invalid or unmapped requests receive a proper HTTP error response.

### crud_handler.h
Defines the CrudHandler class, a RequestHandler implementation that provides a full CRUD (Create, Read, Update, Delete) API with list functionality.
Supports arbitrary entity types (e.g., "Shoes", "Books") with independent ID spaces, allowing the same ID to exist for different entity types.
Uses a FilesystemInterface backend for persistent storage, making it configurable with different storage implementations (filesystem-based or mock for testing).
Implements all five operations: POST (create), GET with ID (read), GET without ID (list), PUT (update), and DELETE (delete).

### sleep_handler.h
Defines the SleepHandler class, a RequestHandler implementation used for testing concurrent request handling.
Blocks for a configurable duration (default 5 seconds) before returning a response, allowing integration tests to verify that the server handles multiple simultaneous requests without blocking.
Essential for proving the multi-threaded architecture works correctly.

### path_router.h
Defines the PathRouter class, which maps incoming request paths to the appropriate RequestHandler based on server configuration.
Uses HandlerFactory to instantiate handlers and maintains a route map of path prefixes to HandlerConfig objects for efficient lookup and dispatching.

### request_handler.h
Defines the abstract base class RequestHandler, which all handlers must inherit from.
Each handler implements handle_request(const HttpRequest&) to generate an HttpResponse and get_handler_name() to return a unique identifier.

### server_config.h
Defines the ServerConfig class and HandlerConfig struct, which store parsed server configuration data.
ServerConfig loads configuration details from an Nginx-style config file using NginxConfigParser, including the server port and route-to-handler mappings.
Each HandlerConfig holds a handler type and key-value settings specific to that handler.

### server.h
Defines the Server class, which manages TCP connections and delegates incoming HTTP requests to the appropriate handlers.
Uses Boost.Asio for asynchronous networking, accepting new client sessions and routing requests via a shared PathRouter instance.
Serves as the main entry point for starting and managing the web server's lifecycle.
Creates Session objects as `shared_ptr` for thread-safe lifetime management.

### session.h
Defines the Session class, which manages an individual client connection using Boost.Asio.
Handles asynchronous reading and writing of HTTP data, buffers incoming requests, and delegates processing to the appropriate RequestHandler through the PathRouter.
Encapsulates per-connection state and communication logic within the server.
Inherits from `std::enable_shared_from_this<Session>` to enable safe use across multiple threads without race conditions.

## Request Handlers
### Adding a New Request Handler

`EchoHandler` is one of the simplest and most useful examples of how to implement a request handler.
It inherits from `RequestHandler` and responds by returning the exact contents of the incoming HTTP request in the response body.
This makes it perfect for testing the server's request parsing, connection handling, and end-to-end functionality.

### The `RequestHandler` Interface

All handlers inherit from `RequestHandler`, defined in **`request_handler.h`**:

```cpp
class RequestHandler {
 public:
    virtual ~RequestHandler() = default;

    virtual HttpResponse handle_request(const HttpRequest& request) = 0;
    virtual std::string get_handler_name() const = 0;
};
```

Every derived handler must implement:

* `handle_request()` — processes the incoming `HttpRequest` and returns an `HttpResponse`.
* `get_handler_name()` — returns a string identifier for logging and debugging.

### The EchoHandler Definition

**Header: `echo_handler.h`**

```cpp
#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "http_request.h"
#include "http_response.h"
#include "request_handler.h"

// Handles requests containing "echo" in their path.
// Returns the full HTTP request as the response body.
class EchoHandler : public RequestHandler {
 public:
    HttpResponse handle_request(const HttpRequest& request) override;
    std::string get_handler_name() const override;
};

#endif
```

### The EchoHandler Implementation

**Source: `echo_handler.cc`**

```cpp
#include "echo_handler.h"
#include "logger.h"

HttpResponse EchoHandler::handle_request(const HttpRequest& request) {
    HttpResponse response;
    response.status_code = 200;
    response.reason_phrase = "OK";

    // Convert the incoming request to a string and echo it back.
    response.body = request.to_string();  
    response.headers["Content-Type"] = "text/plain";
    return response;
}

std::string EchoHandler::get_handler_name() const {
    return "EchoHandler";
}
```

### How the Server Uses It

1. The **configuration file** maps a URL path prefix to the `EchoHandler`:

   ```nginx
   location /echo {
    handler EchoHandler;
    }
   ```

2. When a client sends a request like:

   ```
   GET /echo HTTP/1.1
   Host: localhost:8080
   ```

   the server matches `/echo` to the `EchoHandler` route.

3. The `HandlerFactory` creates an instance of `EchoHandler`:

   ```cpp
   if (config.type == "EchoHandler") {
       return std::make_unique<EchoHandler>();
   }
   ```

4. The server calls `handle_request()`, which returns an `HttpResponse` containing the full request string as its body.

5. The client receives the echoed request data as the HTTP response.

### Example Response

**Request:**

```
GET /echo HTTP/1.1
Host: localhost:8080
User-Agent: curl/8.0
Accept: */*
```

**Response:**

```
HTTP/1.1 200 OK
Content-Type: text/plain

GET /echo HTTP/1.1
Host: localhost:8080
User-Agent: curl/8.0
Accept: */*
```

### Why It's Useful

* **Simple to test:** Confirms that the entire request pipeline (socket read → parse → handler → response → write) works correctly.
* **Great reference:** Demonstrates all steps required to create a functioning handler.
* **Debug tool:** Useful for verifying that HTTP requests are being received as expected.

### How to Use It as a Template

When creating a new handler, follow the same structure:

1. Define a subclass of `RequestHandler`.
2. Implement `handle_request()` and `get_handler_name()`.
3. Register it in `HandlerFactory`.
4. Add a route in the server config.
5. Test it by sending a request to the configured path.

### Best Practices

* Keep handlers **stateless** — avoid persistent member variables unless needed.
* Always set an appropriate `Content-Type` header.
* Handle invalid inputs gracefully by returning 4xx or 5xx responses.
* Reuse utility classes like `Logger` for consistent debugging output.
* Document your handler with clear comments explaining its purpose and behavior.
* **Thread Safety**: Handlers should be stateless or use proper synchronization. Multiple threads may call `handle_request()` simultaneously on different handler instances.

## CRUD API Handler

The `CrudHandler` implements a RESTful CRUD API that allows clients to create, read, update, delete, and list entities stored on the server.

### API Endpoints

The CRUD handler listens on a configurable route prefix (typically `/api`) and supports the following operations:

#### 1. Create Entity (POST)
**Endpoint:** `POST /api/<Entity>`

Creates a new entity with auto-generated ID.

**Request:**
```http
POST /api/Shoes HTTP/1.1
Content-Type: application/json

{"name": "Running Shoes", "price": 99.99}
```

**Response:**
```http
HTTP/1.1 201 Created
Content-Type: application/json

{"id": 1}
```

#### 2. Read Entity (GET with ID)
**Endpoint:** `GET /api/<Entity>/<id>`

Retrieves a specific entity by ID.

**Request:**
```http
GET /api/Shoes/1 HTTP/1.1
```

**Response:**
```http
HTTP/1.1 200 OK
Content-Type: application/json

{"name": "Running Shoes", "price": 99.99}
```

#### 3. List Entity IDs (GET without ID)
**Endpoint:** `GET /api/<Entity>`

Returns a JSON array of all existing IDs for the entity type.

**Request:**
```http
GET /api/Shoes HTTP/1.1
```

**Response:**
```http
HTTP/1.1 200 OK
Content-Type: application/json

["1", "2", "3"]
```

#### 4. Update Entity (PUT)
**Endpoint:** `PUT /api/<Entity>/<id>`

Updates an existing entity with new JSON data.

**Request:**
```http
PUT /api/Shoes/1 HTTP/1.1
Content-Type: application/json

{"name": "Updated Shoes", "price": 149.99}
```

**Response:**
```http
HTTP/1.1 200 OK
Content-Type: application/json

{"name": "Updated Shoes", "price": 149.99}
```

#### 5. Delete Entity (DELETE)
**Endpoint:** `DELETE /api/<Entity>/<id>`

Deletes an entity by ID.

**Request:**
```http
DELETE /api/Shoes/1 HTTP/1.1
```

**Response:**
```http
HTTP/1.1 200 OK
Content-Type: text/plain

Entity deleted successfully
```

### Entity Types and ID Spaces

The API supports multiple entity types, each with its own independent ID space. For example:
- `GET /api/Shoes/1` and `GET /api/Books/1` can return different objects
- IDs are unique within an entity type but can overlap across different types
- Entity types are defined by their name in the URL path

### Error Responses

- **400 Bad Request**: Invalid path format, missing required ID, or ID in path when not allowed
- **404 Not Found**: Entity or ID does not exist
- **500 Internal Server Error**: Filesystem operation failed

### Configuration

The CrudHandler requires:
- A route prefix (e.g., `/api`)
- A `FilesystemInterface` implementation for storage
- Optional `data_path` parameter for filesystem-based storage root directory

### Example Usage

```cpp
// Create handler with filesystem backend
auto filesystem = std::make_shared<RealFilesystem>("/path/to/data");
CrudHandler handler("/api", filesystem);

// Handle requests
HttpRequest request;
request.set_method("POST");
request.set_path("/api/Products");
request.set_body("{\"name\": \"Laptop\"}");
HttpResponse response = handler.handle_request(request);
```

## Multi-Threading Testing

### SleepHandler for Concurrency Testing

The `SleepHandler` is specifically designed to test the server's ability to handle concurrent requests. It blocks for a configurable duration before responding, allowing tests to verify that other requests can be processed simultaneously.

**Configuration Example:**
```nginx
location /sleep {
    handler SleepHandler;
    sleep_seconds 5;
}
```

### Multi-Threading Integration Test

The test script `mit.sh` proves concurrent request handling through four key tests:

#### Test 1: SleepHandler Blocks Correctly
Verifies that the sleep handler blocks for the expected duration (~5 seconds).

#### Test 2: Simultaneous Requests (Core Test)
- Starts a slow 5-second request to `/sleep` in the background
- Immediately starts a fast request to `/echo`
- **Verifies the fast request completes in < 2 seconds**
- This proves requests are handled concurrently, not sequentially

#### Test 3: Multiple Simultaneous Slow Requests
- Launches 3 slow requests in parallel
- If sequential: would take 15+ seconds (3 × 5)
- If concurrent: takes ~5 seconds
- Verifies total time is 4-10 seconds

#### Test 4: Interleaved Fast and Slow Requests
- Pattern: slow, fast, slow, fast
- Verifies both fast requests complete quickly despite slow requests running
- Proves complex interleaving scenarios work correctly

**Running the test:**
```bash
cd tests
./test_multithreading.sh
```

**Expected output when passing:**
```
========== Test 1: SleepHandler Blocks Correctly ==========
Test 1 passed - SleepHandler blocks correctly

========== Test 2: Server Handles Simultaneous Requests ==========
Fast request completed in 0 seconds
Test 2 passed - server handles simultaneous requests correctly

========== Test 3: Multiple Simultaneous Slow Requests ==========
All 3 requests completed in 5 seconds total
Test 3 passed - multiple simultaneous requests handled concurrently

========== Test 4: Interleaved Fast and Slow Requests ==========
Test 4 passed - interleaved requests handled correctly

All multi-threading tests passed!
Server successfully handles concurrent requests
```

## Source Code Structure
Here is the Folder layout of the source code, we ignored the .git folder, build folder, and any insignificant folder/file.

```
📦d20
 ┣ 📂configs
 ┃ ┣ 📜dev_config
 ┃ ┣ 📜example_config
 ┃ ┗ 📜server_config
 ┣ 📂docker
 ┃ ┣ 📜base.Dockerfile
 ┃ ┣ 📜cloudbuild.yaml
 ┃ ┣ 📜coverage.Dockerfile
 ┃ ┗ 📜Dockerfile
 ┣ 📂include
 ┃ ┣ 📜config_parser.h
 ┃ ┣ 📜crud_handler.h
 ┃ ┣ 📜echo_handler.h
 ┃ ┣ 📜file_handler.h
 ┃ ┣ 📜filesystem_interface.h
 ┃ ┣ 📜handler_factory.h
 ┃ ┣ 📜header.h
 ┃ ┣ 📜health_handler.h
 ┃ ┣ 📜http_helper.h
 ┃ ┣ 📜http_request.h
 ┃ ┣ 📜http_response.h
 ┃ ┣ 📜logger.h
 ┃ ┣ 📜mock_filesystem.h
 ┃ ┣ 📜not_found_handler.h
 ┃ ┣ 📜path_router.h
 ┃ ┣ 📜request_handler.h
 ┃ ┣ 📜request.h
 ┃ ┣ 📜server_config.h
 ┃ ┣ 📜server.h
 ┃ ┣ 📜session.h
 ┃ ┗ 📜sleep_handler.h
 ┣ 📂src
 ┃ ┣ 📜config_parser_main.cc
 ┃ ┣ 📜config_parser.cc
 ┃ ┣ 📜crud_handler.cc
 ┃ ┣ 📜echo_handler.cc
 ┃ ┣ 📜file_handler.cc
 ┃ ┣ 📜handler_factory.cc
 ┃ ┣ 📜health_handler.cc
 ┃ ┣ 📜http_helper.cc
 ┃ ┣ 📜http_request.cc
 ┃ ┣ 📜http_response.cc
 ┃ ┣ 📜logger.cc
 ┃ ┣ 📜mock_filesystem.cc
 ┃ ┣ 📜not_found_handler.cc
 ┃ ┣ 📜path_router.cc
 ┃ ┣ 📜server_config.cc
 ┃ ┣ 📜server_main.cc
 ┃ ┣ 📜server.cc
 ┃ ┣ 📜session.cc
 ┃ ┗ 📜sleep_handler.cc
 ┣ 📂static_content
 ┃ ┗ 📜index.html
 ┣ 📂tests
 ┃ ┣ 📜config_parser_test.cc
 ┃ ┣ 📜crud_handler_test.cc
 ┃ ┣ 📜echo_handler_test.cc
 ┃ ┣ 📜file_handler_test.cc
 ┃ ┣ 📜handler_factory_test.cc
 ┃ ┣ 📜health_handler_test.cc
 ┃ ┣ 📜http_helper_test.cc
 ┃ ┣ 📜http_request_test.cc
 ┃ ┣ 📜http_response_test.cc
 ┃ ┣ 📜mock_file_system_test.cc
 ┃ ┣ 📜not_found_handler_test.cc
 ┃ ┣ 📜path_router_test.cc
 ┃ ┣ 📜server_config_test.cc
 ┃ ┣ 📜integration_test.sh
 ┃ ┗ 📜mit.sh
 ┣ 📜CMakeLists.txt
 ┗ 📜README.md
```