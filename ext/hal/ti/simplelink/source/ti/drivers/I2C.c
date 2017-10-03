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
 *  ======== I2C.c ========
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/I2C.h>

extern const I2C_Config I2C_config[];
extern const uint_least8_t I2C_count;

/* Default I2C parameters structure */
const I2C_Params I2C_defaultParams = {
    I2C_MODE_BLOCKING,  /* transferMode */
    NULL,               /* transferCallbackFxn */
    I2C_100kHz,         /* bitRate */
    NULL                /* custom */
};

static bool isInitialized = false;

/*
 *  ======== I2C_cancel ========
 */
void I2C_cancel(I2C_Handle handle)
{
    handle->fxnTablePtr->cancelFxn(handle);
}

/*
 *  ======== I2C_close ========
 */
void I2C_close(I2C_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== I2C_control ========
 */
int_fast16_t I2C_control(I2C_Handle handle, uint_fast16_t cmd, void *controlArg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, controlArg));
}

/*
 *  ======== I2C_init ========
 */
void I2C_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < I2C_count; i++) {
            I2C_config[i].fxnTablePtr->initFxn((I2C_Handle)&(I2C_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== I2C_open ========
 */
I2C_Handle I2C_open(uint_least8_t index, I2C_Params *params)
{
    I2C_Handle handle = NULL;

    if (isInitialized && (index < I2C_count)) {
        /* If params are NULL use defaults. */
        if (params == NULL) {
            params = (I2C_Params *) &I2C_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (I2C_Handle)&(I2C_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== I2C_Params_init =======
 */
void I2C_Params_init(I2C_Params *params)
{
    *params = I2C_defaultParams;
}

/*
 *  ======== I2C_transfer ========
 */
bool I2C_transfer(I2C_Handle handle, I2C_Transaction *transaction)
{
    return (handle->fxnTablePtr->transferFxn(handle, transaction));
}
