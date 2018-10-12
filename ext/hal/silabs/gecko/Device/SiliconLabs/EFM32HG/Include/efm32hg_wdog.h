/**************************************************************************//**
 * @file efm32hg_wdog.h
 * @brief EFM32HG_WDOG register and bit field definitions
 * @version 5.1.2
 ******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/
/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/
/**************************************************************************//**
 * @defgroup EFM32HG_WDOG
 * @{
 * @brief EFM32HG_WDOG Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;     /**< Control Register  */
  __IOM uint32_t CMD;      /**< Command Register  */

  __IM uint32_t  SYNCBUSY; /**< Synchronization Busy Register  */
} WDOG_TypeDef;            /** @} */

/**************************************************************************//**
 * @defgroup EFM32HG_WDOG_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for WDOG CTRL */
#define _WDOG_CTRL_RESETVALUE            0x00000F00UL                         /**< Default value for WDOG_CTRL */
#define _WDOG_CTRL_MASK                  0x00003F7FUL                         /**< Mask for WDOG_CTRL */
#define WDOG_CTRL_EN                     (0x1UL << 0)                         /**< Watchdog Timer Enable */
#define _WDOG_CTRL_EN_SHIFT              0                                    /**< Shift value for WDOG_EN */
#define _WDOG_CTRL_EN_MASK               0x1UL                                /**< Bit mask for WDOG_EN */
#define _WDOG_CTRL_EN_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EN_DEFAULT             (_WDOG_CTRL_EN_DEFAULT << 0)         /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_DEBUGRUN               (0x1UL << 1)                         /**< Debug Mode Run Enable */
#define _WDOG_CTRL_DEBUGRUN_SHIFT        1                                    /**< Shift value for WDOG_DEBUGRUN */
#define _WDOG_CTRL_DEBUGRUN_MASK         0x2UL                                /**< Bit mask for WDOG_DEBUGRUN */
#define _WDOG_CTRL_DEBUGRUN_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_DEBUGRUN_DEFAULT       (_WDOG_CTRL_DEBUGRUN_DEFAULT << 1)   /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM2RUN                 (0x1UL << 2)                         /**< Energy Mode 2 Run Enable */
#define _WDOG_CTRL_EM2RUN_SHIFT          2                                    /**< Shift value for WDOG_EM2RUN */
#define _WDOG_CTRL_EM2RUN_MASK           0x4UL                                /**< Bit mask for WDOG_EM2RUN */
#define _WDOG_CTRL_EM2RUN_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM2RUN_DEFAULT         (_WDOG_CTRL_EM2RUN_DEFAULT << 2)     /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM3RUN                 (0x1UL << 3)                         /**< Energy Mode 3 Run Enable */
#define _WDOG_CTRL_EM3RUN_SHIFT          3                                    /**< Shift value for WDOG_EM3RUN */
#define _WDOG_CTRL_EM3RUN_MASK           0x8UL                                /**< Bit mask for WDOG_EM3RUN */
#define _WDOG_CTRL_EM3RUN_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM3RUN_DEFAULT         (_WDOG_CTRL_EM3RUN_DEFAULT << 3)     /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_LOCK                   (0x1UL << 4)                         /**< Configuration lock */
#define _WDOG_CTRL_LOCK_SHIFT            4                                    /**< Shift value for WDOG_LOCK */
#define _WDOG_CTRL_LOCK_MASK             0x10UL                               /**< Bit mask for WDOG_LOCK */
#define _WDOG_CTRL_LOCK_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_LOCK_DEFAULT           (_WDOG_CTRL_LOCK_DEFAULT << 4)       /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM4BLOCK               (0x1UL << 5)                         /**< Energy Mode 4 Block */
#define _WDOG_CTRL_EM4BLOCK_SHIFT        5                                    /**< Shift value for WDOG_EM4BLOCK */
#define _WDOG_CTRL_EM4BLOCK_MASK         0x20UL                               /**< Bit mask for WDOG_EM4BLOCK */
#define _WDOG_CTRL_EM4BLOCK_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM4BLOCK_DEFAULT       (_WDOG_CTRL_EM4BLOCK_DEFAULT << 5)   /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_SWOSCBLOCK             (0x1UL << 6)                         /**< Software Oscillator Disable Block */
#define _WDOG_CTRL_SWOSCBLOCK_SHIFT      6                                    /**< Shift value for WDOG_SWOSCBLOCK */
#define _WDOG_CTRL_SWOSCBLOCK_MASK       0x40UL                               /**< Bit mask for WDOG_SWOSCBLOCK */
#define _WDOG_CTRL_SWOSCBLOCK_DEFAULT    0x00000000UL                         /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_SWOSCBLOCK_DEFAULT     (_WDOG_CTRL_SWOSCBLOCK_DEFAULT << 6) /**< Shifted mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_PERSEL_SHIFT          8                                    /**< Shift value for WDOG_PERSEL */
#define _WDOG_CTRL_PERSEL_MASK           0xF00UL                              /**< Bit mask for WDOG_PERSEL */
#define _WDOG_CTRL_PERSEL_DEFAULT        0x0000000FUL                         /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_PERSEL_DEFAULT         (_WDOG_CTRL_PERSEL_DEFAULT << 8)     /**< Shifted mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_CLKSEL_SHIFT          12                                   /**< Shift value for WDOG_CLKSEL */
#define _WDOG_CTRL_CLKSEL_MASK           0x3000UL                             /**< Bit mask for WDOG_CLKSEL */
#define _WDOG_CTRL_CLKSEL_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_CLKSEL_ULFRCO         0x00000000UL                         /**< Mode ULFRCO for WDOG_CTRL */
#define _WDOG_CTRL_CLKSEL_LFRCO          0x00000001UL                         /**< Mode LFRCO for WDOG_CTRL */
#define _WDOG_CTRL_CLKSEL_LFXO           0x00000002UL                         /**< Mode LFXO for WDOG_CTRL */
#define WDOG_CTRL_CLKSEL_DEFAULT         (_WDOG_CTRL_CLKSEL_DEFAULT << 12)    /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_CLKSEL_ULFRCO          (_WDOG_CTRL_CLKSEL_ULFRCO << 12)     /**< Shifted mode ULFRCO for WDOG_CTRL */
#define WDOG_CTRL_CLKSEL_LFRCO           (_WDOG_CTRL_CLKSEL_LFRCO << 12)      /**< Shifted mode LFRCO for WDOG_CTRL */
#define WDOG_CTRL_CLKSEL_LFXO            (_WDOG_CTRL_CLKSEL_LFXO << 12)       /**< Shifted mode LFXO for WDOG_CTRL */

