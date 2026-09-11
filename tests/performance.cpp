// Real picker result-update code, hidden and without desktop input or profile IO.
// This intentionally does not claim to measure foreground activation or painting.
#define wWinMain SwashMojiUnusedApplicationMain
#include "../src/main.cpp"
#undef wWinMain
#include "test_support.h"
#include <chrono>
#include <iomanip>
#include <set>

using Clock = std::chrono::steady_clock;

template<class Action> void Measure(const char* name, Action action) {
    for (int i = 0; i < 10; ++i) action();
    std::vector<double> milliseconds;
    for (int i = 0; i < 200; ++i) {
        const auto start = Clock::now(); action();
        milliseconds.push_back(std::chrono::duration<double, std::milli>(Clock::now() - start).count());
    }
    std::sort(milliseconds.begin(), milliseconds.end());
    std::cout << name << ',' << milliseconds.size() << ',' << milliseconds[99] << ','
              << milliseconds[189] << ',' << milliseconds.back() << '\n';
}

int main() {
    try {
        CHECK(LoadEmojis());
        std::set<ResultId> families;
        for (const auto& emoji : g_catalog.Entries()) families.insert({ResultKind::Emoji, emoji.family.value});
        std::vector<ResultId> ids(families.begin(), families.end()); CHECK(ids.size() >= kMaxHistory);
        for (const auto& id : ids) g_profile.usage[id] = 10;
        g_profile.history.assign(ids.begin(), ids.begin() + kMaxHistory);
        g_profile.pins.assign(ids.begin(), ids.begin() + kMaxPins);
        for (size_t i = 0; i < kMaxAliases; ++i) {
            const auto phrase = L"personal phrase " + std::to_wstring(i);
            CHECK(SetAlias(g_profile, g_catalog, phrase, ids[i % ids.size()]) == AliasResult::Saved);
        }
        for (size_t i = 0; i < kMaxQueryChoices; ++i)
            g_profile.queryChoices.push_back({L"personal phrase " + std::to_wstring(i), ids[i % ids.size()], 10});
        for (size_t i = 0; i < kMaxCombinations; ++i) {
            Combination combination; combination.name = L"sequence " + std::to_wstring(i);
            for (const auto* glyph : {L"🚀", L"✨"}) {
                const auto* emoji = g_catalog.Find(glyph); CHECK(emoji);
                combination.entries.push_back({emoji->family.value, emoji->glyph});
            }
            std::wstring error; CHECK(SaveCombination(g_profile, g_catalog, combination, error));
        }
        CHECK(DecodeProfile(EncodeProfile(g_profile)).skippedRecords == 0);
        g_uiFont = g_statusFont = g_emojiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        g_backgroundBrush = CreateSolidBrush(kBackground); g_inputBrush = CreateSolidBrush(kInputBackground);
        WNDCLASSW type{}; type.hInstance = GetModuleHandleW(nullptr);
        type.lpszClassName = L"SwashMojiPerformance"; type.lpfnWndProc = WindowProc;
        CHECK(RegisterClassW(&type));
        g_window = CreateWindowExW(0, type.lpszClassName, L"Performance", WS_POPUP,
            0, 0, kPickerWidth, PickerHeight(), nullptr, nullptr, type.hInstance, nullptr); CHECK(g_window);
        std::cout << "# Release; hidden native controls; 10 warmups; 500 aliases; 200 combinations; 1000 query pairs; 40 recent; 10 pins; "
                  << ids.size() << " family usage records\nmetric,samples,p50_ms,p95_ms,max_ms\n" << std::fixed << std::setprecision(3);
        Measure("session_prepare_empty", [] { BeginPickerSession(); });
        for (const auto* query : {L"r", L"rocket", L"takk", L"thank you", L"personal phrase 499", L"sequence 199", L"roket", L"zzzzzzzz"}) {
            const auto name = "query_" + WideToUtf8(query);
            // WM_SETTEXT synchronously delivers EN_CHANGE through the real picker.
            Measure(name.c_str(), [query] { SetWindowTextW(g_edit, query); });
        }
        DestroyWindow(g_window);
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
