//  Copyright (c) 2016 Christopher Gassib. All rights reserved.
//

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the KEYFILTER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// KEYFILTER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef KEYFILTER_EXPORTS
#define KEYFILTER_API __declspec(dllexport)
#else
#define KEYFILTER_API __declspec(dllimport)
#endif

// Bit maps of 256 scancodes and virtual keys.
using KeyMap = uint32_t[256u / (sizeof(uint32_t) * 8u)];
using KeyInterceptionCallback = void(*)(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation);

extern "C"
{
    KEYFILTER_API HRESULT Initialize(KeyMap* pInterceptedScancodeMakes, KeyMap* pInterceptedScancodeBreaks,
        KeyMap* pInterceptedVirtualKeyMakes, KeyMap* pInterceptedVirtualKeyBreaks,
        KeyInterceptionCallback interceptedScancodeMake, KeyInterceptionCallback interceptedScancodeBreak,
        KeyInterceptionCallback interceptedVirtualKeyMake, KeyInterceptionCallback interceptedVirtualKeyBreak);

    KEYFILTER_API LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
} // extern "C"
