/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FreeRTOS API shim for Bouffalo Lab BLE controller binary blobs.
 *
 * The BLE blobs call FreeRTOS functions (xTaskCreate, xQueueGenericSend,
 * pvPortMalloc, etc.). This shim maps them to Zephyr kernel primitives.
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stddef.h>

/* FreeRTOS compatibility constants */
#define pdPASS                                1
#define pdFAIL                                0
#define pdFALSE                               0
#define errQUEUE_FULL                         0
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY (-1)
#define STACK_WORD_SIZE                       4U
#define portMAX_DELAY                         0xFFFFFFFFU

static K_HEAP_DEFINE(shim_heap, CONFIG_BFLB_SHIM_HEAP_SIZE);

typedef void *TaskHandle_t;
typedef void *QueueHandle_t;
typedef void (*TaskFunction_t)(void *);

struct bflb_thread {
	struct k_thread thread;
	k_thread_stack_t *stack;
	size_t stack_size;
	TaskFunction_t func;
	void *arg;
};

struct bflb_queue {
	bool is_sem;
	union {
		struct k_sem sem;
		struct k_msgq msgq;
	};
	char buf[];
};

static k_timeout_t ticks_to_timeout(uint32_t xTicksToWait)
{
	if (xTicksToWait == 0U) {
		return K_NO_WAIT;
	} else if (xTicksToWait == portMAX_DELAY) {
		return K_FOREVER;
	}

	return K_MSEC(xTicksToWait);
}

static void task_entry_wrapper(void *p1, void *p2, void *p3)
{
	struct bflb_thread *bt = (struct bflb_thread *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	bt->func(bt->arg);
}

int32_t xTaskCreate(TaskFunction_t pxTaskCode, const char *pcName, uint16_t usStackDepth,
		    void *pvParameters, uint32_t uxPriority, TaskHandle_t *pxCreatedTask)
{
	size_t stack_bytes = (size_t)usStackDepth * STACK_WORD_SIZE;
	struct bflb_thread *bt;
	int zprio;

	bt = k_heap_alloc(&shim_heap, sizeof(*bt), K_NO_WAIT);
	if (bt == NULL) {
		return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
	}

	bt->stack = k_heap_alloc(&shim_heap, stack_bytes, K_NO_WAIT);
	if (bt->stack == NULL) {
		k_heap_free(&shim_heap, bt);
		return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
	}
	bt->stack_size = stack_bytes;
	bt->func = pxTaskCode;
	bt->arg = pvParameters;

	/* FreeRTOS: higher number = higher priority
	 * Zephyr: lower number = higher priority
	 */
	zprio = CONFIG_NUM_PREEMPT_PRIORITIES - 1 - (int)uxPriority;
	if (zprio < 0) {
		zprio = 0;
	}

	k_thread_create(&bt->thread, bt->stack, stack_bytes, task_entry_wrapper, bt, NULL, NULL,
			zprio, 0, K_FOREVER);
	k_thread_name_set(&bt->thread, pcName);

	if (pxCreatedTask != NULL) {
		*pxCreatedTask = bt;
	}

	k_thread_start(&bt->thread);

	return pdPASS;
}

void vTaskDelete(TaskHandle_t xTaskToDelete)
{
	struct bflb_thread *bt = (struct bflb_thread *)xTaskToDelete;

	if (bt != NULL) {
		k_thread_abort(&bt->thread);
		k_heap_free(&shim_heap, bt->stack);
		k_heap_free(&shim_heap, bt);
	}
}

void vTaskDelay(uint32_t xTicksToDelay)
{
	k_sleep(K_MSEC(xTicksToDelay));
}

void vTaskSwitchContext(void)
{
	k_yield();
}

uint32_t uxTaskPriorityGet(TaskHandle_t xTask)
{
	ARG_UNUSED(xTask);
	return CONFIG_BFLB_BTBLE_TASK_PRIO;
}

QueueHandle_t xQueueGenericCreate(uint32_t uxQueueLength, uint32_t uxItemSize, uint8_t ucQueueType)
{
	struct bflb_queue *q;

	ARG_UNUSED(ucQueueType);

	if (uxItemSize == 0U) {
		/*
		 * FreeRTOS uses zero-sized items for semaphores and mutexes:
		 * xQueueGenericCreate(1, 0, queueQUEUE_TYPE_BINARY_SEMAPHORE).
		 * Map to k_sem — no buffer allocation needed.
		 */
		q = k_heap_alloc(&shim_heap, sizeof(*q), K_NO_WAIT);
		if (q == NULL) {
			return NULL;
		}
		q->is_sem = true;
		k_sem_init(&q->sem, 0, uxQueueLength);
	} else {
		size_t buf_bytes = (size_t)uxQueueLength * uxItemSize;

		q = k_heap_alloc(&shim_heap, sizeof(*q) + buf_bytes, K_NO_WAIT);
		if (q == NULL) {
			return NULL;
		}
		q->is_sem = false;
		k_msgq_init(&q->msgq, q->buf, uxItemSize, uxQueueLength);
	}

	return q;
}

int32_t xQueueGenericSend(QueueHandle_t xQueue, const void *pvItemToQueue, uint32_t xTicksToWait,
			  int32_t xCopyPosition)
{
	struct bflb_queue *q = (struct bflb_queue *)xQueue;

	ARG_UNUSED(xCopyPosition);

	if (q->is_sem) {
		k_sem_give(&q->sem);
		return pdPASS;
	}

	return (k_msgq_put(&q->msgq, pvItemToQueue, ticks_to_timeout(xTicksToWait)) == 0)
		       ? pdPASS
		       : errQUEUE_FULL;
}

int32_t xQueueGenericSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue,
				 int32_t *pxHigherPriorityTaskWoken, int32_t xCopyPosition)
{
	struct bflb_queue *q = (struct bflb_queue *)xQueue;

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

int32_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, uint32_t xTicksToWait)
{
	struct bflb_queue *q = (struct bflb_queue *)xQueue;
	k_timeout_t timeout = ticks_to_timeout(xTicksToWait);

	if (q->is_sem) {
		return (k_sem_take(&q->sem, timeout) == 0) ? pdPASS : pdFAIL;
	}

	return (k_msgq_get(&q->msgq, pvBuffer, timeout) == 0) ? pdPASS : pdFAIL;
}

void vQueueDelete(QueueHandle_t xQueue)
{
	struct bflb_queue *q = (struct bflb_queue *)xQueue;

	if (q != NULL) {
		if (q->is_sem) {
			k_sem_reset(&q->sem);
		} else {
			k_msgq_purge(&q->msgq);
		}
		k_heap_free(&shim_heap, q);
	}
}

void *pvPortMalloc(size_t xWantedSize)
{
	return k_heap_alloc(&shim_heap, xWantedSize, K_NO_WAIT);
}

void vPortFree(void *pv)
{
	k_heap_free(&shim_heap, pv);
}
