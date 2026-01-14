#include "gtest/gtest.h"
#include "mock_filesystem.h"
#include "filesystem_interface.h"
#include <unordered_set>
#include <string>

static Entity make_entity(const std::string& name) {
    Entity e;
    e.name = name;
    return e;
}

class MockFilesystemTest : public testing::Test {
protected:
    MockFilesystem fs_;
    Entity e1_;
    Entity e2_;

    void SetUp() override {
        e1_ = make_entity("entity 1");
        e2_ = make_entity("entity 2");
    }
};

TEST_F(MockFilesystemTest, EntityDoesntExist) {
    EXPECT_FALSE(fs_.entity_exists(e1_, "1"));
}

TEST_F(MockFilesystemTest, EntityAddedExists) {
    fs_.write_entity(e1_, "1", "data1");
    EXPECT_TRUE(fs_.entity_exists(e1_, "1"));
}

TEST_F(MockFilesystemTest, ReadEntityReturnsWrittenData) {
    fs_.write_entity(e1_, "1", "hello");
    fs_.write_entity(e1_, "2", "world");

    EXPECT_EQ(fs_.read_entity(e1_, "1"), "hello");
    EXPECT_EQ(fs_.read_entity(e1_, "2"), "world");
}

TEST_F(MockFilesystemTest, OverwriteEntityUpdatesExistingData) {
    fs_.write_entity(e1_, "1", "old data");
    EXPECT_EQ(fs_.read_entity(e1_, "1"), "old data");
    fs_.write_entity(e1_, "1", "new data");
    EXPECT_EQ(fs_.read_entity(e1_, "1"), "new data");
}

TEST_F(MockFilesystemTest, ReadEntityThrowsWhenMissing) {
    EXPECT_THROW(fs_.read_entity(e1_, "99999999"), std::runtime_error);

    fs_.write_entity(e1_, "1", "data");
    EXPECT_THROW(fs_.read_entity(e1_, "2"), std::runtime_error);
}

TEST_F(MockFilesystemTest, DeleteEntityReturnsFalseWhenEntityDoesntExist) {
    EXPECT_FALSE(fs_.delete_entity(e2_, "1"));
}

TEST_F(MockFilesystemTest, DeleteEntityReturnsFalseWhenIdMissing) {
    fs_.write_entity(e1_, "1", "data");

    EXPECT_FALSE(fs_.delete_entity(e1_, "2"));
    EXPECT_TRUE(fs_.entity_exists(e1_, "1"));
}

TEST_F(MockFilesystemTest, DeleteEntityRemovesExistingId) {
    fs_.write_entity(e1_, "1", "data");
    EXPECT_TRUE(fs_.entity_exists(e1_, "1"));

    EXPECT_TRUE(fs_.delete_entity(e1_, "1"));
    EXPECT_FALSE(fs_.entity_exists(e1_, "1"));
}

TEST_F(MockFilesystemTest, ListEntityIdsEmptyWhenNoEntities) {
    auto ids = fs_.list_entity_ids(e1_);
    EXPECT_TRUE(ids.empty());
}

TEST_F(MockFilesystemTest, ListEntityIdsReturnsAllIdsForEntity) {
    fs_.write_entity(e1_, "1", "a");
    fs_.write_entity(e1_, "2", "b");
    fs_.write_entity(e1_, "3", "c");

    auto ids = fs_.list_entity_ids(e1_);
    std::unordered_set<std::string> id_set(ids.begin(), ids.end());

    EXPECT_EQ(id_set.size(), 3u);
    EXPECT_TRUE(id_set.count("1"));
    EXPECT_TRUE(id_set.count("2"));
    EXPECT_TRUE(id_set.count("3"));
}

TEST_F(MockFilesystemTest, ListEntityIdsIsPerEntityType) {
    fs_.write_entity(e1_, "1", "hello");
    fs_.write_entity(e2_, "1", "hello again");
    fs_.write_entity(e2_, "2", "goodbye");

    auto e1_ids = fs_.list_entity_ids(e1_);
    auto e2_ids = fs_.list_entity_ids(e2_);

    std::unordered_set<std::string> e1_set(e1_ids.begin(), e1_ids.end());
    std::unordered_set<std::string> e2_set(e2_ids.begin(), e2_ids.end());

    EXPECT_EQ(e1_set.size(), 1);
    EXPECT_TRUE(e1_set.count("1"));

    EXPECT_EQ(e2_set.size(), 2);
    EXPECT_TRUE(e2_set.count("1"));
    EXPECT_TRUE(e2_set.count("2"));
}

TEST_F(MockFilesystemTest, NextEntityIdForEmptyEntityIsOne) {
    EXPECT_EQ(fs_.next_entity_id(e1_), "1");
}

TEST_F(MockFilesystemTest, NextEntityIdFillsGapInNumericIds) {
    fs_.write_entity(e1_, "1", "a");
    fs_.write_entity(e1_, "2", "b");
    fs_.write_entity(e1_, "4", "c");
    fs_.write_entity(e1_, "10", "d");

    EXPECT_EQ(fs_.next_entity_id(e1_), "3");
}

TEST_F(MockFilesystemTest, NextEntityIdIgnoresNonNumericAndNonPositiveIds) {
    // non numeric / non pos
    fs_.write_entity(e1_, "foo", "x");
    fs_.write_entity(e1_, "-1", "y");
    fs_.write_entity(e1_, "0", "z");

    // no positive numeric IDs yet
    EXPECT_EQ(fs_.next_entity_id(e1_), "1");

    fs_.write_entity(e1_, "1", "real");
    fs_.write_entity(e1_, "bar", "x");

    EXPECT_EQ(fs_.next_entity_id(e1_), "2");
}

TEST_F(MockFilesystemTest, ResetClearsAllData) {
    fs_.write_entity(e1_, "1", "alice");
    fs_.write_entity(e2_, "1", "post1");

    EXPECT_TRUE(fs_.entity_exists(e1_, "1"));
    EXPECT_TRUE(fs_.entity_exists(e2_, "1"));

    fs_.reset();

    EXPECT_FALSE(fs_.entity_exists(e1_, "1"));
    EXPECT_FALSE(fs_.entity_exists(e2_, "1"));
    EXPECT_TRUE(fs_.list_entity_ids(e1_).empty());
    EXPECT_TRUE(fs_.list_entity_ids(e2_).empty());
}