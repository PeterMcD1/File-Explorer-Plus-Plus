#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>

#include <filesystem>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// --- Data Structures ---

struct FileEntry {
    std::string name;
    std::string size_str;
    bool is_dir;
    std::string path;
};

// Global State
std::vector<FileEntry> g_files;
std::mutex g_files_mutex;
std::atomic<bool> g_loading{false};
std::string g_current_path;

// UI Components (Global pointers for easy access in callbacks)
class FileTable;
Fl_Input* g_address_bar = nullptr;
FileTable* g_file_table = nullptr;
Fl_Double_Window* g_window = nullptr;

// --- Helper Functions ---

std::string FormatSize(uintmax_t size) {
    if (size < 1024) return std::to_string(size) + " B";
    if (size < 1024 * 1024) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (size / 1024.0) << " KB";
        return ss.str();
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << " MB";
    return ss.str();
}

// --- Worker Thread Logic ---

// Callback to update UI from main thread
void UpdateUICallback(void*) {
    std::lock_guard<std::mutex> lock(g_files_mutex);
    if (g_file_table) {
        g_file_table->rows((int)g_files.size());
        g_file_table->redraw();
    }
}

void LoadDirectoryWorker(std::string path) {
    g_loading = true;
    
    // Clear existing data
    {
        std::lock_guard<std::mutex> lock(g_files_mutex);
        g_files.clear();
        g_current_path = path;
    }
    Fl::awake(UpdateUICallback, nullptr);

    std::vector<FileEntry> batch;
    batch.reserve(1000);

    std::error_code ec;
    // Use directory_iterator for non-recursive listing
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) {
            // Handle permission denied or other errors gracefully
            std::cerr << "Error accessing " << path << ": " << ec.message() << std::endl;
            break; 
        }

        FileEntry fe;
        fe.path = entry.path().string();
        fe.name = entry.path().filename().string();
        
        // Check status safely
        std::error_code status_ec;
        bool is_directory = entry.is_directory(status_ec);
        fe.is_dir = !status_ec && is_directory;

        if (fe.is_dir) {
            fe.size_str = "<DIR>";
        } else {
            uintmax_t size = entry.file_size(status_ec);
            fe.size_str = status_ec ? "Unknown" : FormatSize(size);
        }

        batch.push_back(fe);

        // Update UI every 1000 items to avoid locking too often but keep it responsive
        if (batch.size() >= 1000) {
            {
                std::lock_guard<std::mutex> lock(g_files_mutex);
                g_files.insert(g_files.end(), batch.begin(), batch.end());
            }
            batch.clear();
            Fl::awake(UpdateUICallback, nullptr);
        }
    }

    // Add remaining items
    if (!batch.empty()) {
        std::lock_guard<std::mutex> lock(g_files_mutex);
        g_files.insert(g_files.end(), batch.begin(), batch.end());
    }

    g_loading = false;
    Fl::awake(UpdateUICallback, nullptr);
}

void StartLoading(const std::string& path) {
    if (g_loading) return; // Simple protection, could be improved to cancel previous
    if (g_address_bar) g_address_bar->value(path.c_str());
    std::thread(LoadDirectoryWorker, path).detach();
}

// --- UI Implementation ---

class FileTable : public Fl_Table_Row {
public:
    FileTable(int x, int y, int w, int h, const char* l = 0) : Fl_Table_Row(x, y, w, h, l) {
        rows(0);
        cols(3); // Name, Size, Type
        
        col_header(1);
        col_resize(1);
        
        col_width(0, 400); // Name
        col_width(1, 100); // Size
        col_width(2, 80);  // Type (Dir/File)
        
        end();
    }

    void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) override {
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
                // Lock mutex briefly to copy data for this row? 
                // For strict O(1) rendering without blocking UI, we assume vector access is safe 
                // if we only resize in main thread (which we do via UpdateUICallback).
                // However, the data content (strings) might be read while being written if we are not careful.
                // But we only append to vector in worker. 
                // To be 100% safe, we should lock, but locking in draw_cell is bad for perf.
                // Since we only append, and 'rows()' is updated in main thread, 
                // accessing indices < rows() should be safe-ish if vector doesn't reallocate.
                // Vector reallocation invalidates iterators/pointers.
                // To avoid crash, we MUST lock. 
                // To keep it fast, we hope the lock contention is low (worker only locks every 1000 items).
                
                std::lock_guard<std::mutex> lock(g_files_mutex);
                
                if (R < g_files.size()) {
                    const auto& entry = g_files[R];
                    
                    // Icon/Color
                    if (entry.is_dir) {
                        fl_color(FL_YELLOW);
                        fl_rectf(X + 2, Y + 2, 10, H - 4); // Simple folder icon
                        fl_color(FL_BLACK);
                    } else {
                        fl_color(FL_BLUE); // Simple file icon
                        fl_rectf(X + 4, Y + 4, 6, H - 8);
                        fl_color(FL_BLACK);
                    }

                    // Text
                    int text_x = X + 15;
                    int text_w = W - 15;
                    
                    if (C == 0) fl_draw(entry.name.c_str(), text_x, Y, text_w, H, FL_ALIGN_LEFT);
                    else if (C == 1) fl_draw(entry.size_str.c_str(), X, Y, W, H, FL_ALIGN_LEFT);
                    else if (C == 2) fl_draw(entry.is_dir ? "DIR" : "FILE", X, Y, W, H, FL_ALIGN_LEFT);
                }
            }
            
            // Border
            fl_color(FL_LIGHT2);
            fl_rect(X, Y, W, H);
            
            fl_pop_clip();
            return;

        default:
            return;
        }
    }

    int handle(int event) override {
        if (event == FL_PUSH && Fl::event_clicks()) { // Double click
            int r = callback_row();
            if (r >= 0) {
                std::string new_path;
                {
                    std::lock_guard<std::mutex> lock(g_files_mutex);
                    if (r < g_files.size() && g_files[r].is_dir) {
                        new_path = g_files[r].path;
                    }
                }
                if (!new_path.empty()) {
                    StartLoading(new_path);
                }
            }
            return 1;
        }
        return Fl_Table_Row::handle(event);
    }
};

void AddressCallback(Fl_Widget* w, void*) {
    Fl_Input* input = (Fl_Input*)w;
    StartLoading(input->value());
}

int main(int argc, char** argv) {
    // Enable multi-thread support in FLTK
    Fl::lock();

    g_window = new Fl_Double_Window(800, 600, "Flash Explorer");
    
    g_address_bar = new Fl_Input(10, 10, 700, 30);
    g_address_bar->callback(AddressCallback);
    g_address_bar->when(FL_WHEN_ENTER_KEY);
    
    Fl_Button* go_btn = new Fl_Button(720, 10, 70, 30, "Go");
    go_btn->callback(AddressCallback, g_address_bar);

    g_file_table = new FileTable(10, 50, 780, 540);

    g_window->resizable(g_file_table);
    g_window->end();
    g_window->show(argc, argv);

    // Initial load
    StartLoading("C:/");

    return Fl::run();
}
