#pragma once
     
#include <type_traits>
#include <stdint.h>

using FormatCallback = bool (*)(void* data, char character);

namespace impl {

enum class FormatArgType : uint8_t
{
	Undef,
	CharPtr,
	Int8,
	Int16,
	Int32,
	UInt8,
	UInt16,
	UInt32,
	Bool,
	Pointer,
	Char,
	Float,
};

struct FormatArg
{
	const void* pointer = nullptr;
	FormatArgType type = {};

	
	FormatArg(const int8_t   &v) : pointer(&v), type(FormatArgType::Int8)    {}
	FormatArg(const int16_t  &v) : pointer(&v), type(FormatArgType::Int16)   {}
	FormatArg(const int32_t  &v) : pointer(&v), type(FormatArgType::Int32)   {}
	FormatArg(const uint8_t  &v) : pointer(&v), type(FormatArgType::UInt8)   {}
	FormatArg(const uint16_t &v) : pointer(&v), type(FormatArgType::UInt16)  {}
	FormatArg(const uint32_t &v) : pointer(&v), type(FormatArgType::UInt32)  {}
	FormatArg(bool           &v) : pointer(&v), type(FormatArgType::Bool)    {}
	FormatArg(const char     *v) : pointer(v),  type(FormatArgType::CharPtr) {}

	template <typename T>
	FormatArg(const T        *v) : pointer(v),  type(FormatArgType::Pointer) {}

	FormatArg(const char     &v) : pointer(&v), type(FormatArgType::Char) {}
	FormatArg(const float    &v) : pointer(&v), type(FormatArgType::Float)   {}
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

} // namespace impl

template <typename ... Args>
size_t cb_format(FormatCallback callback, void* data, const char* format, const Args& ... args)
{
	const impl::FormatArg args_arr[] = { args ... };
	impl::FormatCtx ctx { callback, data, args_arr, sizeof ... (args), 0U };
	impl::format_impl(ctx, format);
	return ctx.chars_printed;
}
