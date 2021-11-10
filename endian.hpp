/*************************************************************************
    > File Name: endian.hpp
    > Author: hsz
    > Mail:
    > Created Time: Mon 18 Oct 2021 03:58:17 PM CST
 ************************************************************************/

#ifndef __ENDIAN_HPP__
#define __ENDIAN_HPP__

#include <endian.h>     // for BYTE_ORDER
#include <byteswap.h>
#include <stdint.h>
#include <type_traits>  // for enable_if

#define JARVIS_BIG_ENDIAN       1   // 大端
#define JARVIS_LITTLE_ENDIAN    2   // 小端

#if BYTE_ORDER == BIG_ENDIAN
#define JARVIS_BYTE_ORDER JARVIS_BIG_ENDIAN
#else
#define JARVIS_BYTE_ORDER JARVIS_LITTLE_ENDIAN
#endif

namespace Jarvis {
// 8字节类型转换
template<typename T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value)
{
    return (T)bswap_64((uint64_t)value);
}

// 4字节类型转换
template<typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value)
{
    return (T)bswap_32((uint32_t)value);
}

// 2字节类型转换
template<typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value)
{
    return (T)bswap_16((uint16_t)value);
}

#if JARVIS_BYTE_ORDER == JARVIS_BIG_ENDIAN
template<typename T>
T LittleEndian2BigEndian(T value)
{
    return value;
}

template<typename T>
T BigEndian2LittleEndian(T value)
{
    return byteswap(value);
}

#else

// 将value转换为大端字节数，在小端机执行byteswap
template<typename T>
T LittleEndian2BigEndian(T value)
{
    return byteswap(value);
}

// 将value转换为小端字节数，在小端机直接返回
template<typename T>
T BigEndian2LittleEndian(T value)
{
    return value;
}
#endif

} // namespace Jarvis

#endif // __ENDIAN_HPP__