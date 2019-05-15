/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016, NXP
 * All rights reserved.
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
/*! @brief reset driver version 2.0.0. */
#define FSL_RESET_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*!
 * @brief Enumeration for peripheral reset control bits
 *
 * Defines the enumeration for peripheral reset control bits in PRESETCTRL/ASYNCPRESETCTRL registers
 */
typedef enum _SYSCON_RSTn
{
    kROM_RST_SHIFT_RSTn = 0 | 1U,      /**< ROM reset control */
    kSRAM1_RST_SHIFT_RSTn = 0 | 3U,    /**< SRAM1 reset control */
    kSRAM2_RST_SHIFT_RSTn = 0 | 4U,    /**< SRAM2 reset control */
    kSRAM3_RST_SHIFT_RSTn = 0 | 5U,    /**< SRAM3 reset control */
    kSRAM4_RST_SHIFT_RSTn = 0 | 6U,    /**< SRAM4 reset control */
    kFLASH_RST_SHIFT_RSTn = 0 | 7U,    /**< Flash controller reset control */
    kFMC_RST_SHIFT_RSTn = 0 | 8U,      /**< Flash accelerator reset control */
    kSPIFI_RST_SHIFT_RSTn = 0 | 10U,   /**< SPIFI reset control */
    kMUX0_RST_SHIFT_RSTn = 0 | 11U,    /**< Input mux0 reset control */
    kIOCON_RST_SHIFT_RSTn = 0 | 13U,   /**< IOCON reset control */
    kGPIO0_RST_SHIFT_RSTn = 0 | 14U,   /**< GPIO0 reset control */
    kGPIO1_RST_SHIFT_RSTn = 0 | 15U,   /**< GPIO1 reset control */
    kGPIO2_RST_SHIFT_RSTn = 0 | 16U,   /**< GPIO2 reset control */
    kGPIO3_RST_SHIFT_RSTn = 0 | 17U,   /**< GPIO3 reset control */
    kPINT_RST_SHIFT_RSTn = 0 | 18U,    /**< Pin interrupt (PINT) reset control */
    kGINT_RST_SHIFT_RSTn = 0 | 19U,    /**< Grouped interrupt (PINT) reset control. */
    kDMA0_RST_SHIFT_RSTn = 0 | 20U,    /**< DMA reset control */
    kCRC_RST_SHIFT_RSTn = 0 | 21U,     /**< CRC reset control */
    kWWDT_RST_SHIFT_RSTn = 0 | 22U,    /**< Watchdog timer reset control */
    kRTC_RST_SHIFT_RSTn = 0 | 23U,     /**< RTC reset control */
    kMAILBOX_RST_SHIFT_RSTn = 0 | 26U, /**< Mailbox reset control */
    kADC0_RST_SHIFT_RSTn = 0 | 27U,    /**< ADC0 reset control */

    kMRT_RST_SHIFT_RSTn = 65536 | 0U,      /**< Multi-rate timer (MRT) reset control */
    kOSTIMER0_RST_SHIFT_RSTn = 65536 | 1U, /**< OSTimer0 reset control */
    kSCT0_RST_SHIFT_RSTn = 65536 | 2U,     /**< SCTimer/PWM 0 (SCT0) reset control */
    kSCTIPU_RST_SHIFT_RSTn = 65536 | 6U,   /**< SCTIPU reset control */
    kUTICK_RST_SHIFT_RSTn = 65536 | 10U,   /**< Micro-tick timer reset control */
    kFC0_RST_SHIFT_RSTn = 65536 | 11U,     /**< Flexcomm Interface 0 reset control */
    kFC1_RST_SHIFT_RSTn = 65536 | 12U,     /**< Flexcomm Interface 1 reset control */
    kFC2_RST_SHIFT_RSTn = 65536 | 13U,     /**< Flexcomm Interface 2 reset control */
    kFC3_RST_SHIFT_RSTn = 65536 | 14U,     /**< Flexcomm Interface 3 reset control */
    kFC4_RST_SHIFT_RSTn = 65536 | 15U,     /**< Flexcomm Interface 4 reset control */
    kFC5_RST_SHIFT_RSTn = 65536 | 16U,     /**< Flexcomm Interface 5 reset control */
    kFC6_RST_SHIFT_RSTn = 65536 | 17U,     /**< Flexcomm Interface 6 reset control */
    kFC7_RST_SHIFT_RSTn = 65536 | 18U,     /**< Flexcomm Interface 7 reset control */
    kCTIMER2_RST_SHIFT_RSTn = 65536 | 22U, /**< CTimer 2 reset control */
    kUSB0D_RST_SHIFT_RSTn = 65536 | 25U,   /**< USB0 Device reset control */
    kCTIMER0_RST_SHIFT_RSTn = 65536 | 26U, /**< CTimer 0 reset control */
    kCTIMER1_RST_SHIFT_RSTn = 65536 | 27U, /**< CTimer 1 reset control */
    kPVT_RST_SHIFT_RSTn = 65536 | 28U,     /**< PVT reset control */
    kEZHA_RST_SHIFT_RSTn = 65536 | 30U,    /**< EZHA reset control */
    kEZHB_RST_SHIFT_RSTn = 65536 | 31U,    /**< EZHB reset control */

    kDMA1_RST_SHIFT_RSTn = 131072 | 1U,        /**< DMA1 reset control */
    kCMP_RST_SHIFT_RSTn = 131072 | 2U,         /**< CMP reset control */
    kSDIO_RST_SHIFT_RSTn = 131072 | 3U,        /**< SDIO reset control */
    kUSB1H_RST_SHIFT_RSTn = 131072 | 4U,       /**< USBHS Host reset control */
    kUSB1D_RST_SHIFT_RSTn = 131072 | 5U,       /**< USBHS Device reset control */
    kUSB1RAM_RST_SHIFT_RSTn = 131072 | 6U,     /**< USB RAM reset control */
    kUSB1_RST_SHIFT_RSTn = 131072 | 7U,        /**< USBHS reset control */
    kFREQME_RST_SHIFT_RSTn = 131072 | 8U,      /**< FREQME reset control */
    kGPIO4_RST_SHIFT_RSTn = 131072 | 9U,       /**< GPIO4 reset control */
    kGPIO5_RST_SHIFT_RSTn = 131072 | 10U,      /**< GPIO5 reset control */
    kAES_RST_SHIFT_RSTn = 131072 | 11U,        /**< AES reset control */
    kOTP_RST_SHIFT_RSTn = 131072 | 12U,        /**< OTP reset control */
    kRNG_RST_SHIFT_RSTn = 131072 | 13U,        /**< RNG  reset control */
    kMUX1_RST_SHIFT_RSTn = 131072 | 14U,       /**< Input mux1 reset control */
    kUSB0HMR_RST_SHIFT_RSTn = 131072 | 16U,    /**< USB0HMR reset control */
    kUSB0HSL_RST_SHIFT_RSTn = 131072 | 17U,    /**< USB0HSL reset control */
    kHASHCRYPT_RST_SHIFT_RSTn = 131072 | 18U,  /**< HASHCRYPT reset control */
    kPOWERQUAD_RST_SHIFT_RSTn = 131072 | 19U,  /**< PowerQuad reset control */
    kPLULUT_RST_SHIFT_RSTn = 131072 | 20U,     /**< PLU LUT reset control */
    kCTIMER3_RST_SHIFT_RSTn = 131072 | 21U,    /**< CTimer 3 reset control */
    kCTIMER4_RST_SHIFT_RSTn = 131072 | 22U,    /**< CTimer 4 reset control */
    kPUF_RST_SHIFT_RSTn = 131072 | 23U,        /**< PUF reset control */
    kCASPER_RST_SHIFT_RSTn = 131072 | 24U,     /**< CASPER reset control */
    kCAP0_RST_SHIFT_RSTn = 131072 | 25U,       /**< CASPER reset control */
    kOSTIMER1_RST_SHIFT_RSTn = 131072 | 26U,   /**< OSTIMER1 reset control */
    kANALOGCTL_RST_SHIFT_RSTn = 131072 | 27U,  /**< ANALOG_CTL reset control */
    kHSLSPI_RST_SHIFT_RSTn = 131072 | 28U,     /**< HS LSPI reset control */
    kGPIOSEC_RST_SHIFT_RSTn = 131072 | 29U,    /**< GPIO Secure reset control */
    kGPIOSECINT_RST_SHIFT_RSTn = 131072 | 30U, /**< GPIO Secure int reset control */
} SYSCON_RSTn_t;

