//  Copyright (c) 2016 Christopher Gassib. All rights reserved.
//

#include "stdafx.h"
#include "UberKey.h"

#include <fstream>
#include <iostream>
#include <cassert>

using std::exception;
using std::bad_alloc;
using std::logic_error;
using std::runtime_error;
using std::bad_alloc;
using std::array;
using std::vector;
using std::unordered_map;
using std::string;
using std::wstring;
using std::stringstream;
using std::move;
using std::endl;
using std::min;
using std::max;
using std::numeric_limits;

// Worth looking at later:
//  ToUnicode() function for interpreting input from the keyboard as unicode.
//  https://msdn.microsoft.com/en-us/library/windows/desktop/ms646320%28v=vs.85%29.aspx
//
//  Along the same lines, currently not sure how to simulate unicode input from the SendInput() function.
//
// LowLevelKeyboardProc() callback function for filtering out key presses.
//  https://msdn.microsoft.com/en-us/library/windows/desktop/ms644985%28v=vs.85%29.aspx
//  http://www.codeproject.com/Articles/1264/KeyBoard-Hooks
//  https://msdn.microsoft.com/en-us/library/windows/desktop/ms644991%28v=vs.85%29.aspx
//
// GetKeyNameText()
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms646300%28v=vs.85%29.aspx
//
// VkKeyScanEx()
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms646332%28v=vs.85%29.aspx
//
// MapVirtualKeyEx()
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms646307%28v=vs.85%29.aspx
//

struct VirtualKeyMeta
{
    const char* name;
    const char* info;
    const char** altNames;

    VirtualKeyMeta(const char* name, const char* info, const char* altNames[])
        : name(name), info(info), altNames(altNames)
    {
    }
};

const char* altNames0x15[] = { "hangul", "hangeul", nullptr };
const char* altNames0x19[] = { "hanja", nullptr };
const char* altNames0x92[] = { "oem_nec_equal", nullptr };
const size_t altNameCount = ((sizeof(altNames0x15) + sizeof(altNames0x19) + sizeof(altNames0x92)) / sizeof(char*)) - 3;

