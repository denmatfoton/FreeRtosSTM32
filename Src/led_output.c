#include "led_output.h"
#include "main.h"
#include "stm32f3xx_hal.h"


#define LEDS_NUM  8

void Set_LEDs(uint8_t value)
{
    static const uint16_t ledArray[LEDS_NUM] = {LD3_Pin, LD5_Pin, LD7_Pin,
                                LD9_Pin, LD10_Pin, LD8_Pin, LD6_Pin, LD4_Pin};
    uint16_t i, output = 0;
    
    HAL_GPIO_WritePin(GPIOE, LD3_Pin | LD4_Pin | LD5_Pin 
                          | LD6_Pin | LD7_Pin | LD8_Pin | LD9_Pin | LD10_Pin
                          , GPIO_PIN_RESET);
    
    for (i = 0; i < LEDS_NUM; ++i)
    {
        if (value & (1 << i))
        {
            output |= ledArray[i];
        }
    }
    
    HAL_GPIO_WritePin(GPIOE, output, GPIO_PIN_SET);
}
