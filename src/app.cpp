#include "app.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <assert.h>

constexpr static UINT WM_MENUNOTIFY = WM_USER;
constexpr static UINT TID_RENDER    = 0x6006f00d;

struct KawaiiTrayWndPrivate {
    constexpr static UINT TRAYID = 0;

    HMENU                    menu;
    HMENU                    themeMenu;
    std::vector<std::string> themes;
    int                      currentThemeId;
    NOTIFYICONDATA           nid;
    std::queue<HICON>        iconAnimeSet;

    KawaiiTrayWndPrivate()
        : menu{nullptr}
        , themeMenu{nullptr}
        , currentThemeId{-1} {}

    ~KawaiiTrayWndPrivate() {
        if (menu != nullptr) {
            DestroyMenu(menu);
            menu = nullptr;
        }
        if (themeMenu != nullptr) {
            DestroyMenu(themeMenu);
            themeMenu = nullptr;
        }
        release();
    }

    void setupTrayIcon(HWND parent, const char* hint, UINT msgid) {
        memset(&nid, 0, sizeof(nid));
        nid.cbSize           = sizeof(NOTIFYICONDATA);
        nid.hWnd             = parent;
        nid.uID              = 0;
        nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = msgid;
        strcpy_s(nid.szTip, hint);
        Shell_NotifyIcon(NIM_ADD, &nid);
    }

    void setupMenu() {
        menu      = CreatePopupMenu();
        themeMenu = CreatePopupMenu();
        assert(menu != nullptr);
        assert(themeMenu != nullptr);

        using Action = KawaiiTrayWnd::Action;
        assert(!themes.empty());
        for (int themeId = 0; themeId < themes.size(); ++themeId) {
            AppendMenu(
                themeMenu,
                MF_STRING | MF_UNCHECKED,
                Action::SelectTheme | (themeId + 1),
                themes[themeId].c_str());
        }

        AppendMenu(
            menu,
            MF_POPUP | MF_STRING,
            reinterpret_cast<UINT_PTR>(themeMenu),
            "Theme");
        AppendMenu(menu, MF_STRING, Action::Quit, "Quit");

        assert(
            currentThemeId >= 0 && currentThemeId < themes.size()
            || currentThemeId == -1);
        if (currentThemeId != -1) {
            CheckMenuItem(
                themeMenu, currentThemeId, MF_BYPOSITION | MF_CHECKED);
        }
    }

    void switchTo(int themeId) {
        assert(themeId >= 0 && themeId < themes.size());

        const auto path = fs::path(KawaiiTrayWnd::assetLocation())
                              .append("themes")
                              .append(themes[themeId]);
        assert(fs::exists(path));

        release();

        for (const auto& e : fs::directory_iterator(path)) {
            if (!e.is_regular_file()) { continue; }
            auto ext = e.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
            if (ext != ".ico") { continue; }
            auto handle = LoadImage(
                nullptr,
                e.path().string().c_str(),
                IMAGE_ICON,
                0,
                0,
                LR_LOADFROMFILE | LR_DEFAULTSIZE);
            if (handle == nullptr) { continue; }
            iconAnimeSet.push(static_cast<HICON>(handle));
        }
        assert(!iconAnimeSet.empty());

        if (currentThemeId != -1) {
            CheckMenuItem(
                themeMenu, currentThemeId, MF_BYPOSITION | MF_UNCHECKED);
        }
        CheckMenuItem(themeMenu, themeId, MF_BYPOSITION | MF_CHECKED);
        currentThemeId = themeId;
    }

    void updateAnime() {
        if (currentThemeId == -1) { return; }
        assert(!iconAnimeSet.empty());
        const auto e = iconAnimeSet.front();
        iconAnimeSet.pop();
        iconAnimeSet.push(e);
        nid.hIcon = e;
        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }

    void release() {
        while (!iconAnimeSet.empty()) {
            DestroyIcon(iconAnimeSet.front());
            iconAnimeSet.pop();
        }
    }
};

KawaiiTrayWnd::KawaiiTrayWnd()
    : WndHelper(CLSID_KAWAIITRAY)
    , d{std::make_unique<KawaiiTrayWndPrivate>()} {
    const auto resp = loadThemes();
    assert(resp > 0);

    d->setupMenu();
    d->setupTrayIcon(handle(), "Kawaii Tray", WM_MENUNOTIFY);

    SetTimer(handle(), TID_RENDER, 10, nullptr);
}

KawaiiTrayWnd::~KawaiiTrayWnd() {
    KillTimer(handle(), TID_RENDER);
}

size_t KawaiiTrayWnd::loadThemes() {
    const auto path = fs::path(KawaiiTrayWnd::assetLocation()).append("themes");
    if (!fs::exists(path)) { fs::create_directories(path); }
    for (const auto& e : fs::directory_iterator(path)) {
        if (!e.is_directory()) { continue; }
        d->themes.push_back(e.path().filename().string());
    }
    return d->themes.size();
}

void KawaiiTrayWnd::setTheme(int themeId) {
    d->switchTo(themeId);
}

bool KawaiiTrayWnd::notify(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_MENUNOTIFY) {
        assert(HIWORD(lParam) == KawaiiTrayWndPrivate::TRAYID);
        switch (LOWORD(lParam)) {
            case WM_RBUTTONUP: {
                POINT pt{};
                GetCursorPos(&pt);
                SetForegroundWindow(handle());
                const auto id = TrackPopupMenu(
                    d->menu, TPM_RETURNCMD, pt.x, pt.y, 0, handle(), nullptr);
                if (id != 0) { handleAction(static_cast<Action>(id)); }
            } break;
        }
        return true;
    }

    if (msg == WM_TIMER && wParam == TID_RENDER) {
        d->updateAnime();
        return true;
    }

    return false;
}

void KawaiiTrayWnd::handleAction(Action action) {
    switch (action) {
        case Action::Quit: {
            Shell_NotifyIcon(NIM_DELETE, &d->nid);
            PostQuitMessage(0);
        } break;
        default: {
            if ((action & 0xffff0000) == Action::SelectTheme) {
                const auto themeId = (action & 0xffff) - 1;
                setTheme(themeId);
            }
        } break;
    }
}

fs::path KawaiiTrayWnd::assetLocation() {
    char buffer[MAX_PATH]{};
    GetModuleFileName(nullptr, buffer, MAX_PATH);
    assert(GetLastError() == 0);
    return fs::absolute(buffer).replace_filename("assets");
}
