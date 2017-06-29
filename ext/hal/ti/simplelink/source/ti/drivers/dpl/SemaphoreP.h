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
 *  @file       SemaphoreP.h
 *
 *  @brief      Semaphore module for the RTOS Porting Interface
 *
 *  Semaphores can be counting semaphores or binary semaphores. Counting
 *  semaphores keep track of the number of times the semaphore has been posted
 *  with post functions. This is useful, for example, if you have a group of
 *  resources that are shared between tasks. Such tasks might call pend() to see
 *  if a resource is available before using one. A count of zero for a counting
 *  semaphore denotes that it is not available. A positive count denotes
 *  how many times a SemaphoreP_pend can be called before it is blocked (or
 *  returns SemaphoreP_TIMEOUT).
 *
 *  Binary semaphores can have only two states: available (count = 1) and
 *  unavailable (count = 0). They can be used to share a single resource
 *  between tasks. They can also be used for a basic signalling mechanism, where
 *  the semaphore can be posted multiple times. Binary semaphores do not keep
 *  track of the count; they simply track whether the semaphore has been posted
 *  or not.
 *
 *  ============================================================================
 */

#ifndef ti_dpl_SemaphoreP__include
#define ti_dpl_SemaphoreP__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*!
 *  @brief    Status codes for SemaphoreP APIs
 */
typedef enum SemaphoreP_Status {
    /*! API completed successfully */
    SemaphoreP_OK = 0,
    /*! API failed */
    SemaphoreP_FAILURE = -1,
    /*! API failed because of a timeout */
    SemaphoreP_TIMEOUT = -2
} SemaphoreP_Status;

/*!
 *  @brief    Wait forever define
 */
#define SemaphoreP_WAIT_FOREVER ~(0)

/*!
 *  @brief    No wait define
 */
#define SemaphoreP_NO_WAIT       (0)

/*!
 *  @brief    Number of bytes greater than or equal to the size of any RTOS
 *            SemaphoreP object.
 *
 *  nortos:   20
 *  FreeRTOS: 84
 *  SysBIOS:  28
 */
#define SemaphoreP_STRUCT_SIZE   (84)

/*!
 *  @brief    Opaque client reference to an instance of a SemaphoreP
 *
 *  A SemaphoreP_Handle returned from the ::SemaphoreP_create represents that
 *  instance and  is used in the other instance based functions (e.g.
 *  ::SemaphoreP_post or ::SemaphoreP_pend, etc.).
 */
typedef  void *SemaphoreP_Handle;

/*!
 *  @brief    Mode of the semaphore
 */
typedef enum SemaphoreP_Mode {
    SemaphoreP_Mode_COUNTING = 0x0,
    SemaphoreP_Mode_BINARY   = 0x1
} SemaphoreP_Mode;

/*!
 *  @brief    Basic SemaphoreP Parameters
 *
 *  Structure that contains the parameters are passed into ::SemaphoreP_create
 *  when creating a SemaphoreP instance. The ::SemaphoreP_Params_init function
 *  should be used to initialize the fields to default values before the
 *  application sets the fields manually. The SemaphoreP default parameters are
 *  noted in SemaphoreP_Params_init.
 */
typedef struct SemaphoreP_Params {
    char *name;           /*!< Name of the semaphore instance. Memory must
                               persist for the life of the semaphore instance */
    SemaphoreP_Mode mode; /*!< Mode for the semaphore */
    uint32_t maxCount;    /*!< The max count allowed for counting semaphore */
    void (*callback)(void); /*!< Callback while pending for semaphore post */
} SemaphoreP_Params;

/*!
 *  @brief    SemaphoreP structure.
 *
 *  Opaque structure that should be large enough to hold any of the
 *  RTOS specific SemaphoreP objects.
 */
typedef struct SemaphoreP_Struct {
    union {
        double d;  /*!< Align object */
        char   data[SemaphoreP_STRUCT_SIZE];
    } sem;
} SemaphoreP_Struct;

