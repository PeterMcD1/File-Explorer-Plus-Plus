#pragma once
#include <FL/Fl_RGB_Image.H>
#include <string>
#include <map>
#include <mutex>

namespace ui {

class IconManager {
public:
    static IconManager& Get();
    
    // Returns a pointer to a cached Fl_RGB_Image.
    // Do not delete the returned pointer.
    Fl_RGB_Image* GetIcon(const std::string& path, bool is_dir);

    // Returns a new Fl_RGB_Image for a specific file (not cached).
    // Caller owns the returned pointer.
    Fl_RGB_Image* GetSpecificIcon(const std::string& path);

private:
    IconManager() = default;
    ~IconManager();
    
    // Cache key: extension for files, "DIR" for directories
    std::map<std::string, Fl_RGB_Image*> icon_cache_;
    std::mutex cache_mutex_;
    
    Fl_RGB_Image* LoadIconFromSystem(const std::string& path, bool is_dir, bool specific = false);
};

}
