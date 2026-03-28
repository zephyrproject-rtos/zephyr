/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * ROM function addresses for BL702 FreeRTOS and BLE.
 * These are PROVIDE (weak) definitions — our shim implementations override
 * them. They serve as a safety net: if any symbol is accidentally not
 * overridden, the linker resolves it to the ROM address rather than failing.
 *
 * Addresses extracted from vendor SDK flash_rom.ld.
 */

/* FreeRTOS ROM functions at 0x2102xxxx */
PROVIDE(xTaskCreate = 0x00000000210220e4);
PROVIDE(xTaskCreateStatic = 0x000000002102204e);
PROVIDE(vTaskDelete = 0x0000000021022160);
PROVIDE(vTaskDelay = 0x00000000210229dc);
PROVIDE(vTaskDelayUntil = 0x0000000021022968);
PROVIDE(vTaskSwitchContext = 0x0000000021022af2);
PROVIDE(xTaskGetCurrentTaskHandle = 0x000000002102327e);
PROVIDE(xTaskGetTickCount = 0x0000000021022642);
PROVIDE(xTaskGetTickCountFromISR = 0x0000000021022658);
/* uxTaskPriorityGet is NOT in BL702 ROM — shimmed in freertos_shim.c */

PROVIDE(xQueueGenericCreate = 0x0000000021020744);
PROVIDE(xQueueGenericCreateStatic = 0x00000000210206c2);
PROVIDE(xQueueGenericSend = 0x0000000021020834);
PROVIDE(xQueueGenericSendFromISR = 0x0000000021020a4c);
PROVIDE(xQueueReceive = 0x0000000021020b8a);
PROVIDE(vQueueDelete = 0x00000000210211d8);
PROVIDE(xQueueGiveFromISR = 0x0000000021020b0e);
PROVIDE(xQueueSemaphoreTake = 0x0000000021020ce8);
PROVIDE(xQueueCreateMutex = 0x0000000021020a1c);
PROVIDE(xQueueCreateMutexStatic = 0x0000000021020994);
PROVIDE(uxQueueMessagesWaiting = 0x0000000021021168);
PROVIDE(xQueueCreateCountingSemaphore = 0x0000000021020800);
PROVIDE(xQueueCreateCountingSemaphoreStatic = 0x00000000210207c2);
PROVIDE(xQueueGiveMutexRecursive = 0x00000000210209c8);
PROVIDE(xQueueTakeMutexRecursive = 0x0000000021020e70);

PROVIDE(pvPortMalloc = 0x0000000021024762);
PROVIDE(vPortFree = 0x000000002102483a);
PROVIDE(xPortGetFreeHeapSize = 0x00000000210248ce);
PROVIDE(vPortDefineHeapRegions = 0x00000000210248da);

PROVIDE(vTaskEnterCritical = 0x0000000021023510);
PROVIDE(vTaskExitCritical = 0x0000000021023520);
PROVIDE(vTaskList = 0x0000000021023534);
PROVIDE(vTaskStartScheduler = 0x00000000210225ba);
PROVIDE(pcTaskGetName = 0x0000000021022664);
PROVIDE(uxTaskGetStackHighWaterMark = 0x000000002102323c);

PROVIDE(vTaskNotifyGiveFromISR = 0x0000000021023a28);
PROVIDE(ulTaskNotifyTake = 0x0000000021023674);
PROVIDE(xTaskGenericNotify = 0x00000000210249d0);
PROVIDE(xTaskGenericNotifyFromISR = 0x00000000210249e4);

PROVIDE(xTimerGenericCommand = 0x0000000021023d2c);
PROVIDE(xTimerCreateTimerTask = 0x0000000021023bde);
PROVIDE(xTimerCreate = 0x0000000021023c36);
PROVIDE(xTimerCreateStatic = 0x0000000021023ca6);
PROVIDE(pvTimerGetTimerID = 0x0000000021024114);

PROVIDE(xTaskGetTickCount2 = 0x0000000021022648);
PROVIDE(xEventGroupCreateStatic = 0x0000000021020000);
PROVIDE(xEventGroupWaitBits = 0x0000000021020086);
PROVIDE(xEventGroupSetBits = 0x00000000210201e0);
PROVIDE(vEventGroupDelete = 0x00000000210203be);

PROVIDE(xStreamBufferGenericCreateStatic = 0x00000000210215c0);
PROVIDE(xStreamBufferReceive = 0x00000000210219ae);
PROVIDE(xStreamBufferSend = 0x00000000210217a8);
