/***************************************************************************//**
 * @file em_cmu.h
 * @brief Clock management unit (CMU) API
 * @version 5.1.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/
#ifndef EM_CMU_H
#define EM_CMU_H

#include "em_device.h"
#if defined( CMU_PRESENT )

#include <stdbool.h>
#include "em_assert.h"
#include "em_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CMU
 * @{
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/* Select register id's, for internal use. */
#define CMU_NOSEL_REG              0
#define CMU_HFCLKSEL_REG           1
#define CMU_LFACLKSEL_REG          2
#define CMU_LFBCLKSEL_REG          3
#define CMU_LFCCLKSEL_REG          4
#define CMU_LFECLKSEL_REG          5
#define CMU_DBGCLKSEL_REG          6
#define CMU_USBCCLKSEL_REG         7

#define CMU_SEL_REG_POS            0
#define CMU_SEL_REG_MASK           0xf

/* Divisor/prescaler register id's, for internal use. */
#define CMU_NODIV_REG              0
#define CMU_NOPRESC_REG            0
#define CMU_HFPRESC_REG            1
#define CMU_HFCLKDIV_REG           1
#define CMU_HFEXPPRESC_REG         2
#define CMU_HFCLKLEPRESC_REG       3
#define CMU_HFPERPRESC_REG         4
#define CMU_HFPERCLKDIV_REG        4
#define CMU_HFCOREPRESC_REG        5
#define CMU_HFCORECLKDIV_REG       5
#define CMU_LFAPRESC0_REG          6
#define CMU_LFBPRESC0_REG          7
#define CMU_LFEPRESC0_REG          8

#define CMU_PRESC_REG_POS          4
#define CMU_DIV_REG_POS            CMU_PRESC_REG_POS
#define CMU_PRESC_REG_MASK         0xf
#define CMU_DIV_REG_MASK           CMU_PRESC_REG_MASK

/* Enable register id's, for internal use. */
#define CMU_NO_EN_REG              0
#define CMU_CTRL_EN_REG            1
#define CMU_HFPERCLKDIV_EN_REG     1
#define CMU_HFPERCLKEN0_EN_REG     2
#define CMU_HFCORECLKEN0_EN_REG    3
#define CMU_HFBUSCLKEN0_EN_REG     5
#define CMU_LFACLKEN0_EN_REG       6
#define CMU_LFBCLKEN0_EN_REG       7
#define CMU_LFCCLKEN0_EN_REG       8
#define CMU_LFECLKEN0_EN_REG       9
#define CMU_PCNT_EN_REG            10

#define CMU_EN_REG_POS             8
#define CMU_EN_REG_MASK            0xf

/* Enable register bit positions, for internal use. */
#define CMU_EN_BIT_POS             12
#define CMU_EN_BIT_MASK            0x1f

/* Clock branch bitfield positions, for internal use. */
#define CMU_HF_CLK_BRANCH          0
#define CMU_HFCORE_CLK_BRANCH      1
#define CMU_HFPER_CLK_BRANCH       2
#define CMU_HFBUS_CLK_BRANCH       4
#define CMU_HFEXP_CLK_BRANCH       5
#define CMU_DBG_CLK_BRANCH         6
#define CMU_AUX_CLK_BRANCH         7
#define CMU_RTC_CLK_BRANCH         8
#define CMU_RTCC_CLK_BRANCH        9
#define CMU_LETIMER0_CLK_BRANCH    10
#define CMU_LEUART0_CLK_BRANCH     11
#define CMU_LEUART1_CLK_BRANCH     12
#define CMU_LFA_CLK_BRANCH         13
#define CMU_LFB_CLK_BRANCH         14
#define CMU_LFC_CLK_BRANCH         15
#define CMU_LFE_CLK_BRANCH         16
#define CMU_USBC_CLK_BRANCH        17
#define CMU_USBLE_CLK_BRANCH       18
#define CMU_LCDPRE_CLK_BRANCH      19
#define CMU_LCD_CLK_BRANCH         20
#define CMU_LESENSE_CLK_BRANCH     21
#define CMU_CSEN_LF_CLK_BRANCH     22

#define CMU_CLK_BRANCH_POS         17
#define CMU_CLK_BRANCH_MASK        0x1f

#if defined( _EMU_CMD_EM01VSCALE0_MASK )
/* Max clock frequency for VSCALE voltages */
#define CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX     20000000
#endif
/** @endcond */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Clock divisors. These values are valid for prescalers. */
#define cmuClkDiv_1     1     /**< Divide clock by 1. */
#define cmuClkDiv_2     2     /**< Divide clock by 2. */
#define cmuClkDiv_4     4     /**< Divide clock by 4. */
#define cmuClkDiv_8     8     /**< Divide clock by 8. */
#define cmuClkDiv_16    16    /**< Divide clock by 16. */
#define cmuClkDiv_32    32    /**< Divide clock by 32. */
#define cmuClkDiv_64    64    /**< Divide clock by 64. */
#define cmuClkDiv_128   128   /**< Divide clock by 128. */
#define cmuClkDiv_256   256   /**< Divide clock by 256. */
#define cmuClkDiv_512   512   /**< Divide clock by 512. */
#define cmuClkDiv_1024  1024  /**< Divide clock by 1024. */
#define cmuClkDiv_2048  2048  /**< Divide clock by 2048. */
#define cmuClkDiv_4096  4096  /**< Divide clock by 4096. */
#define cmuClkDiv_8192  8192  /**< Divide clock by 8192. */
#define cmuClkDiv_16384 16384 /**< Divide clock by 16384. */
#define cmuClkDiv_32768 32768 /**< Divide clock by 32768. */

/** Clock divider configuration */
typedef uint32_t CMU_ClkDiv_TypeDef;

#if defined( _SILICON_LABS_32B_SERIES_1 )
/** Clockprescaler configuration */
typedef uint32_t CMU_ClkPresc_TypeDef;
#endif

