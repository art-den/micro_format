#include <string>
#include <string_view>
#include <stdio.h>
#include <assert.h>

#include "../micro_format.hpp"

static const std::string error_str = "{{error}}";
static int errors_count = 0;

template <typename ... Args>
void test_eq(std::string_view desired, const char *format, const Args& ... args)
{
	auto print_to_str_callback = [](void* data, char character)
	{
		auto str = (std::string*)data;
		str->push_back(character);
		return true;
	};

	std::string result;

	cb_format(print_to_str_callback, &result, format, args...);

	bool ok = result == desired;

	if (!ok)
	{
		errors_count++;

		std::string desired_str{ desired };
		printf("ERROR: %s \"%s\" -> \"%s\"\n", format, desired_str.c_str(), result.c_str());

		// for debuging purposes
		std::string another_result;
		cb_format(print_to_str_callback, &another_result, format, args...);
	}

	assert(ok);
}

void test_cmp_printf(const char* format, double value)
{
	auto print_to_str_callback = [](void* data, char character)
	{
		auto str = (std::string*)data;
		str->push_back(character);
		return true;
	};

	std::string result;
	std::string fmt1 = "{:";
	fmt1.append(format);
	fmt1.append("}");
	cb_format(print_to_str_callback, &result, fmt1.c_str(), value);

	std::string fmt2 = "%";
	fmt2.append(format);
	fmt2.append("f");
	char printf_buffer[256] = {};
	sprintf_s(printf_buffer, fmt2.c_str(), value);

	bool ok = result == printf_buffer;

	if (!ok)
	{
		errors_count++;

		printf("ERROR: %s \"%s\" -> \"%s\" (%.20f)\n", format, printf_buffer, result.c_str(), value);

		// for debuging purposes
		std::string another_result;
		cb_format(print_to_str_callback, &result, fmt1.c_str(), value);
	}

	assert(ok);
}


static void test_common()
{
	test_eq("", "");
	test_eq("Simple text", "Simple text");

	test_eq("Simple text arg Another text", "Simple text {} Another text", "arg");
}

static void test_integer()
{
	// decimal

	test_eq("42",     "{}",  42);
	test_eq("42",     "{:d}", 42);
	test_eq("-42",    "{}", -42);
	test_eq("+42",    "{:+}", 42);
	test_eq("-42",    "{:+}", -42);
	test_eq("+42",    "{:+}", 42U);
	test_eq("42",     "{:-}", 42);
	test_eq("-42",    "{:-}", -42);
	test_eq("   42",  "{:5}", 42);
	test_eq("  -42",  "{:5}", -42);
	test_eq("  +42",  "{:+5}", 42);
	test_eq("  -42",  "{:+5}", -42);
	test_eq("00042",  "{:05}", 42);
	test_eq("-0042",  "{:05}", -42);
	test_eq("+0042",  "{:+05}", 42);
	test_eq("000123", "{:06}", 123);
	test_eq("-00123", "{:06}", -123);
	test_eq("-00123", "{:+06}", -123);
	test_eq("+00123", "{:+06}", 123);
	test_eq(" 00123", "{: 06}", 123);
	test_eq("-00123", "{: 06}", -123);
	test_eq(" 123",   "{: }", 123);
	test_eq("-123",   "{: }", -123);


	// hex

	test_eq("a",        "{:x}", 0xa);
	test_eq("A",        "{:X}", 0xa);
	test_eq("5533",     "{:x}", 0x5533);
	test_eq("0x5533",   "{:#x}", 0x5533);
	test_eq("0X5533",   "{:#X}", 0x5533);
	test_eq("0X553A",   "{:#X}", 0x553A);
	test_eq("-0x5533",  "{:#x}", -0x5533);
	test_eq("  0x123",  "{:#7x}", 0x123);
	test_eq(" -0x123",  "{:#7x}", -0x123);
	test_eq("-0x0123",  "{:#07x}", -0x123);
	test_eq("0x00123",  "{:#07x}", 0x123);
	test_eq("0000123",  "{:07x}", 0x123);
	test_eq("123    ",  "{:<7x}", 0x123);
	test_eq("123    ",  "{:<7x}", 0x123);
	test_eq("0x123  ",  "{:<#7x}", 0x123);
	test_eq("  123  ",  "{:^7x}", 0x123);
	test_eq("  123   ", "{:^8x}", 0x123);
	test_eq("  -123  ", "{:^8x}", -0x123);


	// binary

	test_eq("1",           "{:b}", 1);
	test_eq("1",           "{:B}", 1);
	test_eq("0b1",         "{:#b}", 1);
	test_eq("0B1",         "{:#B}", 1);
	test_eq("11001100",    "{:b}", 0b11001100);
	test_eq("-11001100",   "{:b}", -0b11001100);
	test_eq("0b11001100",  "{:#b}", 0b11001100);
	test_eq("-0b11001100", "{:#b}", -0b11001100);
	test_eq("0011001100",  "{:010b}", 0b11001100);
	test_eq("  11001100",  "{:10b}", 0b11001100);


	// octal

	test_eq("1234567",    "{:o}", 01234567);
	test_eq("01234567",   "{:#o}", 01234567);
	test_eq("  01234567", "{:#10o}", 01234567);
	test_eq("   1234567", "{:10o}", 01234567);
	test_eq("  -1234567", "{:10o}", -01234567);


	// to char

	test_eq("AB", "{:c}{:c}", 65, 66);


	// errors

	test_eq(error_str, "{:s}", 123);
	test_eq(error_str, "{:f}", 123);
}

