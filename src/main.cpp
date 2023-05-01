#include "app.h"

#ifdef _WINDOWS
int WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char* argv[])
#endif
{
    if (FindWindowA(nullptr, KawaiiTrayWnd::CLSID_KAWAIITRAY) != nullptr) {
        return -1;
    }

    auto app = std::make_unique<KawaiiTrayWnd>();

    if (app->themes().empty()) {
        MessageBox(nullptr, "No themes available", "Kawaii Tray", MB_OK);
        return -2;
    }

    app->setTheme(0);
    return app->exec();
}
