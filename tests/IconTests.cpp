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

TEST(IconTests, GetSpecificIcon_Explorer) {
    // Should return a valid image for explorer.exe
    Fl_RGB_Image* icon = ui::IconManager::Get().GetSpecificIcon("C:\\Windows\\explorer.exe");
    ASSERT_NE(icon, nullptr);
    delete icon; // We own this one
}