#if defined( _CMU_HFRCOCTRL_BAND_MASK )
/** High frequency system RCO bands */
typedef enum
{
  cmuHFRCOBand_1MHz  = _CMU_HFRCOCTRL_BAND_1MHZ,      /**< 1MHz HFRCO band  */
  cmuHFRCOBand_7MHz  = _CMU_HFRCOCTRL_BAND_7MHZ,      /**< 7MHz HFRCO band  */
  cmuHFRCOBand_11MHz = _CMU_HFRCOCTRL_BAND_11MHZ,     /**< 11MHz HFRCO band */
  cmuHFRCOBand_14MHz = _CMU_HFRCOCTRL_BAND_14MHZ,     /**< 14MHz HFRCO band */
  cmuHFRCOBand_21MHz = _CMU_HFRCOCTRL_BAND_21MHZ,     /**< 21MHz HFRCO band */
#if defined( CMU_HFRCOCTRL_BAND_28MHZ )
  cmuHFRCOBand_28MHz = _CMU_HFRCOCTRL_BAND_28MHZ,     /**< 28MHz HFRCO band */
#endif
} CMU_HFRCOBand_TypeDef;
#endif /* _CMU_HFRCOCTRL_BAND_MASK */

#if defined( _CMU_AUXHFRCOCTRL_BAND_MASK )
/** AUX High frequency RCO bands */
typedef enum
{
  cmuAUXHFRCOBand_1MHz  = _CMU_AUXHFRCOCTRL_BAND_1MHZ,  /**< 1MHz RC band  */
  cmuAUXHFRCOBand_7MHz  = _CMU_AUXHFRCOCTRL_BAND_7MHZ,  /**< 7MHz RC band  */
  cmuAUXHFRCOBand_11MHz = _CMU_AUXHFRCOCTRL_BAND_11MHZ, /**< 11MHz RC band */
  cmuAUXHFRCOBand_14MHz = _CMU_AUXHFRCOCTRL_BAND_14MHZ, /**< 14MHz RC band */
  cmuAUXHFRCOBand_21MHz = _CMU_AUXHFRCOCTRL_BAND_21MHZ, /**< 21MHz RC band */
#if defined( CMU_AUXHFRCOCTRL_BAND_28MHZ )
  cmuAUXHFRCOBand_28MHz = _CMU_AUXHFRCOCTRL_BAND_28MHZ, /**< 28MHz RC band */
#endif
} CMU_AUXHFRCOBand_TypeDef;
#endif

#if defined( _CMU_USHFRCOCONF_BAND_MASK )
/** USB High frequency RC bands. */
typedef enum
{
  /** 24MHz RC band. */
  cmuUSHFRCOBand_24MHz = _CMU_USHFRCOCONF_BAND_24MHZ,
  /** 48MHz RC band. */
  cmuUSHFRCOBand_48MHz = _CMU_USHFRCOCONF_BAND_48MHZ,
} CMU_USHFRCOBand_TypeDef;
#endif

#if defined( _CMU_HFRCOCTRL_FREQRANGE_MASK )
/** High frequency system RCO bands */
typedef enum
{
  cmuHFRCOFreq_1M0Hz            = 1000000U,             /**< 1MHz RC band   */
  cmuHFRCOFreq_2M0Hz            = 2000000U,             /**< 2MHz RC band   */
  cmuHFRCOFreq_4M0Hz            = 4000000U,             /**< 4MHz RC band   */
  cmuHFRCOFreq_7M0Hz            = 7000000U,             /**< 7MHz RC band   */
  cmuHFRCOFreq_13M0Hz           = 13000000U,            /**< 13MHz RC band  */
  cmuHFRCOFreq_16M0Hz           = 16000000U,            /**< 16MHz RC band  */
  cmuHFRCOFreq_19M0Hz           = 19000000U,            /**< 19MHz RC band  */
  cmuHFRCOFreq_26M0Hz           = 26000000U,            /**< 26MHz RC band  */
  cmuHFRCOFreq_32M0Hz           = 32000000U,            /**< 32MHz RC band  */
  cmuHFRCOFreq_38M0Hz           = 38000000U,            /**< 38MHz RC band  */
  cmuHFRCOFreq_UserDefined      = 0,
} CMU_HFRCOFreq_TypeDef;
#define CMU_HFRCO_MIN           cmuHFRCOFreq_1M0Hz
#define CMU_HFRCO_MAX           cmuHFRCOFreq_38M0Hz
#endif

#if defined( _CMU_AUXHFRCOCTRL_FREQRANGE_MASK )
/** AUX High frequency RCO bands */
typedef enum
{
  cmuAUXHFRCOFreq_1M0Hz         = 1000000U,             /**< 1MHz RC band   */
  cmuAUXHFRCOFreq_2M0Hz         = 2000000U,             /**< 2MHz RC band   */
  cmuAUXHFRCOFreq_4M0Hz         = 4000000U,             /**< 4MHz RC band   */
  cmuAUXHFRCOFreq_7M0Hz         = 7000000U,             /**< 7MHz RC band   */
  cmuAUXHFRCOFreq_13M0Hz        = 13000000U,            /**< 13MHz RC band  */
  cmuAUXHFRCOFreq_16M0Hz        = 16000000U,            /**< 16MHz RC band  */
  cmuAUXHFRCOFreq_19M0Hz        = 19000000U,            /**< 19MHz RC band  */
  cmuAUXHFRCOFreq_26M0Hz        = 26000000U,            /**< 26MHz RC band  */
  cmuAUXHFRCOFreq_32M0Hz        = 32000000U,            /**< 32MHz RC band  */
  cmuAUXHFRCOFreq_38M0Hz        = 38000000U,            /**< 38MHz RC band  */
  cmuAUXHFRCOFreq_UserDefined   = 0,
} CMU_AUXHFRCOFreq_TypeDef;
#define CMU_AUXHFRCO_MIN        cmuAUXHFRCOFreq_1M0Hz
#define CMU_AUXHFRCO_MAX        cmuAUXHFRCOFreq_38M0Hz
#endif


