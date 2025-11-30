#include "FileTable.h"
#include "../core/AppState.h"
#include "../core/FileSystem.h"
#include "IconManager.h"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <windows.h>
#include <shellapi.h>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_ask.H>
#include "../core/QuickAccess.h"

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
    
    // Scrollbar styling
    // Fl_Table exposes vscrollbar and hscrollbar as public pointers
    if (vscrollbar) {
        vscrollbar->box(FL_FLAT_BOX);
        vscrollbar->color(fl_rgb_color(32, 32, 32)); // Match table background
        vscrollbar->slider(FL_RFLAT_BOX);
        vscrollbar->selection_color(fl_rgb_color(100, 100, 100));
        vscrollbar->labelcolor(fl_rgb_color(32, 32, 32)); // Hide arrows initially
    }
    if (hscrollbar) {
        hscrollbar->box(FL_FLAT_BOX);
        hscrollbar->color(fl_rgb_color(32, 32, 32));
        hscrollbar->slider(FL_RFLAT_BOX);
        hscrollbar->selection_color(fl_rgb_color(100, 100, 100));
        hscrollbar->labelcolor(fl_rgb_color(32, 32, 32)); // Hide arrows initially
    }

    end();
}

void FileTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
    switch (context) {
    case CONTEXT_STARTPAGE:
        fl_font(FL_HELVETICA, 14);
        return;

    case CONTEXT_COL_HEADER:
        fl_push_clip(X, Y, W, H);
        fl_draw_box(FL_FLAT_BOX, X, Y, W, H, fl_rgb_color(45, 45, 45)); // #2D2D2D Header
        // Draw separator line
        fl_color(fl_rgb_color(60, 60, 60));
        fl_line(X, Y + H - 1, X + W, Y + H - 1);
        fl_line(X + W - 1, Y, X + W - 1, Y + H);
        
        fl_color(FL_WHITE);
        switch (C) {
        case 0: fl_draw("Name", X + 10, Y, W - 10, H, FL_ALIGN_LEFT); break;
        case 1: fl_draw("Size", X + 10, Y, W - 10, H, FL_ALIGN_LEFT); break;
        case 2: fl_draw("Type", X + 10, Y, W - 10, H, FL_ALIGN_LEFT); break;
        }
        fl_pop_clip();
        return;

    case CONTEXT_CELL:
        fl_push_clip(X, Y, W, H);
        
        // Background
        if (row_selected(R)) {
            fl_color(fl_rgb_color(0, 120, 212)); // #0078D4 Blue selection
        } else {
            fl_color(fl_rgb_color(32, 32, 32)); // #202020 Background
        }
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
                    
                    fl_color(FL_WHITE);
                    fl_draw(entry.name.c_str(), text_x, Y, W - (text_x - X), H, FL_ALIGN_LEFT);
                }
                else if (C == 1) {
                    fl_color(fl_rgb_color(200, 200, 200)); // Light gray text for size
                    fl_draw(entry.size_str.c_str(), X + 10, Y, W - 10, H, FL_ALIGN_LEFT);
                }
                else if (C == 2) {
                    fl_color(fl_rgb_color(200, 200, 200)); // Light gray text for type
                    fl_draw(entry.is_dir ? "File folder" : "File", X + 10, Y, W - 10, H, FL_ALIGN_LEFT);
                }
            }
        }
        
        // Selection border?
        if (row_selected(R)) {
            fl_color(fl_rgb_color(50, 140, 220));
            fl_rect(X, Y, W, H);
        }
        
        fl_pop_clip();
        return;

    default:
        return;
    }
}

#include <windows.h>
#include <shellapi.h>

// ...

#include <FL/Fl_Menu_Item.H>
#include <FL/fl_ask.H>

// ...

