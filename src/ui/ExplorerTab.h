#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <memory>
#include "../core/TabContext.h"
#include "FileTable.h"
#include <functional>

namespace ui {

class ExplorerTab : public Fl_Group {
public:
    ExplorerTab(int x, int y, int w, int h);
    ~ExplorerTab();

    void Navigate(const char* path);
    void Refresh();
    
    std::shared_ptr<core::TabContext> GetContext() { return context; }
    
    // Callback for close request (from parent window logic if needed, but now handled by TabBar)
    std::function<void(ExplorerTab*)> on_close;

private:
    FileTable* file_table;
    std::shared_ptr<core::TabContext> context;
};

}
