/**************************************************************************//**
 * @file efr32mg12p_vdac.h
 * @brief EFR32MG12P_VDAC register and bit field definitions
 * @version 5.5.0
 ******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories, Inc. www.silabs.com</b>
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

#if defined(__ICCARM__)
#pragma system_include       /* Treat file as system include file. */
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang system_header  /* Treat file as system include file. */
#endif

/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/
/**************************************************************************//**
 * @defgroup EFR32MG12P_VDAC VDAC
 * @{
 * @brief EFR32MG12P_VDAC Register Declaration
 *****************************************************************************/
/** VDAC Register Declaration */
typedef struct {
  __IOM uint32_t   CTRL;          /**< Control Register  */
  __IM uint32_t    STATUS;        /**< Status Register  */
  __IOM uint32_t   CH0CTRL;       /**< Channel 0 Control Register  */
  __IOM uint32_t   CH1CTRL;       /**< Channel 1 Control Register  */
  __IOM uint32_t   CMD;           /**< Command Register  */
  __IM uint32_t    IF;            /**< Interrupt Flag Register  */
  __IOM uint32_t   IFS;           /**< Interrupt Flag Set Register  */
  __IOM uint32_t   IFC;           /**< Interrupt Flag Clear Register  */
  __IOM uint32_t   IEN;           /**< Interrupt Enable Register  */
  __IOM uint32_t   CH0DATA;       /**< Channel 0 Data Register  */
  __IOM uint32_t   CH1DATA;       /**< Channel 1 Data Register  */
  __IOM uint32_t   COMBDATA;      /**< Combined Data Register  */
  __IOM uint32_t   CAL;           /**< Calibration Register  */

  uint32_t         RESERVED0[27]; /**< Reserved registers */
  VDAC_OPA_TypeDef OPA[3];        /**< OPA Registers */
} VDAC_TypeDef;                   /** @} */

/**************************************************************************//**
 * @addtogroup EFR32MG12P_VDAC
 * @{
 * @defgroup EFR32MG12P_VDAC_BitFields  VDAC Bit Fields
 * @{
 *****************************************************************************/

/* Bit fields for VDAC CTRL */
#define _VDAC_CTRL_RESETVALUE                              0x00000000UL                                /**< Default value for VDAC_CTRL */
#define _VDAC_CTRL_MASK                                    0x937F0771UL                                /**< Mask for VDAC_CTRL */
#define VDAC_CTRL_DIFF                                     (0x1UL << 0)                                /**< Differential Mode */
#define _VDAC_CTRL_DIFF_SHIFT                              0                                           /**< Shift value for VDAC_DIFF */
#define _VDAC_CTRL_DIFF_MASK                               0x1UL                                       /**< Bit mask for VDAC_DIFF */
#define _VDAC_CTRL_DIFF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_DIFF_DEFAULT                             (_VDAC_CTRL_DIFF_DEFAULT << 0)              /**< Shifted mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_SINEMODE                                 (0x1UL << 4)                                /**< Sine Mode */
#define _VDAC_CTRL_SINEMODE_SHIFT                          4                                           /**< Shift value for VDAC_SINEMODE */
#define _VDAC_CTRL_SINEMODE_MASK                           0x10UL                                      /**< Bit mask for VDAC_SINEMODE */
#define _VDAC_CTRL_SINEMODE_DEFAULT                        0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_SINEMODE_DEFAULT                         (_VDAC_CTRL_SINEMODE_DEFAULT << 4)          /**< Shifted mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_OUTENPRS                                 (0x1UL << 5)                                /**< PRS Controlled Output Enable */
#define _VDAC_CTRL_OUTENPRS_SHIFT                          5                                           /**< Shift value for VDAC_OUTENPRS */
#define _VDAC_CTRL_OUTENPRS_MASK                           0x20UL                                      /**< Bit mask for VDAC_OUTENPRS */
#define _VDAC_CTRL_OUTENPRS_DEFAULT                        0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_OUTENPRS_DEFAULT                         (_VDAC_CTRL_OUTENPRS_DEFAULT << 5)          /**< Shifted mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_CH0PRESCRST                              (0x1UL << 6)                                /**< Channel 0 Start Reset Prescaler */
#define _VDAC_CTRL_CH0PRESCRST_SHIFT                       6                                           /**< Shift value for VDAC_CH0PRESCRST */
#define _VDAC_CTRL_CH0PRESCRST_MASK                        0x40UL                                      /**< Bit mask for VDAC_CH0PRESCRST */
#define _VDAC_CTRL_CH0PRESCRST_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_CH0PRESCRST_DEFAULT                      (_VDAC_CTRL_CH0PRESCRST_DEFAULT << 6)       /**< Shifted mode DEFAULT for VDAC_CTRL */
#define _VDAC_CTRL_REFSEL_SHIFT                            8                                           /**< Shift value for VDAC_REFSEL */
#define _VDAC_CTRL_REFSEL_MASK                             0x700UL                                     /**< Bit mask for VDAC_REFSEL */
#define _VDAC_CTRL_REFSEL_DEFAULT                          0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define _VDAC_CTRL_REFSEL_1V25LN                           0x00000000UL                                /**< Mode 1V25LN for VDAC_CTRL */
#define _VDAC_CTRL_REFSEL_2V5LN                            0x00000001UL                                /**< Mode 2V5LN for VDAC_CTRL */
#define _VDAC_CTRL_REFSEL_1V25                             0x00000002UL                                /**< Mode 1V25 for VDAC_CTRL */
#define _VDAC_CTRL_REFSEL_2V5                              0x00000003UL                                /**< Mode 2V5 for VDAC_CTRL */
#define _VDAC_CTRL_REFSEL_VDD                              0x00000004UL                                /**< Mode VDD for VDAC_CTRL */
#define _VDAC_CTRL_REFSEL_EXT                              0x00000006UL                                /**< Mode EXT for VDAC_CTRL */
#define VDAC_CTRL_REFSEL_DEFAULT                           (_VDAC_CTRL_REFSEL_DEFAULT << 8)            /**< Shifted mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_REFSEL_1V25LN                            (_VDAC_CTRL_REFSEL_1V25LN << 8)             /**< Shifted mode 1V25LN for VDAC_CTRL */
#define VDAC_CTRL_REFSEL_2V5LN                             (_VDAC_CTRL_REFSEL_2V5LN << 8)              /**< Shifted mode 2V5LN for VDAC_CTRL */
#define VDAC_CTRL_REFSEL_1V25                              (_VDAC_CTRL_REFSEL_1V25 << 8)               /**< Shifted mode 1V25 for VDAC_CTRL */
#define VDAC_CTRL_REFSEL_2V5                               (_VDAC_CTRL_REFSEL_2V5 << 8)                /**< Shifted mode 2V5 for VDAC_CTRL */
#define VDAC_CTRL_REFSEL_VDD                               (_VDAC_CTRL_REFSEL_VDD << 8)                /**< Shifted mode VDD for VDAC_CTRL */
#define VDAC_CTRL_REFSEL_EXT                               (_VDAC_CTRL_REFSEL_EXT << 8)                /**< Shifted mode EXT for VDAC_CTRL */
#define _VDAC_CTRL_PRESC_SHIFT                             16                                          /**< Shift value for VDAC_PRESC */
#define _VDAC_CTRL_PRESC_MASK                              0x7F0000UL                                  /**< Bit mask for VDAC_PRESC */
#define _VDAC_CTRL_PRESC_DEFAULT                           0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define _VDAC_CTRL_PRESC_NODIVISION                        0x00000000UL                                /**< Mode NODIVISION for VDAC_CTRL */
#define VDAC_CTRL_PRESC_DEFAULT                            (_VDAC_CTRL_PRESC_DEFAULT << 16)            /**< Shifted mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_PRESC_NODIVISION                         (_VDAC_CTRL_PRESC_NODIVISION << 16)         /**< Shifted mode NODIVISION for VDAC_CTRL */
#define _VDAC_CTRL_REFRESHPERIOD_SHIFT                     24                                          /**< Shift value for VDAC_REFRESHPERIOD */
#define _VDAC_CTRL_REFRESHPERIOD_MASK                      0x3000000UL                                 /**< Bit mask for VDAC_REFRESHPERIOD */
#define _VDAC_CTRL_REFRESHPERIOD_DEFAULT                   0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define _VDAC_CTRL_REFRESHPERIOD_8CYCLES                   0x00000000UL                                /**< Mode 8CYCLES for VDAC_CTRL */
#define _VDAC_CTRL_REFRESHPERIOD_16CYCLES                  0x00000001UL                                /**< Mode 16CYCLES for VDAC_CTRL */
#define _VDAC_CTRL_REFRESHPERIOD_32CYCLES                  0x00000002UL                                /**< Mode 32CYCLES for VDAC_CTRL */
#define _VDAC_CTRL_REFRESHPERIOD_64CYCLES                  0x00000003UL                                /**< Mode 64CYCLES for VDAC_CTRL */
#define VDAC_CTRL_REFRESHPERIOD_DEFAULT                    (_VDAC_CTRL_REFRESHPERIOD_DEFAULT << 24)    /**< Shifted mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_REFRESHPERIOD_8CYCLES                    (_VDAC_CTRL_REFRESHPERIOD_8CYCLES << 24)    /**< Shifted mode 8CYCLES for VDAC_CTRL */
#define VDAC_CTRL_REFRESHPERIOD_16CYCLES                   (_VDAC_CTRL_REFRESHPERIOD_16CYCLES << 24)   /**< Shifted mode 16CYCLES for VDAC_CTRL */
#define VDAC_CTRL_REFRESHPERIOD_32CYCLES                   (_VDAC_CTRL_REFRESHPERIOD_32CYCLES << 24)   /**< Shifted mode 32CYCLES for VDAC_CTRL */
#define VDAC_CTRL_REFRESHPERIOD_64CYCLES                   (_VDAC_CTRL_REFRESHPERIOD_64CYCLES << 24)   /**< Shifted mode 64CYCLES for VDAC_CTRL */
#define VDAC_CTRL_WARMUPMODE                               (0x1UL << 28)                               /**< Warm-up Mode */
#define _VDAC_CTRL_WARMUPMODE_SHIFT                        28                                          /**< Shift value for VDAC_WARMUPMODE */
#define _VDAC_CTRL_WARMUPMODE_MASK                         0x10000000UL                                /**< Bit mask for VDAC_WARMUPMODE */
#define _VDAC_CTRL_WARMUPMODE_DEFAULT                      0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define _VDAC_CTRL_WARMUPMODE_NORMAL                       0x00000000UL                                /**< Mode NORMAL for VDAC_CTRL */
#define _VDAC_CTRL_WARMUPMODE_KEEPINSTANDBY                0x00000001UL                                /**< Mode KEEPINSTANDBY for VDAC_CTRL */
#define VDAC_CTRL_WARMUPMODE_DEFAULT                       (_VDAC_CTRL_WARMUPMODE_DEFAULT << 28)       /**< Shifted mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_WARMUPMODE_NORMAL                        (_VDAC_CTRL_WARMUPMODE_NORMAL << 28)        /**< Shifted mode NORMAL for VDAC_CTRL */
#define VDAC_CTRL_WARMUPMODE_KEEPINSTANDBY                 (_VDAC_CTRL_WARMUPMODE_KEEPINSTANDBY << 28) /**< Shifted mode KEEPINSTANDBY for VDAC_CTRL */
#define VDAC_CTRL_DACCLKMODE                               (0x1UL << 31)                               /**< Clock Mode */
#define _VDAC_CTRL_DACCLKMODE_SHIFT                        31                                          /**< Shift value for VDAC_DACCLKMODE */
#define _VDAC_CTRL_DACCLKMODE_MASK                         0x80000000UL                                /**< Bit mask for VDAC_DACCLKMODE */
#define _VDAC_CTRL_DACCLKMODE_DEFAULT                      0x00000000UL                                /**< Mode DEFAULT for VDAC_CTRL */
#define _VDAC_CTRL_DACCLKMODE_SYNC                         0x00000000UL                                /**< Mode SYNC for VDAC_CTRL */
#define _VDAC_CTRL_DACCLKMODE_ASYNC                        0x00000001UL                                /**< Mode ASYNC for VDAC_CTRL */
#define VDAC_CTRL_DACCLKMODE_DEFAULT                       (_VDAC_CTRL_DACCLKMODE_DEFAULT << 31)       /**< Shifted mode DEFAULT for VDAC_CTRL */
#define VDAC_CTRL_DACCLKMODE_SYNC                          (_VDAC_CTRL_DACCLKMODE_SYNC << 31)          /**< Shifted mode SYNC for VDAC_CTRL */
#define VDAC_CTRL_DACCLKMODE_ASYNC                         (_VDAC_CTRL_DACCLKMODE_ASYNC << 31)         /**< Shifted mode ASYNC for VDAC_CTRL */

/* Bit fields for VDAC STATUS */
#define _VDAC_STATUS_RESETVALUE                            0x0000000CUL                                   /**< Default value for VDAC_STATUS */
#define _VDAC_STATUS_MASK                                  0x7777003FUL                                   /**< Mask for VDAC_STATUS */
#define VDAC_STATUS_CH0ENS                                 (0x1UL << 0)                                   /**< Channel 0 Enabled Status */
#define _VDAC_STATUS_CH0ENS_SHIFT                          0                                              /**< Shift value for VDAC_CH0ENS */
#define _VDAC_STATUS_CH0ENS_MASK                           0x1UL                                          /**< Bit mask for VDAC_CH0ENS */
#define _VDAC_STATUS_CH0ENS_DEFAULT                        0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH0ENS_DEFAULT                         (_VDAC_STATUS_CH0ENS_DEFAULT << 0)             /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH1ENS                                 (0x1UL << 1)                                   /**< Channel 1 Enabled Status */
#define _VDAC_STATUS_CH1ENS_SHIFT                          1                                              /**< Shift value for VDAC_CH1ENS */
#define _VDAC_STATUS_CH1ENS_MASK                           0x2UL                                          /**< Bit mask for VDAC_CH1ENS */
#define _VDAC_STATUS_CH1ENS_DEFAULT                        0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH1ENS_DEFAULT                         (_VDAC_STATUS_CH1ENS_DEFAULT << 1)             /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH0BL                                  (0x1UL << 2)                                   /**< Channel 0 Buffer Level */
#define _VDAC_STATUS_CH0BL_SHIFT                           2                                              /**< Shift value for VDAC_CH0BL */
#define _VDAC_STATUS_CH0BL_MASK                            0x4UL                                          /**< Bit mask for VDAC_CH0BL */
#define _VDAC_STATUS_CH0BL_DEFAULT                         0x00000001UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH0BL_DEFAULT                          (_VDAC_STATUS_CH0BL_DEFAULT << 2)              /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH1BL                                  (0x1UL << 3)                                   /**< Channel 1 Buffer Level */
#define _VDAC_STATUS_CH1BL_SHIFT                           3                                              /**< Shift value for VDAC_CH1BL */
#define _VDAC_STATUS_CH1BL_MASK                            0x8UL                                          /**< Bit mask for VDAC_CH1BL */
#define _VDAC_STATUS_CH1BL_DEFAULT                         0x00000001UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH1BL_DEFAULT                          (_VDAC_STATUS_CH1BL_DEFAULT << 3)              /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH0WARM                                (0x1UL << 4)                                   /**< Channel 0 Warm */
#define _VDAC_STATUS_CH0WARM_SHIFT                         4                                              /**< Shift value for VDAC_CH0WARM */
#define _VDAC_STATUS_CH0WARM_MASK                          0x10UL                                         /**< Bit mask for VDAC_CH0WARM */
#define _VDAC_STATUS_CH0WARM_DEFAULT                       0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH0WARM_DEFAULT                        (_VDAC_STATUS_CH0WARM_DEFAULT << 4)            /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH1WARM                                (0x1UL << 5)                                   /**< Channel 1 Warm */
#define _VDAC_STATUS_CH1WARM_SHIFT                         5                                              /**< Shift value for VDAC_CH1WARM */
#define _VDAC_STATUS_CH1WARM_MASK                          0x20UL                                         /**< Bit mask for VDAC_CH1WARM */
#define _VDAC_STATUS_CH1WARM_DEFAULT                       0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_CH1WARM_DEFAULT                        (_VDAC_STATUS_CH1WARM_DEFAULT << 5)            /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA0APORTCONFLICT                      (0x1UL << 16)                                  /**< OPA0 Bus Conflict Output */
#define _VDAC_STATUS_OPA0APORTCONFLICT_SHIFT               16                                             /**< Shift value for VDAC_OPA0APORTCONFLICT */
#define _VDAC_STATUS_OPA0APORTCONFLICT_MASK                0x10000UL                                      /**< Bit mask for VDAC_OPA0APORTCONFLICT */
#define _VDAC_STATUS_OPA0APORTCONFLICT_DEFAULT             0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA0APORTCONFLICT_DEFAULT              (_VDAC_STATUS_OPA0APORTCONFLICT_DEFAULT << 16) /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA1APORTCONFLICT                      (0x1UL << 17)                                  /**< OPA1 Bus Conflict Output */
#define _VDAC_STATUS_OPA1APORTCONFLICT_SHIFT               17                                             /**< Shift value for VDAC_OPA1APORTCONFLICT */
#define _VDAC_STATUS_OPA1APORTCONFLICT_MASK                0x20000UL                                      /**< Bit mask for VDAC_OPA1APORTCONFLICT */
#define _VDAC_STATUS_OPA1APORTCONFLICT_DEFAULT             0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA1APORTCONFLICT_DEFAULT              (_VDAC_STATUS_OPA1APORTCONFLICT_DEFAULT << 17) /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA2APORTCONFLICT                      (0x1UL << 18)                                  /**< OPA2 Bus Conflict Output */
#define _VDAC_STATUS_OPA2APORTCONFLICT_SHIFT               18                                             /**< Shift value for VDAC_OPA2APORTCONFLICT */
#define _VDAC_STATUS_OPA2APORTCONFLICT_MASK                0x40000UL                                      /**< Bit mask for VDAC_OPA2APORTCONFLICT */
#define _VDAC_STATUS_OPA2APORTCONFLICT_DEFAULT             0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA2APORTCONFLICT_DEFAULT              (_VDAC_STATUS_OPA2APORTCONFLICT_DEFAULT << 18) /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA0ENS                                (0x1UL << 20)                                  /**< OPA0 Enabled Status */
#define _VDAC_STATUS_OPA0ENS_SHIFT                         20                                             /**< Shift value for VDAC_OPA0ENS */
#define _VDAC_STATUS_OPA0ENS_MASK                          0x100000UL                                     /**< Bit mask for VDAC_OPA0ENS */
#define _VDAC_STATUS_OPA0ENS_DEFAULT                       0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA0ENS_DEFAULT                        (_VDAC_STATUS_OPA0ENS_DEFAULT << 20)           /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA1ENS                                (0x1UL << 21)                                  /**< OPA1 Enabled Status */
#define _VDAC_STATUS_OPA1ENS_SHIFT                         21                                             /**< Shift value for VDAC_OPA1ENS */
#define _VDAC_STATUS_OPA1ENS_MASK                          0x200000UL                                     /**< Bit mask for VDAC_OPA1ENS */
#define _VDAC_STATUS_OPA1ENS_DEFAULT                       0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA1ENS_DEFAULT                        (_VDAC_STATUS_OPA1ENS_DEFAULT << 21)           /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA2ENS                                (0x1UL << 22)                                  /**< OPA2 Enabled Status */
#define _VDAC_STATUS_OPA2ENS_SHIFT                         22                                             /**< Shift value for VDAC_OPA2ENS */
#define _VDAC_STATUS_OPA2ENS_MASK                          0x400000UL                                     /**< Bit mask for VDAC_OPA2ENS */
#define _VDAC_STATUS_OPA2ENS_DEFAULT                       0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA2ENS_DEFAULT                        (_VDAC_STATUS_OPA2ENS_DEFAULT << 22)           /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA0WARM                               (0x1UL << 24)                                  /**< OPA0 Warm Status */
#define _VDAC_STATUS_OPA0WARM_SHIFT                        24                                             /**< Shift value for VDAC_OPA0WARM */
#define _VDAC_STATUS_OPA0WARM_MASK                         0x1000000UL                                    /**< Bit mask for VDAC_OPA0WARM */
#define _VDAC_STATUS_OPA0WARM_DEFAULT                      0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA0WARM_DEFAULT                       (_VDAC_STATUS_OPA0WARM_DEFAULT << 24)          /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA1WARM                               (0x1UL << 25)                                  /**< OPA1 Warm Status */
#define _VDAC_STATUS_OPA1WARM_SHIFT                        25                                             /**< Shift value for VDAC_OPA1WARM */
#define _VDAC_STATUS_OPA1WARM_MASK                         0x2000000UL                                    /**< Bit mask for VDAC_OPA1WARM */
#define _VDAC_STATUS_OPA1WARM_DEFAULT                      0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA1WARM_DEFAULT                       (_VDAC_STATUS_OPA1WARM_DEFAULT << 25)          /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA2WARM                               (0x1UL << 26)                                  /**< OPA2 Warm Status */
#define _VDAC_STATUS_OPA2WARM_SHIFT                        26                                             /**< Shift value for VDAC_OPA2WARM */
#define _VDAC_STATUS_OPA2WARM_MASK                         0x4000000UL                                    /**< Bit mask for VDAC_OPA2WARM */
#define _VDAC_STATUS_OPA2WARM_DEFAULT                      0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA2WARM_DEFAULT                       (_VDAC_STATUS_OPA2WARM_DEFAULT << 26)          /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA0OUTVALID                           (0x1UL << 28)                                  /**< OPA0 Output Valid Status */
#define _VDAC_STATUS_OPA0OUTVALID_SHIFT                    28                                             /**< Shift value for VDAC_OPA0OUTVALID */
#define _VDAC_STATUS_OPA0OUTVALID_MASK                     0x10000000UL                                   /**< Bit mask for VDAC_OPA0OUTVALID */
#define _VDAC_STATUS_OPA0OUTVALID_DEFAULT                  0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA0OUTVALID_DEFAULT                   (_VDAC_STATUS_OPA0OUTVALID_DEFAULT << 28)      /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA1OUTVALID                           (0x1UL << 29)                                  /**< OPA1 Output Valid Status */
#define _VDAC_STATUS_OPA1OUTVALID_SHIFT                    29                                             /**< Shift value for VDAC_OPA1OUTVALID */
#define _VDAC_STATUS_OPA1OUTVALID_MASK                     0x20000000UL                                   /**< Bit mask for VDAC_OPA1OUTVALID */
#define _VDAC_STATUS_OPA1OUTVALID_DEFAULT                  0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA1OUTVALID_DEFAULT                   (_VDAC_STATUS_OPA1OUTVALID_DEFAULT << 29)      /**< Shifted mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA2OUTVALID                           (0x1UL << 30)                                  /**< OPA2 Output Valid Status */
#define _VDAC_STATUS_OPA2OUTVALID_SHIFT                    30                                             /**< Shift value for VDAC_OPA2OUTVALID */
#define _VDAC_STATUS_OPA2OUTVALID_MASK                     0x40000000UL                                   /**< Bit mask for VDAC_OPA2OUTVALID */
#define _VDAC_STATUS_OPA2OUTVALID_DEFAULT                  0x00000000UL                                   /**< Mode DEFAULT for VDAC_STATUS */
#define VDAC_STATUS_OPA2OUTVALID_DEFAULT                   (_VDAC_STATUS_OPA2OUTVALID_DEFAULT << 30)      /**< Shifted mode DEFAULT for VDAC_STATUS */

