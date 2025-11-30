#include "FileSystem.h"
#include "AppState.h"
#include "Logger.h"
#include <FL/Fl.H>
#include <filesystem>
#include <thread>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace core {

std::string FormatSize(uintmax_t size) {
    if (size < 1024) return std::to_string(size) + " B";
    if (size < 1024 * 1024) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (size / 1024.0) << " KB";
        return ss.str();
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << " MB";
    return ss.str();
}

void LoadDirectoryWorker(std::string path) {
    Log("Worker started for: " + path);
    try {
        // Clear existing data immediately so UI shows empty/loading state
        {
            std::lock_guard<std::mutex> lock(g_files_mutex);
            g_files.clear();
            g_current_path = path;
            g_status_text = "Loading...";
        }
        if (g_update_callback) Fl::awake(g_update_callback, nullptr);

        std::vector<FileEntry> all_files;
        // Reserve some space to avoid reallocations
        all_files.reserve(4096);

        std::error_code ec;
        int count = 0;
        for (const auto& entry : fs::directory_iterator(path, ec)) {
            if (ec) {
                Log("Error accessing " + path + ": " + ec.message());
                break; 
            }

            FileEntry fe;
            try {
                fe.path = entry.path().string();
                fe.name = entry.path().filename().string();
                
                std::error_code status_ec;
                bool is_directory = entry.is_directory(status_ec);
                fe.is_dir = !status_ec && is_directory;

                if (fe.is_dir) {
                    fe.size_str = "<DIR>";
                } else {
                    uintmax_t size = entry.file_size(status_ec);
                    fe.size_str = status_ec ? "Unknown" : FormatSize(size);
                }

                all_files.push_back(fe);
                count++;
                
                if (count % 1000 == 0) {
                    g_status_text = "Loading... " + std::to_string(count) + " items found";
                    if (g_update_callback) Fl::awake(g_update_callback, nullptr);
                }

            } catch (const std::exception& e) {
                Log("Error processing entry: " + std::string(e.what()));
            }
        }

        // Sort: Directories first, then alphabetical (case-insensitive)
        std::sort(all_files.begin(), all_files.end(), [](const FileEntry& a, const FileEntry& b) {
            if (a.is_dir != b.is_dir) {
                return a.is_dir > b.is_dir; // True (Dir) comes before False (File)
            }
            
            // Case-insensitive comparison
            return std::lexicographical_compare(
                a.name.begin(), a.name.end(),
                b.name.begin(), b.name.end(),
                [](unsigned char c1, unsigned char c2) {
                    return std::tolower(c1) < std::tolower(c2);
                }
            );
        });

        // Update global state in one go
        {
            std::lock_guard<std::mutex> lock(g_files_mutex);
            g_files = std::move(all_files);
            g_status_text = std::to_string(g_files.size()) + " items";
        }

    } catch (const std::exception& e) {
        Log("Worker crashed: " + std::string(e.what()));
        g_status_text = "Error loading directory";
    } catch (...) {
        Log("Worker crashed with unknown error");
        g_status_text = "Unknown error";
    }

    g_loading = false;
    if (g_update_callback) Fl::awake(g_update_callback, nullptr);
    Log("Worker finished for: " + path);
}

void StartLoading(const std::string& path) {
    if (g_loading) {
        Log("Skipping load, already loading.");
        return; 
    }
    g_loading = true;
    Log("Requesting load for: " + path);
    
    // Update address bar if window exists
    if (g_set_address_callback) {
        g_set_address_callback(path.c_str());
    }
    
    std::thread(LoadDirectoryWorker, path).detach();
}

}
