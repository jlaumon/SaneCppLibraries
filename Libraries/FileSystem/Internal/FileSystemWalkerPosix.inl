// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
// clang-format off
#include "../FileSystemWalker.h"
#include "../../Foundation/Strings/StringConverter.h"
#include "../../Foundation/Strings/SmallString.h"
#include "../../Foundation/Containers/SmallVector.h"

// clang-format on
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace SC
{
SC::Result getErrorCode(int errorCode); // defined in FileSystemInternalPosix.inl
}

struct SC::FileSystemWalker::Internal
{
    struct StackEntry
    {
        DIR*   dirEnumerator     = nullptr;
        size_t textLengthInBytes = 0;
        int    fileDescriptor    = -1; // TODO: use actual filedescriptor class here
        bool   gotDot1           = false;
        bool   gotDot2           = false;

        Result init(int fd)
        {
            fileDescriptor = fd;
            if (fileDescriptor == -1)
            {
                return getErrorCode(errno);
            }
            dirEnumerator = fdopendir(fileDescriptor);
            if (dirEnumerator == nullptr)
            {
                return getErrorCode(errno);
            }
            return Result(true);
        }
        void close()
        {
            if (fileDescriptor != -1)
            {
                ::close(fileDescriptor);
            }
        }
    };
    SmallVector<StackEntry, 64> recurseStack;
    StringNative<512>           currentPathString = StringEncoding::Native;
    Internal() {}

    ~Internal()
    {
        while (not recurseStack.isEmpty())
        {
            recurseStack.back().close();
            SC_TRUST_RESULT(recurseStack.pop_back());
        }
    }

    [[nodiscard]] Result init(StringView directory)
    {
        StringConverter currentPath(currentPathString, StringConverter::Clear);
        SC_TRY(currentPath.appendNullTerminated(directory));
        StackEntry entry;
        entry.textLengthInBytes = currentPathString.view().sizeInBytesIncludingTerminator();
        SC_TRY(entry.init(open(currentPathString.view().getNullTerminatedNative(), O_DIRECTORY)));
        SC_TRY(recurseStack.push_back(move(entry)));
        return Result(true);
    }

    Result enumerateNext(Entry& entry, const Options& opts)
    {
        StringConverter currentPath(currentPathString);
        if (recurseStack.isEmpty())
            return Result::Error("Forgot to call init");
        StackEntry&    parent = recurseStack.back();
        struct dirent* item;
        for (;;)
        {
            item = readdir(parent.dirEnumerator);
            if (item == nullptr)
            {
                recurseStack.back().close();
                SC_TRY(recurseStack.pop_back());
                if (recurseStack.isEmpty())
                {
                    entry.parentFileDescriptor.detach();
                    return Result::Error("Iteration Finished");
                }
                parent = recurseStack.back();
                SC_TRY(currentPath.setTextLengthInBytesIncludingTerminator(parent.textLengthInBytes));
                continue;
            }
            if (not(parent.gotDot1 and parent.gotDot2))
            {
                const bool isDot1 = strncmp(item->d_name, ".", 1) == 0;
                const bool isDot2 = strncmp(item->d_name, "..", 2) == 0;
                if (isDot1)
                    parent.gotDot1 = true;
                if (isDot2)
                    parent.gotDot2 = true;
            }
            else
            {
                break;
            }
        }
        entry.name = StringView(item->d_name, item->d_namlen, true, StringEncoding::Utf8);
        SC_TRY(currentPath.setTextLengthInBytesIncludingTerminator(recurseStack.back().textLengthInBytes));
        SC_TRY(currentPath.appendNullTerminated("/"_u8));
        SC_TRY(currentPath.appendNullTerminated(entry.name));
        entry.path  = currentPathString.view();
        entry.level = static_cast<decltype(entry.level)>(recurseStack.size() - 1);
        entry.parentFileDescriptor.detach();
        SC_TRY(entry.parentFileDescriptor.assign(parent.fileDescriptor));
        if (item->d_type == DT_DIR)
        {
            entry.type = Type::Directory;
            if (opts.recursive)
            {
                SC_TRY(recurseSubdirectory(entry));
            }
        }
        else
        {
            entry.type = Type::File;
        }
        return Result(true);
    }

    [[nodiscard]] Result recurseSubdirectory(Entry& entry)
    {
        StringConverter currentPath(currentPathString);
        StackEntry      newParent;
        SC_TRY(currentPath.setTextLengthInBytesIncludingTerminator(recurseStack.back().textLengthInBytes));
        SC_TRY(currentPath.appendNullTerminated("/"_u8));
        SC_TRY(currentPath.appendNullTerminated(entry.name));
        newParent.textLengthInBytes = currentPathString.view().sizeInBytesIncludingTerminator();
        FileDescriptor::Handle handle;
        SC_TRY(entry.parentFileDescriptor.get(handle, Result::Error("recurseSubdirectory - InvalidHandle")));
        SC_TRY(newParent.init(openat(handle, entry.name.bytesIncludingTerminator(), O_DIRECTORY)));
        SC_TRY(recurseStack.push_back(newParent))
        return Result(true);
    }
};