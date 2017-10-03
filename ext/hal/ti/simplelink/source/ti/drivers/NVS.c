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
 *  ======== NVS.c ========
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/NVS.h>

extern NVS_Config NVS_config[];
extern const uint8_t NVS_count;

static bool isInitialized = false;

/* Default NVS parameters structure */
const NVS_Params NVS_defaultParams = {
    NULL    /* custom */
};

/*
 *  ======== NVS_close =======
 */
void NVS_close(NVS_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== NVS_control ========
 */
int_fast16_t NVS_control(NVS_Handle handle, uint_fast16_t cmd, uintptr_t arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== NVS_erase =======
 */
int_fast16_t NVS_erase(NVS_Handle handle, size_t offset, size_t size)
{
    return (handle->fxnTablePtr->eraseFxn(handle, offset, size));
}

/*
 *  ======== NVS_getAttrs =======
 */
void NVS_getAttrs(NVS_Handle handle, NVS_Attrs *attrs)
{
    handle->fxnTablePtr->getAttrsFxn(handle, attrs);
}

/*
 *  ======== NVS_init =======
 */
void NVS_init(void)
{
    uint_least8_t i;

    /* Call each driver's init function */
    for (i = 0; i < NVS_count; i++) {
        NVS_config[i].fxnTablePtr->initFxn();
    }

    isInitialized = true;
}

/*
 *  ======== NVS_open =======
 */
NVS_Handle NVS_open(uint_least8_t index, NVS_Params *params)
{
    NVS_Handle handle = NULL;

    /* do init if not done yet */
    if (!isInitialized) {
        NVS_init();
    }

    if (index < NVS_count) {
        if (params == NULL) {
            /* No params passed in, so use the defaults */
            params = (NVS_Params *)&NVS_defaultParams;
        }
        handle = NVS_config[index].fxnTablePtr->openFxn(index, params);
    }

    return (handle);
}

/*
 *  ======== NVS_Params_init =======
 */
void NVS_Params_init(NVS_Params *params)
{
    *params = NVS_defaultParams;
}

/*
 *  ======== NVS_read =======
 */
int_fast16_t NVS_read(NVS_Handle handle, size_t offset, void *buffer,
             size_t bufferSize)
{
    return (handle->fxnTablePtr->readFxn(handle, offset, buffer, bufferSize));
}

/*
 *  ======== NVS_write =======
 */
int_fast16_t NVS_write(NVS_Handle handle, size_t offset, void *buffer,
              size_t bufferSize, uint_fast16_t flags)
{
    return (handle->fxnTablePtr->writeFxn(handle, offset, buffer,
                    bufferSize, flags));
}
