#include "utils.h"

#include <stdexcept>
#include <iostream>
#include <assert.h>
#include <memory>
#include <queue>
#include <filesystem>

namespace fs = std::filesystem;

class KawaiiTrayWnd : public WndHelper {
public:
    enum Action : UINT_PTR {
        Quit        = 1,
        SelectTheme = 0xfeef0000,
    };

    constexpr static const char* PathToThemes{"./assets/themes"};
    constexpr static const char* CLSID_KAWAIITRAY{"KawaiiTrayIconWnd"};
    constexpr static UINT        WM_MENUNOTIFY = WM_USER;
    constexpr static UINT        TID_RENDER    = 0x6006f00d;

    KawaiiTrayWnd()
        : WndHelper(CLSID_KAWAIITRAY)
        , menu_{nullptr}
        , nid_{}
        , currentThemeId_{-1} {
        nid_.cbSize           = sizeof(NOTIFYICONDATA);
        nid_.hWnd             = handle();
        nid_.uID              = 0;
        nid_.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid_.uCallbackMessage = WM_MENUNOTIFY;
        strcpy_s(nid_.szTip, "Kawaii Tray");
        Shell_NotifyIcon(NIM_ADD, &nid_);

        int themeId = 1;
        themeMenu_  = CreatePopupMenu();
        for (const auto& e : fs::directory_iterator(PathToThemes)) {
            if (!e.is_directory()) { continue; }
            themes_.push_back(e.path().filename().string());
        }
        while (themeId <= themes_.size()) {
            AppendMenu(
                themeMenu_,
                MF_STRING | MF_UNCHECKED,
                Action::SelectTheme | themeId,
                themes_[themeId - 1].c_str());
            ++themeId;
        }

        menu_ = CreatePopupMenu();
        AppendMenu(
            menu_,
            MF_POPUP | MF_STRING,
            reinterpret_cast<UINT_PTR>(themeMenu_),
            "Theme");
        AppendMenu(menu_, MF_STRING, Action::Quit, "Quit");

        SetTimer(handle(), TID_RENDER, 10, nullptr);
    }

    bool setTheme(int themeId) {
        const auto& target = themes_[themeId - 1].c_str();
        if (currentThemeId_ != -1) {
            CheckMenuItem(
                themeMenu_, currentThemeId_ - 1, MF_BYPOSITION | MF_UNCHECKED);
        }
        CheckMenuItem(themeMenu_, themeId - 1, MF_BYPOSITION | MF_CHECKED);
        currentThemeId_ = themeId;

        const auto path = fs::path(PathToThemes).append(target);
        if (!fs::exists(path)) { return false; }
        resetAnimeSet();
        for (const auto& e : fs::directory_iterator(path)) {
            if (!e.is_regular_file()) { continue; }
            auto handle = LoadImage(
                nullptr,
                e.path().relative_path().string().c_str(),
                IMAGE_ICON,
                0,
                0,
                LR_LOADFROMFILE | LR_DEFAULTSIZE);
            iconAnimeSet_.push(static_cast<HICON>(handle));
        }
        return true;
    }

    ~KawaiiTrayWnd() {
        DestroyMenu(menu_);
        DestroyMenu(themeMenu_);

        KillTimer(handle(), TID_RENDER);
    }

    bool notify(UINT msg, WPARAM wParam, LPARAM lParam) override {
        if (msg == WM_MENUNOTIFY) {
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP: {
                    POINT pt{};
                    GetCursorPos(&pt);
                    SetForegroundWindow(handle());
                    const auto id = TrackPopupMenu(
                        menu_, TPM_RETURNCMD, pt.x, pt.y, 0, handle(), nullptr);
                    if (id != 0) { handleAction(static_cast<Action>(id)); }
                } break;
            }
            return true;
        }

        if (msg == WM_TIMER && wParam == TID_RENDER && !iconAnimeSet_.empty()) {
            const auto e = iconAnimeSet_.front();
            iconAnimeSet_.pop();
            iconAnimeSet_.push(e);
            nid_.hIcon = e;
            Shell_NotifyIconA(NIM_MODIFY, &nid_);
            return true;
        }

        return false;
    }

    void handleAction(Action action) {
        switch (action) {
            case Action::Quit: {
                Shell_NotifyIcon(NIM_DELETE, &nid_);
                PostQuitMessage(0);
            } break;
            default: {
                if ((action & 0xffff0000) == Action::SelectTheme) {
                    const auto themeId = action & 0xffff;
                    setTheme(themeId);
                }
            } break;
        }
    }

protected:
    void resetAnimeSet() {
        while (!iconAnimeSet_.empty()) {
            DestroyIcon(iconAnimeSet_.front());
            iconAnimeSet_.pop();
        }
    }

private:
    HMENU                    menu_;
    HMENU                    themeMenu_;
    std::vector<std::string> themes_;
    int                      currentThemeId_;
    NOTIFYICONDATA           nid_;
    std::queue<HICON>        iconAnimeSet_;
};

int main(int argc, char* argv[]) {
    if (FindWindowA(nullptr, KawaiiTrayWnd::CLSID_KAWAIITRAY) != nullptr) {
        return -1;
    }

    auto app = std::make_unique<KawaiiTrayWnd>();
    app->setTheme(1);
    return app->exec();
}
