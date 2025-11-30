
#include "ExplorerWindow.h"
#include "../core/AppState.h"
#include "../core/FileSystem.h"
#include "IconManager.h"
#include <thread>
#include <windows.h> // For CoInitialize
#include <FL/fl_draw.H>
#include <FL/x.H>
#include <fstream>

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
    // Remove OS border
    border(0);
    
    // Schedule icon loading to run after startup
    Fl::add_timeout(0.0, ScheduledIconLoad, this);

    // Try to load position, otherwise center
    if (!LoadWindowPos()) {
        int screen_x, screen_y, screen_w, screen_h;
        Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h);
        position(screen_x + (screen_w - w) / 2, screen_y + (screen_h - h) / 2);
    }

    // --- Title Bar Area (Top) ---
    int title_h = 35;
    title_bar = new Fl_Group(0, 0, w, title_h);
    title_bar->box(FL_FLAT_BOX);
    title_bar->color(fl_rgb_color(240, 240, 240)); // Light gray
    
    // Window Controls (Right)
    int btn_w = 45;
    int btn_h = 30;
    int controls_x = w - (btn_w * 3);
    
    btn_min = new Fl_Button(controls_x, 0, btn_w, btn_h, "_");
    btn_min->box(FL_FLAT_BOX);
    btn_min->callback(WindowControlCallback, this);
    
    btn_max = new Fl_Button(controls_x + btn_w, 0, btn_w, btn_h, "[]");
    btn_max->box(FL_FLAT_BOX);
    btn_max->callback(WindowControlCallback, this);
    
    btn_close = new Fl_Button(controls_x + (btn_w * 2), 0, btn_w, btn_h, "X");
    btn_close->box(FL_FLAT_BOX);
    btn_close->color(fl_rgb_color(232, 17, 35)); // Red
    btn_close->labelcolor(FL_WHITE);
    btn_close->callback(WindowControlCallback, this);
    
    // Tab Bar (Left of controls)
    // Tabs should start from left, maybe after an icon or just left.
    // Let's put them at x=10.
    int tabs_w = controls_x - 10;
    tab_bar = new TabBar(10, 5, tabs_w, 30); // Slightly offset y
    tab_bar->on_tab_selected = [this](void* data) {
        this->SetActiveTab((ExplorerTab*)data);
    };
    tab_bar->on_tab_closed = [this](void* data) {
        this->CloseTab((ExplorerTab*)data);
    };
    tab_bar->on_add_click = [this]() {
        this->AddTab("C:/");
    };
    
    title_bar->end();
    title_bar->resizable(tab_bar); // Allow tabs to expand? No, TabBar handles scrolling?
    // Actually TabBar is a Scroll? Yes.
    
    // --- Navigation Area (Below Title Bar) ---
    int nav_h = 40;
    nav_area = new Fl_Group(0, title_h, w, nav_h);
    nav_area->box(FL_FLAT_BOX);
    nav_area->color(FL_WHITE);
    
    // Address Bar
    address_bar = new Fl_Input(10, title_h + 8, w - 80, 24);
    address_bar->callback(AddressCallback, this);
    address_bar->when(FL_WHEN_ENTER_KEY);
    
    // Go Button
    go_btn = new Fl_Button(w - 65, title_h + 8, 55, 24, "Go");
    go_btn->callback(AddressCallback, this);
    
    nav_area->end();
    nav_area->resizable(address_bar);
    
    // --- Main Content Area (Below Nav) ---
    int main_y = title_h + nav_h;
    int main_h = h - main_y - 25; // Minus status bar
    
    // Sidebar (Left)
    int sidebar_w = 180;
    sidebar = new Sidebar(0, main_y, sidebar_w, main_h);
    sidebar->SetNavigateCallback([this](const std::string& path) {
        this->Navigate(path.c_str());
    });

    // Content Area (Right of Sidebar)
    int content_x = sidebar_w;
    int content_w = w - content_x;
    
    content_area = new Fl_Group(content_x, main_y, content_w, main_h);
    content_area->end(); // Empty initially

    // Status Bar (Bottom)
    status_bar = new Fl_Box(0, h - 25, w, 20, "Ready");
    status_bar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_bar->box(FL_FLAT_BOX);
    status_bar->color(fl_rgb_color(240, 240, 240));

    // Resizing logic
    // We want content_area to resize, sidebar fixed width.
    // And address bar to resize horizontally.
    // This is tricky with flat groups.
    // We can use a tile or just rely on Fl_Group resizable.
    // If we set resizable(content_area), then sidebar stays fixed?
    // But content_area is not direct child of window, it is sibling of sidebar.
    // Wait, they are all children of window.
    // We need a group to hold sidebar + content?
    // Let's keep it simple.
    
    resizable(content_area);
    end();
    
    core::g_main_window = this;
    
    // Add initial tab
    AddTab("C:/");
}