/* Bit fields for WDOG CMD */
#define _WDOG_CMD_RESETVALUE             0x00000000UL                     /**< Default value for WDOG_CMD */
#define _WDOG_CMD_MASK                   0x00000001UL                     /**< Mask for WDOG_CMD */
#define WDOG_CMD_CLEAR                   (0x1UL << 0)                     /**< Watchdog Timer Clear */
#define _WDOG_CMD_CLEAR_SHIFT            0                                /**< Shift value for WDOG_CLEAR */
#define _WDOG_CMD_CLEAR_MASK             0x1UL                            /**< Bit mask for WDOG_CLEAR */
#define _WDOG_CMD_CLEAR_DEFAULT          0x00000000UL                     /**< Mode DEFAULT for WDOG_CMD */
#define _WDOG_CMD_CLEAR_UNCHANGED        0x00000000UL                     /**< Mode UNCHANGED for WDOG_CMD */
#define _WDOG_CMD_CLEAR_CLEARED          0x00000001UL                     /**< Mode CLEARED for WDOG_CMD */
#define WDOG_CMD_CLEAR_DEFAULT           (_WDOG_CMD_CLEAR_DEFAULT << 0)   /**< Shifted mode DEFAULT for WDOG_CMD */
#define WDOG_CMD_CLEAR_UNCHANGED         (_WDOG_CMD_CLEAR_UNCHANGED << 0) /**< Shifted mode UNCHANGED for WDOG_CMD */
#define WDOG_CMD_CLEAR_CLEARED           (_WDOG_CMD_CLEAR_CLEARED << 0)   /**< Shifted mode CLEARED for WDOG_CMD */

/* Bit fields for WDOG SYNCBUSY */
#define _WDOG_SYNCBUSY_RESETVALUE        0x00000000UL                       /**< Default value for WDOG_SYNCBUSY */
#define _WDOG_SYNCBUSY_MASK              0x00000003UL                       /**< Mask for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_CTRL               (0x1UL << 0)                       /**< CTRL Register Busy */
#define _WDOG_SYNCBUSY_CTRL_SHIFT        0                                  /**< Shift value for WDOG_CTRL */
#define _WDOG_SYNCBUSY_CTRL_MASK         0x1UL                              /**< Bit mask for WDOG_CTRL */
#define _WDOG_SYNCBUSY_CTRL_DEFAULT      0x00000000UL                       /**< Mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_CTRL_DEFAULT       (_WDOG_SYNCBUSY_CTRL_DEFAULT << 0) /**< Shifted mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_CMD                (0x1UL << 1)                       /**< CMD Register Busy */
#define _WDOG_SYNCBUSY_CMD_SHIFT         1                                  /**< Shift value for WDOG_CMD */
#define _WDOG_SYNCBUSY_CMD_MASK          0x2UL                              /**< Bit mask for WDOG_CMD */
#define _WDOG_SYNCBUSY_CMD_DEFAULT       0x00000000UL                       /**< Mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_CMD_DEFAULT        (_WDOG_SYNCBUSY_CMD_DEFAULT << 1)  /**< Shifted mode DEFAULT for WDOG_SYNCBUSY */

/** @} End of group EFM32HG_WDOG */
/** @} End of group Parts */

