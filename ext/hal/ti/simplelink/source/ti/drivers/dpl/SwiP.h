/*
 * Copyright (c) 2017, Texas Instruments Incorporated
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
 *  @file       SwiP.h
 *
 *  @brief      Software Interrupt module for the RTOS Porting Interface
 *
 *  ============================================================================
 */

#ifndef ti_dpl_SwiP__include
#define ti_dpl_SwiP__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*!
 *  @brief    Number of bytes greater than or equal to the size of any RTOS
 *            SwiP object.
 *
 *  nortos:   40
 *  SysBIOS:  52
 */
#define SwiP_STRUCT_SIZE   (52)

/*!
 *  @brief    SemaphoreP structure.
 *
 *  Opaque structure that should be large enough to hold any of the
 *  RTOS specific SwiP objects.
 */
typedef union SwiP_Struct {
    uint32_t dummy;  /*!< Align object */
    char     data[SwiP_STRUCT_SIZE];
} SwiP_Struct;

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*!
 *  @brief    Opaque client reference to an instance of a SwiP
 *
 *  A SwiP_Handle returned from the ::SwiP_create represents that instance.
 */
typedef  void *SwiP_Handle;

/*!
 *  @brief    Status codes for SwiP APIs
 *  TODO: See if we need more error codes.
 */
typedef enum SwiP_Status {
    SwiP_OK = 0,
    SwiP_FAILURE = -1
} SwiP_Status;

/*!
 *  @brief  Prototype for the entry function for a hardware interrupt
 */
typedef void (*SwiP_Fxn)(uintptr_t arg0, uintptr_t arg1);

/*!
 *  @brief    Basic SwiP Parameters
 *
 *  Structure that contains the parameters passed into ::SwiP_create
 *  and ::SwiP_construct when creating or constructing a SwiP instance.
 *  The ::SwiP_Params_init function should be used to initialize the
 *  fields to default values before the application sets the fields
 *  manually. The SwiP default parameters are noted in ::SwiP_Params_init.
 *
 *  Each SwiP object has a "trigger" used either to determine whether to
 *  post the SwiP or as a value that can be evaluated within the SwiP's
 *  function.
 *
 *  The SwiP_andn and SwiP_dec functions post the SwiP
 *  if the trigger value transitions to 0. The SwiP_or and
 *  SwiP_inc functions also modify the trigger value. SwiP_or
 *  sets bits, and SwiP_andn clears bits.
 */
typedef struct SwiP_Params {
    uintptr_t  arg0;      /*!< Argument passed into the SwiP function. */
    uintptr_t  arg1;      /*!< Argument passed into the SwiP function. */
    uint32_t   priority;  /*!< priority, 0 is min, 1, 2, ..., ~0 for max */
    uint32_t   trigger;   /*!< Initial SwiP trigger value. */
} SwiP_Params;

/*!
 *  @brief  Function to construct a software interrupt object.
 *
 *  @param  swiP   Pointer to SwiP_Struct object.
 *  @param  swiFxn entry function of the software interrupt
 *
 *  @param  params    Pointer to the instance configuration parameters. NULL
 *                    denotes to use the default parameters. The SwiP default
 *                    parameters are noted in ::SwiP_Params_init.
 *
 *  @return A SwiP_Handle on success or a NULL on an error
 */
extern SwiP_Handle SwiP_construct(SwiP_Struct *swiP, SwiP_Fxn swiFxn,
                               SwiP_Params *params);

/*!
 *  @brief  Function to destruct a software interrupt object
 *
 *  @param  swiP  Pointer to a SwiP_Struct object that was passed to
 *                SwiP_construct().
 *
 *  @return
 */
extern void SwiP_destruct(SwiP_Struct *swiP);

/*!
 *  @brief  Initialize params structure to default values.
 *
 *  The default parameters are:
 *   - name: NULL
 *
 *  @param params  Pointer to the instance configuration parameters.
 */
extern void SwiP_Params_init(SwiP_Params *params);

/*!
 *  @brief  Function to create a software interrupt object.
 *
 *  @param  swiFxn entry function of the software interrupt
 *
 *  @param  params    Pointer to the instance configuration parameters. NULL
 *                    denotes to use the default parameters. The SwiP default
 *                    parameters are noted in ::SwiP_Params_init.
 *
 *  @return A SwiP_Handle on success or a NULL on an error
 */
extern SwiP_Handle SwiP_create(SwiP_Fxn swiFxn,
                               SwiP_Params *params);

/*!
 *  @brief  Function to delete a software interrupt object
 *
 *  @param  handle returned from the SwiP_create call
 *
 */
extern void SwiP_delete(SwiP_Handle handle);

/*!
 *  @brief  Function to disable software interrupts
 *
 *  This function can be called multiple times, but must unwound in the reverse
 *  order. For example
 *  @code
 *  uintptr_t key1, key2;
 *  key1 = SwiP_disable();
 *  key2 = SwiP_disable();
 *  SwiP_restore(key2);
 *  SwiP_restore(key1);
 *  @endcode
 *
 *  @return A key that must be passed to SwiP_restore to re-enable interrupts.
 */
extern uintptr_t SwiP_disable(void);

/*!
 *  @brief  Function to get the trigger value of the currently running SwiP.
 *
 */
extern uint32_t SwiP_getTrigger();

/*!
 *  @brief  Clear bits in SwiP's trigger. Post SwiP if trigger becomes 0.
 *
 *  @param  handle returned from the SwiP_create or SwiP_construct call
 *  @param  mask   inverse value to be ANDed
 */
extern void SwiP_andn(SwiP_Handle handle, uint32_t mask);

/*!
 *  @brief  Decrement SwiP's trigger value. Post SwiP if trigger becomes 0.
 *
 *  @param  handle returned from the SwiP_create or SwiP_construct call
 */
extern void SwiP_dec(SwiP_Handle handle);

/*!
 *  @brief  Increment the SwiP's trigger value and post the SwiP.
 *
 *  @param  handle returned from the SwiP_create or SwiP_construct call
 */
extern void SwiP_inc(SwiP_Handle handle);

/*!
 *  @brief  Function  to return a status based on whether it is in a
 *      software interrupt context.
 *
 *  @return A status: indicating whether the function was called in a
 *      software interrupt routine (true) or not (false).
 */
extern bool SwiP_inISR(void);

/*!
 *  @brief  Or the mask with the SwiP's trigger value and post the SwiP.
 *
 *  @param  handle returned from the SwiP_create or SwiP_construct call
 *  @param  mask   value to be ORed
 */
extern void SwiP_or(SwiP_Handle handle, uint32_t mask);

/*!
 *  @brief  Unconditionally post a software interrupt.
 *
 *  @param  handle returned from the SwiP_create or SwiP_construct call
 */
extern void SwiP_post(SwiP_Handle handle);

/*!
 *  @brief  Function to restore software interrupts
 *
 *  @param  key return from SwiP_disable
 */
extern void SwiP_restore(uintptr_t key);

/*!
 *  @brief  Function to set the priority of a software interrupt
 *
 *  @param  handle returned from the SwiP_create or SwiP_construct call
 *  @param  priority new priority
 */
extern void SwiP_setPriority(SwiP_Handle handle, uint32_t priority);

#ifdef __cplusplus
}
#endif

#endif /* ti_dpl_SwiP__include */
