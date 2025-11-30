#include "ExplorerWindow.h"
#include "../core/AppState.h"
#include "../core/FileSystem.h"
#include "../core/Logger.h"
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
        
        Fl_RGB_Image* icon = IconManager::Get().GetSpecificIcon("C:\\Windows\\explorer.exe", true);
        
        CoUninitialize();

        if (icon) {
            Fl::awake(SetIconCallback, new std::pair<ExplorerWindow*, Fl_RGB_Image*>(win, icon));
        }
    }).detach();
}

// Custom Button for Navigation to handle disabled state styling
class NavButton : public Fl_Button {
public:
    NavButton(int x, int y, int w, int h, const char* l = 0) : Fl_Button(x, y, w, h, l) {
        box(FL_FLAT_BOX);
        align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    }
    
    void draw() override {
        // Draw background
        fl_color(color());
        fl_rectf(x(), y(), w(), h());
        
        // Draw label
        if (active_r()) {
            fl_color(labelcolor());
        } else {
            fl_color(fl_rgb_color(100, 100, 100)); // Greyed out
        }
        
        fl_font(labelfont(), labelsize());
        fl_draw(label(), x(), y(), w(), h(), align());
    }
};

ExplorerWindow::ExplorerWindow(int w, int h, const char* title, std::chrono::steady_clock::time_point start_time) 
    : Fl_Double_Window(w, h, title), start_time(start_time) {
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
    int title_h = 40;
    title_bar = new Fl_Group(0, 0, w, title_h);
    title_bar->box(FL_FLAT_BOX);
    title_bar->color(fl_rgb_color(45, 45, 45)); // #2D2D2D
    
    // Window Controls (Right)
    int btn_w = 45;
    int btn_h = 30;
    int controls_x = w - (btn_w * 3);
    
    btn_min = new Fl_Button(controls_x, 0, btn_w, btn_h, "_");
    btn_min->box(FL_FLAT_BOX);
    btn_min->color(fl_rgb_color(45, 45, 45));
    btn_min->labelcolor(FL_WHITE);
    btn_min->callback(WindowControlCallback, this);
    
    btn_max = new Fl_Button(controls_x + btn_w, 0, btn_w, btn_h, "[]");
    btn_max->box(FL_FLAT_BOX);
    btn_max->color(fl_rgb_color(45, 45, 45));
    btn_max->labelcolor(FL_WHITE);
    btn_max->callback(WindowControlCallback, this);
    
    btn_close = new Fl_Button(controls_x + (btn_w * 2), 0, btn_w, btn_h, "X");
    btn_close->box(FL_FLAT_BOX);
    btn_close->color(fl_rgb_color(45, 45, 45));
    btn_close->labelcolor(FL_WHITE);
    btn_close->callback(WindowControlCallback, this);
    
    // Tab Bar (Left of controls)
    int tabs_w = controls_x - 10;
    tab_bar = new TabBar(10, 5, tabs_w, 35);
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
    title_bar->resizable(tab_bar);
    
    // --- Navigation Area (Below Title Bar) ---
    int nav_h = 40;
    nav_area = new Fl_Group(0, title_h, w, nav_h);
    nav_area->box(FL_FLAT_BOX);
    nav_area->color(fl_rgb_color(56, 56, 56)); // #383838 Slightly lighter than title bar
    
    int btn_x = 10;
    int nav_btn_w = 30;
    int nav_btn_h = 24;
    int nav_btn_y = title_h + 8;
    int spacing = 5;
    
    // Back
    btn_back = new NavButton(btn_x, nav_btn_y, nav_btn_w, nav_btn_h, "←");
    btn_back->color(fl_rgb_color(56, 56, 56));
    btn_back->labelcolor(FL_WHITE);
    btn_back->labelsize(18); // Larger icon
    btn_back->callback(NavButtonCallback, this);
    btn_back->deactivate();
    btn_x += nav_btn_w + spacing;
    
    // Forward
    btn_forward = new NavButton(btn_x, nav_btn_y, nav_btn_w, nav_btn_h, "→");
    btn_forward->color(fl_rgb_color(56, 56, 56));
    btn_forward->labelcolor(FL_WHITE);
    btn_forward->labelsize(18);
    btn_forward->callback(NavButtonCallback, this);
    btn_forward->deactivate();
    btn_x += nav_btn_w + spacing;
    
    // Up
    btn_up = new NavButton(btn_x, nav_btn_y, nav_btn_w, nav_btn_h, "↑");
    btn_up->color(fl_rgb_color(56, 56, 56));
    btn_up->labelcolor(FL_WHITE);
    btn_up->labelsize(18);
    btn_up->callback(NavButtonCallback, this);
    btn_x += nav_btn_w + spacing;
    
    // Refresh
    btn_refresh = new NavButton(btn_x, nav_btn_y, nav_btn_w, nav_btn_h, "⟳");
    btn_refresh->color(fl_rgb_color(56, 56, 56));
    btn_refresh->labelcolor(FL_WHITE);
    btn_refresh->labelsize(18);
    btn_refresh->callback(NavButtonCallback, this);
    btn_x += nav_btn_w + spacing;
    
    // Address Bar
    address_bar = new Fl_Input(btn_x, nav_btn_y, w - btn_x - 10, 24);
    address_bar->box(FL_FLAT_BOX);
    address_bar->color(fl_rgb_color(51, 51, 51)); // #333333
    address_bar->textcolor(FL_WHITE);
    address_bar->callback(AddressCallback, this);
    address_bar->when(FL_WHEN_ENTER_KEY);
    
    nav_area->end();
    nav_area->resizable(address_bar);
    
    // --- Main Content Area (Below Nav) ---
    int main_y = title_h + nav_h;
    int main_h = h - main_y - 25; // Minus status bar
    
    // Sidebar (Left)
    int sidebar_w = 200;
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
    status_bar->color(fl_rgb_color(45, 45, 45)); // #2D2D2D
    status_bar->labelcolor(FL_WHITE);

    resizable(content_area);
    end();
    
    core::g_main_window = this;
    
    // Add initial tab
    AddTab("C:/");
    
    // Force layout update to match current window size (in case LoadWindowPos changed it)
    resize(x(), y(), this->w(), this->h());
}

