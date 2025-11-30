#include "Sidebar.h"
#include "IconManager.h"
#include "../core/QuickAccess.h"
#include <shlobj.h>
#include <windows.h>
#include <iostream>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Item.H>

namespace ui {

Sidebar::Sidebar(int x, int y, int w, int h) : Fl_Scroll(x, y, w, h) {
    box(FL_FLAT_BOX);
    color(fl_rgb_color(37, 37, 38)); // #252526 Darker gray
    
    // Scrollbar settings
    type(Fl_Scroll::VERTICAL_ALWAYS); // Always show vertical scrollbar
    scrollbar_size(8); // Thinner (8px)
    scrollbar.box(FL_FLAT_BOX);
    scrollbar.color(fl_rgb_color(37, 37, 38)); // Track matches background
    scrollbar.slider(FL_RFLAT_BOX); // Rounded flat box
    scrollbar.selection_color(fl_rgb_color(100, 100, 100)); // Thumb color
    // Hide arrows initially by matching color to track
    scrollbar.labelcolor(fl_rgb_color(37, 37, 38));
    
    Refresh();
    
    end();
    
    // Register callback
    core::QuickAccess::Get().SetUpdateCallback([this]() {
        // We need to run this on main thread
        Fl::awake([](void* data) {
            static_cast<Sidebar*>(data)->Refresh();
        }, this);
    });
}

Sidebar::SidebarButton::SidebarButton(int x, int y, int w, int h, const char* label)
    : Fl_Button(x, y, w, h, label) {
    box(FL_FLAT_BOX);
    color(fl_rgb_color(37, 37, 38)); // Match sidebar background
    selection_color(fl_rgb_color(0, 120, 212)); // #0078D4 Blue selection
    labelcolor(FL_WHITE);
    clear_visible_focus();
    align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
}

void Sidebar::SidebarButton::draw() {
    // Draw background
    if (value()) {
        fl_color(selection_color());
    } else {
        fl_color(color());
    }
    fl_rectf(x(), y(), w(), h());
    
    // Draw icon
    int text_x = x() + 5;
    if (icon) {
        // Center icon vertically
        int iy = y() + (h() - icon->h()) / 2;
        icon->draw(x() + 5, iy);
        text_x += icon->w() + 5;
    }
    
    // Draw text
    fl_color(labelcolor());
    fl_font(labelfont(), labelsize());
    fl_draw(label(), text_x, y(), w() - text_x - 20, h(), FL_ALIGN_LEFT);
    
    // Draw pin if pinned
    if (pinned) {
        fl_draw("📌", x() + w() - 25, y(), 20, h(), FL_ALIGN_RIGHT);
    }
}

int Sidebar::SidebarButton::handle(int event) {
    if (event == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE) {
        if (pinned) {
            ::Fl_Menu_Item menu[] = {
                { "Unpin from Quick Access", 0, 0, 0, 0 },
                { 0 }
            };
            const ::Fl_Menu_Item* m = menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
            if (m) {
                if (strcmp(m->label(), "Unpin from Quick Access") == 0) {
                    core::QuickAccess::Get().Unpin(path);
                }
            }
            return 1;
        }
    }
    return Fl_Button::handle(event);
}

void Sidebar::Refresh() {
    this->begin(); // Ensure buttons are added to this group

    // Clear existing buttons
    for (auto& item : items) {
        delete item.btn;
    }
    items.clear();
    
    // We do NOT clear icon_cache here. We keep it.
    
    int cur_y = y() + 10;
    
    // Quick Access
    auto items_list = core::QuickAccess::Get().GetItems(100);
    for (const auto& item : items_list) {
        std::string label = item.path;
        if (label.back() == '/' || label.back() == '\\') label.pop_back();
        size_t pos = label.find_last_of("/\\");
        if (pos != std::string::npos) label = label.substr(pos + 1);
        if (label.empty()) label = item.path;
        
        AddButton(label.c_str(), item.path, cur_y, item.pinned);
    }
    
    this->end(); // Stop adding to this group
    
    redraw();
    
    // Defer icon loading
    Fl::remove_timeout(LoadIconsCallback, this);
    Fl::add_timeout(0.1, LoadIconsCallback, this);
}

Sidebar::~Sidebar() {
    for (auto icon : icons) {
        delete icon;
    }
    icons.clear();
    core::QuickAccess::Get().SetUpdateCallback(nullptr);
}

void Sidebar::SetNavigateCallback(std::function<void(const std::string&)> cb) {
    on_navigate = cb;
}

void Sidebar::AddButton(const char* label, const std::string& path, int& cur_y, bool pinned) {
    if (path.empty()) return;
    
    SidebarButton* btn = new SidebarButton(x() + 10, cur_y, w() - 25, 30, label);
    btn->copy_label(label);
    btn->pinned = pinned;
    btn->path = path;
    
    // Check cache
    auto it = icon_cache.find(path);
    if (it != icon_cache.end()) {
        btn->icon = it->second;
    }
    
    std::string* path_ptr = new std::string(path);
    btn->callback([](Fl_Widget* w, void* data) {
        Sidebar* sidebar = (Sidebar*)w->parent();
        std::string* p = (std::string*)data;
        if (sidebar->on_navigate && p) {
            sidebar->on_navigate(*p);
        }
    }, path_ptr);
    
    items.push_back({btn, path});
    cur_y += 35;
}

void Sidebar::LoadIconsCallback(void* data) {
    Sidebar* sidebar = (Sidebar*)data;
    if (sidebar) {
        sidebar->LoadIcons();
    }
}

void Sidebar::LoadIcons() {
    for (auto& item : items) {
        if (item.btn->icon) continue; // Already has icon
        
        // Check cache again (maybe loaded by another item?)
        auto it = icon_cache.find(item.path);
        if (it != icon_cache.end()) {
            item.btn->icon = it->second;
            item.btn->redraw();
            continue;
        }
        
        Fl_RGB_Image* icon = IconManager::Get().GetSpecificIcon(item.path);
        if (icon) {
            item.btn->icon = icon;
            icons.push_back(icon); // Take ownership
            icon_cache[item.path] = icon; // Add to cache
            item.btn->redraw();
        }
    }
}

std::string Sidebar::GetKnownFolderPath(const void* rfid) {
    PWSTR path = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(*(KNOWNFOLDERID*)rfid, 0, NULL, &path))) {
        std::wstring ws(path);
        CoTaskMemFree(path);
        // Convert wstring to string (UTF-8)
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }
    return "";
}

int Sidebar::handle(int event) {
    if (event == FL_MOVE || event == FL_ENTER || event == FL_LEAVE) {
        int mx = Fl::event_x();
        int my = Fl::event_y();
        
        // Check if mouse is over scrollbar
        // Scrollbar is on the right side
        int sb_x = x() + w() - scrollbar.w();
        int sb_y = y();
        int sb_w = scrollbar.w();
        int sb_h = h();
        
        bool hover = (mx >= sb_x && mx < sb_x + sb_w && my >= sb_y && my < sb_y + sb_h);
        
        if (hover) {
            scrollbar.labelcolor(FL_WHITE); // Show arrows
        } else {
            scrollbar.labelcolor(fl_rgb_color(37, 37, 38)); // Hide arrows
        }
        scrollbar.redraw();
    }
    return Fl_Scroll::handle(event);
}

}
