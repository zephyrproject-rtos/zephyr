/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016, NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_RESET_H_
#define _FSL_RESET_H_

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "fsl_device_registers.h"

/*!
 * @addtogroup ksdk_common
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief reset driver version 2.0.1. */
#define FSL_RESET_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*!
 * @brief Enumeration for peripheral reset control bits
 *
 * Defines the enumeration for peripheral reset control bits in PRESETCTRL/ASYNCPRESETCTRL registers
 */
typedef enum _SYSCON_RSTn
{
    kFLASH_RST_SHIFT_RSTn = 0 | 7U,          /**< Flash controller reset control */
    kFMC_RST_SHIFT_RSTn = 0 | 8U,            /**< Flash accelerator reset control */
    kMUX_RST_SHIFT_RSTn = 0 | 11U,           /**< Input mux reset control */
    kIOCON_RST_SHIFT_RSTn = 0 | 13U,         /**< IOCON reset control */
    kGPIO0_RST_SHIFT_RSTn = 0 | 14U,         /**< GPIO0 reset control */
    kGPIO1_RST_SHIFT_RSTn = 0 | 15U,         /**< GPIO1 reset control */
    kPINT_RST_SHIFT_RSTn = 0 | 18U,          /**< Pin interrupt (PINT) reset control */
    kGINT_RST_SHIFT_RSTn = 0 | 19U,          /**< Grouped interrupt (PINT) reset control. */
    kDMA_RST_SHIFT_RSTn = 0 | 20U,           /**< DMA reset control */
    kCRC_RST_SHIFT_RSTn = 0 | 21U,           /**< CRC reset control */
    kWWDT_RST_SHIFT_RSTn = 0 | 22U,          /**< Watchdog timer reset control */
    kADC0_RST_SHIFT_RSTn = 0 | 27U,          /**< ADC0 reset control */
    kMRT_RST_SHIFT_RSTn = 65536 | 0U,        /**< Multi-rate timer (MRT) reset control */
    kSCT0_RST_SHIFT_RSTn = 65536 | 2U,       /**< SCTimer/PWM 0 (SCT0) reset control */
    kUTICK_RST_SHIFT_RSTn = 65536 | 10U,     /**< Micro-tick timer reset control */
    kFC0_RST_SHIFT_RSTn = 65536 | 11U,       /**< Flexcomm Interface 0 reset control */
    kFC1_RST_SHIFT_RSTn = 65536 | 12U,       /**< Flexcomm Interface 1 reset control */
    kFC2_RST_SHIFT_RSTn = 65536 | 13U,       /**< Flexcomm Interface 2 reset control */
    kFC3_RST_SHIFT_RSTn = 65536 | 14U,       /**< Flexcomm Interface 3 reset control */
    kFC4_RST_SHIFT_RSTn = 65536 | 15U,       /**< Flexcomm Interface 4 reset control */
    kFC5_RST_SHIFT_RSTn = 65536 | 16U,       /**< Flexcomm Interface 5 reset control */
    kFC6_RST_SHIFT_RSTn = 65536 | 17U,       /**< Flexcomm Interface 6 reset control */
    kFC7_RST_SHIFT_RSTn = 65536 | 18U,       /**< Flexcomm Interface 7 reset control */
    kDMIC_RST_SHIFT_RSTn = 65536 | 19U,      /**< Digital microphone interface reset control */
    kCT32B2_RST_SHIFT_RSTn = 65536 | 22U,    /**< CT32B2 reset control */
    kUSB_RST_SHIFT_RSTn = 65536 | 25U,       /**< USB reset control */
    kCT32B0_RST_SHIFT_RSTn = 65536 | 26U,    /**< CT32B0 reset control */
    kCT32B1_RST_SHIFT_RSTn = 65536 | 27U,    /**< CT32B1 reset control */
    kCT32B3_RST_SHIFT_RSTn = 67108864 | 13U, /**< CT32B3 reset control */
    kCT32B4_RST_SHIFT_RSTn = 67108864 | 14U, /**< CT32B4 reset control */
} SYSCON_RSTn_t;

/** Array initializers with peripheral reset bits **/
#define ADC_RSTS             \
    {                        \
        kADC0_RST_SHIFT_RSTn \
    } /* Reset bits for ADC peripheral */
#define CRC_RSTS            \
    {                       \
        kCRC_RST_SHIFT_RSTn \
    } /* Reset bits for CRC peripheral */