/* Bit fields for VDAC CH0CTRL */
#define _VDAC_CH0CTRL_RESETVALUE                           0x00000000UL                             /**< Default value for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_MASK                                 0x0000F171UL                             /**< Mask for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_CONVMODE                              (0x1UL << 0)                             /**< Conversion Mode */
#define _VDAC_CH0CTRL_CONVMODE_SHIFT                       0                                        /**< Shift value for VDAC_CONVMODE */
#define _VDAC_CH0CTRL_CONVMODE_MASK                        0x1UL                                    /**< Bit mask for VDAC_CONVMODE */
#define _VDAC_CH0CTRL_CONVMODE_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_CONVMODE_CONTINUOUS                  0x00000000UL                             /**< Mode CONTINUOUS for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_CONVMODE_SAMPLEOFF                   0x00000001UL                             /**< Mode SAMPLEOFF for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_CONVMODE_DEFAULT                      (_VDAC_CH0CTRL_CONVMODE_DEFAULT << 0)    /**< Shifted mode DEFAULT for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_CONVMODE_CONTINUOUS                   (_VDAC_CH0CTRL_CONVMODE_CONTINUOUS << 0) /**< Shifted mode CONTINUOUS for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_CONVMODE_SAMPLEOFF                    (_VDAC_CH0CTRL_CONVMODE_SAMPLEOFF << 0)  /**< Shifted mode SAMPLEOFF for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_TRIGMODE_SHIFT                       4                                        /**< Shift value for VDAC_TRIGMODE */
#define _VDAC_CH0CTRL_TRIGMODE_MASK                        0x70UL                                   /**< Bit mask for VDAC_TRIGMODE */
#define _VDAC_CH0CTRL_TRIGMODE_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_TRIGMODE_SW                          0x00000000UL                             /**< Mode SW for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_TRIGMODE_PRS                         0x00000001UL                             /**< Mode PRS for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_TRIGMODE_REFRESH                     0x00000002UL                             /**< Mode REFRESH for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_TRIGMODE_SWPRS                       0x00000003UL                             /**< Mode SWPRS for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_TRIGMODE_SWREFRESH                   0x00000004UL                             /**< Mode SWREFRESH for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_TRIGMODE_LESENSE                     0x00000005UL                             /**< Mode LESENSE for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_TRIGMODE_DEFAULT                      (_VDAC_CH0CTRL_TRIGMODE_DEFAULT << 4)    /**< Shifted mode DEFAULT for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_TRIGMODE_SW                           (_VDAC_CH0CTRL_TRIGMODE_SW << 4)         /**< Shifted mode SW for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_TRIGMODE_PRS                          (_VDAC_CH0CTRL_TRIGMODE_PRS << 4)        /**< Shifted mode PRS for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_TRIGMODE_REFRESH                      (_VDAC_CH0CTRL_TRIGMODE_REFRESH << 4)    /**< Shifted mode REFRESH for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_TRIGMODE_SWPRS                        (_VDAC_CH0CTRL_TRIGMODE_SWPRS << 4)      /**< Shifted mode SWPRS for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_TRIGMODE_SWREFRESH                    (_VDAC_CH0CTRL_TRIGMODE_SWREFRESH << 4)  /**< Shifted mode SWREFRESH for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_TRIGMODE_LESENSE                      (_VDAC_CH0CTRL_TRIGMODE_LESENSE << 4)    /**< Shifted mode LESENSE for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSASYNC                              (0x1UL << 8)                             /**< Channel 0 PRS Asynchronous Enable */
#define _VDAC_CH0CTRL_PRSASYNC_SHIFT                       8                                        /**< Shift value for VDAC_PRSASYNC */
#define _VDAC_CH0CTRL_PRSASYNC_MASK                        0x100UL                                  /**< Bit mask for VDAC_PRSASYNC */
#define _VDAC_CH0CTRL_PRSASYNC_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSASYNC_DEFAULT                      (_VDAC_CH0CTRL_PRSASYNC_DEFAULT << 8)    /**< Shifted mode DEFAULT for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_SHIFT                         12                                       /**< Shift value for VDAC_PRSSEL */
#define _VDAC_CH0CTRL_PRSSEL_MASK                          0xF000UL                                 /**< Bit mask for VDAC_PRSSEL */
#define _VDAC_CH0CTRL_PRSSEL_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH0                        0x00000000UL                             /**< Mode PRSCH0 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH1                        0x00000001UL                             /**< Mode PRSCH1 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH2                        0x00000002UL                             /**< Mode PRSCH2 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH3                        0x00000003UL                             /**< Mode PRSCH3 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH4                        0x00000004UL                             /**< Mode PRSCH4 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH5                        0x00000005UL                             /**< Mode PRSCH5 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH6                        0x00000006UL                             /**< Mode PRSCH6 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH7                        0x00000007UL                             /**< Mode PRSCH7 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH8                        0x00000008UL                             /**< Mode PRSCH8 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH9                        0x00000009UL                             /**< Mode PRSCH9 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH10                       0x0000000AUL                             /**< Mode PRSCH10 for VDAC_CH0CTRL */
#define _VDAC_CH0CTRL_PRSSEL_PRSCH11                       0x0000000BUL                             /**< Mode PRSCH11 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_DEFAULT                        (_VDAC_CH0CTRL_PRSSEL_DEFAULT << 12)     /**< Shifted mode DEFAULT for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH0                         (_VDAC_CH0CTRL_PRSSEL_PRSCH0 << 12)      /**< Shifted mode PRSCH0 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH1                         (_VDAC_CH0CTRL_PRSSEL_PRSCH1 << 12)      /**< Shifted mode PRSCH1 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH2                         (_VDAC_CH0CTRL_PRSSEL_PRSCH2 << 12)      /**< Shifted mode PRSCH2 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH3                         (_VDAC_CH0CTRL_PRSSEL_PRSCH3 << 12)      /**< Shifted mode PRSCH3 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH4                         (_VDAC_CH0CTRL_PRSSEL_PRSCH4 << 12)      /**< Shifted mode PRSCH4 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH5                         (_VDAC_CH0CTRL_PRSSEL_PRSCH5 << 12)      /**< Shifted mode PRSCH5 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH6                         (_VDAC_CH0CTRL_PRSSEL_PRSCH6 << 12)      /**< Shifted mode PRSCH6 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH7                         (_VDAC_CH0CTRL_PRSSEL_PRSCH7 << 12)      /**< Shifted mode PRSCH7 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH8                         (_VDAC_CH0CTRL_PRSSEL_PRSCH8 << 12)      /**< Shifted mode PRSCH8 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH9                         (_VDAC_CH0CTRL_PRSSEL_PRSCH9 << 12)      /**< Shifted mode PRSCH9 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH10                        (_VDAC_CH0CTRL_PRSSEL_PRSCH10 << 12)     /**< Shifted mode PRSCH10 for VDAC_CH0CTRL */
#define VDAC_CH0CTRL_PRSSEL_PRSCH11                        (_VDAC_CH0CTRL_PRSSEL_PRSCH11 << 12)     /**< Shifted mode PRSCH11 for VDAC_CH0CTRL */

/* Bit fields for VDAC CH1CTRL */
#define _VDAC_CH1CTRL_RESETVALUE                           0x00000000UL                             /**< Default value for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_MASK                                 0x0000F171UL                             /**< Mask for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_CONVMODE                              (0x1UL << 0)                             /**< Conversion Mode */
#define _VDAC_CH1CTRL_CONVMODE_SHIFT                       0                                        /**< Shift value for VDAC_CONVMODE */
#define _VDAC_CH1CTRL_CONVMODE_MASK                        0x1UL                                    /**< Bit mask for VDAC_CONVMODE */
#define _VDAC_CH1CTRL_CONVMODE_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_CONVMODE_CONTINUOUS                  0x00000000UL                             /**< Mode CONTINUOUS for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_CONVMODE_SAMPLEOFF                   0x00000001UL                             /**< Mode SAMPLEOFF for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_CONVMODE_DEFAULT                      (_VDAC_CH1CTRL_CONVMODE_DEFAULT << 0)    /**< Shifted mode DEFAULT for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_CONVMODE_CONTINUOUS                   (_VDAC_CH1CTRL_CONVMODE_CONTINUOUS << 0) /**< Shifted mode CONTINUOUS for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_CONVMODE_SAMPLEOFF                    (_VDAC_CH1CTRL_CONVMODE_SAMPLEOFF << 0)  /**< Shifted mode SAMPLEOFF for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_TRIGMODE_SHIFT                       4                                        /**< Shift value for VDAC_TRIGMODE */
#define _VDAC_CH1CTRL_TRIGMODE_MASK                        0x70UL                                   /**< Bit mask for VDAC_TRIGMODE */
#define _VDAC_CH1CTRL_TRIGMODE_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_TRIGMODE_SW                          0x00000000UL                             /**< Mode SW for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_TRIGMODE_PRS                         0x00000001UL                             /**< Mode PRS for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_TRIGMODE_REFRESH                     0x00000002UL                             /**< Mode REFRESH for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_TRIGMODE_SWPRS                       0x00000003UL                             /**< Mode SWPRS for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_TRIGMODE_SWREFRESH                   0x00000004UL                             /**< Mode SWREFRESH for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_TRIGMODE_LESENSE                     0x00000005UL                             /**< Mode LESENSE for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_TRIGMODE_DEFAULT                      (_VDAC_CH1CTRL_TRIGMODE_DEFAULT << 4)    /**< Shifted mode DEFAULT for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_TRIGMODE_SW                           (_VDAC_CH1CTRL_TRIGMODE_SW << 4)         /**< Shifted mode SW for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_TRIGMODE_PRS                          (_VDAC_CH1CTRL_TRIGMODE_PRS << 4)        /**< Shifted mode PRS for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_TRIGMODE_REFRESH                      (_VDAC_CH1CTRL_TRIGMODE_REFRESH << 4)    /**< Shifted mode REFRESH for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_TRIGMODE_SWPRS                        (_VDAC_CH1CTRL_TRIGMODE_SWPRS << 4)      /**< Shifted mode SWPRS for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_TRIGMODE_SWREFRESH                    (_VDAC_CH1CTRL_TRIGMODE_SWREFRESH << 4)  /**< Shifted mode SWREFRESH for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_TRIGMODE_LESENSE                      (_VDAC_CH1CTRL_TRIGMODE_LESENSE << 4)    /**< Shifted mode LESENSE for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSASYNC                              (0x1UL << 8)                             /**< Channel 1 PRS Asynchronous Enable */
#define _VDAC_CH1CTRL_PRSASYNC_SHIFT                       8                                        /**< Shift value for VDAC_PRSASYNC */
#define _VDAC_CH1CTRL_PRSASYNC_MASK                        0x100UL                                  /**< Bit mask for VDAC_PRSASYNC */
#define _VDAC_CH1CTRL_PRSASYNC_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSASYNC_DEFAULT                      (_VDAC_CH1CTRL_PRSASYNC_DEFAULT << 8)    /**< Shifted mode DEFAULT for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_SHIFT                         12                                       /**< Shift value for VDAC_PRSSEL */
#define _VDAC_CH1CTRL_PRSSEL_MASK                          0xF000UL                                 /**< Bit mask for VDAC_PRSSEL */
#define _VDAC_CH1CTRL_PRSSEL_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH0                        0x00000000UL                             /**< Mode PRSCH0 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH1                        0x00000001UL                             /**< Mode PRSCH1 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH2                        0x00000002UL                             /**< Mode PRSCH2 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH3                        0x00000003UL                             /**< Mode PRSCH3 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH4                        0x00000004UL                             /**< Mode PRSCH4 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH5                        0x00000005UL                             /**< Mode PRSCH5 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH6                        0x00000006UL                             /**< Mode PRSCH6 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH7                        0x00000007UL                             /**< Mode PRSCH7 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH8                        0x00000008UL                             /**< Mode PRSCH8 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH9                        0x00000009UL                             /**< Mode PRSCH9 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH10                       0x0000000AUL                             /**< Mode PRSCH10 for VDAC_CH1CTRL */
#define _VDAC_CH1CTRL_PRSSEL_PRSCH11                       0x0000000BUL                             /**< Mode PRSCH11 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_DEFAULT                        (_VDAC_CH1CTRL_PRSSEL_DEFAULT << 12)     /**< Shifted mode DEFAULT for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH0                         (_VDAC_CH1CTRL_PRSSEL_PRSCH0 << 12)      /**< Shifted mode PRSCH0 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH1                         (_VDAC_CH1CTRL_PRSSEL_PRSCH1 << 12)      /**< Shifted mode PRSCH1 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH2                         (_VDAC_CH1CTRL_PRSSEL_PRSCH2 << 12)      /**< Shifted mode PRSCH2 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH3                         (_VDAC_CH1CTRL_PRSSEL_PRSCH3 << 12)      /**< Shifted mode PRSCH3 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH4                         (_VDAC_CH1CTRL_PRSSEL_PRSCH4 << 12)      /**< Shifted mode PRSCH4 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH5                         (_VDAC_CH1CTRL_PRSSEL_PRSCH5 << 12)      /**< Shifted mode PRSCH5 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH6                         (_VDAC_CH1CTRL_PRSSEL_PRSCH6 << 12)      /**< Shifted mode PRSCH6 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH7                         (_VDAC_CH1CTRL_PRSSEL_PRSCH7 << 12)      /**< Shifted mode PRSCH7 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH8                         (_VDAC_CH1CTRL_PRSSEL_PRSCH8 << 12)      /**< Shifted mode PRSCH8 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH9                         (_VDAC_CH1CTRL_PRSSEL_PRSCH9 << 12)      /**< Shifted mode PRSCH9 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH10                        (_VDAC_CH1CTRL_PRSSEL_PRSCH10 << 12)     /**< Shifted mode PRSCH10 for VDAC_CH1CTRL */
#define VDAC_CH1CTRL_PRSSEL_PRSCH11                        (_VDAC_CH1CTRL_PRSSEL_PRSCH11 << 12)     /**< Shifted mode PRSCH11 for VDAC_CH1CTRL */

/* Bit fields for VDAC CMD */
#define _VDAC_CMD_RESETVALUE                               0x00000000UL                      /**< Default value for VDAC_CMD */
#define _VDAC_CMD_MASK                                     0x003F000FUL                      /**< Mask for VDAC_CMD */
#define VDAC_CMD_CH0EN                                     (0x1UL << 0)                      /**< DAC Channel 0 Enable */
#define _VDAC_CMD_CH0EN_SHIFT                              0                                 /**< Shift value for VDAC_CH0EN */
#define _VDAC_CMD_CH0EN_MASK                               0x1UL                             /**< Bit mask for VDAC_CH0EN */
#define _VDAC_CMD_CH0EN_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_CH0EN_DEFAULT                             (_VDAC_CMD_CH0EN_DEFAULT << 0)    /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_CH0DIS                                    (0x1UL << 1)                      /**< DAC Channel 0 Disable */
#define _VDAC_CMD_CH0DIS_SHIFT                             1                                 /**< Shift value for VDAC_CH0DIS */
#define _VDAC_CMD_CH0DIS_MASK                              0x2UL                             /**< Bit mask for VDAC_CH0DIS */
#define _VDAC_CMD_CH0DIS_DEFAULT                           0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_CH0DIS_DEFAULT                            (_VDAC_CMD_CH0DIS_DEFAULT << 1)   /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_CH1EN                                     (0x1UL << 2)                      /**< DAC Channel 1 Enable */
#define _VDAC_CMD_CH1EN_SHIFT                              2                                 /**< Shift value for VDAC_CH1EN */
#define _VDAC_CMD_CH1EN_MASK                               0x4UL                             /**< Bit mask for VDAC_CH1EN */
#define _VDAC_CMD_CH1EN_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_CH1EN_DEFAULT                             (_VDAC_CMD_CH1EN_DEFAULT << 2)    /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_CH1DIS                                    (0x1UL << 3)                      /**< DAC Channel 1 Disable */
#define _VDAC_CMD_CH1DIS_SHIFT                             3                                 /**< Shift value for VDAC_CH1DIS */
#define _VDAC_CMD_CH1DIS_MASK                              0x8UL                             /**< Bit mask for VDAC_CH1DIS */
#define _VDAC_CMD_CH1DIS_DEFAULT                           0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_CH1DIS_DEFAULT                            (_VDAC_CMD_CH1DIS_DEFAULT << 3)   /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA0EN                                    (0x1UL << 16)                     /**< OPA0 Enable */
#define _VDAC_CMD_OPA0EN_SHIFT                             16                                /**< Shift value for VDAC_OPA0EN */
#define _VDAC_CMD_OPA0EN_MASK                              0x10000UL                         /**< Bit mask for VDAC_OPA0EN */
#define _VDAC_CMD_OPA0EN_DEFAULT                           0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA0EN_DEFAULT                            (_VDAC_CMD_OPA0EN_DEFAULT << 16)  /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA0DIS                                   (0x1UL << 17)                     /**< OPA0 Disable */
#define _VDAC_CMD_OPA0DIS_SHIFT                            17                                /**< Shift value for VDAC_OPA0DIS */
#define _VDAC_CMD_OPA0DIS_MASK                             0x20000UL                         /**< Bit mask for VDAC_OPA0DIS */
#define _VDAC_CMD_OPA0DIS_DEFAULT                          0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA0DIS_DEFAULT                           (_VDAC_CMD_OPA0DIS_DEFAULT << 17) /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA1EN                                    (0x1UL << 18)                     /**< OPA1 Enable */
#define _VDAC_CMD_OPA1EN_SHIFT                             18                                /**< Shift value for VDAC_OPA1EN */
#define _VDAC_CMD_OPA1EN_MASK                              0x40000UL                         /**< Bit mask for VDAC_OPA1EN */
#define _VDAC_CMD_OPA1EN_DEFAULT                           0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA1EN_DEFAULT                            (_VDAC_CMD_OPA1EN_DEFAULT << 18)  /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA1DIS                                   (0x1UL << 19)                     /**< OPA1 Disable */
#define _VDAC_CMD_OPA1DIS_SHIFT                            19                                /**< Shift value for VDAC_OPA1DIS */
#define _VDAC_CMD_OPA1DIS_MASK                             0x80000UL                         /**< Bit mask for VDAC_OPA1DIS */
#define _VDAC_CMD_OPA1DIS_DEFAULT                          0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA1DIS_DEFAULT                           (_VDAC_CMD_OPA1DIS_DEFAULT << 19) /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA2EN                                    (0x1UL << 20)                     /**< OPA2 Enable */
#define _VDAC_CMD_OPA2EN_SHIFT                             20                                /**< Shift value for VDAC_OPA2EN */
#define _VDAC_CMD_OPA2EN_MASK                              0x100000UL                        /**< Bit mask for VDAC_OPA2EN */
#define _VDAC_CMD_OPA2EN_DEFAULT                           0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA2EN_DEFAULT                            (_VDAC_CMD_OPA2EN_DEFAULT << 20)  /**< Shifted mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA2DIS                                   (0x1UL << 21)                     /**< OPA2 Disable */
#define _VDAC_CMD_OPA2DIS_SHIFT                            21                                /**< Shift value for VDAC_OPA2DIS */
#define _VDAC_CMD_OPA2DIS_MASK                             0x200000UL                        /**< Bit mask for VDAC_OPA2DIS */
#define _VDAC_CMD_OPA2DIS_DEFAULT                          0x00000000UL                      /**< Mode DEFAULT for VDAC_CMD */
#define VDAC_CMD_OPA2DIS_DEFAULT                           (_VDAC_CMD_OPA2DIS_DEFAULT << 21) /**< Shifted mode DEFAULT for VDAC_CMD */