/** Clock points in CMU. Please refer to CMU overview in reference manual. */
typedef enum
{
  /*******************/
  /* HF clock branch */
  /*******************/

  /** High frequency clock */
#if defined( _CMU_CTRL_HFCLKDIV_MASK ) \
    || defined( _CMU_HFPRESC_MASK )
  cmuClock_HF = (CMU_HFCLKDIV_REG << CMU_DIV_REG_POS)
                | (CMU_HFCLKSEL_REG << CMU_SEL_REG_POS)
                | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                | (0 << CMU_EN_BIT_POS)
                | (CMU_HF_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#else
  cmuClock_HF = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                | (CMU_HFCLKSEL_REG << CMU_SEL_REG_POS)
                | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                | (0 << CMU_EN_BIT_POS)
                | (CMU_HF_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /** Debug clock */
  cmuClock_DBG = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_DBGCLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_DBG_CLK_BRANCH << CMU_CLK_BRANCH_POS),

  /** AUX clock */
  cmuClock_AUX = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_AUX_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined( _CMU_HFEXPPRESC_MASK )
  /**********************/
  /* HF export sub-branch */
  /**********************/

  /** Export clock */
  cmuClock_EXPORT = (CMU_HFEXPPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                    | (0 << CMU_EN_BIT_POS)
                    | (CMU_HFEXP_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( _CMU_HFBUSCLKEN0_MASK )
/**********************************/
  /* HF bus clock sub-branch */
  /**********************************/

  /** High frequency bus clock. */
  cmuClock_BUS = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined( CMU_HFBUSCLKEN0_CRYPTO )
  /** Cryptography accelerator clock. */
  cmuClock_CRYPTO = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFBUSCLKEN0_CRYPTO_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFBUSCLKEN0_CRYPTO0 )
  /** Cryptography accelerator 0 clock. */
  cmuClock_CRYPTO0 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFBUSCLKEN0_CRYPTO0_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFBUSCLKEN0_CRYPTO1 )
  /** Cryptography accelerator 1 clock. */
  cmuClock_CRYPTO1 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFBUSCLKEN0_CRYPTO1_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFBUSCLKEN0_LDMA )
  /** Direct memory access controller clock. */
  cmuClock_LDMA = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFBUSCLKEN0_LDMA_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFBUSCLKEN0_GPCRC )
  /** General purpose cyclic redundancy checksum clock. */
  cmuClock_GPCRC = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFBUSCLKEN0_GPCRC_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFBUSCLKEN0_GPIO )
  /** General purpose input/output clock. */
  cmuClock_GPIO = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFBUSCLKEN0_GPIO_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /** Low energy clock divided down from HFBUSCLK. */
  cmuClock_HFLE = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFBUSCLKEN0_LE_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined( CMU_HFBUSCLKEN0_PRS )
  /** Peripheral reflex system clock. */
  cmuClock_PRS = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFBUSCLKEN0_PRS_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#endif

  /**********************************/
  /* HF peripheral clock sub-branch */
  /**********************************/

  /** High frequency peripheral clock */
#if defined( _CMU_HFPRESC_MASK )
  cmuClock_HFPER = (CMU_HFPERPRESC_REG << CMU_PRESC_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_CTRL_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_CTRL_HFPERCLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#else
  cmuClock_HFPER = (CMU_HFPERCLKDIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKDIV_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKDIV_HFPERCLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_USART0 )
  /** Universal sync/async receiver/transmitter 0 clock. */
  cmuClock_USART0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART0_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_USARTRF0 )
  /** Universal sync/async receiver/transmitter 0 clock. */
  cmuClock_USARTRF0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                      | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                      | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                      | (_CMU_HFPERCLKEN0_USARTRF0_SHIFT << CMU_EN_BIT_POS)
                      | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_USARTRF1 )
  /** Universal sync/async receiver/transmitter 0 clock. */
  cmuClock_USARTRF1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                      | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                      | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                      | (_CMU_HFPERCLKEN0_USARTRF1_SHIFT << CMU_EN_BIT_POS)
                      | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_USART1 )
  /** Universal sync/async receiver/transmitter 1 clock. */
  cmuClock_USART1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART1_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_USART2 )
  /** Universal sync/async receiver/transmitter 2 clock. */
  cmuClock_USART2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART2_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_USART3 )
  /** Universal sync/async receiver/transmitter 3 clock. */
  cmuClock_USART3 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART3_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_USART4 )
  /** Universal sync/async receiver/transmitter 4 clock. */
  cmuClock_USART4 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART4_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_USART5 )
  /** Universal sync/async receiver/transmitter 5 clock. */
  cmuClock_USART5 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART5_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif


