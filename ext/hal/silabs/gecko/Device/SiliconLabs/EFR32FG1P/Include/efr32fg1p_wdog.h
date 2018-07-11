/**************************************************************************//**
 * @file efr32fg1p_wdog.h
 * @brief EFR32FG1P_WDOG register and bit field definitions
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
 * @defgroup EFR32FG1P_WDOG
 * @{
 * @brief EFR32FG1P_WDOG Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t   CTRL;         /**< Control Register  */
  __IOM uint32_t   CMD;          /**< Command Register  */

  __IM uint32_t    SYNCBUSY;     /**< Synchronization Busy Register  */

  WDOG_PCH_TypeDef PCH[2];       /**< PCH */

  uint32_t         RESERVED0[2]; /**< Reserved for future use **/
  __IM uint32_t    IF;           /**< Watchdog Interrupt Flags  */
  __IOM uint32_t   IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t   IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t   IEN;          /**< Interrupt Enable Register  */
} WDOG_TypeDef;                  /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_WDOG_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for WDOG CTRL */
#define _WDOG_CTRL_RESETVALUE                     0x00000F00UL                          /**< Default value for WDOG_CTRL */
#define _WDOG_CTRL_MASK                           0xC7033F7FUL                          /**< Mask for WDOG_CTRL */
#define WDOG_CTRL_EN                              (0x1UL << 0)                          /**< Watchdog Timer Enable */
#define _WDOG_CTRL_EN_SHIFT                       0                                     /**< Shift value for WDOG_EN */
#define _WDOG_CTRL_EN_MASK                        0x1UL                                 /**< Bit mask for WDOG_EN */
#define _WDOG_CTRL_EN_DEFAULT                     0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EN_DEFAULT                      (_WDOG_CTRL_EN_DEFAULT << 0)          /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_DEBUGRUN                        (0x1UL << 1)                          /**< Debug Mode Run Enable */
#define _WDOG_CTRL_DEBUGRUN_SHIFT                 1                                     /**< Shift value for WDOG_DEBUGRUN */
#define _WDOG_CTRL_DEBUGRUN_MASK                  0x2UL                                 /**< Bit mask for WDOG_DEBUGRUN */
#define _WDOG_CTRL_DEBUGRUN_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_DEBUGRUN_DEFAULT                (_WDOG_CTRL_DEBUGRUN_DEFAULT << 1)    /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM2RUN                          (0x1UL << 2)                          /**< Energy Mode 2 Run Enable */
#define _WDOG_CTRL_EM2RUN_SHIFT                   2                                     /**< Shift value for WDOG_EM2RUN */
#define _WDOG_CTRL_EM2RUN_MASK                    0x4UL                                 /**< Bit mask for WDOG_EM2RUN */
#define _WDOG_CTRL_EM2RUN_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM2RUN_DEFAULT                  (_WDOG_CTRL_EM2RUN_DEFAULT << 2)      /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM3RUN                          (0x1UL << 3)                          /**< Energy Mode 3 Run Enable */
#define _WDOG_CTRL_EM3RUN_SHIFT                   3                                     /**< Shift value for WDOG_EM3RUN */
#define _WDOG_CTRL_EM3RUN_MASK                    0x8UL                                 /**< Bit mask for WDOG_EM3RUN */
#define _WDOG_CTRL_EM3RUN_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM3RUN_DEFAULT                  (_WDOG_CTRL_EM3RUN_DEFAULT << 3)      /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_LOCK                            (0x1UL << 4)                          /**< Configuration lock */
#define _WDOG_CTRL_LOCK_SHIFT                     4                                     /**< Shift value for WDOG_LOCK */
#define _WDOG_CTRL_LOCK_MASK                      0x10UL                                /**< Bit mask for WDOG_LOCK */
#define _WDOG_CTRL_LOCK_DEFAULT                   0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_LOCK_DEFAULT                    (_WDOG_CTRL_LOCK_DEFAULT << 4)        /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM4BLOCK                        (0x1UL << 5)                          /**< Energy Mode 4 Block */
#define _WDOG_CTRL_EM4BLOCK_SHIFT                 5                                     /**< Shift value for WDOG_EM4BLOCK */
#define _WDOG_CTRL_EM4BLOCK_MASK                  0x20UL                                /**< Bit mask for WDOG_EM4BLOCK */
#define _WDOG_CTRL_EM4BLOCK_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_EM4BLOCK_DEFAULT                (_WDOG_CTRL_EM4BLOCK_DEFAULT << 5)    /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_SWOSCBLOCK                      (0x1UL << 6)                          /**< Software Oscillator Disable Block */
#define _WDOG_CTRL_SWOSCBLOCK_SHIFT               6                                     /**< Shift value for WDOG_SWOSCBLOCK */
#define _WDOG_CTRL_SWOSCBLOCK_MASK                0x40UL                                /**< Bit mask for WDOG_SWOSCBLOCK */
#define _WDOG_CTRL_SWOSCBLOCK_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_SWOSCBLOCK_DEFAULT              (_WDOG_CTRL_SWOSCBLOCK_DEFAULT << 6)  /**< Shifted mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_PERSEL_SHIFT                   8                                     /**< Shift value for WDOG_PERSEL */
#define _WDOG_CTRL_PERSEL_MASK                    0xF00UL                               /**< Bit mask for WDOG_PERSEL */
#define _WDOG_CTRL_PERSEL_DEFAULT                 0x0000000FUL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_PERSEL_DEFAULT                  (_WDOG_CTRL_PERSEL_DEFAULT << 8)      /**< Shifted mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_CLKSEL_SHIFT                   12                                    /**< Shift value for WDOG_CLKSEL */
#define _WDOG_CTRL_CLKSEL_MASK                    0x3000UL                              /**< Bit mask for WDOG_CLKSEL */
#define _WDOG_CTRL_CLKSEL_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_CLKSEL_ULFRCO                  0x00000000UL                          /**< Mode ULFRCO for WDOG_CTRL */
#define _WDOG_CTRL_CLKSEL_LFRCO                   0x00000001UL                          /**< Mode LFRCO for WDOG_CTRL */
#define _WDOG_CTRL_CLKSEL_LFXO                    0x00000002UL                          /**< Mode LFXO for WDOG_CTRL */
#define WDOG_CTRL_CLKSEL_DEFAULT                  (_WDOG_CTRL_CLKSEL_DEFAULT << 12)     /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_CLKSEL_ULFRCO                   (_WDOG_CTRL_CLKSEL_ULFRCO << 12)      /**< Shifted mode ULFRCO for WDOG_CTRL */
#define WDOG_CTRL_CLKSEL_LFRCO                    (_WDOG_CTRL_CLKSEL_LFRCO << 12)       /**< Shifted mode LFRCO for WDOG_CTRL */
#define WDOG_CTRL_CLKSEL_LFXO                     (_WDOG_CTRL_CLKSEL_LFXO << 12)        /**< Shifted mode LFXO for WDOG_CTRL */
#define _WDOG_CTRL_WARNSEL_SHIFT                  16                                    /**< Shift value for WDOG_WARNSEL */
#define _WDOG_CTRL_WARNSEL_MASK                   0x30000UL                             /**< Bit mask for WDOG_WARNSEL */
#define _WDOG_CTRL_WARNSEL_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_WARNSEL_DEFAULT                 (_WDOG_CTRL_WARNSEL_DEFAULT << 16)    /**< Shifted mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_WINSEL_SHIFT                   24                                    /**< Shift value for WDOG_WINSEL */
#define _WDOG_CTRL_WINSEL_MASK                    0x7000000UL                           /**< Bit mask for WDOG_WINSEL */
#define _WDOG_CTRL_WINSEL_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_WINSEL_DEFAULT                  (_WDOG_CTRL_WINSEL_DEFAULT << 24)     /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_CLRSRC                          (0x1UL << 30)                         /**< Watchdog Clear Source */
#define _WDOG_CTRL_CLRSRC_SHIFT                   30                                    /**< Shift value for WDOG_CLRSRC */
#define _WDOG_CTRL_CLRSRC_MASK                    0x40000000UL                          /**< Bit mask for WDOG_CLRSRC */
#define _WDOG_CTRL_CLRSRC_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_CLRSRC_SW                      0x00000000UL                          /**< Mode SW for WDOG_CTRL */
#define _WDOG_CTRL_CLRSRC_PCH0                    0x00000001UL                          /**< Mode PCH0 for WDOG_CTRL */
#define WDOG_CTRL_CLRSRC_DEFAULT                  (_WDOG_CTRL_CLRSRC_DEFAULT << 30)     /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_CLRSRC_SW                       (_WDOG_CTRL_CLRSRC_SW << 30)          /**< Shifted mode SW for WDOG_CTRL */
#define WDOG_CTRL_CLRSRC_PCH0                     (_WDOG_CTRL_CLRSRC_PCH0 << 30)        /**< Shifted mode PCH0 for WDOG_CTRL */
#define WDOG_CTRL_WDOGRSTDIS                      (0x1UL << 31)                         /**< Watchdog Reset Disable */
#define _WDOG_CTRL_WDOGRSTDIS_SHIFT               31                                    /**< Shift value for WDOG_WDOGRSTDIS */
#define _WDOG_CTRL_WDOGRSTDIS_MASK                0x80000000UL                          /**< Bit mask for WDOG_WDOGRSTDIS */
#define _WDOG_CTRL_WDOGRSTDIS_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for WDOG_CTRL */
#define _WDOG_CTRL_WDOGRSTDIS_EN                  0x00000000UL                          /**< Mode EN for WDOG_CTRL */
#define _WDOG_CTRL_WDOGRSTDIS_DIS                 0x00000001UL                          /**< Mode DIS for WDOG_CTRL */
#define WDOG_CTRL_WDOGRSTDIS_DEFAULT              (_WDOG_CTRL_WDOGRSTDIS_DEFAULT << 31) /**< Shifted mode DEFAULT for WDOG_CTRL */
#define WDOG_CTRL_WDOGRSTDIS_EN                   (_WDOG_CTRL_WDOGRSTDIS_EN << 31)      /**< Shifted mode EN for WDOG_CTRL */
#define WDOG_CTRL_WDOGRSTDIS_DIS                  (_WDOG_CTRL_WDOGRSTDIS_DIS << 31)     /**< Shifted mode DIS for WDOG_CTRL */