/* Bit fields for VDAC IF */
#define _VDAC_IF_RESETVALUE                                0x000000C0UL                               /**< Default value for VDAC_IF */
#define _VDAC_IF_MASK                                      0x707780FFUL                               /**< Mask for VDAC_IF */
#define VDAC_IF_CH0CD                                      (0x1UL << 0)                               /**< Channel 0 Conversion Done Interrupt Flag */
#define _VDAC_IF_CH0CD_SHIFT                               0                                          /**< Shift value for VDAC_CH0CD */
#define _VDAC_IF_CH0CD_MASK                                0x1UL                                      /**< Bit mask for VDAC_CH0CD */
#define _VDAC_IF_CH0CD_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH0CD_DEFAULT                              (_VDAC_IF_CH0CD_DEFAULT << 0)              /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH1CD                                      (0x1UL << 1)                               /**< Channel 1 Conversion Done Interrupt Flag */
#define _VDAC_IF_CH1CD_SHIFT                               1                                          /**< Shift value for VDAC_CH1CD */
#define _VDAC_IF_CH1CD_MASK                                0x2UL                                      /**< Bit mask for VDAC_CH1CD */
#define _VDAC_IF_CH1CD_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH1CD_DEFAULT                              (_VDAC_IF_CH1CD_DEFAULT << 1)              /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH0OF                                      (0x1UL << 2)                               /**< Channel 0 Data Overflow Interrupt Flag */
#define _VDAC_IF_CH0OF_SHIFT                               2                                          /**< Shift value for VDAC_CH0OF */
#define _VDAC_IF_CH0OF_MASK                                0x4UL                                      /**< Bit mask for VDAC_CH0OF */
#define _VDAC_IF_CH0OF_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH0OF_DEFAULT                              (_VDAC_IF_CH0OF_DEFAULT << 2)              /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH1OF                                      (0x1UL << 3)                               /**< Channel 1 Data Overflow Interrupt Flag */
#define _VDAC_IF_CH1OF_SHIFT                               3                                          /**< Shift value for VDAC_CH1OF */
#define _VDAC_IF_CH1OF_MASK                                0x8UL                                      /**< Bit mask for VDAC_CH1OF */
#define _VDAC_IF_CH1OF_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH1OF_DEFAULT                              (_VDAC_IF_CH1OF_DEFAULT << 3)              /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH0UF                                      (0x1UL << 4)                               /**< Channel 0 Data Underflow Interrupt Flag */
#define _VDAC_IF_CH0UF_SHIFT                               4                                          /**< Shift value for VDAC_CH0UF */
#define _VDAC_IF_CH0UF_MASK                                0x10UL                                     /**< Bit mask for VDAC_CH0UF */
#define _VDAC_IF_CH0UF_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH0UF_DEFAULT                              (_VDAC_IF_CH0UF_DEFAULT << 4)              /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH1UF                                      (0x1UL << 5)                               /**< Channel 1 Data Underflow Interrupt Flag */
#define _VDAC_IF_CH1UF_SHIFT                               5                                          /**< Shift value for VDAC_CH1UF */
#define _VDAC_IF_CH1UF_MASK                                0x20UL                                     /**< Bit mask for VDAC_CH1UF */
#define _VDAC_IF_CH1UF_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH1UF_DEFAULT                              (_VDAC_IF_CH1UF_DEFAULT << 5)              /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH0BL                                      (0x1UL << 6)                               /**< Channel 0 Buffer Level Interrupt Flag */
#define _VDAC_IF_CH0BL_SHIFT                               6                                          /**< Shift value for VDAC_CH0BL */
#define _VDAC_IF_CH0BL_MASK                                0x40UL                                     /**< Bit mask for VDAC_CH0BL */
#define _VDAC_IF_CH0BL_DEFAULT                             0x00000001UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH0BL_DEFAULT                              (_VDAC_IF_CH0BL_DEFAULT << 6)              /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH1BL                                      (0x1UL << 7)                               /**< Channel 1 Buffer Level Interrupt Flag */
#define _VDAC_IF_CH1BL_SHIFT                               7                                          /**< Shift value for VDAC_CH1BL */
#define _VDAC_IF_CH1BL_MASK                                0x80UL                                     /**< Bit mask for VDAC_CH1BL */
#define _VDAC_IF_CH1BL_DEFAULT                             0x00000001UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_CH1BL_DEFAULT                              (_VDAC_IF_CH1BL_DEFAULT << 7)              /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_EM23ERR                                    (0x1UL << 15)                              /**< EM2/3 Entry Error Flag */
#define _VDAC_IF_EM23ERR_SHIFT                             15                                         /**< Shift value for VDAC_EM23ERR */
#define _VDAC_IF_EM23ERR_MASK                              0x8000UL                                   /**< Bit mask for VDAC_EM23ERR */
#define _VDAC_IF_EM23ERR_DEFAULT                           0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_EM23ERR_DEFAULT                            (_VDAC_IF_EM23ERR_DEFAULT << 15)           /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA0APORTCONFLICT                          (0x1UL << 16)                              /**< OPA0 Bus Conflict Output Interrupt Flag */
#define _VDAC_IF_OPA0APORTCONFLICT_SHIFT                   16                                         /**< Shift value for VDAC_OPA0APORTCONFLICT */
#define _VDAC_IF_OPA0APORTCONFLICT_MASK                    0x10000UL                                  /**< Bit mask for VDAC_OPA0APORTCONFLICT */
#define _VDAC_IF_OPA0APORTCONFLICT_DEFAULT                 0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA0APORTCONFLICT_DEFAULT                  (_VDAC_IF_OPA0APORTCONFLICT_DEFAULT << 16) /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA1APORTCONFLICT                          (0x1UL << 17)                              /**< OPA1 Bus Conflict Output Interrupt Flag */
#define _VDAC_IF_OPA1APORTCONFLICT_SHIFT                   17                                         /**< Shift value for VDAC_OPA1APORTCONFLICT */
#define _VDAC_IF_OPA1APORTCONFLICT_MASK                    0x20000UL                                  /**< Bit mask for VDAC_OPA1APORTCONFLICT */
#define _VDAC_IF_OPA1APORTCONFLICT_DEFAULT                 0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA1APORTCONFLICT_DEFAULT                  (_VDAC_IF_OPA1APORTCONFLICT_DEFAULT << 17) /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA2APORTCONFLICT                          (0x1UL << 18)                              /**< OPA2 Bus Conflict Output Interrupt Flag */
#define _VDAC_IF_OPA2APORTCONFLICT_SHIFT                   18                                         /**< Shift value for VDAC_OPA2APORTCONFLICT */
#define _VDAC_IF_OPA2APORTCONFLICT_MASK                    0x40000UL                                  /**< Bit mask for VDAC_OPA2APORTCONFLICT */
#define _VDAC_IF_OPA2APORTCONFLICT_DEFAULT                 0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA2APORTCONFLICT_DEFAULT                  (_VDAC_IF_OPA2APORTCONFLICT_DEFAULT << 18) /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA0PRSTIMEDERR                            (0x1UL << 20)                              /**< OPA0 PRS Trigger Mode Error Interrupt Flag */
#define _VDAC_IF_OPA0PRSTIMEDERR_SHIFT                     20                                         /**< Shift value for VDAC_OPA0PRSTIMEDERR */
#define _VDAC_IF_OPA0PRSTIMEDERR_MASK                      0x100000UL                                 /**< Bit mask for VDAC_OPA0PRSTIMEDERR */
#define _VDAC_IF_OPA0PRSTIMEDERR_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA0PRSTIMEDERR_DEFAULT                    (_VDAC_IF_OPA0PRSTIMEDERR_DEFAULT << 20)   /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA1PRSTIMEDERR                            (0x1UL << 21)                              /**< OPA1 PRS Trigger Mode Error Interrupt Flag */
#define _VDAC_IF_OPA1PRSTIMEDERR_SHIFT                     21                                         /**< Shift value for VDAC_OPA1PRSTIMEDERR */
#define _VDAC_IF_OPA1PRSTIMEDERR_MASK                      0x200000UL                                 /**< Bit mask for VDAC_OPA1PRSTIMEDERR */
#define _VDAC_IF_OPA1PRSTIMEDERR_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA1PRSTIMEDERR_DEFAULT                    (_VDAC_IF_OPA1PRSTIMEDERR_DEFAULT << 21)   /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA2PRSTIMEDERR                            (0x1UL << 22)                              /**< OPA2 PRS Trigger Mode Error Interrupt Flag */
#define _VDAC_IF_OPA2PRSTIMEDERR_SHIFT                     22                                         /**< Shift value for VDAC_OPA2PRSTIMEDERR */
#define _VDAC_IF_OPA2PRSTIMEDERR_MASK                      0x400000UL                                 /**< Bit mask for VDAC_OPA2PRSTIMEDERR */
#define _VDAC_IF_OPA2PRSTIMEDERR_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA2PRSTIMEDERR_DEFAULT                    (_VDAC_IF_OPA2PRSTIMEDERR_DEFAULT << 22)   /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA0OUTVALID                               (0x1UL << 28)                              /**< OPA0 Output Valid Interrupt Flag */
#define _VDAC_IF_OPA0OUTVALID_SHIFT                        28                                         /**< Shift value for VDAC_OPA0OUTVALID */
#define _VDAC_IF_OPA0OUTVALID_MASK                         0x10000000UL                               /**< Bit mask for VDAC_OPA0OUTVALID */
#define _VDAC_IF_OPA0OUTVALID_DEFAULT                      0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA0OUTVALID_DEFAULT                       (_VDAC_IF_OPA0OUTVALID_DEFAULT << 28)      /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA1OUTVALID                               (0x1UL << 29)                              /**< OPA1 Output Valid Interrupt Flag */
#define _VDAC_IF_OPA1OUTVALID_SHIFT                        29                                         /**< Shift value for VDAC_OPA1OUTVALID */
#define _VDAC_IF_OPA1OUTVALID_MASK                         0x20000000UL                               /**< Bit mask for VDAC_OPA1OUTVALID */
#define _VDAC_IF_OPA1OUTVALID_DEFAULT                      0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA1OUTVALID_DEFAULT                       (_VDAC_IF_OPA1OUTVALID_DEFAULT << 29)      /**< Shifted mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA2OUTVALID                               (0x1UL << 30)                              /**< OPA3 Output Valid Interrupt Flag */
#define _VDAC_IF_OPA2OUTVALID_SHIFT                        30                                         /**< Shift value for VDAC_OPA2OUTVALID */
#define _VDAC_IF_OPA2OUTVALID_MASK                         0x40000000UL                               /**< Bit mask for VDAC_OPA2OUTVALID */
#define _VDAC_IF_OPA2OUTVALID_DEFAULT                      0x00000000UL                               /**< Mode DEFAULT for VDAC_IF */
#define VDAC_IF_OPA2OUTVALID_DEFAULT                       (_VDAC_IF_OPA2OUTVALID_DEFAULT << 30)      /**< Shifted mode DEFAULT for VDAC_IF */

/* Bit fields for VDAC IFS */
#define _VDAC_IFS_RESETVALUE                               0x00000000UL                                /**< Default value for VDAC_IFS */
#define _VDAC_IFS_MASK                                     0x7077803FUL                                /**< Mask for VDAC_IFS */
#define VDAC_IFS_CH0CD                                     (0x1UL << 0)                                /**< Set CH0CD Interrupt Flag */
#define _VDAC_IFS_CH0CD_SHIFT                              0                                           /**< Shift value for VDAC_CH0CD */
#define _VDAC_IFS_CH0CD_MASK                               0x1UL                                       /**< Bit mask for VDAC_CH0CD */
#define _VDAC_IFS_CH0CD_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH0CD_DEFAULT                             (_VDAC_IFS_CH0CD_DEFAULT << 0)              /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH1CD                                     (0x1UL << 1)                                /**< Set CH1CD Interrupt Flag */
#define _VDAC_IFS_CH1CD_SHIFT                              1                                           /**< Shift value for VDAC_CH1CD */
#define _VDAC_IFS_CH1CD_MASK                               0x2UL                                       /**< Bit mask for VDAC_CH1CD */
#define _VDAC_IFS_CH1CD_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH1CD_DEFAULT                             (_VDAC_IFS_CH1CD_DEFAULT << 1)              /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH0OF                                     (0x1UL << 2)                                /**< Set CH0OF Interrupt Flag */
#define _VDAC_IFS_CH0OF_SHIFT                              2                                           /**< Shift value for VDAC_CH0OF */
#define _VDAC_IFS_CH0OF_MASK                               0x4UL                                       /**< Bit mask for VDAC_CH0OF */
#define _VDAC_IFS_CH0OF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH0OF_DEFAULT                             (_VDAC_IFS_CH0OF_DEFAULT << 2)              /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH1OF                                     (0x1UL << 3)                                /**< Set CH1OF Interrupt Flag */
#define _VDAC_IFS_CH1OF_SHIFT                              3                                           /**< Shift value for VDAC_CH1OF */
#define _VDAC_IFS_CH1OF_MASK                               0x8UL                                       /**< Bit mask for VDAC_CH1OF */
#define _VDAC_IFS_CH1OF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH1OF_DEFAULT                             (_VDAC_IFS_CH1OF_DEFAULT << 3)              /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH0UF                                     (0x1UL << 4)                                /**< Set CH0UF Interrupt Flag */
#define _VDAC_IFS_CH0UF_SHIFT                              4                                           /**< Shift value for VDAC_CH0UF */
#define _VDAC_IFS_CH0UF_MASK                               0x10UL                                      /**< Bit mask for VDAC_CH0UF */
#define _VDAC_IFS_CH0UF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH0UF_DEFAULT                             (_VDAC_IFS_CH0UF_DEFAULT << 4)              /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH1UF                                     (0x1UL << 5)                                /**< Set CH1UF Interrupt Flag */
#define _VDAC_IFS_CH1UF_SHIFT                              5                                           /**< Shift value for VDAC_CH1UF */
#define _VDAC_IFS_CH1UF_MASK                               0x20UL                                      /**< Bit mask for VDAC_CH1UF */
#define _VDAC_IFS_CH1UF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_CH1UF_DEFAULT                             (_VDAC_IFS_CH1UF_DEFAULT << 5)              /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_EM23ERR                                   (0x1UL << 15)                               /**< Set EM23ERR Interrupt Flag */
#define _VDAC_IFS_EM23ERR_SHIFT                            15                                          /**< Shift value for VDAC_EM23ERR */
#define _VDAC_IFS_EM23ERR_MASK                             0x8000UL                                    /**< Bit mask for VDAC_EM23ERR */
#define _VDAC_IFS_EM23ERR_DEFAULT                          0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_EM23ERR_DEFAULT                           (_VDAC_IFS_EM23ERR_DEFAULT << 15)           /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA0APORTCONFLICT                         (0x1UL << 16)                               /**< Set OPA0APORTCONFLICT Interrupt Flag */
#define _VDAC_IFS_OPA0APORTCONFLICT_SHIFT                  16                                          /**< Shift value for VDAC_OPA0APORTCONFLICT */
#define _VDAC_IFS_OPA0APORTCONFLICT_MASK                   0x10000UL                                   /**< Bit mask for VDAC_OPA0APORTCONFLICT */
#define _VDAC_IFS_OPA0APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA0APORTCONFLICT_DEFAULT                 (_VDAC_IFS_OPA0APORTCONFLICT_DEFAULT << 16) /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA1APORTCONFLICT                         (0x1UL << 17)                               /**< Set OPA1APORTCONFLICT Interrupt Flag */
#define _VDAC_IFS_OPA1APORTCONFLICT_SHIFT                  17                                          /**< Shift value for VDAC_OPA1APORTCONFLICT */
#define _VDAC_IFS_OPA1APORTCONFLICT_MASK                   0x20000UL                                   /**< Bit mask for VDAC_OPA1APORTCONFLICT */
#define _VDAC_IFS_OPA1APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA1APORTCONFLICT_DEFAULT                 (_VDAC_IFS_OPA1APORTCONFLICT_DEFAULT << 17) /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA2APORTCONFLICT                         (0x1UL << 18)                               /**< Set OPA2APORTCONFLICT Interrupt Flag */
#define _VDAC_IFS_OPA2APORTCONFLICT_SHIFT                  18                                          /**< Shift value for VDAC_OPA2APORTCONFLICT */
#define _VDAC_IFS_OPA2APORTCONFLICT_MASK                   0x40000UL                                   /**< Bit mask for VDAC_OPA2APORTCONFLICT */
#define _VDAC_IFS_OPA2APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA2APORTCONFLICT_DEFAULT                 (_VDAC_IFS_OPA2APORTCONFLICT_DEFAULT << 18) /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA0PRSTIMEDERR                           (0x1UL << 20)                               /**< Set OPA0PRSTIMEDERR Interrupt Flag */
#define _VDAC_IFS_OPA0PRSTIMEDERR_SHIFT                    20                                          /**< Shift value for VDAC_OPA0PRSTIMEDERR */
#define _VDAC_IFS_OPA0PRSTIMEDERR_MASK                     0x100000UL                                  /**< Bit mask for VDAC_OPA0PRSTIMEDERR */
#define _VDAC_IFS_OPA0PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA0PRSTIMEDERR_DEFAULT                   (_VDAC_IFS_OPA0PRSTIMEDERR_DEFAULT << 20)   /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA1PRSTIMEDERR                           (0x1UL << 21)                               /**< Set OPA1PRSTIMEDERR Interrupt Flag */
#define _VDAC_IFS_OPA1PRSTIMEDERR_SHIFT                    21                                          /**< Shift value for VDAC_OPA1PRSTIMEDERR */
#define _VDAC_IFS_OPA1PRSTIMEDERR_MASK                     0x200000UL                                  /**< Bit mask for VDAC_OPA1PRSTIMEDERR */
#define _VDAC_IFS_OPA1PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA1PRSTIMEDERR_DEFAULT                   (_VDAC_IFS_OPA1PRSTIMEDERR_DEFAULT << 21)   /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA2PRSTIMEDERR                           (0x1UL << 22)                               /**< Set OPA2PRSTIMEDERR Interrupt Flag */
#define _VDAC_IFS_OPA2PRSTIMEDERR_SHIFT                    22                                          /**< Shift value for VDAC_OPA2PRSTIMEDERR */
#define _VDAC_IFS_OPA2PRSTIMEDERR_MASK                     0x400000UL                                  /**< Bit mask for VDAC_OPA2PRSTIMEDERR */
#define _VDAC_IFS_OPA2PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA2PRSTIMEDERR_DEFAULT                   (_VDAC_IFS_OPA2PRSTIMEDERR_DEFAULT << 22)   /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA0OUTVALID                              (0x1UL << 28)                               /**< Set OPA0OUTVALID Interrupt Flag */
#define _VDAC_IFS_OPA0OUTVALID_SHIFT                       28                                          /**< Shift value for VDAC_OPA0OUTVALID */
#define _VDAC_IFS_OPA0OUTVALID_MASK                        0x10000000UL                                /**< Bit mask for VDAC_OPA0OUTVALID */
#define _VDAC_IFS_OPA0OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA0OUTVALID_DEFAULT                      (_VDAC_IFS_OPA0OUTVALID_DEFAULT << 28)      /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA1OUTVALID                              (0x1UL << 29)                               /**< Set OPA1OUTVALID Interrupt Flag */
#define _VDAC_IFS_OPA1OUTVALID_SHIFT                       29                                          /**< Shift value for VDAC_OPA1OUTVALID */
#define _VDAC_IFS_OPA1OUTVALID_MASK                        0x20000000UL                                /**< Bit mask for VDAC_OPA1OUTVALID */
#define _VDAC_IFS_OPA1OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA1OUTVALID_DEFAULT                      (_VDAC_IFS_OPA1OUTVALID_DEFAULT << 29)      /**< Shifted mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA2OUTVALID                              (0x1UL << 30)                               /**< Set OPA2OUTVALID Interrupt Flag */
#define _VDAC_IFS_OPA2OUTVALID_SHIFT                       30                                          /**< Shift value for VDAC_OPA2OUTVALID */
#define _VDAC_IFS_OPA2OUTVALID_MASK                        0x40000000UL                                /**< Bit mask for VDAC_OPA2OUTVALID */
#define _VDAC_IFS_OPA2OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IFS */
#define VDAC_IFS_OPA2OUTVALID_DEFAULT                      (_VDAC_IFS_OPA2OUTVALID_DEFAULT << 30)      /**< Shifted mode DEFAULT for VDAC_IFS */

