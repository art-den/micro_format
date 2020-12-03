#include <limits>
#include "micro_format.hpp"

namespace impl {

#if defined (MICRO_FORMAT_DOUBLE)
using FloatType = double;
#elif defined (MICRO_FORMAT_FLOAT)
using FloatType = float;
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
		(arg_type == FormatArgType::Short) ||
		(arg_type == FormatArgType::UShort) ||
		(arg_type == FormatArgType::Int) ||
		(arg_type == FormatArgType::UInt) ||
		(arg_type == FormatArgType::Long) ||
		(arg_type == FormatArgType::ULong);
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

static bool is_boolarg_type(FormatArgType arg_type)
{
	return
		(arg_type == FormatArgType::Bool);
}

static void put_char(FormatCtx& ctx, char chr)
{
	ctx.callback(ctx.data, chr);
	++ctx.chars_printed;
}

static void print_raw_string(FormatCtx& ctx, const char *text)
{
	while (*text)
		put_char(ctx, *text++);
}

static void print_error(FormatCtx& ctx)
{
	print_raw_string(ctx, "{{error}}");
}

static int strlen(const char *str)
{
	if (!str) return 0;
	int result = 0;
	while (*str++) result++;
	return result;
}

static const char* get_format_specifier(FormatCtx& ctx, const char* format_str, FormatSpec& format_spec)
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

	if (is_integer_arg_type(type) && !is_integer_presentation && (f != 'c') && (f != 0))
		return false;

	if (is_char_arg_type(type) && !is_integer_presentation && (f != 'c') && (f != 0))
		return false;

	if (is_boolarg_type(type) && !is_integer_presentation && (f != 0))
		return false;

	return true;
}

static void correct_format_specifier(FormatCtx& ctx, FormatSpec& format_spec, int index)
{
	if (format_spec.index == -1)
		format_spec.index = index;

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
	case FormatArgType::Double:
		if (format_spec.precision == -1)
			format_spec.precision = 6;
		break;
	}
}

static void print_presentation(FormatCtx& ctx, const FormatSpec& format_spec)
{
	if (!format_spec.flags.octothorp) return;

	switch (format_spec.format)
	{
	case 'x': case 'p':
		put_char(ctx, '0');
		put_char(ctx, format_spec.flags.upper_case ? 'X' : 'x');
		break;

	case 'b':
		put_char(ctx, '0');
		put_char(ctx, format_spec.flags.upper_case ? 'B' : 'b');
		break;

	case 'o':
		put_char(ctx, '0');
		break;
	}
}

