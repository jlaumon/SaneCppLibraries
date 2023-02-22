// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#include "FileSystem.h"
// clang-format off
#include <stdio.h>
#include <wchar.h>
#include <Windows.h>
// clang-format on

static constexpr const SC::StringView getErrorCode(int errorCode)
{
    switch (errorCode)
    {
    case EEXIST: return "EEXIST"_a8;
    case ENOENT: return "ENOENT"_a8;
    }
    return "Unknown"_a8;
}

struct SC::FileSystem::Internal
{
#define SC_TRY_LIBC(func)                                                                                              \
    {                                                                                                                  \
        if (func != 0)                                                                                                 \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    }

    [[nodiscard]] static bool makeDirectory(const wchar_t* dir)
    {
        SC_TRY_LIBC(_wmkdir(dir));
        return true;
    }

    [[nodiscard]] static bool exists(const wchar_t* fileOrDirectory)
    {
        const DWORD res = GetFileAttributesW(fileOrDirectory);
        return res != INVALID_FILE_ATTRIBUTES;
    }

    [[nodiscard]] static bool existsAndIsDirectory(const wchar_t* dir)
    {
        const DWORD res = GetFileAttributesW(dir);
        if (res == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }
        return (res & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    [[nodiscard]] static bool existsAndIsFile(const wchar_t* dir)
    {
        const DWORD res = GetFileAttributesW(dir);
        if (res == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }
        return (res & FILE_ATTRIBUTE_NORMAL) != 0;
    }

    [[nodiscard]] static bool removeEmptyDirectory(const wchar_t* dir)
    {
        SC_TRY_LIBC(_wrmdir(dir));
        return true;
    }

    [[nodiscard]] static bool removeFile(const wchar_t* file)
    {
        SC_TRY_LIBC(_wremove(file));
        return true;
    }

    [[nodiscard]] static bool openFileRead(const wchar_t* path, FILE*& file)
    {
        return _wfopen_s(&file, path, L"rb") == 0;
    }

    [[nodiscard]] static bool openFileWrite(const wchar_t* path, FILE*& file)
    {
        return _wfopen_s(&file, path, L"wb") == 0;
    }

    [[nodiscard]] static bool formatError(int errorNumber, String& buffer)
    {
        buffer.encoding = StringEncoding::Utf16;
        SC_TRY_IF(buffer.data.resizeWithoutInitializing(buffer.data.capacity()));
        const int res = _wcserror_s(buffer.nativeWritableBytesIncludingTerminator(),
                                    buffer.sizeInBytesIncludingTerminator() / sizeof(wchar_t), errorNumber);
        if (res == 0)
        {
            const size_t numUtf16Points = wcslen(buffer.nativeWritableBytesIncludingTerminator()) + 1;
            return buffer.data.resizeWithoutInitializing(numUtf16Points * sizeof(wchar_t));
        }
        (void)buffer.data.resizeWithoutInitializing(0);
        return false;
    }

    [[nodiscard]] static bool formatWindowsError(int errorNumber, String& buffer)
    {
        LPWSTR messageBuffer = nullptr;
        size_t size          = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            errorNumber, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&messageBuffer), 0, NULL);
        auto deferFree = MakeDeferred([&]() { LocalFree(messageBuffer); });

        // TODO: We should use a fallible append here instead of the assignment
        const StringView sv =
            StringView(Span<const wchar_t>(messageBuffer, size * sizeof(wchar_t)), true, StringEncoding::Utf16);
        SC_TRY_IF(buffer.assign(sv));
        return true;
    }

    [[nodiscard]] static bool copyFile(const wchar_t* sourceFile, const wchar_t* destinationFile,
                                       FileSystem::CopyFlags options)
    {
        DWORD flags = COPY_FILE_FAIL_IF_EXISTS;
        if (options.overwrite)
            flags &= ~COPY_FILE_FAIL_IF_EXISTS;
        BOOL res = CopyFileExW(sourceFile, destinationFile, nullptr, nullptr, nullptr, flags);
        return res == TRUE;
    }

    template <int N>
    [[nodiscard]] static ReturnCode copyDirectory(StringNative<N>& sourceDirectory,
                                                  StringNative<N>& destinationDirectory, FileSystem::CopyFlags options)
    {
        SHFILEOPSTRUCTW s    = {0};
        const wchar_t*  dest = destinationDirectory.view().getNullTerminatedNative();
        if (not options.overwrite)
        {
            if (existsAndIsDirectory(dest))
            {
                return "Directory already exists"_a8;
            }
            else if (existsAndIsFile(dest))
            {
                return "A file already exists at the location"_a8;
            }
        }
        s.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI;
        s.wFunc  = FO_COPY;
        SC_TRY_IF(sourceDirectory.appendNullTerminated(L"\\*"));
        // SHFileOperationW needs two null termination bytes
        sourceDirectory.text.pushNullTerm();
        destinationDirectory.text.pushNullTerm();
        s.pFrom       = sourceDirectory.view().getNullTerminatedNative();
        s.pTo         = dest;
        const int res = SHFileOperationW(&s);
        return res == 0;
    }

    template <int N>
    [[nodiscard]] static bool removeDirectoryRecursive(StringNative<N>& sourceDirectory)
    {
        SHFILEOPSTRUCTW s = {0};
        s.fFlags          = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI;
        s.wFunc           = FO_DELETE;
        // SHFileOperationW needs two null termination bytes
        sourceDirectory.text.pushNullTerm();
        s.pFrom       = sourceDirectory.view().getNullTerminatedNative();
        const int res = SHFileOperationW(&s);
        return res == 0;
    }
#undef SC_TRY_LIBC
};