/* C++ library for std::format-like text formating for microcontrollers
   https://github.com/art-den/micro_format

   MIT License

   Copyright (c) 2020-2021 Artyomov Denis (denis.artyomov@gmail.com)

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE. */


#pragma once

#include <type_traits>
#include <stddef.h>
#include <stdint.h>

namespace mf {

using WideChar = uint32_t;

using FormatCallback = bool (*)(void* data, char character);
using FormatWideCallback = bool (*)(void* data, WideChar character);

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
	LLong,
	ULLong,
};

struct FormatArg
{
	const volatile void* pointer = nullptr;
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

	FormatArg(const volatile char           &v) : pointer(&v), type(FormatArgType::Char) {}
	FormatArg(const volatile unsigned char  &v) : pointer(&v), type(FormatArgType::UChar) {}
	FormatArg(const volatile short          &v) : pointer(&v), type(FormatArgType::Short) {}
	FormatArg(const volatile unsigned short &v) : pointer(&v), type(FormatArgType::UShort) {}
	FormatArg(const volatile int            &v) : pointer(&v), type(FormatArgType::Int) {}
	FormatArg(const volatile unsigned int   &v) : pointer(&v), type(FormatArgType::UInt) {}
	FormatArg(const volatile long           &v) : pointer(&v), type(FormatArgType::Long) {}
	FormatArg(const volatile unsigned long  &v) : pointer(&v), type(FormatArgType::ULong) {}
	FormatArg(const volatile bool           &v) : pointer(&v), type(FormatArgType::Bool) {}


	// nullptr as default parameter is used to create one element array args_arr if arguments is empty
	FormatArg(const char  *v = nullptr) : pointer(v),  type(FormatArgType::CharPtr) {}

	template <typename T>
	FormatArg(const T *v) : pointer(v), type(FormatArgType::Pointer) {}

	template <typename T>
	FormatArg(const volatile T* v) : pointer(v), type(FormatArgType::Pointer) {}

#if defined (MICRO_FORMAT_FLOAT) || defined (MICRO_FORMAT_DOUBLE)
	FormatArg(const float &v) : pointer(&v), type(FormatArgType::Float) {}
	FormatArg(const volatile float &v) : pointer((const void*)&v), type(FormatArgType::Float) {}
#endif
#if defined (MICRO_FORMAT_DOUBLE)
	FormatArg(const double &v) : pointer(&v), type(FormatArgType::Double) {}
	FormatArg(const volatile double &v) : pointer((const void*)&v), type(FormatArgType::Double) {}
#endif
#if defined(MICRO_FORMAT_INT64)
	FormatArg(const volatile long long& v) : pointer((const void*)&v), type(FormatArgType::LLong) {}
	FormatArg(const long long& v) : pointer(&v), type(FormatArgType::LLong) {}
	FormatArg(const volatile unsigned long long& v) : pointer((const void*)&v), type(FormatArgType::ULLong) {}
	FormatArg(const unsigned long long& v) : pointer(&v), type(FormatArgType::ULLong) {}
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

void format_impl(FormatCtx& ctx, const char* format_str);

// callback data for printing into string buffer
struct FormatBufData
{
	char* buffer;
	size_t buffer_size;
};

bool format_buf_callback(void* data, char character);

template <typename PrintFun>
size_t format_buf_impl(char* buffer, size_t buffer_size, const PrintFun &print_fun)
{
	if (buffer_size == 0) return 0;
	impl::FormatBufData data = { buffer, buffer_size - 1 };
	auto result = print_fun(data);
	buffer[result] = 0;
	return result;
}

struct Utf8Receiver
{
	FormatWideCallback cb = nullptr;
	void* cb_data = nullptr;

	WideChar character = 0;
	uint8_t count = 0;
	WideChar wrong_char = '?';
	size_t chars_printed = 0;
};

bool utf8_char_callback(void* data, char chr);

} // namespace impl

class BufferPrinter
{
public:
	template <size_t BufSize>
	BufferPrinter(char (&buffer)[BufSize]) :
		buf_ptr_(buffer),
		free_space_(BufSize)
	{}

	char* get_buf()
	{
		return buf_ptr_;
	}

	size_t get_free_buf_space()
	{
		return free_space_;
	}

	void reduce(size_t len)
	{
		if (len > free_space_) len = free_space_;
		buf_ptr_ += len;
		free_space_ -= len;
	}

private:
	char* buf_ptr_ = nullptr;
	size_t free_space_ = 0;
};