static void print_sign(FormatCtx& ctx, const FormatSpec& format_spec, bool is_negative)
{
	if (is_negative)
		put_char(ctx, '-');

	else if ((format_spec.sign == '+') || (format_spec.sign == ' '))
		put_char(ctx, format_spec.sign);
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
		put_char(ctx, char_to_print);
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
		put_char(ctx, ' ');
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

static void print_string_impl(FormatCtx& ctx, const FormatSpec& format_spec, const char* str, bool is_negative)
{
	int len = strlen(str);
	if (is_negative || (format_spec.sign == '+') || (format_spec.sign == ' ')) len++;
	print_sign_and_leading_spaces(ctx, format_spec, is_negative, len, true);
	print_raw_string(ctx, str);
	print_trailing_spaces(ctx, format_spec, len);
}

static void print_char_impl(FormatCtx& ctx, const FormatSpec& format_spec, char value)
{
	char str[] = { value , 0 };
	print_string_impl(ctx, format_spec, str, false);
}

static void print_uint_impl(FormatCtx& ctx, unsigned value, unsigned base, bool upper_case)
{
	if (value >= base)
		print_uint_impl(ctx, value / base, base, upper_case);

	unsigned value_to_print = value % base;

	char char_to_print =
		(value_to_print < 10)
		? (value_to_print + '0')
		: (value_to_print - 10 + (upper_case ? 'A' : 'a'));

	put_char(ctx, char_to_print);
}

static int find_integer_len(unsigned value, unsigned base)
{
	unsigned int len = 0;
	while (value != 0) { value /= base; len++; }
	if (len == 0) len = 1;
	return len;
}

static void print_int_generic(FormatCtx& ctx, const FormatSpec& format_spec, unsigned value, bool is_negative)
{
	unsigned base =
		(format_spec.format == 'b') ? 2 :
		(format_spec.format == 'd') ? 10 :
		(format_spec.format == 'x') ? 16 :
		(format_spec.format == 'p') ? 16 :
		(format_spec.format == 'o') ? 8 : 10;

	// calculate length

	unsigned char len = find_integer_len(value, base);
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
	print_uint_impl(ctx, value, base, format_spec.flags.upper_case);

	// after spaces or zeros
	print_trailing_spaces(ctx, format_spec, len);
}

static void print_char(FormatCtx& ctx, const FormatSpec& format_spec, char value)
{
	if ((format_spec.format == 'c') || (format_spec.format == 0))
		print_char_impl(ctx, format_spec, value);

	else
		print_int_generic(ctx, format_spec, (int)value, false);
}

static void print_string(FormatCtx& ctx, const FormatSpec& format_spec, const char* str)
{
	print_string_impl(ctx, format_spec, str, false);
}

static void print_int(FormatCtx& ctx, const FormatSpec& format_spec, int value)
{
	bool is_negative = value < 0;

	if (format_spec.format != 'c')
	{
		if (is_negative) value = -value;
		print_int_generic(ctx, format_spec, value, is_negative);
	}
	else
	{
		if (is_negative || (value > 255))
			print_error(ctx);
		else
			print_char_impl(ctx, format_spec, (char)value);
	}
}

static void print_uint(FormatCtx& ctx, const FormatSpec& format_spec, unsigned value)
{
	if (format_spec.format != 'c')
		print_int_generic(ctx, format_spec, value, false);
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
		print_int_generic(ctx, format_spec, (unsigned char)value, false);
}

static void print_pointer(FormatCtx& ctx, const FormatSpec& format_spec, const void *pointer)
{
	print_int_generic(ctx, format_spec, (unsigned)pointer, false);
}


#if defined (MICRO_FORMAT_FLOAT) || defined (MICRO_FORMAT_DOUBLE)

static void print_f_number(FormatCtx& ctx, const FormatSpec& format_spec, FloatType value)
{
	bool is_negative = value < (FloatType)0.0f;
	if (is_negative)
		value = -value;

	// do rounding for last digit

	FloatType round = (FloatType)0.5f;
	for (int i = 0; i < format_spec.precision; i++)
		round /= 10.0f;
	value += round;

	// start calculate all text len

	int len = 0;

	// size for sign

	if (is_negative || (format_spec.sign == '+') || (format_spec.sign == ' ')) len++;

	// size for integral part

	int integral_len = 0;
	FloatType div = 1;
	bool is_greater_eq_1 = value >= (FloatType)1.0f;
	if (is_greater_eq_1)
	{
		while (value > div)
		{
			div *= (FloatType)10.0f;
			len++;
			integral_len++;
		}
		if ((int)(value / div) == 0)
			div /= (FloatType)10.0f;
		else
			integral_len++;
	}
	else
	{
		len++;
	}

	// size for point

	if (format_spec.precision != 0)
		len++;

	// size for decimal part

	len += format_spec.precision;

	// print sign, leading spaces or zeros

	print_sign_and_leading_spaces(ctx, format_spec, is_negative, len, false);

	// print integral part

	if (is_greater_eq_1)
	{
		while (integral_len--)
		{
			int int_val = (int)(value / div);
			put_char(ctx, '0' + int_val);
			value -= int_val * div;
			div /= (FloatType)10.0f;
		}
	}
	else
	{
		put_char(ctx, '0');
	}

	// print decimal part

	if (format_spec.precision)
		put_char(ctx, '.');

	for (int i = 0; i < format_spec.precision; i++)
	{
		value *= (FloatType)10.0f;
		int int_val = (int)value;
		put_char(ctx, '0' + int_val);
		value -= int_val;
	}

	// after spaces

	print_trailing_spaces(ctx, format_spec, len);
}

static void print_float(FormatCtx& ctx, const FormatSpec& format_spec, FloatType value)
{
	if (value != value) // nan
	{
		print_string_impl(ctx, format_spec, format_spec.flags.upper_case ? "NAN" : "nan", false);
		return;
	}

	bool is_p_inf = (value > std::numeric_limits<FloatType>::max());
	bool is_n_inf = (value < std::numeric_limits<FloatType>::lowest());

	if (is_p_inf || is_n_inf) // +inf or -inf
	{
		print_string_impl(ctx, format_spec, format_spec.flags.upper_case ? "INF" : "inf", is_n_inf);
		return;
	}

	print_f_number(ctx, format_spec, value);
}

#endif

static void print_by_argument_type(FormatCtx& ctx, const FormatSpec& format_spec)
{
	auto arg_pointer = ctx.args[format_spec.index].pointer;

	switch (ctx.args[format_spec.index].type)
	{
	case FormatArgType::Char:
		print_char(ctx, format_spec, *(const char*)arg_pointer);
		break;

	case FormatArgType::UChar:
		print_char(ctx, format_spec, *(const unsigned char*)arg_pointer);
		break;

	case FormatArgType::Short:
		print_int(ctx, format_spec, *(const short*)arg_pointer);
		break;

	case FormatArgType::UShort:
		print_uint(ctx, format_spec, *(const unsigned short*)arg_pointer);
		break;

	case FormatArgType::Int:
		print_int(ctx, format_spec, *(const int*)arg_pointer);
		break;

	case FormatArgType::UInt:
		print_uint(ctx, format_spec, *(const unsigned int*)arg_pointer);
		break;

	case FormatArgType::Long:
		print_int(ctx, format_spec, *(const long*)arg_pointer);
		break;

	case FormatArgType::ULong:
		print_uint(ctx, format_spec, *(const unsigned long*)arg_pointer);
		break;

	case FormatArgType::Bool:
		print_bool(ctx, format_spec, *(const bool*)arg_pointer);
		break;

	case FormatArgType::CharPtr:
		print_string(ctx, format_spec, (const char*)arg_pointer);
		break;

	case FormatArgType::Pointer:
		print_pointer(ctx, format_spec, arg_pointer);
		break;

#if defined (MICRO_FORMAT_DOUBLE) || defined (MICRO_FORMAT_FLOAT)
	case FormatArgType::Float:
		print_float(ctx, format_spec, *(const float*)arg_pointer);
		break;
#endif
#if defined (MICRO_FORMAT_DOUBLE)
	case FormatArgType::Double:
		print_float(ctx, format_spec, *(const double*)arg_pointer);
		break;
#endif
	}
}

void format_impl(FormatCtx& ctx, const char* format)
{
	int index = 0;
	ctx.chars_printed = 0;

	for (;;)
	{
		char chr = *format++;
		if (chr == 0) break;

		if (chr == '{')
		{
			if (*format != '{')
			{
				FormatSpec spec {};

				format = get_format_specifier(ctx, format, spec);

				bool ok = spec.flags.parsed_ok && check_format_specifier(ctx, spec);

				if (ok)
				{
					correct_format_specifier(ctx, spec, index);
					print_by_argument_type(ctx, spec);
					index++;
				}
				else
					print_error(ctx);
			}
			else
			{
				put_char(ctx, '{');
				format++;
			}
		}
		else
			put_char(ctx, chr);
	}
}

bool s_format_callback(void* data, char character)
{
	auto* sdata = (SFormatData*)data;
	if (sdata->buffer_size == 0) return false;

	*sdata->buffer++ = character;
	--sdata->buffer_size;

	return true;
}


} // namespace impl