#define DMA_RSTS_N            \
    {                       \
        kDMA_RST_SHIFT_RSTn \
    } /* Reset bits for DMA peripheral */
#define DMIC_RSTS            \
    {                        \
        kDMIC_RST_SHIFT_RSTn \
    } /* Reset bits for ADC peripheral */
#define FLEXCOMM_RSTS                                                                                            \
    {                                                                                                            \
        kFC0_RST_SHIFT_RSTn, kFC1_RST_SHIFT_RSTn, kFC2_RST_SHIFT_RSTn, kFC3_RST_SHIFT_RSTn, kFC4_RST_SHIFT_RSTn, \
            kFC5_RST_SHIFT_RSTn, kFC6_RST_SHIFT_RSTn, kFC7_RST_SHIFT_RSTn                                        \
    } /* Reset bits for FLEXCOMM peripheral */
#define GINT_RSTS                                  \
    {                                              \
        kGINT_RST_SHIFT_RSTn, kGINT_RST_SHIFT_RSTn \
    } /* Reset bits for GINT peripheral. GINT0 & GINT1 share same slot */
#define GPIO_RSTS_N                                    \
    {                                                \
        kGPIO0_RST_SHIFT_RSTn, kGPIO1_RST_SHIFT_RSTn \
    } /* Reset bits for GPIO peripheral */
#define INPUTMUX_RSTS       \
    {                       \
        kMUX_RST_SHIFT_RSTn \
    } /* Reset bits for INPUTMUX peripheral */
#define IOCON_RSTS            \
    {                         \
        kIOCON_RST_SHIFT_RSTn \
    } /* Reset bits for IOCON peripheral */
#define FLASH_RSTS                                 \
    {                                              \
        kFLASH_RST_SHIFT_RSTn, kFMC_RST_SHIFT_RSTn \
    } /* Reset bits for Flash peripheral */
#define MRT_RSTS            \
    {                       \
        kMRT_RST_SHIFT_RSTn \
    } /* Reset bits for MRT peripheral */
#define PINT_RSTS            \
    {                        \
        kPINT_RST_SHIFT_RSTn \
    } /* Reset bits for PINT peripheral */
#define SCT_RSTS             \
    {                        \
        kSCT0_RST_SHIFT_RSTn \
    } /* Reset bits for SCT peripheral */
#define CTIMER_RSTS                                                                                      \
    {                                                                                                   \
        kCT32B0_RST_SHIFT_RSTn, kCT32B1_RST_SHIFT_RSTn, kCT32B2_RST_SHIFT_RSTn, kCT32B3_RST_SHIFT_RSTn, \
            kCT32B4_RST_SHIFT_RSTn                                                                      \
    } /* Reset bits for TIMER peripheral */
#define USB_RSTS            \
    {                       \
        kUSB_RST_SHIFT_RSTn \
    } /* Reset bits for USB peripheral */
#define UTICK_RSTS            \
    {                         \
        kUTICK_RST_SHIFT_RSTn \
    } /* Reset bits for UTICK peripheral */
#define WWDT_RSTS            \
    {                        \
        kWWDT_RST_SHIFT_RSTn \
    } /* Reset bits for WWDT peripheral */

typedef SYSCON_RSTn_t reset_ip_name_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Assert reset to peripheral.
 *
 * Asserts reset signal to specified peripheral module.
 *
 * @param peripheral Assert reset to this peripheral. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_SetPeripheralReset(reset_ip_name_t peripheral);

/*!
 * @brief Clear reset to peripheral.
 *
 * Clears reset signal to specified peripheral module, allows it to operate.
 *
 * @param peripheral Clear reset to this peripheral. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_ClearPeripheralReset(reset_ip_name_t peripheral);

/*!
 * @brief Reset peripheral module.
 *
 * Reset peripheral module.
 *
 * @param peripheral Peripheral to reset. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_PeripheralReset(reset_ip_name_t peripheral);

/*!
 * @brief Set slave core to reset state and hold.
 */
void RESET_SetSlaveCoreReset(void);

/*!
 * @brief Release slave core from reset state.
 */
void RESET_ClearSlaveCoreReset(void);

/*!
 * @brief Reset slave core with the boot entry.
 */
void RESET_SlaveCoreReset(uint32_t bootAddr, uint32_t bootStackPointer);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FSL_RESET_H_ */
