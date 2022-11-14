#pragma once
#include "Compiler.h"
#include "Types.h"

namespace SC
{
template <bool B, class T = void>
struct EnableIf
{
};

template <class T>
struct EnableIf<true, T>
{
    typedef T type;
};

template <typename T, typename U>
struct IsSame
{
    static constexpr bool value = false;
};

template <typename T>
struct IsSame<T, T>
{
    static constexpr bool value = true;
};

template <class T>
struct RemoveReference
{
    typedef T type;
};
template <class T>
struct RemoveReference<T&>
{
    typedef T type;
};
template <class T>
struct RemoveReference<T&&>
{
    typedef T type;
};

template <class T>
struct RemoveConst
{
    typedef T type;
};
template <class T>
struct RemoveConst<const T>
{
    typedef T type;
};

template <class T, T v>
struct IntegralConstant
{
    static constexpr T value = v;

    using value_type = T;
    using type       = IntegralConstant;

    constexpr            operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};
using true_type  = IntegralConstant<bool, true>;
using false_type = IntegralConstant<bool, false>;

template <typename T>
struct IsConst : public false_type
{
    typedef T type;
};
template <typename T>
struct IsConst<const T> : public true_type
{
};

template <typename T>
struct IsUnion : false_type
{
}; // TODO: This needs compiler instrinsics
template <class T>
IntegralConstant<bool, !IsUnion<T>::value> isClassTest(char T::*);
template <class>
false_type isClassTest(...);

template <class T>
struct IsClass : decltype(isClassTest<T>(nullptr))
{
};

template <typename T>
struct IsArray : false_type
{
};
template <typename T, int N>
struct IsArray<T[N]> : true_type
{
    typedef T type;
};
template <typename _Tp>
struct IsTriviallyCopyable : public IntegralConstant<bool, __is_trivially_copyable(_Tp)>
{
};
template <class T>
struct is_lvalue_reference : false_type
{
};
template <class T>
struct is_lvalue_reference<T&> : true_type
{
};
template <class T>
struct is_rvalue_reference : false_type
{
};
template <class T>
struct is_rvalue_reference<T&&> : true_type
{
};
template <class T>
struct IsReference : IntegralConstant<bool, is_lvalue_reference<T>::value || is_rvalue_reference<T>::value>
{
};

template <bool B, class T, class F>
struct Conditional
{
    using type = T;
};

template <class T, class F>
struct Conditional<false, T, F>
{
    using type = F;
};

template <typename SourceType, typename T>
struct SameConstnessAs
{
    using type = typename Conditional<IsConst<SourceType>::value, const T, T>::type;
};

template <typename T>
struct ReferenceWrapper
{
    const typename RemoveReference<T>::type* ptr;

