#include "insertion.h"
#include <algorithm>

namespace SwashMoji {
bool ValidInputText(const std::wstring& text) {
    if (text.empty() || text.size() > 4096) return false;
    for (size_t i = 0; i < text.size(); ++i) {
        const auto unit = static_cast<unsigned int>(text[i]);
        if (!unit || unit > 0xFFFF) return false;
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (++i == text.size() || text[i] < 0xDC00 || text[i] > 0xDFFF) return false;
        } else if (unit >= 0xDC00 && unit <= 0xDFFF) return false;
    }
    return true;
}

namespace {
KeyEvent Modifier(std::uint16_t key, bool up) {
    return {key, false, up, key == 0xA3}; // right Ctrl is extended; Shift is not
}
}

InsertionOutcome InsertText(InputPlatform& platform, const InputTarget& target, const std::wstring& text) {
    if (!ValidInputText(text)) return {InsertionStatus::InvalidPayload};
    if (!target.window || !platform.ValidTarget(target)) return {InsertionStatus::NoTarget};
    const auto modifiers = platform.HeldModifiers();
    // Releasing Alt/Windows can itself activate menus or the shell. Do not send
    // text under those keys. Ctrl+Enter continues to work while Ctrl stays held.
    for (const auto key : modifiers) {
        if (key != 0xA0 && key != 0xA1 && key != 0xA2 && key != 0xA3) {
            return {InsertionStatus::ReleaseModifiers};
        }
    }
    std::vector<KeyEvent> events;
    events.reserve(text.size() * 2 + modifiers.size() * 2);
    for (const auto key : modifiers) events.push_back(Modifier(key, true));
    for (const auto unit : text) {
        events.push_back({static_cast<std::uint16_t>(unit), true, false, false});
        events.push_back({static_cast<std::uint16_t>(unit), true, true, false});
    }
    for (const auto key : modifiers) events.push_back(Modifier(key, false));

    platform.Activate(target.window);
    // An activation request can be denied or become stale; neither case types
    // into the currently focused application. No automatic activation retries.
    if (!platform.ValidTarget(target)) return {InsertionStatus::NoTarget};
    if (platform.Foreground() != target.window) return {InsertionStatus::FocusFailed};
    const size_t sent = std::min(platform.Send(events), events.size());
    InsertionOutcome outcome{sent == events.size() ? InsertionStatus::FullySubmitted :
        (sent ? InsertionStatus::PartiallySubmitted : InsertionStatus::NoneSubmitted), sent, events.size()};
    if (!sent || sent == events.size()) return outcome;

    // Partial submission is never retried. Only release keys: do not manufacture
    // another text key-down or a modifier key-down after an uncertain attempt.
    std::vector<KeyEvent> cleanup;
    if (events[sent - 1].unicode && !events[sent - 1].up) {
        auto up = events[sent - 1];
        up.up = true;
        cleanup.push_back(up);
    }
    for (const auto key : modifiers) cleanup.push_back(Modifier(key, true));
    outcome.cleanupComplete = cleanup.empty() || platform.Send(cleanup) == cleanup.size();
    return outcome;
}

void FocusReturn::Arm(std::uint64_t token, WindowToken target) { token_ = token; target_ = target; pending_ = true; }
void FocusReturn::Cancel() { pending_ = false; }
void FocusReturn::ForegroundChanged(WindowToken window) { if (window != target_) Cancel(); }
bool FocusReturn::Take(std::uint64_t token, WindowToken foreground, bool visible, bool validTarget) {
    if (!pending_ || token != token_) return false;
    pending_ = false;
    return visible && validTarget && foreground == target_;
}

CopyOutcome CopyText(ClipboardPlatform& platform, const std::wstring& text) {
    if (!ValidInputText(text)) return {CopyStatus::InvalidPayload};
    struct Cleanup { ClipboardPlatform& platform; ~Cleanup() { platform.Release(); } } cleanup{platform};
    if (!platform.Prepare(text)) return {CopyStatus::PreparationFailed};
    if (!platform.Open()) return {CopyStatus::Busy};
    if (!platform.Empty()) {
        return {CopyStatus::EmptyFailed, false, false, platform.Close()};
    }
    if (!platform.Publish()) {
        return {CopyStatus::PublishFailed, true, false, platform.Close()};
    }
    if (!platform.Close()) return {CopyStatus::CloseFailed, true, true, false};
    return {CopyStatus::Copied, true, true};
}
}
