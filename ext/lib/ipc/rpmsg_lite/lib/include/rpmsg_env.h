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
*       rpmsg_env.h
*
* COMPONENT
*
*         OpenAMP stack.
*
* DESCRIPTION
*
*       This file defines abstraction layer for OpenAMP stack. The implementor
*       must provide definition of all the functions.
*
* DATA STRUCTURES
*
*        none
*
* FUNCTIONS
*
*       env_allocate_memory
*       env_free_memory
*       env_memset
*       env_memcpy
*       env_strncpy
*       env_print
*       env_map_vatopa
*       env_map_patova
*       env_mb
*       env_rmb
*       env_wmb
*       env_create_mutex
*       env_delete_mutex
*       env_lock_mutex
*       env_unlock_mutex
*       env_sleep_msec
*       env_disable_interrupt
*       env_enable_interrupt
*       env_create_queue
*       env_delete_queue
*       env_put_queue
*       env_get_queue
*
**************************************************************************/
#ifndef _RPMSG_ENV_H_
#define _RPMSG_ENV_H_

#include <stdio.h>
#include "rpmsg_default_config.h"
#include "rpmsg_platform.h"

/*!
 * env_init
 *
 * Initializes OS/BM environment.
 *
 * @param env_context        Pointer to preallocated environment context data
 * @param env_init_data      Initialization data for the environment layer
 *
 * @returns - execution status
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
int env_init(void ** env_context, void * env_init_data);
#else
int env_init(void);
#endif

/*!
 * env_deinit
 *
 * Uninitializes OS/BM environment.
 *
 * @param env_context   Pointer to environment context data
 *
 * @returns - execution status
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
int env_deinit(void *env_context);
#else
int env_deinit(void);
#endif

/*!
 * -------------------------------------------------------------------------
 *
 * Dynamic memory management functions. The parameters
 * are similar to standard c functions.
 *
 *-------------------------------------------------------------------------
 **/

/*!
 * env_allocate_memory
 *
 * Allocates memory with the given size.
 *
 * @param size - size of memory to allocate
 *
 * @return - pointer to allocated memory
 */
void *env_allocate_memory(unsigned int size);

/*!
 * env_free_memory
 *
 * Frees memory pointed by the given parameter.
 *
 * @param ptr - pointer to memory to free
 */
void env_free_memory(void *ptr);

/*!
 * -------------------------------------------------------------------------
 *
 * RTL Functions
 *
 *-------------------------------------------------------------------------
 */

void env_memset(void *ptr, int value, unsigned long size);
void env_memcpy(void *dst, void const *src, unsigned long len);
int env_strcmp(const char *dst, const char *src);
void env_strncpy(char *dest, const char *src, unsigned long len);
int env_strncmp(char *dest, const char *src, unsigned long len);
#define env_print(...) printf(__VA_ARGS__)

/*!
 *-----------------------------------------------------------------------------
 *
 *  Functions to convert physical address to virtual address and vice versa.
 *
 *-----------------------------------------------------------------------------
 */

/*!
 * env_map_vatopa
 *
 * Converts logical address to physical address
 *
 * @param env       Pointer to environment context data
 * @param address   Pointer to logical address
 *
 * @return  - physical address
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
unsigned long env_map_vatopa(void *env, void *address);
#else
unsigned long env_map_vatopa(void *address);
#endif

/*!
 * env_map_patova
 *
 * Converts physical address to logical address
 *
 * @param env_context   Pointer to environment context data
 * @param address       Pointer to physical address
 *
 * @return  - logical address
 *
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
void *env_map_patova(void * env, unsigned long address);
#else
void *env_map_patova(unsigned long address);
#endif

/*!
 *-----------------------------------------------------------------------------
 *
 *  Abstractions for memory barrier instructions.
 *
 *-----------------------------------------------------------------------------
 */

/*!
 * env_mb
 *
 * Inserts memory barrier.
 */

void env_mb(void);

