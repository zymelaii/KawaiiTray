#include "app.h"
#include "cpu_monitor.h"

#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <string>
#include <stdarg.h>
#include <assert.h>

constexpr static UINT WM_MENUNOTIFY          = WM_USER;
constexpr static UINT TID_RENDER             = 0x6006f00d;
constexpr static auto RENDER_INTERVAL_MINMAX = std::make_pair(20, 100);

struct KawaiiTrayWndPrivate {
    constexpr static UINT TRAYID = 0;

    using theme_pair_t = std::pair<std::string, std::string>;

    HMENU                     menu;
    HMENU                     themeMenu;
    std::vector<theme_pair_t> themes;
    int                       currentThemeId;
    NOTIFYICONDATA            nid;
    std::queue<HICON>         iconAnimeSet;
    int                       updateInterval;

    KawaiiTrayWndPrivate()
        : menu{nullptr}
        , themeMenu{nullptr}
        , currentThemeId{-1}
        , updateInterval{RENDER_INTERVAL_MINMAX.second} {}

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
                themes[themeId].first.c_str());
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
                              .append(themes[themeId].second);
        assert(fs::exists(path));

        release();

        const auto node = YAML::LoadFile((path / "mainfest").string());
        const auto seq  = node["sequence"];
        for (const auto& frame : seq) {
            const auto framePath = path / frame.as<std::string>();
            assert(fs::is_regular_file(framePath));
            assert(({
                auto ext = framePath.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
                ext == ".ico";
            }));
            auto handle = LoadImage(
                nullptr,
                framePath.string().c_str(),
                IMAGE_ICON,
                0,
                0,
                LR_LOADFROMFILE | LR_DEFAULTSIZE);
            assert(handle != nullptr);
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

    if (resp > 0) {
        d->setupMenu();
        d->setupTrayIcon(handle(), "Kawaii Tray", WM_MENUNOTIFY);
    }

    SetTimer(handle(), TID_RENDER, d->updateInterval, nullptr);

    CpuMonitor::getInstance().addListener(std::bind(
        &KawaiiTrayWnd::cpuUsageUpdated, this, std::placeholders::_1));
}

KawaiiTrayWnd::~KawaiiTrayWnd() {
    KillTimer(handle(), TID_RENDER);
}

std::vector<std::string> KawaiiTrayWnd::themes() const {
    std::vector<std::string> resp{};
    std::transform(
        d->themes.begin(),
        d->themes.end(),
        std::back_inserter(resp),
        [](const auto& e) {
            return e.first;
        });
    return std::move(resp);
}

size_t KawaiiTrayWnd::loadThemes() {
    const auto path = fs::path(KawaiiTrayWnd::assetLocation()).append("themes");
    if (!fs::exists(path)) { fs::create_directories(path); }
    for (const auto& e : fs::directory_iterator(path)) {
        if (!e.is_directory()) { continue; }
        const auto path = e.path() / "mainfest";
        if (!fs::exists(path)) { continue; }
        try {
            auto node = YAML::LoadFile(path.string());
            if (!node["name"]) { continue; }
            const auto seq = node["sequence"];
            if (!seq || !seq.IsSequence()) { continue; }
            bool allFramesExist = true;
            for (const auto& frame : seq) {
                if (!fs::exists(e.path() / frame.as<std::string>())) {
                    allFramesExist = false;
                    break;
                }
            }
            if (seq.size() == 0 || !allFramesExist) { continue; }
            const auto name = node["name"].as<std::string>();
            const auto dir  = e.path().filename().string();
            d->themes.push_back({name, dir});
        } catch (YAML::BadFile) { continue; }
    }
    return d->themes.size();
}

void KawaiiTrayWnd::setTheme(int themeId) {
    d->switchTo(themeId);
}

void KawaiiTrayWnd::setTipText(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf_s(d->nid.szTip, fmt, args);
    va_end(args);
    Shell_NotifyIcon(NIM_MODIFY, &d->nid);
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

void KawaiiTrayWnd::cpuUsageUpdated(float usage) {
    setTipText("CPU: %.2f%%", usage * 100);
    const auto& [minval, maxval] = RENDER_INTERVAL_MINMAX;
    d->updateInterval = static_cast<int>(maxval + (minval - maxval) * usage);
    SetTimer(handle(), TID_RENDER, d->updateInterval, nullptr);
}
