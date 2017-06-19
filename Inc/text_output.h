#ifndef __TEXT_OUTPUT_H
#define __TEXT_OUTPUT_H
#include <stdint.h>
#include "main.h"

#define MAX_UART_STR_SIZE  127

void Shell_UART_Init(void);
int UART_printf(char *format, ...);

#endif // __TEXT_OUTPUT_H