/* Bit fields for VDAC IFC */
#define _VDAC_IFC_RESETVALUE                               0x00000000UL                                /**< Default value for VDAC_IFC */
#define _VDAC_IFC_MASK                                     0x7077803FUL                                /**< Mask for VDAC_IFC */
#define VDAC_IFC_CH0CD                                     (0x1UL << 0)                                /**< Clear CH0CD Interrupt Flag */
#define _VDAC_IFC_CH0CD_SHIFT                              0                                           /**< Shift value for VDAC_CH0CD */
#define _VDAC_IFC_CH0CD_MASK                               0x1UL                                       /**< Bit mask for VDAC_CH0CD */
#define _VDAC_IFC_CH0CD_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH0CD_DEFAULT                             (_VDAC_IFC_CH0CD_DEFAULT << 0)              /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH1CD                                     (0x1UL << 1)                                /**< Clear CH1CD Interrupt Flag */
#define _VDAC_IFC_CH1CD_SHIFT                              1                                           /**< Shift value for VDAC_CH1CD */
#define _VDAC_IFC_CH1CD_MASK                               0x2UL                                       /**< Bit mask for VDAC_CH1CD */
#define _VDAC_IFC_CH1CD_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH1CD_DEFAULT                             (_VDAC_IFC_CH1CD_DEFAULT << 1)              /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH0OF                                     (0x1UL << 2)                                /**< Clear CH0OF Interrupt Flag */
#define _VDAC_IFC_CH0OF_SHIFT                              2                                           /**< Shift value for VDAC_CH0OF */
#define _VDAC_IFC_CH0OF_MASK                               0x4UL                                       /**< Bit mask for VDAC_CH0OF */
#define _VDAC_IFC_CH0OF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH0OF_DEFAULT                             (_VDAC_IFC_CH0OF_DEFAULT << 2)              /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH1OF                                     (0x1UL << 3)                                /**< Clear CH1OF Interrupt Flag */
#define _VDAC_IFC_CH1OF_SHIFT                              3                                           /**< Shift value for VDAC_CH1OF */
#define _VDAC_IFC_CH1OF_MASK                               0x8UL                                       /**< Bit mask for VDAC_CH1OF */
#define _VDAC_IFC_CH1OF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH1OF_DEFAULT                             (_VDAC_IFC_CH1OF_DEFAULT << 3)              /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH0UF                                     (0x1UL << 4)                                /**< Clear CH0UF Interrupt Flag */
#define _VDAC_IFC_CH0UF_SHIFT                              4                                           /**< Shift value for VDAC_CH0UF */
#define _VDAC_IFC_CH0UF_MASK                               0x10UL                                      /**< Bit mask for VDAC_CH0UF */
#define _VDAC_IFC_CH0UF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH0UF_DEFAULT                             (_VDAC_IFC_CH0UF_DEFAULT << 4)              /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH1UF                                     (0x1UL << 5)                                /**< Clear CH1UF Interrupt Flag */
#define _VDAC_IFC_CH1UF_SHIFT                              5                                           /**< Shift value for VDAC_CH1UF */
#define _VDAC_IFC_CH1UF_MASK                               0x20UL                                      /**< Bit mask for VDAC_CH1UF */
#define _VDAC_IFC_CH1UF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_CH1UF_DEFAULT                             (_VDAC_IFC_CH1UF_DEFAULT << 5)              /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_EM23ERR                                   (0x1UL << 15)                               /**< Clear EM23ERR Interrupt Flag */
#define _VDAC_IFC_EM23ERR_SHIFT                            15                                          /**< Shift value for VDAC_EM23ERR */
#define _VDAC_IFC_EM23ERR_MASK                             0x8000UL                                    /**< Bit mask for VDAC_EM23ERR */
#define _VDAC_IFC_EM23ERR_DEFAULT                          0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_EM23ERR_DEFAULT                           (_VDAC_IFC_EM23ERR_DEFAULT << 15)           /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA0APORTCONFLICT                         (0x1UL << 16)                               /**< Clear OPA0APORTCONFLICT Interrupt Flag */
#define _VDAC_IFC_OPA0APORTCONFLICT_SHIFT                  16                                          /**< Shift value for VDAC_OPA0APORTCONFLICT */
#define _VDAC_IFC_OPA0APORTCONFLICT_MASK                   0x10000UL                                   /**< Bit mask for VDAC_OPA0APORTCONFLICT */
#define _VDAC_IFC_OPA0APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA0APORTCONFLICT_DEFAULT                 (_VDAC_IFC_OPA0APORTCONFLICT_DEFAULT << 16) /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA1APORTCONFLICT                         (0x1UL << 17)                               /**< Clear OPA1APORTCONFLICT Interrupt Flag */
#define _VDAC_IFC_OPA1APORTCONFLICT_SHIFT                  17                                          /**< Shift value for VDAC_OPA1APORTCONFLICT */
#define _VDAC_IFC_OPA1APORTCONFLICT_MASK                   0x20000UL                                   /**< Bit mask for VDAC_OPA1APORTCONFLICT */
#define _VDAC_IFC_OPA1APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA1APORTCONFLICT_DEFAULT                 (_VDAC_IFC_OPA1APORTCONFLICT_DEFAULT << 17) /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA2APORTCONFLICT                         (0x1UL << 18)                               /**< Clear OPA2APORTCONFLICT Interrupt Flag */
#define _VDAC_IFC_OPA2APORTCONFLICT_SHIFT                  18                                          /**< Shift value for VDAC_OPA2APORTCONFLICT */
#define _VDAC_IFC_OPA2APORTCONFLICT_MASK                   0x40000UL                                   /**< Bit mask for VDAC_OPA2APORTCONFLICT */
#define _VDAC_IFC_OPA2APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA2APORTCONFLICT_DEFAULT                 (_VDAC_IFC_OPA2APORTCONFLICT_DEFAULT << 18) /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA0PRSTIMEDERR                           (0x1UL << 20)                               /**< Clear OPA0PRSTIMEDERR Interrupt Flag */
#define _VDAC_IFC_OPA0PRSTIMEDERR_SHIFT                    20                                          /**< Shift value for VDAC_OPA0PRSTIMEDERR */
#define _VDAC_IFC_OPA0PRSTIMEDERR_MASK                     0x100000UL                                  /**< Bit mask for VDAC_OPA0PRSTIMEDERR */
#define _VDAC_IFC_OPA0PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA0PRSTIMEDERR_DEFAULT                   (_VDAC_IFC_OPA0PRSTIMEDERR_DEFAULT << 20)   /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA1PRSTIMEDERR                           (0x1UL << 21)                               /**< Clear OPA1PRSTIMEDERR Interrupt Flag */
#define _VDAC_IFC_OPA1PRSTIMEDERR_SHIFT                    21                                          /**< Shift value for VDAC_OPA1PRSTIMEDERR */
#define _VDAC_IFC_OPA1PRSTIMEDERR_MASK                     0x200000UL                                  /**< Bit mask for VDAC_OPA1PRSTIMEDERR */
#define _VDAC_IFC_OPA1PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA1PRSTIMEDERR_DEFAULT                   (_VDAC_IFC_OPA1PRSTIMEDERR_DEFAULT << 21)   /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA2PRSTIMEDERR                           (0x1UL << 22)                               /**< Clear OPA2PRSTIMEDERR Interrupt Flag */
#define _VDAC_IFC_OPA2PRSTIMEDERR_SHIFT                    22                                          /**< Shift value for VDAC_OPA2PRSTIMEDERR */
#define _VDAC_IFC_OPA2PRSTIMEDERR_MASK                     0x400000UL                                  /**< Bit mask for VDAC_OPA2PRSTIMEDERR */
#define _VDAC_IFC_OPA2PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA2PRSTIMEDERR_DEFAULT                   (_VDAC_IFC_OPA2PRSTIMEDERR_DEFAULT << 22)   /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA0OUTVALID                              (0x1UL << 28)                               /**< Clear OPA0OUTVALID Interrupt Flag */
#define _VDAC_IFC_OPA0OUTVALID_SHIFT                       28                                          /**< Shift value for VDAC_OPA0OUTVALID */
#define _VDAC_IFC_OPA0OUTVALID_MASK                        0x10000000UL                                /**< Bit mask for VDAC_OPA0OUTVALID */
#define _VDAC_IFC_OPA0OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA0OUTVALID_DEFAULT                      (_VDAC_IFC_OPA0OUTVALID_DEFAULT << 28)      /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA1OUTVALID                              (0x1UL << 29)                               /**< Clear OPA1OUTVALID Interrupt Flag */
#define _VDAC_IFC_OPA1OUTVALID_SHIFT                       29                                          /**< Shift value for VDAC_OPA1OUTVALID */
#define _VDAC_IFC_OPA1OUTVALID_MASK                        0x20000000UL                                /**< Bit mask for VDAC_OPA1OUTVALID */
#define _VDAC_IFC_OPA1OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA1OUTVALID_DEFAULT                      (_VDAC_IFC_OPA1OUTVALID_DEFAULT << 29)      /**< Shifted mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA2OUTVALID                              (0x1UL << 30)                               /**< Clear OPA2OUTVALID Interrupt Flag */
#define _VDAC_IFC_OPA2OUTVALID_SHIFT                       30                                          /**< Shift value for VDAC_OPA2OUTVALID */
#define _VDAC_IFC_OPA2OUTVALID_MASK                        0x40000000UL                                /**< Bit mask for VDAC_OPA2OUTVALID */
#define _VDAC_IFC_OPA2OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IFC */
#define VDAC_IFC_OPA2OUTVALID_DEFAULT                      (_VDAC_IFC_OPA2OUTVALID_DEFAULT << 30)      /**< Shifted mode DEFAULT for VDAC_IFC */

/* Bit fields for VDAC IEN */
#define _VDAC_IEN_RESETVALUE                               0x00000000UL                                /**< Default value for VDAC_IEN */
#define _VDAC_IEN_MASK                                     0x707780FFUL                                /**< Mask for VDAC_IEN */
#define VDAC_IEN_CH0CD                                     (0x1UL << 0)                                /**< CH0CD Interrupt Enable */
#define _VDAC_IEN_CH0CD_SHIFT                              0                                           /**< Shift value for VDAC_CH0CD */
#define _VDAC_IEN_CH0CD_MASK                               0x1UL                                       /**< Bit mask for VDAC_CH0CD */
#define _VDAC_IEN_CH0CD_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH0CD_DEFAULT                             (_VDAC_IEN_CH0CD_DEFAULT << 0)              /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH1CD                                     (0x1UL << 1)                                /**< CH1CD Interrupt Enable */
#define _VDAC_IEN_CH1CD_SHIFT                              1                                           /**< Shift value for VDAC_CH1CD */
#define _VDAC_IEN_CH1CD_MASK                               0x2UL                                       /**< Bit mask for VDAC_CH1CD */
#define _VDAC_IEN_CH1CD_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH1CD_DEFAULT                             (_VDAC_IEN_CH1CD_DEFAULT << 1)              /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH0OF                                     (0x1UL << 2)                                /**< CH0OF Interrupt Enable */
#define _VDAC_IEN_CH0OF_SHIFT                              2                                           /**< Shift value for VDAC_CH0OF */
#define _VDAC_IEN_CH0OF_MASK                               0x4UL                                       /**< Bit mask for VDAC_CH0OF */
#define _VDAC_IEN_CH0OF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH0OF_DEFAULT                             (_VDAC_IEN_CH0OF_DEFAULT << 2)              /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH1OF                                     (0x1UL << 3)                                /**< CH1OF Interrupt Enable */
#define _VDAC_IEN_CH1OF_SHIFT                              3                                           /**< Shift value for VDAC_CH1OF */
#define _VDAC_IEN_CH1OF_MASK                               0x8UL                                       /**< Bit mask for VDAC_CH1OF */
#define _VDAC_IEN_CH1OF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH1OF_DEFAULT                             (_VDAC_IEN_CH1OF_DEFAULT << 3)              /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH0UF                                     (0x1UL << 4)                                /**< CH0UF Interrupt Enable */
#define _VDAC_IEN_CH0UF_SHIFT                              4                                           /**< Shift value for VDAC_CH0UF */
#define _VDAC_IEN_CH0UF_MASK                               0x10UL                                      /**< Bit mask for VDAC_CH0UF */
#define _VDAC_IEN_CH0UF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH0UF_DEFAULT                             (_VDAC_IEN_CH0UF_DEFAULT << 4)              /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH1UF                                     (0x1UL << 5)                                /**< CH1UF Interrupt Enable */
#define _VDAC_IEN_CH1UF_SHIFT                              5                                           /**< Shift value for VDAC_CH1UF */
#define _VDAC_IEN_CH1UF_MASK                               0x20UL                                      /**< Bit mask for VDAC_CH1UF */
#define _VDAC_IEN_CH1UF_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH1UF_DEFAULT                             (_VDAC_IEN_CH1UF_DEFAULT << 5)              /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH0BL                                     (0x1UL << 6)                                /**< CH0BL Interrupt Enable */
#define _VDAC_IEN_CH0BL_SHIFT                              6                                           /**< Shift value for VDAC_CH0BL */
#define _VDAC_IEN_CH0BL_MASK                               0x40UL                                      /**< Bit mask for VDAC_CH0BL */
#define _VDAC_IEN_CH0BL_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH0BL_DEFAULT                             (_VDAC_IEN_CH0BL_DEFAULT << 6)              /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH1BL                                     (0x1UL << 7)                                /**< CH1BL Interrupt Enable */
#define _VDAC_IEN_CH1BL_SHIFT                              7                                           /**< Shift value for VDAC_CH1BL */
#define _VDAC_IEN_CH1BL_MASK                               0x80UL                                      /**< Bit mask for VDAC_CH1BL */
#define _VDAC_IEN_CH1BL_DEFAULT                            0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_CH1BL_DEFAULT                             (_VDAC_IEN_CH1BL_DEFAULT << 7)              /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_EM23ERR                                   (0x1UL << 15)                               /**< EM23ERR Interrupt Enable */
#define _VDAC_IEN_EM23ERR_SHIFT                            15                                          /**< Shift value for VDAC_EM23ERR */
#define _VDAC_IEN_EM23ERR_MASK                             0x8000UL                                    /**< Bit mask for VDAC_EM23ERR */
#define _VDAC_IEN_EM23ERR_DEFAULT                          0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_EM23ERR_DEFAULT                           (_VDAC_IEN_EM23ERR_DEFAULT << 15)           /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA0APORTCONFLICT                         (0x1UL << 16)                               /**< OPA0APORTCONFLICT Interrupt Enable */
#define _VDAC_IEN_OPA0APORTCONFLICT_SHIFT                  16                                          /**< Shift value for VDAC_OPA0APORTCONFLICT */
#define _VDAC_IEN_OPA0APORTCONFLICT_MASK                   0x10000UL                                   /**< Bit mask for VDAC_OPA0APORTCONFLICT */
#define _VDAC_IEN_OPA0APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA0APORTCONFLICT_DEFAULT                 (_VDAC_IEN_OPA0APORTCONFLICT_DEFAULT << 16) /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA1APORTCONFLICT                         (0x1UL << 17)                               /**< OPA1APORTCONFLICT Interrupt Enable */
#define _VDAC_IEN_OPA1APORTCONFLICT_SHIFT                  17                                          /**< Shift value for VDAC_OPA1APORTCONFLICT */
#define _VDAC_IEN_OPA1APORTCONFLICT_MASK                   0x20000UL                                   /**< Bit mask for VDAC_OPA1APORTCONFLICT */
#define _VDAC_IEN_OPA1APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA1APORTCONFLICT_DEFAULT                 (_VDAC_IEN_OPA1APORTCONFLICT_DEFAULT << 17) /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA2APORTCONFLICT                         (0x1UL << 18)                               /**< OPA2APORTCONFLICT Interrupt Enable */
#define _VDAC_IEN_OPA2APORTCONFLICT_SHIFT                  18                                          /**< Shift value for VDAC_OPA2APORTCONFLICT */
#define _VDAC_IEN_OPA2APORTCONFLICT_MASK                   0x40000UL                                   /**< Bit mask for VDAC_OPA2APORTCONFLICT */
#define _VDAC_IEN_OPA2APORTCONFLICT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA2APORTCONFLICT_DEFAULT                 (_VDAC_IEN_OPA2APORTCONFLICT_DEFAULT << 18) /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA0PRSTIMEDERR                           (0x1UL << 20)                               /**< OPA0PRSTIMEDERR Interrupt Enable */
#define _VDAC_IEN_OPA0PRSTIMEDERR_SHIFT                    20                                          /**< Shift value for VDAC_OPA0PRSTIMEDERR */
#define _VDAC_IEN_OPA0PRSTIMEDERR_MASK                     0x100000UL                                  /**< Bit mask for VDAC_OPA0PRSTIMEDERR */
#define _VDAC_IEN_OPA0PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA0PRSTIMEDERR_DEFAULT                   (_VDAC_IEN_OPA0PRSTIMEDERR_DEFAULT << 20)   /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA1PRSTIMEDERR                           (0x1UL << 21)                               /**< OPA1PRSTIMEDERR Interrupt Enable */
#define _VDAC_IEN_OPA1PRSTIMEDERR_SHIFT                    21                                          /**< Shift value for VDAC_OPA1PRSTIMEDERR */
#define _VDAC_IEN_OPA1PRSTIMEDERR_MASK                     0x200000UL                                  /**< Bit mask for VDAC_OPA1PRSTIMEDERR */
#define _VDAC_IEN_OPA1PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA1PRSTIMEDERR_DEFAULT                   (_VDAC_IEN_OPA1PRSTIMEDERR_DEFAULT << 21)   /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA2PRSTIMEDERR                           (0x1UL << 22)                               /**< OPA2PRSTIMEDERR Interrupt Enable */
#define _VDAC_IEN_OPA2PRSTIMEDERR_SHIFT                    22                                          /**< Shift value for VDAC_OPA2PRSTIMEDERR */
#define _VDAC_IEN_OPA2PRSTIMEDERR_MASK                     0x400000UL                                  /**< Bit mask for VDAC_OPA2PRSTIMEDERR */
#define _VDAC_IEN_OPA2PRSTIMEDERR_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA2PRSTIMEDERR_DEFAULT                   (_VDAC_IEN_OPA2PRSTIMEDERR_DEFAULT << 22)   /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA0OUTVALID                              (0x1UL << 28)                               /**< OPA0OUTVALID Interrupt Enable */
#define _VDAC_IEN_OPA0OUTVALID_SHIFT                       28                                          /**< Shift value for VDAC_OPA0OUTVALID */
#define _VDAC_IEN_OPA0OUTVALID_MASK                        0x10000000UL                                /**< Bit mask for VDAC_OPA0OUTVALID */
#define _VDAC_IEN_OPA0OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA0OUTVALID_DEFAULT                      (_VDAC_IEN_OPA0OUTVALID_DEFAULT << 28)      /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA1OUTVALID                              (0x1UL << 29)                               /**< OPA1OUTVALID Interrupt Enable */
#define _VDAC_IEN_OPA1OUTVALID_SHIFT                       29                                          /**< Shift value for VDAC_OPA1OUTVALID */
#define _VDAC_IEN_OPA1OUTVALID_MASK                        0x20000000UL                                /**< Bit mask for VDAC_OPA1OUTVALID */
#define _VDAC_IEN_OPA1OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA1OUTVALID_DEFAULT                      (_VDAC_IEN_OPA1OUTVALID_DEFAULT << 29)      /**< Shifted mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA2OUTVALID                              (0x1UL << 30)                               /**< OPA2OUTVALID Interrupt Enable */
#define _VDAC_IEN_OPA2OUTVALID_SHIFT                       30                                          /**< Shift value for VDAC_OPA2OUTVALID */
#define _VDAC_IEN_OPA2OUTVALID_MASK                        0x40000000UL                                /**< Bit mask for VDAC_OPA2OUTVALID */
#define _VDAC_IEN_OPA2OUTVALID_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for VDAC_IEN */
#define VDAC_IEN_OPA2OUTVALID_DEFAULT                      (_VDAC_IEN_OPA2OUTVALID_DEFAULT << 30)      /**< Shifted mode DEFAULT for VDAC_IEN */

