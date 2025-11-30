#include "ui/ExplorerWindow.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <FL/Fl.H>
#include <chrono>
#include <string>

#include <windows.h> // For CoInitialize

int main(int argc, char** argv) {
    // Initialize COM for Shell APIs
    CoInitialize(NULL);

    auto start_time = std::chrono::high_resolution_clock::now();

    Fl::lock();

    ui::ExplorerWindow window(800, 600, "Flash Explorer");
    window.show();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    core::Log("Startup time: " + std::to_string(duration) + " ms");

    int result = Fl::run();
    
    CoUninitialize();
    return result;
}
