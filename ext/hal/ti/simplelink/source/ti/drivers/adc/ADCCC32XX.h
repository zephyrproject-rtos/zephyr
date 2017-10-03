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
/** ============================================================================
 *  @file       ADCCC32XX.h
 *
 *  @brief      ADC driver implementation for the ADC peripheral on CC32XX
 *
 *  This ADC driver implementation is designed to operate on a CC32XX ADC
 *  peripheral.  The ADCCC32XX header file should be included in an application
 *  as follows:
 *  @code
 *  #include <ti/drivers/ADC.h>
 *  #include <ti/drivers/ADCCC32XX.h>
 *  @endcode
 *
 *  Refer to @ref ADC.h for a complete description of APIs & example of use.
 *
 *  ============================================================================
 */
#ifndef ti_drivers_adc_ADCMSP432__include
#define ti_drivers_adc_ADCMSP432__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <ti/drivers/ADC.h>

/*
 *  The bits in the pin mode macros are as follows:
 *  The lower 8 bits of the macro refer to the pin, offset by 1, to match
 *  driverlib pin defines.
 *  The upper 16 bits is the value that can be passed to APIs as the
 *  ulChannel parameter.
 */

#define ADCCC32XX_PIN_57_CH_0  (ADC_CH_0 << 8) | 0x38 /*!< PIN 57 is used for ADC channel 0 */
#define ADCCC32XX_PIN_58_CH_1  (ADC_CH_1 << 8) | 0x39 /*!< PIN 58 is used for ADC channel 1 */
#define ADCCC32XX_PIN_59_CH_2  (ADC_CH_2 << 8) | 0x3a /*!< PIN 59 is used for ADC channel 2 */
#define ADCCC32XX_PIN_60_CH_3  (ADC_CH_3 << 8) | 0x3b /*!< PIN 60 is used for ADC channel 3 */

/* ADC function table pointer */
extern const ADC_FxnTable ADCCC32XX_fxnTable;

/*!
 *  @brief  ADCCC32XX Hardware attributes
 *
 *  These fields are used by driverlib APIs and therefore must be populated by
 *  driverlib macro definitions. For CC32XXWare these definitions are found in:
 *      - ti/devices/cc32xx/driverlib/adc.h
 *
 *  A sample structure is shown below:
 *  @code
 *  const ADCCC32XX_HWAttrsV1 adcCC32XXHWAttrs[Board_ADCCHANNELCOUNT] = {
 *      {
 *          .adcPin = ADCCC32XX_PIN_57
 *      }
 *  };
 *  @endcode
 */
typedef struct ADCCC32XX_HWAttrsV1 {
    uint_fast16_t adcPin;
} ADCCC32XX_HWAttrsV1;

/*!
 *  @brief ADCCC32XX_Status
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct ADCCC32XX_State {
    uint_fast32_t     baseAddr;
    uint_least8_t     numOpenChannels;
} ADCCC32XX_State;

/*!
 *  @brief  ADCCC32XX Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct ADCCC32XX_Object {
    bool              isOpen;
} ADCCC32XX_Object;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_adc_ADCMSP432__include */
