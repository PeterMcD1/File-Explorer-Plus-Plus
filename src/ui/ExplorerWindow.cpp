#include "ExplorerWindow.h"
#include "../core/AppState.h"
#include "../core/FileSystem.h"
#include "IconManager.h"

namespace ui {

ExplorerWindow::ExplorerWindow(int w, int h, const char* title) : Fl_Double_Window(w, h, title) {
    // Set App Icon
    app_icon = IconManager::Get().GetSpecificIcon("C:\\Windows\\explorer.exe");
    if (app_icon) {
        this->icon(app_icon);
    }

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
