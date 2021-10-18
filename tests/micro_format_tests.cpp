#include <string>

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include <boost/locale.hpp>

#include "../micro_format.hpp"

static const std::string error_str = "{{error}}";

template <typename ... Args>
void test_eq(const std::string &desired, const char *format_str, const Args& ... args)
{
	char result[256] = {};
	mf::format(result, format_str, args...);
	assert(desired == result);
}

template <typename ... Args>
void test_eq_unicode(const std::string& desired, const char* format_str, const Args& ... args)
{
	std::wstring wstr;

	auto add_wide_char_cb = [](void* data, mf::WideChar chr)
	{
		auto* str = (std::wstring*)data;
		str->push_back(chr);
		return true;
	};

	auto wide_chars_count = mf::format_u8(add_wide_char_cb, &wstr, format_str, args...);
	auto desired_w = boost::locale::conv::utf_to_utf<wchar_t>(desired);

	assert(desired_w == wstr);
	assert(wide_chars_count == desired_w.size());
}

void test_cmp_printf(const char* format_str, double value)
{
	char result[256] = {};
	std::string fmt1 = "{:";
	fmt1.append(format_str);
	fmt1.append("}");
	mf::format(result, fmt1.c_str(), value);

	std::string fmt2 = "%";
	fmt2.append(format_str);
	fmt2.append("f");
	char printf_buffer[256] = {};
	sprintf_s(printf_buffer, fmt2.c_str(), value);

	assert(strcmp(result, printf_buffer) == 0);
}

static void test_common()
{
	test_eq("", "");
	test_eq("Simple text", "Simple text");

	test_eq("Simple text arg Another text", "Simple text {} Another text", "arg");
}

