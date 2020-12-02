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

template <typename ... Args>
void test_eq(const char *desired, const char *format, const Args& ... args)
{
	std::string result;

	cb_format(print_to_str_callback, &result, format, args...);

	bool ok = result == desired;

	if (!ok)
	{
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
	test_eq("42",  "{}",  42);
	test_eq("-42", "{}", -42);

	test_eq("+42", "{:+}", 42);
	test_eq("-42", "{:+}", -42);
	test_eq("+42", "{:+}", 42U);
	test_eq("42",  "{:-}", 42);
	test_eq("-42", "{:-}", -42);

	test_eq("   42", "{:5}", 42);
	test_eq("  -42", "{:5}", -42);

	test_eq("  +42", "{:+5}", 42);
	test_eq("  -42", "{:+5}", -42);

	test_eq("00042", "{:05}", 42);
	test_eq("-0042", "{:05}", -42);
	test_eq("+0042", "{:+05}", 42);

	test_eq("a", "{:x}", 0xa);
	test_eq("A", "{:X}", 0xa);

	test_eq("55553333", "{:x}", 0x55553333);
	test_eq("0x55553333", "{:#x}", 0x55553333);
	test_eq("0X55553333", "{:#X}", 0x55553333);

	test_eq("-0x55553333", "{:#x}", -0x55553333);

	test_eq("  0x123", "{:#7x}", 0x123);
	test_eq(" -0x123", "{:#7x}", -0x123);
	test_eq("-0x0123", "{:#07x}", -0x123);
	test_eq("0x00123", "{:#07x}", 0x123);
	test_eq("0000123", "{:07x}", 0x123);

	test_eq("123    ",  "{:<7x}", 0x123);
	test_eq("123    ",  "{:<7x}", 0x123);
	test_eq("0x123  ",  "{:<#7x}", 0x123);
	test_eq("  123  ",  "{:^7x}", 0x123);
	test_eq("  123   ", "{:^8x}", 0x123);
}

int main()
{
	test_common();

	test_integer();
}