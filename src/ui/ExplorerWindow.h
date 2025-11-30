#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Group.H>
#include "ExplorerTab.h"
#include "TabBar.h"
#include <memory>
#include <vector>

namespace ui {

class ExplorerWindow : public Fl_Double_Window {
public:
    ExplorerWindow(int w, int h, const char* title);
    ~ExplorerWindow();
    
    void RefreshUI(); // Updates status from active tab
    void SetAddress(const char* path); // Deprecated or delegates to active tab
    void UpdateStatus();
    void SetAppIcon(Fl_RGB_Image* icon);
    void Navigate(const char* path);
    void AddTab(const char* path);
    void CloseTab(ExplorerTab* tab);
    void SetActiveTab(ExplorerTab* tab);

private:
    TabBar* tab_bar;
    Fl_Group* content_area;
    Fl_Box* status_bar;
    Fl_RGB_Image* app_icon = nullptr;
    
    ExplorerTab* active_tab = nullptr;
    
    static void NewTabCallback(Fl_Widget* w, void*);
};

}
