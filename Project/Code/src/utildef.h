
#pragma once

#define BIT(x)   (1<<(x))

// const wchar_t* var = W("Text");
#define W_0(x) L ## x
#define W(x)   W_0(x)

// int array[3];
// size_t size = ARRAY_SIZE(array);
#define ARRAY_SIZE(arr)   sizeof(*Detail::ArraySizeHelper(arr))

// STATIC_ASSERT(1 == 1);
#define STATIC_ASSERT(condition)   Detail::StaticAssert<condition>::apply()


namespace Detail
{
	template <typename Type, size_t Size>
	char (*ArraySizeHelper(Type (&)[Size]))[Size];

	template<bool condition>
	struct StaticAssert {
		static void apply() { static const char junk[condition ? 1 : -1]; }
	};

	template<>
	struct StaticAssert<true> {
		static void apply() {}
	};
}
