# C++ library for std::format-like text formating for microcontrollers
```diff
! Warning! Library is in developing and not ready to be used in production code !
```

## Supported features
* Presentations: `b`, `B`, `c`, `d`, `o`, `x`, `X`, `f`, `F`, `s`, `p`
* Flags: `-`, `+`, ` `, `0`, `#`,  `<`, `^`, `>`
* Argument position
* Field width
* `float` and `double` types are supported (`-inf`, `+inf` and `nan` also works)
* Wrong type error detection

## Limitations
* `#` not supported for float types
* `L` option (locale-specific formatting) not supported
* Only `f` presentation for float type is supported. 
* Format strings like `{{}.{}}` not supported

## How to use

Read it first. How to write "replacement fields" for output strings: https://fmt.dev/latest/syntax.html#format-specification-mini-language 

### Print to string buffer
```cpp
char my_buffer[64];
mf::format(my_buffer, "{} {} {}", "Printing", "to", "buffer");
```

### Callback version
1
```cpp
static bool uart_format_callback(void* data, char character)
{
    uart_send_char(character);
    return true;
}

mf::format(uart_format_callback, nullptr, "{:.2} {} {:10}", 1.2f, 2, 42U);
```
2
```cpp
template <typename ... Args>
size_t print_to_uart(const char* format, const Args& ... args)
{
    auto uart_format_callback = [](auto, char character)
    {
        uart_send_char(character);
        return true;
    };
    return mf::format(uart_format_callback, nullptr, format, args...);
}

...

print_to_uart("Hello world!!!\n");
print_to_uart("Hello {}\n", "world!!!");
print_to_uart("{} {}\n", "Hello", "world!!!");
print_to_uart("{1} {0}\n", "world!!!", "Hello");
print_to_uart("U={:8.2}v, I={:8.2}A\n", 11.2f, 0.1f);
```

More examples or replacement fields are in test sources: [micro_format_tests.cpp](tests/micro_format_tests.cpp)

### Misc functions
Library contains simple functions for non-format style convertion values to string:
* `format_dec` - to print integer as decimal number
* `format_hex` - to print integer as hexadecimal number
* `format_bin` - to print integer as binary number
* `format_float` - to print floating point number

Both print to buffer and callback versions are presented

```cpp
mf::format_dec(my_buffer, 42);

mf::format_dec(uart_format_callback, nullptr, 42);

mf::format_float(my_buffer, 1234.5678, 4);

```

## Using of float and double arguments
Library doesn't compile with `float` and `double` types support by default to reduce binary size of firmware. To use `float` type you have do define `MICRO_FORMAT_FLOAT` macro in you project. To use both `float` and `double` define `MICRO_FORMAT_DOUBLE`

## Compiled binary size (gcc-arm-9 -Os)
* Binary size of compiled library without `float` and `double` support takes less than 2Kb for my cortex-m0 micrcocontroller
* Each new combination of arguments types for `cb_format` takes about 80 bytes
* Call `cb_format` for existing combination of types of arguments takes about 40 bytes
