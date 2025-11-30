#include "ui/ExplorerWindow.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <FL/Fl.H>
#include <chrono>
#include <string>

int main(int argc, char** argv) {
    auto start_time = std::chrono::high_resolution_clock::now();

    Fl::lock();

    ui::ExplorerWindow window(800, 600, "Flash Explorer");
    window.show(argc, argv);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    core::Log("Startup time: " + std::to_string(duration) + " ms");

    core::StartLoading("C:/");

    return Fl::run();
}