/* Bit fields for VDAC CH0DATA */
#define _VDAC_CH0DATA_RESETVALUE                           0x00000800UL                      /**< Default value for VDAC_CH0DATA */
#define _VDAC_CH0DATA_MASK                                 0x00000FFFUL                      /**< Mask for VDAC_CH0DATA */
#define _VDAC_CH0DATA_DATA_SHIFT                           0                                 /**< Shift value for VDAC_DATA */
#define _VDAC_CH0DATA_DATA_MASK                            0xFFFUL                           /**< Bit mask for VDAC_DATA */
#define _VDAC_CH0DATA_DATA_DEFAULT                         0x00000800UL                      /**< Mode DEFAULT for VDAC_CH0DATA */
#define VDAC_CH0DATA_DATA_DEFAULT                          (_VDAC_CH0DATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for VDAC_CH0DATA */

/* Bit fields for VDAC CH1DATA */
#define _VDAC_CH1DATA_RESETVALUE                           0x00000800UL                      /**< Default value for VDAC_CH1DATA */
#define _VDAC_CH1DATA_MASK                                 0x00000FFFUL                      /**< Mask for VDAC_CH1DATA */
#define _VDAC_CH1DATA_DATA_SHIFT                           0                                 /**< Shift value for VDAC_DATA */
#define _VDAC_CH1DATA_DATA_MASK                            0xFFFUL                           /**< Bit mask for VDAC_DATA */
#define _VDAC_CH1DATA_DATA_DEFAULT                         0x00000800UL                      /**< Mode DEFAULT for VDAC_CH1DATA */
#define VDAC_CH1DATA_DATA_DEFAULT                          (_VDAC_CH1DATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for VDAC_CH1DATA */

/* Bit fields for VDAC COMBDATA */
#define _VDAC_COMBDATA_RESETVALUE                          0x08000800UL                           /**< Default value for VDAC_COMBDATA */
#define _VDAC_COMBDATA_MASK                                0x0FFF0FFFUL                           /**< Mask for VDAC_COMBDATA */
#define _VDAC_COMBDATA_CH0DATA_SHIFT                       0                                      /**< Shift value for VDAC_CH0DATA */
#define _VDAC_COMBDATA_CH0DATA_MASK                        0xFFFUL                                /**< Bit mask for VDAC_CH0DATA */
#define _VDAC_COMBDATA_CH0DATA_DEFAULT                     0x00000800UL                           /**< Mode DEFAULT for VDAC_COMBDATA */
#define VDAC_COMBDATA_CH0DATA_DEFAULT                      (_VDAC_COMBDATA_CH0DATA_DEFAULT << 0)  /**< Shifted mode DEFAULT for VDAC_COMBDATA */
#define _VDAC_COMBDATA_CH1DATA_SHIFT                       16                                     /**< Shift value for VDAC_CH1DATA */
#define _VDAC_COMBDATA_CH1DATA_MASK                        0xFFF0000UL                            /**< Bit mask for VDAC_CH1DATA */
#define _VDAC_COMBDATA_CH1DATA_DEFAULT                     0x00000800UL                           /**< Mode DEFAULT for VDAC_COMBDATA */
#define VDAC_COMBDATA_CH1DATA_DEFAULT                      (_VDAC_COMBDATA_CH1DATA_DEFAULT << 16) /**< Shifted mode DEFAULT for VDAC_COMBDATA */

/* Bit fields for VDAC CAL */
#define _VDAC_CAL_RESETVALUE                               0x00082004UL                             /**< Default value for VDAC_CAL */
#define _VDAC_CAL_MASK                                     0x000F3F07UL                             /**< Mask for VDAC_CAL */
#define _VDAC_CAL_OFFSETTRIM_SHIFT                         0                                        /**< Shift value for VDAC_OFFSETTRIM */
#define _VDAC_CAL_OFFSETTRIM_MASK                          0x7UL                                    /**< Bit mask for VDAC_OFFSETTRIM */
#define _VDAC_CAL_OFFSETTRIM_DEFAULT                       0x00000004UL                             /**< Mode DEFAULT for VDAC_CAL */
#define VDAC_CAL_OFFSETTRIM_DEFAULT                        (_VDAC_CAL_OFFSETTRIM_DEFAULT << 0)      /**< Shifted mode DEFAULT for VDAC_CAL */
#define _VDAC_CAL_GAINERRTRIM_SHIFT                        8                                        /**< Shift value for VDAC_GAINERRTRIM */
#define _VDAC_CAL_GAINERRTRIM_MASK                         0x3F00UL                                 /**< Bit mask for VDAC_GAINERRTRIM */
#define _VDAC_CAL_GAINERRTRIM_DEFAULT                      0x00000020UL                             /**< Mode DEFAULT for VDAC_CAL */
#define VDAC_CAL_GAINERRTRIM_DEFAULT                       (_VDAC_CAL_GAINERRTRIM_DEFAULT << 8)     /**< Shifted mode DEFAULT for VDAC_CAL */
#define _VDAC_CAL_GAINERRTRIMCH1_SHIFT                     16                                       /**< Shift value for VDAC_GAINERRTRIMCH1 */
#define _VDAC_CAL_GAINERRTRIMCH1_MASK                      0xF0000UL                                /**< Bit mask for VDAC_GAINERRTRIMCH1 */
#define _VDAC_CAL_GAINERRTRIMCH1_DEFAULT                   0x00000008UL                             /**< Mode DEFAULT for VDAC_CAL */
#define VDAC_CAL_GAINERRTRIMCH1_DEFAULT                    (_VDAC_CAL_GAINERRTRIMCH1_DEFAULT << 16) /**< Shifted mode DEFAULT for VDAC_CAL */

/* Bit fields for VDAC OPA_APORTREQ */
#define _VDAC_OPA_APORTREQ_RESETVALUE                      0x00000000UL                                 /**< Default value for VDAC_OPA_APORTREQ */
#define _VDAC_OPA_APORTREQ_MASK                            0x000003FCUL                                 /**< Mask for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT1XREQ                       (0x1UL << 2)                                 /**< 1 If the Bus Connected to APORT2X is Requested */
#define _VDAC_OPA_APORTREQ_APORT1XREQ_SHIFT                2                                            /**< Shift value for VDAC_OPAAPORT1XREQ */
#define _VDAC_OPA_APORTREQ_APORT1XREQ_MASK                 0x4UL                                        /**< Bit mask for VDAC_OPAAPORT1XREQ */
#define _VDAC_OPA_APORTREQ_APORT1XREQ_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT1XREQ_DEFAULT               (_VDAC_OPA_APORTREQ_APORT1XREQ_DEFAULT << 2) /**< Shifted mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT1YREQ                       (0x1UL << 3)                                 /**< 1 If the Bus Connected to APORT1X is Requested */
#define _VDAC_OPA_APORTREQ_APORT1YREQ_SHIFT                3                                            /**< Shift value for VDAC_OPAAPORT1YREQ */
#define _VDAC_OPA_APORTREQ_APORT1YREQ_MASK                 0x8UL                                        /**< Bit mask for VDAC_OPAAPORT1YREQ */
#define _VDAC_OPA_APORTREQ_APORT1YREQ_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT1YREQ_DEFAULT               (_VDAC_OPA_APORTREQ_APORT1YREQ_DEFAULT << 3) /**< Shifted mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT2XREQ                       (0x1UL << 4)                                 /**< 1 If the Bus Connected to APORT2X is Requested */
#define _VDAC_OPA_APORTREQ_APORT2XREQ_SHIFT                4                                            /**< Shift value for VDAC_OPAAPORT2XREQ */
#define _VDAC_OPA_APORTREQ_APORT2XREQ_MASK                 0x10UL                                       /**< Bit mask for VDAC_OPAAPORT2XREQ */
#define _VDAC_OPA_APORTREQ_APORT2XREQ_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT2XREQ_DEFAULT               (_VDAC_OPA_APORTREQ_APORT2XREQ_DEFAULT << 4) /**< Shifted mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT2YREQ                       (0x1UL << 5)                                 /**< 1 If the Bus Connected to APORT2Y is Requested */
#define _VDAC_OPA_APORTREQ_APORT2YREQ_SHIFT                5                                            /**< Shift value for VDAC_OPAAPORT2YREQ */
#define _VDAC_OPA_APORTREQ_APORT2YREQ_MASK                 0x20UL                                       /**< Bit mask for VDAC_OPAAPORT2YREQ */
#define _VDAC_OPA_APORTREQ_APORT2YREQ_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT2YREQ_DEFAULT               (_VDAC_OPA_APORTREQ_APORT2YREQ_DEFAULT << 5) /**< Shifted mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT3XREQ                       (0x1UL << 6)                                 /**< 1 If the Bus Connected to APORT3X is Requested */
#define _VDAC_OPA_APORTREQ_APORT3XREQ_SHIFT                6                                            /**< Shift value for VDAC_OPAAPORT3XREQ */
#define _VDAC_OPA_APORTREQ_APORT3XREQ_MASK                 0x40UL                                       /**< Bit mask for VDAC_OPAAPORT3XREQ */
#define _VDAC_OPA_APORTREQ_APORT3XREQ_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT3XREQ_DEFAULT               (_VDAC_OPA_APORTREQ_APORT3XREQ_DEFAULT << 6) /**< Shifted mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT3YREQ                       (0x1UL << 7)                                 /**< 1 If the Bus Connected to APORT3Y is Requested */
#define _VDAC_OPA_APORTREQ_APORT3YREQ_SHIFT                7                                            /**< Shift value for VDAC_OPAAPORT3YREQ */
#define _VDAC_OPA_APORTREQ_APORT3YREQ_MASK                 0x80UL                                       /**< Bit mask for VDAC_OPAAPORT3YREQ */
#define _VDAC_OPA_APORTREQ_APORT3YREQ_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT3YREQ_DEFAULT               (_VDAC_OPA_APORTREQ_APORT3YREQ_DEFAULT << 7) /**< Shifted mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT4XREQ                       (0x1UL << 8)                                 /**< 1 If the Bus Connected to APORT4X is Requested */
#define _VDAC_OPA_APORTREQ_APORT4XREQ_SHIFT                8                                            /**< Shift value for VDAC_OPAAPORT4XREQ */
#define _VDAC_OPA_APORTREQ_APORT4XREQ_MASK                 0x100UL                                      /**< Bit mask for VDAC_OPAAPORT4XREQ */
#define _VDAC_OPA_APORTREQ_APORT4XREQ_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT4XREQ_DEFAULT               (_VDAC_OPA_APORTREQ_APORT4XREQ_DEFAULT << 8) /**< Shifted mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT4YREQ                       (0x1UL << 9)                                 /**< 1 If the Bus Connected to APORT4Y is Requested */
#define _VDAC_OPA_APORTREQ_APORT4YREQ_SHIFT                9                                            /**< Shift value for VDAC_OPAAPORT4YREQ */
#define _VDAC_OPA_APORTREQ_APORT4YREQ_MASK                 0x200UL                                      /**< Bit mask for VDAC_OPAAPORT4YREQ */
#define _VDAC_OPA_APORTREQ_APORT4YREQ_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for VDAC_OPA_APORTREQ */
#define VDAC_OPA_APORTREQ_APORT4YREQ_DEFAULT               (_VDAC_OPA_APORTREQ_APORT4YREQ_DEFAULT << 9) /**< Shifted mode DEFAULT for VDAC_OPA_APORTREQ */

/* Bit fields for VDAC OPA_APORTCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_RESETVALUE                 0x00000000UL                                           /**< Default value for VDAC_OPA_APORTCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_MASK                       0x000003FCUL                                           /**< Mask for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT1XCONFLICT             (0x1UL << 2)                                           /**< 1 If the Bus Connected to APORT1X is in Conflict With Another Peripheral */
#define _VDAC_OPA_APORTCONFLICT_APORT1XCONFLICT_SHIFT      2                                                      /**< Shift value for VDAC_OPAAPORT1XCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT1XCONFLICT_MASK       0x4UL                                                  /**< Bit mask for VDAC_OPAAPORT1XCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT1XCONFLICT_DEFAULT    0x00000000UL                                           /**< Mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT1XCONFLICT_DEFAULT     (_VDAC_OPA_APORTCONFLICT_APORT1XCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT1YCONFLICT             (0x1UL << 3)                                           /**< 1 If the Bus Connected to APORT1X is in Conflict With Another Peripheral */
#define _VDAC_OPA_APORTCONFLICT_APORT1YCONFLICT_SHIFT      3                                                      /**< Shift value for VDAC_OPAAPORT1YCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT1YCONFLICT_MASK       0x8UL                                                  /**< Bit mask for VDAC_OPAAPORT1YCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT1YCONFLICT_DEFAULT    0x00000000UL                                           /**< Mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT1YCONFLICT_DEFAULT     (_VDAC_OPA_APORTCONFLICT_APORT1YCONFLICT_DEFAULT << 3) /**< Shifted mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT2XCONFLICT             (0x1UL << 4)                                           /**< 1 If the Bus Connected to APORT2X is in Conflict With Another Peripheral */
#define _VDAC_OPA_APORTCONFLICT_APORT2XCONFLICT_SHIFT      4                                                      /**< Shift value for VDAC_OPAAPORT2XCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT2XCONFLICT_MASK       0x10UL                                                 /**< Bit mask for VDAC_OPAAPORT2XCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT2XCONFLICT_DEFAULT    0x00000000UL                                           /**< Mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT2XCONFLICT_DEFAULT     (_VDAC_OPA_APORTCONFLICT_APORT2XCONFLICT_DEFAULT << 4) /**< Shifted mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT2YCONFLICT             (0x1UL << 5)                                           /**< 1 If the Bus Connected to APORT2Y is in Conflict With Another Peripheral */
#define _VDAC_OPA_APORTCONFLICT_APORT2YCONFLICT_SHIFT      5                                                      /**< Shift value for VDAC_OPAAPORT2YCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT2YCONFLICT_MASK       0x20UL                                                 /**< Bit mask for VDAC_OPAAPORT2YCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT2YCONFLICT_DEFAULT    0x00000000UL                                           /**< Mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT2YCONFLICT_DEFAULT     (_VDAC_OPA_APORTCONFLICT_APORT2YCONFLICT_DEFAULT << 5) /**< Shifted mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT3XCONFLICT             (0x1UL << 6)                                           /**< 1 If the Bus Connected to APORT3X is in Conflict With Another Peripheral */
#define _VDAC_OPA_APORTCONFLICT_APORT3XCONFLICT_SHIFT      6                                                      /**< Shift value for VDAC_OPAAPORT3XCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT3XCONFLICT_MASK       0x40UL                                                 /**< Bit mask for VDAC_OPAAPORT3XCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT3XCONFLICT_DEFAULT    0x00000000UL                                           /**< Mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT3XCONFLICT_DEFAULT     (_VDAC_OPA_APORTCONFLICT_APORT3XCONFLICT_DEFAULT << 6) /**< Shifted mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT3YCONFLICT             (0x1UL << 7)                                           /**< 1 If the Bus Connected to APORT3Y is in Conflict With Another Peripheral */
#define _VDAC_OPA_APORTCONFLICT_APORT3YCONFLICT_SHIFT      7                                                      /**< Shift value for VDAC_OPAAPORT3YCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT3YCONFLICT_MASK       0x80UL                                                 /**< Bit mask for VDAC_OPAAPORT3YCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT3YCONFLICT_DEFAULT    0x00000000UL                                           /**< Mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT3YCONFLICT_DEFAULT     (_VDAC_OPA_APORTCONFLICT_APORT3YCONFLICT_DEFAULT << 7) /**< Shifted mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT4XCONFLICT             (0x1UL << 8)                                           /**< 1 If the Bus Connected to APORT4X is in Conflict With Another Peripheral */
#define _VDAC_OPA_APORTCONFLICT_APORT4XCONFLICT_SHIFT      8                                                      /**< Shift value for VDAC_OPAAPORT4XCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT4XCONFLICT_MASK       0x100UL                                                /**< Bit mask for VDAC_OPAAPORT4XCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT4XCONFLICT_DEFAULT    0x00000000UL                                           /**< Mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT4XCONFLICT_DEFAULT     (_VDAC_OPA_APORTCONFLICT_APORT4XCONFLICT_DEFAULT << 8) /**< Shifted mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT4YCONFLICT             (0x1UL << 9)                                           /**< 1 If the Bus Connected to APORT4Y is in Conflict With Another Peripheral */
#define _VDAC_OPA_APORTCONFLICT_APORT4YCONFLICT_SHIFT      9                                                      /**< Shift value for VDAC_OPAAPORT4YCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT4YCONFLICT_MASK       0x200UL                                                /**< Bit mask for VDAC_OPAAPORT4YCONFLICT */
#define _VDAC_OPA_APORTCONFLICT_APORT4YCONFLICT_DEFAULT    0x00000000UL                                           /**< Mode DEFAULT for VDAC_OPA_APORTCONFLICT */
#define VDAC_OPA_APORTCONFLICT_APORT4YCONFLICT_DEFAULT     (_VDAC_OPA_APORTCONFLICT_APORT4YCONFLICT_DEFAULT << 9) /**< Shifted mode DEFAULT for VDAC_OPA_APORTCONFLICT */

