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
#include <ti/drivers/SD.h>

extern const SD_Config SD_config[];
extern const uint_least8_t SD_count;

/* Default SD parameters structure */
const SD_Params SD_defaultParams = {
    NULL /* custom */
};

static bool isInitialized = false;

/*
 *  ======== SD_close ========
 */
void SD_close(SD_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== SD_control ========
 */
int_fast16_t SD_control(SD_Handle handle, uint_fast16_t cmd, void *arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== SD_getNumSectors ========
 */
uint_fast32_t SD_getNumSectors(SD_Handle handle)
{
    return (handle->fxnTablePtr->getNumSectorsFxn(handle));
}

/*
 *  ======== SD_getSectorSize ========
 */
uint_fast32_t SD_getSectorSize(SD_Handle handle)
{
    return (handle->fxnTablePtr->getSectorSizeFxn(handle));
}

/*
 *  ======== SD_init ========
 */
void SD_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < SD_count; i++) {
            SD_config[i].fxnTablePtr->initFxn((SD_Handle)&(SD_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== SD_initialize ========
 */
int_fast16_t SD_initialize(SD_Handle handle)
{
    return (handle->fxnTablePtr->initializeFxn(handle));
}

/*
 *  ======== SD_open ========
 */
SD_Handle SD_open(uint_least8_t index, SD_Params *params)
{
    SD_Handle handle = NULL;

    /* Verify driver index and state */
    if (isInitialized && (index < SD_count)) {
        /* If params are NULL use defaults */
        if (params == NULL) {
            params = (SD_Params *) &SD_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (SD_Handle)&(SD_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== SD_Params_init ========
 */
void SD_Params_init(SD_Params *params)
{
    *params = SD_defaultParams;
}

/*
 *  ======== SD_read ========
 */
int_fast16_t SD_read(SD_Handle handle, void *buf,
    int_fast32_t sector, uint_fast32_t secCount)
{
    return (handle->fxnTablePtr->readFxn(handle, buf, sector, secCount));
}

/*
 *  ======== SD_write ========
 */
int_fast16_t SD_write(SD_Handle handle, const void *buf,
    int_fast32_t sector, uint_fast32_t secCount)
{
    return (handle->fxnTablePtr->writeFxn(handle, buf, sector, secCount));
}
