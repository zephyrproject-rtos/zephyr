/*
 * Copyright (c) 2017, Texas Instruments Incorporated
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
/*!*****************************************************************************
 *  @file       CaptureCC32XX.h
 *  @brief      Capture driver interface for CC32XX devices
 *
 *  # Operation #
 *  This driver uses a general purpose timer hardware peripheral to implement
 *  the capture functionality. The capture driver only uses half of a timer
 *  peripheral (16-bit timer). This is not a software limitation but due to the
 *  general purpose timer hardware implementation. For CC32XX devices, the
 *  system clock is 80 MHz. A 16-bit timer peripheral has 24-bits of
 *  resolution when the prescaler register is used as an 8-bit linear extension.
 *
 *  # Resource Allocation #
 *  Each general purpose timer block contains two timers, Timer A and Timer B,
 *  that can be configured to operate independently. This behavior is managed
 *  through a set of resource allocation APIs. For example, the
 *  TimerCC32XX_allocateTimerResource API will allocate a timer for exclusive
 *  use. Any attempt to re-allocate this resource by the TimerCC32XX, PWMCC32XX,
 *  or CaptureCC32xx drivers will result in a false value being returned from
 *  the allocation API. To free a timer resource, the
 *  TimerCC32XX_freeTimerResource is used. The application is not responsible
 *  for calling these allocation APIs directly.
 *
 *******************************************************************************
 */
#ifndef ti_drivers_capture_CaptureCC32XX__include
#define ti_drivers_capture_CaptureCC32XX__include

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

#include <ti/drivers/Capture.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>

/*! \cond */
/*
 *  Capture port/pin defines for pin configuration.
 *
 *  The timer id (0, 1, 2, or 3) is stored in bits 31 - 30
 *  The timer half (1 = A, 2 = B) is stored in bits 29 - 28
 *  The interrupt number is stored in bits 27 - 20
 *  The GPIO port (0, 1, 2, or 3) is stored in bits 19 - 16
 *  The GPIO pad offset is stored in bits 15 - 4
 *  The pin mode is stored in bits 3 - 0
 *
 *    31 - 30    29 - 28    27 - 20    19 - 16        15 - 4         3 - 0
 *  --------------------------------------------------------------------------
 *  | TimerId | Timer half | IntNum | GPIO Port | GPIO Pad Offset | Pin Mode |
 *  --------------------------------------------------------------------------
 *
 *  The CC32XX has fixed GPIO assignments and pin modes for a given pin.
 *  A Capture pin mode for a given pin has a fixed timer/timer-half.
 */
#define CaptureCC32XX_T0A    (0x10000000 | (INT_TIMERA0A << 20))
#define CaptureCC32XX_T0B    (0x20000000 | (INT_TIMERA0B << 20))
#define CaptureCC32XX_T1A    (0x50000000 | (INT_TIMERA1A << 20))
#define CaptureCC32XX_T1B    (0x60000000 | (INT_TIMERA1B << 20))
#define CaptureCC32XX_T2A    (0x90000000 | (INT_TIMERA2A << 20))
#define CaptureCC32XX_T2B    (0xA0000000 | (INT_TIMERA2B << 20))
#define CaptureCC32XX_T3A    (0xD0000000 | (INT_TIMERA3A << 20))
#define CaptureCC32XX_T3B    (0xE0000000 | (INT_TIMERA3B << 20))

#define CaptureCC32XX_GPIO0  (0x00000000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_0 << 4))
#define CaptureCC32XX_GPIO1  (0x00000000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_1 << 4))
#define CaptureCC32XX_GPIO2  (0x00000000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_2 << 4))
#define CaptureCC32XX_GPIO5  (0x00000000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_5 << 4))
#define CaptureCC32XX_GPIO6  (0x00000000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_6 << 4))
#define CaptureCC32XX_GPIO8  (0x00010000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_8 << 4))
#define CaptureCC32XX_GPIO9  (0x00010000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_9 << 4))
#define CaptureCC32XX_GPIO10 (0x00010000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_10 << 4))
#define CaptureCC32XX_GPIO11 (0x00010000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_11 << 4))
#define CaptureCC32XX_GPIO12 (0x00010000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_12 << 4))
#define CaptureCC32XX_GPIO13 (0x00010000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_13 << 4))
#define CaptureCC32XX_GPIO14 (0x00010000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_14 << 4))
#define CaptureCC32XX_GPIO15 (0x00010000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_15 << 4))
#define CaptureCC32XX_GPIO16 (0x00020000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_16 << 4))
#define CaptureCC32XX_GPIO22 (0x00020000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_22 << 4))
#define CaptureCC32XX_GPIO24 (0x00030000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_24 << 4))
#define CaptureCC32XX_GPIO30 (0x00030000 | (OCP_SHARED_O_GPIO_PAD_CONFIG_30 << 4))
/*! \endcond */

