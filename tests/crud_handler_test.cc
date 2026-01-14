// crud_handler_test.cc
// ideas aided by gpt
#include "gtest/gtest.h"
#include "crud_handler.h"
#include "mock_filesystem.h"
#include "http_request.h"
#include "http_response.h"
#include <memory>

class CrudHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        filesystem_ = std::make_shared<MockFilesystem>();
        route_prefix_ = "/api";
        handler_ = std::make_unique<CrudHandler>(route_prefix_, filesystem_);
        
        // Set up some test data
        Entity shoes("Shoes");
        Entity books("Books");
        
        filesystem_->write_entity(shoes, "1", "{\"name\": \"Nike Running Shoes\", \"price\": 99.99}");
        filesystem_->write_entity(shoes, "2", "{\"name\": \"Moon boots\", \"price\": 200.00}");
        filesystem_->write_entity(books, "1", "{\"title\": \"The Florentine Deception: A Novel\", \"author\": \"Carey Nachenberg\"}");
    }
    
    void TearDown() override {
        filesystem_->reset();
    }
    
    HttpRequest create_get_request(const std::string& path) {
        HttpRequest request;
        request.set_method("GET");
        request.set_path(path);
        request.set_version("HTTP/1.1");
        return request;
    }
    
    HttpRequest create_post_request(const std::string& path, const std::string& body = "") {
        HttpRequest request;
        request.set_method("POST");
        request.set_path(path);
        request.set_version("HTTP/1.1");
        request.set_body(body);
        return request;
    }
    
    HttpRequest create_put_request(const std::string& path, const std::string& body = "") {
        HttpRequest request;
        request.set_method("PUT");
        request.set_path(path);
        request.set_version("HTTP/1.1");
        request.set_body(body);
        return request;
    }
    
    HttpRequest create_delete_request(const std::string& path) {
        HttpRequest request;
        request.set_method("DELETE");
        request.set_path(path);
        request.set_version("HTTP/1.1");
        return request;
    }
    
    std::shared_ptr<MockFilesystem> filesystem_;
    std::string route_prefix_;
    std::unique_ptr<CrudHandler> handler_;
};

class CrudHandlerFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        filesystem_ = std::make_shared<MockFilesystem>();
        handler = std::make_unique<CrudHandler>("/api", filesystem_);
        
        filesystem_->write_entity(
            Entity{"file_data"}, 
            "1", 
            "{\"name\":\"d20 Project Documentation\",\"tag\":\"d20\",\"upload_date\":\"2024-11-21T10:00:00Z\",\"file_id\":\"1\"}"
        );
        
        filesystem_->write_entity(
            Entity{"file_data"}, 
            "2", 
            "{\"name\":\"API Reference Guide\",\"tag\":\"documentation\",\"upload_date\":\"2024-11-21T11:00:00Z\",\"file_id\":\"2\"}"
        );
        
        filesystem_->write_entity(
            Entity{"file_data"}, 
            "3", 
            "{\"name\":\"d20 Testing Strategy\",\"tag\":\"d20\",\"upload_date\":\"2024-11-21T12:00:00Z\",\"file_id\":\"3\"}"
        );
    }
    
    std::shared_ptr<MockFilesystem> filesystem_;
    std::unique_ptr<CrudHandler> handler;
};

// Test: GET request for existing entity with ID returns 200
TEST_F(CrudHandlerTest, GetExistingEntityById) {
    HttpRequest request = create_get_request("/api/Shoes/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_reason_phrase(), "OK");
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    EXPECT_EQ(response.get_message_body(), "{\"name\": \"Nike Running Shoes\", \"price\": 99.99}");
}

// Test: GET request for non-existent entity returns 404
TEST_F(CrudHandlerTest, GetNonExistentEntityById) {
    HttpRequest request = create_get_request("/api/Shoes/999");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_header("Content-Type"), "text/html");
    EXPECT_EQ(response.get_message_body(), "Entity not found");
}

// Test: GET request for different entity type with same ID
TEST_F(CrudHandlerTest, GetDifferentEntityTypeWithSameId) {
    HttpRequest request = create_get_request("/api/Books/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    EXPECT_EQ(response.get_message_body(), "{\"title\": \"The Florentine Deception: A Novel\", \"author\": \"Carey Nachenberg\"}");
    
    // Verify Shoes/1 is different from Books/1
    HttpRequest shoes_request = create_get_request("/api/Shoes/1");
    HttpResponse shoes_response = handler_->handle_request(shoes_request);
    EXPECT_NE(shoes_response.get_message_body(), response.get_message_body());
}

// Test: GET request for another existing entity
TEST_F(CrudHandlerTest, GetAnotherExistingEntity) {
    HttpRequest request = create_get_request("/api/Shoes/2");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    EXPECT_EQ(response.get_message_body(), "{\"name\": \"Moon boots\", \"price\": 200.00}");
}

// Test: GET request with invalid path (no entity name)
TEST_F(CrudHandlerTest, GetWithInvalidPathNoEntity) {
    HttpRequest request = create_get_request("/api");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "Invalid CRUD path");
}

// Test: GET request with invalid path (extra slashes)
TEST_F(CrudHandlerTest, GetWithInvalidPathExtraSlashes) {
    HttpRequest request = create_get_request("/api/Shoes/1/extra");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "Invalid CRUD path");
}

// Test: GET request with different route prefix
TEST_F(CrudHandlerTest, GetWithDifferentRoutePrefix) {
    CrudHandler custom_handler("/custom", filesystem_);
    HttpRequest request = create_get_request("/custom/Shoes/1");
    HttpResponse response = custom_handler.handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
}

