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

#include <ti/drivers/Capture.h>
#include <ti/drivers/dpl/HwiP.h>

extern const Capture_Config Capture_config[];
extern const uint_least8_t Capture_count;

/* Default Parameters */
static const Capture_Params defaultParams = {
    .callbackFxn = NULL,
    .mode        = Capture_RISING_EDGE,
    .periodUnit  = Capture_PERIOD_COUNTS
};

static bool isInitialized = false;

/*
 *  ======== Capture_close ========
 */
void Capture_close(Capture_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== Capture_control ========
 */
int_fast16_t Capture_control(Capture_Handle handle, uint_fast16_t cmd,
    void *arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== Capture_init ========
 */
void Capture_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < Capture_count; i++) {
            Capture_config[i].fxnTablePtr->initFxn((Capture_Handle) &(Capture_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== Capture_open ========
 */
Capture_Handle Capture_open(uint_least8_t index, Capture_Params *params)
{
    Capture_Handle handle = NULL;

    /* Verify driver index and state */
    if (isInitialized && (index < Capture_count)) {
        /* If parameters are NULL use defaults */
        if (params == NULL) {
            params = (Capture_Params *) &defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (Capture_Handle) &(Capture_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== Capture_Params_init ========
 */
void Capture_Params_init(Capture_Params *params)
{
    *params = defaultParams;
}

/*
 *  ======== Capture_start ========
 */
int32_t Capture_start(Capture_Handle handle)
{
    return (handle->fxnTablePtr->startFxn(handle));
}

/*
 *  ======== Capture_stop ========
 */
void Capture_stop(Capture_Handle handle)
{
    handle->fxnTablePtr->stopFxn(handle);
}
