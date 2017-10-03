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
 *  ======== SPI.c ========
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/SPI.h>

extern const SPI_Config SPI_config[];
extern const uint_least8_t SPI_count;

/* Default SPI parameters structure */
const SPI_Params SPI_defaultParams = {
    SPI_MODE_BLOCKING,  /* transferMode */
    SPI_WAIT_FOREVER,   /* transferTimeout */
    NULL,               /* transferCallbackFxn */
    SPI_MASTER,         /* mode */
    1000000,            /* bitRate */
    8,                  /* dataSize */
    SPI_POL0_PHA0,      /* frameFormat */
    NULL                /* custom */
};

static bool isInitialized = false;

/*
 *  ======== SPI_close ========
 */
void SPI_close(SPI_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== SPI_control ========
 */
int_fast16_t SPI_control(SPI_Handle handle, uint_fast16_t cmd, void *controlArg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, controlArg));
}

/*
 *  ======== SPI_init ========
 */
void SPI_init(void)
{
    uint_least8_t i;
    uint_fast8_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < SPI_count; i++) {
            SPI_config[i].fxnTablePtr->initFxn((SPI_Handle)&(SPI_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== SPI_open ========
 */
SPI_Handle SPI_open(uint_least8_t index, SPI_Params *params)
{
    SPI_Handle handle = NULL;

    if (isInitialized && (index < SPI_count)) {
        /* If params are NULL use defaults */
        if (params == NULL) {
            params = (SPI_Params *) &SPI_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (SPI_Handle)&(SPI_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== SPI_Params_init ========
 */
void SPI_Params_init(SPI_Params *params)
{
    *params = SPI_defaultParams;
}

/*
 *  ======== SPI_transfer ========
 */
bool SPI_transfer(SPI_Handle handle, SPI_Transaction *transaction)
{
    return (handle->fxnTablePtr->transferFxn(handle, transaction));
}

/*
 *  ======== SPI_transferCancel ========
 */
void SPI_transferCancel(SPI_Handle handle)
{
    handle->fxnTablePtr->transferCancelFxn(handle);
}
