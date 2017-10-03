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
 *  ======== Camera.c ========
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/Camera.h>
#include <ti/drivers/dpl/HwiP.h>

extern const Camera_Config Camera_config[];
extern const uint_least8_t Camera_count;

/* Default Camera parameters structure */
const Camera_Params Camera_defaultParams = {
     Camera_MODE_BLOCKING,               /* captureMode */
     24000000,                           /* outputClock */
     Camera_HSYNC_POLARITY_HIGH,         /* hsyncPolarity */
     Camera_VSYNC_POLARITY_HIGH,         /* vsyncPolarity */
     Camera_PCLK_CONFIG_RISING_EDGE,     /* pixelClkConfig */
     Camera_BYTE_ORDER_NORMAL,           /* byteOrder */
     Camera_INTERFACE_SYNC_ON,           /* interfaceSync */
     Camera_STOP_CAPTURE_FRAME_END,      /* stopConfig */
     Camera_START_CAPTURE_FRAME_START,   /* startConfig */
     Camera_WAIT_FOREVER,                /* captureTimeout */
     NULL,                               /* captureCallback */
     NULL
};

static bool isInitialized = false;

/*
 *  ======== Camera_close ========
 */
void Camera_close(Camera_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== Camera_control ========
 */
int_fast16_t Camera_control(Camera_Handle handle, uint_fast16_t cmd, void *arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== Camera_init ========
 */
void Camera_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < Camera_count; i++) {
            Camera_config[i].fxnTablePtr->initFxn((Camera_Handle)&(Camera_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== Camera_open ========
 */
Camera_Handle Camera_open(uint_least8_t index, Camera_Params *params)
{
    Camera_Handle         handle = NULL;

    /* Verify driver index and state */
    if (isInitialized && (index < Camera_count)) {
        /* If params are NULL use defaults. */
        if (params == NULL) {
            params = (Camera_Params *)&Camera_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (Camera_Handle)&(Camera_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== Camera_Params_init ========
 */
void Camera_Params_init(Camera_Params *params)
{
    *params = Camera_defaultParams;
}

/*
 *  ======== Camera_capture ========
 */
int_fast16_t Camera_capture(Camera_Handle handle, void *buffer,
    size_t bufferlen, size_t *frameLen)
{
    return (handle->fxnTablePtr->captureFxn(handle, buffer, bufferlen, frameLen));
}
