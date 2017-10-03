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
 *  ======== I2S.c ========
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/I2S.h>

extern const I2S_Config I2S_config[];
extern const uint_least8_t I2S_count;

/* Default I2S parameters structure */
const I2S_Params I2S_defaultParams = {
    I2S_OPMODE_TX_RX_SYNC,        /* I2SMode */
    16000,                        /* Sampling Freq */
    16,                           /* Slot length */
    16,                           /* Bits/Sample */
    2,                            /* Mono/Stero */
    I2S_MODE_ISSUERECLAIM,        /* Read mode */
    NULL,                         /* Read callback */
    I2S_WAIT_FOREVER,             /* Read timeout */
    I2S_MODE_ISSUERECLAIM,        /* Write mode */
    NULL,                         /* Write callback */
    I2S_WAIT_FOREVER,             /* Write timeout  */
    NULL                          /* customParams */
};

static bool isInitialized = false;

/*
 *  ======== I2S_close ========
 */
void I2S_close(I2S_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== I2S_control ========
 */
int_fast16_t I2S_control(I2S_Handle handle, uint_fast16_t cmd, void *arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== I2S_init ========
 */
void I2S_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < I2S_count; i++) {
            I2S_config[i].fxnTablePtr->initFxn((I2S_Handle)&(I2S_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== I2S_open ========
 */
I2S_Handle I2S_open(uint_least8_t index, I2S_Params *params)
{
    I2S_Handle handle = NULL;

    if (isInitialized && (index < I2S_count)) {
        /* If params are NULL use defaults. */
        if (params == NULL) {
            params = (I2S_Params *) &I2S_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (I2S_Handle)&(I2S_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== I2S_Params_init ========
 */
void I2S_Params_init(I2S_Params *params)
{
    *params = I2S_defaultParams;
}

/*
 *  ======== I2S_read ========
 */
int_fast16_t I2S_read(I2S_Handle handle, I2S_BufDesc *desc)
{
    return (handle->fxnTablePtr->readIssueFxn(handle, desc));
}

/*
 *  ======== I2S_readIssue ========
 */
int_fast16_t I2S_readIssue(I2S_Handle handle, I2S_BufDesc *desc)
{
    return (handle->fxnTablePtr->readIssueFxn(handle, desc));
}

/*
 *  ======== I2S_readReclaim ========
 */
size_t I2S_readReclaim(I2S_Handle handle, I2S_BufDesc **pDesc)
{
    return (handle->fxnTablePtr->readReclaimFxn(handle, pDesc));
}

/*
 *  ======== I2S_write ========
 */
int_fast16_t I2S_write(I2S_Handle handle, I2S_BufDesc *desc)
{
    return (handle->fxnTablePtr->writeIssueFxn(handle, desc));
}

/*
 *  ======== I2S_writeIssue ========
 */
int_fast16_t I2S_writeIssue(I2S_Handle handle, I2S_BufDesc *desc)
{
    return (handle->fxnTablePtr->writeIssueFxn(handle, desc));
}

/*
 *  ======== I2S_writeReclaim ========
 */
size_t I2S_writeReclaim(I2S_Handle handle, I2S_BufDesc **pDesc)
{
    return (handle->fxnTablePtr->writeReclaimFxn(handle, pDesc));
}
