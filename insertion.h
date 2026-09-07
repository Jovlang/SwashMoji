#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SwashMoji {
using WindowToken = std::uintptr_t;
struct InputTarget {
    WindowToken window{};
    std::uint32_t process{}, thread{};
};

struct KeyEvent {
    std::uint16_t code{};
    bool unicode{}, up{}, extended{};
};

class InputPlatform {
public:
    virtual ~InputPlatform() = default;
    virtual bool ValidTarget(const InputTarget& target) = 0;
    virtual WindowToken Foreground() = 0;
    virtual void Activate(WindowToken window) = 0;
    virtual std::vector<std::uint16_t> HeldModifiers() = 0;
    virtual size_t Send(const std::vector<KeyEvent>& events) = 0;
};

enum class InsertionStatus { NoTarget, InvalidPayload, ReleaseModifiers, FocusFailed, NoneSubmitted, PartiallySubmitted, FullySubmitted };
struct InsertionOutcome {
    InsertionStatus status{InsertionStatus::NoTarget};
    size_t submitted{}, requested{};
    bool cleanupComplete{true};
};

bool ValidInputText(const std::wstring& text);
InsertionOutcome InsertText(InputPlatform& platform, const InputTarget& target, const std::wstring& text);

// One-shot guard for delayed focus restoration. The caller uses unique timer
// IDs because KillTimer does not remove already queued WM_TIMER messages.
class FocusReturn {
public:
    void Arm(std::uint64_t token, WindowToken target);
    void Cancel();
    void ForegroundChanged(WindowToken window);
    bool Take(std::uint64_t token, WindowToken foreground, bool visible, bool validTarget);
    bool Pending() const { return pending_; }
private:
    bool pending_{};
    std::uint64_t token_{};
    WindowToken target_{};
};

class ClipboardPlatform {
public:
    virtual ~ClipboardPlatform() = default;
    virtual bool Prepare(const std::wstring& text) = 0;
    virtual bool Open() = 0;
    virtual bool Empty() = 0;
    virtual bool Publish() = 0; // Transfers ownership of the prepared allocation.
    virtual bool Close() = 0;
    virtual void Release() = 0; // Frees only allocations still owned by the caller.
};

enum class CopyStatus { InvalidPayload, PreparationFailed, Busy, EmptyFailed, PublishFailed, CloseFailed, Copied };
struct CopyOutcome {
    CopyStatus status{CopyStatus::InvalidPayload};
    bool clipboardChanged{}, textPublished{};
    bool clipboardClosed{true};
};
CopyOutcome CopyText(ClipboardPlatform& platform, const std::wstring& text);
}
