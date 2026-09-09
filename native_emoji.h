#pragma once

#include <windows.h>
#include <d2d1.h>
#include <dwrite_3.h>
#include <string>

namespace SwashMoji::NativeEmoji {

struct Resources {
    ID2D1Factory* d2d{};
    ID2D1DCRenderTarget* target{};
    IDWriteFactory3* write{};
    Resources() {
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d))) return;
        D2D1_RENDER_TARGET_PROPERTIES properties{};
        properties.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        properties.dpiX = properties.dpiY = 96.0f;
        properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        if (FAILED(d2d->CreateDCRenderTarget(&properties, &target))) return;
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3),
                            reinterpret_cast<IUnknown**>(&write));
    }
    ~Resources() {
        if (write) write->Release();
        if (target) target->Release();
        if (d2d) d2d->Release();
    }
};

inline Resources& Get() { static Resources resources; return resources; }

inline void DrawLine(HDC dc, const RECT& bounds, const std::wstring& text, float size,
                     COLORREF color, bool centered, bool colorEmoji, bool singleLine = false) {
    auto& resources = Get();
    if (!resources.target || !resources.write || FAILED(resources.target->BindDC(dc, &bounds))) {
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, color);
        DrawTextW(dc, text.c_str(), -1, const_cast<RECT*>(&bounds),
                  DT_SINGLELINE | DT_VCENTER | (centered ? DT_CENTER : DT_LEFT) | DT_END_ELLIPSIS);
        return;
    }
    IDWriteTextFormat* format{};
    if (FAILED(resources.write->CreateTextFormat(L"Segoe UI Emoji", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"", &format))) return;
    format->SetTextAlignment(centered ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    if (singleLine) {
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        IDWriteInlineObject* ellipsis{};
        if (SUCCEEDED(resources.write->CreateEllipsisTrimmingSign(format, &ellipsis))) {
            DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
            format->SetTrimming(&trimming, ellipsis);
            ellipsis->Release();
        }
    }
    resources.target->BeginDraw();
    ID2D1SolidColorBrush* brush{};
    const D2D1_COLOR_F value{GetRValue(color) / 255.0f, GetGValue(color) / 255.0f,
                             GetBValue(color) / 255.0f, 1.0f};
    if (SUCCEEDED(resources.target->CreateSolidColorBrush(value, &brush))) {
        const D2D1_RECT_F area{4.0f, 0.0f, static_cast<float>(bounds.right - bounds.left - 4),
                               static_cast<float>(bounds.bottom - bounds.top)};
        resources.target->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), format, area, brush,
            colorEmoji ? D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT : D2D1_DRAW_TEXT_OPTIONS_NONE,
            DWRITE_MEASURING_MODE_NATURAL);
        brush->Release();
    }
    resources.target->EndDraw();
    format->Release();
}

} // namespace SwashMoji::NativeEmoji
