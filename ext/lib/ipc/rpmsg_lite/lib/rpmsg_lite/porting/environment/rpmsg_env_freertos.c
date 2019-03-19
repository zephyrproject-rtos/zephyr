/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2015 Xilinx, Inc.
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************
 * FILE NAME
 *
 *       freertos_env.c
 *
 *
 * DESCRIPTION
 *
 *       This file is FreeRTOS Implementation of env layer for OpenAMP.
 *
 *
 **************************************************************************/

#include "rpmsg_env.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "rpmsg_platform.h"
#include "virtqueue.h"
#include "rpmsg_compiler.h"

#include <stdlib.h>
#include <string.h>

static int env_init_counter = 0;
SemaphoreHandle_t env_sema = NULL;

/* RL_ENV_MAX_MUTEX_COUNT is an arbitrary count greater than 'count'
   if the inital count is 1, this function behaves as a mutex
   if it is greater than 1, it acts as a "resource allocator" with 
   the maximum of 'count' resources available. 
   Currently, only the first use-case is applicable/applied in RPMsg-Lite.
 */
#define RL_ENV_MAX_MUTEX_COUNT (10)

/* Max supported ISR counts */
#define ISR_COUNT (32)
/*!
 * Structure to keep track of registered ISR's.
 */
struct isr_info
{
    void *data;
};
static struct isr_info isr_table[ISR_COUNT];

#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
#error "This RPMsg-Lite port requires RL_USE_ENVIRONMENT_CONTEXT set to 0"
#endif

/*!
 * env_in_isr
 *
 * @returns - true, if currently in ISR
 *
 */
static int env_in_isr(void)
{
    return platform_in_isr();
}

/*!
 * env_init
 *
 * Initializes OS/BM environment.
 *
 */
int env_init(void)
{
    int retval;
    vTaskSuspendAll(); /* stop scheduler */
    // verify 'env_init_counter'
    RL_ASSERT(env_init_counter >= 0);
    if (env_init_counter < 0)
    {
        xTaskResumeAll(); /* re-enable scheduler */
        return -1;
    }
    env_init_counter++;
    // multiple call of 'env_init' - return ok
    if (env_init_counter <= 1)
    {
        // first call
        env_sema = xSemaphoreCreateBinary();
        memset(isr_table, 0, sizeof(isr_table));
        xTaskResumeAll();
        retval = platform_init();
        xSemaphoreGive(env_sema);

        return retval;
    }
    else
    {
        xTaskResumeAll();
        /* Get the semaphore and then return it,
         * this allows for platform_init() to block
         * if needed and other tasks to wait for the
         * blocking to be done.
         * This is in ENV layer as this is ENV specific.*/
        (void)xSemaphoreTake(env_sema, portMAX_DELAY);
        xSemaphoreGive(env_sema);
        return 0;
    }
}

/*!
 * env_deinit
 *
 * Uninitializes OS/BM environment.
 *
 * @returns - execution status
 */
int env_deinit(void)
{
    int retval;

    vTaskSuspendAll(); /* stop scheduler */
    // verify 'env_init_counter'
    RL_ASSERT(env_init_counter > 0);
    if (env_init_counter <= 0)
    {
        xTaskResumeAll(); /* re-enable scheduler */
        return -1;
    }

    // counter on zero - call platform deinit
    env_init_counter--;
    // multiple call of 'env_deinit' - return ok
    if (env_init_counter <= 0)
    {
        // last call
        memset(isr_table, 0, sizeof(isr_table));
        retval = platform_deinit();
        vSemaphoreDelete(env_sema);
        env_sema = NULL;
        xTaskResumeAll();

        return retval;
    }
    else
    {
        xTaskResumeAll();
        return 0;
    }
}

/*!
 * env_allocate_memory - implementation
 *
 * @param size
 */
void *env_allocate_memory(unsigned int size)
{
    return (pvPortMalloc(size));
}

/*!
 * env_free_memory - implementation
 *
 * @param ptr
 */
void env_free_memory(void *ptr)
{
    if (ptr != NULL)
    {
        vPortFree(ptr);
    }
}

/*!
 *
 * env_memset - implementation
 *
 * @param ptr
 * @param value
 * @param size
 */
void env_memset(void *ptr, int value, unsigned long size)
{
    memset(ptr, value, size);
}

/*!
 *
 * env_memcpy - implementation
 *
 * @param dst
 * @param src
 * @param len
 */
void env_memcpy(void *dst, void const *src, unsigned long len)
{
    memcpy(dst, src, len);
}

/*!
 *
 * env_strcmp - implementation
 *
 * @param dst
 * @param src
 */

int env_strcmp(const char *dst, const char *src)
{
    return (strcmp(dst, src));
}

/*!
 *
 * env_strncpy - implementation
 *
 * @param dest
 * @param src
 * @param len
 */