/* Bit fields for WDOG CMD */
#define _WDOG_CMD_RESETVALUE                      0x00000000UL                     /**< Default value for WDOG_CMD */
#define _WDOG_CMD_MASK                            0x00000001UL                     /**< Mask for WDOG_CMD */
#define WDOG_CMD_CLEAR                            (0x1UL << 0)                     /**< Watchdog Timer Clear */
#define _WDOG_CMD_CLEAR_SHIFT                     0                                /**< Shift value for WDOG_CLEAR */
#define _WDOG_CMD_CLEAR_MASK                      0x1UL                            /**< Bit mask for WDOG_CLEAR */
#define _WDOG_CMD_CLEAR_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for WDOG_CMD */
#define _WDOG_CMD_CLEAR_UNCHANGED                 0x00000000UL                     /**< Mode UNCHANGED for WDOG_CMD */
#define _WDOG_CMD_CLEAR_CLEARED                   0x00000001UL                     /**< Mode CLEARED for WDOG_CMD */
#define WDOG_CMD_CLEAR_DEFAULT                    (_WDOG_CMD_CLEAR_DEFAULT << 0)   /**< Shifted mode DEFAULT for WDOG_CMD */
#define WDOG_CMD_CLEAR_UNCHANGED                  (_WDOG_CMD_CLEAR_UNCHANGED << 0) /**< Shifted mode UNCHANGED for WDOG_CMD */
#define WDOG_CMD_CLEAR_CLEARED                    (_WDOG_CMD_CLEAR_CLEARED << 0)   /**< Shifted mode CLEARED for WDOG_CMD */

