#ifndef CRUD_HANDLER_H
#define CRUD_HANDLER_H

#include "http_request.h"
#include "http_response.h"
#include "request_handler.h"
#include "filesystem_interface.h"

#include <memory>
#include <string>

// This class is responsible for handling CRUD API requests under a given
// route prefix (e.g. "/api") using a FilesystemInterface backend.
// 
// For now, this handler simply returns a placeholder rsesponse for all
// requests. The actual CRUD methods (starting with POST) will be
// implemented in a follow-up change.
class CrudHandler : public RequestHandler {
public:
    CrudHandler(const std::string& route_prefix,
                std::shared_ptr<FilesystemInterface> filesystem);

    HttpResponse handle_request(const HttpRequest& request) override;

    std::string get_handler_name() const override;

private:
    std::string route_prefix_;
    std::shared_ptr<FilesystemInterface> filesystem_;

    // Placeholder for POST handling to be implemented later.
    //
    // Expected future behavior:
    //   - POST <route_prefix_>/<Entity>
    //   - Use filesystem_->next_entity_id(...) and filesystem_->write_entity(...)
    //   - Return 201 with JSON body containing {"id": <id>}
    bool parse_entity_and_id_from_path(const std::string& path,
                                   Entity& entity_out,
                                   std::string& id_out,
                                   bool& has_id_out) const;

    HttpResponse handle_post(const HttpRequest& request,
                            const Entity& entity);
    
    HttpResponse handle_get(const HttpRequest& request,
                           const Entity& entity,
                           const std::string& id);
    
    HttpResponse handle_put(const HttpRequest& request,
                           const Entity& entity,
                           const std::string& id);

    HttpResponse handle_delete(const HttpRequest& request,
                            const Entity& entity,
                            const std::string& id);
    
    HttpResponse handle_list(const HttpRequest& request,
                            const Entity& entity) const;

    std::string extract_json_field(const std::string& json, const std::string& field_name) const;

};

#endif