///////////////////////////////////////////////////////////////////////////////


// Print values formating by {} syntax calling callback for each character
template <typename ... Args>
size_t format(FormatCallback callback, void* data, const char* format_str, const Args& ... args)
{
	constexpr unsigned arr_size = (sizeof ... (args)) ? (sizeof ... (args)) : 1;
	const impl::FormatArg args_arr[arr_size] = { args ... };
	impl::FormatCtx ctx{ { callback, data, 0 }, args_arr, sizeof ... (args) };
	impl::format_impl(ctx, format_str);
	return ctx.dst.chars_printed;
}

// Print values formating by {} syntax calling callback for each wide character
// format_str and string arguments must be in utf8 enconding
// Return value is number of wide chars printed in function
template <typename ... Args>
size_t format_u8(FormatWideCallback callback, void* data, const char* format_str_utf8, const Args& ... args)
{
	impl::Utf8Receiver utf8 = { callback, data, 0, 0, '?', 0 };
	format(impl::utf8_char_callback, &utf8, format_str_utf8, args...);
	return utf8.chars_printed;
}

// Print values formating by {} syntax into buffer
template <typename ... Args>
size_t format(char* buffer, size_t buffer_size, const char* format_str, const Args& ... args)
{
	return impl::format_buf_impl(
		buffer,
		buffer_size,
		[&](auto& data) { return format(impl::format_buf_callback, &data, format_str, args...); }
	);
}

// Print values formating by {} syntax into constant-sized buffer
template <typename ... Args, size_t BufSize>
size_t format(char (&buffer)[BufSize], const char* format_str, const Args& ... args)
{
	return format(buffer, BufSize, format_str, args...);
}

// Append text into buffer
template <typename ... Args>
size_t format(BufferPrinter &buf_printer, const char* format_str, const Args& ... args)
{
	size_t size = impl::format_buf_impl(
		buf_printer.get_buf(),
		buf_printer.get_free_buf_space(),
		[&](auto& data) { return format(impl::format_buf_callback, &data, format_str, args...); }
	);

	buf_printer.reduce(size);

	return size;
}

// Print integer as decimal value calling callback for each character
size_t format_dec(FormatCallback callback, void* data, int value);

// Print integer as decimal value into buffer
size_t format_dec(char* buffer, size_t buffer_size, int value);

// Print integer as decimal value into constant-sized buffer
template <size_t BufSize>
size_t format_dec(char(&buffer)[BufSize], int value)
{
	return format_dec(buffer, BufSize, value);
}

// Print unsigned integer as decimal value calling callback for each character
size_t format_dec(FormatCallback callback, void* data, unsigned value);

// Print unsigned integer as decimal value into buffer
size_t format_dec(char* buffer, size_t buffer_size, unsigned value);

// Print unsigned integer as decimal value into constant-sized buffer
template <size_t BufSize>
size_t format_dec(char(&buffer)[BufSize], unsigned value)
{
	return format_dec(buffer, BufSize, value);
}

// Print unsigned integer as hexadecimal value calling callback for each character
size_t format_hex(FormatCallback callback, void* data, unsigned value);

// Print unsigned integer as hexadecimal value into buffer
size_t format_hex(char* buffer, size_t buffer_size, unsigned value);

// Print unsigned integer as hexadecimal value into constant-sized buffer
template <size_t BufSize>
size_t format_hex(char(&buffer)[BufSize], unsigned value)
{
	return format_hex(buffer, BufSize, value);
}

// Print unsigned integer as binary value calling callback for each character
size_t format_bin(FormatCallback callback, void* data, unsigned value);

// Print unsigned integer as binary value value into buffer
size_t format_bin(char* buffer, size_t buffer_size, unsigned value);

// Print unsigned integer as binary value into constant-sized buffer
template <size_t BufSize>
size_t format_bin(char(&buffer)[BufSize], unsigned value)
{
	return format_bin(buffer, BufSize, value);
}

#if defined (MICRO_FORMAT_DOUBLE) || defined (MICRO_FORMAT_FLOAT)

// Print floating point number calling callback for each character
size_t format_float(FormatCallback callback, void* data, impl::FloatType value, int precision);

// Print floating point number into buffer
size_t format_float(char* buffer, size_t buffer_size, impl::FloatType value, int precision);

// Print floating point number into constant-sized buffer
template <size_t BufSize>
size_t format_float(char(&buffer)[BufSize], impl::FloatType value, int precision)
{
	return format_float(buffer, BufSize, value, precision);
}

#endif

} // namespace mf

