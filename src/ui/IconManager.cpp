#include "IconManager.h"
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <algorithm>

// Ensure NOMINMAX is defined if not already (usually in CMake, but good safety)
#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace ui {

IconManager& IconManager::Get() {
    static IconManager instance;
    return instance;
}

IconManager::~IconManager() {
    for (auto& pair : icon_cache_) {
        delete pair.second;
    }
}

Fl_RGB_Image* IconManager::GetIcon(const std::string& path, bool is_dir) {
    std::string key;
    if (is_dir) {
        key = "DIR";
    } else {
        size_t dot_pos = path.find_last_of('.');
        if (dot_pos != std::string::npos) {
            key = path.substr(dot_pos);
            // Normalize extension to lower case
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        } else {
            key = "NONE";
        }
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);
    if (icon_cache_.find(key) != icon_cache_.end()) {
        return icon_cache_[key];
    }

    // Default to small icons for GetIcon
    Fl_RGB_Image* img = LoadIconFromSystem(path, is_dir, false, false);
    if (img) {
        icon_cache_[key] = img;
    }
    return img;
}

Fl_RGB_Image* IconManager::GetSpecificIcon(const std::string& path, bool large) {
    return LoadIconFromSystem(path, false, true, large);
}

// Helper to convert HICON to Fl_RGB_Image
Fl_RGB_Image* HIconToFlImage(HICON hIcon) {
    if (!hIcon) return nullptr;

    ICONINFO iconInfo;
    GetIconInfo(hIcon, &iconInfo);
    
    // Get dimensions
    BITMAP bmp;
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);
    int width = bmp.bmWidth;
    int height = bmp.bmHeight; // Could be negative if top-down? No, GetObject returns abs usually for BITMAP struct?
    // Actually BITMAP struct has bmHeight > 0.
    
    HDC hDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hDC);
    
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    
    HGDIOBJ oldObj = SelectObject(hMemDC, hBitmap);
    
    // Clear background (transparent)
    DrawIconEx(hMemDC, 0, 0, hIcon, width, height, 0, NULL, DI_NORMAL);

    // Now we have the bits in BGRA format (Windows default)
    // FLTK wants RGBA or RGB.
    // We need to copy and swap channels.
    
    std::vector<unsigned char> rgba_data(width * height * 4);
    unsigned char* src = static_cast<unsigned char*>(bits);
    
    for (int i = 0; i < width * height; ++i) {
        unsigned char b = src[i * 4 + 0];
        unsigned char g = src[i * 4 + 1];
        unsigned char r = src[i * 4 + 2];
        unsigned char a = src[i * 4 + 3];
        
        rgba_data[i * 4 + 0] = r;
        rgba_data[i * 4 + 1] = g;
        rgba_data[i * 4 + 2] = b;
        rgba_data[i * 4 + 3] = a;
    }

    SelectObject(hMemDC, oldObj);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);

    // Create FLTK image (depth 4 for RGBA)
    Fl_RGB_Image* temp = new Fl_RGB_Image(rgba_data.data(), width, height, 4);
    Fl_RGB_Image* final_img = (Fl_RGB_Image*)temp->copy(width, height);
    delete temp;
    
    return final_img;
}

Fl_RGB_Image* IconManager::LoadIconFromSystem(const std::string& path, bool is_dir, bool specific, bool large) {
    SHFILEINFOA sfi = {0};
    UINT flags = SHGFI_ICON;
    if (large) flags |= SHGFI_LARGEICON;
    else flags |= SHGFI_SMALLICON;
    
    std::string lookup_path = path;
    std::replace(lookup_path.begin(), lookup_path.end(), '/', '\\');

    if (!specific) {
        // Generic lookup: Use attributes and dummy path
        flags |= SHGFI_USEFILEATTRIBUTES;
        DWORD attributes = is_dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        
        if (is_dir) {
            lookup_path = "directory";
        } else {
            size_t dot_pos = path.find_last_of('.');
            if (dot_pos != std::string::npos) {
                lookup_path = "dummy" + path.substr(dot_pos);
            } else {
                lookup_path = "dummy.dat"; 
            }
        }
        
        if (SHGetFileInfoA(lookup_path.c_str(), attributes, &sfi, sizeof(sfi), flags)) {
            Fl_RGB_Image* img = HIconToFlImage(sfi.hIcon);
            DestroyIcon(sfi.hIcon);
            return img;
        }
    } else {
        // Specific lookup: Use real path, access file
        if (SHGetFileInfoA(lookup_path.c_str(), 0, &sfi, sizeof(sfi), flags)) {
            Fl_RGB_Image* img = HIconToFlImage(sfi.hIcon);
            DestroyIcon(sfi.hIcon);
            return img;
        }
    }

    return nullptr;
}
}
