cmake_minimum_required(VERSION 3.15)
project(FlashExplorer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use FetchContent to download and build FLTK
include(FetchContent)
message(STATUS "Fetching FLTK... this may take a while on the first build.")

FetchContent_Declare(
  fltk
  GIT_REPOSITORY https://github.com/fltk/fltk.git
  GIT_TAG        master # Using master for 1.4 features
)

# Disable unnecessary FLTK parts to speed up build
set(FLTK_BUILD_TEST OFF CACHE BOOL " " FORCE)
set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(FLTK_BUILD_FLUID OFF CACHE BOOL " " FORCE)
set(FLTK_BUILD_HTML_DOCS OFF CACHE BOOL " " FORCE)
set(FLTK_BUILD_PDF_DOCS OFF CACHE BOOL " " FORCE)

FetchContent_MakeAvailable(fltk)
link_directories(${fltk_BINARY_DIR}/lib)

# --- Core Library ---
add_library(core_lib STATIC
    src/core/Logger.cpp
    src/core/AppState.cpp
    src/core/FileSystem.cpp
    src/core/QuickAccess.cpp
)
target_include_directories(core_lib PUBLIC src)
target_include_directories(core_lib PUBLIC 
    ${fltk_SOURCE_DIR} 
    ${FLTK_SOURCE_DIR}
    ${fltk_BINARY_DIR}
    ${FLTK_BINARY_DIR}
)
target_link_libraries(core_lib PUBLIC fltk)

# --- UI Library ---
add_library(ui_lib STATIC
    src/ui/FileTable.cpp
    src/ui/ExplorerWindow.cpp
    src/ui/IconManager.cpp
    src/ui/ExplorerTab.cpp
    src/ui/TabBar.cpp
    src/ui/Sidebar.cpp
)
target_include_directories(ui_lib PUBLIC src)
target_link_libraries(ui_lib PUBLIC core_lib fltk user32 shell32 gdi32)

# --- Main Executable ---
add_executable(FlashExplorer WIN32 src/main.cpp)
target_link_libraries(FlashExplorer PRIVATE ui_lib core_lib fltk)

# --- Testing ---
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(FlashTests tests/FileSystemTests.cpp tests/IconTests.cpp tests/UITests.cpp tests/QuickAccessTests.cpp)
target_link_libraries(FlashTests PRIVATE core_lib ui_lib GTest::gtest_main fltk)

include(GoogleTest)
gtest_discover_tests(FlashTests)
