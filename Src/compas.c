#include <stdio.h>
#include "main.h"
#include "stm32f3xx_hal.h"
#include "events_handler.h"
#include "lsm303dlhc_driver.h"


static I2C_HandleTypeDef hi2c1;
osSemaphoreId  compasSemaphore;

static void MX_I2C1_Init(void);
static void compas_task(void const* param);
static void configure_compas(void);

void SetHandleI2C(I2C_HandleTypeDef* hi2c_p);

void init_compas(void)
{
    MX_I2C1_Init();
    configure_compas();
    
    HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    
    osSemaphoreDef(compasSemaphore);
    compasSemaphore = osSemaphoreCreate(osSemaphore(compasSemaphore), 1);
    
    osThreadDef(compasTask, compas_task, osPriorityNormal, 0, 128);
    osThreadCreate(osThread(compasTask), NULL);
}


/* I2C1 init function */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x2000090E;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    /**Configure Analogue filter 
    */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    /**Configure Digital filter 
    */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
}


static void configure_compas(void)
{
    SetHandleI2C(&hi2c1);
    
    printf("configure accelerometer\n");
    USER_ASSERT(SetMode(POWER_DOWN) == MEMS_SUCCESS);
    USER_ASSERT(SetMode(NORMAL) == MEMS_SUCCESS);
    USER_ASSERT(SetODR(ODR_25Hz) == MEMS_SUCCESS);
    USER_ASSERT(SetAxis(X_ENABLE | Y_ENABLE | Z_ENABLE) == MEMS_SUCCESS);
    USER_ASSERT(SetFullScale(FULLSCALE_2) == MEMS_SUCCESS);     // Full-scale +-2g
    USER_ASSERT(SetHPFMode(HPM_NORMAL_MODE) == MEMS_SUCCESS);   // High pass filter mode selection
    USER_ASSERT(SetHPFCutOFF(HPFCF_2) == MEMS_SUCCESS);         // High pass filter cut-off frequency selection
    USER_ASSERT(SetFilterDataSel(MEMS_ENABLE) == MEMS_SUCCESS); // enable filter
    USER_ASSERT(SetBLE(BLE_LSB) == MEMS_SUCCESS); // endian
    USER_ASSERT(SetInt1Pin(1 << I1_AOI1) == MEMS_SUCCESS);     // enable AOI1 interrupt on INT1
    USER_ASSERT(Int1LatchEnable(MEMS_ENABLE) == MEMS_SUCCESS); // If there is an interrupt from AOI1, INT1 pin will go high from
                                  // low and stay high. Reading the INT1_SRC(31h) register will clear
                                  // the interrupt signal on INT1 pin.
    USER_ASSERT(SetIntMode(INT_MODE_6D_MOVEMENT) == MEMS_SUCCESS); // detect movement
    USER_ASSERT(SetInt1Configuration(0x3f) == MEMS_SUCCESS);   // enable all directions
    USER_ASSERT(SetInt1Threshold(10) == MEMS_SUCCESS);         // 10 * 15.625mg
}

//volatile int releaseCompas = 3;

static void compas_task(void const* param)
{
    static AccAxesRaw_t receivedAxes;
    
    while(1)
    {
        osSemaphoreWait(compasSemaphore, osWaitForever);
        
        //if (releaseCompas > 0)
        {
            USER_ASSERT(GetAccAxesRaw(&receivedAxes) == MEMS_SUCCESS);
            printf("x: %d, y: %d, z: %d\n",
                receivedAxes.AXIS_X, receivedAxes.AXIS_Y, receivedAxes.AXIS_Z);
        }
        /*printf("work");
        osDelay(1000);*/
    }
}


void EXTI4_IRQHandler(void)
{
    osSemaphoreRelease(compasSemaphore);
    //printf("EXTI4");
    
    HAL_NVIC_ClearPendingIRQ(EXTI4_IRQn);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI0_IRQHandler(void)
{
    //printf("s\n");
    
    //xSemaphoreGiveFromISR( compasSemaphore, NULL );
    osSemaphoreRelease(compasSemaphore);
    //printf("EXTI0\n");
    //releaseCompas++;
    
    HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}