/** Array initializers with peripheral reset bits **/
#define ADC_RSTS             \
    {                        \
        kADC0_RST_SHIFT_RSTn \
    } /* Reset bits for ADC peripheral */
#define AES_RSTS            \
    {                       \
        kAES_RST_SHIFT_RSTn \
    } /* Reset bits for AES peripheral */
#define CRC_RSTS            \
    {                       \
        kCRC_RST_SHIFT_RSTn \
    } /* Reset bits for CRC peripheral */
#define CTIMER_RSTS                                                                                         \
    {                                                                                                       \
        kCTIMER0_RST_SHIFT_RSTn, kCTIMER1_RST_SHIFT_RSTn, kCTIMER2_RST_SHIFT_RSTn, kCTIMER3_RST_SHIFT_RSTn, \
            kCTIMER4_RST_SHIFT_RSTn                                                                         \
    } /* Reset bits for CTIMER peripheral */
#define DMA_RSTS_N                                 \
    {                                              \
        kDMA0_RST_SHIFT_RSTn, kDMA1_RST_SHIFT_RSTn \
    } /* Reset bits for DMA peripheral */

#define FLEXCOMM_RSTS                                                                                            \
    {                                                                                                            \
        kFC0_RST_SHIFT_RSTn, kFC1_RST_SHIFT_RSTn, kFC2_RST_SHIFT_RSTn, kFC3_RST_SHIFT_RSTn, kFC4_RST_SHIFT_RSTn, \
            kFC5_RST_SHIFT_RSTn, kFC6_RST_SHIFT_RSTn, kFC7_RST_SHIFT_RSTn, kHSLSPI_RST_SHIFT_RSTn                \
    } /* Reset bits for FLEXCOMM peripheral */
