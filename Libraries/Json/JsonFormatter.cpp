// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#include "JsonFormatter.h"
#include "../Containers/Vector.h"
#include "../Strings/StringFormat.h"
#include "../Strings/StringView.h"

#include <stdio.h> // snprintf
#include <string.h>

SC::JsonFormatter::JsonFormatter(Vector<State>& state, StringFormatOutput& output) : state(state), output(output)
{
#if SC_COMPILER_MSVC || SC_COMPILER_CLANG_CL
    strcpy_s(floatFormat, "%.2f");
#else
    strcpy(floatFormat, "%.2f");
#endif
}

bool SC::JsonFormatter::writeSeparator()
{
    if (not state.isEmpty())
    {
        bool  printComma = true;
        auto& currState  = state.back();
        if (currState == ArrayFirst)
        {
            currState  = Array;
            printComma = false;
        }
        else if (currState == ObjectFirst)
        {
            currState  = Object;
            printComma = false;
        }
        else if (currState == ObjectValue)
        {
            printComma = false;
        }
        if (printComma)
        {
            return output.append(","_a8);
        }
    }
    return true;
}
bool SC::JsonFormatter::setOptions(Options opt)
{
    options       = opt;
    const int res = snprintf(floatFormat, sizeof(floatFormat), "%%.%df", options.floatDigits);

    return res > 0;
}

bool SC::JsonFormatter::onBeforeValue()
{
    if (runState == BeforeStart)
    {
        output.onFormatBegin();
        runState = Running;
    }
    else
    {
        if (state.isEmpty())
        {
            return false;
        }
        auto currState = state.back();
        if (currState != ObjectValue and currState != Array and currState != ArrayFirst)
        {
            return false;
        }
    }
    return true;
}

bool SC::JsonFormatter::onAfterValue()
{
    if (state.isEmpty())
    {
        runState = AfterEnd;
        return output.onFormatSucceded();
    }
    if (state.back() == ObjectValue)
    {
        return state.pop_back();
    }
    return true;
}

bool SC::JsonFormatter::writeValue(int writtenChars, const char* buffer)
{
    if (not onBeforeValue() or not writeSeparator())
    {
        return false;
    }
    if (writtenChars > 0)
    {
        if (output.append(StringView(buffer, static_cast<size_t>(writtenChars), true, StringEncoding::Ascii)))
        {
            return onAfterValue();
        }
    }
    return false;
}

bool SC::JsonFormatter::writeFloat(float value)
{
    char buffer[100];
    return writeValue(snprintf(buffer, sizeof(buffer), floatFormat, value), buffer);
}

bool SC::JsonFormatter::writeDouble(double value)
{
    char buffer[100];
    return writeValue(snprintf(buffer, sizeof(buffer), floatFormat, value), buffer);
}

bool SC::JsonFormatter::writeInt8(int8_t value)
{
    char buffer[30];
    return writeValue(snprintf(buffer, sizeof(buffer), "%d", value), buffer);
}

bool SC::JsonFormatter::writeUint8(uint8_t value)
{
    char buffer[30];
    return writeValue(snprintf(buffer, sizeof(buffer), "%d", value), buffer);
}

bool SC::JsonFormatter::writeInt32(int32_t value)
{
    char buffer[30];
    return writeValue(snprintf(buffer, sizeof(buffer), "%d", value), buffer);
}

bool SC::JsonFormatter::writeUint32(uint32_t value)
{
    char buffer[30];
    return writeValue(snprintf(buffer, sizeof(buffer), "%d", value), buffer);
}

bool SC::JsonFormatter::writeInt64(int64_t value)
{
    char buffer[30];
    return writeValue(snprintf(buffer, sizeof(buffer), "%lld", value), buffer);
}

bool SC::JsonFormatter::writeUint64(uint64_t value)
{
    char buffer[30];
    return writeValue(snprintf(buffer, sizeof(buffer), "%lld", value), buffer);
}

bool SC::JsonFormatter::writeBoolean(bool value) { return writeValue(value ? 4 : 5, value ? "true" : "false"); }

bool SC::JsonFormatter::writeNull() { return writeValue(4, "null"); }

bool SC::JsonFormatter::writeString(StringView value)
{
    if (not onBeforeValue() or not writeSeparator())
    {
        return false;
    }
    // TODO: Escape json string
    if (output.append("\""_a8) and output.append(value) and output.append("\""_a8))
    {
        return onAfterValue();
    }
    return false;
}

bool SC::JsonFormatter::startArray()
{
    if (not onBeforeValue() or not writeSeparator())
    {
        return false;
    }
    if (output.append("["_a8))
    {
        return state.push_back(ArrayFirst);
    }
    return false;
}

bool SC::JsonFormatter::endArray()
{
    if (state.isEmpty() or ((state.back() != Array) and (state.back() != ArrayFirst)))
    {
        return false;
    }
    if (output.append("]"_a8))
    {
        if (state.pop_back())
        {
            return onAfterValue();
        }
    }
    return false;
}

bool SC::JsonFormatter::startObject()
{
    if (not onBeforeValue() or not writeSeparator())
    {
        return false;
    }

    if (output.append("{"_a8))
    {
        return state.push_back(ObjectFirst);
    }
    return false;
}

bool SC::JsonFormatter::endObject()
{
    if (state.isEmpty() or ((state.back() != Object) and (state.back() != ObjectFirst)))
    {
        return false;
    }
    if (output.append("}"_a8))
    {
        if (state.pop_back())
        {
            return onAfterValue();
        }
    }
    return false;
}

bool SC::JsonFormatter::startObjectField(StringView name)
{
    if (state.isEmpty() or ((state.back() != Object) and (state.back() != ObjectFirst)))
    {
        return false;
    }
    if (state.back() == Object)
    {
        if (not output.append(","_a8))
        {
            return false;
        }
    }
    else
    {
        state.back() = Object;
    }
    // TODO: Escape JSON name
    if (output.append("\""_a8) and output.append(name) and output.append("\":"_a8))
    {
        return state.push_back(ObjectValue);
    }
    return false;
}