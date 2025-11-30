#pragma once
#include <string>

namespace core {

struct FileEntry {
    std::string name;
    std::string size_str;
    bool is_dir;
    std::string path;
};

}
