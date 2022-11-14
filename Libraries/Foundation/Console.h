#pragma once
#include "Compiler.h"
#include "OS.h"
#include "StringView.h"
#include "Types.h"

namespace SC
{
struct String;
struct Console
{
    static int  c_printf(const char_t* format, ...) SC_ATTRIBUTE_PRINTF(1, 2);
    static void printUTF8(const StringView str);
    static void printUTF8(const String& str);
};
} // namespace SC