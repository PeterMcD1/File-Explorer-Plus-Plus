#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <functional>
#include <vector>
#include <string>

namespace ui {

class TabButton : public Fl_Group {
public:
    TabButton(int x, int y, int w, int h, const char* label);
    
    void SetActive(bool active);
    void SetLabel(const char* label);
    void SetCloseCallback(std::function<void()> cb);
    void SetSelectCallback(std::function<void()> cb);
    
    int handle(int event) override;

private:
    Fl_Box* label_box;
    Fl_Button* close_btn;
    bool is_active = false;
    std::function<void()> on_select;
};

class TabBar : public Fl_Group {
public:
    TabBar(int x, int y, int w, int h);
    
    void AddTab(const char* label, void* data);
    void RemoveTab(void* data);
    void SelectTab(void* data);
    void UpdateTabLabel(void* data, const char* label);
    void UpdateLayout();
    
    std::function<void(void*)> on_tab_selected;
    std::function<void(void*)> on_tab_closed;
    std::function<void()> on_add_click;

private:
    struct TabItem {
        TabButton* btn;
        void* data; // Pointer to ExplorerTab
    };
    std::vector<TabItem> tabs;
    Fl_Button* add_btn;
};

}