/* Bit fields for VDAC OPA_CTRL */
#define _VDAC_OPA_CTRL_RESETVALUE                          0x0000000EUL                                   /**< Default value for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_MASK                                0x00313F1FUL                                   /**< Mask for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_DRIVESTRENGTH_SHIFT                 0                                              /**< Shift value for VDAC_OPADRIVESTRENGTH */
#define _VDAC_OPA_CTRL_DRIVESTRENGTH_MASK                  0x3UL                                          /**< Bit mask for VDAC_OPADRIVESTRENGTH */
#define _VDAC_OPA_CTRL_DRIVESTRENGTH_DEFAULT               0x00000002UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_DRIVESTRENGTH_DEFAULT                (_VDAC_OPA_CTRL_DRIVESTRENGTH_DEFAULT << 0)    /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_INCBW                                (0x1UL << 2)                                   /**< OPAx Unity Gain Bandwidth Scale */
#define _VDAC_OPA_CTRL_INCBW_SHIFT                         2                                              /**< Shift value for VDAC_OPAINCBW */
#define _VDAC_OPA_CTRL_INCBW_MASK                          0x4UL                                          /**< Bit mask for VDAC_OPAINCBW */
#define _VDAC_OPA_CTRL_INCBW_DEFAULT                       0x00000001UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_INCBW_DEFAULT                        (_VDAC_OPA_CTRL_INCBW_DEFAULT << 2)            /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_HCMDIS                               (0x1UL << 3)                                   /**< High Common Mode Disable */
#define _VDAC_OPA_CTRL_HCMDIS_SHIFT                        3                                              /**< Shift value for VDAC_OPAHCMDIS */
#define _VDAC_OPA_CTRL_HCMDIS_MASK                         0x8UL                                          /**< Bit mask for VDAC_OPAHCMDIS */
#define _VDAC_OPA_CTRL_HCMDIS_DEFAULT                      0x00000001UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_HCMDIS_DEFAULT                       (_VDAC_OPA_CTRL_HCMDIS_DEFAULT << 3)           /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_OUTSCALE                             (0x1UL << 4)                                   /**< Scale OPAx Output Driving Strength */
#define _VDAC_OPA_CTRL_OUTSCALE_SHIFT                      4                                              /**< Shift value for VDAC_OPAOUTSCALE */
#define _VDAC_OPA_CTRL_OUTSCALE_MASK                       0x10UL                                         /**< Bit mask for VDAC_OPAOUTSCALE */
#define _VDAC_OPA_CTRL_OUTSCALE_DEFAULT                    0x00000000UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_OUTSCALE_FULL                       0x00000000UL                                   /**< Mode FULL for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_OUTSCALE_HALF                       0x00000001UL                                   /**< Mode HALF for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_OUTSCALE_DEFAULT                     (_VDAC_OPA_CTRL_OUTSCALE_DEFAULT << 4)         /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_OUTSCALE_FULL                        (_VDAC_OPA_CTRL_OUTSCALE_FULL << 4)            /**< Shifted mode FULL for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_OUTSCALE_HALF                        (_VDAC_OPA_CTRL_OUTSCALE_HALF << 4)            /**< Shifted mode HALF for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSEN                                (0x1UL << 8)                                   /**< OPAx PRS Trigger Enable */
#define _VDAC_OPA_CTRL_PRSEN_SHIFT                         8                                              /**< Shift value for VDAC_OPAPRSEN */
#define _VDAC_OPA_CTRL_PRSEN_MASK                          0x100UL                                        /**< Bit mask for VDAC_OPAPRSEN */
#define _VDAC_OPA_CTRL_PRSEN_DEFAULT                       0x00000000UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSEN_DEFAULT                        (_VDAC_OPA_CTRL_PRSEN_DEFAULT << 8)            /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSMODE                              (0x1UL << 9)                                   /**< OPAx PRS Trigger Mode */
#define _VDAC_OPA_CTRL_PRSMODE_SHIFT                       9                                              /**< Shift value for VDAC_OPAPRSMODE */
#define _VDAC_OPA_CTRL_PRSMODE_MASK                        0x200UL                                        /**< Bit mask for VDAC_OPAPRSMODE */
#define _VDAC_OPA_CTRL_PRSMODE_DEFAULT                     0x00000000UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSMODE_PULSED                      0x00000000UL                                   /**< Mode PULSED for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSMODE_TIMED                       0x00000001UL                                   /**< Mode TIMED for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSMODE_DEFAULT                      (_VDAC_OPA_CTRL_PRSMODE_DEFAULT << 9)          /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSMODE_PULSED                       (_VDAC_OPA_CTRL_PRSMODE_PULSED << 9)           /**< Shifted mode PULSED for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSMODE_TIMED                        (_VDAC_OPA_CTRL_PRSMODE_TIMED << 9)            /**< Shifted mode TIMED for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_SHIFT                        10                                             /**< Shift value for VDAC_OPAPRSSEL */
#define _VDAC_OPA_CTRL_PRSSEL_MASK                         0x3C00UL                                       /**< Bit mask for VDAC_OPAPRSSEL */
#define _VDAC_OPA_CTRL_PRSSEL_DEFAULT                      0x00000000UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH0                       0x00000000UL                                   /**< Mode PRSCH0 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH1                       0x00000001UL                                   /**< Mode PRSCH1 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH2                       0x00000002UL                                   /**< Mode PRSCH2 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH3                       0x00000003UL                                   /**< Mode PRSCH3 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH4                       0x00000004UL                                   /**< Mode PRSCH4 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH5                       0x00000005UL                                   /**< Mode PRSCH5 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH6                       0x00000006UL                                   /**< Mode PRSCH6 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH7                       0x00000007UL                                   /**< Mode PRSCH7 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH8                       0x00000008UL                                   /**< Mode PRSCH8 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH9                       0x00000009UL                                   /**< Mode PRSCH9 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH10                      0x0000000AUL                                   /**< Mode PRSCH10 for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSSEL_PRSCH11                      0x0000000BUL                                   /**< Mode PRSCH11 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_DEFAULT                       (_VDAC_OPA_CTRL_PRSSEL_DEFAULT << 10)          /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH0                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH0 << 10)           /**< Shifted mode PRSCH0 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH1                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH1 << 10)           /**< Shifted mode PRSCH1 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH2                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH2 << 10)           /**< Shifted mode PRSCH2 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH3                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH3 << 10)           /**< Shifted mode PRSCH3 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH4                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH4 << 10)           /**< Shifted mode PRSCH4 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH5                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH5 << 10)           /**< Shifted mode PRSCH5 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH6                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH6 << 10)           /**< Shifted mode PRSCH6 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH7                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH7 << 10)           /**< Shifted mode PRSCH7 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH8                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH8 << 10)           /**< Shifted mode PRSCH8 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH9                        (_VDAC_OPA_CTRL_PRSSEL_PRSCH9 << 10)           /**< Shifted mode PRSCH9 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH10                       (_VDAC_OPA_CTRL_PRSSEL_PRSCH10 << 10)          /**< Shifted mode PRSCH10 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSSEL_PRSCH11                       (_VDAC_OPA_CTRL_PRSSEL_PRSCH11 << 10)          /**< Shifted mode PRSCH11 for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSOUTMODE                           (0x1UL << 16)                                  /**< OPAx PRS Output Select */
#define _VDAC_OPA_CTRL_PRSOUTMODE_SHIFT                    16                                             /**< Shift value for VDAC_OPAPRSOUTMODE */
#define _VDAC_OPA_CTRL_PRSOUTMODE_MASK                     0x10000UL                                      /**< Bit mask for VDAC_OPAPRSOUTMODE */
#define _VDAC_OPA_CTRL_PRSOUTMODE_DEFAULT                  0x00000000UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSOUTMODE_WARM                     0x00000000UL                                   /**< Mode WARM for VDAC_OPA_CTRL */
#define _VDAC_OPA_CTRL_PRSOUTMODE_OUTVALID                 0x00000001UL                                   /**< Mode OUTVALID for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSOUTMODE_DEFAULT                   (_VDAC_OPA_CTRL_PRSOUTMODE_DEFAULT << 16)      /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSOUTMODE_WARM                      (_VDAC_OPA_CTRL_PRSOUTMODE_WARM << 16)         /**< Shifted mode WARM for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_PRSOUTMODE_OUTVALID                  (_VDAC_OPA_CTRL_PRSOUTMODE_OUTVALID << 16)     /**< Shifted mode OUTVALID for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_APORTXMASTERDIS                      (0x1UL << 20)                                  /**< APORT Bus Master Disable */
#define _VDAC_OPA_CTRL_APORTXMASTERDIS_SHIFT               20                                             /**< Shift value for VDAC_OPAAPORTXMASTERDIS */
#define _VDAC_OPA_CTRL_APORTXMASTERDIS_MASK                0x100000UL                                     /**< Bit mask for VDAC_OPAAPORTXMASTERDIS */
#define _VDAC_OPA_CTRL_APORTXMASTERDIS_DEFAULT             0x00000000UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_APORTXMASTERDIS_DEFAULT              (_VDAC_OPA_CTRL_APORTXMASTERDIS_DEFAULT << 20) /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_APORTYMASTERDIS                      (0x1UL << 21)                                  /**< APORT Bus Master Disable */
#define _VDAC_OPA_CTRL_APORTYMASTERDIS_SHIFT               21                                             /**< Shift value for VDAC_OPAAPORTYMASTERDIS */
#define _VDAC_OPA_CTRL_APORTYMASTERDIS_MASK                0x200000UL                                     /**< Bit mask for VDAC_OPAAPORTYMASTERDIS */
#define _VDAC_OPA_CTRL_APORTYMASTERDIS_DEFAULT             0x00000000UL                                   /**< Mode DEFAULT for VDAC_OPA_CTRL */
#define VDAC_OPA_CTRL_APORTYMASTERDIS_DEFAULT              (_VDAC_OPA_CTRL_APORTYMASTERDIS_DEFAULT << 21) /**< Shifted mode DEFAULT for VDAC_OPA_CTRL */

/* Bit fields for VDAC OPA_TIMER */
#define _VDAC_OPA_TIMER_RESETVALUE                         0x00010700UL                               /**< Default value for VDAC_OPA_TIMER */
#define _VDAC_OPA_TIMER_MASK                               0x03FF7F3FUL                               /**< Mask for VDAC_OPA_TIMER */
#define _VDAC_OPA_TIMER_STARTUPDLY_SHIFT                   0                                          /**< Shift value for VDAC_OPASTARTUPDLY */
#define _VDAC_OPA_TIMER_STARTUPDLY_MASK                    0x3FUL                                     /**< Bit mask for VDAC_OPASTARTUPDLY */
#define _VDAC_OPA_TIMER_STARTUPDLY_DEFAULT                 0x00000000UL                               /**< Mode DEFAULT for VDAC_OPA_TIMER */
#define VDAC_OPA_TIMER_STARTUPDLY_DEFAULT                  (_VDAC_OPA_TIMER_STARTUPDLY_DEFAULT << 0)  /**< Shifted mode DEFAULT for VDAC_OPA_TIMER */
#define _VDAC_OPA_TIMER_WARMUPTIME_SHIFT                   8                                          /**< Shift value for VDAC_OPAWARMUPTIME */
#define _VDAC_OPA_TIMER_WARMUPTIME_MASK                    0x7F00UL                                   /**< Bit mask for VDAC_OPAWARMUPTIME */
#define _VDAC_OPA_TIMER_WARMUPTIME_DEFAULT                 0x00000007UL                               /**< Mode DEFAULT for VDAC_OPA_TIMER */
#define VDAC_OPA_TIMER_WARMUPTIME_DEFAULT                  (_VDAC_OPA_TIMER_WARMUPTIME_DEFAULT << 8)  /**< Shifted mode DEFAULT for VDAC_OPA_TIMER */
#define _VDAC_OPA_TIMER_SETTLETIME_SHIFT                   16                                         /**< Shift value for VDAC_OPASETTLETIME */
#define _VDAC_OPA_TIMER_SETTLETIME_MASK                    0x3FF0000UL                                /**< Bit mask for VDAC_OPASETTLETIME */
#define _VDAC_OPA_TIMER_SETTLETIME_DEFAULT                 0x00000001UL                               /**< Mode DEFAULT for VDAC_OPA_TIMER */
#define VDAC_OPA_TIMER_SETTLETIME_DEFAULT                  (_VDAC_OPA_TIMER_SETTLETIME_DEFAULT << 16) /**< Shifted mode DEFAULT for VDAC_OPA_TIMER */