/* Bit fields for WDOG SYNCBUSY */
#define _WDOG_SYNCBUSY_RESETVALUE                 0x00000000UL                               /**< Default value for WDOG_SYNCBUSY */
#define _WDOG_SYNCBUSY_MASK                       0x0000000FUL                               /**< Mask for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_CTRL                        (0x1UL << 0)                               /**< CTRL Register Busy */
#define _WDOG_SYNCBUSY_CTRL_SHIFT                 0                                          /**< Shift value for WDOG_CTRL */
#define _WDOG_SYNCBUSY_CTRL_MASK                  0x1UL                                      /**< Bit mask for WDOG_CTRL */
#define _WDOG_SYNCBUSY_CTRL_DEFAULT               0x00000000UL                               /**< Mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_CTRL_DEFAULT                (_WDOG_SYNCBUSY_CTRL_DEFAULT << 0)         /**< Shifted mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_CMD                         (0x1UL << 1)                               /**< CMD Register Busy */
#define _WDOG_SYNCBUSY_CMD_SHIFT                  1                                          /**< Shift value for WDOG_CMD */
#define _WDOG_SYNCBUSY_CMD_MASK                   0x2UL                                      /**< Bit mask for WDOG_CMD */
#define _WDOG_SYNCBUSY_CMD_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_CMD_DEFAULT                 (_WDOG_SYNCBUSY_CMD_DEFAULT << 1)          /**< Shifted mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_PCH0_PRSCTRL                (0x1UL << 2)                               /**< PCH0_PRSCTRL Register Busy */
#define _WDOG_SYNCBUSY_PCH0_PRSCTRL_SHIFT         2                                          /**< Shift value for WDOG_PCH0_PRSCTRL */
#define _WDOG_SYNCBUSY_PCH0_PRSCTRL_MASK          0x4UL                                      /**< Bit mask for WDOG_PCH0_PRSCTRL */
#define _WDOG_SYNCBUSY_PCH0_PRSCTRL_DEFAULT       0x00000000UL                               /**< Mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_PCH0_PRSCTRL_DEFAULT        (_WDOG_SYNCBUSY_PCH0_PRSCTRL_DEFAULT << 2) /**< Shifted mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_PCH1_PRSCTRL                (0x1UL << 3)                               /**< PCH1_PRSCTRL Register Busy */
#define _WDOG_SYNCBUSY_PCH1_PRSCTRL_SHIFT         3                                          /**< Shift value for WDOG_PCH1_PRSCTRL */
#define _WDOG_SYNCBUSY_PCH1_PRSCTRL_MASK          0x8UL                                      /**< Bit mask for WDOG_PCH1_PRSCTRL */
#define _WDOG_SYNCBUSY_PCH1_PRSCTRL_DEFAULT       0x00000000UL                               /**< Mode DEFAULT for WDOG_SYNCBUSY */
#define WDOG_SYNCBUSY_PCH1_PRSCTRL_DEFAULT        (_WDOG_SYNCBUSY_PCH1_PRSCTRL_DEFAULT << 3) /**< Shifted mode DEFAULT for WDOG_SYNCBUSY */