static const VirtualKeyMeta virtualKeys[256] =
{
    { "", "", nullptr }, // 0x00
    { "lbutton", "", nullptr }, // 0x01
    { "rbutton", "", nullptr }, // 0x02
    { "cancel", "", nullptr }, // 0x03
    { "mbutton", "not contiguous with l & rbutton", nullptr }, // 0x04
    { "xbutton1", "not contiguous with l & rbutton", nullptr }, // 0x05
    { "xbutton2", "not contiguous with l & rbutton", nullptr }, // 0x06
    { "", "unassigned", nullptr }, // 0x07
    { "back", "", nullptr }, // 0x08
    { "tab", "", nullptr }, // 0x09
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, // 0x0a - 0x0b
    { "clear", "", nullptr }, // 0x0c
    { "return", "", nullptr }, // 0x0d
    { "", "", nullptr }, { "", "", nullptr }, // 0x0e - 0x0f
    { "shift", "", nullptr }, // 0x10
    { "control", "", nullptr }, // 0x11
    { "menu", "", nullptr }, // 0x12
    { "pause", "", nullptr }, // 0x13
    { "capital", "", nullptr }, // 0x14
    { "kana", "Japanese and Korean versions are different", altNames0x15 }, // 0x15
    { "", "", nullptr }, // 0x16
    { "junja", "", nullptr }, // 0x17
    { "final", "", nullptr }, // 0x18
    { "kanji", "Japanese and Korean versions are different", altNames0x19 }, // 0x19
    { "", "", nullptr }, // 0x1a
    { "escape", "", nullptr }, // 0x1b
    { "convert", "", nullptr }, // 0x1c
    { "nonconvert", "", nullptr }, // 0x1d
    { "accept", "", nullptr }, // 0x1e
    { "modechange", "", nullptr }, // 0x1f
    { "space", "", nullptr }, // 0x20
    { "prior", "", nullptr }, // 0x21
    { "next", "", nullptr }, // 0x22
    { "end", "", nullptr }, // 0x23
    { "home", "", nullptr }, // 0x24
    { "left", "", nullptr }, // 0x25
    { "up", "", nullptr }, // 0x26
    { "right", "", nullptr }, // 0x27
    { "down", "", nullptr }, // 0x28
    { "select", "", nullptr }, // 0x29
    { "print", "", nullptr }, // 0x2a
    { "execute", "", nullptr }, // 0x2b
    { "snapshot", "", nullptr }, // 0x2c
    { "insert", "", nullptr }, // 0x2d
    { "delete", "", nullptr }, // 0x2e
    { "help", "", nullptr }, // 0x2f
    { "_0", "same as ASCII '0'", nullptr }, // 0x30
    { "_1", "same as ASCII '1'", nullptr }, // 0x31
    { "_2", "same as ASCII '2'", nullptr }, // 0x32
    { "_3", "same as ASCII '3'", nullptr }, // 0x33
    { "_4", "same as ASCII '4'", nullptr }, // 0x34
    { "_5", "same as ASCII '5'", nullptr }, // 0x35
    { "_6", "same as ASCII '6'", nullptr }, // 0x36
    { "_7", "same as ASCII '7'", nullptr }, // 0x37
    { "_8", "same as ASCII '8'", nullptr }, // 0x38
    { "_9", "same as ASCII '9'", nullptr }, // 0x39
    { "", "", nullptr }, { "", "", nullptr }, { "", "", nullptr }, // 0x3a - 0x3f
    { "", "", nullptr }, { "", "", nullptr }, { "", "", nullptr },
    { "", "unassigned", nullptr }, // 0x40
    { "a", "same as ASCII 'A'", nullptr }, // 0x41
    { "b", "same as ASCII 'B'", nullptr }, // 0x42
    { "c", "same as ASCII 'C'", nullptr }, // 0x43
    { "d", "same as ASCII 'D'", nullptr }, // 0x44
    { "e", "same as ASCII 'E'", nullptr }, // 0x45
    { "f", "same as ASCII 'F'", nullptr }, // 0x46
    { "g", "same as ASCII 'G'", nullptr }, // 0x47
    { "h", "same as ASCII 'H'", nullptr }, // 0x48
    { "i", "same as ASCII 'I'", nullptr }, // 0x49
    { "j", "same as ASCII 'J'", nullptr }, // 0x4a
    { "k", "same as ASCII 'K'", nullptr }, // 0x4b
    { "l", "same as ASCII 'L'", nullptr }, // 0x4c
    { "m", "same as ASCII 'M'", nullptr }, // 0x4d
    { "n", "same as ASCII 'N'", nullptr }, // 0x4e
    { "o", "same as ASCII 'O'", nullptr }, // 0x4f
    { "p", "same as ASCII 'P'", nullptr }, // 0x50
    { "q", "same as ASCII 'Q'", nullptr }, // 0x51
    { "r", "same as ASCII 'R'", nullptr }, // 0x52
    { "s", "same as ASCII 'S'", nullptr }, // 0x53
    { "t", "same as ASCII 'T'", nullptr }, // 0x54
    { "u", "same as ASCII 'U'", nullptr }, // 0x55
    { "v", "same as ASCII 'V'", nullptr }, // 0x56
    { "w", "same as ASCII 'W'", nullptr }, // 0x57
    { "x", "same as ASCII 'X'", nullptr }, // 0x58
    { "y", "same as ASCII 'Y'", nullptr }, // 0x59
    { "z", "same as ASCII 'Z'", nullptr }, // 0x5a
    { "lwin", "", nullptr }, // 0x5b
    { "rwin", "", nullptr }, // 0x5c
    { "apps", "", nullptr }, // 0x5d
    { "", "reserved", nullptr }, // 0x5e
    { "sleep", "", nullptr }, // 0x5f
    { "numpad0", "", nullptr }, // 0x60
    { "numpad1", "", nullptr }, // 0x61
    { "numpad2", "", nullptr }, // 0x62
    { "numpad3", "", nullptr }, // 0x63
    { "numpad4", "", nullptr }, // 0x64
    { "numpad5", "", nullptr }, // 0x65
    { "numpad6", "", nullptr }, // 0x66
    { "numpad7", "", nullptr }, // 0x67
    { "numpad8", "", nullptr }, // 0x68
    { "numpad9", "", nullptr }, // 0x69
    { "multiply", "", nullptr }, // 0x6a
    { "add", "", nullptr }, // 0x6b
    { "separator", "", nullptr }, // 0x6c
    { "subtract", "", nullptr }, // 0x6d
    { "decimal", "", nullptr }, // 0x6e
    { "divide", "", nullptr }, // 0x6f
    { "f1", "", nullptr }, // 0x70
    { "f2", "", nullptr }, // 0x71
    { "f3", "", nullptr }, // 0x72
    { "f4", "", nullptr }, // 0x73
    { "f5", "", nullptr }, // 0x74
    { "f6", "", nullptr }, // 0x75
    { "f7", "", nullptr }, // 0x76
    { "f8", "", nullptr }, // 0x77
    { "f9", "", nullptr }, // 0x78
    { "f10", "", nullptr }, // 0x79
    { "f11", "", nullptr }, // 0x7a
    { "f12", "", nullptr }, // 0x7b
    { "f13", "", nullptr }, // 0x7c
    { "f14", "", nullptr }, // 0x7d
    { "f15", "", nullptr }, // 0x7e
    { "f16", "", nullptr }, // 0x7f
    { "f17", "", nullptr }, // 0x80
    { "f18", "", nullptr }, // 0x81
    { "f19", "", nullptr }, // 0x82
    { "f20", "", nullptr }, // 0x83
    { "f21", "", nullptr }, // 0x84
    { "f22", "", nullptr }, // 0x85
    { "f23", "", nullptr }, // 0x86
    { "f24", "", nullptr }, // 0x87
    { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, // 0x88 - 0x8f
    { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, { "", "unassigned", nullptr },
    { "", "unassigned", nullptr }, { "", "unassigned", nullptr },
    { "numlock", "", nullptr }, // 0x90
    { "scroll", "", nullptr }, // 0x91
    { "oem_fj_jisho", "Fujitsu/OASYS 'dictionary' key; NEC PC-9800 '=' key on numpad", altNames0x92 }, // 0x92
    { "oem_fj_masshou", "Fujitsu/OASYS 'unregister word' key", nullptr }, // 0x93
    { "oem_fj_touroku", "Fujitsu/OASYS 'register word' key", nullptr }, // 0x94
    { "oem_fj_loya", "Fujitsu/OASYS 'left oyayubi' key", nullptr }, // 0x95
    { "oem_fj_roya", "Fujitsu/OASYS 'right oyayubi' key", nullptr }, // 0x96
    { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, // 0x97 - 0x9f
    { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, { "", "unassigned", nullptr },
    { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, { "", "unassigned", nullptr },
    { "lshift", "left Shift; Used only as parameters to GetAsyncKeyState() and GetKeyState(). No other API or message will distinguish left and right keys in this way.", nullptr }, // 0xa0
    { "rshift", "right Shift; Used only as parameters to GetAsyncKeyState() and GetKeyState(). No other API or message will distinguish left and right keys in this way.", nullptr }, // 0xa1
    { "lcontrol", "left Ctrl; Used only as parameters to GetAsyncKeyState() and GetKeyState(). No other API or message will distinguish left and right keys in this way.", nullptr }, // 0xa2
    { "rcontrol", "right Ctrl; Used only as parameters to GetAsyncKeyState() and GetKeyState(). No other API or message will distinguish left and right keys in this way.", nullptr }, // 0xa3
    { "lmenu", "left Alt; Used only as parameters to GetAsyncKeyState() and GetKeyState(). No other API or message will distinguish left and right keys in this way.", nullptr }, // 0xa4
    { "rmenu", "right Alt; Used only as parameters to GetAsyncKeyState() and GetKeyState(). No other API or message will distinguish left and right keys in this way.", nullptr }, // 0xa5
    { "browser_back", "", nullptr }, // 0xa6
    { "browser_forward", "", nullptr }, // 0xa7
    { "browser_refresh", "", nullptr }, // 0xa8
    { "browser_stop", "", nullptr }, // 0xa9
    { "browser_search", "", nullptr }, // 0xaa
    { "browser_favorites", "", nullptr }, // 0xab
    { "browser_home", "", nullptr }, // 0xac
    { "volume_mute", "", nullptr }, // 0xad
    { "volume_down", "", nullptr }, // 0xae
    { "volume_up", "", nullptr }, // 0xaf
    { "media_next_track", "", nullptr }, // 0xb0
    { "media_prev_track", "", nullptr }, // 0xb1
    { "media_stop", "", nullptr }, // 0xb2
    { "media_play_pause", "", nullptr }, // 0xb3
    { "launch_mail", "", nullptr }, // 0xb4
    { "launch_media_select", "", nullptr }, // 0xb5
    { "launch_app1", "", nullptr }, // 0xb6
    { "launch_app2", "", nullptr }, // 0xb7
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, // 0xb8 - 0xb9
    { "oem_1", "';:' for us", nullptr }, // 0xba
    { "oem_plus", "'+' any country", nullptr }, // 0xbb
    { "oem_comma", "',' any country", nullptr }, // 0xbc
    { "oem_minus", "'-' any country", nullptr }, // 0xbd
    { "oem_period", "'.' any country", nullptr }, // 0xbe
    { "oem_2", "'/?' for us", nullptr }, // 0xbf
    { "oem_3", "'`~' for us", nullptr }, // 0xc0
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, { "", "reserved", nullptr }, // 0xc1 - 0xd7
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, { "", "reserved", nullptr },
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, { "", "reserved", nullptr },
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, { "", "reserved", nullptr },
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, { "", "reserved", nullptr },
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, { "", "reserved", nullptr },
    { "", "reserved", nullptr }, { "", "reserved", nullptr }, { "", "reserved", nullptr },
    { "", "reserved", nullptr }, { "", "reserved", nullptr },
    { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, { "", "unassigned", nullptr }, // 0xd8 - 0xda
    { "oem_4", "'[{' for us", nullptr }, // 0xdb
    { "oem_5", "'\\|' for us", nullptr }, // 0xdc
    { "oem_6", "']}' for us", nullptr }, // 0xdd
    { "oem_7", "''\"' for us", nullptr }, // 0xde
    { "oem_8", "", nullptr }, // 0xdf
    { "", "reserved", nullptr }, // 0xe0
    { "oem_ax", "Various extended or enhanced keyboards; 'ax' key on japanese ax kbd", nullptr }, // 0xe1
    { "oem_102", "Various extended or enhanced keyboards; \"<>\" or \"\\|\" on rt 102-key kbd.", nullptr }, // 0xe2
    { "ico_help", "Various extended or enhanced keyboards; help key on ico", nullptr }, // 0xe3
    { "ico_00", "Various extended or enhanced keyboards; 00 key on ico", nullptr }, // 0xe4
    { "processkey", "", nullptr }, // 0xe5
    { "ico_clear", "", nullptr }, // 0xe6
    { "packet", "", nullptr }, // 0xe7
    { "", "unassigned", nullptr }, // 0xe8
    { "oem_reset", "Nokia/Ericsson", nullptr }, // 0xe9
    { "oem_jump", "Nokia/Ericsson", nullptr }, // 0xea
    { "oem_pa1", "Nokia/Ericsson", nullptr }, // 0xeb
    { "oem_pa2", "Nokia/Ericsson", nullptr }, // 0xec
    { "oem_pa3", "Nokia/Ericsson", nullptr }, // 0xed
    { "oem_wsctrl", "Nokia/Ericsson", nullptr }, // 0xee
    { "oem_cusel", "Nokia/Ericsson", nullptr }, // 0xef
    { "oem_attn", "Nokia/Ericsson", nullptr }, // 0xf0
    { "oem_finish", "Nokia/Ericsson", nullptr }, // 0xf1
    { "oem_copy", "Nokia/Ericsson", nullptr }, // 0xf2
    { "oem_auto", "Nokia/Ericsson", nullptr }, // 0xf3
    { "oem_enlw", "Nokia/Ericsson", nullptr }, // 0xf4
    { "oem_backtab", "Nokia/Ericsson", nullptr }, // 0xf5
    { "attn", "", nullptr }, // 0xf6
    { "crsel", "", nullptr }, // 0xf7
    { "exsel", "", nullptr }, // 0xf8
    { "ereof", "", nullptr }, // 0xf9
    { "play", "", nullptr }, // 0xfa
    { "zoom", "", nullptr }, // 0xfb
    { "noname", "", nullptr }, // 0xfc
    { "pa1", "", nullptr }, // 0xfd
    { "oem_clear", "", nullptr }, // 0xfe
    { "", "reserved", nullptr }  // 0xff
};

const auto virtualKeyCount = sizeof(virtualKeys) / sizeof(virtualKeys[0]);


// Types
//////////////////////////////////////////////////////////////////

struct Sizeui {
    unsigned int width;
    unsigned int height;
};

//struct KeyPair
//{
//    uint_fast16_t   scancode;       // OEM hardware code ("device-dependent identifier")
//    uint_fast8_t    virtualKey; // common name of the key
//};

using WindowMessageHandler = LRESULT(*)(WPARAM, LPARAM);
using MessageMap = unordered_map<UINT, WindowMessageHandler>;

// Bit maps of 256 scancodes and virtual keys.
using KeyMap = uint32_t[256u / (sizeof(uint32_t) * 8u)];

using KeyInterceptionCallback = void(*)(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation);

// Implementation Data
//////////////////////////////////////////////////////////////////
// Class constants.
const WCHAR WindowClassName[]   = L"MAIN_WINDOW";
const DWORD WindowStyle         = WS_POPUP | WS_CAPTION |
                                  WS_SYSMENU | WS_MINIMIZEBOX;
const DWORD WindowExtendedStyle = 0;

// Windows message handler functions.
MessageMap      messageMap;

HINSTANCE               _moduleHandle = nullptr;
HWND                    _windowHandle = nullptr;

///////////////////////////////////////////////

//wstring                 _filteredCharacters; // TODO: Remove this.
vector<uint8_t>         _rawInputBuffer;
bool                    _isReadingRawKeyboard;

// Maps of the currently depressed keys.
KeyMap madeScancodes = {};
KeyMap madeVirtualKeys = {};
KeyMap latchedScancodeMakes = {};
KeyMap latchedVirtualKeyMakes = {};
KeyMap latchedScancodeBreaks = {};
KeyMap latchedVirtualKeyBreaks = {};
KeyMap interceptedScancodeMakes = {};
KeyMap interceptedVirtualKeyMakes = {};
KeyMap interceptedScancodeBreaks = {};
KeyMap interceptedVirtualKeyBreaks = {};

///////////////////////////////////////////////

lua_State* luaState = nullptr;
vector<uint8_t> luaScriptBuffer;
vector<uint8_t>::size_type luaScriptBufferReadPos;
///////////////////////////////////////////////

// Bit flag array template functions:

template< typename T, size_t S >
inline void Set(T (&array)[S], const unsigned int index)
{
    const auto WordBitCount = sizeof(T) * 8u;
    const auto BitCountMask = WordBitCount - 1;
    const auto ArraySizeMask = WordBitCount * S - 1;
    const unsigned int clamped = ArraySizeMask & index;
    array[clamped / WordBitCount] |= 0x1 << (BitCountMask & clamped);
}

template< typename T, size_t S >
inline void Clear(T (&array)[S], const unsigned int index)
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

inline void MakeVirtualKey(const uint_fast16_t virtualKey) { Set(madeVirtualKeys, virtualKey); }
inline void BreakVirtualKey(const uint_fast16_t virtualKey) { Clear(madeVirtualKeys, virtualKey); }
inline bool IsVirtualKeyMade(const uint_fast16_t virtualKey) { return IsSet(madeVirtualKeys, virtualKey); }
inline void ClearVirtualKeys() { Clear(madeVirtualKeys); }

inline void MakeScancode(const uint_fast16_t scancode) { Set(madeScancodes, scancode); }
inline void BreakScancode(const uint_fast16_t scancode) { Clear(madeScancodes, scancode); }
inline bool IsScancodeMade(const uint_fast16_t scancode) { return IsSet(madeScancodes, scancode); }
inline void ClearScancodes() { Clear(madeScancodes); }

inline void LatchScancodeMake(const uint_fast16_t scancode) { Set(latchedScancodeMakes, scancode); }
inline void UnlatchScancodeMake(const uint_fast16_t scancode) { Clear(latchedScancodeMakes, scancode); }
inline bool IsScancodeMakeLatched(const uint_fast16_t scancode) { return IsSet(latchedScancodeMakes, scancode); }
inline void ClearScancodeMakeLatches() { Clear(latchedScancodeMakes); }

inline void LatchVirtualKeyMake(const uint_fast16_t virtualKey) { Set(latchedVirtualKeyMakes, virtualKey); }
inline void UnlatchVirtualKeyMake(const uint_fast16_t virtualKey) { Clear(latchedVirtualKeyMakes, virtualKey); }
inline bool IsVirtualKeyMakeLatched(const uint_fast16_t virtualKey) { return IsSet(latchedVirtualKeyMakes, virtualKey); }
inline void ClearVirtualKeyMakeLatches() { Clear(latchedVirtualKeyMakes); }

inline void LatchScancodeBreak(const uint_fast16_t scancode) { Set(latchedScancodeBreaks, scancode); }
inline void UnlatchScancodeBreak(const uint_fast16_t scancode) { Clear(latchedScancodeBreaks, scancode); }
inline bool IsScancodeBreakLatched(const uint_fast16_t scancode) { return IsSet(latchedScancodeBreaks, scancode); }
inline void ClearScancodeBreakLatches() { Clear(latchedScancodeBreaks); }

inline void LatchVirtualKeyBreak(const uint_fast16_t virtualKey) { Set(latchedVirtualKeyBreaks, virtualKey); }
inline void UnlatchVirtualKeyBreak(const uint_fast16_t virtualKey) { Clear(latchedVirtualKeyBreaks, virtualKey); }
inline bool IsVirtualKeyBreakLatched(const uint_fast16_t virtualKey) { return IsSet(latchedVirtualKeyBreaks, virtualKey); }
inline void ClearVirtualKeyBreakLatches() { Clear(latchedVirtualKeyBreaks); }

///////////////////////////////////////////////

wstring GetProgramExecutablePath()
{
    vector<wchar_t> buffer(MAX_PATH);

    for (int i = 0; i < 5; i++)
    {
        const auto length = ::GetModuleFileNameW(nullptr, &buffer[0], static_cast<DWORD>(buffer.size()));
        if (0 == length)
        {
            throw exception("failed to get a path to this program's executable");
        }
        else if (buffer.size() == length && ERROR_INSUFFICIENT_BUFFER == ::GetLastError())
        {
            buffer.resize(buffer.size() * 2);
            continue;
        }

        wstring result(&buffer[0], &buffer[length]);

        auto bsi = result.find_last_of(L'\\');
        result.resize(bsi + 1);

        return result;
    }

    throw exception("failed to get a reasonable path to this program's executable");
}

const char* LuaReader(lua_State* L, void* data, size_t* size)
{
    UNREFERENCED_PARAMETER(L);
    UNREFERENCED_PARAMETER(data);

    bool isEof = false;

    if (luaScriptBuffer.empty())
    {
        isEof = true;
    }
    else if (luaScriptBufferReadPos >= luaScriptBuffer.size())
    {
        luaScriptBuffer.clear();
        isEof = true;
    }

    if (isEof)
    {
        if (nullptr != size)
        {
            *size = 0u;
        }
        return nullptr;
    }

    if (nullptr == size)
    {
        return nullptr;
    }

    const auto pos = luaScriptBufferReadPos;

    luaScriptBufferReadPos = luaScriptBuffer.size();

    *size = luaScriptBuffer.size() - pos;

    return reinterpret_cast<const char*>(&luaScriptBuffer[pos]);
}

void BackgroundApplicationProcessing()
{
    //std::cout << std::hex;
    //for (auto i = 0; i < 256; i++)
    //{
    //    if (IsKeyMade(i))
    //    {
    //        std::cout << std::hex << i << ' ';
    //    }
    //}
    //std::cout << std::dec;

    ClearScancodes();
    ClearVirtualKeys();
}

void Close()
{
    ::PostMessageW(_windowHandle, WM_CLOSE, 0, 0);
}

LRESULT Paint(WPARAM wParam, LPARAM lParam)
{
    //pGraphicsRenderer->process_wm_paint();
    return ::DefWindowProcW(_windowHandle, WM_PAINT, wParam, lParam);
}

string LuaTypeToString(lua_State* L, int stackIndex)
{
    string result;
    stringstream ss;

    // if (the stack index is relative) convert it to an absolute index
    const auto absIndex = (stackIndex < 0) ? lua_gettop(L) + 1 + stackIndex : stackIndex;

    const auto typeId = lua_type(L, absIndex);

    switch (typeId)
    {
    case LUA_TBOOLEAN:
        ss << ((lua_toboolean(L, absIndex)) ? "true" : "false");
        break;
    case LUA_TFUNCTION:
        if (lua_iscfunction(L, absIndex))
        {
            ss << "Native function: 0x" << std::hex << lua_tocfunction(L, absIndex) << std::dec;
        }
        else
        {
            ss << "Lua function";
        }
        break;
    case LUA_TLIGHTUSERDATA:
        ss << "Light user-data: 0x" << std::hex << lua_touserdata(L, absIndex) << std::dec;
        break;
    case LUA_TNIL:
        ss << "nil";
        break;
    case LUA_TNUMBER:
        ss << lua_tonumber(L, absIndex);
        break;
    case LUA_TSTRING:
        ss << '"' << lua_tostring(L, absIndex) << '"'; // NOTE: this ignores the possibility of embedded nulls
        break;
    case LUA_TTABLE:
        ss << "{ ";

        lua_pushnil(L); // prime the pump: push the first table key to start the enumeration
        if (0 != lua_next(L, absIndex))
        {
            ss << LuaTypeToString(L, -2) << " = " << LuaTypeToString(L, -1); // print key = value

            lua_pop(L, 1); // pop the value; leave the key for the next iteration

            while (0 != lua_next(L, absIndex))
            {
                ss << ", " << LuaTypeToString(L, -2) << " = " << LuaTypeToString(L, -1); // print the key = value

                lua_pop(L, 1); // pop the value; leave the key for the next iteration
            }
        }

        ss << " }";
        break;
    case LUA_TTHREAD:
        ss << "Lua thread: 0x" << std::hex << lua_tothread(L, absIndex) << std::dec;
        break;
    case LUA_TUSERDATA:
        ss << "User-data block: 0x" << std::hex << lua_touserdata(L, absIndex) << std::dec;
        break;
    default:
        ss << "unrecognized-type";
        break;
    }

    result = move(ss.str());

    return result;
}

int LuaDumpStack(lua_State* L)
{
    const auto argc = lua_gettop(L);

    if (0 == argc)
    {
        std::wcout << "\nstack is empty" << endl;
        return 0;
    }

    std::wcout << L"\nStack Dump" << endl;
    std::wcout << L"----------" << endl;

    for (auto i = argc; i >= 1; i--)
    {
        std::cout << i << ": " << LuaTypeToString(L, i) << endl;
    }

    return argc;
}

int LuaPrintReplacement(lua_State* L)
{
    const auto topIndex = lua_gettop(L);

    for (int i = 1; i <= topIndex; i++)
    {
        if (lua_isstring(L, i)) // if (the object is a string or a number that's easily converted to a string)
        {
            std::cout << lua_tostring(L, i);
        }
        else // else (see if the object has a __tostring() method)
        {
            if (0 == luaL_getmetafield(L, i, "__tostring")) // if (the object doesn't have a __tostring() method)
            {
                std::cout << LuaTypeToString(L, i);
            }
            else if (LUA_TFUNCTION != lua_type(L, lua_gettop(L))) // if (the object's __tostring member isn't a legal function)
            {
                // NOTE: This would be a strange case.
                lua_pop(L, 1);
                std::cout << LuaTypeToString(L, i);
            }
            else // else (the __tostring() method is on the stack)
            {
                lua_pushvalue(L, i); // duplicate the object to invoke tostring() on
                const auto result = lua_pcall(L, 1, LUA_MULTRET, 0); // call __tostring(obj)
                if (LUA_ERRRUN == result)
                {
                    std::wcout << "Lua runtime error." << std::endl;
                }
                else if (LUA_ERRMEM == result)
                {
                    std::wcout << "Lua memory allocation error." << std::endl;
                }
                else if (LUA_ERRERR == result)
                {
                    std::wcout << "Lua error while running the error handler function." << std::endl;
                }
                else if (0 != result)
                {
                    std::wcout << "Lua unknown error." << std::endl;
                }

                // Output the result of __tostring(obj)
                std::cout << lua_tostring(L, lua_gettop(L));

                // Pop the string to restore the stack.
                lua_pop(L, 1);
            }
        }
    }

    std::wcout << std::endl;

    return 0;
}

// NOTE: These declarations are needed by hook::InstallLowLevelKeyboardHook().
namespace api
{
    void InterceptedScancodeMakeHander(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation);
    void InterceptedScancodeBreakHander(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation);
    void InterceptedVirtualKeyMakeHander(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation);
    void InterceptedVirtualKeyBreakHander(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation);
} // namespace api

// Hook Procedure
namespace hook
{
    class LowLevelKeyboardHook final
    {
    public:
        LowLevelKeyboardHook()
            : _hLowLevelKeyboardHook(nullptr)
        {
        }

        explicit LowLevelKeyboardHook(const HHOOK hLowLevelKeyboardHook)
            : _hLowLevelKeyboardHook(hLowLevelKeyboardHook)
        {
        }

        ~LowLevelKeyboardHook()
        {
            reset();
        }

        explicit operator bool() const
        {
            return nullptr != _hLowLevelKeyboardHook;
        }

        operator const HHOOK&() const
        {
            return _hLowLevelKeyboardHook;
        }

        void reset(const HHOOK hLowLevelKeyboardHook = nullptr)
        {
            if (nullptr != _hLowLevelKeyboardHook)
            {
                const auto result = ::UnhookWindowsHookEx(_hLowLevelKeyboardHook);
                if (0 == result)
                {
                    const auto lastError = ::GetLastError();
                    std::wcout << L"failed to unhook low-level keyboard filter: 0x" << std::hex << lastError << std::dec << std::endl;
                    assert(false);
                }
            }

            _hLowLevelKeyboardHook = hLowLevelKeyboardHook;
        }

    private:
        HHOOK _hLowLevelKeyboardHook;

        LowLevelKeyboardHook(const LowLevelKeyboardHook&) = delete;
        LowLevelKeyboardHook& operator =(const LowLevelKeyboardHook&) = delete;
    };

    LowLevelKeyboardHook keyboardHook;

    void InstallLowLevelKeyboardHook()
    {
        HMODULE hDllModule;
        {
            hDllModule = ::LoadLibraryExW(L"KeyFilter.dll", nullptr, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
            if (nullptr == hDllModule)
            {
                std::wcout << L"LoadLibrary failed: 0x" << std::hex << ::GetLastError() << std::dec << std::endl;
                return;
            }
        }

        auto freeLibrary = [hDllModule]()
        {
            const auto result = ::FreeLibrary(hDllModule);
            if (0 == result)
            {
                std::wcout << L"failed to free hook library 0x" << std::hex << ::GetLastError() << std::dec << std::endl;
            }
        };

        using InitializeFilterHooks_t = HRESULT (*)(KeyMap* pInterceptedScancodeMakes, KeyMap* pInterceptedScancodeBreaks,
            KeyMap* pInterceptedVirtualKeyMakes, KeyMap* pInterceptedVirtualKeyBreaks,
            KeyInterceptionCallback interceptedScancodeMake, KeyInterceptionCallback interceptedScancodeBreak,
            KeyInterceptionCallback interceptedVirtualKeyMake, KeyInterceptionCallback interceptedVirtualKeyBreak);

        InitializeFilterHooks_t InitializeFilterHooks;
        {
            InitializeFilterHooks = reinterpret_cast<InitializeFilterHooks_t>(::GetProcAddress(hDllModule, "Initialize"));
            if (nullptr == InitializeFilterHooks)
            {
                std::wcout << L"GetProcAddress failed: 0x" << std::hex << ::GetLastError() << std::dec << std::endl;

                freeLibrary();

                return;
            }
        }

        {
            const auto hr = InitializeFilterHooks(&interceptedScancodeMakes, &interceptedScancodeBreaks,
                &interceptedVirtualKeyMakes, &interceptedVirtualKeyBreaks,
                &api::InterceptedScancodeMakeHander, &api::InterceptedScancodeBreakHander,
                &api::InterceptedVirtualKeyMakeHander, &api::InterceptedVirtualKeyBreakHander);
            if (FAILED(hr))
            {
                std::wcout << L"InitializeFilterHooks failed" << std::endl;

                assert(false);

                freeLibrary();

                return;
            }
        }

        HOOKPROC pHookFunction;
        {
            pHookFunction = reinterpret_cast<HOOKPROC>(::GetProcAddress(hDllModule, "LowLevelKeyboardProc"));
            if (nullptr == pHookFunction)
            {
                std::wcout << L"GetProcAddress failed: 0x" << std::hex << ::GetLastError() << std::dec << std::endl;

                freeLibrary();

                return;
            }
        }

        keyboardHook.reset(::SetWindowsHookExW(WH_KEYBOARD_LL, pHookFunction, hDllModule, 0));

        if (!keyboardHook)
        {
            std::wcout << L"SetWindowsHook failed: 0x" << std::hex << ::GetLastError() << std::dec << std::endl;

            freeLibrary();

            return;
        }
    }

    void DisableLowLevelKeyboardHook()
    {
        keyboardHook.reset();
        // NOTE: It's not possible to unload the DLL after the hook has been made.
    }
} // namespace hook

namespace api
{
    template<const char* const CodeTypename>
    inline uint_fast16_t CheckCodeArgumentFromLua(lua_State* L, int argumentIndex)
    {
        uint_fast16_t result;

        const auto code = luaL_checkinteger(L, argumentIndex);
        if (code < 0 || code > numeric_limits<uint16_t>::max())
        {
            luaL_error(L, "%s (%p) is out of range", CodeTypename, code);
        }

        result = static_cast<uint_fast16_t>(code);
        return result;
    }

    template<
        typename T,
        T& bitmap,
        const char* const Typename,
        const char* const MetatableTypename,
        const char* const Luaname
    >
    class CodeTable
    {
    public:

        static int SetIndex(lua_State* L)
        {
            luaL_error(L, "%s table is not user writable", Typename); // Does a long jump; never returns.

            // NOTE: luaL_error() prevents this code from executing.
            return 0;
        }

        static int Index(lua_State* L)
        {
            (void)luaL_checkudata(L, 1, MetatableTypename);
            const auto code = CheckCodeArgumentFromLua<Typename>(L, 2);

            const auto isMade = IsSet(bitmap, code);
            lua_pushboolean(L, isMade);
            lua_replace(L, 2);
            return 1;
        }

        static int Length(lua_State* L)
        {
            const auto totalBitCount = sizeof(bitmap) * 8u;
            lua_pushinteger(L, totalBitCount);
            return 1;
        }

        static int ToString(lua_State* L)
        {
            string table;

            {
                string result;

                const auto LineLength = 16u;

                const auto wordSize = sizeof(bitmap[0]);
                const auto wordCount = sizeof(bitmap) / wordSize;
                const auto wordBitCount = wordSize * 8u;

                stringstream s;
                s << std::hex;

                auto key = 0u;
                for (auto w = 0; w < wordCount; w++)
                {
                    const auto word = bitmap[w];
                    for (auto b = 0; b < wordBitCount; b++, key++)
                    {
                        const auto isMade = 0u != (word & (0x1 << b));
                        if (isMade)
                        {
                            s << key;
                        }
                        else
                        {
                            s << "..";
                        }

                        if (0 == (key + 1) % LineLength)
                        {
                            s << '\n';
                        }
                        else if (0 == (key + 1) % (LineLength / 2))
                        {
                            s << " -- ";
                        }
                        else
                        {
                            s << ' ';
                        }
                    }
                }

                table = move(s.str());
                table.resize(table.size() - 1); // shave off the last linefeed
            }

            lua_pushlstring(L, table.c_str(), table.length());
            return 1;
        }

        static void CreateTable(lua_State* L)
        {
            static const luaL_Reg MetatableFunctions[] =
            {
                { "__newindex", &SetIndex },
                { "__index", &Index },
                { "__len", &Length },
                { "__tostring", &ToString },
                { nullptr, nullptr }
            };

            {
                void* v = lua_newuserdata(L, 0); // push the userdata handle onto stack
                if (nullptr == v)
                {
                    throw runtime_error("failed to create a new Lua userdata object (likely caused by out-of-memory condition)");
                }
            }

            {
                const auto result = luaL_newmetatable(L, MetatableTypename); // push meta-table
                if (0 == result)
                {
                    throw logic_error("failed to create a new Lua metatable, it already exists");
                }

                luaL_register(L, nullptr, MetatableFunctions); // register meta-table functions
            }

            {
                const auto result = lua_setmetatable(L, -2); // pop the metatable; assign the metatable to the userdata handle
                if (0 == result)
                {
                    throw runtime_error("failed to set a Lua userdata object's metatable");
                }
            }

            lua_setglobal(L, Luaname); // pop the userdata handle, and add it to the global scope
        }
    }; // class CodeTable

    enum class CodeType { VirtualKey, Scancode };
    enum class KeyAction { Make, Break };

    // Define a scancode types for Lua.
    namespace sc
    {
        extern const char Typename[] = "scancode";
        extern const char MetatableTypename[] = "UberKey.ScancodeStates";
        extern const char Luaname[] = "scancodes";
        extern const char MakeLatches[] = "UberKey.ScancodeMakeLatches";
        extern const char BreakLatches[] = "UberKey.ScancodeBreakLatches";
        extern const char MakeInterceptions[] = "UberKey.ScancodeMakeInterceptions";
        extern const char BreakInterceptions[] = "UberKey.ScancodeBreakInterceptions";

        using ScancodeTable = CodeTable<decltype(madeScancodes), madeVirtualKeys, Typename, MetatableTypename, Luaname>;
    } // namespace sct

    // Define a virtual key types for Lua.
    namespace vk
    {
        extern const char Typename[] = "virtual key";
        extern const char MetatableTypename[] = "UberKey.VirtualKeyStates";
        extern const char Luaname[] = "virtual_keys";
        extern const char MakeLatches[] = "UberKey.VirtualKeyMakeLatches";
        extern const char BreakLatches[] = "UberKey.VirtualKeyBreakLatches";
        extern const char MakeInterceptions[] = "UberKey.VirtualKeyMakeInterceptions";
        extern const char BreakInterceptions[] = "UberKey.VirtualKeyBreakInterceptions";

        using VirtualKeyTable = CodeTable<decltype(madeVirtualKeys), madeVirtualKeys, Typename, MetatableTypename, Luaname>;
    } // namespace vkt

    template<CodeType useCode, const char* const CallbackTablename>
    void KeyCallbackHandler(lua_State* L, uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, bool e1, uint_fast32_t extraInformation)
    {
        lua_pushstring(L, CallbackTablename); // push the callback table's name
        lua_rawget(L, LUA_REGISTRYINDEX); // pop table name; push callback table

        assert(lua_istable(L, lua_gettop(L)));

        lua_rawgeti(L, -1, (useCode == CodeType::VirtualKey) ? virtualKey : scancode); // push callback function

        assert(lua_isfunction(L, -1));

        lua_replace(L, -2); // overwrite the callback table with the callback function; NOTE: not required, just frees a stack position

        // push the callback function default parameters, starting with the virtual key code
        lua_pushinteger(L, virtualKey);
        lua_pushinteger(L, scancode);
        lua_pushboolean(L, e0);
        lua_pushboolean(L, e1);
        lua_pushinteger(L, extraInformation);

        // Do callback(virtualKey, scancode, e0, e1, extraInformation)
        {
            const auto result = lua_pcall(L, 5, 0, 0);

            if (LUA_ERRRUN == result)
            {
                std::wcout << "Lua runtime error." << std::endl;
            }
            else if (LUA_ERRMEM == result)
            {
                std::wcout << "Lua memory allocation error." << std::endl;
            }
            else if (LUA_ERRERR == result)
            {
                std::wcout << "Lua error while running the error handler function." << std::endl;
            }
            else if (0 != result)
            {
                std::wcout << "Lua unknown error." << std::endl;
            }
        }
    }

    void InterceptedVirtualKeyMakeHander(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation)
    {
        if (nullptr == luaState)
        {
            return;
        }
        KeyCallbackHandler<CodeType::VirtualKey, vk::MakeInterceptions>(luaState, virtualKey, scancode, e0, false, extraInformation);
    }

    void InterceptedVirtualKeyBreakHander(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation)
    {
        if (nullptr == luaState)
        {
            return;
        }
        KeyCallbackHandler<CodeType::VirtualKey, vk::BreakInterceptions>(luaState, virtualKey, scancode, e0, false, extraInformation);
    }

    void InterceptedScancodeMakeHander(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation)
    {
        if (nullptr == luaState)
        {
            return;
        }
        KeyCallbackHandler<CodeType::Scancode, sc::MakeInterceptions>(luaState, virtualKey, scancode, e0, false, extraInformation);
    }

    void InterceptedScancodeBreakHander(uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, uint_fast32_t extraInformation)
    {
        if (nullptr == luaState)
        {
            return;
        }
        KeyCallbackHandler<CodeType::Scancode, sc::BreakInterceptions>(luaState, virtualKey, scancode, e0, false, extraInformation);
    }

    template<KeyMap& keyMap, const char* const CallbackTablename, const char* const Typename>
    int SetKeyCallback(lua_State* L)
    {
        // Argument checking
        if (lua_gettop(L) < 2) // if (there are less than 2 Lua arguments passed to this function)
        {
            luaL_error(L, "not enough arguments; ([integer] %s, [function] callback)", Typename);
        }

        const auto code = CheckCodeArgumentFromLua<Typename>(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        // Add function to callback table.
        Set(keyMap, code);

        lua_pushstring(L, CallbackTablename); // push the name of the callback table
        lua_rawget(L, LUA_REGISTRYINDEX); // pop table name; push callback table

        lua_replace(L, 1); // pop the callback table and move it over the key code argument on the stack

        lua_rawseti(L, 1, code); // callbacks[code] = argv[2]; pop callback

        assert(lua_gettop(L) == 1);
        // NOTE: let Lua clean the callback table off the stack

        return 0;
    }

    template<KeyMap& keyMap, const char* const CallbackTablename, const char* const Typename>
    int ClearKeyCallback(lua_State* L)
    {
        // Argument checking
        if (lua_gettop(L) < 1) // if (no arguments passed to this function)
        {
            luaL_error(L, "not enough arguments; ([integer] %s)", Typename);
        }

        const auto code = CheckCodeArgumentFromLua<Typename>(L, 1);

        // Remove function from callback table.
        Clear(keyMap, code);

        lua_pushstring(L, CallbackTablename); // push the name of the callback table
        lua_rawget(L, LUA_REGISTRYINDEX); // pop table name; push callback table

        lua_replace(L, 1); // pop the callback table and move it over the key code argument on the stack

        lua_pushnil(L); // push nil to delete any Lua callback
        lua_rawseti(L, 1, code); // callbacks[code] = nil; pop nil

        assert(lua_gettop(L) == 1);
        // NOTE: letting Lua clean the callback table off the stack

        return 0;
    }

    void CreateCallbackTables(lua_State* L)
    {
        // Create tables in the Lua registery for tracking latch callbacks
        lua_pushstring(L, sc::MakeLatches); // push callback table name
        lua_createtable(L, 256, 0); // push callback table
        lua_rawset(L, LUA_REGISTRYINDEX); // pop the callback table; pop the table name

        lua_pushstring(L, sc::BreakLatches); // push callback table name
        lua_createtable(L, 256, 0); // push callback table
        lua_rawset(L, LUA_REGISTRYINDEX); // pop the callback table; pop the table name

        lua_pushstring(L, vk::MakeLatches); // push callback table name
        lua_createtable(L, 256, 0); // push callback table
        lua_rawset(L, LUA_REGISTRYINDEX); // pop the callback table; pop the table name

        lua_pushstring(L, vk::BreakLatches); // push callback table name
        lua_createtable(L, 256, 0); // push callback table
        lua_rawset(L, LUA_REGISTRYINDEX); // pop the callback table; pop the table name

        // Create tables in the Lua registery for tracking latch callbacks
        lua_pushstring(L, sc::MakeInterceptions); // push callback table name
        lua_createtable(L, 256, 0); // push callback table
        lua_rawset(L, LUA_REGISTRYINDEX); // pop the callback table; pop the table name

        lua_pushstring(L, sc::BreakInterceptions); // push callback table name
        lua_createtable(L, 256, 0); // push callback table
        lua_rawset(L, LUA_REGISTRYINDEX); // pop the callback table; pop the table name

        lua_pushstring(L, vk::MakeInterceptions); // push callback table name
        lua_createtable(L, 256, 0); // push callback table
        lua_rawset(L, LUA_REGISTRYINDEX); // pop the callback table; pop the table name

        lua_pushstring(L, vk::BreakInterceptions); // push callback table name
        lua_createtable(L, 256, 0); // push callback table
        lua_rawset(L, LUA_REGISTRYINDEX); // pop the callback table; pop the table name
    }

    uint_fast16_t VirtualKeyToScancode(uint_fast16_t virtualKey)
    {
        const auto result = ::MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
        return static_cast<uint_fast16_t>(result);
    }

    uint_fast16_t ScancodeToVirtualKey(uint_fast16_t scancode)
    {
        // NOTE: Even using the MAPVK_VSC_TO_VK_EX flag to convert the enhanced scancodes to
        // virtual keys that distinguish left and right, it turns out that the SendInput() API will
        // end up down-casting them to the generic virtual key codes anyway.
        const auto result = ::MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX);
        return static_cast<uint_fast16_t>(result);
    }

    template< const char* const Typename, CodeType codeType, KeyAction keyAction >
    int SendKey(lua_State* L)
    {
        const auto argc = lua_gettop(L);

        if (argc < 1)
        {
            luaL_error(L, "not enough arguments; ([integer], [[integer]])");
        }

        const auto extendedCode = CheckCodeArgumentFromLua<sc::Typename>(L, 1);
        const auto code = (argc >= 2) ? CheckCodeArgumentFromLua<Typename>(L, 2) : CheckCodeArgumentFromLua<Typename>(L, 1);

        // TODO: decide if this needs to be here
        //if (CodeType::VirtualKey == codeType && (code < 1 || code > 254))
        //{
        //    luaL_error(L, "argument out-of-range; virtual key must be in the range of 1 to 254");
        //}

        INPUT input;
        input.type = INPUT_KEYBOARD;
        auto& ki = input.ki;

        ki.wVk = static_cast<WORD>((CodeType::VirtualKey == codeType) ? code : ScancodeToVirtualKey((extendedCode << 8) | code));
        ki.wScan = static_cast<WORD>((CodeType::Scancode == codeType) ? code : VirtualKeyToScancode(code));
        ki.dwFlags = ((0xe0 == extendedCode) ? KEYEVENTF_EXTENDEDKEY : 0u) | ((KeyAction::Make == keyAction) ? 0u : KEYEVENTF_KEYUP) | 
            ((CodeType::Scancode == codeType) ? KEYEVENTF_SCANCODE : 0u);
        ki.time = 0u;
        ki.dwExtraInfo = 0u;

        const auto result = ::SendInput(1, &input, sizeof(input));
        if (0 == result)
        {
            std::wcout << L"failed to send input -- error code: 0x" << std::hex << ::GetLastError() << std::dec << std::endl;
        }

        // letting Lua clean the stack

        return 0;
    }

    array<INPUT, 32u> inputBuffer; // NOTE: The size of this array needs to be an even number.

    int SendKeys(lua_State* L)
    {
        const auto argc = lua_gettop(L);

        for (auto argi = 0; argi < argc;)
        {
            UINT ui = 0u;

            for (; ui < inputBuffer.size() && argi < argc; ui++, argi++)
            {
                const auto virtualKey = static_cast<WORD>(CheckCodeArgumentFromLua<vk::Typename>(L, argi + 1));
                const auto scancode = static_cast<WORD>(VirtualKeyToScancode(virtualKey));

                inputBuffer[ui].type = INPUT_KEYBOARD;
                auto& kiMake = inputBuffer[ui].ki;

                kiMake.wVk = virtualKey;
                kiMake.wScan = scancode;
                kiMake.dwFlags = 0u;
                kiMake.time = 0u;
                kiMake.dwExtraInfo = 0u;

                ui++;

                inputBuffer[ui].type = INPUT_KEYBOARD;
                auto& kiBreak = inputBuffer[ui].ki;

                kiBreak.wVk = virtualKey;
                kiBreak.wScan = scancode;
                kiBreak.dwFlags = KEYEVENTF_KEYUP;
                kiBreak.time = 0u;
                kiBreak.dwExtraInfo = 0u;
            }

            const auto result = ::SendInput(ui, &inputBuffer[0], sizeof(inputBuffer[0]));
            if (0 == result)
            {
                std::wcout << L"failed to send input -- error code: 0x" << std::hex << ::GetLastError() << std::dec << std::endl;
            }
        }

        // letting Lua clean the stack

        return 0;
    }

    array<WCHAR, 2048u> unicodeOutput;

    // This function checks and flushes the input buffer if full.
    struct CheckFlushInputBuffer
    {
        CheckFlushInputBuffer(UINT& inputBufferIndex)
            : _inputBufferIndex(inputBufferIndex) {}

        void operator()(bool forceFlush = false)
        {
            if (0 == _inputBufferIndex)
            {
                return;
            }

            if (_inputBufferIndex < inputBuffer.size() && !forceFlush)
            {
                return;
            }

            const auto result = ::SendInput(_inputBufferIndex, &inputBuffer[0], sizeof(inputBuffer[0]));

            if (0 == result)
            {
                std::wcout << L"failed to send artificial keyboard input -- error code: 0x" <<
                    std::hex << ::GetLastError() << std::dec << std::endl;
            }

            _inputBufferIndex = 0u;
        }

    private:
        UINT& _inputBufferIndex;
    };

    struct VirtualKeyRecordWriter
    {
        VirtualKeyRecordWriter(UINT& inputBufferIndex)
            : _inputBufferIndex(inputBufferIndex) {}

        template< KeyAction keyAction >
        void WriteVirtualKey(uint_fast16_t virtualKey)
        {
            inputBuffer[_inputBufferIndex].type = INPUT_KEYBOARD;
            auto& ki = inputBuffer[_inputBufferIndex].ki;

            const auto scancode = static_cast<WORD>(VirtualKeyToScancode(virtualKey));

            ki.wVk = static_cast<WORD>(virtualKey);
            ki.wScan = scancode;
            ki.dwFlags = (KeyAction::Make == keyAction) ? 0u : KEYEVENTF_KEYUP;
            ki.time = 0u;
            ki.dwExtraInfo = 0u;

            _inputBufferIndex++;
        }

        template< KeyAction keyAction, uint_fast16_t virtualKey >
        void WriteVirtualKey()
        {
            inputBuffer[_inputBufferIndex].type = INPUT_KEYBOARD;
            auto& ki = inputBuffer[_inputBufferIndex].ki;

            const auto scancode = static_cast<WORD>(VirtualKeyToScancode(virtualKey));

            ki.wVk = virtualKey;
            ki.wScan = scancode;
            ki.dwFlags = (KeyAction::Make == keyAction) ? 0u : KEYEVENTF_KEYUP;
            ki.time = 0u;
            ki.dwExtraInfo = 0u;

            _inputBufferIndex++;
        }

    private:
        UINT& _inputBufferIndex;
    };

    struct CheckSetModifier
    {
        CheckSetModifier(VirtualKeyRecordWriter& virtualKeyBuffer, CheckFlushInputBuffer& checkFlushInputBuffer)
            : _vkbuf(virtualKeyBuffer), _CheckFlushVirtualKeys(checkFlushInputBuffer) {}

        template< uint_fast16_t virtualKey >
        void CheckSet(bool modifierKey, bool& currentModifierKeyState)
        {
            if (modifierKey != currentModifierKeyState)
            {
                if (currentModifierKeyState)
                {
                    // break virtual key
                    _vkbuf.WriteVirtualKey<KeyAction::Break, virtualKey>();
                }
                else
                {
                    // make virtual key
                    _vkbuf.WriteVirtualKey<KeyAction::Make, virtualKey>();
                }

                currentModifierKeyState = !currentModifierKeyState;

                // if (that fills the input buffer) send and restart
                _CheckFlushVirtualKeys();
            }
        }

    private:
        VirtualKeyRecordWriter& _vkbuf;
        CheckFlushInputBuffer& _CheckFlushVirtualKeys;
    };

    struct Utf8To16Converter
    {
        Utf8To16Converter(const char* str, size_t length)
            : _str(str)
            , _strLength(length)
            , _offset(0)
            , _maxOutputLength(unicodeOutput.size() / 3) {}

        // Returns count of characters convertered.
        int operator()()
        {
            int result; // characters converted

            if (_offset == _strLength)
            {
                return 0;
            }

            int length = static_cast<int>(min(_maxOutputLength, _strLength - _offset));
            int insufficientBufferRetries = 3;

            // Adjust length so that it falls between code points.
            auto BreakUtf8String = [this, &length]()
            {
                if ((_offset + length) >= _strLength)
                {
                    return;
                }

                if (0 == (0x80 & _str[length]) || 0x3 == (_str[length] >> 6))
                {
                    return;
                }

                int i = length - 1;
                auto ch = _str[i];

                // back up to the start of the previous code point
                while (0 != (0x80 & ch) && 0x3 != (ch >> 6) && i > 0)
                {
                    ch = _str[--i];
                }

                length = i + 1;
            };

            BreakUtf8String();

            for (;;)
            {
                result = ::MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    &_str[_offset],
                    static_cast<int>(length),
                    &unicodeOutput[0],
                    static_cast<int>(unicodeOutput.size())
                    );

                if (0xFFFD == result) // if (invalid UTF encodings)
                {
                    std::wcout << L"UTF8 to UTF16 translation failed; the input byte/char sequences are invalid" << std::endl;
                    // TODO: produce an error message for Lua?
                    return 0;
                }

                if (0 == result)
                {
                    const auto lastError = ::GetLastError();

                    // if (the problem is insufficient buffer)
                    if (ERROR_INSUFFICIENT_BUFFER == lastError && insufficientBufferRetries)
                    {
                        // try again with a smaller range
                        length /= 2;
                        insufficientBufferRetries--;

                        // Adjust length so that it falls between code points.
                        BreakUtf8String();

                        continue;
                    }

                    switch (lastError)
                    {
                    case ERROR_INSUFFICIENT_BUFFER:
                        std::wcout << L"UTF8 to UTF16 translation failed; insufficient buffer" << std::endl;
                        break;
                    case ERROR_INVALID_FLAGS:
                        std::wcout << L"UTF8 to UTF16 translation failed; invalid flags passed to MultiByteToWideChar()" << std::endl;
                        break;
                    case ERROR_INVALID_PARAMETER:
                        std::wcout << L"UTF8 to UTF16 translation failed; invalid parameter passed to MultiByteToWideChar()" << std::endl;
                        break;
                    case ERROR_NO_UNICODE_TRANSLATION:
                        std::wcout << L"UTF8 to UTF16 translation failed; invalid unicode was found in string" << std::endl;
                        break;
                    default:
                        std::wcout << L"UTF8 to UTF16 translation failed with unrecognized error code: 0x" << std::hex << lastError << std::dec << std::endl;
                        break;
                    }

                    // TODO: produce an error message for Lua

                    return 0;
                }

                break;
            }

            _offset += length;

            return result;
        }

    private:
        const char* const _str;
        const size_t _strLength;
        const size_t _maxOutputLength;

        size_t _offset;
    };

    int SendText(lua_State* L)
    {
        //VkKeyScanEx();
        const auto argc = lua_gettop(L);

        // Current write position into the virtual key inputBuffer.
        UINT inputBufferIndex = 0u;

        bool shiftMade = false;
        bool ctrlMade = false;
        bool altMade = false;
        bool hankakuMade = false;

        auto CheckFlushVirtualKeys = CheckFlushInputBuffer(inputBufferIndex);
        auto vkbuf = VirtualKeyRecordWriter(inputBufferIndex);
        auto modifierState = CheckSetModifier(vkbuf, CheckFlushVirtualKeys);

        for (auto arg = 0; arg < argc; arg++)
        {
            size_t strLength;
            const char* str;
            {
                str = lua_tolstring(L, arg + 1, &strLength);

                // make sure the argument is a string
                if (nullptr == str)
                {
                    std::wcout << L"failed to convert function parameter " << (arg + 1) << L" to a string" << std::endl;
                    // TODO: maybe produce a Lua error message?
                    continue; // the function argument is not a string
                }
            }

            // NOTE: This won't handle strings longer than numeric_limits<int>::max().
            strLength = min(static_cast<decltype(strLength)>(numeric_limits<int>::max()), strLength);

            // assume the string is UTF-8 and convert it to native UTF-16

            // do the string conversion and output in chunks
            auto StringConvert = Utf8To16Converter(str, strLength);

            auto length = StringConvert();

            while (0 != length)
            {
                for (decltype(length) i = 0u; i < length; i++)
                {
                    const auto ch = unicodeOutput[i];

                    uint_fast16_t virtualKey;
                    bool shift;
                    bool ctrl;
                    bool alt;
                    bool hankaku;

                    {
                        const auto result = ::VkKeyScanW(ch);
                        virtualKey = static_cast<uint8_t>(result);
                        uint_fast8_t modifierFlags = static_cast<uint8_t>(result >> 8);

                        if (-1 == static_cast<int8_t>(virtualKey) && -1 == static_cast<int8_t>(modifierFlags))
                        {
                            std::wcout << L"failed to convert unicode code point to virtual key sequence" << std::endl;
                            continue; // move to next code point
                        }

                        shift = 0 != (0x1 & modifierFlags);
                        ctrl = 0 != (0x2 & modifierFlags);
                        alt = 0 != (0x4 & modifierFlags);
                        hankaku = 0 != (0x8 & modifierFlags);
                    }

                    // With the virtual key code and required modifers known,
                    //  output the required sequence of virtual keys.

                    {
                        // Make sure the modifier keys are set correctly.
                        modifierState.CheckSet<VK_SHIFT>(shift, shiftMade);
                        modifierState.CheckSet<VK_CONTROL>(ctrl, ctrlMade);
                        modifierState.CheckSet<VK_MENU>(alt, altMade);
                        modifierState.CheckSet<VK_OEM_AUTO>(hankaku, hankakuMade);

                        // output virtual key make and break
                        vkbuf.WriteVirtualKey<KeyAction::Make>(virtualKey);
                        CheckFlushVirtualKeys();

                        vkbuf.WriteVirtualKey<KeyAction::Break>(virtualKey);
                        CheckFlushVirtualKeys();
                    }
                } // for ( the length of the string range )

                length = StringConvert();
            } // while ( 0 != length )
        } // for ( each string passed to this function from Lua )

        // Clear any modifier keys.
        bool clearFlag = false;
        modifierState.CheckSet<VK_SHIFT>(clearFlag, shiftMade);
        modifierState.CheckSet<VK_CONTROL>(clearFlag, ctrlMade);
        modifierState.CheckSet<VK_MENU>(clearFlag, altMade);
        modifierState.CheckSet<VK_OEM_AUTO>(clearFlag, hankakuMade);
        CheckFlushVirtualKeys(true); // Force a final flush.

        // letting Lua clean the stack

        return 0;
    }

    int HookKeyboard(lua_State* L)
    {
        UNREFERENCED_PARAMETER(L);
        hook::InstallLowLevelKeyboardHook();
        return 0;
    }

    int UnhookKeyboard(lua_State* L)
    {
        UNREFERENCED_PARAMETER(L);
        hook::DisableLowLevelKeyboardHook();
        return 0;
    }

    // SIDE-EFFECT: Leaves the keyboard table on the Lua stack.
    void RegisterKeyboardFunctions(lua_State* L)
    {
        static const luaL_Reg KeyboardFunctions[] =
        {
            { "listen_for_virtual_key_make", &SetKeyCallback<latchedVirtualKeyMakes, vk::MakeLatches, vk::Typename> },
            { "listen_for_virtual_key_break", &SetKeyCallback<latchedVirtualKeyBreaks, vk::BreakLatches, vk::Typename> },
            { "listen_for_scancode_make", &SetKeyCallback<latchedScancodeMakes, sc::MakeLatches, sc::Typename> },
            { "listen_for_scancode_break", &SetKeyCallback<latchedScancodeBreaks, sc::BreakLatches, sc::Typename> },
            { "stop_listening_for_virtual_key_make", &ClearKeyCallback<latchedVirtualKeyMakes, vk::MakeLatches, vk::Typename> },
            { "stop_listening_for_virtual_key_break", &ClearKeyCallback<latchedVirtualKeyBreaks, vk::BreakLatches, vk::Typename> },
            { "stop_listening_for_scancode_make", &ClearKeyCallback<latchedScancodeMakes, sc::MakeLatches, sc::Typename> },
            { "stop_listening_for_scancode_break", &ClearKeyCallback<latchedScancodeBreaks, sc::BreakLatches, sc::Typename> },
            { "send_virtual_key_make", &SendKey<vk::Typename, CodeType::VirtualKey, KeyAction::Make> },
            { "send_virtual_key_break", &SendKey<vk::Typename, CodeType::VirtualKey, KeyAction::Break> },
            { "send_scancode_make", &SendKey<vk::Typename, CodeType::Scancode, KeyAction::Make> },
            { "send_scancode_break", &SendKey<vk::Typename, CodeType::Scancode, KeyAction::Break> },
            { "send_keys", &SendKeys },
            { "send_text", &SendText },
            { "hook", &HookKeyboard },
            { "unhook", &UnhookKeyboard },
            { "intercept_virtual_key_make", &SetKeyCallback<interceptedVirtualKeyMakes, vk::MakeInterceptions, vk::Typename> },
            { "intercept_virtual_key_break", &SetKeyCallback<interceptedVirtualKeyBreaks, vk::BreakInterceptions, vk::Typename> },
            { "intercept_scancode_make", &SetKeyCallback<interceptedScancodeMakes, sc::MakeInterceptions, sc::Typename> },
            { "intercept_scancode_break", &SetKeyCallback<interceptedScancodeBreaks, sc::BreakInterceptions, sc::Typename> },
            { "stop_intercepting_virtual_key_make", &ClearKeyCallback<interceptedVirtualKeyMakes, vk::MakeInterceptions, vk::Typename> },
            { "stop_intercepting_virtual_key_break", &ClearKeyCallback<interceptedVirtualKeyBreaks, vk::BreakInterceptions, vk::Typename> },
            { "stop_intercepting_scancode_make", &ClearKeyCallback<interceptedScancodeMakes, sc::MakeInterceptions, sc::Typename> },
            { "stop_intercepting_scancode_break", &ClearKeyCallback<interceptedScancodeBreaks, sc::BreakInterceptions, sc::Typename> },
            { nullptr, nullptr }
        };

        // Register latch functions
        luaL_register(L, "keyboard", KeyboardFunctions);
    }

    // Creates virtual key description table
    // NOTE: Creates a new table and inserts that table inside the table at the top of the Lua stack.
    void CreateVirtualKeyDescriptionTable(lua_State* L)
    {
        lua_pushliteral(L, "virtual_key_descriptions"); // push the name of the virtual key description table

        lua_createtable(L, virtualKeyCount, 0); // create virtual key description table t and push onto stack

        for (auto i = 0u; i < virtualKeyCount; i++)
        {
            const auto& vk = virtualKeys[i];

            lua_pushstring(L, vk.info); // push the virtual key metadata v
            lua_rawseti(L, -2, i); // t[virtualKey] = v
        }

        lua_rawset(L, -3); // insert the description table into the table below it on the stack; pop description table.
    }

    // Creates the virtual key symbolic maps
    void CreateVirtualKeySymbolicNameTable(lua_State* L)
    {
        lua_createtable(L, virtualKeyCount, virtualKeyCount + altNameCount); // create table t and push onto stack

        for (auto i = 0u; i < virtualKeyCount; i++)
        {
            const auto& vk = virtualKeys[i];

            // Map virtual key enumeration-to-code
            lua_pushstring(L, vk.name); // push the enumeration name k
            lua_pushinteger(L, i); // push the virtual key code v to the top of the stack
            lua_rawset(L, -3); // t[k] = v

            // Map virtual key known-alternate enumerations-to-code
            if (nullptr != vk.altNames)
            {
                auto altIndex = 0u;
                auto alt = vk.altNames[altIndex++];

                while (nullptr != alt)
                {
                    lua_pushstring(L, alt); // push the enumeration name k
                    lua_pushinteger(L, i); // push the virtual key code v to the top of the stack
                    lua_rawset(L, -3); // t[k] = v

                    alt = vk.altNames[altIndex++];
                }
            }

            // Reverse lookup, map virtual key code-to-enumeration
            lua_pushstring(L, vk.name); // push the enumeration name v
            lua_rawseti(L, -2, i); // t[virtual_key_code] = v
        }

        lua_setglobal(L, "vk"); // push the virtual key names and codes into Lua's global scope
    }

    // Create and expose this application's Lua APIs.
    void OpenUberKeyLuaLibrary(lua_State* L)
    {
        sc::ScancodeTable::CreateTable(L);
        vk::VirtualKeyTable::CreateTable(L);
        CreateCallbackTables(L);
        CreateVirtualKeySymbolicNameTable(L);

        // Create the keyboard namespace in Lua.
        RegisterKeyboardFunctions(L);

        // Stuff that goes in the keyboard namespace below HERE.
        ////////////////////////////////////////////////////////
        CreateVirtualKeyDescriptionTable(L);

        assert(1 == lua_gettop(L));
        lua_pop(L, 1); // Clean the keyboard namespace off the Lua stack.
    }
} // namespace api

LRESULT Create(WPARAM wParam, LPARAM lParam)
{
    luaState = luaL_newstate(); // Create the initial lua state.
    if (nullptr == luaState)
    {
        throw exception("failed to create Lua state");
    }

    // Provide the std libs.
    luaL_openlibs(luaState);
    api::OpenUberKeyLuaLibrary(luaState);

    // Provide some C functions.
    lua_register(luaState, "print", &LuaPrintReplacement);
    lua_register(luaState, "dumpstack", &LuaDumpStack);

    {
        const auto path = GetProgramExecutablePath();

        std::ifstream inFile(path + L"UberKey.lua", std::ios_base::in | std::ios_base::binary);
        if (!inFile.good())
        {
			throw exception("failed to read UberKey.lua");
        }

        luaScriptBuffer.clear();
        decltype(luaScriptBuffer)::size_type pos = 0u;

        while (inFile.good())
        {
            if (pos == luaScriptBuffer.size())
            {
                luaScriptBuffer.resize(luaScriptBuffer.size() + 4096);
            }
            inFile.read(reinterpret_cast<char*>(&luaScriptBuffer[pos]), luaScriptBuffer.size() - pos);
            const auto count = inFile.gcount();
            pos = static_cast<decltype(pos)>((pos + count < numeric_limits<decltype(pos)>::max()) ? pos + count : numeric_limits<decltype(pos)>::max());
        }

        if (pos != luaScriptBuffer.size())
        {
            luaScriptBuffer.resize(pos);
        }
    }

    {
        const auto result = lua_load(luaState, &LuaReader, nullptr, "UberKey_Main_Script");
        if (LUA_ERRSYNTAX == result)
        {
            std::wcout << L"Syntax error compiling initial Lua script." << std::endl;
        }
        else if (LUA_ERRMEM == result)
        {
            throw bad_alloc();
        }
        else if (0 != result)
        {
            std::wcout << L"Unknown error compiling initial Lua script." << std::endl;
        }
    }

    {
        const auto result = lua_pcall(luaState, 0, LUA_MULTRET, 0);
        if (LUA_ERRRUN == result)
        {
            std::wcout << "Lua runtime error." << std::endl;
        }
        else if (LUA_ERRMEM == result)
        {
            std::wcout << "Lua memory allocation error." << std::endl;
        }
        else if (LUA_ERRERR == result)
        {
            std::wcout << "Lua error while running the error handler function." << std::endl;
        }
        else if (0 != result)
        {
            std::wcout << "Lua unknown error." << std::endl;
        }

        if (0 != result)
        {
            LuaDumpStack(luaState);
        }
    }

    {
        //LuaDumpStack(luaState);
    }

    return ::DefWindowProcW(_windowHandle, WM_CREATE, wParam, lParam);
}

/*
LRESULT KeyDown(WPARAM wParam, LPARAM lParam)
{
    // Any key pressed without ALT.

    const uint_fast16_t scancode = (lParam >> 16) & 0xff;
    const uint_fast16_t virtualKey = wParam;
    //const int count = LOWORD(lParam);

    // DEBUG: Handy debug overrides.
    switch (virtualKey)
    {
    case VK_ESCAPE:
    case VK_F12:
        Close();
        return 0;
    default:
        break;
    }
    // DEBUG: end

    MakeScancode(scancode);
    MakeKey(virtualKey);

    return 0;
}

LRESULT KeyUp(WPARAM wParam, LPARAM lParam)
{
    const uint_fast16_t scancode = (lParam >> 16) & 0xff;
    const uint_fast16_t virtualKey = wParam;

    BreakScancode(scancode);
    BreakKey(virtualKey);

    return 0;
}

LRESULT SyskeyDown(WPARAM wParam, LPARAM lParam)
{
    // Caused by F10 or any key pressed with ALT.
    // Can also be sent if no window has keyboard focus.

    const uint_fast16_t scancode = (lParam >> 16) & 0xff;
    const uint_fast16_t virtualKey = wParam;
    //const int count = LOWORD(lParam);

    MakeScancode(scancode);
    MakeKey(virtualKey);

    return 0;
}

LRESULT SyskeyUp(WPARAM wParam, LPARAM lParam)
{
    const uint_fast16_t scancode = (lParam >> 16) & 0xff;
    const uint_fast16_t virtualKey = wParam;

    BreakScancode(scancode);
    BreakKey(virtualKey);

    return 0;
}

LRESULT Character(WPARAM wParam, LPARAM lParam)
{
    const auto ch = static_cast<wchar_t>(wParam);
    const int count = LOWORD(lParam);

    _filteredCharacters.append(count, ch);

    return 0;
}
*/

LRESULT Destroy(WPARAM wParam, LPARAM lParam)
{
    lua_close(luaState);
    ::PostQuitMessage(0);
    return ::DefWindowProcW(_windowHandle, WM_DESTROY, wParam, lParam);
}

LRESULT AppCommand(WPARAM wParam, LPARAM lParam)
{
    return ::DefWindowProcW(_windowHandle, WM_APPCOMMAND, wParam, lParam);
}

void PrintRawKeyboardDebug(bool isMake, uint_fast16_t virtualKey, uint_fast16_t scancode, bool e0, bool e1, uint_fast32_t extraInfo)
{
    std::wcout << ((isMake) ? L"M:" : L"B:");
    if (e0) // if (this is the enhanced/extended-key prefix E0)
    {
        std::wcout << L"E0";
    }
    if (e1) // if (this is the enhancedextended-key prefix E1)
    {
        std::wcout << L"E1";
    }

    std::wcout << std::hex << scancode << L':' << virtualKey;
    if (extraInfo)
    {
        std::wcout << L':' << extraInfo;
    }
    std::wcout << std::dec << L' ';
}

void ProcessRawKeyboardInput(const RAWKEYBOARD& keyboard)
{
    const uint_fast16_t scancode = keyboard.MakeCode;
    const uint_fast16_t virtualKey = keyboard.VKey;

    if (KEYBOARD_OVERRUN_MAKE_CODE == scancode)
    {
        ::OutputDebugStringW(L"Keyboard buffer overrun detected.");
    }

    const auto e0 = 0 != (RI_KEY_E0 & keyboard.Flags);
    const auto e1 = 0 != (RI_KEY_E1 & keyboard.Flags);

    if (0 == (RI_KEY_BREAK & keyboard.Flags)) // if (the key was made)
    {
        PrintRawKeyboardDebug(true, virtualKey, scancode, e0, e1, keyboard.ExtraInformation);

        MakeScancode(scancode);
        MakeVirtualKey(virtualKey);

        if (IsVirtualKeyMakeLatched(virtualKey))
        {
            api::KeyCallbackHandler<api::CodeType::VirtualKey, api::vk::MakeLatches>(luaState, virtualKey, scancode, e0, e1, keyboard.ExtraInformation);
        }

        if (IsScancodeMakeLatched(scancode))
        {
            api::KeyCallbackHandler<api::CodeType::Scancode, api::sc::MakeLatches>(luaState, virtualKey, scancode, e0, e1, keyboard.ExtraInformation);
        }
    }
    else
    {
        //PrintRawKeyboardDebug(false, virtualKey, scancode, e0, e1, keyboard.ExtraInformation);

        BreakScancode(scancode);
        BreakVirtualKey(virtualKey);

        if (IsVirtualKeyBreakLatched(virtualKey))
        {
            api::KeyCallbackHandler<api::CodeType::VirtualKey, api::vk::BreakLatches>(luaState, virtualKey, scancode, e0, e1, keyboard.ExtraInformation);
        }

        if (IsScancodeBreakLatched(scancode))
        {
            api::KeyCallbackHandler<api::CodeType::Scancode, api::sc::BreakLatches>(luaState, virtualKey, scancode, e0, e1, keyboard.ExtraInformation);
        }
    }
}

LRESULT Input(WPARAM wParam, LPARAM lParam)
{
    // TODO: Switch to the GetRawInputBuffer() function; instead of GetRawInputData().
    //const auto rawInputCode = GET_RAWINPUT_CODE_WPARAM(wParam);

    {
        UINT requiredSize = 0;

        const auto bytesCopied = ::GetRawInputData(
            reinterpret_cast<HRAWINPUT>(lParam),
            RID_INPUT,
            nullptr,
            &requiredSize,
            sizeof(RAWINPUTHEADER)
            );

        if (UINT(-1) == bytesCopied)
        {
            throw exception("Failed to get the size required for a raw input buffer.");
        }

        if (requiredSize > _rawInputBuffer.size())
        {
            _rawInputBuffer.resize(requiredSize);
        }
    }

    {
        UINT requiredSize = static_cast<UINT>(_rawInputBuffer.size());

        const auto bytesCopied = ::GetRawInputData(
            reinterpret_cast<HRAWINPUT>(lParam),
            RID_INPUT,
            &_rawInputBuffer[0],
            &requiredSize,
            sizeof(RAWINPUTHEADER)
            );

        if (UINT(-1) == bytesCopied)
        {
            throw exception("Failed to get a raw input buffer.");
        }
    }

    const auto& input = *reinterpret_cast<PRAWINPUT>(&_rawInputBuffer[0]);
    const auto& type = input.header.dwType;

    if (RIM_TYPEKEYBOARD == type)
    {
        ProcessRawKeyboardInput(input.data.keyboard);
    }
    else
    {
        const auto hr = ::DefRawInputProc(reinterpret_cast<PRAWINPUT*>(&_rawInputBuffer[0]), static_cast<INT>(_rawInputBuffer.size()), sizeof(RAWINPUTHEADER));
        if (FAILED(hr))
        {
            ::OutputDebugStringW(L"Failed to pass unhandled raw input to default handler.\n");
        }
    }

    return ::DefWindowProcW(_windowHandle, WM_INPUT, wParam, lParam);
}

void EnableRawKeyboardInput(const bool value)
{
    RAWINPUTDEVICE device;

    device.usUsagePage  = 0x01; // Generic Desktop
    device.usUsage      = 0x06; // Keyboard
    device.dwFlags      = (value) ? (RIDEV_INPUTSINK | RIDEV_NOLEGACY) : RIDEV_REMOVE; // RIDEV_NOLEGACY prevents WM_KEYDOWN, WM_CHAR, etc. messages.
    device.hwndTarget   = (0 != (RIDEV_REMOVE & device.dwFlags)) ? nullptr : _windowHandle;

    {
        const auto regResult = ::RegisterRawInputDevices(
            &device,
            1,
            sizeof(device)
            );
        if (0 == regResult)
        {
            throw exception("Failed to configure keyboard for raw input.");
        }
    }

    _isReadingRawKeyboard = value;
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    try
    {
        // Find the handler this message is addressed to.
        const auto msgIt = messageMap.find(message);
        if (messageMap.cend() == msgIt)
        {
            return ::DefWindowProcW(hWnd, message, wParam, lParam); // This message type isn't handled.
        }

        const WindowMessageHandler MessageHandler = msgIt->second;

        // Process the message with the appropriate handler.
        return MessageHandler(wParam, lParam);
    }
    catch (const exception& e)
    {
        ::OutputDebugStringW(L"Unhandled exception thrown from a window callback handler: ");
        ::OutputDebugStringA(e.what());
        ::OutputDebugStringW(L"\n");
    }
    catch (...)
    {
        ::OutputDebugStringW(L"Unhandled exception of unknown type thrown from a window callback handler!\n");

        // NOTE: Don't rethrow, Windows won't handle it.
    }

    return ::DefWindowProcW(hWnd, message, wParam, lParam);
}

bool TryHandlePlatformEvent(bool& isQuitting, int& exitCode)
{
    isQuitting = false;
    MSG msg;
    const auto result = ::GetMessageW(&msg, nullptr, 0, 0);

    if (-1 == result) // if (error)
    {
        ::OutputDebugStringW(L"Error calling GetMessageW()");
        return false;
    }

    if (0 == result) // if (WM_QUIT)
    {
        isQuitting = true;
        exitCode = static_cast<int>(msg.wParam);
        return true;
    }

    //::TranslateMessage(&msg); // Doesn't make sense if you don't care about legacy keyboard input.
    ::DispatchMessageW(&msg);
    return true;
}

int WindowsEventLoop()
{
    for (;;) // -ever
    {
        // Check for pending OS events:
        bool isQuitting;
        int exitCode;
        if (TryHandlePlatformEvent(isQuitting, exitCode)) // if (an OS event was handled)
        {
            // Check to see if it was a quit event.
            if (isQuitting)
            {
                return exitCode;
            }
        }
    }
}

void CreateApplicationWindow(
    const wchar_t* const    windowTitle,
    const Sizeui&           windowClientSize,
    const HINSTANCE         hInstance,
    const int               nCmdShow
    )
{
    WNDCLASSEX wc;

    // Set up and register window class
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = &WindowProcedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, IDI_APPLICATION);
    wc.hCursor = nullptr; //LoadCursor( nullptr, IDC_ARROW );
    wc.hbrBackground = nullptr; //(HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = WindowClassName;
    wc.hIconSm = nullptr;
    ::RegisterClassExW(&wc);

    //RECT rect = ConvertToWindowsRECT(RasterRectangleui(windowClientSize));
    //const BOOL result = ::AdjustWindowRectEx(&rect, WindowStyle, FALSE, WindowExtendedStyle);
    //if (!result)
    //{
    //    LOG(_T("Failed to calculate the window's non-client area."));
    //}
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = windowClientSize.width;
    rect.bottom = windowClientSize.height;

    // Create the window
    const HWND hWnd = ::CreateWindowExW(
        WindowExtendedStyle,
        WindowClassName,
        windowTitle,
        WindowStyle,
        32, //1680, // DEBUG
        32,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
        );
    if (!hWnd)
    {
        throw exception("Failed to create main window.");
    }

    ::ShowWindow(hWnd, nCmdShow);
    ::UpdateWindow(hWnd);
    ::SetFocus(hWnd);

    _moduleHandle = hInstance;
    _windowHandle = hWnd;
}

void CreateMessageMap()
{
    messageMap[WM_PAINT] = &Paint;
    messageMap[WM_CREATE] = &Create;
    //messageMap[WM_KEYDOWN] = &KeyDown;
    //messageMap[WM_KEYUP] = &KeyUp;
    //messageMap[WM_SYSKEYDOWN] = &SyskeyDown;
    //messageMap[WM_SYSKEYUP] = &SyskeyUp;
    //messageMap[WM_CHAR] = &Character;
    messageMap[WM_DESTROY] = &Destroy;
    messageMap[WM_APPCOMMAND] = &AppCommand;
    messageMap[WM_INPUT] = &Input;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //volatile bool pausing = true;
    //while (pausing) { Sleep(1000); }

    ::AllocConsole();
    FILE* file;
    ::_wfreopen_s(&file, L"CONOUT$", L"w", stdout);
    ::_wfreopen_s(&file, L"CONOUT$", L"w", stderr);
    ::_wfreopen_s(&file, L"CONIN$", L"r", stdin);

    int result = 0;
    {
        try
        {
            CreateMessageMap();

            Sizeui windowClientSize;
            windowClientSize.width = 300;
            windowClientSize.height = 200;

            CreateApplicationWindow(L"Uber Key", windowClientSize, hInstance, nCmdShow);

            EnableRawKeyboardInput(true);

            std::cout << "Entering event loop..." << std::endl;

            result = WindowsEventLoop();
        }
        catch (const bad_alloc&)
        {
            ::OutputDebugStringW(L"Out of memory condition is causing the application to terminate.");
            result = -1;
        }
        catch (const exception& e)
        {
            ::OutputDebugStringW(L"Unhandled exception is terminating the application: ");
            ::OutputDebugStringA(e.what());
            ::OutputDebugStringW(L"\n");
            result = -3;
        }
        catch (...)
        {
            ::OutputDebugStringW(L"Unhandled exception of unknown type is terminating the application.\n");
            result = -4;
        }
    }

    hook::DisableLowLevelKeyboardHook(); // TODO: move this out of here ... probably

    ::FreeConsole();

    return result;
}