#if defined( CMU_HFPERCLKEN0_UART0 )
  /** Universal async receiver/transmitter 0 clock. */
  cmuClock_UART0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_UART0_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_UART1 )
  /** Universal async receiver/transmitter 1 clock. */
  cmuClock_UART1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_UART1_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_TIMER0 )
  /** Timer 0 clock. */
  cmuClock_TIMER0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER0_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_TIMER1 )
  /** Timer 1 clock. */
  cmuClock_TIMER1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER1_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_TIMER2 )
  /** Timer 2 clock. */
  cmuClock_TIMER2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER2_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_TIMER3 )
  /** Timer 3 clock. */
  cmuClock_TIMER3 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER3_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_WTIMER0 )
  /** Wide Timer 0 clock. */
  cmuClock_WTIMER0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_WTIMER0_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_WTIMER1 )
  /** Wide Timer 1 clock. */
  cmuClock_WTIMER1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_WTIMER1_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_CRYOTIMER )
  /** CRYOtimer clock. */
  cmuClock_CRYOTIMER = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                       | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                       | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                       | (_CMU_HFPERCLKEN0_CRYOTIMER_SHIFT << CMU_EN_BIT_POS)
                       | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_ACMP0 )
  /** Analog comparator 0 clock. */
  cmuClock_ACMP0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_ACMP0_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_ACMP1 )
  /** Analog comparator 1 clock. */
  cmuClock_ACMP1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_ACMP1_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_PRS )
  /** Peripheral reflex system clock. */
  cmuClock_PRS = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFPERCLKEN0_PRS_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_DAC0 )
  /** Digital to analog converter 0 clock. */
  cmuClock_DAC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_DAC0_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_VDAC0 )
  /** Voltage digital to analog converter 0 clock. */
  cmuClock_VDAC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_VDAC0_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_IDAC0 )
  /** Current digital to analog converter 0 clock. */
  cmuClock_IDAC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_IDAC0_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_GPIO )
  /** General purpose input/output clock. */
  cmuClock_GPIO = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_GPIO_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_VCMP )
  /** Voltage comparator clock. */
  cmuClock_VCMP = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_VCMP_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_ADC0 )
  /** Analog to digital converter 0 clock. */
  cmuClock_ADC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_ADC0_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_I2C0 )
  /** I2C 0 clock. */
  cmuClock_I2C0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_I2C0_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_I2C1 )
  /** I2C 1 clock. */
  cmuClock_I2C1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_I2C1_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_I2C2 )
  /** I2C 2 clock. */
  cmuClock_I2C2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_I2C2_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_CSEN )
  /** Capacitive Sense HF clock */
  cmuClock_CSEN_HF = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_CSEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFPERCLKEN0_TRNG0 )
  /** True random number generator clock */
  cmuClock_TRNG0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_TRNG0_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /**********************/
  /* HF core sub-branch */
  /**********************/

  /** Core clock */
  cmuClock_CORE = (CMU_HFCORECLKDIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                  | (0 << CMU_EN_BIT_POS)
                  | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined( CMU_HFCORECLKEN0_AES )
  /** Advanced encryption standard accelerator clock. */
  cmuClock_AES = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFCORECLKEN0_AES_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFCORECLKEN0_DMA )
  /** Direct memory access controller clock. */
  cmuClock_DMA = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFCORECLKEN0_DMA_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFCORECLKEN0_LE )
  /** Low energy clock divided down from HFCORECLK. */
  cmuClock_HFLE = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFCORECLKEN0_LE_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFCORECLKEN0_EBI )
  /** External bus interface clock. */
  cmuClock_EBI = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFCORECLKEN0_EBI_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_HFCORECLKEN0_USBC )
  /** USB Core clock. */
  cmuClock_USBC = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_USBCCLKSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFCORECLKEN0_USBC_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_USBC_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#endif