    ReferenceWrapper(typename RemoveReference<T>::type& other) : ptr(&other) {}
    ~ReferenceWrapper() {}
    operator const T&() const { return *ptr; }
    operator T&() { return *ptr; }
};
template <typename T>
constexpr T&& move(T& value)
{
    return static_cast<T&&>(value);
}

template <typename T>
constexpr T&& forward(T& value)
{
    return static_cast<typename RemoveReference<T>::type&&>(value);
}
struct PlacementNew
{
};

template <typename T>
constexpr T min(T t1, T t2)
{
    return t1 < t2 ? t1 : t2;
}

template <typename T>
constexpr T max(T t1, T t2)
{
    return t1 > t2 ? t1 : t2;
}

template <typename T>
void swap(T& t1, T& t2)
{
    T temp = move(t1);
    t1     = move(t2);
    t2     = move(temp);
}
template <typename T>
struct SmallerThan
{
    bool operator()(const T& a, const T& b) { return a < b; }
};

template <typename Iterator, typename Comparison>
void bubbleSort(Iterator first, Iterator last, Comparison comparison)
{
    if (first >= last)
    {
        return;
    }
    bool doSwap = true;
    while (doSwap)
    {
        doSwap      = false;
        Iterator p0 = first;
        Iterator p1 = first + 1;
        while (p1 != last)
        {
            if (comparison(*p1, *p0))
            {
                swap(*p1, *p0);
                doSwap = true;
            }
            ++p0;
            ++p1;
        }
    }
}

template <typename T, size_t N>
constexpr size_t ConstantArraySize(const T (&text)[N])
{
    return N;
}
template <unsigned int N, unsigned int I>
struct FnvHash
{
    static constexpr uint32_t Hash(const char (&str)[N])
    {
        return (FnvHash<N, I - 1>::Hash(str) ^ str[I - 1]) * 16777619u;
    }
};

template <uint32_t N>
struct FnvHash<N, 1>
{
    static constexpr uint32_t Hash(const char (&str)[N]) { return (2166136261u ^ str[0]) * 16777619u; }
};

template <unsigned int N>
constexpr uint32_t StringHash(const char (&str)[N])
{
    return FnvHash<N, N>::Hash(str);
}

template <typename... uint32_t>
constexpr auto CombineHash(uint32_t... hash1)
{
    char parts2[sizeof...(uint32_t)][4] = {
        {static_cast<char>((hash1 & 0xff) >> 0), static_cast<char>((hash1 & 0xff00) >> 8),
         static_cast<char>((hash1 & 0xff0000) >> 16), static_cast<char>((hash1 & 0xff0000) >> 24)}...};
    char combinedChars[sizeof(parts2)] = {0};
    for (int i = 0; i < sizeof...(uint32_t); ++i)
    {
        combinedChars[i * 4 + 0] = parts2[i][0];
        combinedChars[i * 4 + 1] = parts2[i][1];
        combinedChars[i * 4 + 2] = parts2[i][2];
        combinedChars[i * 4 + 3] = parts2[i][3];
    }
    return StringHash(combinedChars);
}

} // namespace SC

#if SC_MSVC
inline void* operator new(size_t, void* p, SC::PlacementNew) noexcept { return p; }
inline void  operator delete(void* p, SC::PlacementNew) noexcept {}
#else
inline void* operator new(SC::size_t, void* p, SC::PlacementNew) noexcept { return p; }
#endif

#if SC_MSVC
#define SC_CPLUSPLUS _MSVC_LANG
#else
#define SC_CPLUSPLUS __cplusplus
#endif

#if SC_CPLUSPLUS >= 202002L

#define SC_CPP_AT_LEAST_20 1
#define SC_CPP_AT_LEAST_17 1
#define SC_CPP_AT_LEAST_14 1

#elif SC_CPLUSPLUS >= 201703L

#define SC_CPP_AT_LEAST_20 0
#define SC_CPP_AT_LEAST_17 1
#define SC_CPP_AT_LEAST_14 1

#elif SC_CPLUSPLUS >= 201402L

#define SC_CPP_AT_LEAST_20 0
#define SC_CPP_AT_LEAST_17 0
#define SC_CPP_AT_LEAST_14 1

#else

#define SC_CPP_AT_LEAST_20 0
#define SC_CPP_AT_LEAST_17 0
#define SC_CPP_AT_LEAST_14 0

#endif

// Using placement new in costructor is C++ 14+
#if SC_CPP_AT_LEAST_14
#define SC_CONSTEXPR_CONSTRUCTOR_NEW constexpr
#else
#define SC_CONSTEXPR_CONSTRUCTOR_NEW
#endif

// Defining a constexpr destructor is C++ 20+
#if SC_CPP_AT_LEAST_20
#define SC_CONSTEXPR_DESTRUCTOR constexpr
#else
#define SC_CONSTEXPR_DESTRUCTOR
#endif

#if (!SC_MSVC) || SC_CPP_AT_LEAST_20
#define SC_LIKELY   [[likely]]
#define SC_UNLIKELY [[unlikely]]
#else
#define SC_LIKELY
#define SC_UNLIKELY
#endif