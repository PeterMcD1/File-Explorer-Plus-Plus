#pragma once
#include <FL/Fl_Table_Row.H>
#include <memory>
#include "../core/TabContext.h"

namespace ui {

class FileTable : public Fl_Table_Row {
public:
    FileTable(int x, int y, int w, int h, const char* l, std::shared_ptr<core::TabContext> context);
    
    void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) override;
    int handle(int event) override;

private:
    std::shared_ptr<core::TabContext> tab_context;
};

}
