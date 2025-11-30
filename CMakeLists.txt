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

add_executable(FlashExplorer WIN32 main.cpp)

# Link FLTK
target_link_libraries(FlashExplorer PRIVATE fltk)

# Copy DLLs if necessary (though we prefer static, FLTK defaults to static usually)
if(WIN32)
    target_compile_definitions(FlashExplorer PRIVATE WIN32)
endif()
