/*
 * Copyright (c) 2016-2017, Texas Instruments Incorporated
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/Timer.h>

extern const Timer_Config Timer_config[];
extern const uint_least8_t Timer_count;

/* Default Parameters */
static const Timer_Params defaultParams = {
    .timerMode     = Timer_ONESHOT_BLOCKING,
    .periodUnits   = Timer_PERIOD_COUNTS,
    .timerCallback = NULL,
    .period        = (uint16_t) ~0
};

static bool isInitialized = false;

/*
 *  ======== Timer_control ========
 */
int_fast16_t Timer_control(Timer_Handle handle, uint_fast16_t cmd, void *arg)
{
    return handle->fxnTablePtr->controlFxn(handle, cmd, arg);
}

/*
 *  ======== Timer_close ========
 */
void Timer_close(Timer_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== Timer_getCount ========
 */
uint32_t Timer_getCount(Timer_Handle handle)
{
    return handle->fxnTablePtr->getCountFxn(handle);
}

/*
 *  ======== Timer_init ========
 */
void Timer_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < Timer_count; i++) {
            Timer_config[i].fxnTablePtr->initFxn((Timer_Handle) &(Timer_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== Timer_open ========
 */
Timer_Handle Timer_open(uint_least8_t index, Timer_Params *params)
{
    Timer_Handle handle = NULL;

    /* Verify driver index and state */
    if (isInitialized && (index < Timer_count)) {
        /* If parameters are NULL use defaults */
        if (params == NULL) {
            params = (Timer_Params *) &defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (Timer_Handle) &(Timer_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== Timer_Params_init ========
 */
void Timer_Params_init(Timer_Params *params)
{
    *params = defaultParams;
}

/*
 *  ======== Timer_start ========
 */
int32_t Timer_start(Timer_Handle handle)
{
    return handle->fxnTablePtr->startFxn(handle);
}

/*
 *  ======== Timer_stop ========
 */
void Timer_stop(Timer_Handle handle)
{
    handle->fxnTablePtr->stopFxn(handle);
}
