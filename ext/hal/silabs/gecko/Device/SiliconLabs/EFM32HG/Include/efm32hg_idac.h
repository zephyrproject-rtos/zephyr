/**************************************************************************//**
 * @file efm32hg_idac.h
 * @brief EFM32HG_IDAC register and bit field definitions
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
 * @defgroup EFM32HG_IDAC
 * @{
 * @brief EFM32HG_IDAC Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;       /**< Control Register  */
  __IOM uint32_t CURPROG;    /**< Current Programming Register  */
  __IOM uint32_t CAL;        /**< Calibration Register  */
  __IOM uint32_t DUTYCONFIG; /**< Duty Cycle Configauration Register  */
} IDAC_TypeDef;              /** @} */

/**************************************************************************//**
 * @defgroup EFM32HG_IDAC_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for IDAC CTRL */
#define _IDAC_CTRL_RESETVALUE                       0x00000000UL                          /**< Default value for IDAC_CTRL */
#define _IDAC_CTRL_MASK                             0x0074001FUL                          /**< Mask for IDAC_CTRL */
#define IDAC_CTRL_EN                                (0x1UL << 0)                          /**< Current DAC Enable */
#define _IDAC_CTRL_EN_SHIFT                         0                                     /**< Shift value for IDAC_EN */
#define _IDAC_CTRL_EN_MASK                          0x1UL                                 /**< Bit mask for IDAC_EN */
#define _IDAC_CTRL_EN_DEFAULT                       0x00000000UL                          /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_EN_DEFAULT                        (_IDAC_CTRL_EN_DEFAULT << 0)          /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_CURSINK                           (0x1UL << 1)                          /**< Current Sink Enable */
#define _IDAC_CTRL_CURSINK_SHIFT                    1                                     /**< Shift value for IDAC_CURSINK */
#define _IDAC_CTRL_CURSINK_MASK                     0x2UL                                 /**< Bit mask for IDAC_CURSINK */
#define _IDAC_CTRL_CURSINK_DEFAULT                  0x00000000UL                          /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_CURSINK_DEFAULT                   (_IDAC_CTRL_CURSINK_DEFAULT << 1)     /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_MINOUTTRANS                       (0x1UL << 2)                          /**< Minimum Output Transition Enable */
#define _IDAC_CTRL_MINOUTTRANS_SHIFT                2                                     /**< Shift value for IDAC_MINOUTTRANS */
#define _IDAC_CTRL_MINOUTTRANS_MASK                 0x4UL                                 /**< Bit mask for IDAC_MINOUTTRANS */
#define _IDAC_CTRL_MINOUTTRANS_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_MINOUTTRANS_DEFAULT               (_IDAC_CTRL_MINOUTTRANS_DEFAULT << 2) /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_OUTEN                             (0x1UL << 3)                          /**< Output Enable */
#define _IDAC_CTRL_OUTEN_SHIFT                      3                                     /**< Shift value for IDAC_OUTEN */
#define _IDAC_CTRL_OUTEN_MASK                       0x8UL                                 /**< Bit mask for IDAC_OUTEN */
#define _IDAC_CTRL_OUTEN_DEFAULT                    0x00000000UL                          /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_OUTEN_DEFAULT                     (_IDAC_CTRL_OUTEN_DEFAULT << 3)       /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_OUTMODE                           (0x1UL << 4)                          /**< Output Modes */
#define _IDAC_CTRL_OUTMODE_SHIFT                    4                                     /**< Shift value for IDAC_OUTMODE */
#define _IDAC_CTRL_OUTMODE_MASK                     0x10UL                                /**< Bit mask for IDAC_OUTMODE */
#define _IDAC_CTRL_OUTMODE_DEFAULT                  0x00000000UL                          /**< Mode DEFAULT for IDAC_CTRL */
#define _IDAC_CTRL_OUTMODE_PIN                      0x00000000UL                          /**< Mode PIN for IDAC_CTRL */
#define _IDAC_CTRL_OUTMODE_ADC                      0x00000001UL                          /**< Mode ADC for IDAC_CTRL */
#define IDAC_CTRL_OUTMODE_DEFAULT                   (_IDAC_CTRL_OUTMODE_DEFAULT << 4)     /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_OUTMODE_PIN                       (_IDAC_CTRL_OUTMODE_PIN << 4)         /**< Shifted mode PIN for IDAC_CTRL */
#define IDAC_CTRL_OUTMODE_ADC                       (_IDAC_CTRL_OUTMODE_ADC << 4)         /**< Shifted mode ADC for IDAC_CTRL */
#define IDAC_CTRL_OUTENPRS                          (0x1UL << 18)                         /**< PRS Controlled Output Enable */
#define _IDAC_CTRL_OUTENPRS_SHIFT                   18                                    /**< Shift value for IDAC_OUTENPRS */
#define _IDAC_CTRL_OUTENPRS_MASK                    0x40000UL                             /**< Bit mask for IDAC_OUTENPRS */
#define _IDAC_CTRL_OUTENPRS_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_OUTENPRS_DEFAULT                  (_IDAC_CTRL_OUTENPRS_DEFAULT << 18)   /**< Shifted mode DEFAULT for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_SHIFT                     20                                    /**< Shift value for IDAC_PRSSEL */
#define _IDAC_CTRL_PRSSEL_MASK                      0x700000UL                            /**< Bit mask for IDAC_PRSSEL */
#define _IDAC_CTRL_PRSSEL_DEFAULT                   0x00000000UL                          /**< Mode DEFAULT for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH0                    0x00000000UL                          /**< Mode PRSCH0 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH1                    0x00000001UL                          /**< Mode PRSCH1 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH2                    0x00000002UL                          /**< Mode PRSCH2 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH3                    0x00000003UL                          /**< Mode PRSCH3 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH4                    0x00000004UL                          /**< Mode PRSCH4 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH5                    0x00000005UL                          /**< Mode PRSCH5 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_DEFAULT                    (_IDAC_CTRL_PRSSEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH0                     (_IDAC_CTRL_PRSSEL_PRSCH0 << 20)      /**< Shifted mode PRSCH0 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH1                     (_IDAC_CTRL_PRSSEL_PRSCH1 << 20)      /**< Shifted mode PRSCH1 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH2                     (_IDAC_CTRL_PRSSEL_PRSCH2 << 20)      /**< Shifted mode PRSCH2 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH3                     (_IDAC_CTRL_PRSSEL_PRSCH3 << 20)      /**< Shifted mode PRSCH3 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH4                     (_IDAC_CTRL_PRSSEL_PRSCH4 << 20)      /**< Shifted mode PRSCH4 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH5                     (_IDAC_CTRL_PRSSEL_PRSCH5 << 20)      /**< Shifted mode PRSCH5 for IDAC_CTRL */

