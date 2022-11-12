#pragma once
#include "Array.h"
#include "ReflectionSC.h"
#include "SerializationTestSuite.h"
#include "SerializationTypeErasedCompiler.h"
#include "Test.h"
#include "Vector.h"

namespace TestNamespace
{
struct SimpleStructure
{
    // Base Types
    SC::uint8_t  f1  = 0;
    SC::uint16_t f2  = 1;
    SC::uint32_t f3  = 2;
    SC::uint64_t f4  = 3;
    SC::int8_t   f5  = 4;
    SC::int16_t  f6  = 5;
    SC::int32_t  f7  = 6;
    SC::int64_t  f8  = 7;
    float        f9  = 8;
    double       f10 = 9;

    int arrayOfInt[3] = {1, 2, 3};
};

struct IntermediateStructure
{
    SC::Vector<int> vectorOfInt;
    SimpleStructure simpleStructure;
};

struct ComplexStructure
{
    SC::uint8_t                 f1 = 0;
    SimpleStructure             simpleStructure;
    SimpleStructure             simpleStructure2;
    SC::uint16_t                f4 = 0;
    IntermediateStructure       intermediateStructure;
    SC::Vector<SimpleStructure> vectorOfStructs;
};
} // namespace TestNamespace

namespace SC
{
namespace Reflection
{

template <>
struct MetaClass<TestNamespace::SimpleStructure> : MetaStruct<MetaClass<TestNamespace::SimpleStructure>>
{
    template <typename MemberVisitor>
    static constexpr bool visit(MemberVisitor&& builder)
    {
        SC_TRY_IF(builder(0, SC_META_MEMBER(f1)));
        SC_TRY_IF(builder(1, SC_META_MEMBER(f2)));
        SC_TRY_IF(builder(2, SC_META_MEMBER(arrayOfInt)));
        return true;
    }
};

template <>
struct MetaClass<TestNamespace::IntermediateStructure> : MetaStruct<MetaClass<TestNamespace::IntermediateStructure>>
{
    template <typename MemberVisitor>
    static constexpr bool visit(MemberVisitor&& builder)
    {
        SC_TRY_IF(builder(0, SC_META_MEMBER(simpleStructure)));
        SC_TRY_IF(builder(1, SC_META_MEMBER(vectorOfInt)));
        return true;
    }
};

template <>
struct MetaClass<TestNamespace::ComplexStructure> : MetaStruct<MetaClass<TestNamespace::ComplexStructure>>
{
    template <typename MemberVisitor>
    static constexpr bool visit(MemberVisitor&& builder)
    {
        SC_TRY_IF(builder(0, SC_META_MEMBER(f1)));
        SC_TRY_IF(builder(1, SC_META_MEMBER(simpleStructure)));
        SC_TRY_IF(builder(2, SC_META_MEMBER(simpleStructure2)));
        SC_TRY_IF(builder(3, SC_META_MEMBER(f4)));
        SC_TRY_IF(builder(4, SC_META_MEMBER(intermediateStructure)));
        SC_TRY_IF(builder(5, SC_META_MEMBER(vectorOfStructs)));
        return true;
    }
};
} // namespace Reflection
} // namespace SC

namespace SC
{
struct ReflectionTest;
}

namespace TestNamespace
{
struct PackedStructWithArray;
struct PackedStruct;
struct UnpackedStruct;
struct NestedUnpackedStruct;
struct StructWithArrayPacked;
struct StructWithArrayUnpacked;
} // namespace TestNamespace

struct TestNamespace::PackedStructWithArray
{
    SC::uint8_t arrayValue[4] = {0, 1, 2, 3};
    float       floatValue    = 1.5f;
    SC::int64_t int64Value    = -13;
};
SC_META_STRUCT_BEGIN(TestNamespace::PackedStructWithArray)
SC_META_STRUCT_MEMBER(0, arrayValue)
SC_META_STRUCT_MEMBER(1, floatValue)
SC_META_STRUCT_MEMBER(2, int64Value)
SC_META_STRUCT_END()

struct TestNamespace::PackedStruct
{
    float x, y, z;
    PackedStruct() : x(0), y(0), z(0) {}
};

SC_META_STRUCT_BEGIN(TestNamespace::PackedStruct)
SC_META_STRUCT_MEMBER(0, x);
SC_META_STRUCT_MEMBER(1, y);
SC_META_STRUCT_MEMBER(2, z);
SC_META_STRUCT_END()

struct TestNamespace::UnpackedStruct
{
    SC::int16_t x;
    float       y, z;

    UnpackedStruct()
    {
        x = 10;
        y = 2;
        z = 3;
    }
};

SC_META_STRUCT_BEGIN(TestNamespace::UnpackedStruct)
SC_META_STRUCT_MEMBER(0, x);
SC_META_STRUCT_MEMBER(1, y);
SC_META_STRUCT_MEMBER(2, z);
SC_META_STRUCT_END()

struct TestNamespace::NestedUnpackedStruct
{
    UnpackedStruct unpackedMember;
};
SC_META_STRUCT_BEGIN(TestNamespace::NestedUnpackedStruct)
SC_META_STRUCT_MEMBER(0, unpackedMember);
SC_META_STRUCT_END()

