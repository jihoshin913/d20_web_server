#include "mock_filesystem.h"

#include <algorithm>
#include <stdexcept>
#include <limits>
#include <unordered_set>
#include <stdexcept>

#include <boost/log/trivial.hpp>

bool MockFilesystem::entity_exists(const Entity& entity, const std::string& id) const {
    auto eit = data_.find(entity.name);
    if (eit == data_.end()) {
        return false;
    }
    return eit->second.find(id) != eit->second.end();
}

bool MockFilesystem::write_entity(const Entity& entity, const std::string& id, const std::string& data) {
    BOOST_LOG_TRIVIAL(debug) << "MockFilesystem: Writing entity " << entity.make_name(id);
    data_[entity.name][id] = data;
    return true;
}

std::string MockFilesystem::read_entity(const Entity& entity, const std::string& id) const {
    if (!entity_exists(entity, id)) {
        BOOST_LOG_TRIVIAL(warning)
            << "MockFilesystem: No such entity or ID: " << entity.make_name(id);

        throw std::runtime_error(
            "MockFilesystem: No such entity or ID: " + entity.make_name(id));
    }

    const auto& entity_map = data_.at(entity.name);
    const auto& value = entity_map.at(id);
    return value;
}

bool MockFilesystem::delete_entity(const Entity& entity, const std::string& id) {
    auto eit = data_.find(entity.name);
    if (eit == data_.end()) {
        BOOST_LOG_TRIVIAL(warning)
            << "MockFilesystem: Could not remove entity (no such type): "
            << entity.make_name(id);
        return false;
    }

    auto it = eit->second.find(id);
    if (it == eit->second.end()) {
        BOOST_LOG_TRIVIAL(warning)
            << "MockFilesystem: Could not remove entity (no such id): "
            << entity.make_name(id);
        return false;
    }

    BOOST_LOG_TRIVIAL(debug)
        << "MockFilesystem: Removing entity " << entity.make_name(id);

    eit->second.erase(it);
    return true;
}

std::vector<std::string> MockFilesystem::list_entity_ids(const Entity& entity) const {
    std::vector<std::string> ids;

    auto eit = data_.find(entity.name);
    if (eit == data_.end()) {
        return ids;  // no such entity type yet
    }

    ids.reserve(eit->second.size());
    for (const auto& [id, _] : eit->second) {
        ids.push_back(id);
    }
    return ids;
}

std::string MockFilesystem::next_entity_id(const Entity& entity) const {
    auto ids = list_entity_ids(entity);
    std::unordered_set<int> used_ids;
    used_ids.reserve(ids.size());

    for (const auto& id_str : ids) {
        try {
            int id = std::stoi(id_str);
            if (id > 0) {
                used_ids.insert(id);
            }
        } catch (const std::invalid_argument&) {
            // Non-numeric ID, ignore
        } catch (const std::out_of_range&) {
            // Too big to be an int, ignore
        }
    }

     // Find the smallest positive integer not in used_ids.
    int candidate = 1;
    while (true) {
        if (used_ids.find(candidate) == used_ids.end()) {
            BOOST_LOG_TRIVIAL(debug)
                << "MockFilesystem: next_entity_id for " << entity.name
                << " -> " << candidate;
            return std::to_string(candidate);
        }

        if (candidate == std::numeric_limits<int>::max()) {
            BOOST_LOG_TRIVIAL(error)
                << "MockFilesystem: next_entity_id overflow for entity "
                << entity.name;
            throw std::overflow_error(
                "MockFilesystem: no available integer IDs for entity " +
                entity.name);
        }

        ++candidate;
    }
}

void MockFilesystem::reset() {
    BOOST_LOG_TRIVIAL(debug) << "MockFilesystem: Resetting";
    data_.clear();
}
