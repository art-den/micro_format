# Very simple implementation of c++x20 string formating for microcontrollers

## How to use

```cpp
static bool uart_format_callback(void* data, char character)
{
    uart_send_char(character);
    return true;
}

cb_format(uart_format_callback, nullptr, "{:.2} {} {:10}", 1.2f, 2, 42U);
```