#if defined( CMU_HFCORECLKEN0_USB )
  /** USB clock. */
  cmuClock_USB = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFCORECLKEN0_USB_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /***************/
  /* LF A branch */
  /***************/

  /** Low frequency A clock */
  cmuClock_LFA = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_LFACLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined( CMU_LFACLKEN0_RTC )
  /** Real time counter clock. */
  cmuClock_RTC = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_LFACLKEN0_RTC_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_RTC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_LFACLKEN0_LETIMER0 )
  /** Low energy timer 0 clock. */
  cmuClock_LETIMER0 = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                      | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                      | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                      | (_CMU_LFACLKEN0_LETIMER0_SHIFT << CMU_EN_BIT_POS)
                      | (CMU_LETIMER0_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_LFACLKEN0_LCD )
  /** Liquid crystal display, pre FDIV clock. */
  cmuClock_LCDpre = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                    | (0 << CMU_EN_BIT_POS)
                    | (CMU_LCDPRE_CLK_BRANCH << CMU_CLK_BRANCH_POS),

  /** Liquid crystal display clock. Please notice that FDIV prescaler
   * must be set by special API. */
  cmuClock_LCD = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_LFACLKEN0_LCD_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_LCD_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_PCNTCTRL_PCNT0CLKEN )
  /** Pulse counter 0 clock. */
  cmuClock_PCNT0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_PCNT_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_PCNTCTRL_PCNT0CLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_PCNTCTRL_PCNT1CLKEN )
  /** Pulse counter 1 clock. */
  cmuClock_PCNT1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_PCNT_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_PCNTCTRL_PCNT1CLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_PCNTCTRL_PCNT2CLKEN )
  /** Pulse counter 2 clock. */
  cmuClock_PCNT2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_PCNT_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_PCNTCTRL_PCNT2CLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#if defined( CMU_LFACLKEN0_LESENSE )
  /** LESENSE clock. */
  cmuClock_LESENSE = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFACLKEN0_LESENSE_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_LESENSE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /***************/
  /* LF B branch */
  /***************/

  /** Low frequency B clock */
  cmuClock_LFB = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_LFBCLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_LFB_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined( CMU_LFBCLKEN0_LEUART0 )
  /** Low energy universal asynchronous receiver/transmitter 0 clock. */
  cmuClock_LEUART0 = (CMU_LFBPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFBCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFBCLKEN0_LEUART0_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_LEUART0_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_LFBCLKEN0_CSEN )
  /** Capacitive Sense LF clock. */
  cmuClock_CSEN_LF = (CMU_LFBPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFBCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFBCLKEN0_CSEN_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_CSEN_LF_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_LFBCLKEN0_LEUART1 )
  /** Low energy universal asynchronous receiver/transmitter 1 clock. */
  cmuClock_LEUART1 = (CMU_LFBPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFBCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFBCLKEN0_LEUART1_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_LEUART1_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( CMU_LFBCLKEN0_SYSTICK )
  /** Cortex SYSTICK LF clock. */
  cmuClock_SYSTICK = (CMU_LFBPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFBCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFBCLKEN0_SYSTICK_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_LEUART0_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined( _CMU_LFCCLKEN0_MASK )
  /***************/
  /* LF C branch */
  /***************/

  /** Low frequency C clock */
  cmuClock_LFC = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_LFCCLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_LFC_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined( CMU_LFCCLKEN0_USBLE )
  /** USB LE clock. */
  cmuClock_USBLE = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_LFCCLKSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_LFCCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_LFCCLKEN0_USBLE_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_USBLE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#endif

#if defined( _CMU_LFECLKEN0_MASK )
  /***************/
  /* LF E branch */
  /***************/

  /** Low frequency A clock */
  cmuClock_LFE = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_LFECLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_LFE_CLK_BRANCH << CMU_CLK_BRANCH_POS),

  /** Real time counter and calendar clock. */
#if defined ( CMU_LFECLKEN0_RTCC )
  cmuClock_RTCC = (CMU_LFEPRESC0_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_LFECLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_LFECLKEN0_RTCC_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_RTCC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#endif

} CMU_Clock_TypeDef;

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/* Deprecated CMU_Clock_TypeDef member */
#define cmuClock_CORELE cmuClock_HFLE
/** @endcond */


/** Oscillator types. */
typedef enum
{
  cmuOsc_LFXO,     /**< Low frequency crystal oscillator. */
  cmuOsc_LFRCO,    /**< Low frequency RC oscillator. */
  cmuOsc_HFXO,     /**< High frequency crystal oscillator. */
  cmuOsc_HFRCO,    /**< High frequency RC oscillator. */
  cmuOsc_AUXHFRCO, /**< Auxiliary high frequency RC oscillator. */
#if defined( _CMU_STATUS_USHFRCOENS_MASK )
  cmuOsc_USHFRCO,  /**< USB high frequency RC oscillator */
#endif
#if defined( CMU_LFCLKSEL_LFAE_ULFRCO ) || defined( CMU_LFACLKSEL_LFA_ULFRCO )
  cmuOsc_ULFRCO,   /**< Ultra low frequency RC oscillator. */
#endif
#if defined( _CMU_STATUS_PLFRCOENS_MASK )
  cmuOsc_PLFRCO,   /**< Precision Low Frequency Oscillator. */
#endif
} CMU_Osc_TypeDef;

/** Oscillator modes. */
typedef enum
{
  cmuOscMode_Crystal,   /**< Crystal oscillator. */
  cmuOscMode_AcCoupled, /**< AC coupled buffer. */
  cmuOscMode_External,  /**< External digital clock. */
} CMU_OscMode_TypeDef;

/** Selectable clock sources. */
typedef enum
{
  cmuSelect_Error,                      /**< Usage error. */
  cmuSelect_Disabled,                   /**< Clock selector disabled. */
  cmuSelect_LFXO,                       /**< Low frequency crystal oscillator. */
  cmuSelect_LFRCO,                      /**< Low frequency RC oscillator. */
  cmuSelect_HFXO,                       /**< High frequency crystal oscillator. */
  cmuSelect_HFRCO,                      /**< High frequency RC oscillator. */
  cmuSelect_HFCLKLE,                    /**< High frequency LE clock divided by 2 or 4. */
  cmuSelect_AUXHFRCO,                   /**< Auxilliary clock source can be used for debug clock */
  cmuSelect_HFCLK,                      /**< Divided HFCLK on Giant for debug clock, undivided on
                                             Tiny Gecko and for USBC (not used on Gecko) */
#if defined( CMU_STATUS_USHFRCOENS )
  cmuSelect_USHFRCO,                    /**< USB high frequency RC oscillator */
#endif
#if defined( CMU_CMD_HFCLKSEL_USHFRCODIV2 )
  cmuSelect_USHFRCODIV2,                /**< USB high frequency RC oscillator */
#endif
#if defined( CMU_LFCLKSEL_LFAE_ULFRCO ) || defined( CMU_LFACLKSEL_LFA_ULFRCO )
  cmuSelect_ULFRCO,                     /**< Ultra low frequency RC oscillator. */
#endif
#if defined( _CMU_STATUS_PLFRCOENS_MASK )
  cmuSelect_PLFRCO,                     /**< Precision Low Frequency Oscillator. */
#endif
} CMU_Select_TypeDef;

#if defined( CMU_HFCORECLKEN0_LE )
/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/* Deprecated CMU_Select_TypeDef member */
#define cmuSelect_CORELEDIV2    cmuSelect_HFCLKLE
/** @endcond */
#endif

#if defined( _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK )
/** HFXO tuning modes */
typedef enum
{
  cmuHFXOTuningMode_Auto               = 0,
  cmuHFXOTuningMode_ShuntCommand       = CMU_CMD_HFXOSHUNTOPTSTART,  /**< Run shunt current optimization only */
  cmuHFXOTuningMode_PeakShuntCommand   = CMU_CMD_HFXOPEAKDETSTART    /**< Run peak and shunt current optimization */
                                         | CMU_CMD_HFXOSHUNTOPTSTART,
} CMU_HFXOTuningMode_TypeDef;
#endif

#if defined( _CMU_CTRL_LFXOBOOST_MASK )
/** LFXO Boost values. */
typedef enum
{
  cmuLfxoBoost70         = 0x0,
  cmuLfxoBoost100        = 0x2,
#if defined( _EMU_AUXCTRL_REDLFXOBOOST_MASK )
  cmuLfxoBoost70Reduced  = 0x1,
  cmuLfxoBoost100Reduced = 0x3,
#endif
} CMU_LFXOBoost_TypeDef;
#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** LFXO initialization structure. Init values should be obtained from a configuration tool,
    app note or xtal datasheet  */
typedef struct
{
#if defined( _CMU_LFXOCTRL_MASK )
  uint8_t ctune;                        /**< CTUNE (load capacitance) value */
  uint8_t gain;                         /**< Gain / max startup margin */
#else
  CMU_LFXOBoost_TypeDef boost;          /**< LFXO boost */
#endif
  uint8_t timeout;                      /**< Startup delay */
  CMU_OscMode_TypeDef mode;             /**< Oscillator mode */
} CMU_LFXOInit_TypeDef;

#if defined( _CMU_LFXOCTRL_MASK )
/** Default LFXO initialization values for platform 2 devices which contain a
 *  separate LFXOCTRL register. */
#define CMU_LFXOINIT_DEFAULT                                                    \
  {                                                                             \
    _CMU_LFXOCTRL_TUNING_DEFAULT,   /* Default CTUNE value, 0 */                \
    _CMU_LFXOCTRL_GAIN_DEFAULT,     /* Default gain, 2 */                       \
    _CMU_LFXOCTRL_TIMEOUT_DEFAULT,  /* Default start-up delay, 32k cycles */    \
    cmuOscMode_Crystal,             /* Crystal oscillator */                    \
  }
#define CMU_LFXOINIT_EXTERNAL_CLOCK                                             \
  {                                                                             \
    0,                              /* No CTUNE value needed */                 \
    0,                              /* No LFXO startup gain */                  \
    _CMU_LFXOCTRL_TIMEOUT_2CYCLES,  /* Minimal lfxo start-up delay, 2 cycles */ \
    cmuOscMode_External,            /* External digital clock */                \
  }
#else
/** Default LFXO initialization values for platform 1 devices. */
#define CMU_LFXOINIT_DEFAULT                        \
  {                                                 \
    cmuLfxoBoost70,                                 \
    _CMU_CTRL_LFXOTIMEOUT_DEFAULT,                  \
    cmuOscMode_Crystal,                             \
  }
#define CMU_LFXOINIT_EXTERNAL_CLOCK                 \
  {                                                 \
    cmuLfxoBoost70,                                 \
    _CMU_CTRL_LFXOTIMEOUT_8CYCLES,                  \
    cmuOscMode_External,                            \
  }
#endif

/** HFXO initialization structure. Init values should be obtained from a configuration tool,
    app note or xtal datasheet  */
typedef struct
{
#if defined( _CMU_HFXOCTRL_MASK )
  bool lowPowerMode;                    /**< Enable low-power mode */
  bool autoStartEm01;                   /**< @deprecated Use @ref CMU_HFXOAutostartEnable instead. */
  bool autoSelEm01;                     /**< @deprecated Use @ref CMU_HFXOAutostartEnable instead. */
  bool autoStartSelOnRacWakeup;         /**< @deprecated Use @ref CMU_HFXOAutostartEnable instead. */
  uint16_t ctuneStartup;                /**< Startup phase CTUNE (load capacitance) value */
  uint16_t ctuneSteadyState;            /**< Steady-state phase CTUNE (load capacitance) value */
  uint8_t regIshSteadyState;            /**< Shunt steady-state current */
  uint8_t xoCoreBiasTrimStartup;        /**< Startup XO core bias current trim */
  uint8_t xoCoreBiasTrimSteadyState;    /**< Steady-state XO core bias current trim */
  uint8_t thresholdPeakDetect;          /**< Peak detection threshold */
  uint8_t timeoutShuntOptimization;     /**< Timeout - shunt optimization */
  uint8_t timeoutPeakDetect;            /**< Timeout - peak detection */
  uint8_t timeoutSteady;                /**< Timeout - steady-state */
  uint8_t timeoutStartup;               /**< Timeout - startup */
#else
  uint8_t boost;                        /**< HFXO Boost, 0=50% 1=70%, 2=80%, 3=100% */
  uint8_t timeout;                      /**< Startup delay */
  bool glitchDetector;                  /**< Enable/disable glitch detector */
#endif
  CMU_OscMode_TypeDef mode;             /**< Oscillator mode */
} CMU_HFXOInit_TypeDef;

#if defined( _CMU_HFXOCTRL_MASK )
/**
 * Default HFXO initialization values for Platform 2 devices which contain a
 * separate HFXOCTRL register.
 */
#if defined( _EFR_DEVICE )
#define CMU_HFXOINIT_DEFAULT                                                    \
{                                                                               \
  false,        /* Low-noise mode for EFR32 */                                  \
  false,        /* @deprecated no longer in use */                              \
  false,        /* @deprecated no longer in use */                              \
  false,        /* @deprecated no longer in use */                              \
  _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,                                           \
  _CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT,                                       \
  _CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT,                                      \
  0x20,         /* Matching errata fix in CHIP_Init() */                        \
  0x7,          /* Recommended steady-state XO core bias current */             \
  0x6,          /* Recommended peak detection threshold */                      \
  _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT,                                 \
  0xA,          /* Recommended peak detection timeout  */                       \
  0x4,          /* Recommended steady timeout */                                \
  _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,                                  \
  cmuOscMode_Crystal,                                                           \
}
#else /* EFM32 device */
#define CMU_HFXOINIT_DEFAULT                                                    \
{                                                                               \
  true,         /* Low-power mode for EFM32 */                                  \
  false,        /* @deprecated no longer in use */                              \
  false,        /* @deprecated no longer in use */                              \
  false,        /* @deprecated no longer in use */                              \
  _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,                                           \
  _CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT,                                       \
  _CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT,                                      \
  0x20,         /* Matching errata fix in CHIP_Init() */                        \
  0x7,          /* Recommended steady-state osc core bias current */            \
  0x6,          /* Recommended peak detection threshold */                      \
  _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT,                                 \
  0xA,          /* Recommended peak detection timeout  */                       \
  0x4,          /* Recommended steady timeout */                                \
  _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,                                  \
  cmuOscMode_Crystal,                                                           \
}
#endif /* _EFR_DEVICE */
#define CMU_HFXOINIT_EXTERNAL_CLOCK                                             \
{                                                                               \
  true,         /* Low-power mode */                                            \
  false,        /* @deprecated no longer in use */                              \
  false,        /* @deprecated no longer in use */                              \
  false,        /* @deprecated no longer in use */                              \
  0,            /* Startup CTUNE=0 recommended for external clock */            \
  0,            /* Steady  CTUNE=0 recommended for external clock */            \
  _CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT,                                      \
  0,            /* Startup IBTRIMXOCORE=0 recommended for external clock */     \
  0,            /* Steady  IBTRIMXOCORE=0 recommended for external clock */     \
  0x6,          /* Recommended peak detection threshold */                      \
  _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT,                                 \
  0x0,          /* Peak-detect not recommended for external clock usage */      \
  _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_2CYCLES,  /* Minimal steady timeout */     \
  _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_2CYCLES, /* Minimal startup timeout */    \
  cmuOscMode_External,                                                          \
}
#else /* _CMU_HFXOCTRL_MASK */
/**
 * Default HFXO initialization values for Platform 1 devices.
 */
#define CMU_HFXOINIT_DEFAULT                                           \
{                                                                      \
  _CMU_CTRL_HFXOBOOST_DEFAULT,   /* 100% HFXO boost */                 \
  _CMU_CTRL_HFXOTIMEOUT_DEFAULT, /* 16k startup delay */               \
  false,                         /* Disable glitch detector */         \
  cmuOscMode_Crystal,            /* Crystal oscillator */              \
}
#define CMU_HFXOINIT_EXTERNAL_CLOCK                                    \
{                                                                      \
  0,                             /* Minimal HFXO boost, 50% */         \
  _CMU_CTRL_HFXOTIMEOUT_8CYCLES, /* Minimal startup delay, 8 cycles */ \
  false,                         /* Disable glitch detector */         \
  cmuOscMode_External,           /* External digital clock */          \
}
#endif /* _CMU_HFXOCTRL_MASK */


/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

#if defined( _CMU_AUXHFRCOCTRL_BAND_MASK )
CMU_AUXHFRCOBand_TypeDef  CMU_AUXHFRCOBandGet(void);
void                      CMU_AUXHFRCOBandSet(CMU_AUXHFRCOBand_TypeDef band);

#elif defined( _CMU_AUXHFRCOCTRL_FREQRANGE_MASK )
CMU_AUXHFRCOFreq_TypeDef  CMU_AUXHFRCOBandGet(void);
void                      CMU_AUXHFRCOBandSet(CMU_AUXHFRCOFreq_TypeDef setFreq);
#endif

uint32_t              CMU_Calibrate(uint32_t HFCycles, CMU_Osc_TypeDef reference);

#if defined( _CMU_CALCTRL_UPSEL_MASK ) && defined( _CMU_CALCTRL_DOWNSEL_MASK )
void                  CMU_CalibrateConfig(uint32_t downCycles, CMU_Osc_TypeDef downSel,
                                          CMU_Osc_TypeDef upSel);
#endif

uint32_t              CMU_CalibrateCountGet(void);
void                  CMU_ClockEnable(CMU_Clock_TypeDef clock, bool enable);
CMU_ClkDiv_TypeDef    CMU_ClockDivGet(CMU_Clock_TypeDef clock);
void                  CMU_ClockDivSet(CMU_Clock_TypeDef clock, CMU_ClkDiv_TypeDef div);
uint32_t              CMU_ClockFreqGet(CMU_Clock_TypeDef clock);

#if defined( _SILICON_LABS_32B_SERIES_1 )
void                  CMU_ClockPrescSet(CMU_Clock_TypeDef clock, uint32_t presc);
uint32_t              CMU_ClockPrescGet(CMU_Clock_TypeDef clock);
#endif

void                  CMU_ClockSelectSet(CMU_Clock_TypeDef clock, CMU_Select_TypeDef ref);
CMU_Select_TypeDef    CMU_ClockSelectGet(CMU_Clock_TypeDef clock);
void                  CMU_FreezeEnable(bool enable);

#if defined( _CMU_HFRCOCTRL_BAND_MASK )
CMU_HFRCOBand_TypeDef CMU_HFRCOBandGet(void);
void                  CMU_HFRCOBandSet(CMU_HFRCOBand_TypeDef band);

#elif defined( _CMU_HFRCOCTRL_FREQRANGE_MASK )
CMU_HFRCOFreq_TypeDef CMU_HFRCOBandGet(void);
void                  CMU_HFRCOBandSet(CMU_HFRCOFreq_TypeDef setFreq);
#endif

uint32_t              CMU_HFRCOStartupDelayGet(void);
void                  CMU_HFRCOStartupDelaySet(uint32_t delay);

#if defined( _CMU_HFXOCTRL_AUTOSTARTEM0EM1_MASK )
void                  CMU_HFXOAutostartEnable(uint32_t userSel,
                                              bool enEM0EM1Start,
                                              bool enEM0EM1StartSel);
#endif

void                  CMU_HFXOInit(const CMU_HFXOInit_TypeDef *hfxoInit);


uint32_t              CMU_LCDClkFDIVGet(void);
void                  CMU_LCDClkFDIVSet(uint32_t div);
void                  CMU_LFXOInit(const CMU_LFXOInit_TypeDef *lfxoInit);

void                  CMU_OscillatorEnable(CMU_Osc_TypeDef osc, bool enable, bool wait);
uint32_t              CMU_OscillatorTuningGet(CMU_Osc_TypeDef osc);
void                  CMU_OscillatorTuningSet(CMU_Osc_TypeDef osc, uint32_t val);

#if defined( _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK )
bool CMU_OscillatorTuningWait(CMU_Osc_TypeDef osc, CMU_HFXOTuningMode_TypeDef mode);
bool CMU_OscillatorTuningOptimize(CMU_Osc_TypeDef osc,
                                  CMU_HFXOTuningMode_TypeDef mode,
                                  bool wait);
#endif

bool                  CMU_PCNTClockExternalGet(unsigned int instance);
void                  CMU_PCNTClockExternalSet(unsigned int instance, bool external);

#if defined( _CMU_USHFRCOCONF_BAND_MASK )
CMU_USHFRCOBand_TypeDef   CMU_USHFRCOBandGet(void);
void                      CMU_USHFRCOBandSet(CMU_USHFRCOBand_TypeDef band);
#endif


#if defined( CMU_CALCTRL_CONT )
/***************************************************************************//**
 * @brief
 *   Configures continuous calibration mode
 * @param[in] enable
 *   If true, enables continuous calibration, if false disables continuous
 *   calibrartion
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateCont(bool enable)
{
  BUS_RegBitWrite(&(CMU->CALCTRL), _CMU_CALCTRL_CONT_SHIFT, enable);
}
#endif


/***************************************************************************//**
 * @brief
 *   Starts calibration
 * @note
 *   This call is usually invoked after CMU_CalibrateConfig() and possibly
 *   CMU_CalibrateCont()
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateStart(void)
{
  CMU->CMD = CMU_CMD_CALSTART;
}


#if defined( CMU_CMD_CALSTOP )
/***************************************************************************//**
 * @brief
 *   Stop the calibration counters
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateStop(void)
{
  CMU->CMD = CMU_CMD_CALSTOP;
}
#endif


/***************************************************************************//**
 * @brief
 *   Convert dividend to logarithmic value. Only works for even
 *   numbers equal to 2^n.
 *
 * @param[in] div
 *   Unscaled dividend.
 *
 * @return
 *   Logarithm of 2, as used by fixed prescalers.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_DivToLog2(CMU_ClkDiv_TypeDef div)
{
  uint32_t log2;

  /* Fixed 2^n prescalers take argument of 32768 or less. */
  EFM_ASSERT((div > 0U) && (div <= 32768U));

  /* Count leading zeroes and "reverse" result */
  log2 = (31U - __CLZ(div));

  return log2;
}


