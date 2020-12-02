# Very simple implementation of c++20 string formating for microcontrollers

# Supported features
* Presentations: `b`, `B`, `c`, `d`, `o`, `x`, `X`, `f`, `F`, `s`, `p`
* Flags: `-`, `+`, ` `, `0`, `#`,  `<`, `^`, `>`
* Argument position
* Field width
* Precision for float type
* Wrong type error detection

# Limitations
* `#` not supported for float types
* `L` option (locale-specific formatting) not supported
* Only `f` presentation for float type is supported. 

## How to use

```cpp
static bool uart_format_callback(void* data, char character)
{
    uart_send_char(character);
    return true;
}

cb_format(uart_format_callback, nullptr, "{:.2} {} {:10}", 1.2f, 2, 42U);
```
