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
/*
 *  ======== ADC.c ========
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/ADC.h>
#include <ti/drivers/dpl/HwiP.h>

extern const ADC_Config ADC_config[];
extern const uint_least8_t ADC_count;

/* Default ADC parameters structure */
const ADC_Params ADC_defaultParams = {
    .custom = NULL,
    .isProtected = true
};

static bool isInitialized = false;

/*
 *  ======== ADC_close ========
 */
void ADC_close(ADC_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== ADC_control ========
 */
int_fast16_t ADC_control(ADC_Handle handle, uint_fast16_t cmd, void *arg)
{
    return (handle->fxnTablePtr->controlFxn(handle, cmd, arg));
}

/*
 *  ======== ADC_convert ========
 */
int_fast16_t ADC_convert(ADC_Handle handle, uint16_t *value)
{
    return (handle->fxnTablePtr->convertFxn(handle, value));
}

/*
 *  ======== ADC_convertToMicroVolts ========
 */
uint32_t ADC_convertToMicroVolts(ADC_Handle handle, uint16_t adcValue)
{
    return (handle->fxnTablePtr->convertToMicroVolts(handle, adcValue));
}

/*
 *  ======== ADC_init ========
 */
void ADC_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < ADC_count; i++) {
            ADC_config[i].fxnTablePtr->initFxn((ADC_Handle)&(ADC_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== ADC_open ========
 */
ADC_Handle ADC_open(uint_least8_t index, ADC_Params *params)
{
    ADC_Handle handle = NULL;

    if (isInitialized && (index < ADC_count)) {
        /* If params are NULL use defaults */
        if (params == NULL) {
            params = (ADC_Params *) &ADC_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (ADC_Handle) &(ADC_config[index]);
        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== ADC_Params_init ========
 */
void ADC_Params_init(ADC_Params *params)
{
    *params = ADC_defaultParams;
}
