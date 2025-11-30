#pragma once
#include <FL/Fl_Table_Row.H>

namespace ui {

class FileTable : public Fl_Table_Row {
public:
    FileTable(int x, int y, int w, int h, const char* l = 0);
    
    void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) override;
    int handle(int event) override;
};

}
