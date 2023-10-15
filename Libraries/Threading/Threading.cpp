// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#include "Threading.h"

#include "../Foundation/Strings/SmallString.h"
#include "../Foundation/Strings/StringConverter.h"

#if SC_PLATFORM_WINDOWS
#include "Internal/ThreadingWindows.inl"
#else
#include "Internal/ThreadingPosix.inl"
#define WINAPI
#endif

struct SC::Thread::CreateParams
{
    CreateParams(Thread* thread) : thread(thread) {}

    EventObject event;
    Action*     callback;
    Action*     syncCallback;
    Thread*     thread;
    StringView  nameNullTerminated;

    Internal::NativeHandle threadHandle;

    static Internal::CallbackReturnType WINAPI threadFunc(void* argument)
    {
        CreateParams& self = *static_cast<CreateParams*>(argument);

        if (not self.nameNullTerminated.isEmpty())
        {
            Internal::setThreadName(self.threadHandle, self.nameNullTerminated);
        }
        if (self.syncCallback)
        {
            (*self.syncCallback)();
        }
        // Note: If cb has captured heap allocated objects, they will get freed on the thread
        auto callback = self.callback;
        self.event.signal();
        if (callback)
        {
            (*callback)();
        }
        return 0;
    }
};

SC::Thread::~Thread() { SC_ASSERT_DEBUG(not thread.hasValue() && "Forgot to call join() or detach()"); }

SC::Result SC::Thread::start(StringView name, Action* func, Action* syncFunc)
{
    if (thread.hasValue())
        return Result::Error("Error thread already started");
    CreateParams self(this);
    self.callback     = func;
    self.syncCallback = syncFunc;

    StringNative<128> nameNative = StringEncoding::Native;
    SC_TRY(StringConverter(nameNative).convertNullTerminateFastPath(name, self.nameNullTerminated));
    OpaqueThread opaqueThread;
    SC_TRY(Internal::createThread(self, opaqueThread, self.threadHandle, &CreateParams::threadFunc));
    thread.assign(move(opaqueThread));
    self.event.wait();
    return Result(true);
}

SC::Result SC::Thread::join()
{
    OpaqueThread* threadNative;
    SC_TRY(thread.get(threadNative));
    SC_TRY(Internal::joinThread(threadNative));
    thread.clear();
    return Result(true);
}

SC::Result SC::Thread::detach()
{
    OpaqueThread* threadNative;
    SC_TRY(thread.get(threadNative));
    SC_TRY(Internal::detachThread(threadNative));
    thread.clear();
    return Result(true);
}

bool SC::Thread::wasStarted() const { return thread.hasValue(); }

void SC::EventObject::wait()
{
    mutex.lock();
    while (not isSignaled)
    {
        cond.wait(mutex);
    }
    if (autoReset)
    {
        isSignaled = false;
    }
    mutex.unlock();
}

void SC::EventObject::signal()
{
    mutex.lock();
    isSignaled = true;
    mutex.unlock();
    cond.signal();
}