/* Bit fields for WDOG PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_RESETVALUE              0x00000000UL                                  /**< Default value for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_MASK                    0x0000010FUL                                  /**< Mask for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_SHIFT            0                                             /**< Shift value for WDOG_PRSSEL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_MASK             0xFUL                                         /**< Bit mask for WDOG_PRSSEL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_DEFAULT          0x00000000UL                                  /**< Mode DEFAULT for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH0           0x00000000UL                                  /**< Mode PRSCH0 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH1           0x00000001UL                                  /**< Mode PRSCH1 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH2           0x00000002UL                                  /**< Mode PRSCH2 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH3           0x00000003UL                                  /**< Mode PRSCH3 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH4           0x00000004UL                                  /**< Mode PRSCH4 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH5           0x00000005UL                                  /**< Mode PRSCH5 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH6           0x00000006UL                                  /**< Mode PRSCH6 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH7           0x00000007UL                                  /**< Mode PRSCH7 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH8           0x00000008UL                                  /**< Mode PRSCH8 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH9           0x00000009UL                                  /**< Mode PRSCH9 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH10          0x0000000AUL                                  /**< Mode PRSCH10 for WDOG_PCH_PRSCTRL */
#define _WDOG_PCH_PRSCTRL_PRSSEL_PRSCH11          0x0000000BUL                                  /**< Mode PRSCH11 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_DEFAULT           (_WDOG_PCH_PRSCTRL_PRSSEL_DEFAULT << 0)       /**< Shifted mode DEFAULT for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH0            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH0 << 0)        /**< Shifted mode PRSCH0 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH1            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH1 << 0)        /**< Shifted mode PRSCH1 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH2            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH2 << 0)        /**< Shifted mode PRSCH2 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH3            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH3 << 0)        /**< Shifted mode PRSCH3 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH4            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH4 << 0)        /**< Shifted mode PRSCH4 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH5            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH5 << 0)        /**< Shifted mode PRSCH5 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH6            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH6 << 0)        /**< Shifted mode PRSCH6 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH7            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH7 << 0)        /**< Shifted mode PRSCH7 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH8            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH8 << 0)        /**< Shifted mode PRSCH8 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH9            (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH9 << 0)        /**< Shifted mode PRSCH9 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH10           (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH10 << 0)       /**< Shifted mode PRSCH10 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSSEL_PRSCH11           (_WDOG_PCH_PRSCTRL_PRSSEL_PRSCH11 << 0)       /**< Shifted mode PRSCH11 for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSMISSRSTEN             (0x1UL << 8)                                  /**< PRS missing event will trigger a watchdog reset */
#define _WDOG_PCH_PRSCTRL_PRSMISSRSTEN_SHIFT      8                                             /**< Shift value for WDOG_PRSMISSRSTEN */
#define _WDOG_PCH_PRSCTRL_PRSMISSRSTEN_MASK       0x100UL                                       /**< Bit mask for WDOG_PRSMISSRSTEN */
#define _WDOG_PCH_PRSCTRL_PRSMISSRSTEN_DEFAULT    0x00000000UL                                  /**< Mode DEFAULT for WDOG_PCH_PRSCTRL */
#define WDOG_PCH_PRSCTRL_PRSMISSRSTEN_DEFAULT     (_WDOG_PCH_PRSCTRL_PRSMISSRSTEN_DEFAULT << 8) /**< Shifted mode DEFAULT for WDOG_PCH_PRSCTRL */

