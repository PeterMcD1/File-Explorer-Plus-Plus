#include "FileTable.h"
#include "../core/AppState.h"
#include "../core/FileSystem.h"
#include "IconManager.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>

namespace ui {

FileTable::FileTable(int x, int y, int w, int h, const char* l, std::shared_ptr<core::TabContext> context) 
    : Fl_Table_Row(x, y, w, h, l), tab_context(context) {
    rows(0);
    cols(3); // Name, Size, Type
    
    col_header(1);
    col_resize(1);
    
    col_width(0, 400); // Name
    col_width(1, 100); // Size
    col_width(2, 80);  // Type (Dir/File)
    
    end();
}

void FileTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
    switch (context) {
    case CONTEXT_STARTPAGE:
        fl_font(FL_HELVETICA, 14);
        return;

    case CONTEXT_COL_HEADER:
        fl_push_clip(X, Y, W, H);
        fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_GRAY);
        fl_color(FL_BLACK);
        switch (C) {
        case 0: fl_draw("Name", X, Y, W, H, FL_ALIGN_CENTER); break;
        case 1: fl_draw("Size", X, Y, W, H, FL_ALIGN_CENTER); break;
        case 2: fl_draw("Type", X, Y, W, H, FL_ALIGN_CENTER); break;
        }
        fl_pop_clip();
        return;

    case CONTEXT_CELL:
        fl_push_clip(X, Y, W, H);
        
        // Background
        fl_color(FL_WHITE);
        fl_rectf(X, Y, W, H);

        // Data
        {
            std::lock_guard<std::mutex> lock(tab_context->mutex);
            
            if (R < tab_context->files.size()) {
                const auto& entry = tab_context->files[R];
                
                // Icon
                int text_x = X + 5;
                
                if (C == 0) {
                    Fl_RGB_Image* icon = IconManager::Get().GetIcon(entry.path, entry.is_dir);
                    if (icon) {
                        // Center icon vertically
                        int icon_y = Y + (H - 16) / 2;
                        icon->draw(X + 2, icon_y);
                        text_x += 20; // Space for icon
                    } else {
                        // Fallback
                        if (entry.is_dir) {
                            fl_color(FL_YELLOW);
                            fl_rectf(X + 2, Y + 2, 10, H - 4);
                        } else {
                            fl_color(FL_BLUE);
                            fl_rectf(X + 4, Y + 4, 6, H - 8);
                        }
                        text_x += 15;
                    }
                    
                    fl_color(FL_BLACK);
                    fl_draw(entry.name.c_str(), text_x, Y, W - (text_x - X), H, FL_ALIGN_LEFT);
                }
                else if (C == 1) {
                    fl_color(FL_BLACK);
                    fl_draw(entry.size_str.c_str(), X, Y, W, H, FL_ALIGN_LEFT);
                }
                else if (C == 2) {
                    fl_color(FL_BLACK);
                    fl_draw(entry.is_dir ? "DIR" : "FILE", X, Y, W, H, FL_ALIGN_LEFT);
                }
            }
        }
        
        fl_color(FL_LIGHT2);
        fl_rect(X, Y, W, H);
        
        fl_pop_clip();
        return;

    default:
        return;
    }
}

int FileTable::handle(int event) {
    if (event == FL_PUSH && Fl::event_clicks()) {
        int r = callback_row();
        if (r >= 0) {
            std::string path;
            bool is_dir = false;
            {
                std::lock_guard<std::mutex> lock(tab_context->mutex);
                if (r < tab_context->files.size()) {
                    path = tab_context->files[r].path;
                    is_dir = tab_context->files[r].is_dir;
                }
            }
            
            if (is_dir && !path.empty()) {
                core::StartLoading(path, tab_context);
                return 1;
            }
        }
    }
    return Fl_Table_Row::handle(event);
}

}