static void test_bool()
{
	test_eq("true",    "{}", true);
	test_eq("false",   "{}", false);
	test_eq("true",    "{:s}", true);
	test_eq("false",   "{:s}", false);
	test_eq("true  ",  "{:6}", true);
	test_eq("false ",  "{:6}", false);
	test_eq("  true",  "{:>6}", true);
	test_eq(" false",  "{:>6}", false);
	test_eq("1",       "{:d}", true);
	test_eq("0",       "{:d}", false);
	test_eq("1",       "{:x}", true);
	test_eq("0",       "{:x}", false);
	test_eq("0x1",     "{:#x}", true);
	test_eq("0x0",     "{:#x}", false);
	test_eq("0b1",     "{:#b}", true);
	test_eq("0b0",     "{:#b}", false);

	test_eq(error_str, "{:c}", true);
	test_eq(error_str, "{:f}", true);
}

static void test_str()
{
	test_eq("str",     "{}", "str");
	test_eq("str",     "{:s}", "str");
	test_eq("str    ", "{:7}", "str");
	test_eq("    str", "{:>7}", "str");
	test_eq("  str  ", "{:^7}", "str");

	test_eq(error_str, "{:c}", "str");
	test_eq(error_str, "{:f}", "str");
	test_eq(error_str, "{:d}", "str");
	test_eq(error_str, "{:x}", "str");
	test_eq(error_str, "{:X}", "str");
	test_eq(error_str, "{:o}", "str");
	test_eq(error_str, "{:b}", "str");
	test_eq(error_str, "{:B}", "str");
}

static void test_char()
{
	test_eq("A",         "{}", 'A');
	test_eq("A",         "{:c}", 'A');
	test_eq("A     ",    "{:6}", 'A');
	test_eq("     A",    "{:>6}", 'A');

	test_eq("65",        "{:d}", 'A');
	test_eq("0x41",      "{:#x}", 'A');
	test_eq("0101",      "{:#o}", 'A');
	test_eq("0b1000001", "{:#b}", 'A');

	test_eq(error_str,   "{:s}", 'E');
	test_eq(error_str,   "{:f}", 'E');
}

