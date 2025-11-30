#pragma once
#include <string>
#include <cstdint>

namespace core {
    void StartLoading(const std::string& path);
    std::string FormatSize(uintmax_t size);
}
