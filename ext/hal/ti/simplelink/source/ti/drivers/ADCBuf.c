/*
 * Copyright (c) 2016, Texas Instruments Incorporated
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
 *  ======== ADCBuf.c ========
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/dpl/HwiP.h>

extern const ADCBuf_Config ADCBuf_config[];
extern const uint_least8_t ADCBuf_count;

/* Default ADC parameters structure */
const ADCBuf_Params ADCBuf_defaultParams = {
    .returnMode         = ADCBuf_RETURN_MODE_BLOCKING,          /*!< Blocking mode */
    .blockingTimeout    = 25000,                                /*!< Timeout of 25000 RTOS ticks */
    .callbackFxn        = NULL,                                 /*!< No callback function */
    .recurrenceMode     = ADCBuf_RECURRENCE_MODE_ONE_SHOT,      /*!< One-shot measurement */
    .samplingFrequency  = 10000,                                /*!< Take samples at 10kHz */
    .custom             = NULL
};

static bool isInitialized = false;

/*
 *  ======== ADCBuf_close ========
 */
void ADCBuf_close(ADCBuf_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== ADCBuf_control ========
 */
int_fast16_t ADCBuf_control(ADCBuf_Handle handle, uint_fast16_t cmd, void *cmdArg)
{
    return handle->fxnTablePtr->controlFxn(handle, cmd, cmdArg);
}

/*
 *  ======== ADCBuf_init ========
 */
void ADCBuf_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < ADCBuf_count; i++) {
            ADCBuf_config[i].fxnTablePtr->initFxn((ADCBuf_Handle) & (ADCBuf_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== ADCBuf_open ========
 */
ADCBuf_Handle ADCBuf_open(uint_least8_t index, ADCBuf_Params *params)
{
    ADCBuf_Handle handle = NULL;

    /* Verify driver index and state */
    if (isInitialized && (index < ADCBuf_count)) {
        /* If params are NULL use defaults */
        if (params == NULL) {
            params = (ADCBuf_Params *)&ADCBuf_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (ADCBuf_Handle)&(ADCBuf_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== ADCBuf_Params_init ========
 */
void ADCBuf_Params_init(ADCBuf_Params *params)
{
    *params = ADCBuf_defaultParams;
}

/*
 *  ======== ADCBuf_convert ========
 */
int_fast16_t ADCBuf_convert(ADCBuf_Handle handle, ADCBuf_Conversion conversions[],  uint_fast8_t channelCount)
{
    return (handle->fxnTablePtr->convertFxn(handle, conversions, channelCount));
}

/*
 *  ======== ADCBuf_convertCancel ========
 */
int_fast16_t ADCBuf_convertCancel(ADCBuf_Handle handle)
{
    return (handle->fxnTablePtr->convertCancelFxn(handle));
}

/*
 *  ======== ADCBuf_getResolution ========
 */
uint_fast8_t ADCBuf_getResolution(ADCBuf_Handle handle)
{
    return (handle->fxnTablePtr->getResolutionFxn(handle));
}

/*
 *  ======== ADCBuf_adjustRawValues ========
 */
int_fast16_t ADCBuf_adjustRawValues(ADCBuf_Handle handle, void *sampleBuf, uint_fast16_t sampleCount, uint32_t adcChan)
{
    return (handle->fxnTablePtr->adjustRawValuesFxn(handle, sampleBuf, sampleCount, adcChan));
}

/*
 *  ======== ADCBuf_convertAdjustedToMicroVolts ========
 */
int_fast16_t ADCBuf_convertAdjustedToMicroVolts(ADCBuf_Handle handle, uint32_t  adcChan, void *adjustedSampleBuffer, uint32_t outputMicroVoltBuffer[], uint_fast16_t sampleCount)
{
    return (handle->fxnTablePtr->convertAdjustedToMicroVoltsFxn(handle, adcChan, adjustedSampleBuffer, outputMicroVoltBuffer, sampleCount));
}
