#include "Logger.h"
#include <fstream>
#include <ctime>
#include <iomanip>

namespace core {

static std::ofstream g_log_file;

void Init(const std::string& path) {
    if (g_log_file.is_open()) {
        g_log_file.close();
    }
    g_log_file.open(path, std::ios::out | std::ios::app);
}

void Log(const std::string& msg) {
    if (g_log_file.is_open()) {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        g_log_file << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S] ") << msg << std::endl;
        g_log_file.flush();
    }
}

}
