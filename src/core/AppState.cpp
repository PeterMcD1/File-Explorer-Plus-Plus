#include "AppState.h"

namespace core {
    std::vector<FileEntry> g_files;
    std::mutex g_files_mutex;
    std::atomic<bool> g_loading{false};
    std::string g_current_path;
    std::string g_status_text = "Ready";
    ui::ExplorerWindow* g_main_window = nullptr;
    UpdateCallback g_update_callback = nullptr;
    SetAddressCallback g_set_address_callback = nullptr;
}
