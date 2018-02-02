/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2015 Xilinx, Inc.
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
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
 *       rpmsg_zephyr_env.c
 *
 *
 * DESCRIPTION
 *
 *       This file is Zephyr RTOS Implementation of env layer for OpenAMP.
 *
 *
 **************************************************************************/

#include "rpmsg_env.h"
#include <zephyr.h>
#include "rpmsg_platform.h"
#include "virtqueue.h"
#include "rpmsg_compiler.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int env_init_counter = 0;
static struct k_sem env_sema = {0};

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

/*!
 * env_in_isr
 *
 * @returns - true, if currently in ISR
 *
 */
int env_in_isr(void)
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
    k_sched_lock(); /* stop scheduler */
    // verify 'env_init_counter'
    assert(env_init_counter >= 0);
    if (env_init_counter < 0)
    {
        k_sched_unlock(); /* re-enable scheduler */
        return -1;
    }
    env_init_counter++;
    // multiple call of 'env_init' - return ok
    if (env_init_counter <= 1)
    {
        // first call
        k_sem_init(&env_sema, 0, 1);
        memset(isr_table, 0, sizeof(isr_table));
        k_sched_unlock();
        retval = platform_init();
        k_sem_give(&env_sema);

        return retval;
    }
    else
    {
        k_sched_unlock();
        /* Get the semaphore and then return it,
         * this allows for platform_init() to block
         * if needed and other tasks to wait for the
         * blocking to be done.
         * This is in ENV layer as this is ENV specific.*/
        k_sem_take(&env_sema, K_FOREVER);
        k_sem_give(&env_sema);
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

    k_sched_lock(); /* stop scheduler */
    // verify 'env_init_counter'
    assert(env_init_counter > 0);
    if (env_init_counter <= 0)
    {
        k_sched_unlock(); /* re-enable scheduler */
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
        k_sem_reset(&env_sema);
        k_sched_unlock();

        return retval;
    }
    else
    {
        k_sched_unlock();
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
    return (k_malloc(size));
}

/*!
 * env_free_memory - implementation
 *
 * @param ptr
 */
void env_free_memory(void *ptr)
{
    k_free(ptr);
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
 * osalr_mb - implementation
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
    struct k_sem *semaphore_ptr;

    semaphore_ptr = (struct k_sem *)env_allocate_memory(sizeof(struct k_sem));
    if(semaphore_ptr == NULL)
    {
        return -1;
    }
    k_sem_init(semaphore_ptr, count, 10);
    *lock = (void*)semaphore_ptr;
    return 0;
}

/*!
 * env_delete_mutex
 *
 * Deletes the given lock
 *
 */
void env_delete_mutex(void *lock)
{
    k_sem_reset(lock);
    env_free_memory(lock);
}

/*!
 * env_lock_mutex
 *
 * Tries to acquire the lock, if lock is not available then call to
 * this function will suspend.
 */
void env_lock_mutex(void *lock)
{
    if (!env_in_isr())
    {
        k_sem_take((struct k_sem *)lock, K_FOREVER);
    }
}

/*!
 * env_unlock_mutex
 *
 * Releases the given lock.
 */
void env_unlock_mutex(void *lock)
{
    if (!env_in_isr())
    {
        k_sem_give((struct k_sem *)lock);
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
        env_delete_mutex(lock);
}

/*!
 * env_acquire_sync_lock
 *
 * Tries to acquire the lock, if lock is not available then call to
 * this function waits for lock to become available.
 */
void env_acquire_sync_lock(void *lock)
{
    if (lock)
        env_lock_mutex(lock);
}

/*!
 * env_release_sync_lock
 *
 * Releases the given lock.
 */
void env_release_sync_lock(void *lock)
{
    if (lock)
        env_unlock_mutex(lock);
}

/*!
 * env_sleep_msec
 *
 * Suspends the calling thread for given time , in msecs.
 */
void env_sleep_msec(int num_msec)
{
    k_sleep(num_msec);
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
    assert(vector_id < ISR_COUNT);
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
    assert(vector_id < ISR_COUNT);
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
 * @param vector_id   - interrupt vector number
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
 * @param vector_id   - interrupt vector number
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

/*========================================================= */
/* Util data / functions  */

void env_isr(int vector)
{
    struct isr_info *info;
    assert(vector < ISR_COUNT);
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
 * @param item_size - queue element size in bytes
 *
 * @return - status of function execution
 */
int env_create_queue(void **queue, int length, int element_size)
{
    struct k_msgq *queue_ptr = NULL;
    char *msgq_buffer_ptr = NULL;

    queue_ptr = (struct k_msgq *)env_allocate_memory(sizeof(struct k_msgq));
    msgq_buffer_ptr = (char *)env_allocate_memory(length * element_size);
    if((queue_ptr == NULL) || (msgq_buffer_ptr == NULL))
    {
        return -1;
    }
    k_msgq_init(queue_ptr, msgq_buffer_ptr, element_size, length);

    *queue = (void*)queue_ptr;
    return 0;
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
    k_msgq_purge((struct k_msgq*)queue);
    env_free_memory(((struct k_msgq*)queue)->buffer_start);
    env_free_memory(queue);
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
//    if (!env_in_isr())
//    {
        if (0 == k_msgq_put((struct k_msgq*)queue, msg, timeout_ms))
        {
            return 1;
        }
//    }
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
//    if (!env_in_isr())
//    {
        if (0 == k_msgq_get((struct k_msgq*)queue, msg, timeout_ms))
        {
            return 1;
        }
//    }
    return 0;
}
