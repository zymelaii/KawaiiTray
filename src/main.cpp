#include "app.h"

int main(int argc, char* argv[]) {
    if (FindWindowA(nullptr, KawaiiTrayWnd::CLSID_KAWAIITRAY) != nullptr) {
        return -1;
    }

    auto app = std::make_unique<KawaiiTrayWnd>();
    app->setTheme(0);
    return app->exec();
}
