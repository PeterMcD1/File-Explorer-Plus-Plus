#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_RGB_Image.H>
#include "FileTable.h"

namespace ui {

class ExplorerWindow : public Fl_Double_Window {
public:
    ExplorerWindow(int w, int h, const char* title);
    ~ExplorerWindow();
    
    void RefreshTable();
    void SetAddress(const char* path);
    void UpdateStatus();
    void SetAppIcon(Fl_RGB_Image* icon);
    
    // Static callback for Fl::awake
    static void UpdateUICallback(void*);

private:
    Fl_Input* address_bar;
    Fl_Button* go_btn;
    FileTable* file_table;
    Fl_Box* status_bar;
    Fl_RGB_Image* app_icon = nullptr;
    
    static void AddressCallback(Fl_Widget* w, void*);
};

}