/* Bit fields for WDOG IF */
#define _WDOG_IF_RESETVALUE                       0x00000000UL                 /**< Default value for WDOG_IF */
#define _WDOG_IF_MASK                             0x0000001FUL                 /**< Mask for WDOG_IF */
#define WDOG_IF_TOUT                              (0x1UL << 0)                 /**< WDOG Timeout Interrupt Flag */
#define _WDOG_IF_TOUT_SHIFT                       0                            /**< Shift value for WDOG_TOUT */
#define _WDOG_IF_TOUT_MASK                        0x1UL                        /**< Bit mask for WDOG_TOUT */
#define _WDOG_IF_TOUT_DEFAULT                     0x00000000UL                 /**< Mode DEFAULT for WDOG_IF */
#define WDOG_IF_TOUT_DEFAULT                      (_WDOG_IF_TOUT_DEFAULT << 0) /**< Shifted mode DEFAULT for WDOG_IF */
#define WDOG_IF_WARN                              (0x1UL << 1)                 /**< WDOG Warning Timeout Interrupt Flag */
#define _WDOG_IF_WARN_SHIFT                       1                            /**< Shift value for WDOG_WARN */
#define _WDOG_IF_WARN_MASK                        0x2UL                        /**< Bit mask for WDOG_WARN */
#define _WDOG_IF_WARN_DEFAULT                     0x00000000UL                 /**< Mode DEFAULT for WDOG_IF */
#define WDOG_IF_WARN_DEFAULT                      (_WDOG_IF_WARN_DEFAULT << 1) /**< Shifted mode DEFAULT for WDOG_IF */
#define WDOG_IF_WIN                               (0x1UL << 2)                 /**< WDOG Window Interrupt Flag */
#define _WDOG_IF_WIN_SHIFT                        2                            /**< Shift value for WDOG_WIN */
#define _WDOG_IF_WIN_MASK                         0x4UL                        /**< Bit mask for WDOG_WIN */
#define _WDOG_IF_WIN_DEFAULT                      0x00000000UL                 /**< Mode DEFAULT for WDOG_IF */
#define WDOG_IF_WIN_DEFAULT                       (_WDOG_IF_WIN_DEFAULT << 2)  /**< Shifted mode DEFAULT for WDOG_IF */
#define WDOG_IF_PEM0                              (0x1UL << 3)                 /**< PRS Channel Zero Event Missing Interrupt Flag */
#define _WDOG_IF_PEM0_SHIFT                       3                            /**< Shift value for WDOG_PEM0 */
#define _WDOG_IF_PEM0_MASK                        0x8UL                        /**< Bit mask for WDOG_PEM0 */
#define _WDOG_IF_PEM0_DEFAULT                     0x00000000UL                 /**< Mode DEFAULT for WDOG_IF */
#define WDOG_IF_PEM0_DEFAULT                      (_WDOG_IF_PEM0_DEFAULT << 3) /**< Shifted mode DEFAULT for WDOG_IF */
#define WDOG_IF_PEM1                              (0x1UL << 4)                 /**< PRS Channel One Event Missing Interrupt Flag */
#define _WDOG_IF_PEM1_SHIFT                       4                            /**< Shift value for WDOG_PEM1 */
#define _WDOG_IF_PEM1_MASK                        0x10UL                       /**< Bit mask for WDOG_PEM1 */
#define _WDOG_IF_PEM1_DEFAULT                     0x00000000UL                 /**< Mode DEFAULT for WDOG_IF */
#define WDOG_IF_PEM1_DEFAULT                      (_WDOG_IF_PEM1_DEFAULT << 4) /**< Shifted mode DEFAULT for WDOG_IF */

