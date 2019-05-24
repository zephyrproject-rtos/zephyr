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
 *  ======== PWM.c ========
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/PWM.h>

extern const PWM_Config PWM_config[];
extern const uint_least8_t PWM_count;

/* Default PWM parameters structure */
const PWM_Params PWM_defaultParams = {
    .periodUnits = PWM_PERIOD_HZ,             /* Period is defined in Hz */
    .periodValue = 1e6,                       /* 1MHz */
    .dutyUnits   = PWM_DUTY_FRACTION,         /* Duty is fraction of period */
    .dutyValue   = 0,                         /* 0% duty cycle */
    .idleLevel   = PWM_IDLE_LOW,              /* Low idle level */
    .custom      = NULL                       /* No custom params */
};

static bool isInitialized = false;

/*
 *  ======== PWM_close ========
 */
void PWM_close(PWM_Handle handle)
{
    handle->fxnTablePtr->closeFxn(handle);
}

/*
 *  ======== PWM_control ========
 */
int_fast16_t PWM_control(PWM_Handle handle, uint_fast16_t cmd, void *arg)
{
    return handle->fxnTablePtr->controlFxn(handle, cmd, arg);
}

/*
 *  ======== PWM_init ========
 */
void PWM_init(void)
{
    uint_least8_t i;
    uint_fast32_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        isInitialized = (bool) true;

        /* Call each driver's init function */
        for (i = 0; i < PWM_count; i++) {
            PWM_config[i].fxnTablePtr->initFxn((PWM_Handle) &(PWM_config[i]));
        }
    }

    HwiP_restore(key);
}

/*
 *  ======== PWM_open ========
 */
PWM_Handle PWM_open(uint_least8_t index, PWM_Params *params)
{
    PWM_Handle handle = NULL;

    if (isInitialized && (index < PWM_count)) {
        /* If params are NULL use defaults */
        if (params == NULL) {
            params = (PWM_Params *) &PWM_defaultParams;
        }

        /* Get handle for this driver instance */
        handle = (PWM_Handle) &(PWM_config[index]);

        handle = handle->fxnTablePtr->openFxn(handle, params);
    }

    return (handle);
}

/*
 *  ======== PWM_Params_init ========
 */
void PWM_Params_init(PWM_Params *params)
{
    *params = PWM_defaultParams;
}

/*
 *  ======== PWM_setDuty ========
 */
int_fast16_t PWM_setDuty(PWM_Handle handle, uint32_t duty)
{
    return(handle->fxnTablePtr->setDutyFxn(handle, duty));
}

/*
 *  ======== PWM_setDuty ========
 */
int_fast16_t PWM_setPeriod(PWM_Handle handle, uint32_t period)
{
    return(handle->fxnTablePtr->setPeriodFxn(handle, period));
}
/*
 *  ======== PWM_setDutyandPeriod ========
 */
int_fast16_t PWM_setDutyAndPeriod(PWM_Handle handle, uint32_t duty, uint32_t period)
{
    return(handle->fxnTablePtr->setDutyAndPeriodFxn(handle, duty, period));
}

/*
 *  ======== PWM_start ========
 */
void PWM_start(PWM_Handle handle)
{
    handle->fxnTablePtr->startFxn(handle);
}

/*
 *  ======== PWM_stop ========
 */
void PWM_stop(PWM_Handle handle)
{
    handle->fxnTablePtr->stopFxn(handle);
}
