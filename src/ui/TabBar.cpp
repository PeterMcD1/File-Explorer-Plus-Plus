#include "TabBar.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>

namespace ui {

// --- TabButton ---

TabButton::TabButton(int x, int y, int w, int h, const char* label) 
    : Fl_Group(x, y, w, h) {
    
    begin(); // Add children to this group
    
    box(FL_FLAT_BOX); // Flat box for tabs
    color(fl_rgb_color(45, 45, 45)); // Default inactive color (matches title bar)
    
    // Close Button (Right aligned)
    int close_size = 16;
    int pad = 4;
    close_btn = new Fl_Button(x + w - close_size - pad, y + (h - close_size) / 2, close_size, close_size, "x"); // Small cross
    close_btn->box(FL_FLAT_BOX);
    close_btn->color(fl_rgb_color(45, 45, 45)); // Match background
    close_btn->labelcolor(fl_rgb_color(180, 180, 180));
    close_btn->clear_visible_focus();
    close_btn->tooltip("Close Tab");
    
    // Icon Box (Left aligned)
    int icon_size = 16;
    icon_box = new Fl_Box(x + pad, y + (h - icon_size) / 2, icon_size, icon_size);
    icon_box->box(FL_NO_BOX);
    
    // Label (Left aligned, takes remaining space)
    label_box = new Fl_Box(x + pad + icon_size + pad, y, w - close_size - pad * 4 - icon_size, h, label);
    label_box->copy_label(label);
    label_box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    label_box->labelsize(12);
    label_box->labelcolor(fl_rgb_color(180, 180, 180)); // Inactive text color
    
    end();
}

void TabButton::SetActive(bool active) {
    is_active = active;
    if (active) {
        color(fl_rgb_color(56, 56, 56)); // Active color (matches nav area)
        label_box->labelfont(FL_BOLD);
        label_box->labelcolor(FL_WHITE);
        close_btn->color(fl_rgb_color(56, 56, 56));
        close_btn->labelcolor(FL_WHITE);
    } else {
        color(fl_rgb_color(45, 45, 45)); // Inactive color
        label_box->labelfont(FL_HELVETICA);
        label_box->labelcolor(fl_rgb_color(180, 180, 180));
        close_btn->color(fl_rgb_color(45, 45, 45));
        close_btn->labelcolor(fl_rgb_color(180, 180, 180));
    }
    redraw();
}

void TabButton::SetLabel(const char* label) {
    label_box->copy_label(label);
    label_box->redraw();
}

void TabButton::SetIcon(Fl_RGB_Image* icon) {
    if (icon) {
        // Scale if needed? For now assume 16x16 or let Fl_Box handle it (it doesn't scale auto)
        // Ideally we should scale to 16x16
        icon_box->image(icon);
    } else {
        icon_box->image(nullptr);
    }
    icon_box->redraw();
}

void TabButton::SetCloseCallback(std::function<void()> cb) {
    close_btn->callback([](Fl_Widget*, void* data) {
        auto func = (std::function<void()>*)data;
        if (*func) (*func)();
    }, new std::function<void()>(cb)); // Leak? Should manage memory better, but for now ok.
}

void TabButton::SetSelectCallback(std::function<void()> cb) {
    on_select = cb;
}

int TabButton::handle(int event) {
    // Give children (close button) a chance to handle the event first
    if (Fl_Group::handle(event)) return 1;
    
    if (event == FL_PUSH) {
        if (Fl::event_button() == FL_LEFT_MOUSE) {
            if (on_select) on_select();
            return 1;
        }
    }
    return 0;
}

// --- TabBar ---

TabBar::TabBar(int x, int y, int w, int h) 
    : Fl_Group(x, y, w, h) {
    box(FL_FLAT_BOX);
    color(fl_rgb_color(45, 45, 45)); // Background for tab strip (matches title bar)
    
    add_btn = new Fl_Button(x, y + 2, 24, h - 4, "+");
    add_btn->box(FL_FLAT_BOX);
    add_btn->color(fl_rgb_color(45, 45, 45));
    add_btn->labelcolor(FL_WHITE);
    add_btn->clear_visible_focus();
    add_btn->callback([](Fl_Widget*, void* data) {
        auto func = (std::function<void()>*)data;
        if (*func) (*func)();
    }, &on_add_click);
    
    end();
}

void TabBar::AddTab(const char* label, void* data) {
    begin();
    int tab_w = 150; // Fixed width for now
    int tab_h = h() - 2; // Slightly smaller than bar
    
    TabButton* btn = new TabButton(x(), y() + 2, tab_w, tab_h, label);
    
    btn->SetSelectCallback([this, data]() {
        if (on_tab_selected) on_tab_selected(data);
    });
    
    btn->SetCloseCallback([this, data]() {
        if (on_tab_closed) on_tab_closed(data);
    });
    
    tabs.push_back({btn, data});
    end();
    
    UpdateLayout();
}

void TabBar::RemoveTab(void* data) {
    for (auto it = tabs.begin(); it != tabs.end(); ++it) {
        if (it->data == data) {
            remove(it->btn);
            delete it->btn;
            tabs.erase(it);
            break;
        }
    }
    UpdateLayout();
}

void TabBar::SelectTab(void* data) {
    for (auto& tab : tabs) {
        tab.btn->SetActive(tab.data == data);
    }
}

void TabBar::UpdateTabLabel(void* data, const char* label) {
    for (auto& tab : tabs) {
        if (tab.data == data) {
            tab.btn->SetLabel(label);
            break;
        }
    }
}

void TabBar::UpdateTabIcon(void* data, Fl_RGB_Image* icon) {
    for (auto& tab : tabs) {
        if (tab.data == data) {
            tab.btn->SetIcon(icon);
            break;
        }
    }
}

void TabBar::UpdateLayout() {
    int cur_x = x() + 2;
    int tab_w = 150;
    
    for (auto& tab : tabs) {
        tab.btn->position(cur_x, y() + 2);
        cur_x += tab_w + 2;
    }
    
    add_btn->position(cur_x, y() + 2);
    redraw();
}

}
