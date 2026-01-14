#include "crud_handler.h"

#include <vector>
#include <iostream>
#include <algorithm>

CrudHandler::CrudHandler(const std::string& route_prefix,
                         std::shared_ptr<FilesystemInterface> filesystem)
    : route_prefix_(route_prefix),
      filesystem_(std::move(filesystem)) {}

// Main entry point. Implements full CRUD API:
//   - POST /api/Entity: Create new entity (returns 201 with new ID)
//   - GET /api/Entity/id: Read entity by ID (returns 200 with JSON data)
//   - GET /api/Entity: List all entity IDs (returns 200 with JSON array)
//   - PUT /api/Entity/id: Update entity by ID (returns 200 with updated JSON)
//   - DELETE /api/Entity/id: Delete entity by ID (returns 200 with success message)
HttpResponse CrudHandler::handle_request(const HttpRequest& request) {
    const std::string method = request.method();
    const std::string path   = request.path();

    Entity entity;
    std::string id;
    bool has_id = false;

    if (!parse_entity_and_id_from_path(path, entity, id, has_id)) {
        // Path doesn't match /<prefix>/<Entity>[/<id>]
        HttpResponse response(
            "HTTP/1.1",
            400,
            "Bad Request",
            {{"Content-Type", "text/plain"}},
            "Invalid CRUD path"
        );
        return response;
    }

    if (method == "POST") {
        // For POST, we only accept /<prefix>/<Entity> (no ID in the path).
        if (has_id) {
            HttpResponse response(
                "HTTP/1.1",
                400,
                "Bad Request",
                {{"Content-Type", "text/plain"}},
                "POST must not include ID in path"
            );
            return response;
        }
        return handle_post(request, entity);
    }

    if (method == "GET") {
        if (has_id) {
            return handle_get(request, entity, id);
        } else {
            return handle_list(request, entity);
        }
    }

    if (method == "PUT") {
        // we want an ID in the path
        if (!has_id) {
            HttpResponse response(
                "HTTP/1.1",
                400,
                "Bad Request",
                {{"Content-Type", "text/plain"}},
                "PUT must include ID in path"
            );
            return response;
        }
        return handle_put(request, entity, id);
    }

    if (method == "DELETE") {
        if (!has_id) {
            HttpResponse response(
                "HTTP/1.1",
                400,
                "Bad Request",
                {{"Content-Type", "text/plain"}},
                "DELETE must include ID in path"
            );
            return response;
        }
        return handle_delete(request, entity, id);
    }

    // TODO: Use method + has_id for future operations, e.g.:
    //   - GET with has_id == false    -> list IDs for an entity type

    HttpResponse response(
        "HTTP/1.1",
        501,
        "Not Implemented",
        {{"Content-Type", "text/plain"}},
        "CRUD method not implemented yet"
    );
    return response;
}

HttpResponse CrudHandler::handle_post(const HttpRequest& request,
                                      const Entity& entity) {
    std::string body = request.body();

    std::string new_id;
    try {
        new_id = filesystem_->next_entity_id(entity);
    } catch (const std::exception& ex) {
        HttpResponse response(
            "HTTP/1.1",
            500,
            "Internal Server Error",
            {{"Content-Type", "text/plain"}},
            std::string("Failed to allocate ID: ") + ex.what()
        );
        return response;
    }

    bool ok = filesystem_->write_entity(entity, new_id, body);
    if (!ok) {
        HttpResponse response(
            "HTTP/1.1",
            500,
            "Internal Server Error",
            {{"Content-Type", "text/plain"}},
            "Failed to store entity"
        );
        return response;
    }

    std::string response_body = "{\"id\": " + new_id + "}";

    HttpResponse response(
        "HTTP/1.1",
        201,
        "Created",
        {{"Content-Type", "application/json"}},
        response_body
    );
    return response;
}

HttpResponse CrudHandler::handle_get(const HttpRequest& request,
                                     const Entity& entity,
                                     const std::string& id) {
    // Check if the entity exists
    if (!filesystem_->entity_exists(entity, id)) {
        HttpResponse response(
            "HTTP/1.1",
            404,
            "Not Found",
            {{"Content-Type", "text/html"}},
            "Entity not found"
        );
        return response;
    }

    std::string entity_data = filesystem_->read_entity(entity, id);
    
    // Return the JSON data
    HttpResponse response(
        "HTTP/1.1",
        200,
        "OK",
        {{"Content-Type", "application/json"}},
        entity_data
    );
    return response;
}

HttpResponse CrudHandler::handle_put(const HttpRequest& request,
                                     const Entity& entity,
                                     const std::string& id) {
    // Check if the entity exists
    if (!filesystem_->entity_exists(entity, id)) {
        HttpResponse response(
            "HTTP/1.1",
            404,
            "Not Found",
            {{"Content-Type", "text/html"}},
            "Entity not found"
        );
        return response;
    }

    std::string body = request.body();

    // Update the entity with new data
    bool ok = filesystem_->write_entity(entity, id, body);
    if (!ok) {
        HttpResponse response(
            "HTTP/1.1",
            500,
            "Internal Server Error",
            {{"Content-Type", "text/plain"}},
            "Failed to update entity"
        );
        return response;
    }

    // Return 200 OK with the updated JSON data
    HttpResponse response(
        "HTTP/1.1",
        200,
        "OK",
        {{"Content-Type", "application/json"}},
        body
    );
    return response;
}

