#pragma once
#include <windows.h>
#include "insertion.h"

namespace SwashMoji {
InputTarget CaptureExternalTarget(HWND window);

class Win32InputPlatform : public InputPlatform {
public:
    bool ValidTarget(const InputTarget& target) override;
    WindowToken Foreground() override;
    void Activate(WindowToken window) override;
    std::vector<std::uint16_t> HeldModifiers() override;
    size_t Send(const std::vector<KeyEvent>& events) override;
};

class Win32ClipboardPlatform : public ClipboardPlatform {
public:
    explicit Win32ClipboardPlatform(HWND owner) : owner_(owner) {}
    ~Win32ClipboardPlatform() override;
    bool Prepare(const std::wstring& text) override;
    bool Open() override;
    bool Empty() override;
    bool Publish() override;
    bool Close() override;
    void Release() override;
private:
    HWND owner_{};
    HGLOBAL memory_{};
    bool open_{};
};
}
