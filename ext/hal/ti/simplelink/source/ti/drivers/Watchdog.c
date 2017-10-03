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
/*
 *  ======== Watchdog.c ========
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/Watchdog.h>
#include <ti/drivers/dpl/HwiP.h>

extern const Watchdog_Config Watchdog_config[];
extern const uint_least8_t Watchdog_count;

/* Default Watchdog parameters structure */
const Watchdog_Params Watchdog_defaultParams = {
    NULL,                    /* callbackFxn */
    Watchdog_RESET_ON,       /* resetMode */
    Watchdog_DEBUG_STALL_ON, /* debugStallMode */
    NULL                     /* custom */
};

static bool isInitialized = false;

/*
 *  ======== Watchdog_clear ========
 */
void Watchdog_clear(Watchdog_Handle handle)
{
    handle->fxnTablePtr->watchdogClear(handle);
}

/*
 *  ======== Watchdog_close ========
 */
void Watchdog_close(Watchdog_Handle handle)
{
    handle->fxnTablePtr->watchdogClose(handle);
}

/*
 *  ======== Watchdog_control ========
 */
int_fast16_t Watchdog_control(Watchdog_Handle handle, uint_fast16_t cmd,
                              void *arg)
{
    return (handle->fxnTablePtr->watchdogControl(handle, cmd, arg));
}

/*
 *  ======== Watchdog_init ========
 */
void Watchdog_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < Watchdog_count; i++) {
            Watchdog_config[i].fxnTablePtr->watchdogInit((Watchdog_Handle)&(Watchdog_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== Watchdog_open ========
 */
Watchdog_Handle Watchdog_open(uint_least8_t index, Watchdog_Params *params)
{
    Watchdog_Handle handle = NULL;

    /* Verify driver index and state */
    if (isInitialized && (index < Watchdog_count)) {
        /* If params are NULL use defaults */
        if (params == NULL) {
            params = (Watchdog_Params *) &Watchdog_defaultParams;
        }

        handle = (Watchdog_Handle)&(Watchdog_config[index]);
        handle = handle->fxnTablePtr->watchdogOpen(handle, params);
    }

    return (handle);
}

/*
 *  ======== Watchdog_Params_init ========
 */
void Watchdog_Params_init(Watchdog_Params *params)
{
    *params = Watchdog_defaultParams;
}


/*
 *  ======== Watchdog_setReload ========
 */
int_fast16_t Watchdog_setReload(Watchdog_Handle handle, uint32_t ticks)
{
    return (handle->fxnTablePtr->watchdogSetReload(handle, ticks));
}

/*
 *  ======== Watchdog_convertMsToTicks ========
 */
uint32_t Watchdog_convertMsToTicks(Watchdog_Handle handle, uint32_t milliseconds)
{
    return (handle->fxnTablePtr->watchdogConvertMsToTicks(handle, milliseconds));
}
