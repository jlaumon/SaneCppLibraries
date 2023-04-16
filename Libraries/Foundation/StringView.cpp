// Copyright (c) 2022, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#include "StringView.h"

#include <stdlib.h> //atoi

struct SC::StringView::Internal
{
    template <typename StringIterator>
    [[nodiscard]] static StringView sliceStartEnd(const StringView& self, size_t start, size_t end)
    {
        auto it = self.getIterator<StringIterator>();
        SC_RELEASE_ASSERT(it.advanceCodePoints(start));
        auto startIt = it;
        SC_RELEASE_ASSERT(start <= end && it.advanceCodePoints(end - start));
        const auto distance = it.bytesDistanceFrom(startIt);
        return StringView(startIt.getIt(), distance,
                          self.hasNullTerm and (start + distance == self.sizeInBytesIncludingTerminator()),
                          self.encoding);
    }

    template <typename StringIterator>
    [[nodiscard]] static StringView sliceStart(const StringView& self, size_t start)
    {
        auto it = self.getIterator<StringIterator>();
        SC_RELEASE_ASSERT(it.advanceCodePoints(start));
        auto startIt = it;
        it.rewindToEnd();
        const auto distance = it.bytesDistanceFrom(startIt);
        return StringView(startIt.getIt(), distance,
                          self.hasNullTerm and (start + distance == self.sizeInBytesIncludingTerminator()),
                          self.encoding);
    }
};

bool SC::StringView::parseInt32(int32_t* value) const
{
    if (isIntegerNumber<StringIteratorASCII>())
    {
        if (hasNullTerm)
        {
            *value = atoi(text.data());
        }
        else
        {
            char_t buffer[12]; // 10 digits + sign + nullterm
            if (text.sizeInBytes() >= sizeof(buffer))
                return false;
            memcpy(buffer, text.data(), text.sizeInBytes());
            buffer[text.sizeInBytes()] = 0;

            *value = atoi(buffer);
        }
        return true;
    }
    else
    {
        return false;
    }
}

template <typename StringIterator>
bool SC::StringView::isIntegerNumber() const
{
    if (text.sizeInBytes() == 0)
        return false;
    StringIteratorASCII it = getIterator<StringIterator>();
    if (it.matchesAny({'-', '+'}))
    {
        (void)it.skipNext();
    }

    // From here, first is either a sign (and size > 1) or a digit
    // We just look for non-digits
    bool matchedAtLeastOneDigit = false;
    do
    {
        if (not it.matchesAny({'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}))
        {
            return false;
        }
        matchedAtLeastOneDigit = true;
    } while (it.skipNext());
    return matchedAtLeastOneDigit;
}

SC::StringComparison SC::StringView::compareASCII(StringView other) const
{
    const int res = memcmp(text.data(), other.text.data(), min(text.sizeInBytes(), other.text.sizeInBytes()));
    if (res < 0)
        return StringComparison::Smaller;
    else if (res == 0)
        return StringComparison::Equals;
    else
        return StringComparison::Bigger;
}

bool SC::StringView::startsWith(const StringView str) const
{
    if (encoding == str.encoding && str.text.sizeInBytes() <= text.sizeInBytes())
    {
        const StringView ours(text.data(), str.text.sizeInBytes(), false, encoding);
        return str == ours;
    }
    return false;
}
bool SC::StringView::endsWith(const StringView str) const
{
    if (hasCompatibleEncoding(str) && str.sizeInBytes() <= sizeInBytes())
    {
        const StringView ours(text.data() + text.sizeInBytes() - str.text.sizeInBytes(), str.text.sizeInBytes(), false,
                              encoding);
        return str == ours;
    }
    return false;
}

template <>
SC::StringView SC::StringView::sliceStartEnd<SC::StringIteratorASCII>(size_t start, size_t end) const
{
    return Internal::sliceStartEnd<SC::StringIteratorASCII>(*this, start, end);
}

/// Returns a section of a string.
/// @param start The index to the beginning of the specified portion of StringView.
template <>
SC::StringView SC::StringView::sliceStart<SC::StringIteratorASCII>(size_t start) const
{
    return Internal::sliceStart<SC::StringIteratorASCII>(*this, start);
}