/* Bit fields for VDAC OPA_MUX */
#define _VDAC_OPA_MUX_RESETVALUE                           0x0016F2F1UL                            /**< Default value for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_MASK                                 0x0717FFFFUL                            /**< Mask for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_SHIFT                         0                                       /**< Shift value for VDAC_OPAPOSSEL */
#define _VDAC_OPA_MUX_POSSEL_MASK                          0xFFUL                                  /**< Bit mask for VDAC_OPAPOSSEL */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH0                    0x00000020UL                            /**< Mode APORT1XCH0 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH2                    0x00000021UL                            /**< Mode APORT1XCH2 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH4                    0x00000022UL                            /**< Mode APORT1XCH4 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH6                    0x00000023UL                            /**< Mode APORT1XCH6 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH8                    0x00000024UL                            /**< Mode APORT1XCH8 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH10                   0x00000025UL                            /**< Mode APORT1XCH10 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH12                   0x00000026UL                            /**< Mode APORT1XCH12 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH14                   0x00000027UL                            /**< Mode APORT1XCH14 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH16                   0x00000028UL                            /**< Mode APORT1XCH16 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH18                   0x00000029UL                            /**< Mode APORT1XCH18 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH20                   0x0000002AUL                            /**< Mode APORT1XCH20 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH22                   0x0000002BUL                            /**< Mode APORT1XCH22 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH24                   0x0000002CUL                            /**< Mode APORT1XCH24 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH26                   0x0000002DUL                            /**< Mode APORT1XCH26 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH28                   0x0000002EUL                            /**< Mode APORT1XCH28 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT1XCH30                   0x0000002FUL                            /**< Mode APORT1XCH30 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH1                    0x00000040UL                            /**< Mode APORT2XCH1 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH3                    0x00000041UL                            /**< Mode APORT2XCH3 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH5                    0x00000042UL                            /**< Mode APORT2XCH5 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH7                    0x00000043UL                            /**< Mode APORT2XCH7 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH9                    0x00000044UL                            /**< Mode APORT2XCH9 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH11                   0x00000045UL                            /**< Mode APORT2XCH11 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH13                   0x00000046UL                            /**< Mode APORT2XCH13 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH15                   0x00000047UL                            /**< Mode APORT2XCH15 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH17                   0x00000048UL                            /**< Mode APORT2XCH17 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH19                   0x00000049UL                            /**< Mode APORT2XCH19 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH21                   0x0000004AUL                            /**< Mode APORT2XCH21 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH23                   0x0000004BUL                            /**< Mode APORT2XCH23 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH25                   0x0000004CUL                            /**< Mode APORT2XCH25 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH27                   0x0000004DUL                            /**< Mode APORT2XCH27 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH29                   0x0000004EUL                            /**< Mode APORT2XCH29 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT2XCH31                   0x0000004FUL                            /**< Mode APORT2XCH31 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH0                    0x00000060UL                            /**< Mode APORT3XCH0 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH2                    0x00000061UL                            /**< Mode APORT3XCH2 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH4                    0x00000062UL                            /**< Mode APORT3XCH4 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH6                    0x00000063UL                            /**< Mode APORT3XCH6 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH8                    0x00000064UL                            /**< Mode APORT3XCH8 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH10                   0x00000065UL                            /**< Mode APORT3XCH10 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH12                   0x00000066UL                            /**< Mode APORT3XCH12 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH14                   0x00000067UL                            /**< Mode APORT3XCH14 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH16                   0x00000068UL                            /**< Mode APORT3XCH16 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH18                   0x00000069UL                            /**< Mode APORT3XCH18 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH20                   0x0000006AUL                            /**< Mode APORT3XCH20 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH22                   0x0000006BUL                            /**< Mode APORT3XCH22 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH24                   0x0000006CUL                            /**< Mode APORT3XCH24 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH26                   0x0000006DUL                            /**< Mode APORT3XCH26 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH28                   0x0000006EUL                            /**< Mode APORT3XCH28 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT3XCH30                   0x0000006FUL                            /**< Mode APORT3XCH30 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH1                    0x00000080UL                            /**< Mode APORT4XCH1 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH3                    0x00000081UL                            /**< Mode APORT4XCH3 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH5                    0x00000082UL                            /**< Mode APORT4XCH5 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH7                    0x00000083UL                            /**< Mode APORT4XCH7 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH9                    0x00000084UL                            /**< Mode APORT4XCH9 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH11                   0x00000085UL                            /**< Mode APORT4XCH11 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH13                   0x00000086UL                            /**< Mode APORT4XCH13 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH15                   0x00000087UL                            /**< Mode APORT4XCH15 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH17                   0x00000088UL                            /**< Mode APORT4XCH17 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH19                   0x00000089UL                            /**< Mode APORT4XCH19 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH21                   0x0000008AUL                            /**< Mode APORT4XCH21 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH23                   0x0000008BUL                            /**< Mode APORT4XCH23 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH25                   0x0000008CUL                            /**< Mode APORT4XCH25 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH27                   0x0000008DUL                            /**< Mode APORT4XCH27 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH29                   0x0000008EUL                            /**< Mode APORT4XCH29 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_APORT4XCH31                   0x0000008FUL                            /**< Mode APORT4XCH31 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_DISABLE                       0x000000F0UL                            /**< Mode DISABLE for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_DEFAULT                       0x000000F1UL                            /**< Mode DEFAULT for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_DAC                           0x000000F1UL                            /**< Mode DAC for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_POSPAD                        0x000000F2UL                            /**< Mode POSPAD for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_OPANEXT                       0x000000F3UL                            /**< Mode OPANEXT for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_POSSEL_OPATAP                        0x000000F4UL                            /**< Mode OPATAP for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH0                     (_VDAC_OPA_MUX_POSSEL_APORT1XCH0 << 0)  /**< Shifted mode APORT1XCH0 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH2                     (_VDAC_OPA_MUX_POSSEL_APORT1XCH2 << 0)  /**< Shifted mode APORT1XCH2 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH4                     (_VDAC_OPA_MUX_POSSEL_APORT1XCH4 << 0)  /**< Shifted mode APORT1XCH4 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH6                     (_VDAC_OPA_MUX_POSSEL_APORT1XCH6 << 0)  /**< Shifted mode APORT1XCH6 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH8                     (_VDAC_OPA_MUX_POSSEL_APORT1XCH8 << 0)  /**< Shifted mode APORT1XCH8 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH10                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH10 << 0) /**< Shifted mode APORT1XCH10 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH12                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH12 << 0) /**< Shifted mode APORT1XCH12 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH14                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH14 << 0) /**< Shifted mode APORT1XCH14 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH16                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH16 << 0) /**< Shifted mode APORT1XCH16 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH18                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH18 << 0) /**< Shifted mode APORT1XCH18 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH20                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH20 << 0) /**< Shifted mode APORT1XCH20 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH22                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH22 << 0) /**< Shifted mode APORT1XCH22 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH24                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH24 << 0) /**< Shifted mode APORT1XCH24 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH26                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH26 << 0) /**< Shifted mode APORT1XCH26 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH28                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH28 << 0) /**< Shifted mode APORT1XCH28 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT1XCH30                    (_VDAC_OPA_MUX_POSSEL_APORT1XCH30 << 0) /**< Shifted mode APORT1XCH30 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH1                     (_VDAC_OPA_MUX_POSSEL_APORT2XCH1 << 0)  /**< Shifted mode APORT2XCH1 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH3                     (_VDAC_OPA_MUX_POSSEL_APORT2XCH3 << 0)  /**< Shifted mode APORT2XCH3 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH5                     (_VDAC_OPA_MUX_POSSEL_APORT2XCH5 << 0)  /**< Shifted mode APORT2XCH5 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH7                     (_VDAC_OPA_MUX_POSSEL_APORT2XCH7 << 0)  /**< Shifted mode APORT2XCH7 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH9                     (_VDAC_OPA_MUX_POSSEL_APORT2XCH9 << 0)  /**< Shifted mode APORT2XCH9 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH11                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH11 << 0) /**< Shifted mode APORT2XCH11 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH13                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH13 << 0) /**< Shifted mode APORT2XCH13 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH15                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH15 << 0) /**< Shifted mode APORT2XCH15 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH17                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH17 << 0) /**< Shifted mode APORT2XCH17 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH19                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH19 << 0) /**< Shifted mode APORT2XCH19 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH21                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH21 << 0) /**< Shifted mode APORT2XCH21 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH23                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH23 << 0) /**< Shifted mode APORT2XCH23 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH25                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH25 << 0) /**< Shifted mode APORT2XCH25 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH27                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH27 << 0) /**< Shifted mode APORT2XCH27 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH29                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH29 << 0) /**< Shifted mode APORT2XCH29 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT2XCH31                    (_VDAC_OPA_MUX_POSSEL_APORT2XCH31 << 0) /**< Shifted mode APORT2XCH31 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH0                     (_VDAC_OPA_MUX_POSSEL_APORT3XCH0 << 0)  /**< Shifted mode APORT3XCH0 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH2                     (_VDAC_OPA_MUX_POSSEL_APORT3XCH2 << 0)  /**< Shifted mode APORT3XCH2 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH4                     (_VDAC_OPA_MUX_POSSEL_APORT3XCH4 << 0)  /**< Shifted mode APORT3XCH4 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH6                     (_VDAC_OPA_MUX_POSSEL_APORT3XCH6 << 0)  /**< Shifted mode APORT3XCH6 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH8                     (_VDAC_OPA_MUX_POSSEL_APORT3XCH8 << 0)  /**< Shifted mode APORT3XCH8 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH10                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH10 << 0) /**< Shifted mode APORT3XCH10 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH12                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH12 << 0) /**< Shifted mode APORT3XCH12 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH14                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH14 << 0) /**< Shifted mode APORT3XCH14 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH16                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH16 << 0) /**< Shifted mode APORT3XCH16 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH18                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH18 << 0) /**< Shifted mode APORT3XCH18 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH20                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH20 << 0) /**< Shifted mode APORT3XCH20 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH22                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH22 << 0) /**< Shifted mode APORT3XCH22 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH24                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH24 << 0) /**< Shifted mode APORT3XCH24 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH26                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH26 << 0) /**< Shifted mode APORT3XCH26 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH28                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH28 << 0) /**< Shifted mode APORT3XCH28 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT3XCH30                    (_VDAC_OPA_MUX_POSSEL_APORT3XCH30 << 0) /**< Shifted mode APORT3XCH30 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH1                     (_VDAC_OPA_MUX_POSSEL_APORT4XCH1 << 0)  /**< Shifted mode APORT4XCH1 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH3                     (_VDAC_OPA_MUX_POSSEL_APORT4XCH3 << 0)  /**< Shifted mode APORT4XCH3 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH5                     (_VDAC_OPA_MUX_POSSEL_APORT4XCH5 << 0)  /**< Shifted mode APORT4XCH5 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH7                     (_VDAC_OPA_MUX_POSSEL_APORT4XCH7 << 0)  /**< Shifted mode APORT4XCH7 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH9                     (_VDAC_OPA_MUX_POSSEL_APORT4XCH9 << 0)  /**< Shifted mode APORT4XCH9 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH11                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH11 << 0) /**< Shifted mode APORT4XCH11 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH13                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH13 << 0) /**< Shifted mode APORT4XCH13 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH15                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH15 << 0) /**< Shifted mode APORT4XCH15 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH17                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH17 << 0) /**< Shifted mode APORT4XCH17 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH19                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH19 << 0) /**< Shifted mode APORT4XCH19 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH21                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH21 << 0) /**< Shifted mode APORT4XCH21 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH23                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH23 << 0) /**< Shifted mode APORT4XCH23 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH25                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH25 << 0) /**< Shifted mode APORT4XCH25 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH27                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH27 << 0) /**< Shifted mode APORT4XCH27 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH29                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH29 << 0) /**< Shifted mode APORT4XCH29 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_APORT4XCH31                    (_VDAC_OPA_MUX_POSSEL_APORT4XCH31 << 0) /**< Shifted mode APORT4XCH31 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_DISABLE                        (_VDAC_OPA_MUX_POSSEL_DISABLE << 0)     /**< Shifted mode DISABLE for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_DEFAULT                        (_VDAC_OPA_MUX_POSSEL_DEFAULT << 0)     /**< Shifted mode DEFAULT for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_DAC                            (_VDAC_OPA_MUX_POSSEL_DAC << 0)         /**< Shifted mode DAC for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_POSPAD                         (_VDAC_OPA_MUX_POSSEL_POSPAD << 0)      /**< Shifted mode POSPAD for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_OPANEXT                        (_VDAC_OPA_MUX_POSSEL_OPANEXT << 0)     /**< Shifted mode OPANEXT for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_POSSEL_OPATAP                         (_VDAC_OPA_MUX_POSSEL_OPATAP << 0)      /**< Shifted mode OPATAP for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_SHIFT                         8                                       /**< Shift value for VDAC_OPANEGSEL */
#define _VDAC_OPA_MUX_NEGSEL_MASK                          0xFF00UL                                /**< Bit mask for VDAC_OPANEGSEL */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH1                    0x00000030UL                            /**< Mode APORT1YCH1 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH3                    0x00000031UL                            /**< Mode APORT1YCH3 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH5                    0x00000032UL                            /**< Mode APORT1YCH5 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH7                    0x00000033UL                            /**< Mode APORT1YCH7 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH9                    0x00000034UL                            /**< Mode APORT1YCH9 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH11                   0x00000035UL                            /**< Mode APORT1YCH11 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH13                   0x00000036UL                            /**< Mode APORT1YCH13 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH15                   0x00000037UL                            /**< Mode APORT1YCH15 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH17                   0x00000038UL                            /**< Mode APORT1YCH17 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH19                   0x00000039UL                            /**< Mode APORT1YCH19 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH21                   0x0000003AUL                            /**< Mode APORT1YCH21 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH23                   0x0000003BUL                            /**< Mode APORT1YCH23 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH25                   0x0000003CUL                            /**< Mode APORT1YCH25 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH27                   0x0000003DUL                            /**< Mode APORT1YCH27 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH29                   0x0000003EUL                            /**< Mode APORT1YCH29 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT1YCH31                   0x0000003FUL                            /**< Mode APORT1YCH31 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH0                    0x00000050UL                            /**< Mode APORT2YCH0 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH2                    0x00000051UL                            /**< Mode APORT2YCH2 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH4                    0x00000052UL                            /**< Mode APORT2YCH4 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH6                    0x00000053UL                            /**< Mode APORT2YCH6 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH8                    0x00000054UL                            /**< Mode APORT2YCH8 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH10                   0x00000055UL                            /**< Mode APORT2YCH10 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH12                   0x00000056UL                            /**< Mode APORT2YCH12 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH14                   0x00000057UL                            /**< Mode APORT2YCH14 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH16                   0x00000058UL                            /**< Mode APORT2YCH16 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH18                   0x00000059UL                            /**< Mode APORT2YCH18 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH20                   0x0000005AUL                            /**< Mode APORT2YCH20 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH22                   0x0000005BUL                            /**< Mode APORT2YCH22 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH24                   0x0000005CUL                            /**< Mode APORT2YCH24 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH26                   0x0000005DUL                            /**< Mode APORT2YCH26 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH28                   0x0000005EUL                            /**< Mode APORT2YCH28 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT2YCH30                   0x0000005FUL                            /**< Mode APORT2YCH30 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH1                    0x00000070UL                            /**< Mode APORT3YCH1 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH3                    0x00000071UL                            /**< Mode APORT3YCH3 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH5                    0x00000072UL                            /**< Mode APORT3YCH5 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH7                    0x00000073UL                            /**< Mode APORT3YCH7 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH9                    0x00000074UL                            /**< Mode APORT3YCH9 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH11                   0x00000075UL                            /**< Mode APORT3YCH11 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH13                   0x00000076UL                            /**< Mode APORT3YCH13 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH15                   0x00000077UL                            /**< Mode APORT3YCH15 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH17                   0x00000078UL                            /**< Mode APORT3YCH17 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH19                   0x00000079UL                            /**< Mode APORT3YCH19 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH21                   0x0000007AUL                            /**< Mode APORT3YCH21 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH23                   0x0000007BUL                            /**< Mode APORT3YCH23 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH25                   0x0000007CUL                            /**< Mode APORT3YCH25 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH27                   0x0000007DUL                            /**< Mode APORT3YCH27 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH29                   0x0000007EUL                            /**< Mode APORT3YCH29 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT3YCH31                   0x0000007FUL                            /**< Mode APORT3YCH31 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH0                    0x00000090UL                            /**< Mode APORT4YCH0 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH2                    0x00000091UL                            /**< Mode APORT4YCH2 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH4                    0x00000092UL                            /**< Mode APORT4YCH4 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH6                    0x00000093UL                            /**< Mode APORT4YCH6 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH8                    0x00000094UL                            /**< Mode APORT4YCH8 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH10                   0x00000095UL                            /**< Mode APORT4YCH10 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH12                   0x00000096UL                            /**< Mode APORT4YCH12 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH14                   0x00000097UL                            /**< Mode APORT4YCH14 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH16                   0x00000098UL                            /**< Mode APORT4YCH16 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH18                   0x00000099UL                            /**< Mode APORT4YCH18 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH20                   0x0000009AUL                            /**< Mode APORT4YCH20 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH22                   0x0000009BUL                            /**< Mode APORT4YCH22 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH24                   0x0000009CUL                            /**< Mode APORT4YCH24 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH26                   0x0000009DUL                            /**< Mode APORT4YCH26 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH28                   0x0000009EUL                            /**< Mode APORT4YCH28 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_APORT4YCH30                   0x0000009FUL                            /**< Mode APORT4YCH30 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_DISABLE                       0x000000F0UL                            /**< Mode DISABLE for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_UG                            0x000000F1UL                            /**< Mode UG for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_DEFAULT                       0x000000F2UL                            /**< Mode DEFAULT for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_OPATAP                        0x000000F2UL                            /**< Mode OPATAP for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_NEGSEL_NEGPAD                        0x000000F3UL                            /**< Mode NEGPAD for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH1                     (_VDAC_OPA_MUX_NEGSEL_APORT1YCH1 << 8)  /**< Shifted mode APORT1YCH1 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH3                     (_VDAC_OPA_MUX_NEGSEL_APORT1YCH3 << 8)  /**< Shifted mode APORT1YCH3 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH5                     (_VDAC_OPA_MUX_NEGSEL_APORT1YCH5 << 8)  /**< Shifted mode APORT1YCH5 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH7                     (_VDAC_OPA_MUX_NEGSEL_APORT1YCH7 << 8)  /**< Shifted mode APORT1YCH7 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH9                     (_VDAC_OPA_MUX_NEGSEL_APORT1YCH9 << 8)  /**< Shifted mode APORT1YCH9 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH11                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH11 << 8) /**< Shifted mode APORT1YCH11 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH13                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH13 << 8) /**< Shifted mode APORT1YCH13 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH15                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH15 << 8) /**< Shifted mode APORT1YCH15 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH17                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH17 << 8) /**< Shifted mode APORT1YCH17 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH19                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH19 << 8) /**< Shifted mode APORT1YCH19 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH21                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH21 << 8) /**< Shifted mode APORT1YCH21 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH23                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH23 << 8) /**< Shifted mode APORT1YCH23 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH25                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH25 << 8) /**< Shifted mode APORT1YCH25 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH27                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH27 << 8) /**< Shifted mode APORT1YCH27 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH29                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH29 << 8) /**< Shifted mode APORT1YCH29 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT1YCH31                    (_VDAC_OPA_MUX_NEGSEL_APORT1YCH31 << 8) /**< Shifted mode APORT1YCH31 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH0                     (_VDAC_OPA_MUX_NEGSEL_APORT2YCH0 << 8)  /**< Shifted mode APORT2YCH0 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH2                     (_VDAC_OPA_MUX_NEGSEL_APORT2YCH2 << 8)  /**< Shifted mode APORT2YCH2 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH4                     (_VDAC_OPA_MUX_NEGSEL_APORT2YCH4 << 8)  /**< Shifted mode APORT2YCH4 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH6                     (_VDAC_OPA_MUX_NEGSEL_APORT2YCH6 << 8)  /**< Shifted mode APORT2YCH6 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH8                     (_VDAC_OPA_MUX_NEGSEL_APORT2YCH8 << 8)  /**< Shifted mode APORT2YCH8 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH10                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH10 << 8) /**< Shifted mode APORT2YCH10 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH12                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH12 << 8) /**< Shifted mode APORT2YCH12 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH14                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH14 << 8) /**< Shifted mode APORT2YCH14 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH16                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH16 << 8) /**< Shifted mode APORT2YCH16 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH18                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH18 << 8) /**< Shifted mode APORT2YCH18 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH20                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH20 << 8) /**< Shifted mode APORT2YCH20 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH22                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH22 << 8) /**< Shifted mode APORT2YCH22 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH24                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH24 << 8) /**< Shifted mode APORT2YCH24 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH26                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH26 << 8) /**< Shifted mode APORT2YCH26 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH28                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH28 << 8) /**< Shifted mode APORT2YCH28 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT2YCH30                    (_VDAC_OPA_MUX_NEGSEL_APORT2YCH30 << 8) /**< Shifted mode APORT2YCH30 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH1                     (_VDAC_OPA_MUX_NEGSEL_APORT3YCH1 << 8)  /**< Shifted mode APORT3YCH1 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH3                     (_VDAC_OPA_MUX_NEGSEL_APORT3YCH3 << 8)  /**< Shifted mode APORT3YCH3 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH5                     (_VDAC_OPA_MUX_NEGSEL_APORT3YCH5 << 8)  /**< Shifted mode APORT3YCH5 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH7                     (_VDAC_OPA_MUX_NEGSEL_APORT3YCH7 << 8)  /**< Shifted mode APORT3YCH7 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH9                     (_VDAC_OPA_MUX_NEGSEL_APORT3YCH9 << 8)  /**< Shifted mode APORT3YCH9 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH11                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH11 << 8) /**< Shifted mode APORT3YCH11 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH13                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH13 << 8) /**< Shifted mode APORT3YCH13 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH15                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH15 << 8) /**< Shifted mode APORT3YCH15 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH17                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH17 << 8) /**< Shifted mode APORT3YCH17 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH19                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH19 << 8) /**< Shifted mode APORT3YCH19 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH21                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH21 << 8) /**< Shifted mode APORT3YCH21 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH23                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH23 << 8) /**< Shifted mode APORT3YCH23 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH25                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH25 << 8) /**< Shifted mode APORT3YCH25 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH27                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH27 << 8) /**< Shifted mode APORT3YCH27 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH29                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH29 << 8) /**< Shifted mode APORT3YCH29 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT3YCH31                    (_VDAC_OPA_MUX_NEGSEL_APORT3YCH31 << 8) /**< Shifted mode APORT3YCH31 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH0                     (_VDAC_OPA_MUX_NEGSEL_APORT4YCH0 << 8)  /**< Shifted mode APORT4YCH0 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH2                     (_VDAC_OPA_MUX_NEGSEL_APORT4YCH2 << 8)  /**< Shifted mode APORT4YCH2 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH4                     (_VDAC_OPA_MUX_NEGSEL_APORT4YCH4 << 8)  /**< Shifted mode APORT4YCH4 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH6                     (_VDAC_OPA_MUX_NEGSEL_APORT4YCH6 << 8)  /**< Shifted mode APORT4YCH6 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH8                     (_VDAC_OPA_MUX_NEGSEL_APORT4YCH8 << 8)  /**< Shifted mode APORT4YCH8 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH10                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH10 << 8) /**< Shifted mode APORT4YCH10 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH12                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH12 << 8) /**< Shifted mode APORT4YCH12 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH14                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH14 << 8) /**< Shifted mode APORT4YCH14 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH16                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH16 << 8) /**< Shifted mode APORT4YCH16 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH18                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH18 << 8) /**< Shifted mode APORT4YCH18 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH20                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH20 << 8) /**< Shifted mode APORT4YCH20 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH22                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH22 << 8) /**< Shifted mode APORT4YCH22 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH24                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH24 << 8) /**< Shifted mode APORT4YCH24 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH26                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH26 << 8) /**< Shifted mode APORT4YCH26 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH28                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH28 << 8) /**< Shifted mode APORT4YCH28 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_APORT4YCH30                    (_VDAC_OPA_MUX_NEGSEL_APORT4YCH30 << 8) /**< Shifted mode APORT4YCH30 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_DISABLE                        (_VDAC_OPA_MUX_NEGSEL_DISABLE << 8)     /**< Shifted mode DISABLE for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_UG                             (_VDAC_OPA_MUX_NEGSEL_UG << 8)          /**< Shifted mode UG for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_DEFAULT                        (_VDAC_OPA_MUX_NEGSEL_DEFAULT << 8)     /**< Shifted mode DEFAULT for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_OPATAP                         (_VDAC_OPA_MUX_NEGSEL_OPATAP << 8)      /**< Shifted mode OPATAP for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_NEGSEL_NEGPAD                         (_VDAC_OPA_MUX_NEGSEL_NEGPAD << 8)      /**< Shifted mode NEGPAD for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESINMUX_SHIFT                       16                                      /**< Shift value for VDAC_OPARESINMUX */
#define _VDAC_OPA_MUX_RESINMUX_MASK                        0x70000UL                               /**< Bit mask for VDAC_OPARESINMUX */
#define _VDAC_OPA_MUX_RESINMUX_DISABLE                     0x00000000UL                            /**< Mode DISABLE for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESINMUX_OPANEXT                     0x00000001UL                            /**< Mode OPANEXT for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESINMUX_NEGPAD                      0x00000002UL                            /**< Mode NEGPAD for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESINMUX_POSPAD                      0x00000003UL                            /**< Mode POSPAD for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESINMUX_COMPAD                      0x00000004UL                            /**< Mode COMPAD for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESINMUX_CENTER                      0x00000005UL                            /**< Mode CENTER for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESINMUX_DEFAULT                     0x00000006UL                            /**< Mode DEFAULT for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESINMUX_VSS                         0x00000006UL                            /**< Mode VSS for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESINMUX_DISABLE                      (_VDAC_OPA_MUX_RESINMUX_DISABLE << 16)  /**< Shifted mode DISABLE for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESINMUX_OPANEXT                      (_VDAC_OPA_MUX_RESINMUX_OPANEXT << 16)  /**< Shifted mode OPANEXT for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESINMUX_NEGPAD                       (_VDAC_OPA_MUX_RESINMUX_NEGPAD << 16)   /**< Shifted mode NEGPAD for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESINMUX_POSPAD                       (_VDAC_OPA_MUX_RESINMUX_POSPAD << 16)   /**< Shifted mode POSPAD for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESINMUX_COMPAD                       (_VDAC_OPA_MUX_RESINMUX_COMPAD << 16)   /**< Shifted mode COMPAD for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESINMUX_CENTER                       (_VDAC_OPA_MUX_RESINMUX_CENTER << 16)   /**< Shifted mode CENTER for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESINMUX_DEFAULT                      (_VDAC_OPA_MUX_RESINMUX_DEFAULT << 16)  /**< Shifted mode DEFAULT for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESINMUX_VSS                          (_VDAC_OPA_MUX_RESINMUX_VSS << 16)      /**< Shifted mode VSS for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_GAIN3X                                (0x1UL << 20)                           /**< OPAx Dedicated 3x Gain Resistor Ladder */
#define _VDAC_OPA_MUX_GAIN3X_SHIFT                         20                                      /**< Shift value for VDAC_OPAGAIN3X */
#define _VDAC_OPA_MUX_GAIN3X_MASK                          0x100000UL                              /**< Bit mask for VDAC_OPAGAIN3X */
#define _VDAC_OPA_MUX_GAIN3X_DEFAULT                       0x00000001UL                            /**< Mode DEFAULT for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_GAIN3X_DEFAULT                        (_VDAC_OPA_MUX_GAIN3X_DEFAULT << 20)    /**< Shifted mode DEFAULT for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_SHIFT                         24                                      /**< Shift value for VDAC_OPARESSEL */
#define _VDAC_OPA_MUX_RESSEL_MASK                          0x7000000UL                             /**< Bit mask for VDAC_OPARESSEL */
#define _VDAC_OPA_MUX_RESSEL_DEFAULT                       0x00000000UL                            /**< Mode DEFAULT for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_RES0                          0x00000000UL                            /**< Mode RES0 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_RES1                          0x00000001UL                            /**< Mode RES1 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_RES2                          0x00000002UL                            /**< Mode RES2 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_RES3                          0x00000003UL                            /**< Mode RES3 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_RES4                          0x00000004UL                            /**< Mode RES4 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_RES5                          0x00000005UL                            /**< Mode RES5 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_RES6                          0x00000006UL                            /**< Mode RES6 for VDAC_OPA_MUX */
#define _VDAC_OPA_MUX_RESSEL_RES7                          0x00000007UL                            /**< Mode RES7 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_DEFAULT                        (_VDAC_OPA_MUX_RESSEL_DEFAULT << 24)    /**< Shifted mode DEFAULT for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_RES0                           (_VDAC_OPA_MUX_RESSEL_RES0 << 24)       /**< Shifted mode RES0 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_RES1                           (_VDAC_OPA_MUX_RESSEL_RES1 << 24)       /**< Shifted mode RES1 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_RES2                           (_VDAC_OPA_MUX_RESSEL_RES2 << 24)       /**< Shifted mode RES2 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_RES3                           (_VDAC_OPA_MUX_RESSEL_RES3 << 24)       /**< Shifted mode RES3 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_RES4                           (_VDAC_OPA_MUX_RESSEL_RES4 << 24)       /**< Shifted mode RES4 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_RES5                           (_VDAC_OPA_MUX_RESSEL_RES5 << 24)       /**< Shifted mode RES5 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_RES6                           (_VDAC_OPA_MUX_RESSEL_RES6 << 24)       /**< Shifted mode RES6 for VDAC_OPA_MUX */
#define VDAC_OPA_MUX_RESSEL_RES7                           (_VDAC_OPA_MUX_RESSEL_RES7 << 24)       /**< Shifted mode RES7 for VDAC_OPA_MUX */

