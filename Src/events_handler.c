#include <stdio.h>
#include "events_handler.h"
#include "stm32f3xx_hal.h"


static osThreadId    eventsThread;
xQueueHandle  h_eventsQueue;

static void events_handler(const void* param);

void init_events_handler(void)
{
    h_eventsQueue = xQueueCreate(8, sizeof(event_t));
    
    osThreadDef(defaultTask, events_handler, osPriorityNormal, 0, 128);
    eventsThread = osThreadCreate(osThread(defaultTask), NULL);
}

static void events_handler(const void* param)
{
    while (1)
    {
        osDelay(10);
    }
}