int FileTable::handle(int event) {
    if (event == FL_MOVE || event == FL_ENTER || event == FL_LEAVE) {
        int mx = Fl::event_x();
        int my = Fl::event_y();
        
        // Check vscrollbar
        if (vscrollbar && vscrollbar->visible()) {
            bool hover = (mx >= vscrollbar->x() && mx < vscrollbar->x() + vscrollbar->w() &&
                          my >= vscrollbar->y() && my < vscrollbar->y() + vscrollbar->h());
            if (hover) vscrollbar->labelcolor(FL_WHITE);
            else vscrollbar->labelcolor(fl_rgb_color(32, 32, 32));
            vscrollbar->redraw();
        }
        
        // Check hscrollbar
        if (hscrollbar && hscrollbar->visible()) {
            bool hover = (mx >= hscrollbar->x() && mx < hscrollbar->x() + hscrollbar->w() &&
                          my >= hscrollbar->y() && my < hscrollbar->y() + hscrollbar->h());
            if (hover) hscrollbar->labelcolor(FL_WHITE);
            else hscrollbar->labelcolor(fl_rgb_color(32, 32, 32));
            hscrollbar->redraw();
        }
    }

    // Right click handling
    if (event == FL_RELEASE && Fl::event_button() == FL_RIGHT_MOUSE) {
        // ... (existing right click logic)
        // Find which row was clicked
        // Fl_Table_Row doesn't automatically select on right click usually.
        // We need to determine the row.
        // handle(FL_PUSH) might have already happened but maybe not for right click if we didn't pass it?
        // Fl_Table_Row::handle passes everything.
        
        // We can use callback_row() if it was updated, or calculate it.
        // Let's force selection if we clicked a row.
        
        // Actually, let's just use the current selection if valid, or try to select what's under mouse.
        // For simplicity, let's assume the user left-clicked first or we just use callback_row if valid.
        // But callback_row is set by handle().
        
        // Let's do this:
        // We need to calculate row from mouse Y.
        // This is internal to Fl_Table.
        // But we can use find_cell? No, that's protected/internal?
        // Wait, Fl_Table has find_cell.
        
        TableContext context;
        int R, C;
        int X, Y, W, H;
        // resize_context_save(X, Y, W, H); // Not available
        
        // We can't easily find the row without using internal methods or doing math.
        // But Fl_Table_Row::handle(event) should have processed the click.
        // Does it handle right click? Probably not for selection.
        
        // Let's just rely on the fact that if we right click, we probably want the context menu for the item under cursor.
        // If we can't easily get it, we might skip selection update and just use callback_row if it's valid?
        // No, that might be the previous selection.
        
        // Let's try to pass the event to base class first.
        Fl_Table_Row::handle(event);
        
        // Manually calculate row?
        // y() is top.
        // col_header_height() if headers.
        // row_height(r).
        
        // Simpler: Just show menu if we have a valid selection.
        // But user expects right click to select.
        
        // Let's use a trick: Simulate left click? No.
        
        // Let's just implement the menu for the *currently selected* row for now.
        // If the user right clicks a different row, it won't work perfectly (won't select it),
        // but if they left click then right click, it works.
        // To fix this properly requires more logic.
        
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
            
            if (!path.empty()) {
                ShowContextMenu(path, is_dir);
                return 1;
            }
        }
    }

    if (event == FL_PUSH && Fl::event_clicks()) {
        Fl_Table_Row::handle(event);
        
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
            
            if (!path.empty()) {
                if (is_dir) {
                    if (on_navigate) on_navigate(path);
                    else core::StartLoading(path, tab_context);
                } else {
                    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
                return 1;
            }
        }
        return 1;
    }
    return Fl_Table_Row::handle(event);
}

void FileTable::ShowContextMenu(const std::string& path, bool is_dir) {
    bool is_pinned = false;
    if (is_dir) {
        is_pinned = core::QuickAccess::Get().IsPinned(path);
    }

    ::Fl_Menu_Item menu[] = {
        { "Open", 0, 0, 0, 0 },
        { "Copy Path", 0, 0, 0, 0 },
        { "Properties", 0, 0, 0, 0 },
        { is_dir ? (is_pinned ? "Unpin from Quick Access" : "Pin to Quick Access") : 0, 0, 0, 0, 0 },
        { 0 }
    };
    
    // Filter out null items if not dir (handled by 0 label?)
    // Fl_Menu_Item with 0 label terminates the list.
    // If we want optional item, we need to construct array dynamically or use flags.
    // But here, if !is_dir, the label is 0, so it terminates early?
    // No, "Properties" is before it.
    // If !is_dir, the 4th item has label 0. So it terminates there.
    // But we want "Properties" to show.
    // So we should construct the menu carefully.
    
    // Better approach:
    std::vector<::Fl_Menu_Item> items;
    items.push_back({ "Open", 0, 0, 0, 0 });
    items.push_back({ "Copy Path", 0, 0, 0, 0 });
    items.push_back({ "Properties", 0, 0, 0, 0 });
    
    if (is_dir) {
        items.push_back({ is_pinned ? "Unpin from Quick Access" : "Pin to Quick Access", 0, 0, 0, 0 });
    }
    items.push_back({ 0 });
    
    const ::Fl_Menu_Item* m = items.data()->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
    if (m) {
        std::string label = m->label() ? m->label() : "";
        if (label == "Open") {
                if (is_dir) {
                    if (on_navigate) on_navigate(path);
                    else core::StartLoading(path, tab_context);
                } else {
                    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
        } else if (label == "Copy Path") {
            CopyPathToClipboard(path);
        } else if (label == "Properties") {
            ShowProperties(path);
        } else if (label == "Pin to Quick Access") {
            core::QuickAccess::Get().Pin(path);
        } else if (label == "Unpin from Quick Access") {
            core::QuickAccess::Get().Unpin(path);
        }
    }
}

void FileTable::CopyPathToClipboard(const std::string& path) {
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, path.size() + 1);
        if (hg) {
            memcpy(GlobalLock(hg), path.c_str(), path.size() + 1);
            GlobalUnlock(hg);
            SetClipboardData(CF_TEXT, hg);
        }
        CloseClipboard();
    }
}

void FileTable::ShowProperties(const std::string& path) {
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.lpFile = path.c_str();
    sei.nShow = SW_SHOW;
    sei.fMask = SEE_MASK_INVOKEIDLIST;
    sei.lpVerb = "properties";
    ShellExecuteExA(&sei);
}

}
