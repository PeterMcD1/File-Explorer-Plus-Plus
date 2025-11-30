#include "QuickAccess.h"
#include "FileSystem.h"
#include <fstream>
#include <algorithm>
#include <shlobj.h>
#include <windows.h>
#include <filesystem>

namespace core {

QuickAccess& QuickAccess::Get() {
    static QuickAccess instance;
    return instance;
}

QuickAccess::QuickAccess() {
    // Determine save path
    save_path = GetConfigDir() + "\\quick_access.txt";
    Load();
}

QuickAccess::~QuickAccess() {
    Save();
}

void QuickAccess::AddVisit(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    visit_counts[path]++;
    Save();
    if (on_update) on_update();
}

void QuickAccess::SetUpdateCallback(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(mutex);
    on_update = cb;
}

void QuickAccess::Pin(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    if (std::find(pinned_paths.begin(), pinned_paths.end(), path) == pinned_paths.end()) {
        pinned_paths.push_back(path);
        Save();
        if (on_update) on_update();
    }
}

void QuickAccess::Unpin(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = std::remove(pinned_paths.begin(), pinned_paths.end(), path);
    if (it != pinned_paths.end()) {
        pinned_paths.erase(it, pinned_paths.end());
        Save();
        if (on_update) on_update();
    }
}

bool QuickAccess::IsPinned(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    return std::find(pinned_paths.begin(), pinned_paths.end(), path) != pinned_paths.end();
}

std::vector<QuickAccess::Item> QuickAccess::GetItems(int limit) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<Item> result;
    
    // Add pinned items first
    for (const auto& path : pinned_paths) {
        result.push_back({path, visit_counts[path], true});
    }
    
    // Add frequent items (excluding pinned)
    std::vector<std::pair<std::string, int>> items;
    for (const auto& pair : visit_counts) {
        if (std::find(pinned_paths.begin(), pinned_paths.end(), pair.first) == pinned_paths.end()) {
            items.push_back(pair);
        }
    }
    
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    
    for (int i = 0; i < items.size() && result.size() < limit; ++i) {
        result.push_back({items[i].first, items[i].second, false});
    }
    
    return result;
}

void QuickAccess::Save() {
    if (save_path.empty()) return;
    
    std::ofstream out(save_path);
    if (out.is_open()) {
        out << "[Pinned]\n";
        for (const auto& path : pinned_paths) {
            out << path << "\n";
        }
        out << "[Visits]\n";
        for (const auto& pair : visit_counts) {
            out << pair.first << "|" << pair.second << "\n";
        }
    }
}

void QuickAccess::Load() {
    std::ifstream in(save_path);
    if (in.is_open()) {
        std::string line;
        bool reading_pinned = false;
        bool reading_visits = false;
        
        while (std::getline(in, line)) {
            if (line == "[Pinned]") {
                reading_pinned = true;
                reading_visits = false;
                continue;
            } else if (line == "[Visits]") {
                reading_pinned = false;
                reading_visits = true;
                continue;
            }
            
            if (reading_pinned && !line.empty()) {
                pinned_paths.push_back(line);
            } else if (reading_visits && !line.empty()) {
                size_t pipe = line.find('|');
                if (pipe != std::string::npos) {
                    std::string path = line.substr(0, pipe);
                    int count = std::stoi(line.substr(pipe + 1));
                    visit_counts[path] = count;
                }
            }
        }
    }
    
    // Seed defaults if empty
    if (pinned_paths.empty() && visit_counts.empty()) {
        pinned_paths.push_back(GetKnownFolderPath(&FOLDERID_Desktop));
        pinned_paths.push_back(GetKnownFolderPath(&FOLDERID_Documents));
        pinned_paths.push_back(GetKnownFolderPath(&FOLDERID_Downloads));
        pinned_paths.push_back(GetKnownFolderPath(&FOLDERID_Music));
        pinned_paths.push_back(GetKnownFolderPath(&FOLDERID_Pictures));
        pinned_paths.push_back(GetKnownFolderPath(&FOLDERID_Videos));
        pinned_paths.push_back("C:/");
        
        // Initialize counts for these so they aren't zero?
        // Not strictly necessary if they are pinned.
    }
}

}
