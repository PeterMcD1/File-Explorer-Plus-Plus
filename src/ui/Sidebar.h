#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_RGB_Image.H>
#include <vector>
#include <string>
#include <functional>

namespace ui {

class Sidebar : public Fl_Group {
public:
    Sidebar(int x, int y, int w, int h);
    ~Sidebar();
    
    void SetNavigateCallback(std::function<void(const std::string&)> cb);

private:
    void AddButton(const char* label, const std::string& path, int& cur_y);
    std::string GetKnownFolderPath(const void* rfid); // rfid is KNOWNFOLDERID guid

    std::function<void(const std::string&)> on_navigate;
    std::vector<Fl_Button*> buttons;
    std::vector<Fl_RGB_Image*> icons;
};

}