/***************************************************************************//**
 * @brief
 *   Clear one or more pending CMU interrupts.
 *
 * @param[in] flags
 *   CMU interrupt sources to clear.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntClear(uint32_t flags)
{
  CMU->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more CMU interrupts.
 *
 * @param[in] flags
 *   CMU interrupt sources to disable.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntDisable(uint32_t flags)
{
  CMU->IEN &= ~flags;
}


/***************************************************************************//**
 * @brief
 *   Enable one or more CMU interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using CMU_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] flags
 *   CMU interrupt sources to enable.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntEnable(uint32_t flags)
{
  CMU->IEN |= flags;
}


/***************************************************************************//**
 * @brief
 *   Get pending CMU interrupts.
 *
 * @return
 *   CMU interrupt sources pending.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_IntGet(void)
{
  return CMU->IF;
}


/***************************************************************************//**
 * @brief
 *   Get enabled and pending CMU interrupt flags.
 *
 * @details
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled CMU interrupt sources
 *   The return value is the bitwise AND of
 *   - the enabled interrupt sources in CMU_IEN and
 *   - the pending interrupt flags CMU_IF
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_IntGetEnabled(void)
{
  uint32_t ien;

  ien = CMU->IEN;
  return CMU->IF & ien;
}


/**************************************************************************//**
 * @brief
 *   Set one or more pending CMU interrupts.
 *
 * @param[in] flags
 *   CMU interrupt sources to set to pending.
 *****************************************************************************/
