#ifndef FILESYSTEM_INTERFACE_H
#define FILESYSTEM_INTERFACE_H

#include <string>
#include <vector>

struct Entity {
    std::string name;  
    Entity() = default;
    explicit Entity(const std::string& n) : name(n) {}

    // Convenience for logging / debugging: "Shoes/1"
    std::string make_name(const std::string& id) const {
        return name + "/" + id;
    }
};

class FilesystemInterface {
public:
    virtual ~FilesystemInterface() = default;

    // The following return true on success and false on failure of the operation
    virtual bool entity_exists(const Entity& entity, const std::string& id) const = 0;
    virtual bool write_entity(const Entity& entity, const std::string& id, const std::string& data) = 0;
    virtual bool delete_entity(const Entity& entity, const std::string& id) = 0;

    virtual std::string read_entity(const Entity& entity, const std::string& id) const = 0;

    // List all existing IDs for the given entity.
    virtual std::vector<std::string> list_entity_ids(const Entity& entity) const = 0;

    // Compute the next available ID (as a string) for the given entity.
    virtual std::string next_entity_id(const Entity& entity) const = 0;
};

#endif
