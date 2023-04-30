#pragma once

#include "utils.h"

#include <queue>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

struct KawaiiTrayWndPrivate;

class KawaiiTrayWnd : public WndHelper {
public:
    enum Action : UINT_PTR {
        Quit        = 1,
        SelectTheme = 0xfeef0000,
    };

    constexpr static const char* CLSID_KAWAIITRAY{"KawaiiTrayIconWnd"};

    KawaiiTrayWnd();
    ~KawaiiTrayWnd() override;

    size_t loadThemes();
    void   setTheme(int themeId);
    bool   notify(UINT msg, WPARAM wParam, LPARAM lParam) override;
    void   handleAction(Action action);

    static fs::path assetLocation();

private:
    std::unique_ptr<KawaiiTrayWndPrivate> d;
};
