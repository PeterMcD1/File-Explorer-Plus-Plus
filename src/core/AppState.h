#pragma once
#include "FileEntry.h"
#include <vector>
#include <mutex>
#include <atomic>
#include <string>

// Forward declaration
namespace ui { class ExplorerWindow; }

namespace core {
    extern std::vector<FileEntry> g_files;
    extern std::mutex g_files_mutex;
    extern std::atomic<bool> g_loading;
    extern std::string g_current_path;
    extern std::string g_status_text;
    
    // Global pointer to main window for callbacks
    extern ui::ExplorerWindow* g_main_window;

    using UpdateCallback = void(*)(void*);
    extern UpdateCallback g_update_callback;

    using SetAddressCallback = void(*)(const char*);
    extern SetAddressCallback g_set_address_callback;
}
