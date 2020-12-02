# Simple implementation of c++20 string formating for microcontrollers

# Supported features
* Presentations: `b`, `B`, `c`, `d`, `o`, `x`, `X`, `f`, `F`, `s`, `p`
* Flags: `-`, `+`, ` `, `0`, `#`,  `<`, `^`, `>`
* Argument position
* Field width
* Precision for float type
* `-inf`, `+inf` and `nan` support for float type
* Wrong type error detection

# Limitations
* `#` not supported for float types
* `L` option (locale-specific formatting) not supported
* Only `f` presentation for float type is supported. 
* Format strings like `{{}.{}}` not supported

## How to use

Read it first. How to write "replacement fields" for output strings: https://fmt.dev/latest/syntax.html#format-specification-mini-language 

### Callback version

1
```cpp
static bool uart_format_callback(void* data, char character)
{
    uart_send_char(character);
    return true;
}

cb_format(uart_format_callback, nullptr, "{:.2} {} {:10}", 1.2f, 2, 42U);
```
2
```cpp
static bool uart_format_callback(void* data, char character)
{
    uart_send_char(character);
    return true;
}

template <typename ... Args>
size_t print_to_uart(const char* format, const Args& ... args)
{
    return cb_format(uart_format_callback, nullptr, format, args...);
}

...

print_to_uart("Hello world!!!\n");
print_to_uart("Hello {}\n", "world!!!");
print_to_uart("{} {}\n", "Hello", "world!!!");
print_to_uart("{1} {0}\n", "world!!!", "Hello");
print_to_uart("U={:8.2}v, I={:8.2}A\n", 11.2f, 0.1f);
```

### Print to string buffer
```cpp
char my_buffer[64];
s_format(my_buffer, "{} {} {}", "Printing", "to", "buffer");
```
