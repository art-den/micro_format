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

#include <limits>
#include <math.h>
#include <stdint.h>
#include "micro_format.hpp"

namespace mf {
namespace impl {

#if defined (MICRO_FORMAT_DOUBLE)
#define MODF modf
#elif defined (MICRO_FORMAT_FLOAT)
#define MODF modff
#endif

struct FormatSpecFlags
{
	uint8_t octothorp : 1;
	uint8_t upper_case : 1;
	uint8_t zero : 1;
	uint8_t parsed_ok : 1;
};

struct FormatSpec
{
	int width = -1;
	int precision = -1;
	int length = -1;
	int index = -1;
	FormatSpecFlags flags{};
	char align = 0; // '<', '^', '>'
	char sign = 0;  // '+', '-', ' '
	char format = 0;
};

static bool is_integer_arg_type(FormatArgType arg_type)
{
	return
		(arg_type == FormatArgType::Int) ||
		(arg_type == FormatArgType::UInt);
}

static bool is_float_arg_type(FormatArgType arg_type)
{
	return
		(arg_type == FormatArgType::Float);
}

static bool is_char_arg_type(FormatArgType arg_type)
{
	return
		(arg_type == FormatArgType::Char);
}

static bool is_bool_arg_type(FormatArgType arg_type)
{
	return
		(arg_type == FormatArgType::Bool);
}

static bool is_str_arg_type(FormatArgType arg_type)
{
	return
		(arg_type == FormatArgType::CharPtr);
}


static void put_char(DstData& dst, char chr)
{
	bool char_is_printed = dst.callback(dst.data, chr);
	if (char_is_printed)
		++dst.chars_printed;
}

static void print_raw_string(DstData& dst, const char *text)
{
	while (*text)
		put_char(dst, *text++);
}

static void print_error(FormatCtx& ctx)
{
	print_raw_string(ctx.dst, "{{error}}");
}

static const char* get_format_specifier(const char* format_str, FormatSpec& format_spec, int index)
{
	enum class State : uint8_t
	{
		Undef,
		IndexSpecified,
		PtPassed,
		PrecSpecified,
		FormatSpecified,
		Finished
	};

	State state = State::Undef;

	const char* orig_format_str = format_str;

	int* int_value = &format_spec.index;

	for (;;)
	{
		unsigned char chr = (unsigned char)*format_str++;

		if ((chr >= '0') && (chr <= '9'))
		{
			if ((state >= State::IndexSpecified) &&
				(state < State::PtPassed) &&
				(chr == '0') &&
				(*int_value == -1))
			{
				format_spec.flags.zero = true;
				continue;
			}
			else if (int_value)
			{
				if (*int_value == -1) *int_value = 0;
				*int_value *= 10;
				*int_value += chr - '0';
				continue;
			}
			else
				return orig_format_str;
		}
		else if (int_value && (*int_value != -1))
			int_value = nullptr;

		switch (chr)
		{
		case ':':
			if (state == State::Undef)
			{
				int_value = &format_spec.width;
				state = State::IndexSpecified;
			}
			else
				return orig_format_str;
			break;

		case '.':
			if ((state >= State::IndexSpecified) && (state < State::PtPassed))
			{
				int_value = &format_spec.precision;
				state = State::PtPassed;
			}
			else
				return orig_format_str;
			break;

		case '<': case '>': case '^':
			if (format_spec.align == 0)
				format_spec.align = chr;
			else
				return orig_format_str;
			break;

		case '+': case '-': case ' ':
			if (format_spec.sign == 0)
				format_spec.sign = chr;
			else
				return orig_format_str;
			break;

		case '#':
			format_spec.flags.octothorp = true;
			break;

		case 'B': case 'b': case 'd':
		case 'o': case 'x': case 'X':
		case 'c': case 'f': case 'F':
		case 's':
			if (format_spec.format == 0)
				format_spec.format = chr;
			else
				return orig_format_str;
			state = State::FormatSpecified;
			break;

		case '}':
			state = State::Finished;
			break;

		default:
			return orig_format_str;
		}

		if (state == State::Finished) break;
	}

	auto user_format = format_spec.format;
	switch (format_spec.format)
	{
	case 'F': format_spec.format = 'f'; break;
	case 'X': format_spec.format = 'x'; break;
	case 'B': format_spec.format = 'b'; break;
	}
	format_spec.flags.upper_case = (user_format != format_spec.format);

	format_spec.flags.parsed_ok = true;

	if (format_spec.index == -1)
		format_spec.index = index;

	return format_str;
}

static bool check_format_specifier(FormatCtx& ctx, FormatSpec& format_spec)
{
	if (format_spec.index >= ctx.args_count) return false;

	auto type = ctx.args[format_spec.index].type;
	auto f = format_spec.format;

	if (is_float_arg_type(type) && (f != 'f') && (f != 0))
		return false;

	bool is_integer_presentation =
		(f == 'b') || (f == 'd') || (f == 'o') || (f == 'x');

	if ((is_integer_arg_type(type) || is_char_arg_type(type)) &&
	    !is_integer_presentation && (f != 'c') && (f != 0))
		return false;

	if (is_bool_arg_type(type) && !is_integer_presentation && (f != 's') && (f != 0))
		return false;

	if (is_str_arg_type(type) && (f != 's') && (f != 0))
		return false;

	return true;
}

static void correct_format_specifier(FormatCtx& ctx, FormatSpec& format_spec)
{
	auto arg_type = ctx.args[format_spec.index].type;

	if (format_spec.align == 0)
	{
		if (is_integer_arg_type(arg_type) || is_float_arg_type(arg_type))
			format_spec.align = '>';
		else
			format_spec.align = '<';
	}

	switch (arg_type)
	{
	case FormatArgType::Pointer:
		if (format_spec.format == 0)
		{
			format_spec.format = 'p';
			format_spec.flags.zero = true;
			format_spec.flags.octothorp = true;

			if (format_spec.width == -1)
				format_spec.width = 2 * sizeof(void*) + (format_spec.flags.octothorp ? 2 : 0);
		}
		break;

	case FormatArgType::Float:
		if (format_spec.precision == -1)
			format_spec.precision = 6;
		break;

	default:
		break;
	}
}

static void print_presentation(FormatCtx& ctx, const FormatSpec& format_spec)
{
	if (!format_spec.flags.octothorp) return;

	switch (format_spec.format)
	{
	case 'x': case 'p':
		put_char(ctx.dst, '0');
		put_char(ctx.dst, format_spec.flags.upper_case ? 'X' : 'x');
		break;

	case 'b':
		put_char(ctx.dst, '0');
		put_char(ctx.dst, format_spec.flags.upper_case ? 'B' : 'b');
		break;

	case 'o':
		put_char(ctx.dst, '0');
		break;
	}
}

static void print_sign(FormatCtx& ctx, const FormatSpec& format_spec, bool is_negative)
{
	if (is_negative)
		put_char(ctx.dst, '-');

	else if ((format_spec.sign == '+') || (format_spec.sign == ' '))
		put_char(ctx.dst, format_spec.sign);
}

static void print_leading_spaces(FormatCtx& ctx, const FormatSpec& format_spec, int len, bool ignore_zero_flag)
{
	if ((format_spec.width == -1) || (format_spec.width <= len)) return;

	char char_to_print = 0;
	int chars_count = 0;

	if (format_spec.flags.zero || (format_spec.align == '>'))
	{
		char_to_print = (!ignore_zero_flag && format_spec.flags.zero) ? '0' : ' ';
		chars_count = format_spec.width - len;
	}
	else if (format_spec.align == '^')
	{
		char_to_print = ' ';
		chars_count = (format_spec.width - len) / 2;
	}

	while (chars_count--)
		put_char(ctx.dst, char_to_print);
}

static void print_trailing_spaces(FormatCtx& ctx, const FormatSpec& format_spec, int len)
{
	if ((format_spec.width == -1) || (format_spec.width <= len)) return;

	int chars_count = 0;

	if (!format_spec.flags.zero && (format_spec.align == '<'))
		chars_count = format_spec.width - len;

	else if (format_spec.align == '^')
		chars_count = (format_spec.width - len + 1) / 2;

	while (chars_count--)
		put_char(ctx.dst, ' ');
}

static void print_sign_and_leading_spaces(FormatCtx& ctx, const FormatSpec& format_spec, bool is_negative, int len, bool ignore_zero_flag)
{
	if (format_spec.flags.zero)
	{
		print_sign(ctx, format_spec, is_negative);
		print_presentation(ctx, format_spec);
	}

	print_leading_spaces(ctx, format_spec, len, ignore_zero_flag);

	if (!format_spec.flags.zero)
	{
		print_sign(ctx, format_spec, is_negative);
		print_presentation(ctx, format_spec);
	}
}

static int strlen(const char* str)
{
	int result = 0;
	while (*str++) result++;
	return result;
}

static void print_string_impl(FormatCtx& ctx, const FormatSpec& format_spec, const char* str, bool is_negative)
{
	int len = strlen(str);
	if (is_negative || (format_spec.sign == '+') || (format_spec.sign == ' ')) len++;
	print_sign_and_leading_spaces(ctx, format_spec, is_negative, len, true);
	print_raw_string(ctx.dst, str);
	print_trailing_spaces(ctx, format_spec, len);
}

static void print_char_impl(FormatCtx& ctx, const FormatSpec& format_spec, char value)
{
	char str[] = { value , 0 };
	print_string_impl(ctx, format_spec, str, false);
}

static void print_uint_impl(DstData& dst, UIntType value, unsigned base, bool upper_case)
{
	UIntType div_value = 0;

	switch (base)
	{
	case 2:
#ifdef MICRO_FORMAT_INT64
		div_value = 1ULL << 63;
#else
		div_value = 1UL << 31;
#endif
		break;

	case 8:
#ifdef MICRO_FORMAT_INT64
		div_value = 1LL << (3 * 21);
#else
		div_value = 3UL << (3 * 10);
#endif
		break;

	case 10:
#ifdef MICRO_FORMAT_INT64
		div_value = 10'000'000'000'000'000'000ULL;
#else
		div_value = 1'000'000'000UL;
#endif
		break;

	case 16:
#ifdef MICRO_FORMAT_INT64
		div_value = 1ULL << (4 * 15);
#else
		div_value = 1UL << (4 * 7);
#endif
		break;
	}

	while ((div_value > value) && (div_value >= base))
		div_value /= base;

	for (;;)
	{
		unsigned value_to_print = (unsigned)(value / div_value);

		char char_to_print =
			(value_to_print < 10)
			? (value_to_print + '0')
			: (value_to_print - 10 + (upper_case ? 'A' : 'a'));

		put_char(dst, char_to_print);

		value -= value_to_print * div_value;
		div_value /= base;

		if (div_value == 0) break;
	}
}

static int find_uint_len(UIntType value, unsigned base)
{
	unsigned int len = 0;
	while (value != 0) { value /= base; len++; }
	if (len == 0) len = 1;
	return len;
}

static void print_uint_generic(FormatCtx& ctx, const FormatSpec& format_spec, UIntType value, bool is_negative)
{
	unsigned base =
		(format_spec.format == 'b') ? 2 :
		(format_spec.format == 'd') ? 10 :
		(format_spec.format == 'x') ? 16 :
		(format_spec.format == 'p') ? 16 :
		(format_spec.format == 'o') ? 8 : 10;

	// calculate length

	unsigned char len = find_uint_len(value, base);
	if (format_spec.flags.octothorp)
	{
		if ((format_spec.format == 'x') || (format_spec.format == 'b'))
			len += 2;
		else if (format_spec.format == 'o')
			len++;
	}

	if (is_negative || (format_spec.sign == '+') || (format_spec.sign == ' ')) len++;

	// sign, format specifier and leading spaces or zeros
	print_sign_and_leading_spaces(ctx, format_spec, is_negative, len, false);

	// integer
	print_uint_impl(ctx.dst, value, base, format_spec.flags.upper_case);

	// after spaces or zeros
	print_trailing_spaces(ctx, format_spec, len);
}

static void print_char(FormatCtx& ctx, const FormatSpec& format_spec, char value)
{
	if ((format_spec.format == 'c') || (format_spec.format == 0))
		print_char_impl(ctx, format_spec, value);

	else
		print_uint_generic(ctx, format_spec, (unsigned)value, false);
}

static void print_string(FormatCtx& ctx, const FormatSpec& format_spec, const char* str)
{
	print_string_impl(ctx, format_spec, str, false);
}

static void print_int(FormatCtx& ctx, const FormatSpec& format_spec, IntType value)
{
	bool is_negative = value < 0;

	if (format_spec.format != 'c')
	{
		if (is_negative) value = -value;
		print_uint_generic(ctx, format_spec, value, is_negative);
	}
	else
	{
		if (is_negative || (value > 255))
			print_error(ctx);
		else
			print_char_impl(ctx, format_spec, (char)value);
	}
}

static void print_uint(FormatCtx& ctx, const FormatSpec& format_spec, UIntType value)
{
	if (format_spec.format != 'c')
		print_uint_generic(ctx, format_spec, value, false);
	else
	{
		if (value > 255)
			print_error(ctx);
		else
			print_char_impl(ctx, format_spec, (char)value);
	}
}

static void print_bool(FormatCtx& ctx, const FormatSpec& format_spec, bool value)
{
	if ((format_spec.format == 's') || (format_spec.format == 0))
		print_string_impl(ctx, format_spec, value ? "true" : "false", false);
	else
		print_uint_generic(ctx, format_spec, (unsigned char)value, false);
}

static void print_pointer(FormatCtx& ctx, const FormatSpec& format_spec, uintptr_t pointer)
{
	print_uint_generic(ctx, format_spec, pointer, false);
}


#if defined (MICRO_FORMAT_FLOAT) || defined (MICRO_FORMAT_DOUBLE)

struct PrintFloatData
{
	FloatType rounded_value = {};
	FloatType positive_value = {};
	FloatType round_div = {};
	FloatType integral_div = {};
	int integral_len = {};
	bool is_negative {};
	const char* nan_text = nullptr;
};

static void gather_data_to_print_float(FloatType value, int precision, bool upper_case, PrintFloatData &result)
{
	if (isnan(value))
	{
		result.nan_text = upper_case ? "NAN" : "nan";
		return;
	}

	bool is_p_inf = (value > std::numeric_limits<FloatType>::max());
	bool is_n_inf = (value < std::numeric_limits<FloatType>::lowest());

	if (is_p_inf || is_n_inf) // +inf or -inf
	{
		result.nan_text = upper_case ? "INF" : "inf";
		result.is_negative = is_n_inf;
		return;
	}

	result.is_negative = value < (FloatType)0.0f;
	if (result.is_negative)
		value = -value;

	// calculate data for rounding last decimal digit

	result.round_div = (FloatType)1.0f;
	for (int i = 0; i < precision; i++) result.round_div *= (FloatType)10.0f;

	// do rounding

	result.rounded_value = value + (FloatType)0.5f / result.round_div;

	// calculate integral part len

	result.integral_len = 0;
	result.integral_div = 1;
	if (value >= (FloatType)1.0f)
	{
		while (value > result.integral_div)
		{
			result.integral_div *= (FloatType)10.0f;
			result.integral_len++;
		}
		if ((int)(value / result.integral_div) == 0)
			result.integral_div /= (FloatType)10.0f;
		else
			result.integral_len++;
	}
	else
	{
		result.integral_len = 1;
	}

	result.positive_value = value;
}

static void printf_float_number(const PrintFloatData data, DstData &dst, int precision)
{
	// print integral part

	auto value = data.rounded_value;
	auto integral_len = data.integral_len;
	auto integral_div = data.integral_div;
	while (integral_len--)
	{
		int int_val = (int)(value / integral_div);
		put_char(dst, '0' + int_val);
		value -= int_val * integral_div;
		integral_div /= (FloatType)10.0f;
	}

	// print decimal part

	FloatType integral_part = 0;
	value = MODF(data.positive_value, &integral_part);

	if (precision)
		put_char(dst, '.');

	auto round_div = data.round_div;
	for (int i = 0; i < precision; i++)
	{
		value *= (FloatType)10.0f;
		round_div /= (FloatType)10.0f;
		int int_val = (int)(value + (FloatType)0.5f / round_div);
		value -= int_val;
		if (int_val >= 10) int_val -= 10;
		put_char(dst, '0' + int_val);
	}
}

static void print_float(FormatCtx& ctx, const FormatSpec& format_spec, FloatType value)
{
	PrintFloatData data{};

	gather_data_to_print_float(value, format_spec.precision, format_spec.flags.upper_case, data);

	// if value is non-a-number print nan, +inf or -inf

	if (data.nan_text)
	{
		print_string_impl(ctx, format_spec, data.nan_text, data.is_negative);
		return;
	}

	// start calculate all text len

	int len = data.integral_len;

	// size for sign

	if (data.is_negative || (format_spec.sign == '+') || (format_spec.sign == ' ')) len++;

	// size for point

	if (format_spec.precision != 0)
		len++;

	// size for decimal part

	len += format_spec.precision;

	// print sign, leading spaces or zeros

	print_sign_and_leading_spaces(ctx, format_spec, data.is_negative, len, true);

	// integral and decimal parts

	printf_float_number(data, ctx.dst, format_spec.precision);

	// after spaces

	print_trailing_spaces(ctx, format_spec, len);
}

#endif

static void print_by_argument_type(FormatCtx& ctx, const FormatSpec& format_spec)
{
	const auto &argr = ctx.args[format_spec.index];

	switch (ctx.args[format_spec.index].type)
	{
	case FormatArgType::Char:
		print_char(ctx, format_spec, (char)argr.value.i);
		break;

	case FormatArgType::UChar:
		print_char(ctx, format_spec, (char)argr.value.u);
		break;

	case FormatArgType::Int:
		print_int(ctx, format_spec, argr.value.i);
		break;

	case FormatArgType::UInt:
		print_uint(ctx, format_spec, argr.value.u);
		break;

	case FormatArgType::Bool:
		print_bool(ctx, format_spec, argr.value.u != 0);
		break;

	case FormatArgType::CharPtr:
		print_string(ctx, format_spec, (const char*)argr.value.p);
		break;

	case FormatArgType::Pointer:
		print_pointer(ctx, format_spec, argr.value.p);
		break;

#if defined (MICRO_FORMAT_DOUBLE) || defined (MICRO_FORMAT_FLOAT)
	case FormatArgType::Float:
		print_float(ctx, format_spec, argr.value.f);
		break;
#endif
	default:
		break;
	}
}

void format_impl(FormatCtx& ctx, const char* format_str)
{
	int index = 0;
	ctx.dst.chars_printed = 0;

	for (;;)
	{
		char chr = *format_str++;
		if (chr == 0) break;

		if (chr == '{')
		{
			if (*format_str != '{')
			{
				FormatSpec spec {};

				format_str = get_format_specifier(format_str, spec, index);

				bool ok = spec.flags.parsed_ok && check_format_specifier(ctx, spec);

				if (ok)
				{
					correct_format_specifier(ctx, spec);
					print_by_argument_type(ctx, spec);
					index++;
				}
				else
					print_error(ctx);
			}
			else
			{
				put_char(ctx.dst, '{');
				format_str++;
			}
		}
		else
			put_char(ctx.dst, chr);
	}
}

bool format_buf_callback(void* data, char character)
{
	auto* sdata = (FormatBufData*)data;
	if (sdata->buffer_size == 0) return false;

	*sdata->buffer++ = character;
	--sdata->buffer_size;

	return true;
}

bool utf8_char_callback(void* data, char chr)
{
	Utf8Receiver* r = (Utf8Receiver*)data;

	bool ok = true;

	if (r->count == 0)
	{
		if ((chr & 0b10000000) == 0)
		{
			ok = r->cb(r->cb_data, chr);
			r->chars_printed++;
		}

		else if ((chr & 0b11100000) == 0b11000000)
		{
			r->character = chr & 0b00011111;
			r->count = 1;
		}

		else if ((chr & 0b11110000) == 0b11100000)
		{
			r->character = chr & 0b00001111;
			r->count = 2;
		}

		else if ((chr & 0b11111000) == 0b11110000)
		{
			r->character = chr & 0b00000111;
			r->count = 3;
		}

		else
		{
			ok = r->cb(r->cb_data, r->wrong_char);
			r->chars_printed++;
		}
	}
	else
	{
		if ((chr & 0b11000000) != 0b10000000)
		{
			ok = r->cb(r->cb_data, r->wrong_char);
			r->count = 0;
			r->chars_printed++;
		}
		else
		{
			r->character <<= 6;
			r->character |= (chr & 0b00111111);
			r->count--;

			if (r->count == 0)
			{
				ok = r->cb(r->cb_data, r->character);
				r->chars_printed++;
			}
		}
	}

	return ok;
}

} // namespace impl

static size_t format_uint_impl(FormatCallback callback, void* data, unsigned value, unsigned base)
{
	impl::DstData dst{ callback, data, 0 };
	impl::print_uint_impl(dst, value, base, false);
	return dst.chars_printed;
}

size_t format_dec(FormatCallback callback, void* data, int value)
{
	impl::DstData dst { callback, data, 0 };
	if (value < 0)
	{
		value = -value;
		put_char(dst, '-');
	}
	impl::print_uint_impl(dst, value, 10, false);
	return dst.chars_printed;
}

size_t format_dec(char* buffer, size_t buffer_size, int value)
{
	return impl::format_buf_impl(
		buffer,
		buffer_size,
		[=](auto& data) { return format_dec(impl::format_buf_callback, &data, value); }
	);
}

size_t format_dec(FormatCallback callback, void* data, unsigned value)
{
	return format_uint_impl(callback, data, value, 10);
}

size_t format_dec(char* buffer, size_t buffer_size, unsigned value)
{
	return impl::format_buf_impl(
		buffer,
		buffer_size,
		[=](auto& data) { return format_dec(impl::format_buf_callback, &data, value); }
	);
}

size_t format_hex(FormatCallback callback, void* data, unsigned value)
{
	return format_uint_impl(callback, data, value, 16);
}

size_t format_hex(char* buffer, size_t buffer_size, unsigned value)
{
	return impl::format_buf_impl(
		buffer,
		buffer_size,
		[=](auto& data) { return format_hex(impl::format_buf_callback, &data, value); }
	);
}

size_t format_bin(FormatCallback callback, void* data, unsigned value)
{
	return format_uint_impl(callback, data, value, 2);
}

size_t format_bin(char* buffer, size_t buffer_size, unsigned value)
{
	return impl::format_buf_impl(
		buffer,
		buffer_size,
		[=](auto& data) { return format_bin(impl::format_buf_callback, &data, value); }
	);
}

#if defined (MICRO_FORMAT_DOUBLE) || defined (MICRO_FORMAT_FLOAT)

size_t format_float(FormatCallback callback, void* cb_data, impl::FloatType value, int precision)
{
	impl::DstData dst{ callback, cb_data, 0 };
	impl::PrintFloatData data{};

	impl::gather_data_to_print_float(value, precision, false, data);

	if (data.is_negative) put_char(dst, '-');

	if (data.nan_text)
		impl::print_raw_string(dst, data.nan_text);
	else
		impl::printf_float_number(data, dst, precision);

	return dst.chars_printed;
}

size_t format_float(char* buffer, size_t buffer_size, impl::FloatType value, int precision)
{
	return impl::format_buf_impl(
		buffer,
		buffer_size,
		[=](auto& data) { return format_float(impl::format_buf_callback, &data, value, precision); }
	);
}

#endif

} // namespace mf
