
#include "ExplorerWindow.h"
#include "../core/AppState.h"
#include "../core/FileSystem.h"
#include "IconManager.h"
#include <thread>
#include <windows.h> // For CoInitialize
#include <FL/fl_draw.H>

namespace ui {

// Static callback for setting icon on main thread
void SetIconCallback(void* data) {
    auto* pair = static_cast<std::pair<ExplorerWindow*, Fl_RGB_Image*>*>(data);
    if (pair->first) {
        pair->first->SetAppIcon(pair->second);
    }
    delete pair;
}

// Static callback to start the thread after a delay
void ScheduledIconLoad(void* data) {
    ExplorerWindow* win = static_cast<ExplorerWindow*>(data);
    std::thread([win]() {
        // Initialize COM for this thread
        CoInitialize(NULL);
        
        Fl_RGB_Image* icon = IconManager::Get().GetSpecificIcon("C:\\Windows\\explorer.exe");
        
        CoUninitialize();

        if (icon) {
            Fl::awake(SetIconCallback, new std::pair<ExplorerWindow*, Fl_RGB_Image*>(win, icon));
        }
    }).detach();
}

// ... (Includes and static callbacks remain similar)

ExplorerWindow::ExplorerWindow(int w, int h, const char* title) : Fl_Double_Window(w, h, title) {
    // Schedule icon loading to run after startup
    Fl::add_timeout(0.0, ScheduledIconLoad, this);

    // Tab Bar (Top)
    tab_bar = new TabBar(10, 10, w - 20, 30);
    tab_bar->on_tab_selected = [this](void* data) {
        this->SetActiveTab((ExplorerTab*)data);
    };
    tab_bar->on_tab_closed = [this](void* data) {
        this->CloseTab((ExplorerTab*)data);
    };
    tab_bar->on_add_click = [this]() {
        this->AddTab("C:/");
    };
    
    // Content Area
    content_area = new Fl_Group(10, 40, w - 20, h - 70);
    content_area->end(); // Empty initially

    status_bar = new Fl_Box(10, h - 25, w - 20, 20, "Ready");
    status_bar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_bar->box(FL_FLAT_BOX);

    resizable(content_area);
    end();
    
    core::g_main_window = this;
    
    // Add initial tab
    AddTab("C:/");
}

ExplorerWindow::~ExplorerWindow() {
    if (app_icon) delete app_icon;
}

void ExplorerWindow::SetAppIcon(Fl_RGB_Image* icon) {
    if (app_icon) delete app_icon; // Remove old if any
    app_icon = icon;
    this->icon(app_icon);
    this->redraw();
}

void ExplorerWindow::AddTab(const char* path) {
    content_area->begin();
    ExplorerTab* tab = new ExplorerTab(content_area->x(), content_area->y(), content_area->w(), content_area->h());
    content_area->add(tab);
    content_area->end();
    
    // Hook up update callback
    auto context = tab->GetContext();
    context->on_update = [this, tab]() {
        tab->Refresh();
        if (this->active_tab == tab) {
            this->RefreshUI();
        }
    };
    
    // We don't need tab->on_close anymore because TabBar handles the close button click
    // But wait, ExplorerTab still has the close button in the toolbar?
    // The user wanted it "on the tab".
    // If I put it on the TabBar, I should probably remove it from ExplorerTab toolbar to avoid redundancy?
    // Or keep both?
    // User said "the x button isn't on the tab".
    // I'll keep the one in TabBar.
    // I should probably remove the one in ExplorerTab later.
    
    tab->Navigate(path);
    
    // Add to TabBar
    // Label should be directory name
    std::string label = "New Tab";
    if (path) label = path;
    tab_bar->AddTab(label.c_str(), tab);
    
    SetActiveTab(tab);
}

void ExplorerWindow::CloseTab(ExplorerTab* tab) {
    if (!tab) return;
    
    bool closing_active = (active_tab == tab);
    
    // Remove from TabBar
    tab_bar->RemoveTab(tab);
    
    // Remove from content area
    content_area->remove(tab);
    delete tab;
    
    if (closing_active) {
        active_tab = nullptr;
        // Select last tab if any
        if (content_area->children() > 0) {
            SetActiveTab((ExplorerTab*)content_area->child(content_area->children() - 1));
        } else {
            AddTab("C:/");
        }
    }
}

void ExplorerWindow::SetActiveTab(ExplorerTab* tab) {
    active_tab = tab;
    
    // Show only active tab
    for (int i = 0; i < content_area->children(); i++) {
        Fl_Widget* child = content_area->child(i);
        if (child == tab) child->show();
        else child->hide();
    }
    
    tab_bar->SelectTab(tab);
    RefreshUI();
    content_area->redraw();
}

void ExplorerWindow::RefreshUI() {
    if (!active_tab) return;
    
    auto context = active_tab->GetContext();
    std::lock_guard<std::mutex> lock(context->mutex);
    
    // Update Status
    status_bar->label(context->status_text.c_str());
    status_bar->redraw();
    
    // Update Tab Label
    if (!context->current_path.empty()) {
        std::string label = context->current_path;
        if (label.back() == '/' || label.back() == '\\') label.pop_back();
        size_t pos = label.find_last_of("/\\");
        if (pos != std::string::npos) label = label.substr(pos + 1);
        if (label.empty()) label = context->current_path; // Root like C:/
        
        tab_bar->UpdateTabLabel(active_tab, label.c_str());
    }
}

void ExplorerWindow::SetAddress(const char* path) {
    if (active_tab) active_tab->Navigate(path);
}

void ExplorerWindow::UpdateStatus() {
    RefreshUI();
}

void ExplorerWindow::NewTabCallback(Fl_Widget* w, void* data) {
    ExplorerWindow* win = (ExplorerWindow*)data;
    win->AddTab("C:/");
}

void ExplorerWindow::Navigate(const char* path) {
    if (active_tab) {
        active_tab->Navigate(path);
    }
}

}
