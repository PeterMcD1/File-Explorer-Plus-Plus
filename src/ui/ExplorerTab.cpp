#include "ExplorerTab.h"
#include "../core/FileSystem.h"
#include "IconManager.h"
#include <FL/Fl.H>
#include <FL/Fl_RGB_Image.H>

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
    file_table->on_navigate = [this](const std::string& path) {
        if (this->on_navigate) {
            this->on_navigate(path);
        } else {
            this->Navigate(path.c_str());
        }
    };
    
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
    
    // Update icon
    if (!context->current_path.empty()) {
        Fl_RGB_Image* icon = IconManager::Get().GetSpecificIcon(context->current_path);
        if (!icon) {
            // Fallback to generic directory icon
            icon = IconManager::Get().GetIcon(context->current_path, true);
        }
        
        if (icon != current_icon) {
            current_icon = icon;
            if (on_icon_changed) on_icon_changed(icon);
        }
    }
}

}