// Test: GET request with empty route prefix
TEST_F(CrudHandlerTest, GetWithEmptyRoutePrefix) {
    CrudHandler empty_prefix_handler("", filesystem_);
    HttpRequest request = create_get_request("/Shoes/1");
    HttpResponse response = empty_prefix_handler.handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
}

// Test: GET request for entity type that doesn't exist
TEST_F(CrudHandlerTest, GetNonExistentEntityType) {
    HttpRequest request = create_get_request("/api/NonExistent/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
}

// Test: GET request with numeric ID
TEST_F(CrudHandlerTest, GetWithNumericId) {
    Entity test_entity("TestEntity");
    filesystem_->write_entity(test_entity, "42", "{\"value\": 42}");
    
    HttpRequest request = create_get_request("/api/TestEntity/42");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), "{\"value\": 42}");
}

// Test: GET request with string ID
TEST_F(CrudHandlerTest, GetWithStringId) {
    Entity test_entity("TestEntity");
    filesystem_->write_entity(test_entity, "abc123", "{\"id\": \"abc123\"}");
    
    HttpRequest request = create_get_request("/api/TestEntity/abc123");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), "{\"id\": \"abc123\"}");
}

// Test: GET request with complex JSON data
TEST_F(CrudHandlerTest, GetWithComplexJsonData) {
    Entity test_entity("TestEntity");
    std::string complex_json = "{\"name\": \"Test\", \"items\": [1, 2, 3], \"nested\": {\"key\": \"value\"}}";
    filesystem_->write_entity(test_entity, "1", complex_json);
    
    HttpRequest request = create_get_request("/api/TestEntity/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), complex_json);
}

// Test: GET request with empty JSON data
TEST_F(CrudHandlerTest, GetWithEmptyJsonData) {
    Entity test_entity("TestEntity");
    filesystem_->write_entity(test_entity, "1", "{}");
    
    HttpRequest request = create_get_request("/api/TestEntity/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), "{}");
}

// Test: GET request preserves HTTP version
TEST_F(CrudHandlerTest, GetPreservesHttpVersion) {
    HttpRequest request = create_get_request("/api/Shoes/1");
    request.set_version("HTTP/1.1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_version(), "HTTP/1.1");
}

// Test: GET request with path that doesn't match prefix
TEST_F(CrudHandlerTest, GetWithPathNotMatchingPrefix) {
    HttpRequest request = create_get_request("/other/Shoes/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
}

// Test: GET request with trailing slash in prefix
TEST_F(CrudHandlerTest, GetWithTrailingSlashInPrefix) {
    CrudHandler trailing_slash_handler("/api/", filesystem_);
    HttpRequest request = create_get_request("/api/Shoes/1");
    HttpResponse response = trailing_slash_handler.handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
}

// Test: GET request with entity name containing special characters
TEST_F(CrudHandlerTest, GetWithSpecialCharactersInEntityName) {
    Entity special_entity("Test-Entity_123");
    filesystem_->write_entity(special_entity, "1", "{\"test\": \"data\"}");
    
    HttpRequest request = create_get_request("/api/Test-Entity_123/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), "{\"test\": \"data\"}");
}

// Test: Multiple GET requests for same entity
TEST_F(CrudHandlerTest, MultipleGetRequestsForSameEntity) {
    HttpRequest request1 = create_get_request("/api/Shoes/1");
    HttpResponse response1 = handler_->handle_request(request1);
    
    HttpRequest request2 = create_get_request("/api/Shoes/1");
    HttpResponse response2 = handler_->handle_request(request2);
    
    EXPECT_EQ(response1.get_status_code(), 200);
    EXPECT_EQ(response2.get_status_code(), 200);
    EXPECT_EQ(response1.get_message_body(), response2.get_message_body());
}

// Test: GET request without ID lists all entity IDs
TEST_F(CrudHandlerTest, GetListsAllEntityIds) {
    HttpRequest request = create_get_request("/api/Shoes");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_reason_phrase(), "OK");
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    
    std::string body = response.get_message_body();
    // Should be a JSON array containing "1" and "2"
    EXPECT_TRUE(body.find("\"1\"") != std::string::npos);
    EXPECT_TRUE(body.find("\"2\"") != std::string::npos);
    EXPECT_TRUE(body.find("[") == 0);
    EXPECT_TRUE(body.find("]") == body.length() - 1);
}

//potential other tests:  get list after creating entities,
//                         get list for empty entity type,
//                         get list after deleting entity
//                         get list with invalid path (no entity name),
//                         get list with path not matching prefix,
//                         get list with entity name containing special characters,
//                         get list response contains valid JSON array,

// Test: GET list for entity type with no stored entities returns empty JSON array
TEST_F(CrudHandlerTest, GetListForEmptyEntityType) {
    // "Toys" has never been written to
    HttpRequest request = create_get_request("/api/Toys");
    HttpResponse response = handler_->handle_request(request);

    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_reason_phrase(), "OK");
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");

    std::string body = response.get_message_body();
    EXPECT_EQ(body, "[]");
}

// Test: GET list after creating new entities via POST
TEST_F(CrudHandlerTest, GetListAfterCreatingEntitiesWithPost) {

    std::string new_shoe_json = "{\"name\": \"Trail shoe\", \"price\": 150.00}";
    HttpRequest post_request = create_post_request("/api/Shoes", new_shoe_json);
    HttpResponse post_response = handler_->handle_request(post_request);
    EXPECT_EQ(post_response.get_status_code(), 201);

    // list all shoes
    HttpRequest list_request = create_get_request("/api/Shoes");
    HttpResponse list_response = handler_->handle_request(list_request);

    EXPECT_EQ(list_response.get_status_code(), 200);
    EXPECT_EQ(list_response.get_header("Content-Type"), "application/json");

    std::string body = list_response.get_message_body();
    
    EXPECT_NE(body.find("\"1\""), std::string::npos);
    EXPECT_NE(body.find("\"2\""), std::string::npos);
    EXPECT_NE(body.find("\"3\""), std::string::npos);

    //Expect three ids
    size_t comma_count = std::count(body.begin(), body.end(), ',');
    EXPECT_GE(comma_count, 2);
}

// Test: GET list after deleting an entity should not include deleted ID
TEST_F(CrudHandlerTest, GetListAfterDeletingEntity) {
    // Delete Shoes/1 first
    HttpRequest delete_request = create_delete_request("/api/Shoes/1");
    HttpResponse delete_response = handler_->handle_request(delete_request);
    EXPECT_EQ(delete_response.get_status_code(), 200);

    // Now list Shoes IDs
    HttpRequest list_request = create_get_request("/api/Shoes");
    HttpResponse list_response = handler_->handle_request(list_request);
    EXPECT_EQ(list_response.get_status_code(), 200);

    std::string body = list_response.get_message_body();
    // "1" should be gone; "2" should still be present
    EXPECT_EQ(body.find("\"1\""), std::string::npos);
    EXPECT_NE(body.find("\"2\""), std::string::npos);
}

// Test: GET list for entity type that doesn't exist yet is also empty array
TEST_F(CrudHandlerTest, GetListForNonExistentEntityType) {
    // "NonExistentEntityType" has never been used
    HttpRequest request = create_get_request("/api/NonExistentEntityType");
    HttpResponse response = handler_->handle_request(request);

    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    EXPECT_EQ(response.get_message_body(), "[]");
}

// Test: GET list includes both numeric and string IDs, all quoted
TEST_F(CrudHandlerTest, GetListWithNumericAndStringIds) {
    Entity mixed("MixedIds");
    filesystem_->write_entity(mixed, "1",   "{\"v\": 1}");
    filesystem_->write_entity(mixed, "007", "{\"v\": 7}");
    filesystem_->write_entity(mixed, "abc", "{\"v\": \"abc\"}");

    HttpRequest request = create_get_request("/api/MixedIds");
    HttpResponse response = handler_->handle_request(request);

    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");

    std::string body = response.get_message_body();
    // All IDs should appear wrapped in quotes somewhere
    EXPECT_NE(body.find("\"1\""),   std::string::npos);
    EXPECT_NE(body.find("\"007\""), std::string::npos);
    EXPECT_NE(body.find("\"abc\""), std::string::npos);

    // Basic JSON array sanity checks
    EXPECT_EQ(body.front(), '[');
    EXPECT_EQ(body.back(),  ']');
}

// Test: GET list with entity name containing special characters
TEST_F(CrudHandlerTest, GetListWithSpecialCharactersInEntityName) {
    Entity special("Test-Entity_123");
    filesystem_->write_entity(special, "id1", "{\"test\": 1}");
    filesystem_->write_entity(special, "id2", "{\"test\": 2}");

    HttpRequest request = create_get_request("/api/Test-Entity_123");
    HttpResponse response = handler_->handle_request(request);

    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");

    std::string body = response.get_message_body();
    EXPECT_NE(body.find("\"id1\""), std::string::npos);
    EXPECT_NE(body.find("\"id2\""), std::string::npos);
}

// Test: GET list with different route prefix works
TEST_F(CrudHandlerTest, GetListWithDifferentRoutePrefix) {
    CrudHandler custom_handler("/custom", filesystem_);

    HttpRequest request = create_get_request("/custom/Shoes");
    HttpResponse response = custom_handler.handle_request(request);

    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");

    std::string body = response.get_message_body();
    EXPECT_NE(body.find("\"1\""), std::string::npos);
    EXPECT_NE(body.find("\"2\""), std::string::npos);
}

// Test: GET list with empty route prefix works
TEST_F(CrudHandlerTest, GetListWithEmptyRoutePrefix) {
    CrudHandler empty_prefix_handler("", filesystem_);

    HttpRequest request = create_get_request("/Shoes");
    HttpResponse response = empty_prefix_handler.handle_request(request);

    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");

    std::string body = response.get_message_body();
    EXPECT_NE(body.find("\"1\""), std::string::npos);
    EXPECT_NE(body.find("\"2\""), std::string::npos);
}

// Test: GET list with path not matching prefix returns 400 (list case)
TEST_F(CrudHandlerTest, GetListWithPathNotMatchingPrefix) {
    HttpRequest request = create_get_request("/other/Shoes");
    HttpResponse response = handler_->handle_request(request);

    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "Invalid CRUD path");
}

// Test: GET list response is a valid-looking JSON array even for single element
TEST_F(CrudHandlerTest, GetListSingleElementJsonArrayShape) {
    Entity single("Single");
    filesystem_->write_entity(single, "only", "{\"x\": 1}");

    HttpRequest request = create_get_request("/api/Single");
    HttpResponse response = handler_->handle_request(request);

    EXPECT_EQ(response.get_status_code(), 200);

    std::string body = response.get_message_body();
    // expect somethin glike ["only"]
    EXPECT_EQ(body.front(), '[');
    EXPECT_EQ(body.back(),  ']');


    size_t first_quote = body.find('\"');
    ASSERT_NE(first_quote, std::string::npos);
    size_t second_quote = body.find('\"', first_quote + 1);
    ASSERT_NE(second_quote, std::string::npos);

    size_t third_quote = body.find('\"', second_quote + 1);
    EXPECT_EQ(third_quote, std::string::npos);
}

//////////////////////////////////////////////////////////////
// POST tests
//////////////////////////////////////////////////////////////

// Test: POST request creates new entity and returns 201
TEST_F(CrudHandlerTest, PostCreatesNewEntity) {
    HttpRequest request = create_post_request("/api/Products", "{\"name\": \"Laptop\", \"price\": 999.99}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 201);
    EXPECT_EQ(response.get_reason_phrase(), "Created");
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    
    // Response should contain an ID
    std::string response_body = response.get_message_body();
    EXPECT_TRUE(response_body.find("\"id\"") != std::string::npos);
    EXPECT_TRUE(response_body.find("1") != std::string::npos || response_body.find("3") != std::string::npos);
}

// Test: POST request returns correct JSON format with ID
TEST_F(CrudHandlerTest, PostReturnsCorrectJsonFormat) {
    HttpRequest request = create_post_request("/api/NewEntity", "{\"test\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 201);
    std::string response_body = response.get_message_body();
    
    // Should be valid JSON with id field
    EXPECT_TRUE(response_body.find("{\"id\":") == 0 || response_body.find("{\"id\": ") == 0);
    EXPECT_TRUE(response_body.back() == '}');
}

// Test: POST with ID in path returns 400
TEST_F(CrudHandlerTest, PostWithIdInPathReturns400) {
    HttpRequest request = create_post_request("/api/Products/1", "{\"name\": \"Laptop\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "POST must not include ID in path");
}

// Test: POST creates entity that can be retrieved with GET
TEST_F(CrudHandlerTest, PostCreatesEntityRetrievableByGet) {
    std::string json_data = "{\"name\": \"Tablet\", \"price\": 499.99}";
    HttpRequest post_request = create_post_request("/api/Devices", json_data);
    HttpResponse post_response = handler_->handle_request(post_request);
    
    EXPECT_EQ(post_response.get_status_code(), 201);
    
    // Extract ID from response (format: {"id": 1})
    std::string response_body = post_response.get_message_body();
    size_t colon_pos = response_body.find(":");
    size_t brace_pos = response_body.find("}");
    std::string id = response_body.substr(colon_pos + 1, brace_pos - colon_pos - 1);
    // Trim whitespace
    id.erase(0, id.find_first_not_of(" \t"));
    id.erase(id.find_last_not_of(" \t") + 1);
    
    // Now GET the entity we just created
    HttpRequest get_request = create_get_request("/api/Devices/" + id);
    HttpResponse get_response = handler_->handle_request(get_request);
    
    EXPECT_EQ(get_response.get_status_code(), 200);
    EXPECT_EQ(get_response.get_message_body(), json_data);
}

// Test: POST assigns sequential IDs
TEST_F(CrudHandlerTest, PostAssignsSequentialIds) {
    Entity test_entity("TestEntity");
    
    HttpRequest request1 = create_post_request("/api/TestEntity", "{\"data\": \"first\"}");
    HttpResponse response1 = handler_->handle_request(request1);
    EXPECT_EQ(response1.get_status_code(), 201);
    
    HttpRequest request2 = create_post_request("/api/TestEntity", "{\"data\": \"second\"}");
    HttpResponse response2 = handler_->handle_request(request2);
    EXPECT_EQ(response2.get_status_code(), 201);
    
    HttpRequest request3 = create_post_request("/api/TestEntity", "{\"data\": \"third\"}");
    HttpResponse response3 = handler_->handle_request(request3);
    EXPECT_EQ(response3.get_status_code(), 201);
    
    // All should have different IDs
    std::string id1 = response1.get_message_body();
    std::string id2 = response2.get_message_body();
    std::string id3 = response3.get_message_body();
    
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

// Test: POST with empty body
TEST_F(CrudHandlerTest, PostWithEmptyBody) {
    HttpRequest request = create_post_request("/api/EmptyEntity", "");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 201);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    
    // Entity should be created with empty body
    std::string response_body = response.get_message_body();
    size_t colon_pos = response_body.find(":");
    size_t brace_pos = response_body.find("}");
    std::string id = response_body.substr(colon_pos + 1, brace_pos - colon_pos - 1);
    // Trim whitespace
    id.erase(0, id.find_first_not_of(" \t"));
    id.erase(id.find_last_not_of(" \t") + 1);
    
    HttpRequest get_request = create_get_request("/api/EmptyEntity/" + id);
    HttpResponse get_response = handler_->handle_request(get_request);
    
    EXPECT_EQ(get_response.get_status_code(), 200);
    EXPECT_EQ(get_response.get_message_body(), "");
}

// Test: POST with complex JSON
TEST_F(CrudHandlerTest, PostWithComplexJson) {
    std::string complex_json = "{\"name\": \"Product\", \"items\": [1, 2, 3], \"nested\": {\"key\": \"value\"}, \"price\": 99.99}";
    HttpRequest request = create_post_request("/api/ComplexEntity", complex_json);
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 201);
    
    // Extract ID and verify we can retrieve the complex JSON
    std::string response_body = response.get_message_body();
    size_t colon_pos = response_body.find(":");
    size_t brace_pos = response_body.find("}");
    std::string id = response_body.substr(colon_pos + 1, brace_pos - colon_pos - 1);
    // Trim whitespace
    id.erase(0, id.find_first_not_of(" \t"));
    id.erase(id.find_last_not_of(" \t") + 1);
    
    HttpRequest get_request = create_get_request("/api/ComplexEntity/" + id);
    HttpResponse get_response = handler_->handle_request(get_request);
    
    EXPECT_EQ(get_response.get_status_code(), 200);
    EXPECT_EQ(get_response.get_message_body(), complex_json);
}

// Test: POST with different entity types creates separate ID spaces
TEST_F(CrudHandlerTest, PostDifferentEntityTypesSeparateIdSpaces) {
    HttpRequest request1 = create_post_request("/api/TypeA", "{\"data\": \"A\"}");
    HttpResponse response1 = handler_->handle_request(request1);
    
    HttpRequest request2 = create_post_request("/api/TypeB", "{\"data\": \"B\"}");
    HttpResponse response2 = handler_->handle_request(request2);
    
    EXPECT_EQ(response1.get_status_code(), 201);
    EXPECT_EQ(response2.get_status_code(), 201);
    
    // Both should get ID "1" since they're different entity types
    std::string id1 = response1.get_message_body();
    std::string id2 = response2.get_message_body();
    
    // Both should start with ID 1 (or 3 if Shoes/Books already exist)
    // The important thing is they can coexist
    EXPECT_TRUE(id1.find("1") != std::string::npos || id1.find("3") != std::string::npos);
    EXPECT_TRUE(id2.find("1") != std::string::npos || id2.find("3") != std::string::npos);
}

// Test: POST preserves HTTP version
TEST_F(CrudHandlerTest, PostPreservesHttpVersion) {
    HttpRequest request = create_post_request("/api/TestEntity", "{\"test\": \"data\"}");
    request.set_version("HTTP/1.1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_version(), "HTTP/1.1");
    EXPECT_EQ(response.get_status_code(), 201);
}

// Test: POST with invalid path (no entity name)
TEST_F(CrudHandlerTest, PostWithInvalidPathNoEntity) {
    HttpRequest request = create_post_request("/api", "{\"test\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "Invalid CRUD path");
}

// Test: POST with path not matching prefix
TEST_F(CrudHandlerTest, PostWithPathNotMatchingPrefix) {
    HttpRequest request = create_post_request("/other/Entity", "{\"test\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
}

// Test: POST with entity name containing special characters
TEST_F(CrudHandlerTest, PostWithSpecialCharactersInEntityName) {
    HttpRequest request = create_post_request("/api/Test-Entity_123", "{\"test\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 201);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
}

// Test: POST response contains valid JSON ID
TEST_F(CrudHandlerTest, PostResponseContainsValidJsonId) {
    HttpRequest request = create_post_request("/api/JsonTest", "{\"value\": 42}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 201);
    std::string body = response.get_message_body();
    
    // Should be valid JSON: {"id": <number>}
    EXPECT_TRUE(body.find("{") != std::string::npos);
    EXPECT_TRUE(body.find("}") != std::string::npos);
    EXPECT_TRUE(body.find("\"id\"") != std::string::npos || body.find("id") != std::string::npos);
}

//////////////////////////////////////////////////////////////
// PUT tests
//////////////////////////////////////////////////////////////

// Test: PUT request updates existing entity and returns 200
TEST_F(CrudHandlerTest, PutUpdatesExistingEntity) {
    std::string updated_data = "{\"name\": \"Updated Shoes\", \"price\": 199.99}";
    HttpRequest request = create_put_request("/api/Shoes/1", updated_data);
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_reason_phrase(), "OK");
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    EXPECT_EQ(response.get_message_body(), updated_data);
}

// Test: PUT updates entity that can be retrieved with GET
TEST_F(CrudHandlerTest, PutUpdatesEntityRetrievableByGet) {
    std::string updated_data = "{\"name\": \"New Name\", \"value\": 123}";
    HttpRequest put_request = create_put_request("/api/Shoes/1", updated_data);
    HttpResponse put_response = handler_->handle_request(put_request);
    
    EXPECT_EQ(put_response.get_status_code(), 200);
    
    // Verify the update by GETting the entity
    HttpRequest get_request = create_get_request("/api/Shoes/1");
    HttpResponse get_response = handler_->handle_request(get_request);
    
    EXPECT_EQ(get_response.get_status_code(), 200);
    EXPECT_EQ(get_response.get_message_body(), updated_data);
}

// Test: PUT with non-existent entity returns 404
TEST_F(CrudHandlerTest, PutNonExistentEntityReturns404) {
    HttpRequest request = create_put_request("/api/Shoes/999", "{\"test\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_header("Content-Type"), "text/html");
    EXPECT_EQ(response.get_message_body(), "Entity not found");
}

// Test: PUT without ID in path returns 400
TEST_F(CrudHandlerTest, PutWithoutIdInPathReturns400) {
    HttpRequest request = create_put_request("/api/Shoes", "{\"test\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "PUT must include ID in path");
}

// Test: PUT with empty body
TEST_F(CrudHandlerTest, PutWithEmptyBody) {
    HttpRequest request = create_put_request("/api/Shoes/1", "");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    EXPECT_EQ(response.get_message_body(), "");
    
    // Verify empty body was stored
    HttpRequest get_request = create_get_request("/api/Shoes/1");
    HttpResponse get_response = handler_->handle_request(get_request);
    EXPECT_EQ(get_response.get_message_body(), "");
}

// Test: PUT with complex JSON
TEST_F(CrudHandlerTest, PutWithComplexJson) {
    std::string complex_json = "{\"name\": \"Product\", \"items\": [1, 2, 3], \"nested\": {\"key\": \"value\"}, \"price\": 99.99}";
    HttpRequest request = create_put_request("/api/Shoes/1", complex_json);
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), complex_json);
    
    // Verify complex JSON was stored
    HttpRequest get_request = create_get_request("/api/Shoes/1");
    HttpResponse get_response = handler_->handle_request(get_request);
    EXPECT_EQ(get_response.get_message_body(), complex_json);
}

// Test: PUT updates different entity types independently
TEST_F(CrudHandlerTest, PutDifferentEntityTypesIndependently) {
    std::string shoes_data = "{\"type\": \"shoes\", \"price\": 50}";
    std::string books_data = "{\"type\": \"book\", \"price\": 20}";
    
    HttpRequest put_shoes = create_put_request("/api/Shoes/1", shoes_data);
    HttpResponse response_shoes = handler_->handle_request(put_shoes);
    
    HttpRequest put_books = create_put_request("/api/Books/1", books_data);
    HttpResponse response_books = handler_->handle_request(put_books);
    
    EXPECT_EQ(response_shoes.get_status_code(), 200);
    EXPECT_EQ(response_books.get_status_code(), 200);
    
    // Verify they are independent
    HttpRequest get_shoes = create_get_request("/api/Shoes/1");
    HttpRequest get_books = create_get_request("/api/Books/1");
    
    HttpResponse get_shoes_response = handler_->handle_request(get_shoes);
    HttpResponse get_books_response = handler_->handle_request(get_books);
    
    EXPECT_EQ(get_shoes_response.get_message_body(), shoes_data);
    EXPECT_EQ(get_books_response.get_message_body(), books_data);
    EXPECT_NE(get_shoes_response.get_message_body(), get_books_response.get_message_body());
}

// Test: PUT preserves HTTP version
TEST_F(CrudHandlerTest, PutPreservesHttpVersion) {
    HttpRequest request = create_put_request("/api/Shoes/1", "{\"test\": \"data\"}");
    request.set_version("HTTP/1.1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_version(), "HTTP/1.1");
    EXPECT_EQ(response.get_status_code(), 200);
}

// Test: PUT with invalid path (no entity name)
TEST_F(CrudHandlerTest, PutWithInvalidPathNoEntity) {
    HttpRequest request = create_put_request("/api", "{\"test\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "Invalid CRUD path");
}

// Test: PUT with path not matching prefix
TEST_F(CrudHandlerTest, PutWithPathNotMatchingPrefix) {
    HttpRequest request = create_put_request("/other/Shoes/1", "{\"test\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
}

// Test: PUT with entity name containing special characters
TEST_F(CrudHandlerTest, PutWithSpecialCharactersInEntityName) {
    Entity special_entity("Test-Entity_123");
    filesystem_->write_entity(special_entity, "1", "{\"initial\": \"data\"}");
    
    HttpRequest request = create_put_request("/api/Test-Entity_123/1", "{\"updated\": \"data\"}");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    EXPECT_EQ(response.get_message_body(), "{\"updated\": \"data\"}");
}

// Test: PUT multiple times on same entity
TEST_F(CrudHandlerTest, PutMultipleTimesOnSameEntity) {
    std::string data1 = "{\"version\": 1}";
    std::string data2 = "{\"version\": 2}";
    std::string data3 = "{\"version\": 3}";
    
    HttpRequest request1 = create_put_request("/api/Shoes/1", data1);
    HttpResponse response1 = handler_->handle_request(request1);
    
    HttpRequest request2 = create_put_request("/api/Shoes/1", data2);
    HttpResponse response2 = handler_->handle_request(request2);
    
    HttpRequest request3 = create_put_request("/api/Shoes/1", data3);
    HttpResponse response3 = handler_->handle_request(request3);
    
    EXPECT_EQ(response1.get_status_code(), 200);
    EXPECT_EQ(response2.get_status_code(), 200);
    EXPECT_EQ(response3.get_status_code(), 200);
    
    // Verify final state
    HttpRequest get_request = create_get_request("/api/Shoes/1");
    HttpResponse get_response = handler_->handle_request(get_request);
    EXPECT_EQ(get_response.get_message_body(), data3);
}

// Test: PUT with numeric ID
TEST_F(CrudHandlerTest, PutWithNumericId) {
    Entity test_entity("TestEntity");
    filesystem_->write_entity(test_entity, "42", "{\"initial\": \"value\"}");
    
    std::string updated_data = "{\"updated\": \"value\"}";
    HttpRequest request = create_put_request("/api/TestEntity/42", updated_data);
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), updated_data);
}

// Test: PUT with string ID
TEST_F(CrudHandlerTest, PutWithStringId) {
    Entity test_entity("TestEntity");
    filesystem_->write_entity(test_entity, "abc123", "{\"initial\": \"value\"}");
    
    std::string updated_data = "{\"updated\": \"value\"}";
    HttpRequest request = create_put_request("/api/TestEntity/abc123", updated_data);
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), updated_data);
}

//////////////////////////////////////////////////////////////
// DELETE tests
//////////////////////////////////////////////////////////////

// Test: DELETE request deletes existing entity and returns 200
TEST_F(CrudHandlerTest, DeleteExistingEntity) {
    HttpRequest request = create_delete_request("/api/Shoes/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_reason_phrase(), "OK");
    EXPECT_EQ(response.get_header("Content-Type"), "text/plain");
    EXPECT_EQ(response.get_message_body(), "Entity deleted successfully");
}

// Test: DELETE removes entity that can no longer be retrieved
TEST_F(CrudHandlerTest, DeleteRemovesEntityFromStorage) {
    // Verify entity exists before deletion
    HttpRequest get_before = create_get_request("/api/Shoes/1");
    HttpResponse response_before = handler_->handle_request(get_before);
    EXPECT_EQ(response_before.get_status_code(), 200);
    
    // Delete the entity
    HttpRequest delete_request = create_delete_request("/api/Shoes/1");
    HttpResponse delete_response = handler_->handle_request(delete_request);
    EXPECT_EQ(delete_response.get_status_code(), 200);
    
    // Verify entity no longer exists
    HttpRequest get_after = create_get_request("/api/Shoes/1");
    HttpResponse response_after = handler_->handle_request(get_after);
    EXPECT_EQ(response_after.get_status_code(), 404);
}

// Test: DELETE with non-existent entity returns 404
TEST_F(CrudHandlerTest, DeleteNonExistentEntityReturns404) {
    HttpRequest request = create_delete_request("/api/Shoes/999");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_header("Content-Type"), "text/html");
    EXPECT_EQ(response.get_message_body(), "Entity not found");
}

// Test: DELETE without ID in path returns 400
TEST_F(CrudHandlerTest, DeleteWithoutIdInPathReturns400) {
    HttpRequest request = create_delete_request("/api/Shoes");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "DELETE must include ID in path");
}

// Test: DELETE different entity types independently
TEST_F(CrudHandlerTest, DeleteDifferentEntityTypesIndependently) {
    // Delete from Shoes entity
    HttpRequest delete_shoes = create_delete_request("/api/Shoes/1");
    HttpResponse response_shoes = handler_->handle_request(delete_shoes);
    EXPECT_EQ(response_shoes.get_status_code(), 200);
    
    // Verify Shoes/1 is deleted
    HttpRequest get_shoes = create_get_request("/api/Shoes/1");
    HttpResponse get_shoes_response = handler_->handle_request(get_shoes);
    EXPECT_EQ(get_shoes_response.get_status_code(), 404);
    
    // Verify Books/1 still exists (different entity type)
    HttpRequest get_books = create_get_request("/api/Books/1");
    HttpResponse get_books_response = handler_->handle_request(get_books);
    EXPECT_EQ(get_books_response.get_status_code(), 200);
}

// Test: DELETE preserves HTTP version
TEST_F(CrudHandlerTest, DeletePreservesHttpVersion) {
    HttpRequest request = create_delete_request("/api/Shoes/1");
    request.set_version("HTTP/1.1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_version(), "HTTP/1.1");
    EXPECT_EQ(response.get_status_code(), 200);
}

// Test: DELETE with invalid path (no entity name)
TEST_F(CrudHandlerTest, DeleteWithInvalidPathNoEntity) {
    HttpRequest request = create_delete_request("/api");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
    EXPECT_EQ(response.get_message_body(), "Invalid CRUD path");
}

// Test: DELETE with path not matching prefix
TEST_F(CrudHandlerTest, DeleteWithPathNotMatchingPrefix) {
    HttpRequest request = create_delete_request("/other/Shoes/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 400);
    EXPECT_EQ(response.get_reason_phrase(), "Bad Request");
}

// Test: DELETE with entity name containing special characters
TEST_F(CrudHandlerTest, DeleteWithSpecialCharactersInEntityName) {
    Entity special_entity("Test-Entity_123");
    filesystem_->write_entity(special_entity, "1", "{\"test\": \"data\"}");
    
    HttpRequest request = create_delete_request("/api/Test-Entity_123/1");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), "Entity deleted successfully");
    
    // Verify it's actually deleted
    HttpRequest get_request = create_get_request("/api/Test-Entity_123/1");
    HttpResponse get_response = handler_->handle_request(get_request);
    EXPECT_EQ(get_response.get_status_code(), 404);
}

// Test: DELETE with numeric ID
TEST_F(CrudHandlerTest, DeleteWithNumericId) {
    Entity test_entity("TestEntity");
    filesystem_->write_entity(test_entity, "42", "{\"value\": 42}");
    
    HttpRequest request = create_delete_request("/api/TestEntity/42");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    // Verify deletion
    HttpRequest get_request = create_get_request("/api/TestEntity/42");
    HttpResponse get_response = handler_->handle_request(get_request);
    EXPECT_EQ(get_response.get_status_code(), 404);
}

// Test: DELETE with string ID
TEST_F(CrudHandlerTest, DeleteWithStringId) {
    Entity test_entity("TestEntity");
    filesystem_->write_entity(test_entity, "abc123", "{\"value\": \"test\"}");
    
    HttpRequest request = create_delete_request("/api/TestEntity/abc123");
    HttpResponse response = handler_->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    // Verify deletion
    HttpRequest get_request = create_get_request("/api/TestEntity/abc123");
    HttpResponse get_response = handler_->handle_request(get_request);
    EXPECT_EQ(get_response.get_status_code(), 404);
}

// Test: DELETE multiple entities
TEST_F(CrudHandlerTest, DeleteMultipleEntities) {
    // Delete both Shoes entities
    HttpRequest delete1 = create_delete_request("/api/Shoes/1");
    HttpResponse response1 = handler_->handle_request(delete1);
    EXPECT_EQ(response1.get_status_code(), 200);
    
    HttpRequest delete2 = create_delete_request("/api/Shoes/2");
    HttpResponse response2 = handler_->handle_request(delete2);
    EXPECT_EQ(response2.get_status_code(), 200);
    
    // Verify both are deleted
    HttpRequest get1 = create_get_request("/api/Shoes/1");
    HttpRequest get2 = create_get_request("/api/Shoes/2");
    
    HttpResponse get_response1 = handler_->handle_request(get1);
    HttpResponse get_response2 = handler_->handle_request(get2);
    
    EXPECT_EQ(get_response1.get_status_code(), 404);
    EXPECT_EQ(get_response2.get_status_code(), 404);
}

// Test: DELETE then recreate reuses deleted ID (finds the smallest available ID)
TEST_F(CrudHandlerTest, DeleteThenRecreateSameId) {
    // Delete entity
    HttpRequest delete_request = create_delete_request("/api/Shoes/1");
    HttpResponse delete_response = handler_->handle_request(delete_request);
    EXPECT_EQ(delete_response.get_status_code(), 200);
    
    // Verify deleted
    HttpRequest get_before = create_get_request("/api/Shoes/1");
    HttpResponse get_before_response = handler_->handle_request(get_before);
    EXPECT_EQ(get_before_response.get_status_code(), 404);
    
    // Recreate with POST (will reuse ID 1 since it's the smallest available)
    std::string new_data = "{\"new shoe\": \"more expensive lolll\"}";
    HttpRequest post_request = create_post_request("/api/Shoes", new_data);
    HttpResponse post_response = handler_->handle_request(post_request);
    EXPECT_EQ(post_response.get_status_code(), 201);
    
    // Verify that ID 1 is reused and contains the new data
    HttpRequest get_after = create_get_request("/api/Shoes/1");
    HttpResponse get_after_response = handler_->handle_request(get_after);
    EXPECT_EQ(get_after_response.get_status_code(), 200);
    EXPECT_EQ(get_after_response.get_message_body(), new_data);
}

// Test: List all without filters
TEST_F(CrudHandlerFilterTest, ListAllWithoutFilters) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    
    std::string expected = "[\"1\", \"2\", \"3\"]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filter by name - single match
TEST_F(CrudHandlerFilterTest, FilterByNameSingleMatch) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=API");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[\"2\"]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filter by name - multiple matches
TEST_F(CrudHandlerFilterTest, FilterByNameMultipleMatches) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=d20");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[\"1\", \"3\"]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filter by name - case sensitive
TEST_F(CrudHandlerFilterTest, FilterByNameCaseSensitive) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=D20");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filter by tag
TEST_F(CrudHandlerFilterTest, FilterByTag) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?tag=d20");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[\"1\", \"3\"]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filter by both name AND tag
TEST_F(CrudHandlerFilterTest, FilterByNameAndTag) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=Testing&tag=d20");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[\"3\"]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filter with no matches
TEST_F(CrudHandlerFilterTest, FilterWithNoMatches) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=NonExistent");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filter by tag with no matches
TEST_F(CrudHandlerFilterTest, FilterByTagNoMatches) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?tag=nonexistent");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Empty filter values should match all
TEST_F(CrudHandlerFilterTest, EmptyFilterMatchesAll) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=&tag=");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[\"1\", \"2\", \"3\"]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Partial name match
TEST_F(CrudHandlerFilterTest, PartialNameMatch) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=Project");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[\"1\"]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filtering on empty entity list
TEST_F(CrudHandlerFilterTest, FilterOnEmptyEntityList) {
    auto empty_fs = std::make_shared<MockFilesystem>();
    CrudHandler empty_handler("/api", empty_fs);
    
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=test");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = empty_handler.handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[]";
    EXPECT_EQ(response.get_message_body(), expected);
}

// Test: Filter doesn't affect other entity types
TEST_F(CrudHandlerFilterTest, FilterDoesNotAffectOtherEntities) {
    filesystem_->write_entity(Entity{"Shoes"}, "1", "{\"name\":\"Running Shoes\"}");
    filesystem_->write_entity(Entity{"Shoes"}, "2", "{\"name\":\"Boots\"}");
    
    HttpRequest request1;
    request1.set_method("GET");
    request1.set_path("/api/file_data?name=d20");
    request1.set_version("HTTP/1.1");
    
    HttpResponse response1 = handler->handle_request(request1);
    EXPECT_EQ(response1.get_status_code(), 200);
    EXPECT_EQ(response1.get_message_body(), "[\"1\", \"3\"]");
    
    HttpRequest request2;
    request2.set_method("GET");
    request2.set_path("/api/Shoes");
    request2.set_version("HTTP/1.1");
    
    HttpResponse response2 = handler->handle_request(request2);
    EXPECT_EQ(response2.get_status_code(), 200);
    EXPECT_EQ(response2.get_message_body(), "[\"1\", \"2\"]");
}

// Test: Query string stripped in path parsing
TEST_F(CrudHandlerFilterTest, QueryStringStrippedInPathParsing) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=test&extra=param");
    request.set_version("HTTP/1.1");
    
    // This should not cause a "Bad Request" error
    // The path parser should strip the query string
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
}

// Test: Both filters must match (AND logic)
TEST_F(CrudHandlerFilterTest, BothFiltersMustMatch) {
    HttpRequest request;
    request.set_method("GET");
    request.set_path("/api/file_data?name=d20&tag=documentation");
    request.set_version("HTTP/1.1");
    
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    
    std::string expected = "[]";
    EXPECT_EQ(response.get_message_body(), expected);
}
