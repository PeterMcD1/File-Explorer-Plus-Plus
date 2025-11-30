#include "ui/ExplorerWindow.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <FL/Fl.H>
#include <chrono>
#include <string>
#include <windows.h> // For CoInitialize

int main(int argc, char** argv) {
    // Capture start time
    auto start_time = std::chrono::steady_clock::now();

    // Initialize COM for Shell APIs
    HRESULT hr = CoInitialize(NULL);
    
    // Set DPI awareness
    SetProcessDPIAware();

    Fl::lock();

    // Initialize Logger
    std::string logPath = core::GetConfigDir() + "\\flash_log.txt";
    core::Init(logPath);
    core::Log("Application started.");

    // Modernize UI
    // Fl::scheme("gtk+"); // Removed to allow custom scrollbar styling
    Fl::set_font(FL_HELVETICA, "Segoe UI");
    
    // Dark Theme Global Colors
    Fl::background(32, 32, 32); // #202020
    Fl::foreground(224, 224, 224); // #E0E0E0
    Fl::background2(45, 45, 45); // #2D2D2D (Input fields, etc.)
    
    // Set global scrollbar size
    Fl::scrollbar_size(8); // Thinner (8px)

    ui::ExplorerWindow window(1000, 700, "Flash Explorer", start_time);
    window.show();

    int result = Fl::run();
    
    CoUninitialize();
    return result;
}
