#include "utils.h"

#include <stdexcept>
#include <string>

LRESULT CALLBACK WndHelper::defaultCallbackFunc(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); }
    auto w =
        reinterpret_cast<WndHelper*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (w != nullptr && w->notify(msg, wParam, lParam)) { return S_OK; }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

WndHelper::WndHelper(const char* CLASSID) {
    init();

    WNDCLASS wndcls{};
    wndcls.style         = CS_HREDRAW | CS_VREDRAW;
    wndcls.lpfnWndProc   = &WndHelper::defaultCallbackFunc;
    wndcls.cbClsExtra    = 0;
    wndcls.cbWndExtra    = sizeof(WndHelper*);
    wndcls.hInstance     = GetModuleHandle(nullptr);
    wndcls.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wndcls.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wndcls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndcls.lpszMenuName  = nullptr;
    wndcls.lpszClassName = CLASSID;

    ATOM id = RegisterClass(&wndcls);
    if (id == 0) {
        throw std::runtime_error(
            "failed to register window class: GetLastError(): "
            + std::to_string(GetLastError()));
    }

    instance_ = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        CLASSID,
        CLASSID,
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (instance_ == nullptr) {
        throw std::runtime_error(
            "failed to create the window: GetLastError(): "
            + std::to_string(GetLastError()));
    }

    SetWindowLongPtr(
        instance_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

int WndHelper::exec() const {
    ShowWindow(instance_, SW_NORMAL);
    UpdateWindow(instance_);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