/* Bit fields for WDOG IFS */
#define _WDOG_IFS_RESETVALUE                      0x00000000UL                  /**< Default value for WDOG_IFS */
#define _WDOG_IFS_MASK                            0x0000001FUL                  /**< Mask for WDOG_IFS */
#define WDOG_IFS_TOUT                             (0x1UL << 0)                  /**< Set TOUT Interrupt Flag */
#define _WDOG_IFS_TOUT_SHIFT                      0                             /**< Shift value for WDOG_TOUT */
#define _WDOG_IFS_TOUT_MASK                       0x1UL                         /**< Bit mask for WDOG_TOUT */
#define _WDOG_IFS_TOUT_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_TOUT_DEFAULT                     (_WDOG_IFS_TOUT_DEFAULT << 0) /**< Shifted mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_WARN                             (0x1UL << 1)                  /**< Set WARN Interrupt Flag */
#define _WDOG_IFS_WARN_SHIFT                      1                             /**< Shift value for WDOG_WARN */
#define _WDOG_IFS_WARN_MASK                       0x2UL                         /**< Bit mask for WDOG_WARN */
#define _WDOG_IFS_WARN_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_WARN_DEFAULT                     (_WDOG_IFS_WARN_DEFAULT << 1) /**< Shifted mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_WIN                              (0x1UL << 2)                  /**< Set WIN Interrupt Flag */
#define _WDOG_IFS_WIN_SHIFT                       2                             /**< Shift value for WDOG_WIN */
#define _WDOG_IFS_WIN_MASK                        0x4UL                         /**< Bit mask for WDOG_WIN */
#define _WDOG_IFS_WIN_DEFAULT                     0x00000000UL                  /**< Mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_WIN_DEFAULT                      (_WDOG_IFS_WIN_DEFAULT << 2)  /**< Shifted mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_PEM0                             (0x1UL << 3)                  /**< Set PEM0 Interrupt Flag */
#define _WDOG_IFS_PEM0_SHIFT                      3                             /**< Shift value for WDOG_PEM0 */
#define _WDOG_IFS_PEM0_MASK                       0x8UL                         /**< Bit mask for WDOG_PEM0 */
#define _WDOG_IFS_PEM0_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_PEM0_DEFAULT                     (_WDOG_IFS_PEM0_DEFAULT << 3) /**< Shifted mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_PEM1                             (0x1UL << 4)                  /**< Set PEM1 Interrupt Flag */
#define _WDOG_IFS_PEM1_SHIFT                      4                             /**< Shift value for WDOG_PEM1 */
#define _WDOG_IFS_PEM1_MASK                       0x10UL                        /**< Bit mask for WDOG_PEM1 */
#define _WDOG_IFS_PEM1_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IFS */
#define WDOG_IFS_PEM1_DEFAULT                     (_WDOG_IFS_PEM1_DEFAULT << 4) /**< Shifted mode DEFAULT for WDOG_IFS */

