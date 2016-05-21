//  Copyright (c) 2016 Christopher Gassib. All rights reserved.
//

#include "stdafx.h"
#include "KeyFilter.h"

bool isInitialized = false;

KeyMap* pScancodeMakes;
KeyMap* pVirtualKeyMakes;
KeyMap* pScancodeBreaks;
KeyMap* pVirtualKeyBreaks;

KeyInterceptionCallback InterceptedVirtualKeyMake;
KeyInterceptionCallback InterceptedVirtualKeyBreak;
KeyInterceptionCallback InterceptedScancodeMake;
KeyInterceptionCallback InterceptedScancodeBreak;

///////////////////////////////////////////////
// Bit flag array template functions:

template< typename T, size_t S >
inline void Set(T(&array)[S], const unsigned int index)
{
    const auto WordBitCount = sizeof(T) * 8u;
    const auto BitCountMask = WordBitCount - 1;
    const auto ArraySizeMask = WordBitCount * S - 1;
    const unsigned int clamped = ArraySizeMask & index;
    array[clamped / WordBitCount] |= 0x1 << (BitCountMask & clamped);
}

template< typename T, size_t S >
inline void Clear(T(&array)[S], const unsigned int index)
{
    const auto WordBitCount = sizeof(T) * 8u;
    const auto BitCountMask = WordBitCount - 1;
    const auto ArraySizeMask = WordBitCount * S - 1;
    const unsigned int clamped = ArraySizeMask & index;
    array[clamped / WordBitCount] &= ~(0x1 << (BitCountMask & clamped));
}

template< typename T, size_t S >
inline bool IsSet(T(&array)[S], const unsigned int index)
{
    const auto WordBitCount = sizeof(T) * 8u;
    const auto BitCountMask = WordBitCount - 1;
    const auto ArraySizeMask = WordBitCount * S - 1;
    const unsigned int clamped = ArraySizeMask & index;
    return 0u != (array[clamped / WordBitCount] & (0x1 << (BitCountMask & clamped)));
}

template< typename T, size_t S >
inline void Clear(T(&array)[S])
{
    ::memset(array, 0u, sizeof(array));
}

///////////////////////////////////////////////

inline void FilterOutVirtualKeyMake(const uint_fast16_t virtualKey) { Set(*pVirtualKeyMakes, virtualKey); }
inline void AllowVirtualKeyMake(const uint_fast16_t virtualKey) { Clear(*pVirtualKeyMakes, virtualKey); }
inline bool IsVirtualKeyMakeFiltered(const uint_fast16_t virtualKey) { return IsSet(*pVirtualKeyMakes, virtualKey); }
inline void ClearVirtualKeyMakeFilter() { Clear(*pVirtualKeyMakes); }

inline void FilterOutVirtualKeyBreak(const uint_fast16_t virtualKey) { Set(*pVirtualKeyBreaks, virtualKey); }
inline void AllowVirtualKeyBreak(const uint_fast16_t virtualKey) { Clear(*pVirtualKeyBreaks, virtualKey); }
inline bool IsVirtualKeyBreakFiltered(const uint_fast16_t virtualKey) { return IsSet(*pVirtualKeyBreaks, virtualKey); }
inline void ClearVirtualKeyBreakFilter() { Clear(*pVirtualKeyBreaks); }

inline void FilterOutScancodeMake(const uint_fast16_t scancode) { Set(*pScancodeMakes, scancode); }
inline void AllowScancodeMake(const uint_fast16_t scancode) { Clear(*pScancodeMakes, scancode); }
inline bool IsScancodeMakeFiltered(const uint_fast16_t scancode) { return IsSet(*pScancodeMakes, scancode); }
inline void ClearScancodeMakeFilter() { Clear(*pScancodeMakes); }

inline void FilterOutScancodeBreak(const uint_fast16_t scancode) { Set(*pScancodeBreaks, scancode); }
inline void AllowScancodeBreak(const uint_fast16_t scancode) { Clear(*pScancodeBreaks, scancode); }
inline bool IsScancodeBreakFiltered(const uint_fast16_t scancode) { return IsSet(*pScancodeBreaks, scancode); }
inline void ClearScancodeBreakFilter() { Clear(*pScancodeBreaks); }

///////////////////////////////////////////////

