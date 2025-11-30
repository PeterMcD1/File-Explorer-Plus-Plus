#include "ExplorerTab.h"
#include "../core/FileSystem.h"
#include <FL/Fl.H>

namespace ui {

ExplorerTab::ExplorerTab(int x, int y, int w, int h) 
    : Fl_Group(x, y, w, h) {
    
    context = std::make_shared<core::TabContext>();
    context->on_update = [this]() {
        this->Refresh();
    };

    // Address Bar (Top)
    address_bar = new Fl_Input(x + 5, y + 5, w - 80, 25);
    address_bar->callback(AddressCallback, this);
    address_bar->when(FL_WHEN_ENTER_KEY);
    
    go_btn = new Fl_Button(x + w - 70, y + 5, 65, 25, "Go");
    go_btn->callback(AddressCallback, this);
    
    // File Table (Below)
    file_table = new FileTable(x, y + 35, w, h - 35, 0, context);
    
    end();
    resizable(file_table);
}

ExplorerTab::~ExplorerTab() {
    // Context will be destroyed when shared_ptr goes out of scope
}

void ExplorerTab::Navigate(const char* path) {
    core::StartLoading(path, context);
}

void ExplorerTab::Refresh() {
    std::lock_guard<std::mutex> lock(context->mutex);
    
    // Update Address Bar
    if (address_bar) address_bar->value(context->current_path.c_str());
    
    file_table->rows((int)context->files.size());
    file_table->redraw();
    
    // We do NOT set the label of ExplorerTab itself, as that would draw a label in the content area.
    // The tab label is managed by TabBar via ExplorerWindow.
}

void ExplorerTab::AddressCallback(Fl_Widget* w, void* data) {
    ExplorerTab* tab = (ExplorerTab*)data;
    if (tab && tab->address_bar) {
        tab->Navigate(tab->address_bar->value());
    }
}

}
