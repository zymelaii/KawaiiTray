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

    const std::vector<std::string>& themes() const;

    size_t loadThemes();
    void   setTheme(int themeId);
    void   setTipText(const char* fmt, ...);
    bool   notify(UINT msg, WPARAM wParam, LPARAM lParam) override;
    void   handleAction(Action action);

    static fs::path assetLocation();

protected:
    void cpuUsageUpdated(float usage);

private:
    std::unique_ptr<KawaiiTrayWndPrivate> d;
};
