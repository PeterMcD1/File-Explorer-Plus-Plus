#include "ExplorerTab.h"
#include "../core/FileSystem.h"
#include <FL/Fl.H>

namespace ui {

ExplorerTab::ExplorerTab(int x, int y, int w, int h) 
    : Fl_Group(x, y, w, h) {
    
    context = std::make_shared<core::TabContext>();
    context->on_update = [this]() {
        this->Refresh();
        if (this->on_state_changed) this->on_state_changed();
    };

    // File Table (Full space)
    file_table = new FileTable(x, y, w, h, 0, context);
    
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
    
    file_table->rows((int)context->files.size());
    file_table->redraw();
    
    // We do NOT set the label of ExplorerTab itself, as that would draw a label in the content area.
    // The tab label is managed by TabBar via ExplorerWindow.
}

}