ExplorerWindow::~ExplorerWindow() {
    SaveWindowPos();
    if (app_icon) delete app_icon;
}

int ExplorerWindow::handle(int event) {
    // Prioritize resize cursor and hit testing
    if (event == FL_MOVE) {
        int dir = GetResizeDir(Fl::event_x(), Fl::event_y());
        if (dir != NONE) {
            SetCursor(dir);
            return 1;
        } else {
            // Reset cursor if we moved out of border
            // But we must be careful not to override child cursors (like text input)
            // If we just call base handle, child might set it.
            // If child doesn't set it, it remains as previous.
            // So we set to default, then call base.
            cursor(FL_CURSOR_DEFAULT);
        }
    }

    // Let base class handle it first (buttons, tabs, etc.)
    // BUT only if we are not resizing. If resizing, we consume events.
    if (!resizing) {
        int ret = Fl_Double_Window::handle(event);
        if (ret == 1) return 1; // Child handled it
    }

    if (event == FL_PUSH) {
        int dir = GetResizeDir(Fl::event_x(), Fl::event_y());
        if (dir != NONE) {
            resizing = true;
            resize_dir = dir;
            resize_start_x = Fl::event_x_root();
            resize_start_y = Fl::event_y_root();
            resize_start_w = w();
            resize_start_h = h();
            resize_start_wx = x();
            resize_start_wy = y();
            return 1;
        }

        // Check if click is in title bar area
        if (Fl::event_y() < 35) {
            dragging = true;
            drag_x = Fl::event_x_root() - x_root();
            drag_y = Fl::event_y_root() - y_root();
            return 1;
        }
    } else if (event == FL_DRAG) {
        if (resizing) {
            int dx = Fl::event_x_root() - resize_start_x;
            int dy = Fl::event_y_root() - resize_start_y;
            
            int new_x = resize_start_wx;
            int new_y = resize_start_wy;
            int new_w = resize_start_w;
            int new_h = resize_start_h;

            if (resize_dir & LEFT) {
                new_x += dx;
                new_w -= dx;
            }
            if (resize_dir & RIGHT) {
                new_w += dx;
            }
            if (resize_dir & TOP) {
                new_y += dy;
                new_h -= dy;
            }
            if (resize_dir & BOTTOM) {
                new_h += dy;
            }

            // Min size constraint
            if (new_w < 400) new_w = 400;
            if (new_h < 300) new_h = 300;

            resize(new_x, new_y, new_w, new_h);
            return 1;
        }

        if (dragging) {
            position(Fl::event_x_root() - drag_x, Fl::event_y_root() - drag_y);
            return 1;
        }
    } else if (event == FL_RELEASE) {
        if (resizing) {
            resizing = false;
            cursor(FL_CURSOR_DEFAULT);
            return 1;
        }
        if (dragging) {
            dragging = false;
            return 1;
        }
    } else if (event == FL_LEAVE) {
        cursor(FL_CURSOR_DEFAULT);
    }
    return Fl_Double_Window::handle(event);
}