static void test_float()
{
	// float

	test_eq("1.200000",  "{}", 1.2f);
	test_eq("-1.200000", "{}", -1.2f);
	test_eq("1.200000",  "{:f}",  1.2f);
	test_eq("1.2",       "{:.1}", 1.2f);
	test_eq("-1.2",      "{:.1}", -1.2f);
	test_eq("1",         "{:.0}", 1.2f);
	test_eq("-1",        "{:.0}", -1.2f);
	test_eq("     1",    "{:6.0}", 1.2f);
	test_eq("    -1",    "{:6.0}", -1.2f);
	test_eq("   1.2",    "{:6.1}", 1.2f);
	test_eq("  -1.2",    "{:6.1}", -1.2f);
	test_eq("   1.2",    "{:>6.1}", 1.2f);
	test_eq("  -1.2",    "{:>6.1}", -1.2f);
	test_eq("-1.2  ",    "{:<6.1}", -1.2f);
	test_eq("1.2   ",    "{:<6.1}", 1.2f);
	test_eq(" -1.2 ",    "{:^6.1}", -1.2f);
	test_eq("+1.2",      "{:+.1}", 1.2f);
	test_eq("-1.2",      "{:+.1}", -1.2f);
	test_eq(" 1.2",      "{: .1}", 1.2f);
	test_eq("-1.2",      "{: .1}", -1.2f);
	test_eq("nan",       "{}", NAN);
	test_eq("nan",       "{:f}", NAN);
	test_eq("NAN",       "{:F}", NAN);
	test_eq("  nan",     "{:5}", NAN);
	test_eq("nan  ",     "{:<5}", NAN);
	test_eq("inf",       "{}", INFINITY);
	test_eq("inf",       "{:f}", INFINITY);
	test_eq("INF",       "{:F}", INFINITY);
	test_eq("-inf",      "{}", -INFINITY);
	test_eq("+inf",      "{:+}", INFINITY);
	test_eq("-inf",      "{:+}", -INFINITY);
	test_eq("+INF",      "{:+F}", INFINITY);
	test_eq("-INF",      "{:+F}", -INFINITY);
	test_eq(" inf",      "{: }", INFINITY);
	test_eq("-inf",      "{: }", -INFINITY);
	test_eq("  inf",     "{:5}", INFINITY);
	test_eq(" -inf",     "{:5}", -INFINITY);
	test_eq("inf  ",     "{:<5}", INFINITY);
	test_eq("-inf ",     "{:<5}", -INFINITY);

	test_eq("1000.0", "{:.1}", 1000.0f);
	test_eq("3210.9", "{:.1}", 3210.9f);
	test_eq("7654.3", "{:.1}", 7654.3f);

	test_eq("10.0", "{:.1}", 10.0f);
	test_eq("100.0", "{:.1}", 100.0f);
	test_eq("1000.0", "{:.1}", 1000.0f);
	test_eq("10000.0", "{:.1}", 10000.0f);
	test_eq("100000.0", "{:.1}", 100000.0f);
	test_eq("1000000.0", "{:.1}", 1000000.0f);
	test_eq("10000000.0", "{:.1}", 10000000.0f);
	test_eq("100000000.0", "{:.1}", 100000000.0f);
	test_eq("1000000000.0", "{:.1}", 1000000000.0f);
	test_eq("10000000000.0", "{:.1}", 10000000000.0f);

	// double

	test_eq("1.200000", "{}", 1.2);
	test_eq("-1.200000", "{}", -1.2);

	test_eq("10000000000.0", "{:.1}", 10000000000.0);
	test_eq("1000000000000000.0", "{:.1}", 1000000000000000.0);
	test_eq("100000000000000000000.0", "{:.1}", 100000000000000000000.0);
	test_eq("10000000000000000000000.0", "{:.1}", 10000000000000000000000.0);

	// compalre with printf

	for (int64_t i = -10'000; i < 1'000'000; i += 11)
	{
		double value = i / 1003.123;
		test_cmp_printf(".13", value);
	}

	for (int64_t i = 1'000'000; i < 1'000'000'000; i += 10003)
	{
		double value = i / 1003.321;
		test_cmp_printf(".13", value);
	}

	for (int64_t i = -10'000; i < 1'000'000'000; i += 10003)
	{
		double value = i / 13.777;
		test_cmp_printf(".13", value);
	}

	// errors

	test_eq(error_str, "{:s}", 123.0f);
	test_eq(error_str, "{:s}", 123.0);
	test_eq(error_str, "{:c}", 123.0f);
	test_eq(error_str, "{:c}", 123.0);
	test_eq(error_str, "{:d}", 123.0f);
	test_eq(error_str, "{:d}", 123.0);
	test_eq(error_str, "{:x}", 123.0f);
	test_eq(error_str, "{:x}", 123.0);
	test_eq(error_str, "{:X}", 123.0f);
	test_eq(error_str, "{:X}", 123.0);
	test_eq(error_str, "{:o}", 123.0f);
	test_eq(error_str, "{:o}", 123.0);
	test_eq(error_str, "{:b}", 123.0f);
	test_eq(error_str, "{:b}", 123.0);
	test_eq(error_str, "{:B}", 123.0f);
	test_eq(error_str, "{:B}", 123.0);
}


static void test_arg_pos()
{
	test_eq("1234", "{}{}{}{}", 1, 2, 3, 4);
	test_eq("1234", "{0}{1}{2}{3}", 1, 2, 3, 4);
	test_eq("1144", "{0}{0}{3}{3}", 1, 2, 3, 4);
	test_eq("text", "text", 1, 2, 3, 4);
	test_eq("4321", "{3}{2}{1}{0}", 1, 2, 3, 4);

	test_eq("1"+error_str+"1", "{0}{1}{0}", 1);
}

int main()
{
	test_common();
	test_integer();
	test_bool();
	test_str();
	test_char();
	test_float();
	test_arg_pos();

	return errors_count;
}