/*!
 * env_rmb
 *
 * Inserts read memory barrier
 */

void env_rmb(void);

/*!
 * env_wmb
 *
 * Inserts write memory barrier
 */

void env_wmb(void);

/*!
 *-----------------------------------------------------------------------------
 *
 *  Abstractions for OS lock primitives.
 *
 *-----------------------------------------------------------------------------
 */

/*!
 * env_create_mutex
 *
 * Creates a mutex with given initial count.
 *
 * @param lock -  pointer to created mutex
 * @param count - initial count 0 or 1
 *
 * @return - status of function execution
 */
int env_create_mutex(void **lock, int count);

/*!
 * env_delete_mutex
 *
 * Deletes the given lock.
 *
 * @param lock - mutex to delete
 */

void env_delete_mutex(void *lock);

/*!
 * env_lock_mutex
 *
 * Tries to acquire the lock, if lock is not available then call to
 * this function will suspend.
 *
 * @param lock - mutex to lock
 *
 */

void env_lock_mutex(void *lock);

/*!
 * env_unlock_mutex
 *
 * Releases the given lock.
 *
 * @param lock - mutex to unlock
 */

void env_unlock_mutex(void *lock);

/*!
 * env_create_sync_lock
 *
 * Creates a synchronization lock primitive. It is used
 * when signal has to be sent from the interrupt context to main
 * thread context.
 *
 * @param lock  - pointer to created sync lock object
 * @param state - initial state , lock or unlocked
 *
 * @returns - status of function execution
 */
#define LOCKED 0
#define UNLOCKED 1

int env_create_sync_lock(void **lock, int state);

/*!
 * env_create_sync_lock
 *
 * Deletes given sync lock object.
 *
 * @param lock  - sync lock to delete.
 *
 */

void env_delete_sync_lock(void *lock);

/*!
 * env_acquire_sync_lock
 *
 * Tries to acquire the sync lock.
 *
 * @param lock  - sync lock to acquire.
 */
void env_acquire_sync_lock(void *lock);

/*!
 * env_release_sync_lock
 *
 * Releases synchronization lock.
 *
 * @param lock  - sync lock to release.
 */
void env_release_sync_lock(void *lock);

/*!
 * env_sleep_msec
 *
 * Suspends the calling thread for given time in msecs.
 *
 * @param num_msec -  delay in msecs
 */
void env_sleep_msec(int num_msec);

/*!
 * env_register_isr
 *
 * Registers interrupt handler data for the given interrupt vector.
 *
 * @param env           Pointer to environment context data
 * @param vector_id     Virtual interrupt vector number
 * @param data          Interrupt handler data (virtqueue)
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
void env_register_isr(void *env, int vector_id, void *data);
#else
void env_register_isr(int vector_id, void *data);
#endif

/*!
 * env_unregister_isr
 *
 * Unregisters interrupt handler data for the given interrupt vector.
 *
 * @param env           Pointer to environment context data
 * @param vector_id     Virtual interrupt vector number
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
void env_unregister_isr(void *env, int vector_id);
#else
void env_unregister_isr(int vector_id);
#endif

/*!
 * env_enable_interrupt
 *
 * Enables the given interrupt
 *
 * @param env           Pointer to environment context data
 * @param vector_id     Virtual interrupt vector number
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
void env_enable_interrupt(void *env, unsigned int vector_id);
#else
void env_enable_interrupt(unsigned int vector_id);
#endif

/*!
 * env_disable_interrupt
 *
 * Disables the given interrupt.
 *
 * @param env           Pointer to environment context data
 * @param vector_id     Virtual interrupt vector number
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
void env_disable_interrupt(void *env, unsigned int vector_id);
#else
void env_disable_interrupt(unsigned int vector_id);
#endif

/*!
 * env_map_memory
 *
 * Enables memory mapping for given memory region.
 *
 * @param pa   - physical address of memory
 * @param va   - logical address of memory
 * @param size - memory size
 * param flags - flags for cache/uncached  and access type
 *
 * Currently only first byte of flag parameter is used and bits mapping is defined as follow;
 *
 * Cache bits
 * 0x0000_0001 = No cache
 * 0x0000_0010 = Write back
 * 0x0000_0100 = Write through
 * 0x0000_x000 = Not used
 *
 * Memory types
 *
 * 0x0001_xxxx = Memory Mapped
 * 0x0010_xxxx = IO Mapped
 * 0x0100_xxxx = Shared
 * 0x1000_xxxx = TLB
 */

