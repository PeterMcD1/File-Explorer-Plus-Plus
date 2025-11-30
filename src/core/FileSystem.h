#pragma once
#include <string>
#include <cstdint>
#include <memory>

namespace core {
    struct TabContext;
    void StartLoading(const std::string& path, std::shared_ptr<TabContext> context);
    std::string FormatSize(uintmax_t size);
    std::string GetConfigDir();
    std::string GetKnownFolderPath(const void* rfid);
}
