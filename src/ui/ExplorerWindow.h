#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Group.H>
#include "ExplorerTab.h"
#include "TabBar.h"
#include "Sidebar.h"
#include <memory>
#include <vector>
#include <chrono>

namespace ui {

class ExplorerWindow : public Fl_Double_Window {
public:
    ExplorerWindow(int w, int h, const char* title, std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now());
    ~ExplorerWindow();
    
    void RefreshUI(); // Updates status from active tab
    void SetAddress(const char* path); // Updates global address bar
    void UpdateStatus();
    void SetAppIcon(Fl_RGB_Image* icon);
    void Navigate(const char* path);
    void AddTab(const char* path);
    void CloseTab(ExplorerTab* tab);
    void SetActiveTab(ExplorerTab* tab);
    
    void SaveWindowPos();
    bool LoadWindowPos();

    int handle(int event) override; // For window dragging
    void resize(int x, int y, int w, int h) override;
    void show() override;

private:
    // Title Bar Area
    // Title Bar Area
    Fl_Group* title_bar = nullptr;
    TabBar* tab_bar = nullptr;
    Fl_Button* btn_min = nullptr;
    Fl_Button* btn_max = nullptr;
    Fl_Button* btn_close = nullptr;
    
    // Navigation Area
    Fl_Group* nav_area = nullptr;
    Fl_Button* btn_back = nullptr;
    Fl_Button* btn_forward = nullptr;
    Fl_Button* btn_up = nullptr;
    Fl_Button* btn_refresh = nullptr;
    Fl_Input* address_bar = nullptr;

    Sidebar* sidebar = nullptr;
    Fl_Group* content_area = nullptr;
    Fl_Box* status_bar = nullptr;
    Fl_RGB_Image* app_icon = nullptr;
    
    ExplorerTab* active_tab = nullptr;
    
    static void NewTabCallback(Fl_Widget* w, void*);
    static void WindowControlCallback(Fl_Widget* w, void* data);
    static void AddressCallback(Fl_Widget* w, void* data);
    static void NavButtonCallback(Fl_Widget* w, void* data);
    
    // Dragging state
    int drag_x = 0, drag_y = 0;
    bool dragging = false;

    // Resizing state
    enum ResizeDir {
        NONE = 0,
        LEFT = 1,
        RIGHT = 2,
        TOP = 4,
        BOTTOM = 8,
        TOP_LEFT = 5,
        TOP_RIGHT = 6,
        BOTTOM_LEFT = 9,
        BOTTOM_RIGHT = 10
    };
    int resize_dir = NONE;
    bool resizing = false;
    int resize_start_x = 0, resize_start_y = 0;
    int resize_start_w = 0, resize_start_h = 0;
    int resize_start_wx = 0, resize_start_wy = 0;

    int GetResizeDir(int x, int y);
    void SetCursor(int dir);

    // Startup Logging
    std::chrono::steady_clock::time_point start_time;
    bool startup_logged = false;
    void CheckStartupTime();
};

}