static void test_types()
{
	test_eq("12345", "{}", (int32_t)12345);
	test_eq("12345", "{}", (uint32_t)12345);
	test_eq("12345", "{}", (int64_t)12345);
	test_eq("12345", "{}", (uint64_t)12345);
	test_eq("12345", "{}", (int16_t)12345);
	test_eq("12345", "{}", (uint16_t)12345);

	volatile int64_t vi64 = 12345;
	test_eq("12345", "{}", vi64);

	volatile uint64_t vui64 = 12345;
	test_eq("12345", "{}", vui64);

	volatile int32_t vi32 = 12345;
	test_eq("12345", "{}", vi32);

	volatile uint32_t vui32 = 12345;
	test_eq("12345", "{}", vui32);

	volatile int16_t vi16 = 12345;
	test_eq("12345", "{}", vi16);

	volatile uint16_t uvi16 = 12345;
	test_eq("12345", "{}", uvi16);
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

#ifdef MICRO_FORMAT_INT64
	test_eq("18446744073709551615", "{:}", UINT64_MAX);
	test_eq("9223372036854775807", "{:}", INT64_MAX);
	test_eq("-9223372036854775808", "{:}", INT64_MIN);
#endif
	test_eq("4294967295", "{:}", UINT32_MAX);
	test_eq("2147483647", "{:}", INT32_MAX);
	test_eq("-2147483648", "{:}", INT32_MIN);

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

#ifdef MICRO_FORMAT_INT64
	test_eq("ffffffffffffffff", "{:x}", UINT64_MAX);
#endif
	test_eq("ffffffff", "{:x}", UINT32_MAX);

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

#ifdef MICRO_FORMAT_INT64
	test_eq("1111111111111111111111111111111111111111111111111111111111111111", "{:b}", UINT64_MAX);
#endif
	test_eq("11111111111111111111111111111111", "{:b}", UINT32_MAX);

	// octal

	test_eq("1234567",    "{:o}", 01234567);
	test_eq("01234567",   "{:#o}", 01234567);
	test_eq("  01234567", "{:#10o}", 01234567);
	test_eq("   1234567", "{:10o}", 01234567);
	test_eq("  -1234567", "{:10o}", -01234567);

#ifdef MICRO_FORMAT_INT64
	test_eq("1777777777777777777777", "{:o}", UINT64_MAX);
#endif
	test_eq("37777777777", "{:o}", UINT32_MAX);

	// to char

	test_eq("AB", "{:c}{:c}", 65, 66);

	// errors

	test_eq(error_str, "{:s}", 123);
	test_eq(error_str, "{:f}", 123);

	// x64

	test_eq("1000000000000", "{}", 1000'000'000'000ULL);
	test_eq("-1000000000000", "{}", -1000'000'000'000);

	test_eq("FFFFFFFFFFFFFFFF", "{:X}", 0xFFFFFFFFFFFFFFFFULL);
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

static void test_individual_functions()
{
	char buffer[256] = {};

	mf::format_dec(buffer, 777);
	assert(strcmp(buffer, "777") == 0);

	mf::format_dec(buffer, -12345);
	assert(strcmp(buffer, "-12345") == 0);

	mf::format_dec(buffer, 12345);
	assert(strcmp(buffer, "12345") == 0);

	mf::format_hex(buffer, 0xabcd4321);
	assert(strcmp(buffer, "abcd4321") == 0);

	mf::format_bin(buffer, 0b111100111000110);
	assert(strcmp(buffer, "111100111000110") == 0);

	mf::format_float(buffer, -1234.56789, 3);
	assert(strcmp(buffer, "-1234.568") == 0);

	mf::format_float(buffer, INFINITY, 3);
	assert(strcmp(buffer, "inf") == 0);

	mf::format_float(buffer, -INFINITY, 3);
	assert(strcmp(buffer, "-inf") == 0);

	mf::format_float(buffer, NAN, 3);
	assert(strcmp(buffer, "nan") == 0);
}

static void test_print_to_buffer()
{
	char buffer1[3] = {0, 1, 2};
	auto printed = mf::format_dec(buffer1, 2, -12345);
	assert(printed == 1);
	assert(buffer1[0] == '-');
	assert(buffer1[1] == 0);
	assert(buffer1[2] == 2);

	char buffer2[3] = { 0, 1, 2 };
	printed = mf::format_dec(buffer2, 2, 12345U);
	assert(printed == 1);
	assert(buffer2[0] == '1');
	assert(buffer2[1] == 0);
	assert(buffer2[2] == 2);

	char buffer3[7] = { 0, 1, 2, 3, 4, 5, 6 };
	printed = mf::format_float(buffer3, 6, -1.123456789, 10);
	assert(printed == 5);
	assert(buffer3[0] == '-');
	assert(buffer3[1] == '1');
	assert(buffer3[2] == '.');
	assert(buffer3[3] == '1');
	assert(buffer3[4] == '2');
	assert(buffer3[5] == 0);
	assert(buffer3[6] == 6);

	char buffer4[7] = { 0, 1, 2, 3, 4, 5, 6 };
	printed = mf::format(buffer4, 6, "{}{}{}{}{}{}{}{}{}{}", 10, 20, 30, 40, 50, 60, 70);
	assert(printed == 5);
	assert(buffer4[0] == '1');
	assert(buffer4[1] == '0');
	assert(buffer4[2] == '2');
	assert(buffer4[3] == '0');
	assert(buffer4[4] == '3');
	assert(buffer4[5] == 0);
	assert(buffer4[6] == 6);
}

static void test_utf8()
{
	// correct sequenses
	test_eq_unicode(u8"Русский текст",                u8"Русский текст");
	test_eq_unicode(u8"日本語テキスト",                 u8"日本語テキスト");
	test_eq_unicode(u8"Русский текст 日本語テキスト",   u8"Русский текст {}", u8"日本語テキスト");
	test_eq_unicode(u8"-Русский текст-日本語テキスト-", "-{}-{}-", u8"Русский текст", u8"日本語テキスト");

	// wrong sequenses
	test_eq_unicode("before ? after", "before \xC0\xC1 after");
}

int main()
{
	test_common();
	test_types();
	test_integer();
	test_bool();
	test_str();
	test_char();
	test_float();
	test_arg_pos();
	test_individual_functions();
	test_print_to_buffer();
	test_utf8();
}