__STATIC_INLINE void CMU_IntSet(uint32_t flags)
{
  CMU->IFS = flags;
}


/***************************************************************************//**
 * @brief
 *   Lock the CMU in order to protect some of its registers against unintended
 *   modification.
 *
 * @details
 *   Please refer to the reference manual for CMU registers that will be
 *   locked.
 *
 * @note
 *   If locking the CMU registers, they must be unlocked prior to using any
 *   CMU API functions modifying CMU registers protected by the lock.
 ******************************************************************************/
__STATIC_INLINE void CMU_Lock(void)
{
  CMU->LOCK = CMU_LOCK_LOCKKEY_LOCK;
}


/***************************************************************************//**
 * @brief
 *   Convert logarithm of 2 prescaler to division factor.
 *
 * @param[in] log2
 *   Logarithm of 2, as used by fixed prescalers.
 *
 * @return
 *   Dividend.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_Log2ToDiv(uint32_t log2)
{
  return 1 << log2;
}


#if defined( _SILICON_LABS_32B_SERIES_1 )
/***************************************************************************//**
 * @brief
 *   Convert prescaler dividend to logarithmic value. Only works for even
 *   numbers equal to 2^n.
 *
 * @param[in] presc
 *   Unscaled dividend (dividend = presc + 1).
 *
 * @return
 *   Logarithm of 2, as used by fixed 2^n prescalers.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_PrescToLog2(CMU_ClkPresc_TypeDef presc)
{
  uint32_t log2;

  /* Integer prescalers take argument less than 32768. */
  EFM_ASSERT(presc < 32768U);

  /* Count leading zeroes and "reverse" result */
  log2 = (31U - __CLZ(presc + 1));

  /* Check that presc is a 2^n number */
  EFM_ASSERT(presc == (CMU_Log2ToDiv(log2) - 1));

  return log2;
}
#endif


