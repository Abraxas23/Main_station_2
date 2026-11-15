#include "freertos_monitor.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

extern osThreadId_t defaultTaskHandle;
extern osThreadId_t DallasTaskHandle;
extern osThreadId_t BME280_TASKHandle;

FreeRTOS_MemoryInfo_t FreeRTOS_Monitor_GetMemoryInfo(void)
{
    FreeRTOS_MemoryInfo_t info;

    info.heap_free     = xPortGetFreeHeapSize();
    info.heap_min_free = xPortGetMinimumEverFreeHeapSize();

    return info;
}

//   Так используем в программе:

//   FreeRTOS_MemoryInfo_t mem = FreeRTOS_Monitor_GetMemoryInfo();
//   free_now     = mem.heap_free;              //  значение в байтах
//   free_min     = mem.heap_min_free;          //  значение в байтах



FreeRTOS_StackInfo_t FreeRTOS_Monitor_GetStackInfo(void)                 //    функция для мониторинга свободного стека задач
{
    FreeRTOS_StackInfo_t info;

    info.defaultTask_free =
        uxTaskGetStackHighWaterMark((TaskHandle_t)defaultTaskHandle)
        * sizeof(StackType_t);

    info.DallasTask_free =
        uxTaskGetStackHighWaterMark((TaskHandle_t)DallasTaskHandle)
        * sizeof(StackType_t);

    info.BME280_TASK_free =
        uxTaskGetStackHighWaterMark((TaskHandle_t)BME280_TASKHandle)
        * sizeof(StackType_t);

    return info;
}


/*    так используем в программе:


 	  FreeRTOS_StackInfo_t stack;

	  stack = FreeRTOS_Monitor_GetStackInfo();

	  uint32_t default_free = stack.defaultTask_free;
	  uint32_t dallas_free  = stack.DallasTask_free;
	  uint32_t bme_free     = stack.BME280_TASK_free;


 */


//*******************************************************************************************************************************************************
//  Метод определения распределения времени задач с помощью Ногодрыга GPIO и ЛОГИЧЕСКОГО АНАЛИЗАТОРА
//  добавить во FreeRTOSConfig.h 5 строк:

													//void FreeRTOS_Monitor_TaskIn(void);
													//void FreeRTOS_Monitor_TaskOut(void);

													//#define traceTASK_SWITCHED_IN()  FreeRTOS_Monitor_TaskIn()
													//#define traceTASK_SWITCHED_OUT() FreeRTOS_Monitor_TaskOut()
													//#define INCLUDE_xTaskGetIdleTaskHandle 1



void FreeRTOS_Monitor_TaskIn(void)                  //  PB10-PB13
{
    TaskHandle_t task = xTaskGetCurrentTaskHandle();

    if (task == defaultTaskHandle)
        GPIOB->BSRR = GPIO_PIN_10;

    else if (task == DallasTaskHandle)
        GPIOB->BSRR = GPIO_PIN_11;

    else if (task == BME280_TASKHandle)
        GPIOB->BSRR = GPIO_PIN_12;

    else if (task == xTaskGetIdleTaskHandle())
        GPIOB->BSRR = GPIO_PIN_13;
}

void FreeRTOS_Monitor_TaskOut(void)
{
    TaskHandle_t task = xTaskGetCurrentTaskHandle();

    if (task == defaultTaskHandle)
        GPIOB->BRR = GPIO_PIN_10;

    else if (task == DallasTaskHandle)
        GPIOB->BRR = GPIO_PIN_11;

    else if (task == BME280_TASKHandle)
        GPIOB->BRR = GPIO_PIN_12;

    else if (task == xTaskGetIdleTaskHandle())
        GPIOB->BRR = GPIO_PIN_13;
}
