#pragma once

#include <type_traits>
#include <stddef.h>
#include <stdint.h>

using FormatCallback = bool (*)(void* data, char character);

namespace impl {

#if defined (MICRO_FORMAT_DOUBLE)
	using FloatType = double;
#elif defined (MICRO_FORMAT_FLOAT)
	using FloatType = float;
#endif

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

struct DstData
{
	const FormatCallback callback;
	void* const          data;
	size_t               chars_printed;
};

struct FormatCtx
{
	DstData dst;
	const FormatArg* const args;
	const int              args_count;
};

void format_impl(FormatCtx& ctx, const char* format);

// callback data for printing into string buffer
struct SFormatData
{
	char* buffer;
	size_t buffer_size;
};

bool s_format_callback(void* data, char character);

template <typename PrintFun>
size_t s_format_impl(char* buffer, size_t buffer_size, const PrintFun &print_fun)
{
	if (buffer_size == 0) return 0;
	impl::SFormatData data = { buffer, buffer_size - 1 };
	auto result = print_fun(data);
	buffer[result] = 0;
	return result;
}

} // namespace impl

///////////////////////////////////////////////////////////////////////////////


// Print values formating by {} syntax calling callback for each character
template <typename ... Args>
size_t cb_format(FormatCallback callback, void* data, const char* format, const Args& ... args)
{
	constexpr unsigned arr_size = (sizeof ... (args)) ? (sizeof ... (args)) : 1;
	const impl::FormatArg args_arr[arr_size] = { args ... };
	impl::FormatCtx ctx{ { callback, data }, args_arr, sizeof ... (args) };
	impl::format_impl(ctx, format);
	return ctx.dst.chars_printed;
}

// Print values formating by {} syntax into buffer
template <typename ... Args>
size_t s_format(char* buffer, size_t buffer_size, const char* format, const Args& ... args)
{
	return impl::s_format_impl(
		buffer, 
		buffer_size, 
		[&](auto& data) { return cb_format(impl::s_format_callback, &data, format, args...); }
	);
}

// Print values formating by {} syntax into constant-sized buffer
template <typename ... Args, size_t BufSize>
size_t s_format(char (&buffer)[BufSize], const char* format, const Args& ... args)
{
	return s_format(buffer, BufSize, format, args...);
}

// Print integer as decimal value calling callback for each character
size_t cb_format_int(FormatCallback callback, void* data, int value);

// Print integer as decimal value into buffer
size_t s_format_int(char* buffer, size_t buffer_size, int value);

// Print integer as decimal value into constant-sized buffer
template <typename ... Args, size_t BufSize>
size_t s_format_int(char(&buffer)[BufSize], int value)
{
	return s_format_int(buffer, BufSize, value);
}

// Print unsigned integer as decimal value calling callback for each character
size_t cb_format_uint(FormatCallback callback, void* data, unsigned value);

// Print unsigned integer as decimal value into buffer
size_t s_format_uint(char* buffer, size_t buffer_size, unsigned value);

// Print unsigned integer as decimal value into constant-sized buffer
template <typename ... Args, size_t BufSize>
size_t s_format_uint(char(&buffer)[BufSize], unsigned value)
{
	return s_format_uint(buffer, BufSize, value);
}

// Print unsigned integer as hexadecimal value calling callback for each character
size_t cb_format_hex(FormatCallback callback, void* data, unsigned value);

// Print unsigned integer as hexadecimal value into buffer
size_t s_format_hex(char* buffer, size_t buffer_size, unsigned value);

// Print unsigned integer as hexadecimal value into constant-sized buffer
template <typename ... Args, size_t BufSize>
size_t s_format_hex(char(&buffer)[BufSize], unsigned value)
{
	return s_format_hex(buffer, BufSize, value);
}

// Print unsigned integer as binary value calling callback for each character
size_t cb_format_bin(FormatCallback callback, void* data, unsigned value);

// Print unsigned integer as binary value value into buffer
size_t s_format_bin(char* buffer, size_t buffer_size, unsigned value);

// Print unsigned integer as binary value into constant-sized buffer
template <typename ... Args, size_t BufSize>
size_t s_format_bin(char(&buffer)[BufSize], unsigned value)
{
	return s_format_bin(buffer, BufSize, value);
}


#if defined (MICRO_FORMAT_DOUBLE) || defined (MICRO_FORMAT_FLOAT)

// Print floating point number calling callback for each character
size_t cb_format_float(FormatCallback callback, void* data, impl::FloatType value, int precision);

// Print floating point number into buffer
size_t s_format_float(char* buffer, size_t buffer_size, impl::FloatType value, int precision);

// Print floating point number into constant-sized buffer
template <typename ... Args, size_t BufSize>
size_t s_format_float(char(&buffer)[BufSize], impl::FloatType value, int precision)
{
	return s_format_float(buffer, BufSize, value, precision);
}


#endif