ExplorerWindow::~ExplorerWindow() {
    SaveWindowPos();
    if (app_icon) delete app_icon;
}

int ExplorerWindow::handle(int event) {
    // Let base class handle it first (buttons, tabs, etc.)
    int ret = Fl_Double_Window::handle(event);
    if (ret == 1) return 1; // Child handled it

    if (event == FL_PUSH) {
        // Check if click is in title bar area
        if (Fl::event_y() < 35) {
            dragging = true;
            drag_x = Fl::event_x_root() - x_root();
            drag_y = Fl::event_y_root() - y_root();
            return 1;
        }
    } else if (event == FL_DRAG && dragging) {
        position(Fl::event_x_root() - drag_x, Fl::event_y_root() - drag_y);
        return 1;
    } else if (event == FL_RELEASE) {
        if (dragging) {
            dragging = false;
            return 1;
        }
    }
    return ret;
}

void ExplorerWindow::show() {
    Fl_Double_Window::show();
    
    #ifdef _WIN32
    // Ensure window appears in taskbar even if borderless
    HWND hwnd = fl_xid(this);
    if (hwnd) {
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, style | WS_EX_APPWINDOW);
        // Force update?
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    #endif
}

void ExplorerWindow::WindowControlCallback(Fl_Widget* w, void* data) {
    ExplorerWindow* win = (ExplorerWindow*)data;
    if (w == win->btn_close) {
        win->hide(); // Close application
    } else if (w == win->btn_min) {
        win->iconize();
    } else if (w == win->btn_max) {
        if (win->w() == Fl::w() && win->h() == Fl::h()) {
            // Restore (not easily supported in FLTK without saving rect)
            // For now, just toggle fullscreen-ish or do nothing if already max
            // FLTK doesn't have a simple "restore" from maximize if we are borderless.
            // We'd need to save previous bounds.
            // Let's skip restore for now or just toggle fullscreen.
            win->fullscreen_off(100, 100, 800, 600); // Dummy restore
        } else {
            win->fullscreen();
        }
    }
}

void ExplorerWindow::AddressCallback(Fl_Widget* w, void* data) {
    ExplorerWindow* win = (ExplorerWindow*)data;
    if (win && win->address_bar) {
        win->Navigate(win->address_bar->value());
    }
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
    
    tab->Navigate(path);
    
    // Add to TabBar
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
    
    // Update Address Bar
    if (address_bar) address_bar->value(context->current_path.c_str());
    
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
    if (address_bar) address_bar->value(path);
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

void ExplorerWindow::SaveWindowPos() {
    std::string path = core::GetConfigDir() + "\\window_state.txt";
    std::ofstream out(path);
    if (out.is_open()) {
        out << x() << " " << y() << " " << w() << " " << h();
    }
}

bool ExplorerWindow::LoadWindowPos() {
    std::string path = core::GetConfigDir() + "\\window_state.txt";
    std::ifstream in(path);
    if (in.is_open()) {
        int nx, ny, nw, nh;
        if (in >> nx >> ny >> nw >> nh) {
            resize(nx, ny, nw, nh);
            return true;
        }
    }
    return false;
}

}