int ExplorerWindow::GetResizeDir(int x, int y) {
    int dir = NONE;
    int border = 5; // Resize border thickness

    if (x < border) dir |= LEFT;
    if (x > w() - border) dir |= RIGHT;
    if (y < border) dir |= TOP;
    if (y > h() - border) dir |= BOTTOM;

    return dir;
}

void ExplorerWindow::SetCursor(int dir) {
    switch (dir) {
        case LEFT: case RIGHT: cursor(FL_CURSOR_WE); break;
        case TOP: case BOTTOM: cursor(FL_CURSOR_NS); break;
        case TOP_LEFT: case BOTTOM_RIGHT: cursor(FL_CURSOR_NWSE); break;
        case TOP_RIGHT: case BOTTOM_LEFT: cursor(FL_CURSOR_NESW); break;
        default: cursor(FL_CURSOR_DEFAULT); break;
    }
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

void ExplorerWindow::NavButtonCallback(Fl_Widget* w, void* data) {
    ExplorerWindow* win = (ExplorerWindow*)data;
    if (!win || !win->active_tab) return;
    
    auto context = win->active_tab->GetContext();
    std::string current_path;
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        current_path = context->current_path;
    }
    
    if (w == win->btn_back) {
        std::string prev;
        {
            std::lock_guard<std::mutex> lock(context->mutex);
            if (!context->history_back.empty()) {
                prev = context->history_back.back();
                context->history_back.pop_back();
                // Push current to forward
                if (!current_path.empty()) {
                    context->history_forward.push_back(current_path);
                }
            }
        }
        if (!prev.empty()) win->active_tab->Navigate(prev.c_str());
        
    } else if (w == win->btn_forward) {
        std::string next;
        {
            std::lock_guard<std::mutex> lock(context->mutex);
            if (!context->history_forward.empty()) {
                next = context->history_forward.back();
                context->history_forward.pop_back();
                // Push current to back
                if (!current_path.empty()) {
                    context->history_back.push_back(current_path);
                }
            }
        }
        if (!next.empty()) win->active_tab->Navigate(next.c_str());
        
    } else if (w == win->btn_up) {
        std::string path = current_path;
        if (path.length() <= 3) return; // Already at root (e.g. C:/)
        
        if (path.back() == '/' || path.back() == '\\') path.pop_back();
        size_t pos = path.find_last_of("/\\");
        
        if (pos != std::string::npos) {
            std::string parent = path.substr(0, pos + 1);
            if (parent.length() == 2 && parent[1] == ':') parent += "/"; // Ensure C: becomes C:/
            
            // Navigate (pushes to history)
            win->Navigate(parent.c_str());
        } else {
            // Maybe just drive letter like "C:"?
            if (path.length() == 2 && path[1] == ':') {
                 // Already root?
            }
        }
    } else if (w == win->btn_refresh) {
        // Use Navigate to reload safely
        win->active_tab->Navigate(current_path.c_str());
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
    
    // Hook up state change callback (for UI refresh and logging)
    tab->SetStateChangeCallback([this, tab]() {
        // Refresh UI if this is the active tab
        if (this->active_tab == tab) {
            this->RefreshUI();
        }

        // Check if loading finished (Startup Logging)
        if (!startup_logged) {
            auto context = tab->GetContext();
            std::lock_guard<std::mutex> lock(context->mutex);
            if (!context->is_loading) {
                CheckStartupTime();
            }
        }
        
        // Update tab title
        auto context = tab->GetContext();
        std::string label = context->current_path;
        if (label.empty()) label = "New Tab";
        else {
            if (label.back() == '/' || label.back() == '\\') label.pop_back();
            size_t pos = label.find_last_of("/\\");
            if (pos != std::string::npos) label = label.substr(pos + 1);
        }
        tab_bar->UpdateTabLabel(tab, label.c_str());
    });
    
    // Set icon change callback
    tab->SetIconChangeCallback([this, tab](Fl_RGB_Image* icon) {
        tab_bar->UpdateTabIcon(tab, icon);
    });

    // Set navigation callback (to handle history)
    tab->SetNavigateCallback([this](const std::string& path) {
        this->Navigate(path.c_str());
    });
    
    // Initial navigation
    // Note: We don't push initial path to history usually, or we do?
    // Let's say initial path is just current. History empty.
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
    
    // Update Buttons
    if (btn_back) {
        if (context->history_back.empty()) btn_back->deactivate();
        else btn_back->activate();
    }
    if (btn_forward) {
        if (context->history_forward.empty()) btn_forward->deactivate();
        else btn_forward->activate();
    }
    
    if (btn_up) {
        std::string path = context->current_path;
        if (path.length() <= 3) btn_up->deactivate(); // Root (C:/)
        else btn_up->activate();
    }
    
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
        auto context = active_tab->GetContext();
        {
            std::lock_guard<std::mutex> lock(context->mutex);
            // Push current to history if different
            if (!context->current_path.empty() && context->current_path != path) {
                context->history_back.push_back(context->current_path);
                context->history_forward.clear(); // Clear forward on new navigation
            }
        }
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

void ExplorerWindow::CheckStartupTime() {
    if (startup_logged) return;
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
    
    core::Log("Total startup load time: " + std::to_string(duration) + " ms");
    startup_logged = true;
}

void ExplorerWindow::resize(int x, int y, int w, int h) {
    Fl_Double_Window::resize(x, y, w, h);
    
    // Layout Constants
    int title_h = 40;
    int nav_h = 40;
    int status_h = 25;
    int sidebar_w = 200;
    int btn_w = 45;
    int btn_h = 30;
    
    // 1. Title Bar
    if (title_bar) {
        title_bar->resize(0, 0, w, title_h);
        
        // Controls
        int controls_x = w - (btn_w * 3);
        if (btn_min) btn_min->resize(controls_x, 0, btn_w, btn_h);
        if (btn_max) btn_max->resize(controls_x + btn_w, 0, btn_w, btn_h);
        if (btn_close) btn_close->resize(controls_x + (btn_w * 2), 0, btn_w, btn_h);
        
        // Tab Bar
        if (tab_bar) {
            int tabs_w = controls_x - 10;
            tab_bar->resize(10, 5, tabs_w, 35);
        }
    }
    
    // 2. Navigation Area
    if (nav_area) {
        nav_area->resize(0, title_h, w, nav_h);
        
        int btn_x = 10;
        int nav_btn_w = 30;
        int nav_btn_h = 24;
        int nav_btn_y = title_h + 8;
        int spacing = 5;
        
        if (btn_back) { btn_back->resize(btn_x, nav_btn_y, nav_btn_w, nav_btn_h); btn_x += nav_btn_w + spacing; }
        if (btn_forward) { btn_forward->resize(btn_x, nav_btn_y, nav_btn_w, nav_btn_h); btn_x += nav_btn_w + spacing; }
        if (btn_up) { btn_up->resize(btn_x, nav_btn_y, nav_btn_w, nav_btn_h); btn_x += nav_btn_w + spacing; }
        if (btn_refresh) { btn_refresh->resize(btn_x, nav_btn_y, nav_btn_w, nav_btn_h); btn_x += nav_btn_w + spacing; }
        
        if (address_bar) {
            address_bar->resize(btn_x, nav_btn_y, w - btn_x - 10, 24);
        }
    }
    
    // 3. Main Content Area
    int main_y = title_h + nav_h;
    int main_h = h - main_y - status_h;
    
    if (sidebar) {
        sidebar->resize(0, main_y, sidebar_w, main_h);
    }
    
    if (content_area) {
        content_area->resize(sidebar_w, main_y, w - sidebar_w, main_h);
        // Resize active tab if any
        if (active_tab) {
            active_tab->resize(content_area->x(), content_area->y(), content_area->w(), content_area->h());
        }
    }
    
    // 4. Status Bar
    if (status_bar) {
        status_bar->resize(0, h - status_h, w, 20);
    }
}

}