/* Bit fields for WDOG IFC */
#define _WDOG_IFC_RESETVALUE                      0x00000000UL                  /**< Default value for WDOG_IFC */
#define _WDOG_IFC_MASK                            0x0000001FUL                  /**< Mask for WDOG_IFC */
#define WDOG_IFC_TOUT                             (0x1UL << 0)                  /**< Clear TOUT Interrupt Flag */
#define _WDOG_IFC_TOUT_SHIFT                      0                             /**< Shift value for WDOG_TOUT */
#define _WDOG_IFC_TOUT_MASK                       0x1UL                         /**< Bit mask for WDOG_TOUT */
#define _WDOG_IFC_TOUT_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_TOUT_DEFAULT                     (_WDOG_IFC_TOUT_DEFAULT << 0) /**< Shifted mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_WARN                             (0x1UL << 1)                  /**< Clear WARN Interrupt Flag */
#define _WDOG_IFC_WARN_SHIFT                      1                             /**< Shift value for WDOG_WARN */
#define _WDOG_IFC_WARN_MASK                       0x2UL                         /**< Bit mask for WDOG_WARN */
#define _WDOG_IFC_WARN_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_WARN_DEFAULT                     (_WDOG_IFC_WARN_DEFAULT << 1) /**< Shifted mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_WIN                              (0x1UL << 2)                  /**< Clear WIN Interrupt Flag */
#define _WDOG_IFC_WIN_SHIFT                       2                             /**< Shift value for WDOG_WIN */
#define _WDOG_IFC_WIN_MASK                        0x4UL                         /**< Bit mask for WDOG_WIN */
#define _WDOG_IFC_WIN_DEFAULT                     0x00000000UL                  /**< Mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_WIN_DEFAULT                      (_WDOG_IFC_WIN_DEFAULT << 2)  /**< Shifted mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_PEM0                             (0x1UL << 3)                  /**< Clear PEM0 Interrupt Flag */
#define _WDOG_IFC_PEM0_SHIFT                      3                             /**< Shift value for WDOG_PEM0 */
#define _WDOG_IFC_PEM0_MASK                       0x8UL                         /**< Bit mask for WDOG_PEM0 */
#define _WDOG_IFC_PEM0_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_PEM0_DEFAULT                     (_WDOG_IFC_PEM0_DEFAULT << 3) /**< Shifted mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_PEM1                             (0x1UL << 4)                  /**< Clear PEM1 Interrupt Flag */
#define _WDOG_IFC_PEM1_SHIFT                      4                             /**< Shift value for WDOG_PEM1 */
#define _WDOG_IFC_PEM1_MASK                       0x10UL                        /**< Bit mask for WDOG_PEM1 */
#define _WDOG_IFC_PEM1_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IFC */
#define WDOG_IFC_PEM1_DEFAULT                     (_WDOG_IFC_PEM1_DEFAULT << 4) /**< Shifted mode DEFAULT for WDOG_IFC */

