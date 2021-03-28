#pragma once

#include "types.h"
#include "utildef.h"
#include <cassert>

namespace utils
{

template <typename Integer>
inline void Invert(Integer& value) {
	STATIC_ASSERT(1);
}

template <>
inline void Invert<uint32>(uint32& value) {
	value = ((value & 0x000000FF) << 24)
	      | ((value & 0x0000FF00) << 8)
	      | ((value & 0x00FF0000) >> 8)
	      | ((value & 0xFF000000) >> 24);
}

template <>
inline void Invert<int32>(int32& value) {
	value = ((value & 0x000000FF) << 24)
	      | ((value & 0x0000FF00) << 8)
	      | ((value & 0x00FF0000) >> 8)
	      | ((value & 0xFF000000) >> 24);
}

template <>
inline void Invert<uint16>(uint16& value) {
	value = ((value & 0x00FF) << 8)
	      | ((value & 0xFF00) >> 8);
}

template <>
inline void Invert<int16>(int16& value) {
	value = ((value & 0x00FF) << 8)
	      | ((value & 0xFF00) >> 8);
}

template <typename Integer>
inline Integer GetInvert(Integer value) {
	Invert(value);
	return value;
}

template <typename SwapType>
void ClearMemory(SwapType& object) {
	SwapType emptyObject;
	object.swap(emptyObject);
}

template <typename CharType>
void FillStringWithRandomNumbers(CharType* cstr, size_t count) {
	assert(cstr != 0);
	assert(count != 0);
	static const CharType numbers[10] = {'0','1','2','3','4','5','6','7','8','9'};
	while (count != 0) {
		const int val = ::rand() % ARRAY_SIZE(numbers);
		cstr[--count] = numbers[val];
	}
}

template <typename CharType, size_t Size>
void FillStringWithRandomNumbers(CharType (&cstr)[Size]) {
	cstr[Size - 1] = '\0';
	FillStringWithRandomNumbers(cstr, Size - 1);
}

template <typename StringType>
StringType AppendRandomNumbers(const StringType& str, size_t count)
{
	StringType newStr;
	newStr.reserve(str.size() + count);
	newStr.append(str);
	newStr.append(count, '0');
	typename StringType::value_type* numbers = &newStr[0] + str.size();
	FillStringWithRandomNumbers(numbers, count);
	return newStr;
}

}
