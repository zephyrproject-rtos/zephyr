/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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
 *  ======== SDSPI.c ========
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/SDSPI.h>
#include <ti/drivers/dpl/HwiP.h>

extern const SDSPI_Config SDSPI_config[];
extern const uint8_t SDSPI_count;

/* Default SDSPI parameters structure */
const SDSPI_Params SDSPI_defaultParams = {
    2500000,         /* bitRate */
    NULL             /* custom */
};

static bool isInitialized = false;

/*
 *  ======== SDSPI_close ========
 */
void SDSPI_close(SDSPI_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== SDSPI_control ========
 */
int_fast16_t SDSPI_control(SDSPI_Handle handle, uint_fast16_t cmd, void *arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== SDSPI_init ========
 */
void SDSPI_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < SDSPI_count; i++) {
            SDSPI_config[i].fxnTablePtr->initFxn((SDSPI_Handle)&(SDSPI_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== SDSPI_open ========
 */
SDSPI_Handle SDSPI_open(uint_least8_t index, uint_least8_t drv, SDSPI_Params *params)
{
    SDSPI_Handle handle = NULL;

    if (isInitialized && (index < SDSPI_count)) {
        /* If params are NULL use defaults */
        if (params == NULL) {
            params = (SDSPI_Params *) &SDSPI_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (SDSPI_Handle)&(SDSPI_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, drv, params);
    }

    return (handle);
}

/*
 *  ======== SDSPI_Params_init ========
 *
 *  Defaults values are:
 *  @code
 *  bitRate             = 12500000 (Hz) //CC32XX
 *  bitRate             = 2500000  (Hz) //MSP432
 *  @endcode
 *
 *  @param  params  Parameter structure to initialize
 */
void SDSPI_Params_init(SDSPI_Params *params)
{
    *params = SDSPI_defaultParams;
}
