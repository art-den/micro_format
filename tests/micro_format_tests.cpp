#include <string>
#include <stdio.h>
#include <assert.h>

#include "../micro_format.hpp"


static bool print_to_str_callback(void* data, char character)
{
	auto str = (std::string*)data;
	str->push_back(character);
	return true;
}

static int errors_count = 0;

template <typename ... Args>
void test_eq(const char *desired, const char *format, const Args& ... args)
{
	std::string result;

	cb_format(print_to_str_callback, &result, format, args...);

	bool ok = result == desired;

	if (!ok)
	{
		errors_count++;
		printf("ERROR: \"%s\" -> \"%s\"\n", desired, result.c_str());

		// for debuging purposes
		std::string another_result;
		cb_format(print_to_str_callback, &another_result, format, args...);
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
}


int main()
{
	test_common();

	test_integer();

	return errors_count;
}