#include <gtest/gtest.h>
#include "../src/core/QuickAccess.h"
#include <filesystem>
#include <fstream>

class QuickAccessTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing save file
        std::filesystem::remove("quick_access_test.txt");
    }

    void TearDown() override {
        std::filesystem::remove("quick_access_test.txt");
    }
};

TEST_F(QuickAccessTest, AddVisit_IncrementsCount) {
    auto& qa = core::QuickAccess::Get();
    // We can't easily reset the singleton, so we might be testing state from previous runs if not careful.
    // Ideally we should be able to inject dependencies or reset state.
    // For now, let's just test the logic we can control or assume it's fresh enough for unit tests if run in isolation.
    // But GTest runs all tests in same process usually.
    
    // Let's rely on adding a unique path.
    std::string path = "C:/UniqueTestPath";
    qa.AddVisit(path);
    
    auto items = qa.GetItems(100);
    bool found = false;
    for (const auto& item : items) {
        if (item.path == path) {
            found = true;
            EXPECT_GE(item.score, 1);
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(QuickAccessTest, Pin_AddsToPinnedList) {
    auto& qa = core::QuickAccess::Get();
    std::string path = "C:/PinnedPath";
    
    qa.Pin(path);
    EXPECT_TRUE(qa.IsPinned(path));
    
    auto items = qa.GetItems(100);
    bool found = false;
    for (const auto& item : items) {
        if (item.path == path) {
            found = true;
            EXPECT_TRUE(item.pinned);
            break;
        }
    }
    EXPECT_TRUE(found);
    
    qa.Unpin(path);
    EXPECT_FALSE(qa.IsPinned(path));
}