/***************************************************************************//**
 * @brief
 *   Unlock the CMU so that writing to locked registers again is possible.
 ******************************************************************************/
__STATIC_INLINE void CMU_Unlock(void)
{
  CMU->LOCK = CMU_LOCK_LOCKKEY_UNLOCK;
}


#if defined( _CMU_HFRCOCTRL_FREQRANGE_MASK )
/***************************************************************************//**
 * @brief
 *   Get current HFRCO frequency.
 *
 * @deprecated
 *   Deprecated function. New code should use @ref CMU_HFRCOBandGet().
 *
 * @return
 *   HFRCO frequency
 ******************************************************************************/
__STATIC_INLINE CMU_HFRCOFreq_TypeDef CMU_HFRCOFreqGet(void)
{
  return CMU_HFRCOBandGet();
}


/***************************************************************************//**
 * @brief
 *   Set HFRCO calibration for the selected target frequency
 *
 * @deprecated
 *   Deprecated function. New code should use @ref CMU_HFRCOBandSet().
 *
 * @param[in] setFreq
 *   HFRCO frequency to set
 ******************************************************************************/
__STATIC_INLINE void CMU_HFRCOFreqSet(CMU_HFRCOFreq_TypeDef setFreq)
{
  CMU_HFRCOBandSet(setFreq);
}
#endif


#if defined( _CMU_AUXHFRCOCTRL_FREQRANGE_MASK )
/***************************************************************************//**
 * @brief
 *   Get current AUXHFRCO frequency.
 *
 * @deprecated
 *   Deprecated function. New code should use @ref CMU_AUXHFRCOBandGet().
 *
 * @return
 *   AUXHFRCO frequency
 ******************************************************************************/
__STATIC_INLINE CMU_AUXHFRCOFreq_TypeDef CMU_AUXHFRCOFreqGet(void)
{
  return CMU_AUXHFRCOBandGet();
}


/***************************************************************************//**
 * @brief
 *   Set AUXHFRCO calibration for the selected target frequency
 *
 * @deprecated
 *   Deprecated function. New code should use @ref CMU_AUXHFRCOBandSet().
 *
 * @param[in] setFreq
 *   AUXHFRCO frequency to set
 ******************************************************************************/
__STATIC_INLINE void CMU_AUXHFRCOFreqSet(CMU_AUXHFRCOFreq_TypeDef setFreq)
{
  CMU_AUXHFRCOBandSet(setFreq);
}
#endif

/** @} (end addtogroup CMU) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined( CMU_PRESENT ) */
#endif /* EM_CMU_H */
