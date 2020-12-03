#pragma once

#include <type_traits>
#include <stddef.h>
#include <stdint.h>

using FormatCallback = bool (*)(void* data, char character);

namespace impl {

enum class FormatArgType : uint8_t
{
	Undef,
	Char,
	UChar,
	Short,
	UShort,
	Int,
	UInt,
	Long,
	ULong,
	Bool,
	CharPtr,
	Pointer,
	Float,
	Double,
};

struct FormatArg
{
	const void* pointer = nullptr;
	FormatArgType type = {};

	FormatArg(const char           &v) : pointer(&v), type(FormatArgType::Char) {}
	FormatArg(const unsigned char  &v) : pointer(&v), type(FormatArgType::UChar) {}
	FormatArg(const short          &v) : pointer(&v), type(FormatArgType::Short) {}
	FormatArg(const unsigned short &v) : pointer(&v), type(FormatArgType::UShort) {}
	FormatArg(const int            &v) : pointer(&v), type(FormatArgType::Int) {}
	FormatArg(const unsigned int   &v) : pointer(&v), type(FormatArgType::UInt) {}
	FormatArg(const long           &v) : pointer(&v), type(FormatArgType::Long) {}
	FormatArg(const unsigned long  &v) : pointer(&v), type(FormatArgType::ULong) {}
	FormatArg(const bool           &v) : pointer(&v), type(FormatArgType::Bool) {}

	// nullptr as default parameter is used to create one element array args_arr if arguments is empty
	FormatArg(const char           *v = nullptr) : pointer(v),  type(FormatArgType::CharPtr) {} 

	template <typename T>
	FormatArg(const T              *v) : pointer(v),  type(FormatArgType::Pointer) {}

#if defined (MICRO_FORMAT_FLOAT) || defined (MICRO_FORMAT_DOUBLE)
	FormatArg(const float          &v) : pointer(&v), type(FormatArgType::Float) {}
#endif
#if defined (MICRO_FORMAT_DOUBLE)
	FormatArg(const double         &v) : pointer(&v), type(FormatArgType::Double) {}
#endif
};

struct FormatCtx
{
	FormatCallback callback;
	void *data;
	const FormatArg* args;
	int args_count;
	size_t chars_printed;
};

void format_impl(FormatCtx& ctx, const char* format);

struct SFormatData
{
	char* buffer;
	size_t buffer_size;
};

bool s_format_callback(void* data, char character);

} // namespace impl

template <typename ... Args>
size_t cb_format(FormatCallback callback, void* data, const char* format, const Args& ... args)
{
	constexpr unsigned arr_size = (sizeof ... (args)) ? (sizeof ... (args)) : 1;
	const impl::FormatArg args_arr[arr_size] = { args ... };
	impl::FormatCtx ctx { callback, data, args_arr, sizeof ... (args) };
	impl::format_impl(ctx, format);
	return ctx.chars_printed;
}

template <typename ... Args>
size_t s_format(char* buffer, size_t buffer_size, const char* format, const Args& ... args)
{
	if (buffer_size == 0) return 0;
	impl::SFormatData data = { buffer, buffer_size - 1 };
	auto result = cb_format(impl::s_format_callback, &data, format, args...);
	buffer[result] = 0;
	return result + 1;
}

template <typename ... Args, size_t BufSize>
size_t s_format(char (&buffer)[BufSize], const char* format, const Args& ... args)
{
	return s_format(buffer, BufSize, format, args...);
}
