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
 *  ======== UART.c ========
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/UART.h>

extern const UART_Config UART_config[];
extern const uint_least8_t UART_count;

/* Default UART parameters structure */
const UART_Params UART_defaultParams = {
    UART_MODE_BLOCKING,   /* readMode */
    UART_MODE_BLOCKING,   /* writeMode */
    UART_WAIT_FOREVER,    /* readTimeout */
    UART_WAIT_FOREVER,    /* writeTimeout */
    NULL,                 /* readCallback */
    NULL,                 /* writeCallback */
    UART_RETURN_NEWLINE,  /* readReturnMode */
    UART_DATA_TEXT,       /* readDataMode */
    UART_DATA_TEXT,       /* writeDataMode */
    UART_ECHO_ON,         /* readEcho */
    115200,               /* baudRate */
    UART_LEN_8,           /* dataLength */
    UART_STOP_ONE,        /* stopBits */
    UART_PAR_NONE,        /* parityType */
    NULL                  /* custom */
};

static bool isInitialized = false;

/*
 *  ======== UART_close ========
 */
void UART_close(UART_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== UART_control ========
 */
int_fast16_t UART_control(UART_Handle handle, uint_fast16_t cmd, void *arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== UART_init ========
 */
void UART_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < UART_count; i++) {
            UART_config[i].fxnTablePtr->initFxn((UART_Handle) &(UART_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== UART_open ========
 */
UART_Handle UART_open(uint_least8_t index, UART_Params *params)
{
    UART_Handle handle = NULL;

    if (isInitialized && (index < UART_count)) {
        /* If params are NULL use defaults */
        if (params == NULL) {
            params = (UART_Params *) &UART_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (UART_Handle)&(UART_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== UART_Params_init ========
 */
void UART_Params_init(UART_Params *params)
{
    *params = UART_defaultParams;
}

/*
 *  ======== UART_read ========
 */
int_fast32_t UART_read(UART_Handle handle, void *buffer, size_t size)
{
    return (handle->fxnTablePtr->readFxn(handle, buffer, size));
}

/*
 *  ======== UART_readPolling ========
 */
int_fast32_t UART_readPolling(UART_Handle handle, void *buffer, size_t size)
{
    return (handle->fxnTablePtr->readPollingFxn(handle, buffer, size));
}

/*
 *  ======== UART_readCancel ========
 */
void UART_readCancel(UART_Handle handle)
{
    handle->fxnTablePtr->readCancelFxn(handle);
}

/*
 *  ======== UART_write ========
 */
int_fast32_t UART_write(UART_Handle handle, const void *buffer, size_t size)
{
    return (handle->fxnTablePtr->writeFxn(handle, buffer, size));
}

/*
 *  ======== UART_writePolling ========
 */
int_fast32_t UART_writePolling(UART_Handle handle, const void *buffer,
    size_t size)
{
    return (handle->fxnTablePtr->writePollingFxn(handle, buffer, size));
}

/*
 *  ======== UART_writeCancel ========
 */
void UART_writeCancel(UART_Handle handle)
{
    handle->fxnTablePtr->writeCancelFxn(handle);
}