/* Bit fields for VDAC OPA_OUT */
#define _VDAC_OPA_OUT_RESETVALUE                           0x00000001UL                                  /**< Default value for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_MASK                                 0x00FF01FFUL                                  /**< Mask for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_MAINOUTEN                             (0x1UL << 0)                                  /**< OPAx Main Output Enable */
#define _VDAC_OPA_OUT_MAINOUTEN_SHIFT                      0                                             /**< Shift value for VDAC_OPAMAINOUTEN */
#define _VDAC_OPA_OUT_MAINOUTEN_MASK                       0x1UL                                         /**< Bit mask for VDAC_OPAMAINOUTEN */
#define _VDAC_OPA_OUT_MAINOUTEN_DEFAULT                    0x00000001UL                                  /**< Mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_MAINOUTEN_DEFAULT                     (_VDAC_OPA_OUT_MAINOUTEN_DEFAULT << 0)        /**< Shifted mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_ALTOUTEN                              (0x1UL << 1)                                  /**< OPAx Alternative Output Enable */
#define _VDAC_OPA_OUT_ALTOUTEN_SHIFT                       1                                             /**< Shift value for VDAC_OPAALTOUTEN */
#define _VDAC_OPA_OUT_ALTOUTEN_MASK                        0x2UL                                         /**< Bit mask for VDAC_OPAALTOUTEN */
#define _VDAC_OPA_OUT_ALTOUTEN_DEFAULT                     0x00000000UL                                  /**< Mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_ALTOUTEN_DEFAULT                      (_VDAC_OPA_OUT_ALTOUTEN_DEFAULT << 1)         /**< Shifted mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTEN                            (0x1UL << 2)                                  /**< OPAx Aport Output Enable */
#define _VDAC_OPA_OUT_APORTOUTEN_SHIFT                     2                                             /**< Shift value for VDAC_OPAAPORTOUTEN */
#define _VDAC_OPA_OUT_APORTOUTEN_MASK                      0x4UL                                         /**< Bit mask for VDAC_OPAAPORTOUTEN */
#define _VDAC_OPA_OUT_APORTOUTEN_DEFAULT                   0x00000000UL                                  /**< Mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTEN_DEFAULT                    (_VDAC_OPA_OUT_APORTOUTEN_DEFAULT << 2)       /**< Shifted mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_SHORT                                 (0x1UL << 3)                                  /**< OPAx Main and Alternative Output Short */
#define _VDAC_OPA_OUT_SHORT_SHIFT                          3                                             /**< Shift value for VDAC_OPASHORT */
#define _VDAC_OPA_OUT_SHORT_MASK                           0x8UL                                         /**< Bit mask for VDAC_OPASHORT */
#define _VDAC_OPA_OUT_SHORT_DEFAULT                        0x00000000UL                                  /**< Mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_SHORT_DEFAULT                         (_VDAC_OPA_OUT_SHORT_DEFAULT << 3)            /**< Shifted mode DEFAULT for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_ALTOUTPADEN_SHIFT                    4                                             /**< Shift value for VDAC_OPAALTOUTPADEN */
#define _VDAC_OPA_OUT_ALTOUTPADEN_MASK                     0x1F0UL                                       /**< Bit mask for VDAC_OPAALTOUTPADEN */
#define _VDAC_OPA_OUT_ALTOUTPADEN_DEFAULT                  0x00000000UL                                  /**< Mode DEFAULT for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_ALTOUTPADEN_OUT0                     0x00000001UL                                  /**< Mode OUT0 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_ALTOUTPADEN_OUT1                     0x00000002UL                                  /**< Mode OUT1 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_ALTOUTPADEN_OUT2                     0x00000004UL                                  /**< Mode OUT2 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_ALTOUTPADEN_OUT3                     0x00000008UL                                  /**< Mode OUT3 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_ALTOUTPADEN_OUT4                     0x00000010UL                                  /**< Mode OUT4 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_ALTOUTPADEN_DEFAULT                   (_VDAC_OPA_OUT_ALTOUTPADEN_DEFAULT << 4)      /**< Shifted mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_ALTOUTPADEN_OUT0                      (_VDAC_OPA_OUT_ALTOUTPADEN_OUT0 << 4)         /**< Shifted mode OUT0 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_ALTOUTPADEN_OUT1                      (_VDAC_OPA_OUT_ALTOUTPADEN_OUT1 << 4)         /**< Shifted mode OUT1 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_ALTOUTPADEN_OUT2                      (_VDAC_OPA_OUT_ALTOUTPADEN_OUT2 << 4)         /**< Shifted mode OUT2 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_ALTOUTPADEN_OUT3                      (_VDAC_OPA_OUT_ALTOUTPADEN_OUT3 << 4)         /**< Shifted mode OUT3 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_ALTOUTPADEN_OUT4                      (_VDAC_OPA_OUT_ALTOUTPADEN_OUT4 << 4)         /**< Shifted mode OUT4 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_SHIFT                    16                                            /**< Shift value for VDAC_OPAAPORTOUTSEL */
#define _VDAC_OPA_OUT_APORTOUTSEL_MASK                     0xFF0000UL                                    /**< Bit mask for VDAC_OPAAPORTOUTSEL */
#define _VDAC_OPA_OUT_APORTOUTSEL_DEFAULT                  0x00000000UL                                  /**< Mode DEFAULT for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH1               0x00000030UL                                  /**< Mode APORT1YCH1 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH3               0x00000031UL                                  /**< Mode APORT1YCH3 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH5               0x00000032UL                                  /**< Mode APORT1YCH5 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH7               0x00000033UL                                  /**< Mode APORT1YCH7 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH9               0x00000034UL                                  /**< Mode APORT1YCH9 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH11              0x00000035UL                                  /**< Mode APORT1YCH11 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH13              0x00000036UL                                  /**< Mode APORT1YCH13 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH15              0x00000037UL                                  /**< Mode APORT1YCH15 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH17              0x00000038UL                                  /**< Mode APORT1YCH17 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH19              0x00000039UL                                  /**< Mode APORT1YCH19 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH21              0x0000003AUL                                  /**< Mode APORT1YCH21 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH23              0x0000003BUL                                  /**< Mode APORT1YCH23 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH25              0x0000003CUL                                  /**< Mode APORT1YCH25 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH27              0x0000003DUL                                  /**< Mode APORT1YCH27 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH29              0x0000003EUL                                  /**< Mode APORT1YCH29 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH31              0x0000003FUL                                  /**< Mode APORT1YCH31 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH0               0x00000050UL                                  /**< Mode APORT2YCH0 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH2               0x00000051UL                                  /**< Mode APORT2YCH2 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH4               0x00000052UL                                  /**< Mode APORT2YCH4 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH6               0x00000053UL                                  /**< Mode APORT2YCH6 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH8               0x00000054UL                                  /**< Mode APORT2YCH8 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH10              0x00000055UL                                  /**< Mode APORT2YCH10 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH12              0x00000056UL                                  /**< Mode APORT2YCH12 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH14              0x00000057UL                                  /**< Mode APORT2YCH14 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH16              0x00000058UL                                  /**< Mode APORT2YCH16 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH18              0x00000059UL                                  /**< Mode APORT2YCH18 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH20              0x0000005AUL                                  /**< Mode APORT2YCH20 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH22              0x0000005BUL                                  /**< Mode APORT2YCH22 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH24              0x0000005CUL                                  /**< Mode APORT2YCH24 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH26              0x0000005DUL                                  /**< Mode APORT2YCH26 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH28              0x0000005EUL                                  /**< Mode APORT2YCH28 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH30              0x0000005FUL                                  /**< Mode APORT2YCH30 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH1               0x00000070UL                                  /**< Mode APORT3YCH1 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH3               0x00000071UL                                  /**< Mode APORT3YCH3 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH5               0x00000072UL                                  /**< Mode APORT3YCH5 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH7               0x00000073UL                                  /**< Mode APORT3YCH7 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH9               0x00000074UL                                  /**< Mode APORT3YCH9 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH11              0x00000075UL                                  /**< Mode APORT3YCH11 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH13              0x00000076UL                                  /**< Mode APORT3YCH13 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH15              0x00000077UL                                  /**< Mode APORT3YCH15 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH17              0x00000078UL                                  /**< Mode APORT3YCH17 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH19              0x00000079UL                                  /**< Mode APORT3YCH19 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH21              0x0000007AUL                                  /**< Mode APORT3YCH21 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH23              0x0000007BUL                                  /**< Mode APORT3YCH23 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH25              0x0000007CUL                                  /**< Mode APORT3YCH25 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH27              0x0000007DUL                                  /**< Mode APORT3YCH27 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH29              0x0000007EUL                                  /**< Mode APORT3YCH29 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH31              0x0000007FUL                                  /**< Mode APORT3YCH31 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH0               0x00000090UL                                  /**< Mode APORT4YCH0 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH2               0x00000091UL                                  /**< Mode APORT4YCH2 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH4               0x00000092UL                                  /**< Mode APORT4YCH4 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH6               0x00000093UL                                  /**< Mode APORT4YCH6 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH8               0x00000094UL                                  /**< Mode APORT4YCH8 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH10              0x00000095UL                                  /**< Mode APORT4YCH10 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH12              0x00000096UL                                  /**< Mode APORT4YCH12 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH14              0x00000097UL                                  /**< Mode APORT4YCH14 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH16              0x00000098UL                                  /**< Mode APORT4YCH16 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH18              0x00000099UL                                  /**< Mode APORT4YCH18 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH20              0x0000009AUL                                  /**< Mode APORT4YCH20 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH22              0x0000009BUL                                  /**< Mode APORT4YCH22 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH24              0x0000009CUL                                  /**< Mode APORT4YCH24 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH26              0x0000009DUL                                  /**< Mode APORT4YCH26 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH28              0x0000009EUL                                  /**< Mode APORT4YCH28 for VDAC_OPA_OUT */
#define _VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH30              0x0000009FUL                                  /**< Mode APORT4YCH30 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_DEFAULT                   (_VDAC_OPA_OUT_APORTOUTSEL_DEFAULT << 16)     /**< Shifted mode DEFAULT for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH1                (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH1 << 16)  /**< Shifted mode APORT1YCH1 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH3                (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH3 << 16)  /**< Shifted mode APORT1YCH3 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH5                (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH5 << 16)  /**< Shifted mode APORT1YCH5 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH7                (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH7 << 16)  /**< Shifted mode APORT1YCH7 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH9                (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH9 << 16)  /**< Shifted mode APORT1YCH9 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH11               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH11 << 16) /**< Shifted mode APORT1YCH11 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH13               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH13 << 16) /**< Shifted mode APORT1YCH13 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH15               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH15 << 16) /**< Shifted mode APORT1YCH15 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH17               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH17 << 16) /**< Shifted mode APORT1YCH17 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH19               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH19 << 16) /**< Shifted mode APORT1YCH19 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH21               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH21 << 16) /**< Shifted mode APORT1YCH21 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH23               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH23 << 16) /**< Shifted mode APORT1YCH23 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH25               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH25 << 16) /**< Shifted mode APORT1YCH25 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH27               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH27 << 16) /**< Shifted mode APORT1YCH27 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH29               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH29 << 16) /**< Shifted mode APORT1YCH29 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH31               (_VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH31 << 16) /**< Shifted mode APORT1YCH31 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH0                (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH0 << 16)  /**< Shifted mode APORT2YCH0 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH2                (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH2 << 16)  /**< Shifted mode APORT2YCH2 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH4                (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH4 << 16)  /**< Shifted mode APORT2YCH4 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH6                (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH6 << 16)  /**< Shifted mode APORT2YCH6 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH8                (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH8 << 16)  /**< Shifted mode APORT2YCH8 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH10               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH10 << 16) /**< Shifted mode APORT2YCH10 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH12               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH12 << 16) /**< Shifted mode APORT2YCH12 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH14               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH14 << 16) /**< Shifted mode APORT2YCH14 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH16               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH16 << 16) /**< Shifted mode APORT2YCH16 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH18               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH18 << 16) /**< Shifted mode APORT2YCH18 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH20               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH20 << 16) /**< Shifted mode APORT2YCH20 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH22               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH22 << 16) /**< Shifted mode APORT2YCH22 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH24               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH24 << 16) /**< Shifted mode APORT2YCH24 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH26               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH26 << 16) /**< Shifted mode APORT2YCH26 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH28               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH28 << 16) /**< Shifted mode APORT2YCH28 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH30               (_VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH30 << 16) /**< Shifted mode APORT2YCH30 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH1                (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH1 << 16)  /**< Shifted mode APORT3YCH1 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH3                (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH3 << 16)  /**< Shifted mode APORT3YCH3 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH5                (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH5 << 16)  /**< Shifted mode APORT3YCH5 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH7                (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH7 << 16)  /**< Shifted mode APORT3YCH7 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH9                (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH9 << 16)  /**< Shifted mode APORT3YCH9 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH11               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH11 << 16) /**< Shifted mode APORT3YCH11 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH13               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH13 << 16) /**< Shifted mode APORT3YCH13 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH15               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH15 << 16) /**< Shifted mode APORT3YCH15 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH17               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH17 << 16) /**< Shifted mode APORT3YCH17 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH19               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH19 << 16) /**< Shifted mode APORT3YCH19 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH21               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH21 << 16) /**< Shifted mode APORT3YCH21 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH23               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH23 << 16) /**< Shifted mode APORT3YCH23 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH25               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH25 << 16) /**< Shifted mode APORT3YCH25 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH27               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH27 << 16) /**< Shifted mode APORT3YCH27 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH29               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH29 << 16) /**< Shifted mode APORT3YCH29 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH31               (_VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH31 << 16) /**< Shifted mode APORT3YCH31 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH0                (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH0 << 16)  /**< Shifted mode APORT4YCH0 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH2                (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH2 << 16)  /**< Shifted mode APORT4YCH2 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH4                (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH4 << 16)  /**< Shifted mode APORT4YCH4 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH6                (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH6 << 16)  /**< Shifted mode APORT4YCH6 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH8                (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH8 << 16)  /**< Shifted mode APORT4YCH8 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH10               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH10 << 16) /**< Shifted mode APORT4YCH10 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH12               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH12 << 16) /**< Shifted mode APORT4YCH12 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH14               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH14 << 16) /**< Shifted mode APORT4YCH14 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH16               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH16 << 16) /**< Shifted mode APORT4YCH16 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH18               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH18 << 16) /**< Shifted mode APORT4YCH18 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH20               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH20 << 16) /**< Shifted mode APORT4YCH20 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH22               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH22 << 16) /**< Shifted mode APORT4YCH22 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH24               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH24 << 16) /**< Shifted mode APORT4YCH24 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH26               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH26 << 16) /**< Shifted mode APORT4YCH26 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH28               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH28 << 16) /**< Shifted mode APORT4YCH28 for VDAC_OPA_OUT */
#define VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH30               (_VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH30 << 16) /**< Shifted mode APORT4YCH30 for VDAC_OPA_OUT */

/* Bit fields for VDAC OPA_CAL */
#define _VDAC_OPA_CAL_RESETVALUE                           0x000080E7UL                          /**< Default value for VDAC_OPA_CAL */
#define _VDAC_OPA_CAL_MASK                                 0x7DF6EDEFUL                          /**< Mask for VDAC_OPA_CAL */
#define _VDAC_OPA_CAL_CM1_SHIFT                            0                                     /**< Shift value for VDAC_OPACM1 */
#define _VDAC_OPA_CAL_CM1_MASK                             0xFUL                                 /**< Bit mask for VDAC_OPACM1 */
#define _VDAC_OPA_CAL_CM1_DEFAULT                          0x00000007UL                          /**< Mode DEFAULT for VDAC_OPA_CAL */
#define VDAC_OPA_CAL_CM1_DEFAULT                           (_VDAC_OPA_CAL_CM1_DEFAULT << 0)      /**< Shifted mode DEFAULT for VDAC_OPA_CAL */
#define _VDAC_OPA_CAL_CM2_SHIFT                            5                                     /**< Shift value for VDAC_OPACM2 */
#define _VDAC_OPA_CAL_CM2_MASK                             0x1E0UL                               /**< Bit mask for VDAC_OPACM2 */
#define _VDAC_OPA_CAL_CM2_DEFAULT                          0x00000007UL                          /**< Mode DEFAULT for VDAC_OPA_CAL */
#define VDAC_OPA_CAL_CM2_DEFAULT                           (_VDAC_OPA_CAL_CM2_DEFAULT << 5)      /**< Shifted mode DEFAULT for VDAC_OPA_CAL */
#define _VDAC_OPA_CAL_CM3_SHIFT                            10                                    /**< Shift value for VDAC_OPACM3 */
#define _VDAC_OPA_CAL_CM3_MASK                             0xC00UL                               /**< Bit mask for VDAC_OPACM3 */
#define _VDAC_OPA_CAL_CM3_DEFAULT                          0x00000000UL                          /**< Mode DEFAULT for VDAC_OPA_CAL */
#define VDAC_OPA_CAL_CM3_DEFAULT                           (_VDAC_OPA_CAL_CM3_DEFAULT << 10)     /**< Shifted mode DEFAULT for VDAC_OPA_CAL */
#define _VDAC_OPA_CAL_GM_SHIFT                             13                                    /**< Shift value for VDAC_OPAGM */
#define _VDAC_OPA_CAL_GM_MASK                              0xE000UL                              /**< Bit mask for VDAC_OPAGM */
#define _VDAC_OPA_CAL_GM_DEFAULT                           0x00000004UL                          /**< Mode DEFAULT for VDAC_OPA_CAL */
#define VDAC_OPA_CAL_GM_DEFAULT                            (_VDAC_OPA_CAL_GM_DEFAULT << 13)      /**< Shifted mode DEFAULT for VDAC_OPA_CAL */
#define _VDAC_OPA_CAL_GM3_SHIFT                            17                                    /**< Shift value for VDAC_OPAGM3 */
#define _VDAC_OPA_CAL_GM3_MASK                             0x60000UL                             /**< Bit mask for VDAC_OPAGM3 */
#define _VDAC_OPA_CAL_GM3_DEFAULT                          0x00000000UL                          /**< Mode DEFAULT for VDAC_OPA_CAL */
#define VDAC_OPA_CAL_GM3_DEFAULT                           (_VDAC_OPA_CAL_GM3_DEFAULT << 17)     /**< Shifted mode DEFAULT for VDAC_OPA_CAL */
#define _VDAC_OPA_CAL_OFFSETP_SHIFT                        20                                    /**< Shift value for VDAC_OPAOFFSETP */
#define _VDAC_OPA_CAL_OFFSETP_MASK                         0x1F00000UL                           /**< Bit mask for VDAC_OPAOFFSETP */
#define _VDAC_OPA_CAL_OFFSETP_DEFAULT                      0x00000000UL                          /**< Mode DEFAULT for VDAC_OPA_CAL */
#define VDAC_OPA_CAL_OFFSETP_DEFAULT                       (_VDAC_OPA_CAL_OFFSETP_DEFAULT << 20) /**< Shifted mode DEFAULT for VDAC_OPA_CAL */
#define _VDAC_OPA_CAL_OFFSETN_SHIFT                        26                                    /**< Shift value for VDAC_OPAOFFSETN */
#define _VDAC_OPA_CAL_OFFSETN_MASK                         0x7C000000UL                          /**< Bit mask for VDAC_OPAOFFSETN */
#define _VDAC_OPA_CAL_OFFSETN_DEFAULT                      0x00000000UL                          /**< Mode DEFAULT for VDAC_OPA_CAL */
#define VDAC_OPA_CAL_OFFSETN_DEFAULT                       (_VDAC_OPA_CAL_OFFSETN_DEFAULT << 26) /**< Shifted mode DEFAULT for VDAC_OPA_CAL */

/** @} */
/** @} End of group EFR32MG12P_VDAC */
/** @} End of group Parts */
