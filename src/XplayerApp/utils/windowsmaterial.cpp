#include "windowsmaterial.h"

#ifdef Q_OS_WIN

#include <windows.h>

namespace {

using DwmSetWindowAttributePtr =
    HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
using SetWindowCompositionAttributePtr =
    BOOL(WINAPI*)(HWND, struct WindowCompositionAttributeData*);

struct AccentPolicy {
    int state;
    int flags;
    DWORD gradientColor;
    int animationId;
};

struct WindowCompositionAttributeData {
    int attribute;
    PVOID data;
    SIZE_T size;
};

constexpr DWORD kDwmwaUseImmersiveDarkMode = 20;
constexpr DWORD kDwmwaSystemBackdropType = 38;
constexpr int kDwmSystemBackdropMainWindow = 2;
constexpr int kWcaAccentPolicy = 19;
constexpr int kAccentEnableAcrylicBlurBehind = 4;

HMODULE dwmApi()
{
    static const HMODULE module = LoadLibraryW(L"dwmapi.dll");
    return module;
}

HMODULE userApi()
{
    static const HMODULE module = LoadLibraryW(L"user32.dll");
    return module;
}

} // namespace

namespace WindowsMaterial
{

bool setDarkMode(WId windowId, bool enabled)
{
    if (!windowId || !dwmApi()) {
        return false;
    }

    static const auto setAttribute =
        reinterpret_cast<DwmSetWindowAttributePtr>(
            GetProcAddress(dwmApi(), "DwmSetWindowAttribute"));
    if (!setAttribute) {
        return false;
    }

    const BOOL value = enabled ? TRUE : FALSE;
    return SUCCEEDED(setAttribute(reinterpret_cast<HWND>(windowId),
                                  kDwmwaUseImmersiveDarkMode, &value,
                                  sizeof(value)));
}

bool applyBackdrop(WId windowId, Kind kind)
{
    if (!windowId) {
        return false;
    }

    HWND hwnd = reinterpret_cast<HWND>(windowId);
    if (kind == Kind::MainWindow && dwmApi()) {
        static const auto setAttribute =
            reinterpret_cast<DwmSetWindowAttributePtr>(
                GetProcAddress(dwmApi(), "DwmSetWindowAttribute"));
        if (setAttribute) {
            const int backdropType = kDwmSystemBackdropMainWindow;
            if (SUCCEEDED(setAttribute(hwnd, kDwmwaSystemBackdropType,
                                       &backdropType,
                                       sizeof(backdropType)))) {
                return true;
            }
        }
    }

    if (!userApi()) {
        return false;
    }

    static const auto setCompositionAttribute =
        reinterpret_cast<SetWindowCompositionAttributePtr>(
            GetProcAddress(userApi(), "SetWindowCompositionAttribute"));
    if (!setCompositionAttribute) {
        return false;
    }

    AccentPolicy policy{};
    policy.state = kAccentEnableAcrylicBlurBehind;
    policy.flags = 2;
    policy.gradientColor = 0xB8202020;

    WindowCompositionAttributeData data{};
    data.attribute = kWcaAccentPolicy;
    data.data = &policy;
    data.size = sizeof(policy);
    return setCompositionAttribute(hwnd, &data) == TRUE;
}

} // namespace WindowsMaterial

#else

namespace WindowsMaterial
{

bool applyBackdrop(WId windowId, Kind kind)
{
    Q_UNUSED(windowId);
    Q_UNUSED(kind);
    return false;
}

bool setDarkMode(WId windowId, bool enabled)
{
    Q_UNUSED(windowId);
    Q_UNUSED(enabled);
    return false;
}

} // namespace WindowsMaterial

#endif
