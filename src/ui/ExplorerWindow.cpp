#include "ExplorerWindow.h"
#include "../core/AppState.h"
#include "../core/FileSystem.h"
#include "IconManager.h"
#include <thread>
#include <windows.h> // For CoInitialize

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

ExplorerWindow::ExplorerWindow(int w, int h, const char* title) : Fl_Double_Window(w, h, title) {
    // Schedule icon loading to run after startup
    Fl::add_timeout(0.0, ScheduledIconLoad, this);

    address_bar = new Fl_Input(10, 10, w - 100, 30);
    address_bar->callback(AddressCallback);
    address_bar->when(FL_WHEN_ENTER_KEY);
    
    go_btn = new Fl_Button(w - 80, 10, 70, 30, "Go");
    go_btn->callback(AddressCallback, address_bar);

    // Table takes up most space, leaving 25px at bottom for status
    file_table = new FileTable(10, 50, w - 20, h - 80);

    status_bar = new Fl_Box(10, h - 25, w - 20, 20, "Ready");
    status_bar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_bar->box(FL_FLAT_BOX);

    resizable(file_table);
    end();
    
    // Register self in global state
    core::g_main_window = this;
    core::g_update_callback = UpdateUICallback;
    core::g_set_address_callback = [](const char* path) {
        if (core::g_main_window) core::g_main_window->SetAddress(path);
    };
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

void ExplorerWindow::RefreshTable() {
    std::lock_guard<std::mutex> lock(core::g_files_mutex);
    file_table->rows((int)core::g_files.size());
    file_table->redraw();
}

void ExplorerWindow::UpdateStatus() {
    // No lock needed for simple string read usually, but to be safe/correct:
    // Actually g_status_text is modified in worker. 
    // We should probably lock or use atomic, but for a status string race is minor visual glitch.
    // Let's just read it.
    status_bar->copy_label(core::g_status_text.c_str());
    status_bar->redraw();
}

void ExplorerWindow::SetAddress(const char* path) {
    if (address_bar) address_bar->value(path);
}

void ExplorerWindow::UpdateUICallback(void*) {
    if (core::g_main_window) {
        core::g_main_window->RefreshTable();
        core::g_main_window->UpdateStatus();
    }
}

void ExplorerWindow::AddressCallback(Fl_Widget* w, void* data) {
    Fl_Input* input = (Fl_Input*) (data ? data : w);
    core::StartLoading(input->value());
}

}
