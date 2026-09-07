#include "insertion.h"
#include "test_support.h"
#include <deque>
#include <limits>
#include <map>

using namespace SwashMoji;
const InputTarget target{10, 20, 30};

struct FakeInput : InputPlatform {
    bool valid{true}, denied{}, destroyOnActivate{}, switchOnActivate{};
    WindowToken foreground{99};
    int activations{};
    std::vector<std::uint16_t> modifiers;
    std::deque<size_t> accept;
    std::vector<std::vector<KeyEvent>> batches;
    std::map<std::uint16_t, bool> held;
    bool ValidTarget(const InputTarget& t) override { return valid && t.window == target.window; }
    WindowToken Foreground() override { return foreground; }
    void Activate(WindowToken window) override {
        ++activations;
        if (!denied) foreground = switchOnActivate ? 88 : window;
        if (destroyOnActivate) valid = false;
    }
    std::vector<std::uint16_t> HeldModifiers() override { return modifiers; }
    size_t Send(const std::vector<KeyEvent>& events) override {
        batches.push_back(events);
        const size_t sent = accept.empty() ? events.size() : accept.front();
        if (!accept.empty()) accept.pop_front();
        for (size_t i = 0; i < sent && i < events.size(); ++i) {
            if (!events[i].unicode) held[events[i].code] = !events[i].up;
        }
        return sent;
    }
};

void InvalidAndFocus() {
    FakeInput platform;
    CHECK(InsertText(platform, {}, L"🚀").status == InsertionStatus::NoTarget);
    CHECK(platform.activations == 0 && platform.batches.empty());
    CHECK(InsertText(platform, target, L"").status == InsertionStatus::InvalidPayload);
    CHECK(!ValidInputText(std::wstring(1, 0xD800)));
    CHECK(!ValidInputText(std::wstring(1, 0xDC00)));
    CHECK(!ValidInputText(std::wstring(L"a\0b", 3)));
    CHECK(!ValidInputText(std::wstring(4097, L'a')));
    CHECK(ValidInputText(L"👍🏽👩‍💻🚀✨"));
    platform.denied = true;
    CHECK(InsertText(platform, target, L"🚀").status == InsertionStatus::FocusFailed);
    CHECK(platform.batches.empty());
    platform.denied = false;
    platform.switchOnActivate = true;
    CHECK(InsertText(platform, target, L"🚀").status == InsertionStatus::FocusFailed);
    platform.destroyOnActivate = true;
    CHECK(InsertText(platform, target, L"🚀").status == InsertionStatus::NoTarget);
    CHECK(platform.batches.empty());
}

void FullAndPartialSubmission() {
    const std::wstring text = L"👍🏽👩‍💻🚀✨";
    FakeInput full;
    full.modifiers = {0xA2, 0xA3, 0xA0}; // left/right Ctrl and left Shift
    auto outcome = InsertText(full, target, text);
    CHECK(outcome.status == InsertionStatus::FullySubmitted);
    CHECK(full.batches.size() == 1);
    std::wstring emitted;
    for (const auto& event : full.batches[0]) if (event.unicode && !event.up) emitted += static_cast<wchar_t>(event.code);
    CHECK(emitted == text);
    for (auto key : full.modifiers) CHECK(full.held[key]);
    CHECK(full.batches[0][1].extended); // right Ctrl

    for (size_t prefix = 0; prefix < outcome.requested; ++prefix) {
        FakeInput partial;
        partial.modifiers = full.modifiers;
        partial.accept.push_back(prefix);
        const auto result = InsertText(partial, target, text);
        CHECK(result.status == (prefix ? InsertionStatus::PartiallySubmitted : InsertionStatus::NoneSubmitted));
        CHECK(result.submitted == prefix && result.cleanupComplete);
        CHECK(partial.batches.size() == (prefix ? 2 : 1));
        if (prefix) {
            // No partial prefix ever triggers another text/modifier key-down.
            for (const auto& event : partial.batches[1]) CHECK(event.up);
            for (auto key : partial.modifiers) CHECK(!partial.held[key]);
        }
    }
    FakeInput cleanupFailure;
    cleanupFailure.modifiers = {0xA2};
    cleanupFailure.accept = {2, 0};
    CHECK(!InsertText(cleanupFailure, target, L"🚀").cleanupComplete);
    CHECK(cleanupFailure.batches.size() == 2);
    FakeInput alt;
    for (auto key : {0xA4, 0xA5, 0x5B, 0x5C}) {
        alt.modifiers = {static_cast<std::uint16_t>(key)};
        CHECK(InsertText(alt, target, text).status == InsertionStatus::ReleaseModifiers);
    }
    CHECK(alt.batches.empty() && alt.activations == 0);
}