/* Bit fields for IDAC CURPROG */
#define _IDAC_CURPROG_RESETVALUE                    0x00000000UL                          /**< Default value for IDAC_CURPROG */
#define _IDAC_CURPROG_MASK                          0x00001F03UL                          /**< Mask for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_SHIFT                0                                     /**< Shift value for IDAC_RANGESEL */
#define _IDAC_CURPROG_RANGESEL_MASK                 0x3UL                                 /**< Bit mask for IDAC_RANGESEL */
#define _IDAC_CURPROG_RANGESEL_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_RANGE0               0x00000000UL                          /**< Mode RANGE0 for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_RANGE1               0x00000001UL                          /**< Mode RANGE1 for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_RANGE2               0x00000002UL                          /**< Mode RANGE2 for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_RANGE3               0x00000003UL                          /**< Mode RANGE3 for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_DEFAULT               (_IDAC_CURPROG_RANGESEL_DEFAULT << 0) /**< Shifted mode DEFAULT for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_RANGE0                (_IDAC_CURPROG_RANGESEL_RANGE0 << 0)  /**< Shifted mode RANGE0 for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_RANGE1                (_IDAC_CURPROG_RANGESEL_RANGE1 << 0)  /**< Shifted mode RANGE1 for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_RANGE2                (_IDAC_CURPROG_RANGESEL_RANGE2 << 0)  /**< Shifted mode RANGE2 for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_RANGE3                (_IDAC_CURPROG_RANGESEL_RANGE3 << 0)  /**< Shifted mode RANGE3 for IDAC_CURPROG */
#define _IDAC_CURPROG_STEPSEL_SHIFT                 8                                     /**< Shift value for IDAC_STEPSEL */
#define _IDAC_CURPROG_STEPSEL_MASK                  0x1F00UL                              /**< Bit mask for IDAC_STEPSEL */
#define _IDAC_CURPROG_STEPSEL_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for IDAC_CURPROG */
#define IDAC_CURPROG_STEPSEL_DEFAULT                (_IDAC_CURPROG_STEPSEL_DEFAULT << 8)  /**< Shifted mode DEFAULT for IDAC_CURPROG */

