#ifndef MOCK_FILESYSTEM_H
#define MOCK_FILESYSTEM_H

#include <unordered_map>
#include <string>
#include <vector>

#include "filesystem_interface.h"

// This implementation does NOT touch the real filesystem; it's intended
// for unit-testing the CRUD handler via dependency injection.
class MockFilesystem : public FilesystemInterface {
public:
    MockFilesystem() = default;
    ~MockFilesystem() override = default;

    bool entity_exists(const Entity& entity, const std::string& id) const override;
    bool write_entity(const Entity& entity, const std::string& id, const std::string& data) override;
    std::string read_entity(const Entity& entity, const std::string& id) const override;
    bool delete_entity(const Entity& entity, const std::string& id) override;

    std::vector<std::string> list_entity_ids(const Entity& entity) const override;
    std::string next_entity_id(const Entity& entity) const override;

    // For tests to reset state if needed
    void reset();

private:
    // data_[entity.name][id] = JSON payload
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data_;
};

#endif
