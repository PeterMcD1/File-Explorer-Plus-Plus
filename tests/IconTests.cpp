#include <gtest/gtest.h>
#include "ui/IconManager.h"
#include <FL/Fl_RGB_Image.H>

TEST(IconTests, GetIcon_TextFile) {
    // Should return a valid image for .txt extension
    Fl_RGB_Image* icon = ui::IconManager::Get().GetIcon("test.txt", false);
    ASSERT_NE(icon, nullptr);
    EXPECT_GT(icon->w(), 0);
    EXPECT_GT(icon->h(), 0);
}

TEST(IconTests, GetIcon_Directory) {
    // Should return a valid image for directory
    Fl_RGB_Image* icon = ui::IconManager::Get().GetIcon("any_dir", true);
    ASSERT_NE(icon, nullptr);
}

TEST(IconTests, GetIcon_DirectoryCaching) {
    // Different paths should return the same cached icon for directories
    Fl_RGB_Image* icon1 = ui::IconManager::Get().GetIcon("dir1", true);
    Fl_RGB_Image* icon2 = ui::IconManager::Get().GetIcon("dir2", true);
    ASSERT_EQ(icon1, icon2);
    ASSERT_NE(icon1, nullptr);
}

TEST(IconTests, GetSpecificIcon_Explorer) {
    // Should return a valid image for explorer.exe
    Fl_RGB_Image* icon = ui::IconManager::Get().GetSpecificIcon("C:\\Windows\\explorer.exe");
    ASSERT_NE(icon, nullptr);
    delete icon; // We own this one
}
