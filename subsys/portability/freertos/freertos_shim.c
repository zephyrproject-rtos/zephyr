/*
 * SPDX-File-CopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * FreeRTOS API shim for most FreeRTOS-related APIs.
 *
 * Each function in this file is declared as __weak so that any particular
 * platform can decide to override them with alternative implementation.
 *
 * For instance, implementation provided in a binary file, in a
 * ROM-resident bootloader, or platform-level implementation with
 * customization.
 */

#include <stdint.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/portability/freertos.h>

static K_HEAP_DEFINE(freertos_heap, CONFIG_FREERTOS_HEAP_SIZE);

static k_timeout_t ticks_to_timeout(uint32_t xTicksToWait)
{
	if (xTicksToWait == 0U) {
		return K_NO_WAIT;
	} else if (xTicksToWait == portMAX_DELAY) {
		return K_FOREVER;
	}

	return K_MSEC(xTicksToWait / portTICK_PERIOD_MS);
}

static void task_entry_wrapper(void *p1, void *p2, void *p3)
{
	struct freertos_task *task = (struct freertos_task *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	task->func(task->arg);
}

__weak int32_t xTaskCreate(TaskFunction_t pxTaskCode, const char *pcName, uint16_t usStackDepth,
			   void *pvParameters, uint32_t uxPriority, TaskHandle_t *pxCreatedTask)
{
	size_t stack_bytes = (size_t)usStackDepth * sizeof(portSTACK_TYPE);
	struct freertos_task *task;
	int zprio;

	task = k_heap_alloc(&freertos_heap, sizeof(*task), K_NO_WAIT);
	if (task == NULL) {
		return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
	}

	task->stack = k_heap_alloc(&freertos_heap, stack_bytes, K_NO_WAIT);
	if (task->stack == NULL) {
		k_heap_free(&freertos_heap, task);
		return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
	}
	task->stack_size = stack_bytes;
	task->func = pxTaskCode;
	task->arg = pvParameters;

	/* FreeRTOS: higher number = higher priority
	 * Zephyr: lower number = higher priority
	 */
	zprio = CONFIG_NUM_PREEMPT_PRIORITIES - 1 - (int)uxPriority;
	if (zprio < 0) {
		zprio = 0;
	}

	k_thread_create(&task->thread, task->stack, stack_bytes, task_entry_wrapper,
			task, NULL, NULL, zprio, 0, K_FOREVER);
	k_thread_name_set(&task->thread, pcName);

	if (pxCreatedTask != NULL) {
		*pxCreatedTask = task;
	}

	k_thread_start(&task->thread);

	return pdPASS;
}

__weak void vTaskDelete(TaskHandle_t xTaskToDelete)
{
	struct freertos_task *task = (struct freertos_task *)xTaskToDelete;

	if (task != NULL) {
		k_thread_abort(&task->thread);
		k_heap_free(&freertos_heap, task->stack);
		k_heap_free(&freertos_heap, task);
	}
}

__weak void vTaskDelay(uint32_t xTicksToDelay)
{
	k_sleep(ticks_to_timeout(xTicksToDelay));
}

__weak void vTaskSwitchContext(void)
{
	k_yield();
}

__weak QueueHandle_t xQueueGenericCreate(uint32_t uxQueueLength, uint32_t uxItemSize,
					 uint8_t ucQueueType)
{
	struct freertos_queue *q;

	/* TODO: support checking of queue types */
	ARG_UNUSED(ucQueueType);

	if (uxItemSize == 0U) {
		/*
		 * FreeRTOS uses zero-sized items for semaphores and mutexes:
		 * xQueueGenericCreate(1, 0, queueQUEUE_TYPE_BINARY_SEMAPHORE).
		 * Map to k_sem — no buffer allocation needed.
		 */
		q = k_heap_alloc(&freertos_heap, sizeof(*q), K_NO_WAIT);
		if (q == NULL) {
			return NULL;
		}
		q->is_sem = true;
		k_sem_init(&q->sem, 0, uxQueueLength);
	} else {
		size_t buf_bytes = (size_t)uxQueueLength * uxItemSize;

		q = k_heap_alloc(&freertos_heap, sizeof(*q) + buf_bytes, K_NO_WAIT);
		if (q == NULL) {
			return NULL;
		}
		q->is_sem = false;
		k_msgq_init(&q->msgq, q->buf, uxItemSize, uxQueueLength);
	}

	return q;
}

__weak int32_t xQueueGenericSend(QueueHandle_t xQueue, const void *pvItemToQueue,
				 uint32_t xTicksToWait, int32_t xCopyPosition)
{
	struct freertos_queue *q = (struct freertos_queue *)xQueue;

	/* TODO: support insertion order in the queue */
	ARG_UNUSED(xCopyPosition);

	if (q->is_sem) {
		k_sem_give(&q->sem);
		return pdPASS;
	}

	return (k_msgq_put(&q->msgq, pvItemToQueue, ticks_to_timeout(xTicksToWait)) == 0)
	       ? pdPASS
	       : errQUEUE_FULL;
}

__weak int32_t xQueueGenericSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue,
					int32_t *pxHigherPriorityTaskWoken, int32_t xCopyPosition)
{
	struct freertos_queue *q = (struct freertos_queue *)xQueue;

	/* TODO: support insertion order in the queue */
	ARG_UNUSED(xCopyPosition);

	if (pxHigherPriorityTaskWoken != NULL) {
		*pxHigherPriorityTaskWoken = pdFALSE;
	}

	if (q->is_sem) {
		k_sem_give(&q->sem);
		return pdPASS;
	}

	return (k_msgq_put(&q->msgq, pvItemToQueue, K_NO_WAIT) == 0) ? pdPASS : errQUEUE_FULL;
}

__weak int32_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, uint32_t xTicksToWait)
{
	struct freertos_queue *q = (struct freertos_queue *)xQueue;
	k_timeout_t timeout = ticks_to_timeout(xTicksToWait);

	if (q->is_sem) {
		return (k_sem_take(&q->sem, timeout) == 0) ? pdPASS : pdFAIL;
	}

	return (k_msgq_get(&q->msgq, pvBuffer, timeout) == 0) ? pdPASS : pdFAIL;
}

__weak void vQueueDelete(QueueHandle_t xQueue)
{
	struct freertos_queue *q = (struct freertos_queue *)xQueue;

	if (q != NULL) {
		if (q->is_sem) {
			k_sem_reset(&q->sem);
		} else {
			k_msgq_purge(&q->msgq);
		}
		k_heap_free(&freertos_heap, q);
	}
}

__weak void *pvPortMalloc(size_t xWantedSize)
{
	return k_heap_alloc(&freertos_heap, xWantedSize, K_NO_WAIT);
}

__weak void vPortFree(void *pv)
{
	k_heap_free(&freertos_heap, pv);
}