/*!
 *  \defgroup capturePinIdentifiersCC32XX CaptureCC32XX_HWAttrs 'capturePin'
 *            field options.
 *  @{
 */
/*!
 *  @name PIN 01, GPIO10, uses Timer0B for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_01  CaptureCC32XX_T0B | CaptureCC32XX_GPIO10 | 0xC /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 02, GPIO11, uses Timer1A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_02  CaptureCC32XX_T1A | CaptureCC32XX_GPIO11 | 0xC /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 03, GPIO12, uses Timer1B for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_03  CaptureCC32XX_T1B | CaptureCC32XX_GPIO12 | 0xC /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 04, GPIO13, uses Timer2A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_04  CaptureCC32XX_T2A | CaptureCC32XX_GPIO13 | 0xC /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 05, GPIO14, uses Timer2B for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_05  CaptureCC32XX_T2B | CaptureCC32XX_GPIO14 | 0xC /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 06, GPIO15, uses Timer3A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_06  CaptureCC32XX_T3A | CaptureCC32XX_GPIO15 | 0xD /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 07, GPIO16, uses Timer3B for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_07  CaptureCC32XX_T3B | CaptureCC32XX_GPIO16 | 0xD /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 15, GPIO22, uses Timer2A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_15  CaptureCC32XX_T2A | CaptureCC32XX_GPIO22 | 0x5  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 17, GPIO24 (TDO), uses Timer3A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_17  CaptureCC32XX_T3A | CaptureCC32XX_GPIO24 | 0x4  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 50, GPIO0, uses Timer0A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_50  CaptureCC32XX_T0A | CaptureCC32XX_GPIO0 | 0x7  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 53, GPIO30, uses Timer2B for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_53  CaptureCC32XX_T2B | CaptureCC32XX_GPIO30 | 0x4  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 55, GPIO1, uses Timer0B for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_55  CaptureCC32XX_T0B | CaptureCC32XX_GPIO1 | 0x7  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 57, GPIO2, uses Timer1A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_57  CaptureCC32XX_T1A | CaptureCC32XX_GPIO2 | 0x7  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 60, GPIO5, uses Timer2B for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_60  CaptureCC32XX_T2B | CaptureCC32XX_GPIO5 | 0x7  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 61, GPIO6, uses Timer3A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_61  CaptureCC32XX_T3A | CaptureCC32XX_GPIO6 | 0x7  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 63, GPIO8, uses Timer3A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_63  CaptureCC32XX_T3A | CaptureCC32XX_GPIO8 | 0xC  /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 64, GPIO9, uses Timer0A for Capture.
 *  @{
 */
#define CaptureCC32XX_PIN_64  CaptureCC32XX_T0A | CaptureCC32XX_GPIO9 | 0xC  /*!< @hideinitializer */
/*! @} */
/*! @} */

extern const Capture_FxnTable CaptureCC32XX_fxnTable;

/*!
 *  @brief CaptureCC32XX Hardware Attributes
 *
 *  Timer hardware attributes that tell the CaptureCC32XX driver specific hardware
 *  configurations and interrupt/priority settings.
 *
 *  A sample structure is shown below:
 *  @code
 *  const CaptureCC32XX_HWAttrs captureCC32XXHWAttrs[] =
 *  {
 *      {
 *          .capturePin = CaptureCC32XX_PIN_04,
 *          .intPriority = ~0
 *      },
 *      {
 *          .capturePin = CaptureCC32XX_PIN_05,
 *          .intPriority = ~0
 *      },
 *  };
 *  @endcode
 */
typedef struct CaptureCC32XX_HWAttrs_ {
    /*!< Specifies the input pin for the capture event. There are 17
         pins available as inputs for capture functionality. Each
         available pin must map to a specific general purpose timer
         hardware instance. By specifying this attribute, a fixed
         16-bit timer peripheral is used. */
    uint32_t             capturePin;

    /*!< The interrupt priority. */
    uint32_t             intPriority;
} CaptureCC32XX_HWAttrs;

/*!
 *  @brief CaptureCC32XX_Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct CaptureCC32XX_Object_ {
    HwiP_Handle           hwiHandle;
    Power_NotifyObj       notifyObj;
    Capture_CallBackFxn   callBack;
    Capture_PeriodUnits   periodUnits;
    uint32_t              mode;
    uint32_t              timer;
    uint32_t              previousCount;
    bool                  isRunning;
} CaptureCC32XX_Object;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_capture_CaptureCC32XX__include */
