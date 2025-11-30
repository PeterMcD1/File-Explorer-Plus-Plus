#include "FileSystem.h"
#include "TabContext.h"
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

// Static callback to execute the context's update function on the main thread
static void ContextUpdateCallback(void* data) {
    auto* context = static_cast<TabContext*>(data);
    if (context && context->on_update) {
        context->on_update();
    }
}

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

void LoadDirectoryWorker(std::string path, std::shared_ptr<TabContext> context) {
    Log("Worker started for: " + path);
    try {
        // Clear existing data
        {
            std::lock_guard<std::mutex> lock(context->mutex);
            context->files.clear();
            context->current_path = path;
            context->status_text = "Loading...";
        }
        Fl::awake(ContextUpdateCallback, context.get());

        std::vector<FileEntry> all_files;
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
                    context->status_text = "Loading... " + std::to_string(count) + " items found";
                    Fl::awake(ContextUpdateCallback, context.get());
                }

            } catch (const std::exception& e) {
                Log("Error processing entry: " + std::string(e.what()));
            }
        }

        // Sort
        std::sort(all_files.begin(), all_files.end(), [](const FileEntry& a, const FileEntry& b) {
            if (a.is_dir != b.is_dir) {
                return a.is_dir > b.is_dir; 
            }
            return std::lexicographical_compare(
                a.name.begin(), a.name.end(),
                b.name.begin(), b.name.end(),
                [](unsigned char c1, unsigned char c2) {
                    return std::tolower(c1) < std::tolower(c2);
                }
            );
        });

        // Update state
        {
            std::lock_guard<std::mutex> lock(context->mutex);
            context->files = std::move(all_files);
            context->status_text = std::to_string(context->files.size()) + " items";
        }

    } catch (const std::exception& e) {
        Log("Worker crashed: " + std::string(e.what()));
        context->status_text = "Error loading directory";
    } catch (...) {
        Log("Worker crashed with unknown error");
        context->status_text = "Unknown error";
    }

    context->is_loading = false;
    Fl::awake(ContextUpdateCallback, context.get());
    Log("Worker finished for: " + path);
}

void StartLoading(const std::string& path, std::shared_ptr<TabContext> context) {
    if (context->is_loading) {
        Log("Skipping load, already loading.");
        return; 
    }
    context->is_loading = true;
    Log("Requesting load for: " + path);
    
    // Address bar update is now handled by UI observing the context or explicit UI call
    // We don't update address bar here anymore because we don't know which tab is active
    
    std::thread(LoadDirectoryWorker, path, context).detach();
}

}