/* Bit fields for WDOG IEN */
#define _WDOG_IEN_RESETVALUE                      0x00000000UL                  /**< Default value for WDOG_IEN */
#define _WDOG_IEN_MASK                            0x0000001FUL                  /**< Mask for WDOG_IEN */
#define WDOG_IEN_TOUT                             (0x1UL << 0)                  /**< TOUT Interrupt Enable */
#define _WDOG_IEN_TOUT_SHIFT                      0                             /**< Shift value for WDOG_TOUT */
#define _WDOG_IEN_TOUT_MASK                       0x1UL                         /**< Bit mask for WDOG_TOUT */
#define _WDOG_IEN_TOUT_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_TOUT_DEFAULT                     (_WDOG_IEN_TOUT_DEFAULT << 0) /**< Shifted mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_WARN                             (0x1UL << 1)                  /**< WARN Interrupt Enable */
#define _WDOG_IEN_WARN_SHIFT                      1                             /**< Shift value for WDOG_WARN */
#define _WDOG_IEN_WARN_MASK                       0x2UL                         /**< Bit mask for WDOG_WARN */
#define _WDOG_IEN_WARN_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_WARN_DEFAULT                     (_WDOG_IEN_WARN_DEFAULT << 1) /**< Shifted mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_WIN                              (0x1UL << 2)                  /**< WIN Interrupt Enable */
#define _WDOG_IEN_WIN_SHIFT                       2                             /**< Shift value for WDOG_WIN */
#define _WDOG_IEN_WIN_MASK                        0x4UL                         /**< Bit mask for WDOG_WIN */
#define _WDOG_IEN_WIN_DEFAULT                     0x00000000UL                  /**< Mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_WIN_DEFAULT                      (_WDOG_IEN_WIN_DEFAULT << 2)  /**< Shifted mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_PEM0                             (0x1UL << 3)                  /**< PEM0 Interrupt Enable */
#define _WDOG_IEN_PEM0_SHIFT                      3                             /**< Shift value for WDOG_PEM0 */
#define _WDOG_IEN_PEM0_MASK                       0x8UL                         /**< Bit mask for WDOG_PEM0 */
#define _WDOG_IEN_PEM0_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_PEM0_DEFAULT                     (_WDOG_IEN_PEM0_DEFAULT << 3) /**< Shifted mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_PEM1                             (0x1UL << 4)                  /**< PEM1 Interrupt Enable */
#define _WDOG_IEN_PEM1_SHIFT                      4                             /**< Shift value for WDOG_PEM1 */
#define _WDOG_IEN_PEM1_MASK                       0x10UL                        /**< Bit mask for WDOG_PEM1 */
#define _WDOG_IEN_PEM1_DEFAULT                    0x00000000UL                  /**< Mode DEFAULT for WDOG_IEN */
#define WDOG_IEN_PEM1_DEFAULT                     (_WDOG_IEN_PEM1_DEFAULT << 4) /**< Shifted mode DEFAULT for WDOG_IEN */

/** @} End of group EFR32FG1P_WDOG */
/** @} End of group Parts */

