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
 *  ======== I2CSlave.c ========
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/I2CSlave.h>

extern const I2CSlave_Config I2CSlave_config[];
extern const uint_least8_t I2CSlave_count;

/* Default I2CSlave parameters structure */
const I2CSlave_Params I2CSlave_defaultParams = {
    I2CSLAVE_MODE_BLOCKING,  /* transferMode */
    NULL,                    /* transferCallbackFxn */
    NULL                     /* custom */
};

static bool isInitialized = false;

/*
 *  ======== I2CSlave_close ========
 */
void I2CSlave_close(I2CSlave_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== I2CSlave_control ========
 */
int_fast16_t I2CSlave_control(I2CSlave_Handle handle, uint_fast16_t cmd, void *arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== I2CSlave_init ========
 */
void I2CSlave_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < I2CSlave_count; i++) {
            I2CSlave_config[i].fxnTablePtr->initFxn((I2CSlave_Handle)&(I2CSlave_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== I2CSlave_open ========
 */
I2CSlave_Handle I2CSlave_open(uint_least8_t index, I2CSlave_Params *params)
{
    I2CSlave_Handle handle = NULL;

    /* Verify driver index and state */
    if (isInitialized && (index < I2CSlave_count)) {
        /* Use defaults if params are NULL. */
        if (params == NULL) {
            params = (I2CSlave_Params *) &I2CSlave_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (I2CSlave_Handle)&(I2CSlave_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== I2CSlave_Params_init =======
 */
void I2CSlave_Params_init(I2CSlave_Params *params)
{
    *params = I2CSlave_defaultParams;
}

/*
 *  ======== I2CSlave_read ========
 */
bool I2CSlave_read(I2CSlave_Handle handle, void *buffer, size_t size)
{
    return (handle->fxnTablePtr->readFxn(handle, buffer, size));
}

/*
 *  ======== I2CSlave_write ========
 */
bool I2CSlave_write(I2CSlave_Handle handle, const void *buffer, size_t size)
{
    return (handle->fxnTablePtr->writeFxn(handle, buffer, size));
}
