#pragma once

#include <windows.h>

class WndHelper {
protected:
    static LRESULT CALLBACK
        defaultCallbackFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    WndHelper(const char* CLASSID);
    virtual ~WndHelper();

    inline virtual bool notify(UINT msg, WPARAM wParam, LPARAM lParam) {
        return false;
    }

    int exec() const;

    inline HWND handle() const {
        return instance_;
    }

private:
    HWND        instance_;
    const char* classid_;
};
