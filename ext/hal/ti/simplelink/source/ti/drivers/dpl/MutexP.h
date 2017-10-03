/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** ============================================================================
 *  @file       MutexP.h
 *
 *  @brief      Mutex module for the RTOS Porting Interface
 *
 *  The MutexP module allows task to maintain critical region segments. The
 *  MutexP module has two main functions: ::MutexP_lock and ::MutexP_unlock.
 *
 *  The MutexP module supports recursive calls to the MutexP_lock API by a
 *  single task. The same number of MutexP_unlock calls must be done for the
 *  mutex to be release. Note: the returned key must be provided in the LIFO
 *  order. For example:
 *  @code
 *  uintptr_t key1, key2;
 *  key1 = MutexP_lock();
 *  key2 = MutexP_lock();
 *  MutexP_lock(key2);
 *  MutexP_lock(key1);
 *  @endcode
 *
 *  ============================================================================
 */

#ifndef ti_dpl_MutexP__include
#define ti_dpl_MutexP__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*!
 *  @brief    Number of bytes greater than or equal to the size of any RTOS
 *            MutexP object.
 *
 *  nortos:   12
 *  SysBIOS:  40
 */
#define MutexP_STRUCT_SIZE   (40)

/*!
 *  @brief    MutexP structure.
 *
 *  Opaque structure that should be large enough to hold any of the
 *  RTOS specific MutexP objects.
 */
typedef union MutexP_Struct {
    uint32_t dummy;  /*!< Align object */
    char     data[MutexP_STRUCT_SIZE];
} MutexP_Struct;

/*!
 *  @brief    Status codes for MutexP APIs
 */
typedef enum MutexP_Status {
    /*! API completed successfully */
    MutexP_OK = 0,
    /*! API failed */
    MutexP_FAILURE = -1
} MutexP_Status;

/*!
 *  @brief    Opaque client reference to an instance of a MutexP
 *
 *  A MutexP_Handle returned from the ::MutexP_create represents that instance.
 *  and then is used in the other instance based functions (e.g. ::MutexP_lock,
 *  ::MutexP_unlock, etc.).
 */
typedef void *MutexP_Handle;

/*!
 *  @brief    Basic MutexP Parameters
 *
 *  Structure that contains the parameters are passed into ::MutexP_create
 *  when creating a MutexP instance. The ::MutexP_Params_init function should
 *  be used to initialize the fields to default values before the application
 *  sets the fields manually. The MutexP default parameters are noted in
 *  ::MutexP_Params_init.
 */
typedef struct MutexP_Params {
    void (*callback)(void); /*!< Callback while waiting for mutex unlock */
} MutexP_Params;


/*!
 *  @brief  Function to construct a mutex.
 *
 *  @param  handle Pointer to a MutexP_Struct object
 *
 *  @param  params  Pointer to the instance configuration parameters. NULL
 *                  denotes to use the default parameters (MutexP default
 *                  parameters as noted in ::MutexP_Params_init.
 *
 *  @return A MutexP_Handle on success or a NULL on an error
 */
extern MutexP_Handle MutexP_construct(MutexP_Struct *handle,
        MutexP_Params *params);

/*!
 *  @brief  Function to destruct a mutex object
 *
 *  @param  mutexP  Pointer to a MutexP_Struct object that was passed to
 *                  MutexP_construct().
 *
 *  @return
 */
extern void MutexP_destruct(MutexP_Struct *mutexP);

/*!
 *  @brief  Function to create a mutex.
 *
 *  @param  params  Pointer to the instance configuration parameters. NULL
 *                  denotes to use the default parameters. The MutexP default
 *                  parameters are noted in ::MutexP_Params_init.
 *
 *  @return A MutexP_Handle on success or a NULL on an error
 */
extern MutexP_Handle MutexP_create(MutexP_Params *params);

/*!
 *  @brief  Function to delete a mutex.
 *
 *  @param  handle  A MutexP_Handle returned from MutexP_create
 */
extern void MutexP_delete(MutexP_Handle handle);

/*!
 *  @brief  Initialize params structure to default values.
 *
 *  The default parameters are:
 *      callback - NULL.
 *
 *  @param params  Pointer to the instance configuration parameters.
 */
extern void MutexP_Params_init(MutexP_Params *params);

/*!
 *  @brief  Function to lock a mutex.
 *
 *  This function can only be called from a Task. It cannot be called from
 *  an interrupt. The lock will block until the mutex is available.
 *
 *  Users of a mutex should make every attempt to minimize the duration that
 *  that they have it locked. This is to minimize latency. It is recommended
 *  that the users of the mutex do not block while they have the mutex locked.
 *
 *  This function unlocks the mutex. If the mutex is locked multiple times
 *  by the caller, the same number of unlocks must be called.
 *
 *  @param  handle  A MutexP_Handle returned from ::MutexP_create
 *
 *  @return A key is returned. This key must be passed into ::MutexP_unlock.
 */
extern uintptr_t MutexP_lock(MutexP_Handle handle);

/*!
 *  @brief  Function to unlock a mutex
 *
 *  This function unlocks the mutex. If the mutex is locked multiple times
 *  by the caller, the same number of unlocks must be called. The order of
 *  the keys must be reversed. For example
 *  @code
 *  uintptr_t key1, key2;
 *  key1 = MutexP_lock();
 *  key2 = MutexP_lock();
 *  MutexP_lock(key2);
 *  MutexP_lock(key1);
 *  @endcode
 *
 *  @param  handle  A MutexP_Handle returned from ::MutexP_create
 *
 *  @param  key  Return from ::MutexP_lock.
 */
extern void MutexP_unlock(MutexP_Handle handle, uintptr_t key);

#ifdef __cplusplus
}
#endif

#endif /* ti_dpl_MutexP__include */
