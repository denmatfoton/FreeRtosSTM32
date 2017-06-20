#ifndef __TEXT_OUTPUT_H
#define __TEXT_OUTPUT_H
#include <stdint.h>
#include "main.h"

#define MAX_UART_STR_SIZE  127

void Shell_UART_Init(void);
int _UART_printf(uint32_t millis, char *format, ...);
#define UART_printf(...)  _UART_printf(0, __VA_ARGS__)
#define UART_printf_wait(...)  _UART_printf(0xFFFFFFFF, __VA_ARGS__)

#endif // __TEXT_OUTPUT_H