#define GINT_RSTS                                  \
    {                                              \
        kGINT_RST_SHIFT_RSTn, kGINT_RST_SHIFT_RSTn \
    } /* Reset bits for GINT peripheral. GINT0 & GINT1 share same slot */
#define GPIO_RSTS_N                                                                                 \
    {                                                                                               \
        kGPIO0_RST_SHIFT_RSTn, kGPIO1_RST_SHIFT_RSTn, kGPIO2_RST_SHIFT_RSTn, kGPIO3_RST_SHIFT_RSTn, \
            kGPIO4_RST_SHIFT_RSTn, kGPIO5_RST_SHIFT_RSTn                                            \
    } /* Reset bits for GPIO peripheral */
#define INPUTMUX_RSTS                              \
    {                                              \
        kMUX0_RST_SHIFT_RSTn, kMUX1_RST_SHIFT_RSTn \
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
#define OTP_RSTS            \
    {                       \
        kOTP_RST_SHIFT_RSTn \
    } /* Reset bits for OTP peripheral */
#define PINT_RSTS            \
    {                        \
        kPINT_RST_SHIFT_RSTn \
    } /* Reset bits for PINT peripheral */
#define RNG_RSTS            \
    {                       \
        kRNG_RST_SHIFT_RSTn \
    } /* Reset bits for RNG peripheral */
#define SDIO_RST             \
    {                        \
        kSDIO_RST_SHIFT_RSTn \
    } /* Reset bits for SDIO peripheral */
#define SCT_RSTS             \
    {                        \
        kSCT0_RST_SHIFT_RSTn \
    } /* Reset bits for SCT peripheral */
#define SPIFI_RSTS            \
    {                         \
        kSPIFI_RST_SHIFT_RSTn \
    } /* Reset bits for SPIFI peripheral */
#define USB0D_RST             \
    {                         \
        kUSB0D_RST_SHIFT_RSTn \
    } /* Reset bits for USB0D peripheral */
#define USB0HMR_RST             \
    {                           \
        kUSB0HMR_RST_SHIFT_RSTn \
    } /* Reset bits for USB0HMR peripheral */
#define USB0HSL_RST             \
    {                           \
        kUSB0HSL_RST_SHIFT_RSTn \
    } /* Reset bits for USB0HSL peripheral */
#define USB1H_RST             \
    {                         \
        kUSB1H_RST_SHIFT_RSTn \
    } /* Reset bits for USB1H peripheral */
#define USB1D_RST             \
    {                         \
        kUSB1D_RST_SHIFT_RSTn \
    } /* Reset bits for USB1D peripheral */
#define USB1RAM_RST             \
    {                           \
        kUSB1RAM_RST_SHIFT_RSTn \
    } /* Reset bits for USB1RAM peripheral */
#define UTICK_RSTS            \
    {                         \
        kUTICK_RST_SHIFT_RSTn \
    } /* Reset bits for UTICK peripheral */
#define WWDT_RSTS            \
    {                        \
        kWWDT_RST_SHIFT_RSTn \
    } /* Reset bits for WWDT peripheral */
#define CAPT_RSTS_N          \
    {                        \
        kCAP0_RST_SHIFT_RSTn \
    } /* Reset bits for CAPT peripheral */
#define PLU_RSTS_N             \
    {                          \
        kPLULUT_RST_SHIFT_RSTn \
    } /* Reset bits for PLU peripheral */
#define OSTIMER_RSTS             \
    {                          \
        kOSTIMER0_RST_SHIFT_RSTn \
    } /* Reset bits for OSTIMER peripheral */
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

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FSL_RESET_H_ */
