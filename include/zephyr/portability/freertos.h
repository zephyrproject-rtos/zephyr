/*
 * SPDX-File-CopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup freertos
 * @brief Main header for FreeRTOS compatibility layer
 */

/**
 * @brief FreeRTOS compatibility layer.
 * @defgroup freertos FreeRTOS portability layer
 * @since 4.4
 * @version 0.0.1
 * @ingroup os_services
 *
 * @{
 */

/** FreeRTOS return code for success */
#define pdPASS 1

/** FreeRTOS return code for failure */
#define pdFAIL 0

/** FreeRTOS-specific implementation of C false */
#define pdFALSE 0

/** FreeRTOS error code for queue being full */
#define errQUEUE_FULL 0

/** FreeRTOS error code for memory allocation failure */
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY (-1)

/** FreeRTOS maximum wait time translated to @ref K_FOREVER */
#define portMAX_DELAY 0xFFFFFFFFU

/** FreeRTOS definition of the duration between ticks in milliseconds */
#define portTICK_PERIOD_MS 1

/** FreeRTOS selected size size type, affecting stack allocation sizes */
typedef uint32_t portSTACK_TYPE;

/** Opaque pointer to a taks handle created by @ref xTaskCreate */
typedef void *TaskHandle_t;

/** Opaque pointer to a queue handle created by @ref xQueueGenericCreate */
typedef void *QueueHandle_t;

/** Function pointer type called by @ref xTaskCreate then @ref k_thread_create */
typedef void (*TaskFunction_t)(void *);

/** Thread context with extra fields used by FreeRTOS, used internally */
struct freertos_task {
	/** Zephyr thread providing the FreeRTOS task features */
	struct k_thread thread;
	/** Stack memory for this thread */
	k_thread_stack_t *stack;
	/** Stack size for this thread */
	size_t stack_size;
	/** Function running this thread */
	TaskFunction_t func;
	/** Argument for this thread function  */
	void *arg;
};

/** Queue context with either a semaphore or message queue, used internally */
struct freertos_queue {
	/** Indicates if using a semaphore or message queue as back-end */
	bool is_sem;
	/** Storage for the back-end of the queue depending on type used */
	union {
		/** When used, the semaphore used as back-end */
		struct k_sem sem;
		/** When used, the message queue used as back-end */
		struct k_msgq msgq;
	};
	/** When used, memory used as storage for the message queue. */
	char buf[];
};

/**
 * @brief Create a Zephyr thread
 *
 * The FreeRTOS-specific variables are described and stored by @ref freertos_task.
 *
 * @param pxTaskCode Pointer to the task entry function to execute
 * @param pcName Name of the task, setting the zephyr thread name.
 * @param usStackDepth Size in words of sizeof(portSTACK_TYPE) bytes of the stack.
 * @param pvParameters Pointer to a custom struct passed as parameter to @p pxTaskCode.
 * @param uxPriority Task priority, mapped to Zephyr priorities.
 * @param pxCreatedTask Reference used to identify the task, used by other functions.
 * @retval pdPASS on success
 * @retval errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY
 */
int32_t xTaskCreate(TaskFunction_t pxTaskCode, const char *pcName, uint16_t usStackDepth,
		    void *pvParameters, uint32_t uxPriority, TaskHandle_t *pxCreatedTask);

/**
 * @brief Halt a Zephyr thread created by @ref xTaskCreate
 *
 * @param xTask Task reference created by @ref xTaskCreate.
 */
void vTaskDelete(TaskHandle_t xTask);

/**
 * @brief Calls @ref k_sleep
 *
 * @param xTicksToWait Time to sleep, as described by @ref portTICK_PERIOD_MS
 */
void vTaskDelay(uint32_t xTicksToWait);

/**
 * @brief Calls @ref k_yield
 */
void vTaskSwitchContext(void);

/**
 * @brief Create a Zephyr message queue or semaphore to communicate between threads.
 *
 * If the queue is having an item size of zero, then a semaphore is used,
 * otherwise a message queue is used.
 *
 * @param uxQueueLength Length of the queue
 * @param uxItemSize Size of each individual item.
 * @param ucQueueType Currently unhandled
 * @return a newly allocated queue handle usable by other queue functions
 * @retval NULL on failure
 */
QueueHandle_t xQueueGenericCreate(uint32_t uxQueueLength, uint32_t uxItemSize, uint8_t ucQueueType);

/**
 * @brief Calls @ref k_sem_give or @ref k_msgq_put to communicate with another queue peer.
 *
 * If the queue is having an item size of zero, then a semaphore is used,
 * otherwise a message queue is used.
 *
 * @param xQueue Queue handle created by @ref xQueueGenericCreate
 * @param pvItemToQueue Pointer to the data to queue.
 * @param xTicksToWait Time to sleep, as described by @ref portTICK_PERIOD_MS
 * @param xCopyPosition Currently unsupported
 * @retval pdPASS on success
 * @retval errQUEUE_FULL if using message queues and it is full
 */
int32_t xQueueGenericSend(QueueHandle_t xQueue, const void *pvItemToQueue, uint32_t xTicksToWait,
			  int32_t xCopyPosition);

/**
 * @brief Calls @ref k_sem_give or @ref k_msgq_put to communicate with another queue peer.
 *
 * If the queue is having an item size of zero, then a semaphore is used,
 * otherwise a message queue is used.
 *
 * @param xQueue Queue handle created by @ref xQueueGenericCreate
 * @param pvItemToQueue Pointer to the data to queue.
 * @param pxHigherPriorityTaskWoken Currently unsupported.
 * @param xCopyPosition Currently unsupported
 * @retval pdPASS on success
 * @retval errQUEUE_FULL if using message queues and it is full
 */
int32_t xQueueGenericSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue,
				 int32_t *pxHigherPriorityTaskWoken, int32_t xCopyPosition);

/**
 * @brief Calls @ref k_sem_take or @ref k_msgq_get to communicate with another queue peer.
 *
 * If the queue is having an item size of zero, then a semaphore is used,
 * otherwise a message queue is used.
 *
 * @param xQueue Queue handle created by @ref xQueueGenericCreate
 * @param pvBuffer Pointer filled with queue data if a semaphore is used.
 * @param xTicksToWait Time to sleep, as described by @ref portTICK_PERIOD_MS
 * @retval pdPASS on success
 * @retval pdFAIL on failure
 */
int32_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, uint32_t xTicksToWait);

/**
 * @brief Calls @ref k_sem_reset or @ref k_msgq_purge and releases associated resources
 *
 * @param xQueue Queue handle created by @ref xQueueGenericCreate
 */
void vQueueDelete(QueueHandle_t xQueue);

/**
 * @brief Calls @ref k_heap_alloc with @ref K_NO_WAIT on a locally defined memory heap
 *
 * See @c CONFIG_FREERTOS_HEAP_SIZE for the memory heap size.
 *
 * @param xWantedSize Size to allocate in bytes
 * @return allocated memory
 * @retval NULL if out of memory
 */
void *pvPortMalloc(size_t xWantedSize);

/**
 * @brief Calls @ref k_heap_free
 *
 * @param pv pointer allocated by @ref pvPortMalloc
 */
void vPortFree(void *pv);

/** @} */
