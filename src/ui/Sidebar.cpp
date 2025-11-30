#include "Sidebar.h"
#include "IconManager.h"
#include <shlobj.h>
#include <windows.h>
#include <iostream>

namespace ui {

Sidebar::Sidebar(int x, int y, int w, int h) : Fl_Group(x, y, w, h) {
    box(FL_FLAT_BOX);
    color(FL_LIGHT3); // Slightly darker background
    
    int cur_y = y + 10;
    
    // Common Directories
    AddButton("Desktop", GetKnownFolderPath(&FOLDERID_Desktop), cur_y);
    AddButton("Documents", GetKnownFolderPath(&FOLDERID_Documents), cur_y);
    AddButton("Downloads", GetKnownFolderPath(&FOLDERID_Downloads), cur_y);
    AddButton("Music", GetKnownFolderPath(&FOLDERID_Music), cur_y);
    AddButton("Pictures", GetKnownFolderPath(&FOLDERID_Pictures), cur_y);
    AddButton("Videos", GetKnownFolderPath(&FOLDERID_Videos), cur_y);
    
    // Drives
    AddButton("Local Disk (C:)", "C:/", cur_y);
    
    end();
}

Sidebar::~Sidebar() {
    for (auto icon : icons) {
        delete icon;
    }
    icons.clear();
}

void Sidebar::SetNavigateCallback(std::function<void(const std::string&)> cb) {
    on_navigate = cb;
}

void Sidebar::AddButton(const char* label, const std::string& path, int& cur_y) {
    if (path.empty()) return;
    
    Fl_Button* btn = new Fl_Button(x() + 10, cur_y, w() - 20, 30, label);
    btn->copy_label(label);
    
    // Get Specific Icon for special folders
    Fl_RGB_Image* icon = IconManager::Get().GetSpecificIcon(path);
    if (icon) {
        btn->image(icon);
        icons.push_back(icon);
    }
    
    // Align icon left, text right of icon
    // FL_ALIGN_INSIDE puts everything inside the box
    // FL_ALIGN_LEFT puts text to the left (or image if image is there?)
    // FL_ALIGN_IMAGE_NEXT_TO_TEXT usually puts image left of text
    // Let's try standard left alignment with image
    btn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_IMAGE_NEXT_TO_TEXT);
    
    btn->box(FL_FLAT_BOX);
    btn->color(FL_LIGHT3);
    btn->selection_color(FL_SELECTION_COLOR);
    btn->clear_visible_focus();
    
    // Store path in user_data (need to copy string?)
    // Fl_Widget::user_data stores void*.
    // We can store a new std::string, but need to manage memory.
    // Or capture path in lambda.
    
    std::string* path_ptr = new std::string(path);
    btn->callback([](Fl_Widget* w, void* data) {
        Sidebar* sidebar = (Sidebar*)w->parent();
        std::string* p = (std::string*)data;
        if (sidebar->on_navigate && p) {
            sidebar->on_navigate(*p);
        }
    }, path_ptr);
    
    buttons.push_back(btn);
    cur_y += 35;
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

}
