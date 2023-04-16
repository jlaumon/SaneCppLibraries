// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#include "String.h"

bool SC::String::assign(const StringView& sv)
{
    const size_t length    = sv.sizeInBytes();
    encoding               = sv.getEncoding();
    const uint32_t numZero = StringEncodingGetSize(encoding);
    bool           res     = data.resizeWithoutInitializing(length + numZero);
    if (sv.isNullTerminated())
    {
        memcpy(data.items, sv.bytesWithoutTerminator(), length + numZero);
    }
    else
    {
        memcpy(data.items, sv.bytesWithoutTerminator(), length);
        for (uint32_t idx = 0; idx < numZero; ++idx)
        {
            data.items[length + idx] = 0;
        }
    }
    return res;
}

bool SC::String::popNulltermIfExists()
{
    const auto sizeOfZero = StringEncodingGetSize(encoding);
    const auto dataSize   = data.size();
    if (dataSize >= sizeOfZero)
    {
        return data.resizeWithoutInitializing(dataSize - sizeOfZero);
    }
    return true;
}

bool SC::String::pushNullTerm()
{
    auto numZeros = StringEncodingGetSize(encoding);
    while (numZeros--)
    {
        SC_TRY_IF(data.push_back(0));
    }
    return true;
}

SC::StringView SC::String::view() const
{
    if (data.isEmpty())
    {
        return StringView(nullptr, 0, false, encoding);
    }
    else
    {
        return StringView(data.items, data.size() - StringEncodingGetSize(encoding), true, encoding);
    }
}

bool SC::StringFormatterFor<SC::String>::format(StringFormatOutput& data, const StringIteratorASCII specifier,
                                                const SC::String& value)
{
    return StringFormatterFor<SC::StringView>::format(data, specifier, value.view());
}
