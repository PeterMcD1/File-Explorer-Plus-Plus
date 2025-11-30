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
    std::string appData = GetKnownFolderPath(&FOLDERID_RoamingAppData);
    if (!appData.empty()) {
        std::string dir = appData + "\\FlashExplorer";
        std::filesystem::create_directories(dir);
        save_path = dir + "\\quick_access.txt";
    } else {
        save_path = "quick_access.txt";
    }
    Load();
}

QuickAccess::~QuickAccess() {
    Save();
}

void QuickAccess::AddVisit(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    visit_counts[path]++;
    Save();
}

std::vector<std::string> QuickAccess::GetTopPaths(int limit) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<std::pair<std::string, int>> items;
    for (const auto& pair : visit_counts) {
        items.push_back(pair);
    }
    
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    
    std::vector<std::string> result;
    for (int i = 0; i < items.size() && i < limit; ++i) {
        result.push_back(items[i].first);
    }
    
    // Ensure we have at least defaults if empty (should be handled by Load, but just in case)
    if (result.empty()) {
        // Fallback
    }
    
    return result;
}

void QuickAccess::Save() {
    if (save_path.empty()) return;
    
    std::ofstream out(save_path);
    if (out.is_open()) {
        for (const auto& pair : visit_counts) {
            out << pair.first << "|" << pair.second << "\n";
        }
    }
}

void QuickAccess::Load() {
    std::ifstream in(save_path);
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            size_t pipe = line.find('|');
            if (pipe != std::string::npos) {
                std::string path = line.substr(0, pipe);
                int count = std::stoi(line.substr(pipe + 1));
                visit_counts[path] = count;
            }
        }
    }
    
    // Seed defaults if empty
    if (visit_counts.empty()) {
        visit_counts[GetKnownFolderPath(&FOLDERID_Desktop)] = 1;
        visit_counts[GetKnownFolderPath(&FOLDERID_Documents)] = 1;
        visit_counts[GetKnownFolderPath(&FOLDERID_Downloads)] = 1;
        visit_counts[GetKnownFolderPath(&FOLDERID_Music)] = 1;
        visit_counts[GetKnownFolderPath(&FOLDERID_Pictures)] = 1;
        visit_counts[GetKnownFolderPath(&FOLDERID_Videos)] = 1;
        visit_counts["C:/"] = 1;
    }
}

}
