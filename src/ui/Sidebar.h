#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_RGB_Image.H>
#include <vector>
#include <string>
#include <functional>
#include <map>

namespace ui {

class Sidebar : public Fl_Group {
public:
    Sidebar(int x, int y, int w, int h);
    ~Sidebar();
    
    void SetNavigateCallback(std::function<void(const std::string&)> cb);
    void Refresh();

private:
    class SidebarButton : public Fl_Button {
    public:
        SidebarButton(int x, int y, int w, int h, const char* label);
        void draw() override;
        int handle(int event) override;
        bool pinned = false;
        Fl_Image* icon = nullptr;
        std::string path; // Store path for context menu
    };

    struct SidebarItem {
        SidebarButton* btn;
        std::string path;
    };

    void AddButton(const char* label, const std::string& path, int& cur_y, bool pinned);
    std::string GetKnownFolderPath(const void* rfid);
    
    void LoadIcons();
    static void LoadIconsCallback(void* data);

    std::function<void(const std::string&)> on_navigate;
    std::vector<SidebarItem> items;
    std::vector<Fl_RGB_Image*> icons; // Still needed for ownership if not in cache?
    std::map<std::string, Fl_RGB_Image*> icon_cache;
};

}
