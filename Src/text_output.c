#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include "shell.h"
#include "cmsis_os.h"
#include "led_output.h"
#include "text_output.h"
#include "stm32f3xx_hal.h"

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef shellUart;
DMA_HandleTypeDef hdma_usart2_tx;



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
    //HAL_UART_Transmit(&shellUart, (uint8_t*)&(ch), 1, HAL_MAX_DELAY);
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
    //write a character to UART
    Serial_SendChar(ch);
    // convert '\n' to \n\r
    if (ch=='\n')
    {
        Serial_SendChar('\r');
    }

    return ch;
}

osSemaphoreId  printfSemaphore;
osSemaphoreId  shellSemaphore;


int _UART_printf(uint32_t millis, char *format, ...)
{
    static char outputStr[MAX_UART_STR_SIZE];
    int ret;
    
    ret = osSemaphoreWait(printfSemaphore, millis);
    
    if (ret == osOK)
    {
        va_list aptr;
        int i;
        char *pos;
        uint32_t endPos; 

        va_start(aptr, format);
        ret = vsnprintf(outputStr, MAX_UART_STR_SIZE, format, aptr);
        va_end(aptr);
        
        if (ret < 0)
        {
            return ret;
        }
        
        endPos = ret;
        pos = outputStr;
        for (i = 0; i < ret && endPos < MAX_UART_STR_SIZE; ++i)
        {
            if (*(pos++) == '\n')
            {
                if (i + 1 != ret)
                {
                    memmove(pos + 1, pos, ret - i);
                }
                *(pos++) = '\r';
                endPos++;
            }
        }
        
        HAL_UART_Transmit_DMA(&shellUart, (uint8_t*)outputStr, endPos);
        
        return endPos;
    }
    
    return -1;
}


static void ShellTask(void const* param)
{
    uint8_t ch;
    
    osSemaphoreWait(shellSemaphore, 0);
    
    while (1)
    {
        HAL_UART_Receive_IT(&shellUart, (uint8_t*)&(ch), 1);
        osSemaphoreWait(shellSemaphore, osWaitForever);
        
        if (ch == '\r')
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
    
    osSemaphoreDef(printfSemaphore);
    printfSemaphore = osSemaphoreCreate(osSemaphore(printfSemaphore), 1);
    
    osThreadDef(ShellTask, ShellTask, osPriorityLow, 0, 128);
    osThreadCreate(osThread(ShellTask), NULL);
}


void USART2_IRQHandler(void)
{
    HAL_UART_StateTypeDef oldRxState = shellUart.RxState;
    HAL_UART_StateTypeDef oldTxState = shellUart.gState;
    
    HAL_UART_IRQHandler(&shellUart);
    
    if (shellUart.RxState == HAL_UART_STATE_READY &&
        oldRxState != shellUart.RxState)
    {
        osSemaphoreRelease(shellSemaphore);
    }
        
    if (shellUart.gState == HAL_UART_STATE_READY &&
        oldTxState != shellUart.gState)
    {
        osSemaphoreRelease(printfSemaphore);
    }
}

void DMA1_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
    
    
    if (hdma_usart2_tx.State == HAL_DMA_STATE_READY)
    {
        //osSemaphoreRelease(printfSemaphore);
    }
}