/* Macros for caching scheme used by the shared memory */
#define UNCACHED (1 << 0)
#define WB_CACHE (1 << 1)
#define WT_CACHE (1 << 2)

/* Memory Types */
#define MEM_MAPPED (1 << 4)
#define IO_MAPPED (1 << 5)
#define SHARED_MEM (1 << 6)
#define TLB_MEM (1 << 7)

void env_map_memory(unsigned int pa, unsigned int va, unsigned int size, unsigned int flags);

/*!
 * env_get_timestamp
 *
 * Returns a 64 bit time stamp.
 *
 *
 */
unsigned long long env_get_timestamp(void);

/*!
 * env_disable_cache
 *
 * Disables system caches.
 *
 */

void env_disable_cache(void);

typedef void LOCK;

/*!
 * env_create_queue
 *
 * Creates a message queue.
 *
 * @param queue      Pointer to created queue
 * @param length     Maximum number of elements in the queue
 * @param item_size  Queue element size in bytes
 *
 * @return - status of function execution
 */
int env_create_queue(void **queue, int length, int element_size);

/*!
 * env_delete_queue
 *
 * Deletes the message queue.
 *
 * @param queue   Queue to delete
 */

void env_delete_queue(void *queue);

/*!
 * env_put_queue
 *
 * Put an element in a queue.
 *
 * @param queue       Queue to put element in
 * @param msg         Pointer to the message to be put into the queue
 * @param timeout_ms  Timeout in ms
 *
 * @return - status of function execution
 */

int env_put_queue(void *queue, void *msg, int timeout_ms);

/*!
 * env_get_queue
 *
 * Get an element out of a queue.
 *
 * @param queue       Queue to get element from
 * @param msg         Pointer to a memory to save the message
 * @param timeout_ms  Timeout in ms
 *
 * @return - status of function execution
 */

int env_get_queue(void *queue, void *msg, int timeout_ms);

/*!
 * env_get_current_queue_size
 *
 * Get current queue size.
 *
 * @param queue    Queue pointer
 *
 * @return - Number of queued items in the queue
 */

int env_get_current_queue_size(void *queue);

/*!
 * env_isr
 *
 * Invoke RPMSG/IRQ callback
 *
 * @param env           Pointer to environment context data
 * @param vector        RPMSG IRQ vector ID.
 */
#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
void env_isr(void *env, int vector);
#else
void env_isr(int vector);
#endif


#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
/*!
 * env_get_platform_context
 *
 * Get the platform layer context from the environment platform context
 *
 * @param env     Pointer to environment context data
 *
 * @return        Pointer to platform context data
 */
void * env_get_platform_context(void * env_context);

/*!
 * env_init_interrupt
 *
 * Initialize the ISR data for given virtqueue interrupt
 *
 * @param env       Pointer to environment context data
 * @param vq_id     Virtqueue ID
 * @param isr_data  Pointer to initial ISR data
 *
 * @return        Execution status, 0 on success
 */
int env_init_interrupt(void *env, int vq_id, void *isr_data);

/*!
 * env_deinit_interrupt
 *
 * Deinitialize the ISR data for given virtqueue interrupt
 *
 * @param env       Pointer to environment context data
 * @param vq_id     Virtqueue ID
 *
 * @return        Execution status, 0 on success
 */
int env_deinit_interrupt(void *env, int vq_id);
#endif

#endif /* _RPMSG_ENV_H_ */
