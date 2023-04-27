#include "utils.h"

#include <stdexcept>
#include <iostream>
#include <assert.h>
#include <memory>

class UniqueWndHelper : public WndHelper {
public:
    enum Action : UINT_PTR {
        Quit = 1,
    };

    UniqueWndHelper()
        : WndHelper(CLSID_KAWAIITRAY)
        , menu_{nullptr}
        , nid_{} {
        nid_.cbSize           = sizeof(NOTIFYICONDATA);
        nid_.hWnd             = handle();
        nid_.uID              = 0;
        nid_.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid_.uCallbackMessage = WM_MENUNOTIFY;
        nid_.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
        strcpy_s(nid_.szTip, "Kawaii Tray");
        Shell_NotifyIcon(NIM_ADD, &nid_);

        menu_ = CreatePopupMenu();
        AppendMenu(menu_, MF_STRING, Action::Quit, "Quit");
    }

    ~UniqueWndHelper() {
        assert(menu_ != nullptr);
        DestroyMenu(menu_);
        menu_ = nullptr;
    }

    void init() override {
        if (FindWindowA(nullptr, CLSID_KAWAIITRAY) != nullptr) {
            throw std::logic_error("program is already running");
        }
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

        return false;
    }

    void handleAction(Action action) {
        switch (action) {
            case Action::Quit: {
                Shell_NotifyIcon(NIM_DELETE, &nid_);
                PostQuitMessage(0);
            } break;
        }
    }

private:
    constexpr static const char* CLSID_KAWAIITRAY{"KawaiiTrayIconWnd"};
    constexpr static UINT        WM_MENUNOTIFY = WM_USER;

    HMENU          menu_;
    NOTIFYICONDATA nid_;
};

int main(int argc, char* argv[]) {
    std::unique_ptr<WndHelper> app{};

    try {
        app = std::make_unique<UniqueWndHelper>();
    } catch (const std::logic_error& e) {
        //! program is already running, quit directly
        return 0;
    } catch (const std::runtime_error& e) {
        //! unexpected error
        return -1;
    }

    return app->exec();
}