/*!
 *  @brief  Function to construct a semaphore.
 *
 *  @param  handle Pointer to a SemaphoreP_Struct object
 *
 *  @param  params  Pointer to the instance configuration parameters. NULL
 *                  denotes to use the default parameters (SemaphoreP default
 *                  parameters as noted in ::SemaphoreP_Params_init.
 *
 *  @return A SemaphoreP_Handle on success or a NULL on an error
 */
extern SemaphoreP_Handle SemaphoreP_construct(SemaphoreP_Struct *handle,
        unsigned int count, SemaphoreP_Params *params);

/*!
 *  @brief  Function to create a semaphore.
 *
 *  @param  count  Initial count of the semaphore. For binary semaphores,
 *                 only values of 0 or 1 are valid.
 *
 *  @param  params  Pointer to the instance configuration parameters. NULL
 *                  denotes to use the default parameters (SemaphoreP default
 *                  parameters as noted in ::SemaphoreP_Params_init.
 *
 *  @return A SemaphoreP_Handle on success or a NULL on an error
 */
extern SemaphoreP_Handle SemaphoreP_create(unsigned int count,
                                           SemaphoreP_Params *params);

/*!
 *  @brief  Function to delete a semaphore.
 *
 *  @param  handle  A SemaphoreP_Handle returned from ::SemaphoreP_create
 *
 *  @return Status of the functions
 *    - SemaphoreP_OK: Deleted the semaphore instance
 *    - SemaphoreP_FAILED: Failed to delete the semaphore instance
 */
extern SemaphoreP_Status SemaphoreP_delete(SemaphoreP_Handle handle);

/*!
 *  @brief  Function to destruct a semaphore object
 *
 *  @param  semP  Pointer to a SemaphoreP_Struct object that was passed to
 *                SemaphoreP_construct().
 *
 *  @return
 */
extern void SemaphoreP_destruct(SemaphoreP_Struct *semP);

/*!
 *  @brief  Initialize params structure to default values.
 *
 *  The default parameters are:
 *   - mode: SemaphoreP_Mode_COUNTING
 *   - name: NULL
 *
 *  @param params  Pointer to the instance configuration parameters.
 */
extern void SemaphoreP_Params_init(SemaphoreP_Params *params);

/*!
 *  @brief  Function to pend (wait) on a semaphore.
 *
 *  @param  handle  A SemaphoreP_Handle returned from ::SemaphoreP_create
 *
 *  @param  timeout Timeout (in milliseconds) to wait for the semaphore to
 *                  be posted (signalled).
 *
 *  @return Status of the functions
 *    - SemaphoreP_OK: Obtain the semaphore
 *    - SemaphoreP_TIMEOUT: Timed out. Semaphore was not obtained.
 *    - SemaphoreP_FAILED: Non-time out failure.
 */
extern SemaphoreP_Status SemaphoreP_pend(SemaphoreP_Handle handle,
                                         uint32_t timeout);

/*!
 *  @brief  Function to post (signal) a semaphore from task of ISR context.
 *
 *  @param  handle  A SemaphoreP_Handle returned from ::SemaphoreP_create
 *
 *  @return Status of the functions
 *    - SemaphoreP_OK: Released the semaphore
 *    - SemaphoreP_FAILED: Failed to post the semaphore
 */
extern SemaphoreP_Status SemaphoreP_post(SemaphoreP_Handle handle);

/*!
 *  @brief  Function to post (signal) a semaphore from an ClockP function.
 *
 *  @param  handle  A SemaphoreP_Handle returned from ::SemaphoreP_create
 *
 *  @return Status of the functions
 *    - SemaphoreP_OK: Released the semaphore
 *    - SemaphoreP_FAILED: Failed to post the semaphore
 */
extern SemaphoreP_Status SemaphoreP_postFromClock(SemaphoreP_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* ti_dpl_SemaphoreP__include */
