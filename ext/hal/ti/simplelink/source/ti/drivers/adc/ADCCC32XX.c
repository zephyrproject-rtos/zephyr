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
 * By default disable both asserts and log for this module.
 * This must be done before DebugP.h is included.
 */
#ifndef DebugP_ASSERT_ENABLED
#define DebugP_ASSERT_ENABLED 0
#endif
#ifndef DebugP_LOG_ENABLED
#define DebugP_LOG_ENABLED 0
#endif

#include <stdint.h>

#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>

#include <ti/drivers/adc/ADCCC32XX.h>

#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/rom_patch.h>
#include <ti/devices/cc32xx/driverlib/adc.h>
#include <ti/devices/cc32xx/driverlib/pin.h>

/* Pad configuration register defaults */
#define PAD_CONFIG_BASE (OCP_SHARED_BASE + OCP_SHARED_O_GPIO_PAD_CONFIG_0)
#define PAD_RESET_DRIVE PIN_STRENGTH_6MA
#define PAD_RESET_TYPE  PIN_TYPE_STD
#define PAD_RESET_STATE 0xC61

#define PinConfigChannel(config) (((config) >> 8) & 0xff)
#define PinConfigPin(config) ((config) & 0xff)

void ADCCC32XX_close(ADC_Handle handle);
int_fast16_t ADCCC32XX_control(ADC_Handle handle, uint_fast16_t cmd, void *arg);
int_fast16_t ADCCC32XX_convert(ADC_Handle handle, uint16_t *value);
uint32_t ADCCC32XX_convertToMicroVolts(ADC_Handle handle,
    uint16_t adcValue);
void ADCCC32XX_init(ADC_Handle handle);
ADC_Handle ADCCC32XX_open(ADC_Handle handle, ADC_Params *params);

/* ADC function table for ADCCC32XX implementation */
const ADC_FxnTable ADCCC32XX_fxnTable = {
    ADCCC32XX_close,
    ADCCC32XX_control,
    ADCCC32XX_convert,
    ADCCC32XX_convertToMicroVolts,
    ADCCC32XX_init,
    ADCCC32XX_open
};

/* Internal ADC status structure */
static ADCCC32XX_State state = {
    .baseAddr = ADC_BASE,
    .numOpenChannels = 0
};

/*
 *  ======== ADCCC32XX_close ========
 */
void ADCCC32XX_close(ADC_Handle handle)
{
    uintptr_t         key;
    uint32_t          pin;
    uint32_t          padRegister;
    ADCCC32XX_Object *object = handle->object;
    ADCCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    pin = PinConfigChannel(hwAttrs->adcPin);

    key = HwiP_disable();

    MAP_ADCChannelDisable(state.baseAddr, pin);
    state.numOpenChannels--;

    if (state.numOpenChannels == 0) {
        MAP_ADCDisable(state.baseAddr);
    }

    HwiP_restore(key);

    /* Call PinConfigSet to de-isolate the input */
    pin = PinConfigPin(hwAttrs->adcPin);
    MAP_PinConfigSet(pin, PAD_RESET_DRIVE, PAD_RESET_TYPE);
    /* Set reset state for the pad register */
    padRegister = (PinToPadGet(pin)<<2) + PAD_CONFIG_BASE;
    HWREG(padRegister) = PAD_RESET_STATE;

    object->isOpen = false;

    DebugP_log0("ADC: (%p) closed");
}

/*
 *  ======== ADCCC32XX_control ========
 */
int_fast16_t ADCCC32XX_control(ADC_Handle handle, uint_fast16_t cmd, void *arg)
{
    /* No implementation yet */
    return (ADC_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== ADCCC32XX_convert ========
 */
int_fast16_t ADCCC32XX_convert(ADC_Handle handle, uint16_t *value)
{
    uintptr_t                  key;
    uint16_t                   adcSample = 0;
    uint_fast16_t              adcChannel;
    ADCCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;

    adcChannel = PinConfigChannel(hwAttrs->adcPin);
    key = HwiP_disable();

    /* Wait until the FIFO is not empty */
    while (MAP_ADCFIFOLvlGet(state.baseAddr, adcChannel) == 0) {
        HwiP_restore(key);
        key = HwiP_disable();
    }

    adcSample = MAP_ADCFIFORead(ADC_BASE, adcChannel);

    HwiP_restore(key);

    /* Strip time stamp & reserve bits from ADC sample */
    *value = ((adcSample >> 2) & 0x0FFF);

    return (ADC_STATUS_SUCCESS);
}

/*
 *  ======== ADCCC32XX_convertToMicroVolts ========
 */
uint32_t ADCCC32XX_convertToMicroVolts(ADC_Handle handle,
    uint16_t adcValue)
{
    /* Internal voltage reference is 1.467V (1467000 uV) */
    return ((uint_fast32_t)(adcValue * (1467000.0 / 4095.0)));
}

/*
 *  ======== ADCCC32XX_init ========
 */
void ADCCC32XX_init(ADC_Handle handle)
{
    /* Mark the object as available */
    ((ADCCC32XX_Object *) handle->object)->isOpen = false;
}

/*
 *  ======== ADCCC32XX_open ========
 */
ADC_Handle ADCCC32XX_open(ADC_Handle handle, ADC_Params *params)
{
    uintptr_t                  key;
    uint32_t                   pin;
    ADCCC32XX_Object          *object = handle->object;
    ADCCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    uint32_t                   i;

    key = HwiP_disable();

    if (object->isOpen) {
        HwiP_restore(key);

        DebugP_log1("ADC: (%p) already opened", (uintptr_t) handle);
        return (NULL);
    }

    object->isOpen = true;
    state.numOpenChannels++;

    HwiP_restore(key);

    /* Configure pin as ADC input */
    pin = PinConfigPin(hwAttrs->adcPin);
    MAP_PinTypeADC(pin, PIN_MODE_255);

    /* Enable ADC Peripheral and ADC Channel */
    pin = PinConfigChannel(hwAttrs->adcPin);
    MAP_ADCEnable(state.baseAddr);
    MAP_ADCChannelEnable(state.baseAddr, pin);

    /* Empty 5 samples from the FIFO */
    for (i = 0; i < 5; i++) {
        while (MAP_ADCFIFOLvlGet(state.baseAddr, pin) == 0){
        }
        MAP_ADCFIFORead(state.baseAddr, pin);
    }

    DebugP_log1("ADC: (%p) instance opened.", (uintptr_t) handle);

    return (handle);
}
