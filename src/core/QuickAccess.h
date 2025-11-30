#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace core {

class QuickAccess {
public:
    static QuickAccess& Get();

    void AddVisit(const std::string& path);
    std::vector<std::string> GetTopPaths(int limit);
    void Save();
    void Load();

private:
    QuickAccess();
    ~QuickAccess();

    std::map<std::string, int> visit_counts;
    std::mutex mutex;
    std::string save_path;
};

}
