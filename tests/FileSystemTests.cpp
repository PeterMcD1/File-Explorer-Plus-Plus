#include <gtest/gtest.h>
#include "core/FileSystem.h"

TEST(FileSystemTests, FormatSize_Bytes) {
    EXPECT_EQ(core::FormatSize(0), "0 B");
    EXPECT_EQ(core::FormatSize(500), "500 B");
    EXPECT_EQ(core::FormatSize(1023), "1023 B");
}

TEST(FileSystemTests, FormatSize_Kilobytes) {
    EXPECT_EQ(core::FormatSize(1024), "1.0 KB");
    EXPECT_EQ(core::FormatSize(1536), "1.5 KB");
    EXPECT_EQ(core::FormatSize(1024 * 1024 - 1), "1024.0 KB"); // Boundary check
}

TEST(FileSystemTests, FormatSize_Megabytes) {
    EXPECT_EQ(core::FormatSize(1024 * 1024), "1.0 MB");
    EXPECT_EQ(core::FormatSize(1024 * 1024 * 2.5), "2.5 MB");
}