HttpResponse CrudHandler::handle_delete(const HttpRequest& request,
                                        const Entity& entity,
                                        const std::string& id) {
    // Check if the entity exists
    if (!filesystem_->entity_exists(entity, id)) {
        HttpResponse response(
            "HTTP/1.1",
            404,
            "Not Found",
            {{"Content-Type", "text/html"}},
            "Entity not found"
        );
        return response;
    }

    // Delete the entity
    bool ok = filesystem_->delete_entity(entity, id);
    if (!ok) {
        HttpResponse response(
            "HTTP/1.1",
            500,
            "Internal Server Error",
            {{"Content-Type", "text/plain"}},
            "Failed to delete entity"
        );
        return response;
    }

    HttpResponse response(
        "HTTP/1.1",
        200,
        "OK",
        {{"Content-Type", "text/plain"}},
        "Entity deleted successfully"
    );
    return response;
}

HttpResponse CrudHandler::handle_list(const HttpRequest& request,
    const Entity& entity) const {

    // Get query parameters for filtering
    auto name_filter = request.get_query_param("name");
    auto tag_filter = request.get_query_param("tag");

    // get all IDs for this entity type
    std::vector<std::string> ids = filesystem_->list_entity_ids(entity);

    // Apply filtering if parameters provided
    std::vector<std::string> filtered_ids;
    
    for (const std::string& id : ids) {
        // If no filters, include everything
        if (!name_filter.has_value() && !tag_filter.has_value()) {
            filtered_ids.push_back(id);
            continue;
        }
        
        // Read the entity data to check filters
        std::string entity_data = filesystem_->read_entity(entity, id);
        
        // Check if this entry matches the filters
        bool matches = true;
        
        if (name_filter.has_value()) {
            // Extract name field and check if it contains the filter value
            std::string name_value = extract_json_field(entity_data, "name");
            if (name_value.find(name_filter.value()) == std::string::npos) {
                matches = false;
            }
        }
        
        if (tag_filter.has_value() && matches) {
            // Extract tag field and check if it contains the filter value
            std::string tag_value = extract_json_field(entity_data, "tag");
            if (tag_value.find(tag_filter.value()) == std::string::npos) {
                matches = false;
            }
        }
        
        if (matches) {
            filtered_ids.push_back(id);
        }
    }

    std::sort(filtered_ids.begin(), filtered_ids.end());

    // format as JSON array: ["id1", "id2", "id3"]
    // not sure if ID will always be numeric, so add quotes around each ID
    std::string response_body = "[";
    for (size_t i = 0; i < filtered_ids.size(); ++i) {
        if (i > 0) {
            response_body += ", ";
        }

        // std::cout << "ids[" << i << "] = " << ids[i] << std::endl;

        response_body += "\"" + filtered_ids[i] + "\"";
    }
    response_body += "]";

    HttpResponse response(
        "HTTP/1.1",
        200,
        "OK",
        {{"Content-Type", "application/json"}},
        response_body
    );
    return response;
}

bool CrudHandler::parse_entity_and_id_from_path(const std::string& path,
                                                Entity& entity_out,
                                                std::string& id_out,
                                                bool& has_id_out) const {
    std::string p = path;

    size_t query_pos = p.find('?');
    if (query_pos != std::string::npos) {
        p = p.substr(0, query_pos);
    }

    // Path should start with the route prefix
    if (!route_prefix_.empty()) {
        if (p.rfind(route_prefix_, 0) != 0) {
            return false;
        }
    }

    // Strip the prefix from the path
    std::string rest = p.substr(route_prefix_.size());

    // Remove a leading slash from the remainder, if present
    if (!rest.empty() && rest[0] == '/') {
        rest.erase(0, 1);
    }

    // Split rest by '/' into 0–2 segments: [Entity] or [Entity, id]
    std::string entity_part;
    std::string id_part;
    has_id_out = false;

    auto slash_pos = rest.find('/');
    if (slash_pos == std::string::npos) {
        // Only one segment: "<Entity>"
        entity_part = rest;
    } else {
        entity_part = rest.substr(0, slash_pos);
        id_part = rest.substr(slash_pos + 1);

        // If there's another slash in id_part, it's invalid
        if (id_part.find('/') != std::string::npos) {
            return false;
        }
        if (!id_part.empty()) {
            has_id_out = true;
        }
    }

    if (entity_part.empty()) {
        // No entity name present after the prefix.
        return false;
    }

    entity_out = Entity{entity_part};
    id_out = has_id_out ? id_part : std::string{};
    return true;
}

std::string CrudHandler::get_handler_name() const {
    return "CrudHandler";
}

std::string CrudHandler::extract_json_field(const std::string& json, 
                                           const std::string& field_name) const {
    // Look for "field_name": "value" pattern
    std::string search_pattern = "\"" + field_name + "\"";
    size_t field_pos = json.find(search_pattern);
    
    if (field_pos == std::string::npos) {
        return "";
    }
    
    size_t colon_pos = json.find(':', field_pos);
    if (colon_pos == std::string::npos) {
        return "";
    }
    
    // Skip whitespace and opening quote
    size_t value_start = json.find_first_not_of(" \t\n", colon_pos + 1);
    if (value_start == std::string::npos || json[value_start] != '"') {
        return "";
    }
    value_start++; // Skip the opening quote
    
    size_t value_end = json.find('"', value_start);
    if (value_end == std::string::npos) {
        return "";
    }
    
    return json.substr(value_start, value_end - value_start);
}
