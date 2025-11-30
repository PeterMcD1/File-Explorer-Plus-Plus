#pragma once
#include <FL/Fl_Table_Row.H>
#include <memory>
#include "../core/TabContext.h"
#include <string>

namespace ui {

class FileTable : public Fl_Table_Row {
public:
    FileTable(int x, int y, int w, int h, const char* l, std::shared_ptr<core::TabContext> context);
    
    int handle(int event) override;

private:
    void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) override;
    
    void ShowContextMenu(const std::string& path, bool is_dir);
    void CopyPathToClipboard(const std::string& path);
    void ShowProperties(const std::string& path);

    std::shared_ptr<core::TabContext> tab_context;
    
public:
    std::function<void(const std::string&)> on_navigate;
};

}
