#include <stdio.h>
#include <ctype.h>
#include "shell.h"
#include "cmsis_os.h"
#include "text_output.h"
#include "stm32f3xx_hal.h"

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef shellUart;

/**@struct __FILE
* @brief descriptin for __stdout and stdin
*
* These must be defined to avoid linking in stdio.o from the
* C Library
*/
struct __FILE { int handle;   /* Add whatever you need here */};
FILE __stdout;
FILE __stdin;   ///it is important to define __stdout and __stdin

int last_char_read;     ///< used by __backspace function to return the last char read to the stream
int backspace_called;   ///< fgetc() needs to keep a record of whether __backspace was called directly before it

/** @brief set backspace_called accordingly
*
* The effect of __backspace() should be to return the last character
* read from the stream, such that a subsequent fgetc() will
* return the same character again.__backspace() must also be retargeted
* with this layer to enable scanf().
* @see the Compiler and Libraries Guide.
* @param[in] f  file stream pointer
* @return  always return 1
*/
int __backspace(FILE *f)
{
    backspace_called = 1;
    return 1;
}

__STATIC_INLINE void Serial_SendChar(uint8_t ch)
{
    HAL_UART_Transmit(&shellUart, (uint8_t*)&(ch), 1, HAL_MAX_DELAY);
}

__STATIC_INLINE uint8_t Serial_WaitForChar()
{
    uint8_t ch;
    HAL_UART_Receive(&shellUart, (uint8_t*)&(ch), 1, HAL_MAX_DELAY);
    return ch;
}

/**@brief   Retargeted I/O fputc
*
* The following C library functions make use of semihosting
* to read or write characters to the debugger console: fputc(),
* fgetc(), and _ttywrch().  They must be retargeted to write to
* the SOC_LITE COMBO board UART.
*/
int fputc(int ch, FILE *f)
{
    ///own implementation of fputc here

    ///write a character to UART
    Serial_SendChar(ch);
    // convert '\r' to \n\r
    if (ch=='\n')
    {
        Serial_SendChar('\r');
    }

    return ch;
}

char _getkey(void)
{
    uint8_t tempch;
    /// if we just backspaced, then return the backspaced character
    /// otherwise output the next character in the stream
    if (backspace_called == 1)
    {
        backspace_called = 0;
        return last_char_read;
    }

    tempch = Serial_WaitForChar();
    if(tempch == '\r')
    {
        tempch = '\n';
    }
        
    last_char_read = (int)tempch;       /// backspace must return this value when it gets called
    return tempch;
}

int fgetc(FILE *f)
{
    uint8_t tempch;
    /// if we just backspaced, then return the backspaced character
    /// otherwise output the next character in the stream
    if (backspace_called == 1)
    {
      backspace_called = 0;
      return last_char_read;
    }
    
    tempch = Serial_WaitForChar();
    // do local echo for gets(), fgets(), scanf()
    if(tempch=='\r')
    {
        tempch = '\n';
    }

    if (isprint(tempch) || tempch=='\n')
    {
        Serial_SendChar(tempch);
    }
    else
    {
        Serial_SendChar('.');
    }

    last_char_read = (int)tempch;       /// backspace must return this value when it gets called
    return tempch;
}

/** @brief Emergency output over UART
 *
 * @param[in] ch character to be output to UART
 */
void _ttywrch(int ch)
{
    char tempch = ch;
    Serial_SendChar( tempch );
}

/*
#define SHELL_UART_FIFO_SIZE     10
#define FIFO_INCR(x)             (x) = (x + 1) % SHELL_UART_FIFO_SIZE;

typedef struct
{
    uint8_t uart_fifo_buff[SHELL_UART_FIFO_SIZE];
    uint8_t countUnread;
    uint8_t writePos;
} UartFifo_t;

static UartFifo_t uart_fifo;

__STATIC_INLINE void putToFifo(uint8_t ch)
{
    uart_fifo.uart_fifo_buff[uart_fifo.writePos] = ch;
    if (uart_fifo.countUnread < SHELL_UART_FIFO_SIZE)
        uart_fifo.countUnread++;
    FIFO_INCR(uart_fifo.writePos);
}

__STATIC_INLINE uint8_t getFromFifo(void)
{
    uint8_t readPos = SHELL_UART_FIFO_SIZE + uart_fifo.writePos + uart_fifo.countUnread;
    if (uart_fifo.countUnread > 0)
        uart_fifo.countUnread--;
    return uart_fifo.uart_fifo_buff[readPos];
}*/

osSemaphoreId  shellSemaphore;

static void ShellTask(void const* param)
{
    uint8_t ch;
    
    osSemaphoreWait(shellSemaphore, 0);
    
    while (1)
    {
        HAL_UART_Receive_IT(&shellUart, (uint8_t*)&(ch), 1);
        osSemaphoreWait(shellSemaphore, osWaitForever);
        if(ch == '\r')
        {
            ch = '\n';
        }
        Shell_task(ch);
    }
    
}

/* USART1 init function */
void Shell_UART_Init(void)
{
    shellUart.Instance = USART2;
    shellUart.Init.BaudRate = 38400;
    shellUart.Init.WordLength = UART_WORDLENGTH_8B;
    shellUart.Init.StopBits = UART_STOPBITS_1;
    shellUart.Init.Parity = UART_PARITY_NONE;
    shellUart.Init.Mode = UART_MODE_TX_RX;
    shellUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    shellUart.Init.OverSampling = UART_OVERSAMPLING_16;
    shellUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    shellUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&shellUart) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
    
    osSemaphoreDef(shellSemaphore);
    shellSemaphore = osSemaphoreCreate(osSemaphore(shellSemaphore), 1);
    
    osThreadDef(ShellTask, ShellTask, osPriorityLow, 0, 128);
    osThreadCreate(osThread(ShellTask), NULL);
}


void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&shellUart);
    
    osSemaphoreRelease(shellSemaphore);
}