struct TestNamespace::StructWithArrayPacked
{
    PackedStruct packedMember[3];
};
SC_META_STRUCT_BEGIN(TestNamespace::StructWithArrayPacked)
SC_META_STRUCT_MEMBER(0, packedMember);
SC_META_STRUCT_END()

struct TestNamespace::StructWithArrayUnpacked
{
    NestedUnpackedStruct unpackedMember[3];
};
SC_META_STRUCT_BEGIN(TestNamespace::StructWithArrayUnpacked)
SC_META_STRUCT_MEMBER(0, unpackedMember);
SC_META_STRUCT_END()

struct SC::ReflectionTest : public SC::TestCase
{

    [[nodiscard]] static constexpr bool constexprEquals(const StringView str1, const StringView other)
    {
        if (str1.sizeInBytesWithoutTerminator() != other.sizeInBytesWithoutTerminator())
            return false;
        for (size_t i = 0; i < str1.sizeInBytesWithoutTerminator(); ++i)
            if (str1.bytesWithoutTerminator()[i] != other.bytesWithoutTerminator()[i])
                return false;
        return true;
    }
    ReflectionTest(SC::TestReport& report) : TestCase(report, "ReflectionTest")
    {
        // clang++ -Xclang -ast-dump -Xclang -ast-dump-filter=SimpleStructure -std=c++14
        // libraries/foundation/ReflectionTest.h clang -cc1 -xc++ -fsyntax-only -code-completion-at
        // libraries/Foundation/ReflectionTest.h:94:12 libraries/Foundation/ReflectionTest.h -std=c++14 echo '#include
        // "libraries/Foundation/ReflectionTest.h"\nTestNamespace::SimpleStructure\n::\n"' | clang -cc1 -xc++
        // -fsyntax-only -code-completion-at -:3:3 - -std=c++14
        using namespace SC;
        using namespace SC::Reflection;
        if (test_section("Packing"))
        {
            constexpr auto packedStructWithArray =
                FlatSchemaTypeErased::compile<TestNamespace::PackedStructWithArray>();
            constexpr auto packedStructWithArrayFlags = packedStructWithArray.properties.values[0].getCustomUint32();
            static_assert(packedStructWithArrayFlags & MetaStructFlags::IsPacked,
                          "nestedPacked struct should be recursively packed");

            constexpr auto packedStruct      = FlatSchemaTypeErased::compile<TestNamespace::PackedStruct>();
            constexpr auto packedStructFlags = packedStruct.properties.values[0].getCustomUint32();
            static_assert(packedStructFlags & MetaStructFlags::IsPacked,
                          "nestedPacked struct should be recursively packed");

            constexpr auto unpackedStruct      = FlatSchemaTypeErased::compile<TestNamespace::UnpackedStruct>();
            constexpr auto unpackedStructFlags = unpackedStruct.properties.values[0].getCustomUint32();
            static_assert(not(unpackedStructFlags & MetaStructFlags::IsPacked),
                          "Unpacked struct should be recursively packed");

            constexpr auto nestedUnpackedStruct = FlatSchemaTypeErased::compile<TestNamespace::NestedUnpackedStruct>();
            constexpr auto nestedUnpackedStructFlags = nestedUnpackedStruct.properties.values[0].getCustomUint32();
            static_assert(not(nestedUnpackedStructFlags & MetaStructFlags::IsPacked),
                          "nestedPacked struct should not be recursiely packed");

            constexpr auto structWithArrayPacked =
                FlatSchemaTypeErased::compile<TestNamespace::StructWithArrayPacked>();
            constexpr auto structWithArrayPackedFlags = structWithArrayPacked.properties.values[0].getCustomUint32();
            static_assert(structWithArrayPackedFlags & MetaStructFlags::IsPacked,
                          "structWithArrayPacked struct should not be recursiely packed");

            constexpr auto structWithArrayUnpacked =
                FlatSchemaTypeErased::compile<TestNamespace::StructWithArrayUnpacked>();
            constexpr auto structWithArrayUnpackedFlags =
                structWithArrayUnpacked.properties.values[0].getCustomUint32();
            static_assert(not(structWithArrayUnpackedFlags & MetaStructFlags::IsPacked),
                          "structWithArrayUnpacked struct should not be recursiely packed");
        }
        if (test_section("Print Complex structure"))
        {
            constexpr auto       className    = TypeToString<TestNamespace::ComplexStructure>::get();
            constexpr StringView classNameStr = "TestNamespace::ComplexStructure";
            static_assert(constexprEquals(StringView(className.data, className.length, false), classNameStr),
                          "Please update SC::ClNm for your compiler");
            // auto numlinks =
            // countUniqueLinks<10>(MetaClass<TestNamespace::ComplexStructure>::template
            // getAtoms<10>());
            // SC_RELEASE_ASSERT(numlinks == 3);
            constexpr auto ComplexStructureFlatSchema =
                FlatSchemaTypeErased::compile<TestNamespace::ComplexStructure>();
            printFlatSchema(ComplexStructureFlatSchema.properties.values, ComplexStructureFlatSchema.names.values);
            // constexpr auto SimpleStructureFlatSchema =
            // FlatSchemaTypeErased::compile<TestNamespace::SimpleStructure>();
            // printFlatSchema(SimpleStructureFlatSchema.atoms.values,
            // SimpleStructureFlatSchema.names.values);
        }
    }
};
