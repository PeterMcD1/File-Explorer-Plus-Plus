#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <mutex>

namespace core {

class QuickAccess {
public:
    struct Item {
        std::string path;
        int score;
        bool pinned;
    };

    static QuickAccess& Get();

    void AddVisit(const std::string& path);
    std::vector<Item> GetItems(int limit = 10);
    
    void Pin(const std::string& path);
    void Unpin(const std::string& path);
    bool IsPinned(const std::string& path);
    
    void Save();
    void Load();
    
    void SetUpdateCallback(std::function<void()> cb);

private:
    QuickAccess();
    ~QuickAccess();

    std::map<std::string, int> visit_counts;
    std::vector<std::string> pinned_paths;
    std::mutex mutex;
    std::string save_path;
    std::function<void()> on_update;
};

}
