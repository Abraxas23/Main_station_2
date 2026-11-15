#ifndef INC_FREERTOS_MONITOR_H_
#define INC_FREERTOS_MONITOR_H_

#include <stdint.h>

void FreeRTOS_Monitor_TaskIn(void);
void FreeRTOS_Monitor_TaskOut(void);

typedef struct
{
    uint32_t heap_free;
    uint32_t heap_min_free;
} FreeRTOS_MemoryInfo_t;

FreeRTOS_MemoryInfo_t FreeRTOS_Monitor_GetMemoryInfo(void);

typedef struct
{
    uint32_t defaultTask_free;
    uint32_t DallasTask_free;
    uint32_t BME280_TASK_free;

} FreeRTOS_StackInfo_t;

FreeRTOS_StackInfo_t FreeRTOS_Monitor_GetStackInfo(void);

#endif /* INC_FREERTOS_MONITOR_H_ */