/* Bit fields for IDAC CAL */
#define _IDAC_CAL_RESETVALUE                        0x00000000UL                    /**< Default value for IDAC_CAL */
#define _IDAC_CAL_MASK                              0x0000007FUL                    /**< Mask for IDAC_CAL */
#define _IDAC_CAL_TUNING_SHIFT                      0                               /**< Shift value for IDAC_TUNING */
#define _IDAC_CAL_TUNING_MASK                       0x7FUL                          /**< Bit mask for IDAC_TUNING */
#define _IDAC_CAL_TUNING_DEFAULT                    0x00000000UL                    /**< Mode DEFAULT for IDAC_CAL */
#define IDAC_CAL_TUNING_DEFAULT                     (_IDAC_CAL_TUNING_DEFAULT << 0) /**< Shifted mode DEFAULT for IDAC_CAL */

/* Bit fields for IDAC DUTYCONFIG */
#define _IDAC_DUTYCONFIG_RESETVALUE                 0x00000000UL                                    /**< Default value for IDAC_DUTYCONFIG */
#define _IDAC_DUTYCONFIG_MASK                       0x00000003UL                                    /**< Mask for IDAC_DUTYCONFIG */
#define IDAC_DUTYCONFIG_DUTYCYCLEEN                 (0x1UL << 0)                                    /**< Duty Cycle Enable. */
#define _IDAC_DUTYCONFIG_DUTYCYCLEEN_SHIFT          0                                               /**< Shift value for IDAC_DUTYCYCLEEN */
#define _IDAC_DUTYCONFIG_DUTYCYCLEEN_MASK           0x1UL                                           /**< Bit mask for IDAC_DUTYCYCLEEN */
#define _IDAC_DUTYCONFIG_DUTYCYCLEEN_DEFAULT        0x00000000UL                                    /**< Mode DEFAULT for IDAC_DUTYCONFIG */
#define IDAC_DUTYCONFIG_DUTYCYCLEEN_DEFAULT         (_IDAC_DUTYCONFIG_DUTYCYCLEEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for IDAC_DUTYCONFIG */
#define IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS             (0x1UL << 1)                                    /**< EM2/EM3 Duty Cycle Disable. */
#define _IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_SHIFT      1                                               /**< Shift value for IDAC_EM2DUTYCYCLEDIS */
#define _IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_MASK       0x2UL                                           /**< Bit mask for IDAC_EM2DUTYCYCLEDIS */
#define _IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_DEFAULT    0x00000000UL                                    /**< Mode DEFAULT for IDAC_DUTYCONFIG */
#define IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_DEFAULT     (_IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_DEFAULT << 1) /**< Shifted mode DEFAULT for IDAC_DUTYCONFIG */

/** @} End of group EFM32HG_IDAC */
/** @} End of group Parts */

