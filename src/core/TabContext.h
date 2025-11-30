#pragma once
#include "FileEntry.h"
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <functional>

namespace core {

struct TabContext {
    std::vector<FileEntry> files;
    std::string current_path;
    std::mutex mutex;
    std::atomic<bool> is_loading{false};
    std::string status_text = "Ready";
    
    // Callback to notify UI of updates (called from worker thread)
    std::function<void()> on_update;

    // History
    std::vector<std::string> history_back;
    std::vector<std::string> history_forward;
};

}
