#pragma once
#include <windows.h>
#include <functional>
#include "activation_settings.h"

namespace SwashMoji {
// Reserve a replacement before releasing the live shortcut. Two IDs are enough
// to alternate registrations; failed changes leave the old registration intact.
class ActivationRegistration {
public:
    using Register = std::function<BOOL(HWND, int, UINT, UINT)>;
    using Unregister = std::function<BOOL(HWND, int)>;
    ActivationRegistration(Register add = RegisterHotKey, Unregister remove = UnregisterHotKey)
        : add_(add), remove_(remove) {}
    int Id() const { return id_; }
    bool Prepare(HWND window, unsigned int hotkey) {
        if (!ValidActivationHotkey(hotkey)) return false;
        if (id_ && hotkey == value_) return true;
        pending_ = id_ == 1 ? 2 : 1;
        if (add_(window, pending_, (hotkey >> 8) | MOD_NOREPEAT, hotkey & 255)) return true;
        pending_ = 0;
        return false;
    }
    void Cancel(HWND window) {
        if (pending_) remove_(window, pending_);
        pending_ = 0;
    }
    void Commit(HWND window, unsigned int hotkey) {
        if (pending_) {
            if (id_) remove_(window, id_);
            id_ = pending_; pending_ = 0; value_ = hotkey;
        }
    }
    void Clear(HWND window) {
        Cancel(window);
        if (id_) remove_(window, id_);
        id_ = 0; value_ = 0;
    }
private:
    Register add_;
    Unregister remove_;
    int id_{}, pending_{};
    unsigned int value_{};
};

inline constexpr wchar_t kStartupKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
inline std::wstring StartupCommand(const std::wstring& executable) { return L"\"" + executable + L"\""; }
// Optional key path allows adapter tests to use an isolated registry fixture.
inline LSTATUS ReadStartupCommand(std::wstring& command, const wchar_t* keyPath = kStartupKey) {
    command.clear();
    HKEY key{};
    auto result = RegOpenKeyExW(HKEY_CURRENT_USER, keyPath, 0, KEY_QUERY_VALUE, &key);
    if (result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    if (result != ERROR_SUCCESS) return result;
    DWORD bytes{}, type{};
    result = RegQueryValueExW(key, L"SwashMoji", nullptr, &type, nullptr, &bytes);
    if (result == ERROR_SUCCESS && (type != REG_SZ || bytes > 65536 || bytes % sizeof(wchar_t))) result = ERROR_INVALID_DATA;
    if (result == ERROR_SUCCESS) {
        std::wstring buffer(bytes / sizeof(wchar_t) + 1, L'\0');
        result = RegQueryValueExW(key, L"SwashMoji", nullptr, nullptr, reinterpret_cast<BYTE*>(buffer.data()), &bytes);
        if (result == ERROR_SUCCESS) command.assign(buffer.c_str());
    }
    RegCloseKey(key);
    return result == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : result;
}
inline LSTATUS WriteStartupCommand(const std::wstring& command, const wchar_t* keyPath = kStartupKey) {
    HKEY key{};
    auto result = command.empty()
        ? RegOpenKeyExW(HKEY_CURRENT_USER, keyPath, 0, KEY_SET_VALUE, &key)
        : RegCreateKeyExW(HKEY_CURRENT_USER, keyPath, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (command.empty() && result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    if (result != ERROR_SUCCESS) return result;
    result = command.empty() ? RegDeleteValueW(key, L"SwashMoji") :
        RegSetValueExW(key, L"SwashMoji", 0, REG_SZ, reinterpret_cast<const BYTE*>(command.c_str()),
                      static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return command.empty() && result == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : result;
}
}
