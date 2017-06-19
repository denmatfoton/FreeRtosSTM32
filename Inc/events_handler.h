#ifndef EVENTS_HANDLER_H
#define EVENTS_HANDLER_H
#include <stdint.h>
#include "cmsis_os.h"


typedef enum
{
    EVENT_COMPAS,
    EVENT_ACCEL
} event_type_t;

typedef struct
{
    event_type_t type;
    uint32_t value;
} event_t;

extern xQueueHandle  h_eventsQueue;


void init_events_handler(void);

#endif // EVENTS_HANDLER_H
