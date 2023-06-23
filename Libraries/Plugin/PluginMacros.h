// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#pragma once
#include "../Foundation/Memory.h"
#include "../Foundation/Types.h"

//
// SC_PLUGIN_EXPORT
//

#if SC_MSVC
#define SC_PLUGIN_EXPORT __declspec(dllexport)
#else
#define SC_PLUGIN_EXPORT
#endif

//
// SC_PLUGIN_LINKER_DEFINITIONS
//
#if SC_PLUGIN_LIBRARY
#if SC_MSVC
#define SC_PLUGIN_LINKER_DEFINITIONS                                                                                   \
    void* operator new(SC::size_t len)                                                                                 \
    {                                                                                                                  \
        return SC::memoryAllocate(len);                                                                                \
    }                                                                                                                  \
    void* operator new[](SC::size_t len)                                                                               \
    {                                                                                                                  \
        return SC::memoryAllocate(len);                                                                                \
    }                                                                                                                  \
    void operator delete(void* p, SC::uint64_t) noexcept                                                               \
    {                                                                                                                  \
        if (p != 0)                                                                                                    \
        {                                                                                                              \
            SC::memoryRelease(p);                                                                                      \
        }                                                                                                              \
    }                                                                                                                  \
    extern "C" void __CxxFrameHandler4()                                                                               \
    {                                                                                                                  \
    }                                                                                                                  \
    extern "C" void __CxxFrameHandler3()                                                                               \
    {                                                                                                                  \
    }                                                                                                                  \
    int __stdcall DllMain(void*, unsigned int, void*)                                                                  \
    {                                                                                                                  \
        return 1;                                                                                                      \
    }
#else
// Cannot use builtin like __builtin_bzero as they will generate an infinite loop
// We also use inline asm to disable optimizations
// See: https://nullprogram.com/blog/2023/02/15/
#define SC_PLUGIN_LINKER_DEFINITIONS                                                                                   \
    extern "C" void bzero(void* s, SC::size_t n)                                                                       \
    {                                                                                                                  \
        unsigned char* p = reinterpret_cast<unsigned char*>(s);                                                        \
        while (n-- > 0)                                                                                                \
        {                                                                                                              \
            __asm("");                                                                                                 \
            *p++ = 0;                                                                                                  \
        }                                                                                                              \
    }
#endif
#else
#define SC_PLUGIN_LINKER_DEFINITIONS
#endif

//
// SC_DEFINE_PLUGIN
//
#define SC_DEFINE_PLUGIN(PluginName)                                                                                   \
    SC_PLUGIN_LINKER_DEFINITIONS                                                                                       \
    extern "C" SC_PLUGIN_EXPORT bool PluginName##Init(PluginName*& instance)                                           \
    {                                                                                                                  \
        instance = new PluginName();                                                                                   \
        return instance->init();                                                                                       \
    }                                                                                                                  \
                                                                                                                       \
    extern "C" SC_PLUGIN_EXPORT bool PluginName##Close(PluginName* instance)                                           \
    {                                                                                                                  \
        auto res = instance->close();                                                                                  \
        delete instance;                                                                                               \
        return res;                                                                                                    \
    }