KEYFILTER_API HRESULT Initialize(KeyMap* pInterceptedScancodeMakes, KeyMap* pInterceptedScancodeBreaks,
    KeyMap* pInterceptedVirtualKeyMakes, KeyMap* pInterceptedVirtualKeyBreaks,
    KeyInterceptionCallback interceptedScancodeMake, KeyInterceptionCallback interceptedScancodeBreak,
    KeyInterceptionCallback interceptedVirtualKeyMake, KeyInterceptionCallback interceptedVirtualKeyBreak
    )
{
    if (nullptr == pInterceptedScancodeMakes || nullptr == pInterceptedScancodeBreaks ||
        nullptr == pInterceptedVirtualKeyMakes || nullptr == pInterceptedVirtualKeyBreaks)
    {
        return E_POINTER;
    }

    if (nullptr == interceptedScancodeMake || nullptr == interceptedScancodeBreak ||
        nullptr == interceptedVirtualKeyMake || nullptr == interceptedVirtualKeyBreak)
    {
        return E_POINTER;
    }

    pScancodeMakes = pInterceptedScancodeMakes;
    pScancodeBreaks = pInterceptedScancodeBreaks;
    pVirtualKeyMakes = pInterceptedVirtualKeyMakes;
    pVirtualKeyBreaks = pInterceptedVirtualKeyBreaks;

    InterceptedScancodeMake = interceptedScancodeMake;
    InterceptedScancodeBreak = interceptedScancodeBreak;
    InterceptedVirtualKeyMake = interceptedVirtualKeyMake;
    InterceptedVirtualKeyBreak = interceptedVirtualKeyBreak;

    // TODO: Add memory barrier here; make sure all of those pointers are written.

    isInitialized = true;

    return S_OK;
}

// A code the hook procedure uses to determine how to process the message. If nCode is less than zero,
//  the hook procedure must pass the message to the CallNextHookEx function without further processing
//  and should return the value returned by CallNextHookEx. This parameter can be one of the following
//  values.
//
// If nCode is less than zero, the hook procedure must return the value returned by CallNextHookEx.
//
// If nCode is greater than or equal to zero, and the hook procedure did not process the message, it
//  is highly recommended that you call CallNextHookEx and return the value it returns; otherwise,
//  other applications that have installed WH_KEYBOARD_LL hooks will not receive hook notifications
//  and may behave incorrectly as a result. If the hook procedure processed the message, it may return
//  a nonzero value to prevent the system from passing the message to the rest of the hook chain or
//  the target window procedure.
//
extern "C" KEYFILTER_API LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (!isInitialized)
    {
        return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    // HC_ACTION (0) The wParam and lParam parameters contain information about a keyboard message.
    if (nCode != HC_ACTION)
    {
        return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    if (nullptr == reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam))
    {
        return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    const auto& kbDllHookStruct = *reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);

    const auto vkCode = kbDllHookStruct.vkCode;
    const auto scancode = kbDllHookStruct.scanCode;
    const auto flags = kbDllHookStruct.flags;
    const auto dwExtraInfo = kbDllHookStruct.dwExtraInfo;

    const auto extendedKey = 0 != (LLKHF_EXTENDED & flags);
    //const auto lowIntegrityInjection = 0 != (LLKHF_LOWER_IL_INJECTED & flags);
    //const auto injection = 0 != (LLKHF_INJECTED & flags);
    //const auto altDown = 0 != (LLKHF_ALTDOWN & flags);
    //const auto keyBreaking = 0 != (LLKHF_UP & flags);

    switch (wParam)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (IsVirtualKeyMakeFiltered(vkCode))
        {
            InterceptedVirtualKeyMake(vkCode, scancode, extendedKey, static_cast<DWORD>(dwExtraInfo));
            return 1;
        }
        if (IsScancodeMakeFiltered(scancode))
        {
            InterceptedScancodeMake(vkCode, scancode, extendedKey, static_cast<DWORD>(dwExtraInfo));
            return 1;
        }
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (IsVirtualKeyBreakFiltered(vkCode))
        {
            InterceptedVirtualKeyBreak(vkCode, scancode, extendedKey, static_cast<DWORD>(dwExtraInfo));
            return 1;
        }
        if (IsScancodeBreakFiltered(scancode))
        {
            InterceptedScancodeBreak(vkCode, scancode, extendedKey, static_cast<DWORD>(dwExtraInfo));
            return 1;
        }
        break;
    default:
        break;
    }

    return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
}