void DelayedFocus() {
    FocusReturn pending;
    pending.Arm(1, 10);
    CHECK(pending.Take(1, 10, true, true));
    CHECK(!pending.Take(1, 10, true, true));
    pending.Arm(2, 10);
    pending.Cancel();
    CHECK(!pending.Take(2, 10, true, true));
    pending.Arm(3, 10);
    pending.ForegroundChanged(88);
    pending.ForegroundChanged(10); // switching back must not resurrect the callback
    CHECK(!pending.Take(3, 10, true, true));
    pending.Arm(4, 10);
    CHECK(!pending.Take(3, 10, true, true)); // stale queued timer leaves new attempt intact
    CHECK(pending.Take(4, 10, true, true));
    pending.Arm(5, 10);
    CHECK(!pending.Take(5, 10, false, true));
    pending.Arm(6, 10);
    CHECK(!pending.Take(6, 10, true, false));
    pending.Arm(7, 10);
    CHECK(!pending.Take(7, 88, true, true));
}

struct FakeClipboard : ClipboardPlatform {
    bool prepare{true}, open{true}, empty{true}, publish{true}, close{true};
    bool allocated{}, transferred{}, freed{};
    std::string calls;
    bool Prepare(const std::wstring&) override { calls += 'P'; allocated = true; return prepare; }
    bool Open() override { calls += 'O'; return open; }
    bool Empty() override { calls += 'E'; return empty; }
    bool Publish() override { calls += 'S'; if (publish) { allocated = false; transferred = true; } return publish; }
    bool Close() override { calls += 'C'; return close; }
    void Release() override { calls += 'R'; freed = allocated; allocated = false; }
};

void ClipboardFailures() {
    FakeClipboard clipboard;
    CHECK(CopyText(clipboard, L"").status == CopyStatus::InvalidPayload);
    CHECK(clipboard.calls.empty());
    clipboard.prepare = false;
    auto result = CopyText(clipboard, L"🚀");
    CHECK(result.status == CopyStatus::PreparationFailed && !result.clipboardChanged);
    CHECK(clipboard.calls == "PR" && clipboard.freed);
    clipboard = {};
    clipboard.open = false;
    result = CopyText(clipboard, L"🚀");
    CHECK(result.status == CopyStatus::Busy && !result.clipboardChanged);
    CHECK(clipboard.calls == "POR" && clipboard.freed);
    clipboard = {};
    clipboard.empty = false;
    result = CopyText(clipboard, L"🚀");
    CHECK(result.status == CopyStatus::EmptyFailed && !result.clipboardChanged);
    CHECK(clipboard.calls == "POECR" && clipboard.freed);
    clipboard = {};
    clipboard.empty = false;
    clipboard.close = false;
    result = CopyText(clipboard, L"🚀");
    CHECK(result.status == CopyStatus::EmptyFailed && !result.clipboardClosed && !result.clipboardChanged);
    clipboard = {};
    clipboard.publish = false;
    result = CopyText(clipboard, L"🚀");
    CHECK(result.status == CopyStatus::PublishFailed && result.clipboardChanged && !result.textPublished);
    CHECK(clipboard.calls == "POESCR" && clipboard.freed);
    clipboard = {};
    clipboard.close = false;
    result = CopyText(clipboard, L"🚀");
    CHECK(result.status == CopyStatus::CloseFailed && result.textPublished && result.clipboardChanged && !result.clipboardClosed);
    CHECK(!clipboard.freed && clipboard.transferred);
    clipboard = {};
    result = CopyText(clipboard, L"👍🏽👩‍💻🚀✨");
    CHECK(result.status == CopyStatus::Copied && result.textPublished && result.clipboardChanged);
    CHECK(clipboard.calls == "POESCR" && !clipboard.freed && clipboard.transferred);
}

int main() {
    try {
        InvalidAndFocus(); FullAndPartialSubmission(); DelayedFocus(); ClipboardFailures();
        std::cout << "Input outcomes, partial-prefix cleanup, focus cancellation and clipboard failures passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