void env_strncpy(char *dest, const char *src, unsigned long len)
{
    strncpy(dest, src, len);
}

/*!
 *
 * env_strncmp - implementation
 *
 * @param dest
 * @param src
 * @param len
 */
int env_strncmp(char *dest, const char *src, unsigned long len)
{
    return (strncmp(dest, src, len));
}

/*!
 *
 * env_mb - implementation
 *
 */
void env_mb(void)
{
    MEM_BARRIER();
}

/*!
 * env_rmb - implementation
 */
void env_rmb(void)
{
    MEM_BARRIER();
}

/*!
 * env_wmb - implementation
 */
void env_wmb(void)
{
    MEM_BARRIER();
}

/*!
 * env_map_vatopa - implementation
 *
 * @param address
 */
unsigned long env_map_vatopa(void *address)
{
    return platform_vatopa(address);
}

/*!
 * env_map_patova - implementation
 *
 * @param address
 */
void *env_map_patova(unsigned long address)
{
    return platform_patova(address);
}

/*!
 * env_create_mutex
 *
 * Creates a mutex with the given initial count.
 *
 */
int env_create_mutex(void **lock, int count)
{
    if(count > RL_ENV_MAX_MUTEX_COUNT)
    {
        return -1;   
    }
    
    *lock = xSemaphoreCreateCounting(RL_ENV_MAX_MUTEX_COUNT, count);
    if (*lock)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*!
 * env_delete_mutex
 *
 * Deletes the given lock
 *
 */
void env_delete_mutex(void *lock)
{
    vSemaphoreDelete(lock);
}

/*!
 * env_lock_mutex
 *
 * Tries to acquire the lock, if lock is not available then call to
 * this function will suspend.
 */
void env_lock_mutex(void *lock)
{
    SemaphoreHandle_t xSemaphore = (SemaphoreHandle_t)lock;
    if (env_in_isr() == 0)
    {
        (void)xSemaphoreTake(xSemaphore, portMAX_DELAY);
    }
}

/*!
 * env_unlock_mutex
 *
 * Releases the given lock.
 */
void env_unlock_mutex(void *lock)
{
    SemaphoreHandle_t xSemaphore = (SemaphoreHandle_t)lock;
    if (env_in_isr() == 0)
    {
        xSemaphoreGive(xSemaphore);
    }
}

/*!
 * env_create_sync_lock
 *
 * Creates a synchronization lock primitive. It is used
 * when signal has to be sent from the interrupt context to main
 * thread context.
 */
int env_create_sync_lock(void **lock, int state)
{
    return env_create_mutex(lock, state); /* state=1 .. initially free */
}

/*!
 * env_delete_sync_lock
 *
 * Deletes the given lock
 *
 */
void env_delete_sync_lock(void *lock)
{
    if (lock)
    {
        env_delete_mutex(lock);
    }
}

/*!
 * env_acquire_sync_lock
 *
 * Tries to acquire the lock, if lock is not available then call to
 * this function waits for lock to become available.
 */
void env_acquire_sync_lock(void *lock)
{
    BaseType_t xTaskWokenByReceive = pdFALSE;
    SemaphoreHandle_t xSemaphore = (SemaphoreHandle_t)lock;
    if (env_in_isr())
    {
        xSemaphoreTakeFromISR(xSemaphore, &xTaskWokenByReceive);
        portEND_SWITCHING_ISR(xTaskWokenByReceive);
    }
    else
    {
        (void)xSemaphoreTake(xSemaphore, portMAX_DELAY);
    }
}

/*!
 * env_release_sync_lock
 *
 * Releases the given lock.
 */
void env_release_sync_lock(void *lock)
{
    BaseType_t xTaskWokenByReceive = pdFALSE;
    SemaphoreHandle_t xSemaphore = (SemaphoreHandle_t)lock;
    if (env_in_isr())
    {
        xSemaphoreGiveFromISR(xSemaphore, &xTaskWokenByReceive);
        portEND_SWITCHING_ISR(xTaskWokenByReceive);
    }
    else
    {
        xSemaphoreGive(xSemaphore);
    }
}

/*!
 * env_sleep_msec
 *
 * Suspends the calling thread for given time , in msecs.
 */
void env_sleep_msec(int num_msec)
{
    vTaskDelay(num_msec / portTICK_PERIOD_MS);
}

/*!
 * env_register_isr
 *
 * Registers interrupt handler data for the given interrupt vector.
 *
 * @param vector_id - virtual interrupt vector number
 * @param data      - interrupt handler data (virtqueue)
 */
void env_register_isr(int vector_id, void *data)
{
    RL_ASSERT(vector_id < ISR_COUNT);
    if (vector_id < ISR_COUNT)
    {
        isr_table[vector_id].data = data;
    }
}

/*!
 * env_unregister_isr
 *
 * Unregisters interrupt handler data for the given interrupt vector.
 *
 * @param vector_id - virtual interrupt vector number
 */
void env_unregister_isr(int vector_id)
{
    RL_ASSERT(vector_id < ISR_COUNT);
    if (vector_id < ISR_COUNT)
    {
        isr_table[vector_id].data = NULL;
    }
}

/*!
 * env_enable_interrupt
 *
 * Enables the given interrupt
 *
 * @param vector_id   - virtual interrupt vector number
 */

void env_enable_interrupt(unsigned int vector_id)
{
    platform_interrupt_enable(vector_id);
}

/*!
 * env_disable_interrupt
 *
 * Disables the given interrupt
 *
 * @param vector_id   - virtual interrupt vector number
 */

void env_disable_interrupt(unsigned int vector_id)
{
    platform_interrupt_disable(vector_id);
}

/*!
 * env_map_memory
 *
 * Enables memory mapping for given memory region.
 *
 * @param pa   - physical address of memory
 * @param va   - logical address of memory
 * @param size - memory size
 * param flags - flags for cache/uncached  and access type
 */

void env_map_memory(unsigned int pa, unsigned int va, unsigned int size, unsigned int flags)
{
    platform_map_mem_region(va, pa, size, flags);
}

/*!
 * env_disable_cache
 *
 * Disables system caches.
 *
 */

void env_disable_cache(void)
{
    platform_cache_all_flush_invalidate();
    platform_cache_disable();
}

/*!
 *
 * env_get_timestamp
 *
 * Returns a 64 bit time stamp.
 *
 *
 */
unsigned long long env_get_timestamp(void)
{
    if (env_in_isr())
    {
        return (unsigned long long)xTaskGetTickCountFromISR();
    }
    else
    {
        return (unsigned long long)xTaskGetTickCount();
    }
}

/*========================================================= */
/* Util data / functions  */

void env_isr(int vector)
{
    struct isr_info *info;
    RL_ASSERT(vector < ISR_COUNT);
    if (vector < ISR_COUNT)
    {
        info = &isr_table[vector];
        virtqueue_notification((struct virtqueue *)info->data);
    }
}

/*
 * env_create_queue
 *
 * Creates a message queue.
 *
 * @param queue -  pointer to created queue
 * @param length -  maximum number of elements in the queue
 * @param element_size - queue element size in bytes
 *
 * @return - status of function execution
 */
int env_create_queue(void **queue, int length, int element_size)
{
    *queue = xQueueCreate(length, element_size);
    if (*queue)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*!
 * env_delete_queue
 *
 * Deletes the message queue.
 *
 * @param queue - queue to delete
 */

void env_delete_queue(void *queue)
{
    vQueueDelete(queue);
}

/*!
 * env_put_queue
 *
 * Put an element in a queue.
 *
 * @param queue - queue to put element in
 * @param msg - pointer to the message to be put into the queue
 * @param timeout_ms - timeout in ms
 *
 * @return - status of function execution
 */

int env_put_queue(void *queue, void *msg, int timeout_ms)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (env_in_isr())
    {
        if (xQueueSendFromISR(queue, msg, &xHigherPriorityTaskWoken) == pdPASS)
        {
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            return 1;
        }
    }
    else
    {
        if (xQueueSend(queue, msg, ((portMAX_DELAY == timeout_ms) ? portMAX_DELAY : timeout_ms / portTICK_PERIOD_MS)) ==
            pdPASS)
        {
            return 1;
        }
    }
    return 0;
}

/*!
 * env_get_queue
 *
 * Get an element out of a queue.
 *
 * @param queue - queue to get element from
 * @param msg - pointer to a memory to save the message
 * @param timeout_ms - timeout in ms
 *
 * @return - status of function execution
 */

int env_get_queue(void *queue, void *msg, int timeout_ms)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (env_in_isr())
    {
        if (xQueueReceiveFromISR(queue, msg, &xHigherPriorityTaskWoken) == pdPASS)
        {
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            return 1;
        }
    }
    else
    {
        if (xQueueReceive(queue, msg,
                          ((portMAX_DELAY == timeout_ms) ? portMAX_DELAY : timeout_ms / portTICK_PERIOD_MS)) == pdPASS)
        {
            return 1;
        }
    }
    return 0;
}

/*!
 * env_get_current_queue_size
 *
 * Get current queue size.
 *
 * @param queue - queue pointer
 *
 * @return - Number of queued items in the queue
 */

int env_get_current_queue_size(void *queue)
{
    if (env_in_isr())
    {
        return(uxQueueMessagesWaitingFromISR(queue));
    }
    else
    {
        return(uxQueueMessagesWaiting(queue));
    }
}
