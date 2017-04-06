/**************************************************************************//**
 * @file efm32wg_adc.h
 * @brief EFM32WG_ADC register and bit field definitions
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
 * @defgroup EFM32WG_ADC
 * @{
 * @brief EFM32WG_ADC Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;         /**< Control Register  */
  __IOM uint32_t CMD;          /**< Command Register  */
  __IM uint32_t  STATUS;       /**< Status Register  */
  __IOM uint32_t SINGLECTRL;   /**< Single Sample Control Register  */
  __IOM uint32_t SCANCTRL;     /**< Scan Control Register  */
  __IOM uint32_t IEN;          /**< Interrupt Enable Register  */
  __IM uint32_t  IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;          /**< Interrupt Flag Clear Register  */
  __IM uint32_t  SINGLEDATA;   /**< Single Conversion Result Data  */
  __IM uint32_t  SCANDATA;     /**< Scan Conversion Result Data  */
  __IM uint32_t  SINGLEDATAP;  /**< Single Conversion Result Data Peek Register  */
  __IM uint32_t  SCANDATAP;    /**< Scan Sequence Result Data Peek Register  */
  __IOM uint32_t CAL;          /**< Calibration Register  */

  uint32_t       RESERVED0[1]; /**< Reserved for future use **/
  __IOM uint32_t BIASPROG;     /**< Bias Programming Register  */
} ADC_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_ADC_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for ADC CTRL */
#define _ADC_CTRL_RESETVALUE                    0x001F0000UL                                /**< Default value for ADC_CTRL */
#define _ADC_CTRL_MASK                          0x0F7F7F3BUL                                /**< Mask for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_SHIFT              0                                           /**< Shift value for ADC_WARMUPMODE */
#define _ADC_CTRL_WARMUPMODE_MASK               0x3UL                                       /**< Bit mask for ADC_WARMUPMODE */
#define _ADC_CTRL_WARMUPMODE_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_NORMAL             0x00000000UL                                /**< Mode NORMAL for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_FASTBG             0x00000001UL                                /**< Mode FASTBG for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_KEEPSCANREFWARM    0x00000002UL                                /**< Mode KEEPSCANREFWARM for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_KEEPADCWARM        0x00000003UL                                /**< Mode KEEPADCWARM for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_DEFAULT             (_ADC_CTRL_WARMUPMODE_DEFAULT << 0)         /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_NORMAL              (_ADC_CTRL_WARMUPMODE_NORMAL << 0)          /**< Shifted mode NORMAL for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_FASTBG              (_ADC_CTRL_WARMUPMODE_FASTBG << 0)          /**< Shifted mode FASTBG for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_KEEPSCANREFWARM     (_ADC_CTRL_WARMUPMODE_KEEPSCANREFWARM << 0) /**< Shifted mode KEEPSCANREFWARM for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_KEEPADCWARM         (_ADC_CTRL_WARMUPMODE_KEEPADCWARM << 0)     /**< Shifted mode KEEPADCWARM for ADC_CTRL */
#define ADC_CTRL_TAILGATE                       (0x1UL << 3)                                /**< Conversion Tailgating */
#define _ADC_CTRL_TAILGATE_SHIFT                3                                           /**< Shift value for ADC_TAILGATE */
#define _ADC_CTRL_TAILGATE_MASK                 0x8UL                                       /**< Bit mask for ADC_TAILGATE */
#define _ADC_CTRL_TAILGATE_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_TAILGATE_DEFAULT               (_ADC_CTRL_TAILGATE_DEFAULT << 3)           /**< Shifted mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_LPFMODE_SHIFT                 4                                           /**< Shift value for ADC_LPFMODE */
#define _ADC_CTRL_LPFMODE_MASK                  0x30UL                                      /**< Bit mask for ADC_LPFMODE */
#define _ADC_CTRL_LPFMODE_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_LPFMODE_BYPASS                0x00000000UL                                /**< Mode BYPASS for ADC_CTRL */
#define _ADC_CTRL_LPFMODE_DECAP                 0x00000001UL                                /**< Mode DECAP for ADC_CTRL */
#define _ADC_CTRL_LPFMODE_RCFILT                0x00000002UL                                /**< Mode RCFILT for ADC_CTRL */
#define ADC_CTRL_LPFMODE_DEFAULT                (_ADC_CTRL_LPFMODE_DEFAULT << 4)            /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_LPFMODE_BYPASS                 (_ADC_CTRL_LPFMODE_BYPASS << 4)             /**< Shifted mode BYPASS for ADC_CTRL */
#define ADC_CTRL_LPFMODE_DECAP                  (_ADC_CTRL_LPFMODE_DECAP << 4)              /**< Shifted mode DECAP for ADC_CTRL */
#define ADC_CTRL_LPFMODE_RCFILT                 (_ADC_CTRL_LPFMODE_RCFILT << 4)             /**< Shifted mode RCFILT for ADC_CTRL */
#define _ADC_CTRL_PRESC_SHIFT                   8                                           /**< Shift value for ADC_PRESC */
#define _ADC_CTRL_PRESC_MASK                    0x7F00UL                                    /**< Bit mask for ADC_PRESC */
#define _ADC_CTRL_PRESC_DEFAULT                 0x00000000UL                                /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_PRESC_NODIVISION              0x00000000UL                                /**< Mode NODIVISION for ADC_CTRL */
#define ADC_CTRL_PRESC_DEFAULT                  (_ADC_CTRL_PRESC_DEFAULT << 8)              /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_PRESC_NODIVISION               (_ADC_CTRL_PRESC_NODIVISION << 8)           /**< Shifted mode NODIVISION for ADC_CTRL */
#define _ADC_CTRL_TIMEBASE_SHIFT                16                                          /**< Shift value for ADC_TIMEBASE */
#define _ADC_CTRL_TIMEBASE_MASK                 0x7F0000UL                                  /**< Bit mask for ADC_TIMEBASE */
#define _ADC_CTRL_TIMEBASE_DEFAULT              0x0000001FUL                                /**< Mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_TIMEBASE_DEFAULT               (_ADC_CTRL_TIMEBASE_DEFAULT << 16)          /**< Shifted mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_SHIFT                 24                                          /**< Shift value for ADC_OVSRSEL */
#define _ADC_CTRL_OVSRSEL_MASK                  0xF000000UL                                 /**< Bit mask for ADC_OVSRSEL */
#define _ADC_CTRL_OVSRSEL_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X2                    0x00000000UL                                /**< Mode X2 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X4                    0x00000001UL                                /**< Mode X4 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X8                    0x00000002UL                                /**< Mode X8 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X16                   0x00000003UL                                /**< Mode X16 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X32                   0x00000004UL                                /**< Mode X32 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X64                   0x00000005UL                                /**< Mode X64 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X128                  0x00000006UL                                /**< Mode X128 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X256                  0x00000007UL                                /**< Mode X256 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X512                  0x00000008UL                                /**< Mode X512 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X1024                 0x00000009UL                                /**< Mode X1024 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X2048                 0x0000000AUL                                /**< Mode X2048 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X4096                 0x0000000BUL                                /**< Mode X4096 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_DEFAULT                (_ADC_CTRL_OVSRSEL_DEFAULT << 24)           /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X2                     (_ADC_CTRL_OVSRSEL_X2 << 24)                /**< Shifted mode X2 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X4                     (_ADC_CTRL_OVSRSEL_X4 << 24)                /**< Shifted mode X4 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X8                     (_ADC_CTRL_OVSRSEL_X8 << 24)                /**< Shifted mode X8 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X16                    (_ADC_CTRL_OVSRSEL_X16 << 24)               /**< Shifted mode X16 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X32                    (_ADC_CTRL_OVSRSEL_X32 << 24)               /**< Shifted mode X32 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X64                    (_ADC_CTRL_OVSRSEL_X64 << 24)               /**< Shifted mode X64 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X128                   (_ADC_CTRL_OVSRSEL_X128 << 24)              /**< Shifted mode X128 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X256                   (_ADC_CTRL_OVSRSEL_X256 << 24)              /**< Shifted mode X256 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X512                   (_ADC_CTRL_OVSRSEL_X512 << 24)              /**< Shifted mode X512 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X1024                  (_ADC_CTRL_OVSRSEL_X1024 << 24)             /**< Shifted mode X1024 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X2048                  (_ADC_CTRL_OVSRSEL_X2048 << 24)             /**< Shifted mode X2048 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X4096                  (_ADC_CTRL_OVSRSEL_X4096 << 24)             /**< Shifted mode X4096 for ADC_CTRL */

/* Bit fields for ADC CMD */
#define _ADC_CMD_RESETVALUE                     0x00000000UL                        /**< Default value for ADC_CMD */
#define _ADC_CMD_MASK                           0x0000000FUL                        /**< Mask for ADC_CMD */
#define ADC_CMD_SINGLESTART                     (0x1UL << 0)                        /**< Single Conversion Start */
#define _ADC_CMD_SINGLESTART_SHIFT              0                                   /**< Shift value for ADC_SINGLESTART */
#define _ADC_CMD_SINGLESTART_MASK               0x1UL                               /**< Bit mask for ADC_SINGLESTART */
#define _ADC_CMD_SINGLESTART_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for ADC_CMD */
#define ADC_CMD_SINGLESTART_DEFAULT             (_ADC_CMD_SINGLESTART_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_CMD */
#define ADC_CMD_SINGLESTOP                      (0x1UL << 1)                        /**< Single Conversion Stop */
#define _ADC_CMD_SINGLESTOP_SHIFT               1                                   /**< Shift value for ADC_SINGLESTOP */
#define _ADC_CMD_SINGLESTOP_MASK                0x2UL                               /**< Bit mask for ADC_SINGLESTOP */
#define _ADC_CMD_SINGLESTOP_DEFAULT             0x00000000UL                        /**< Mode DEFAULT for ADC_CMD */
#define ADC_CMD_SINGLESTOP_DEFAULT              (_ADC_CMD_SINGLESTOP_DEFAULT << 1)  /**< Shifted mode DEFAULT for ADC_CMD */
#define ADC_CMD_SCANSTART                       (0x1UL << 2)                        /**< Scan Sequence Start */
#define _ADC_CMD_SCANSTART_SHIFT                2                                   /**< Shift value for ADC_SCANSTART */
#define _ADC_CMD_SCANSTART_MASK                 0x4UL                               /**< Bit mask for ADC_SCANSTART */
#define _ADC_CMD_SCANSTART_DEFAULT              0x00000000UL                        /**< Mode DEFAULT for ADC_CMD */
#define ADC_CMD_SCANSTART_DEFAULT               (_ADC_CMD_SCANSTART_DEFAULT << 2)   /**< Shifted mode DEFAULT for ADC_CMD */
#define ADC_CMD_SCANSTOP                        (0x1UL << 3)                        /**< Scan Sequence Stop */
#define _ADC_CMD_SCANSTOP_SHIFT                 3                                   /**< Shift value for ADC_SCANSTOP */
#define _ADC_CMD_SCANSTOP_MASK                  0x8UL                               /**< Bit mask for ADC_SCANSTOP */
#define _ADC_CMD_SCANSTOP_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for ADC_CMD */
#define ADC_CMD_SCANSTOP_DEFAULT                (_ADC_CMD_SCANSTOP_DEFAULT << 3)    /**< Shifted mode DEFAULT for ADC_CMD */

/* Bit fields for ADC STATUS */
#define _ADC_STATUS_RESETVALUE                  0x00000000UL                             /**< Default value for ADC_STATUS */
#define _ADC_STATUS_MASK                        0x07031303UL                             /**< Mask for ADC_STATUS */
#define ADC_STATUS_SINGLEACT                    (0x1UL << 0)                             /**< Single Conversion Active */
#define _ADC_STATUS_SINGLEACT_SHIFT             0                                        /**< Shift value for ADC_SINGLEACT */
#define _ADC_STATUS_SINGLEACT_MASK              0x1UL                                    /**< Bit mask for ADC_SINGLEACT */
#define _ADC_STATUS_SINGLEACT_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEACT_DEFAULT            (_ADC_STATUS_SINGLEACT_DEFAULT << 0)     /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANACT                      (0x1UL << 1)                             /**< Scan Conversion Active */
#define _ADC_STATUS_SCANACT_SHIFT               1                                        /**< Shift value for ADC_SCANACT */
#define _ADC_STATUS_SCANACT_MASK                0x2UL                                    /**< Bit mask for ADC_SCANACT */
#define _ADC_STATUS_SCANACT_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANACT_DEFAULT              (_ADC_STATUS_SCANACT_DEFAULT << 1)       /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEREFWARM                (0x1UL << 8)                             /**< Single Reference Warmed Up */
#define _ADC_STATUS_SINGLEREFWARM_SHIFT         8                                        /**< Shift value for ADC_SINGLEREFWARM */
#define _ADC_STATUS_SINGLEREFWARM_MASK          0x100UL                                  /**< Bit mask for ADC_SINGLEREFWARM */
#define _ADC_STATUS_SINGLEREFWARM_DEFAULT       0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEREFWARM_DEFAULT        (_ADC_STATUS_SINGLEREFWARM_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANREFWARM                  (0x1UL << 9)                             /**< Scan Reference Warmed Up */
#define _ADC_STATUS_SCANREFWARM_SHIFT           9                                        /**< Shift value for ADC_SCANREFWARM */
#define _ADC_STATUS_SCANREFWARM_MASK            0x200UL                                  /**< Bit mask for ADC_SCANREFWARM */
#define _ADC_STATUS_SCANREFWARM_DEFAULT         0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANREFWARM_DEFAULT          (_ADC_STATUS_SCANREFWARM_DEFAULT << 9)   /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_WARM                         (0x1UL << 12)                            /**< ADC Warmed Up */
#define _ADC_STATUS_WARM_SHIFT                  12                                       /**< Shift value for ADC_WARM */
#define _ADC_STATUS_WARM_MASK                   0x1000UL                                 /**< Bit mask for ADC_WARM */
#define _ADC_STATUS_WARM_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_WARM_DEFAULT                 (_ADC_STATUS_WARM_DEFAULT << 12)         /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEDV                     (0x1UL << 16)                            /**< Single Sample Data Valid */
#define _ADC_STATUS_SINGLEDV_SHIFT              16                                       /**< Shift value for ADC_SINGLEDV */
#define _ADC_STATUS_SINGLEDV_MASK               0x10000UL                                /**< Bit mask for ADC_SINGLEDV */
#define _ADC_STATUS_SINGLEDV_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEDV_DEFAULT             (_ADC_STATUS_SINGLEDV_DEFAULT << 16)     /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANDV                       (0x1UL << 17)                            /**< Scan Data Valid */
#define _ADC_STATUS_SCANDV_SHIFT                17                                       /**< Shift value for ADC_SCANDV */
#define _ADC_STATUS_SCANDV_MASK                 0x20000UL                                /**< Bit mask for ADC_SCANDV */
#define _ADC_STATUS_SCANDV_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANDV_DEFAULT               (_ADC_STATUS_SCANDV_DEFAULT << 17)       /**< Shifted mode DEFAULT for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_SHIFT           24                                       /**< Shift value for ADC_SCANDATASRC */
#define _ADC_STATUS_SCANDATASRC_MASK            0x7000000UL                              /**< Bit mask for ADC_SCANDATASRC */
#define _ADC_STATUS_SCANDATASRC_DEFAULT         0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_CH0             0x00000000UL                             /**< Mode CH0 for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_CH1             0x00000001UL                             /**< Mode CH1 for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_CH2             0x00000002UL                             /**< Mode CH2 for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_CH3             0x00000003UL                             /**< Mode CH3 for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_CH4             0x00000004UL                             /**< Mode CH4 for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_CH5             0x00000005UL                             /**< Mode CH5 for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_CH6             0x00000006UL                             /**< Mode CH6 for ADC_STATUS */
#define _ADC_STATUS_SCANDATASRC_CH7             0x00000007UL                             /**< Mode CH7 for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_DEFAULT          (_ADC_STATUS_SCANDATASRC_DEFAULT << 24)  /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_CH0              (_ADC_STATUS_SCANDATASRC_CH0 << 24)      /**< Shifted mode CH0 for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_CH1              (_ADC_STATUS_SCANDATASRC_CH1 << 24)      /**< Shifted mode CH1 for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_CH2              (_ADC_STATUS_SCANDATASRC_CH2 << 24)      /**< Shifted mode CH2 for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_CH3              (_ADC_STATUS_SCANDATASRC_CH3 << 24)      /**< Shifted mode CH3 for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_CH4              (_ADC_STATUS_SCANDATASRC_CH4 << 24)      /**< Shifted mode CH4 for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_CH5              (_ADC_STATUS_SCANDATASRC_CH5 << 24)      /**< Shifted mode CH5 for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_CH6              (_ADC_STATUS_SCANDATASRC_CH6 << 24)      /**< Shifted mode CH6 for ADC_STATUS */
#define ADC_STATUS_SCANDATASRC_CH7              (_ADC_STATUS_SCANDATASRC_CH7 << 24)      /**< Shifted mode CH7 for ADC_STATUS */

/* Bit fields for ADC SINGLECTRL */
#define _ADC_SINGLECTRL_RESETVALUE              0x00000000UL                             /**< Default value for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_MASK                    0xF1F70F37UL                             /**< Mask for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REP                      (0x1UL << 0)                             /**< Single Sample Repetitive Mode */
#define _ADC_SINGLECTRL_REP_SHIFT               0                                        /**< Shift value for ADC_REP */
#define _ADC_SINGLECTRL_REP_MASK                0x1UL                                    /**< Bit mask for ADC_REP */
#define _ADC_SINGLECTRL_REP_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REP_DEFAULT              (_ADC_SINGLECTRL_REP_DEFAULT << 0)       /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_DIFF                     (0x1UL << 1)                             /**< Single Sample Differential Mode */
#define _ADC_SINGLECTRL_DIFF_SHIFT              1                                        /**< Shift value for ADC_DIFF */
#define _ADC_SINGLECTRL_DIFF_MASK               0x2UL                                    /**< Bit mask for ADC_DIFF */
#define _ADC_SINGLECTRL_DIFF_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_DIFF_DEFAULT             (_ADC_SINGLECTRL_DIFF_DEFAULT << 1)      /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_ADJ                      (0x1UL << 2)                             /**< Single Sample Result Adjustment */
#define _ADC_SINGLECTRL_ADJ_SHIFT               2                                        /**< Shift value for ADC_ADJ */
#define _ADC_SINGLECTRL_ADJ_MASK                0x4UL                                    /**< Bit mask for ADC_ADJ */
#define _ADC_SINGLECTRL_ADJ_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_ADJ_RIGHT               0x00000000UL                             /**< Mode RIGHT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_ADJ_LEFT                0x00000001UL                             /**< Mode LEFT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_ADJ_DEFAULT              (_ADC_SINGLECTRL_ADJ_DEFAULT << 2)       /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_ADJ_RIGHT                (_ADC_SINGLECTRL_ADJ_RIGHT << 2)         /**< Shifted mode RIGHT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_ADJ_LEFT                 (_ADC_SINGLECTRL_ADJ_LEFT << 2)          /**< Shifted mode LEFT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_SHIFT               4                                        /**< Shift value for ADC_RES */
#define _ADC_SINGLECTRL_RES_MASK                0x30UL                                   /**< Bit mask for ADC_RES */
#define _ADC_SINGLECTRL_RES_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_12BIT               0x00000000UL                             /**< Mode 12BIT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_8BIT                0x00000001UL                             /**< Mode 8BIT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_6BIT                0x00000002UL                             /**< Mode 6BIT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_OVS                 0x00000003UL                             /**< Mode OVS for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_DEFAULT              (_ADC_SINGLECTRL_RES_DEFAULT << 4)       /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_12BIT                (_ADC_SINGLECTRL_RES_12BIT << 4)         /**< Shifted mode 12BIT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_8BIT                 (_ADC_SINGLECTRL_RES_8BIT << 4)          /**< Shifted mode 8BIT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_6BIT                 (_ADC_SINGLECTRL_RES_6BIT << 4)          /**< Shifted mode 6BIT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_OVS                  (_ADC_SINGLECTRL_RES_OVS << 4)           /**< Shifted mode OVS for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_SHIFT          8                                        /**< Shift value for ADC_INPUTSEL */
#define _ADC_SINGLECTRL_INPUTSEL_MASK           0xF00UL                                  /**< Bit mask for ADC_INPUTSEL */
#define _ADC_SINGLECTRL_INPUTSEL_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH0            0x00000000UL                             /**< Mode CH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH0CH1         0x00000000UL                             /**< Mode CH0CH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH1            0x00000001UL                             /**< Mode CH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH2CH3         0x00000001UL                             /**< Mode CH2CH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH2            0x00000002UL                             /**< Mode CH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH4CH5         0x00000002UL                             /**< Mode CH4CH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH6CH7         0x00000003UL                             /**< Mode CH6CH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH3            0x00000003UL                             /**< Mode CH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH4            0x00000004UL                             /**< Mode CH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_DIFF0          0x00000004UL                             /**< Mode DIFF0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH5            0x00000005UL                             /**< Mode CH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH6            0x00000006UL                             /**< Mode CH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_CH7            0x00000007UL                             /**< Mode CH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_TEMP           0x00000008UL                             /**< Mode TEMP for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_VDDDIV3        0x00000009UL                             /**< Mode VDDDIV3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_VDD            0x0000000AUL                             /**< Mode VDD for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_VSS            0x0000000BUL                             /**< Mode VSS for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_VREFDIV2       0x0000000CUL                             /**< Mode VREFDIV2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_DAC0OUT0       0x0000000DUL                             /**< Mode DAC0OUT0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_INPUTSEL_DAC0OUT1       0x0000000EUL                             /**< Mode DAC0OUT1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_DEFAULT         (_ADC_SINGLECTRL_INPUTSEL_DEFAULT << 8)  /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH0             (_ADC_SINGLECTRL_INPUTSEL_CH0 << 8)      /**< Shifted mode CH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH0CH1          (_ADC_SINGLECTRL_INPUTSEL_CH0CH1 << 8)   /**< Shifted mode CH0CH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH1             (_ADC_SINGLECTRL_INPUTSEL_CH1 << 8)      /**< Shifted mode CH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH2CH3          (_ADC_SINGLECTRL_INPUTSEL_CH2CH3 << 8)   /**< Shifted mode CH2CH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH2             (_ADC_SINGLECTRL_INPUTSEL_CH2 << 8)      /**< Shifted mode CH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH4CH5          (_ADC_SINGLECTRL_INPUTSEL_CH4CH5 << 8)   /**< Shifted mode CH4CH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH6CH7          (_ADC_SINGLECTRL_INPUTSEL_CH6CH7 << 8)   /**< Shifted mode CH6CH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH3             (_ADC_SINGLECTRL_INPUTSEL_CH3 << 8)      /**< Shifted mode CH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH4             (_ADC_SINGLECTRL_INPUTSEL_CH4 << 8)      /**< Shifted mode CH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_DIFF0           (_ADC_SINGLECTRL_INPUTSEL_DIFF0 << 8)    /**< Shifted mode DIFF0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH5             (_ADC_SINGLECTRL_INPUTSEL_CH5 << 8)      /**< Shifted mode CH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH6             (_ADC_SINGLECTRL_INPUTSEL_CH6 << 8)      /**< Shifted mode CH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_CH7             (_ADC_SINGLECTRL_INPUTSEL_CH7 << 8)      /**< Shifted mode CH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_TEMP            (_ADC_SINGLECTRL_INPUTSEL_TEMP << 8)     /**< Shifted mode TEMP for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_VDDDIV3         (_ADC_SINGLECTRL_INPUTSEL_VDDDIV3 << 8)  /**< Shifted mode VDDDIV3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_VDD             (_ADC_SINGLECTRL_INPUTSEL_VDD << 8)      /**< Shifted mode VDD for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_VSS             (_ADC_SINGLECTRL_INPUTSEL_VSS << 8)      /**< Shifted mode VSS for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_VREFDIV2        (_ADC_SINGLECTRL_INPUTSEL_VREFDIV2 << 8) /**< Shifted mode VREFDIV2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_DAC0OUT0        (_ADC_SINGLECTRL_INPUTSEL_DAC0OUT0 << 8) /**< Shifted mode DAC0OUT0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_INPUTSEL_DAC0OUT1        (_ADC_SINGLECTRL_INPUTSEL_DAC0OUT1 << 8) /**< Shifted mode DAC0OUT1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_SHIFT               16                                       /**< Shift value for ADC_REF */
#define _ADC_SINGLECTRL_REF_MASK                0x70000UL                                /**< Bit mask for ADC_REF */
#define _ADC_SINGLECTRL_REF_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_1V25                0x00000000UL                             /**< Mode 1V25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_2V5                 0x00000001UL                             /**< Mode 2V5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_VDD                 0x00000002UL                             /**< Mode VDD for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_5VDIFF              0x00000003UL                             /**< Mode 5VDIFF for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_EXTSINGLE           0x00000004UL                             /**< Mode EXTSINGLE for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_2XEXTDIFF           0x00000005UL                             /**< Mode 2XEXTDIFF for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_2XVDD               0x00000006UL                             /**< Mode 2XVDD for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_DEFAULT              (_ADC_SINGLECTRL_REF_DEFAULT << 16)      /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_1V25                 (_ADC_SINGLECTRL_REF_1V25 << 16)         /**< Shifted mode 1V25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_2V5                  (_ADC_SINGLECTRL_REF_2V5 << 16)          /**< Shifted mode 2V5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_VDD                  (_ADC_SINGLECTRL_REF_VDD << 16)          /**< Shifted mode VDD for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_5VDIFF               (_ADC_SINGLECTRL_REF_5VDIFF << 16)       /**< Shifted mode 5VDIFF for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_EXTSINGLE            (_ADC_SINGLECTRL_REF_EXTSINGLE << 16)    /**< Shifted mode EXTSINGLE for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_2XEXTDIFF            (_ADC_SINGLECTRL_REF_2XEXTDIFF << 16)    /**< Shifted mode 2XEXTDIFF for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_2XVDD                (_ADC_SINGLECTRL_REF_2XVDD << 16)        /**< Shifted mode 2XVDD for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_SHIFT                20                                       /**< Shift value for ADC_AT */
#define _ADC_SINGLECTRL_AT_MASK                 0xF00000UL                               /**< Bit mask for ADC_AT */
#define _ADC_SINGLECTRL_AT_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_1CYCLE               0x00000000UL                             /**< Mode 1CYCLE for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_2CYCLES              0x00000001UL                             /**< Mode 2CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_4CYCLES              0x00000002UL                             /**< Mode 4CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_8CYCLES              0x00000003UL                             /**< Mode 8CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_16CYCLES             0x00000004UL                             /**< Mode 16CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_32CYCLES             0x00000005UL                             /**< Mode 32CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_64CYCLES             0x00000006UL                             /**< Mode 64CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_128CYCLES            0x00000007UL                             /**< Mode 128CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_256CYCLES            0x00000008UL                             /**< Mode 256CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_DEFAULT               (_ADC_SINGLECTRL_AT_DEFAULT << 20)       /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_1CYCLE                (_ADC_SINGLECTRL_AT_1CYCLE << 20)        /**< Shifted mode 1CYCLE for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_2CYCLES               (_ADC_SINGLECTRL_AT_2CYCLES << 20)       /**< Shifted mode 2CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_4CYCLES               (_ADC_SINGLECTRL_AT_4CYCLES << 20)       /**< Shifted mode 4CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_8CYCLES               (_ADC_SINGLECTRL_AT_8CYCLES << 20)       /**< Shifted mode 8CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_16CYCLES              (_ADC_SINGLECTRL_AT_16CYCLES << 20)      /**< Shifted mode 16CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_32CYCLES              (_ADC_SINGLECTRL_AT_32CYCLES << 20)      /**< Shifted mode 32CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_64CYCLES              (_ADC_SINGLECTRL_AT_64CYCLES << 20)      /**< Shifted mode 64CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_128CYCLES             (_ADC_SINGLECTRL_AT_128CYCLES << 20)     /**< Shifted mode 128CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_256CYCLES             (_ADC_SINGLECTRL_AT_256CYCLES << 20)     /**< Shifted mode 256CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSEN                    (0x1UL << 24)                            /**< Single Sample PRS Trigger Enable */
#define _ADC_SINGLECTRL_PRSEN_SHIFT             24                                       /**< Shift value for ADC_PRSEN */
#define _ADC_SINGLECTRL_PRSEN_MASK              0x1000000UL                              /**< Bit mask for ADC_PRSEN */
#define _ADC_SINGLECTRL_PRSEN_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSEN_DEFAULT            (_ADC_SINGLECTRL_PRSEN_DEFAULT << 24)    /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_SHIFT            28                                       /**< Shift value for ADC_PRSSEL */
#define _ADC_SINGLECTRL_PRSSEL_MASK             0xF0000000UL                             /**< Bit mask for ADC_PRSSEL */
#define _ADC_SINGLECTRL_PRSSEL_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH0           0x00000000UL                             /**< Mode PRSCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH1           0x00000001UL                             /**< Mode PRSCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH2           0x00000002UL                             /**< Mode PRSCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH3           0x00000003UL                             /**< Mode PRSCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH4           0x00000004UL                             /**< Mode PRSCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH5           0x00000005UL                             /**< Mode PRSCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH6           0x00000006UL                             /**< Mode PRSCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH7           0x00000007UL                             /**< Mode PRSCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH8           0x00000008UL                             /**< Mode PRSCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH9           0x00000009UL                             /**< Mode PRSCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH10          0x0000000AUL                             /**< Mode PRSCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_PRSSEL_PRSCH11          0x0000000BUL                             /**< Mode PRSCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_DEFAULT           (_ADC_SINGLECTRL_PRSSEL_DEFAULT << 28)   /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH0            (_ADC_SINGLECTRL_PRSSEL_PRSCH0 << 28)    /**< Shifted mode PRSCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH1            (_ADC_SINGLECTRL_PRSSEL_PRSCH1 << 28)    /**< Shifted mode PRSCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH2            (_ADC_SINGLECTRL_PRSSEL_PRSCH2 << 28)    /**< Shifted mode PRSCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH3            (_ADC_SINGLECTRL_PRSSEL_PRSCH3 << 28)    /**< Shifted mode PRSCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH4            (_ADC_SINGLECTRL_PRSSEL_PRSCH4 << 28)    /**< Shifted mode PRSCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH5            (_ADC_SINGLECTRL_PRSSEL_PRSCH5 << 28)    /**< Shifted mode PRSCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH6            (_ADC_SINGLECTRL_PRSSEL_PRSCH6 << 28)    /**< Shifted mode PRSCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH7            (_ADC_SINGLECTRL_PRSSEL_PRSCH7 << 28)    /**< Shifted mode PRSCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH8            (_ADC_SINGLECTRL_PRSSEL_PRSCH8 << 28)    /**< Shifted mode PRSCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH9            (_ADC_SINGLECTRL_PRSSEL_PRSCH9 << 28)    /**< Shifted mode PRSCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH10           (_ADC_SINGLECTRL_PRSSEL_PRSCH10 << 28)   /**< Shifted mode PRSCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSSEL_PRSCH11           (_ADC_SINGLECTRL_PRSSEL_PRSCH11 << 28)   /**< Shifted mode PRSCH11 for ADC_SINGLECTRL */

/* Bit fields for ADC SCANCTRL */
#define _ADC_SCANCTRL_RESETVALUE                0x00000000UL                           /**< Default value for ADC_SCANCTRL */
#define _ADC_SCANCTRL_MASK                      0xF1F7FF37UL                           /**< Mask for ADC_SCANCTRL */
#define ADC_SCANCTRL_REP                        (0x1UL << 0)                           /**< Scan Sequence Repetitive Mode */
#define _ADC_SCANCTRL_REP_SHIFT                 0                                      /**< Shift value for ADC_REP */
#define _ADC_SCANCTRL_REP_MASK                  0x1UL                                  /**< Bit mask for ADC_REP */
#define _ADC_SCANCTRL_REP_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_REP_DEFAULT                (_ADC_SCANCTRL_REP_DEFAULT << 0)       /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_DIFF                       (0x1UL << 1)                           /**< Scan Sequence Differential Mode */
#define _ADC_SCANCTRL_DIFF_SHIFT                1                                      /**< Shift value for ADC_DIFF */
#define _ADC_SCANCTRL_DIFF_MASK                 0x2UL                                  /**< Bit mask for ADC_DIFF */
#define _ADC_SCANCTRL_DIFF_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_DIFF_DEFAULT               (_ADC_SCANCTRL_DIFF_DEFAULT << 1)      /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_ADJ                        (0x1UL << 2)                           /**< Scan Sequence Result Adjustment */
#define _ADC_SCANCTRL_ADJ_SHIFT                 2                                      /**< Shift value for ADC_ADJ */
#define _ADC_SCANCTRL_ADJ_MASK                  0x4UL                                  /**< Bit mask for ADC_ADJ */
#define _ADC_SCANCTRL_ADJ_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_ADJ_RIGHT                 0x00000000UL                           /**< Mode RIGHT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_ADJ_LEFT                  0x00000001UL                           /**< Mode LEFT for ADC_SCANCTRL */
#define ADC_SCANCTRL_ADJ_DEFAULT                (_ADC_SCANCTRL_ADJ_DEFAULT << 2)       /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_ADJ_RIGHT                  (_ADC_SCANCTRL_ADJ_RIGHT << 2)         /**< Shifted mode RIGHT for ADC_SCANCTRL */
#define ADC_SCANCTRL_ADJ_LEFT                   (_ADC_SCANCTRL_ADJ_LEFT << 2)          /**< Shifted mode LEFT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_SHIFT                 4                                      /**< Shift value for ADC_RES */
#define _ADC_SCANCTRL_RES_MASK                  0x30UL                                 /**< Bit mask for ADC_RES */
#define _ADC_SCANCTRL_RES_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_12BIT                 0x00000000UL                           /**< Mode 12BIT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_8BIT                  0x00000001UL                           /**< Mode 8BIT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_6BIT                  0x00000002UL                           /**< Mode 6BIT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_OVS                   0x00000003UL                           /**< Mode OVS for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_DEFAULT                (_ADC_SCANCTRL_RES_DEFAULT << 4)       /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_12BIT                  (_ADC_SCANCTRL_RES_12BIT << 4)         /**< Shifted mode 12BIT for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_8BIT                   (_ADC_SCANCTRL_RES_8BIT << 4)          /**< Shifted mode 8BIT for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_6BIT                   (_ADC_SCANCTRL_RES_6BIT << 4)          /**< Shifted mode 6BIT for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_OVS                    (_ADC_SCANCTRL_RES_OVS << 4)           /**< Shifted mode OVS for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_SHIFT           8                                      /**< Shift value for ADC_INPUTMASK */
#define _ADC_SCANCTRL_INPUTMASK_MASK            0xFF00UL                               /**< Bit mask for ADC_INPUTMASK */
#define _ADC_SCANCTRL_INPUTMASK_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH0             0x00000001UL                           /**< Mode CH0 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH0CH1          0x00000001UL                           /**< Mode CH0CH1 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH1             0x00000002UL                           /**< Mode CH1 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH2CH3          0x00000002UL                           /**< Mode CH2CH3 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH2             0x00000004UL                           /**< Mode CH2 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH4CH5          0x00000004UL                           /**< Mode CH4CH5 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH6CH7          0x00000008UL                           /**< Mode CH6CH7 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH3             0x00000008UL                           /**< Mode CH3 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH4             0x00000010UL                           /**< Mode CH4 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH5             0x00000020UL                           /**< Mode CH5 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH6             0x00000040UL                           /**< Mode CH6 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_INPUTMASK_CH7             0x00000080UL                           /**< Mode CH7 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_DEFAULT          (_ADC_SCANCTRL_INPUTMASK_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH0              (_ADC_SCANCTRL_INPUTMASK_CH0 << 8)     /**< Shifted mode CH0 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH0CH1           (_ADC_SCANCTRL_INPUTMASK_CH0CH1 << 8)  /**< Shifted mode CH0CH1 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH1              (_ADC_SCANCTRL_INPUTMASK_CH1 << 8)     /**< Shifted mode CH1 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH2CH3           (_ADC_SCANCTRL_INPUTMASK_CH2CH3 << 8)  /**< Shifted mode CH2CH3 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH2              (_ADC_SCANCTRL_INPUTMASK_CH2 << 8)     /**< Shifted mode CH2 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH4CH5           (_ADC_SCANCTRL_INPUTMASK_CH4CH5 << 8)  /**< Shifted mode CH4CH5 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH6CH7           (_ADC_SCANCTRL_INPUTMASK_CH6CH7 << 8)  /**< Shifted mode CH6CH7 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH3              (_ADC_SCANCTRL_INPUTMASK_CH3 << 8)     /**< Shifted mode CH3 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH4              (_ADC_SCANCTRL_INPUTMASK_CH4 << 8)     /**< Shifted mode CH4 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH5              (_ADC_SCANCTRL_INPUTMASK_CH5 << 8)     /**< Shifted mode CH5 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH6              (_ADC_SCANCTRL_INPUTMASK_CH6 << 8)     /**< Shifted mode CH6 for ADC_SCANCTRL */
#define ADC_SCANCTRL_INPUTMASK_CH7              (_ADC_SCANCTRL_INPUTMASK_CH7 << 8)     /**< Shifted mode CH7 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_SHIFT                 16                                     /**< Shift value for ADC_REF */
#define _ADC_SCANCTRL_REF_MASK                  0x70000UL                              /**< Bit mask for ADC_REF */
#define _ADC_SCANCTRL_REF_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_1V25                  0x00000000UL                           /**< Mode 1V25 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_2V5                   0x00000001UL                           /**< Mode 2V5 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_VDD                   0x00000002UL                           /**< Mode VDD for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_5VDIFF                0x00000003UL                           /**< Mode 5VDIFF for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_EXTSINGLE             0x00000004UL                           /**< Mode EXTSINGLE for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_2XEXTDIFF             0x00000005UL                           /**< Mode 2XEXTDIFF for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_2XVDD                 0x00000006UL                           /**< Mode 2XVDD for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_DEFAULT                (_ADC_SCANCTRL_REF_DEFAULT << 16)      /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_1V25                   (_ADC_SCANCTRL_REF_1V25 << 16)         /**< Shifted mode 1V25 for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_2V5                    (_ADC_SCANCTRL_REF_2V5 << 16)          /**< Shifted mode 2V5 for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_VDD                    (_ADC_SCANCTRL_REF_VDD << 16)          /**< Shifted mode VDD for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_5VDIFF                 (_ADC_SCANCTRL_REF_5VDIFF << 16)       /**< Shifted mode 5VDIFF for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_EXTSINGLE              (_ADC_SCANCTRL_REF_EXTSINGLE << 16)    /**< Shifted mode EXTSINGLE for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_2XEXTDIFF              (_ADC_SCANCTRL_REF_2XEXTDIFF << 16)    /**< Shifted mode 2XEXTDIFF for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_2XVDD                  (_ADC_SCANCTRL_REF_2XVDD << 16)        /**< Shifted mode 2XVDD for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_SHIFT                  20                                     /**< Shift value for ADC_AT */
#define _ADC_SCANCTRL_AT_MASK                   0xF00000UL                             /**< Bit mask for ADC_AT */
#define _ADC_SCANCTRL_AT_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_1CYCLE                 0x00000000UL                           /**< Mode 1CYCLE for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_2CYCLES                0x00000001UL                           /**< Mode 2CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_4CYCLES                0x00000002UL                           /**< Mode 4CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_8CYCLES                0x00000003UL                           /**< Mode 8CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_16CYCLES               0x00000004UL                           /**< Mode 16CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_32CYCLES               0x00000005UL                           /**< Mode 32CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_64CYCLES               0x00000006UL                           /**< Mode 64CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_128CYCLES              0x00000007UL                           /**< Mode 128CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_256CYCLES              0x00000008UL                           /**< Mode 256CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_DEFAULT                 (_ADC_SCANCTRL_AT_DEFAULT << 20)       /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_1CYCLE                  (_ADC_SCANCTRL_AT_1CYCLE << 20)        /**< Shifted mode 1CYCLE for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_2CYCLES                 (_ADC_SCANCTRL_AT_2CYCLES << 20)       /**< Shifted mode 2CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_4CYCLES                 (_ADC_SCANCTRL_AT_4CYCLES << 20)       /**< Shifted mode 4CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_8CYCLES                 (_ADC_SCANCTRL_AT_8CYCLES << 20)       /**< Shifted mode 8CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_16CYCLES                (_ADC_SCANCTRL_AT_16CYCLES << 20)      /**< Shifted mode 16CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_32CYCLES                (_ADC_SCANCTRL_AT_32CYCLES << 20)      /**< Shifted mode 32CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_64CYCLES                (_ADC_SCANCTRL_AT_64CYCLES << 20)      /**< Shifted mode 64CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_128CYCLES               (_ADC_SCANCTRL_AT_128CYCLES << 20)     /**< Shifted mode 128CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_256CYCLES               (_ADC_SCANCTRL_AT_256CYCLES << 20)     /**< Shifted mode 256CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSEN                      (0x1UL << 24)                          /**< Scan Sequence PRS Trigger Enable */
#define _ADC_SCANCTRL_PRSEN_SHIFT               24                                     /**< Shift value for ADC_PRSEN */
#define _ADC_SCANCTRL_PRSEN_MASK                0x1000000UL                            /**< Bit mask for ADC_PRSEN */
#define _ADC_SCANCTRL_PRSEN_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSEN_DEFAULT              (_ADC_SCANCTRL_PRSEN_DEFAULT << 24)    /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_SHIFT              28                                     /**< Shift value for ADC_PRSSEL */
#define _ADC_SCANCTRL_PRSSEL_MASK               0xF0000000UL                           /**< Bit mask for ADC_PRSSEL */
#define _ADC_SCANCTRL_PRSSEL_DEFAULT            0x00000000UL                           /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH0             0x00000000UL                           /**< Mode PRSCH0 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH1             0x00000001UL                           /**< Mode PRSCH1 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH2             0x00000002UL                           /**< Mode PRSCH2 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH3             0x00000003UL                           /**< Mode PRSCH3 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH4             0x00000004UL                           /**< Mode PRSCH4 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH5             0x00000005UL                           /**< Mode PRSCH5 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH6             0x00000006UL                           /**< Mode PRSCH6 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH7             0x00000007UL                           /**< Mode PRSCH7 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH8             0x00000008UL                           /**< Mode PRSCH8 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH9             0x00000009UL                           /**< Mode PRSCH9 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH10            0x0000000AUL                           /**< Mode PRSCH10 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_PRSSEL_PRSCH11            0x0000000BUL                           /**< Mode PRSCH11 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_DEFAULT             (_ADC_SCANCTRL_PRSSEL_DEFAULT << 28)   /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH0              (_ADC_SCANCTRL_PRSSEL_PRSCH0 << 28)    /**< Shifted mode PRSCH0 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH1              (_ADC_SCANCTRL_PRSSEL_PRSCH1 << 28)    /**< Shifted mode PRSCH1 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH2              (_ADC_SCANCTRL_PRSSEL_PRSCH2 << 28)    /**< Shifted mode PRSCH2 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH3              (_ADC_SCANCTRL_PRSSEL_PRSCH3 << 28)    /**< Shifted mode PRSCH3 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH4              (_ADC_SCANCTRL_PRSSEL_PRSCH4 << 28)    /**< Shifted mode PRSCH4 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH5              (_ADC_SCANCTRL_PRSSEL_PRSCH5 << 28)    /**< Shifted mode PRSCH5 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH6              (_ADC_SCANCTRL_PRSSEL_PRSCH6 << 28)    /**< Shifted mode PRSCH6 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH7              (_ADC_SCANCTRL_PRSSEL_PRSCH7 << 28)    /**< Shifted mode PRSCH7 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH8              (_ADC_SCANCTRL_PRSSEL_PRSCH8 << 28)    /**< Shifted mode PRSCH8 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH9              (_ADC_SCANCTRL_PRSSEL_PRSCH9 << 28)    /**< Shifted mode PRSCH9 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH10             (_ADC_SCANCTRL_PRSSEL_PRSCH10 << 28)   /**< Shifted mode PRSCH10 for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSSEL_PRSCH11             (_ADC_SCANCTRL_PRSSEL_PRSCH11 << 28)   /**< Shifted mode PRSCH11 for ADC_SCANCTRL */

/* Bit fields for ADC IEN */
#define _ADC_IEN_RESETVALUE                     0x00000000UL                     /**< Default value for ADC_IEN */
#define _ADC_IEN_MASK                           0x00000303UL                     /**< Mask for ADC_IEN */
#define ADC_IEN_SINGLE                          (0x1UL << 0)                     /**< Single Conversion Complete Interrupt Enable */
#define _ADC_IEN_SINGLE_SHIFT                   0                                /**< Shift value for ADC_SINGLE */
#define _ADC_IEN_SINGLE_MASK                    0x1UL                            /**< Bit mask for ADC_SINGLE */
#define _ADC_IEN_SINGLE_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLE_DEFAULT                  (_ADC_IEN_SINGLE_DEFAULT << 0)   /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCAN                            (0x1UL << 1)                     /**< Scan Conversion Complete Interrupt Enable */
#define _ADC_IEN_SCAN_SHIFT                     1                                /**< Shift value for ADC_SCAN */
#define _ADC_IEN_SCAN_MASK                      0x2UL                            /**< Bit mask for ADC_SCAN */
#define _ADC_IEN_SCAN_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCAN_DEFAULT                    (_ADC_IEN_SCAN_DEFAULT << 1)     /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLEOF                        (0x1UL << 8)                     /**< Single Result Overflow Interrupt Enable */
#define _ADC_IEN_SINGLEOF_SHIFT                 8                                /**< Shift value for ADC_SINGLEOF */
#define _ADC_IEN_SINGLEOF_MASK                  0x100UL                          /**< Bit mask for ADC_SINGLEOF */
#define _ADC_IEN_SINGLEOF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLEOF_DEFAULT                (_ADC_IEN_SINGLEOF_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCANOF                          (0x1UL << 9)                     /**< Scan Result Overflow Interrupt Enable */
#define _ADC_IEN_SCANOF_SHIFT                   9                                /**< Shift value for ADC_SCANOF */
#define _ADC_IEN_SCANOF_MASK                    0x200UL                          /**< Bit mask for ADC_SCANOF */
#define _ADC_IEN_SCANOF_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCANOF_DEFAULT                  (_ADC_IEN_SCANOF_DEFAULT << 9)   /**< Shifted mode DEFAULT for ADC_IEN */

/* Bit fields for ADC IF */
#define _ADC_IF_RESETVALUE                      0x00000000UL                    /**< Default value for ADC_IF */
#define _ADC_IF_MASK                            0x00000303UL                    /**< Mask for ADC_IF */
#define ADC_IF_SINGLE                           (0x1UL << 0)                    /**< Single Conversion Complete Interrupt Flag */
#define _ADC_IF_SINGLE_SHIFT                    0                               /**< Shift value for ADC_SINGLE */
#define _ADC_IF_SINGLE_MASK                     0x1UL                           /**< Bit mask for ADC_SINGLE */
#define _ADC_IF_SINGLE_DEFAULT                  0x00000000UL                    /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLE_DEFAULT                   (_ADC_IF_SINGLE_DEFAULT << 0)   /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SCAN                             (0x1UL << 1)                    /**< Scan Conversion Complete Interrupt Flag */
#define _ADC_IF_SCAN_SHIFT                      1                               /**< Shift value for ADC_SCAN */
#define _ADC_IF_SCAN_MASK                       0x2UL                           /**< Bit mask for ADC_SCAN */
#define _ADC_IF_SCAN_DEFAULT                    0x00000000UL                    /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SCAN_DEFAULT                     (_ADC_IF_SCAN_DEFAULT << 1)     /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLEOF                         (0x1UL << 8)                    /**< Single Result Overflow Interrupt Flag */
#define _ADC_IF_SINGLEOF_SHIFT                  8                               /**< Shift value for ADC_SINGLEOF */
#define _ADC_IF_SINGLEOF_MASK                   0x100UL                         /**< Bit mask for ADC_SINGLEOF */
#define _ADC_IF_SINGLEOF_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLEOF_DEFAULT                 (_ADC_IF_SINGLEOF_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SCANOF                           (0x1UL << 9)                    /**< Scan Result Overflow Interrupt Flag */
#define _ADC_IF_SCANOF_SHIFT                    9                               /**< Shift value for ADC_SCANOF */
#define _ADC_IF_SCANOF_MASK                     0x200UL                         /**< Bit mask for ADC_SCANOF */
#define _ADC_IF_SCANOF_DEFAULT                  0x00000000UL                    /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SCANOF_DEFAULT                   (_ADC_IF_SCANOF_DEFAULT << 9)   /**< Shifted mode DEFAULT for ADC_IF */

/* Bit fields for ADC IFS */
#define _ADC_IFS_RESETVALUE                     0x00000000UL                     /**< Default value for ADC_IFS */
#define _ADC_IFS_MASK                           0x00000303UL                     /**< Mask for ADC_IFS */
#define ADC_IFS_SINGLE                          (0x1UL << 0)                     /**< Single Conversion Complete Interrupt Flag Set */
#define _ADC_IFS_SINGLE_SHIFT                   0                                /**< Shift value for ADC_SINGLE */
#define _ADC_IFS_SINGLE_MASK                    0x1UL                            /**< Bit mask for ADC_SINGLE */
#define _ADC_IFS_SINGLE_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SINGLE_DEFAULT                  (_ADC_IFS_SINGLE_DEFAULT << 0)   /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCAN                            (0x1UL << 1)                     /**< Scan Conversion Complete Interrupt Flag Set */
#define _ADC_IFS_SCAN_SHIFT                     1                                /**< Shift value for ADC_SCAN */
#define _ADC_IFS_SCAN_MASK                      0x2UL                            /**< Bit mask for ADC_SCAN */
#define _ADC_IFS_SCAN_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCAN_DEFAULT                    (_ADC_IFS_SCAN_DEFAULT << 1)     /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_SINGLEOF                        (0x1UL << 8)                     /**< Single Result Overflow Interrupt Flag Set */
#define _ADC_IFS_SINGLEOF_SHIFT                 8                                /**< Shift value for ADC_SINGLEOF */
#define _ADC_IFS_SINGLEOF_MASK                  0x100UL                          /**< Bit mask for ADC_SINGLEOF */
#define _ADC_IFS_SINGLEOF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SINGLEOF_DEFAULT                (_ADC_IFS_SINGLEOF_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCANOF                          (0x1UL << 9)                     /**< Scan Result Overflow Interrupt Flag Set */
#define _ADC_IFS_SCANOF_SHIFT                   9                                /**< Shift value for ADC_SCANOF */
#define _ADC_IFS_SCANOF_MASK                    0x200UL                          /**< Bit mask for ADC_SCANOF */
#define _ADC_IFS_SCANOF_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCANOF_DEFAULT                  (_ADC_IFS_SCANOF_DEFAULT << 9)   /**< Shifted mode DEFAULT for ADC_IFS */

/* Bit fields for ADC IFC */
#define _ADC_IFC_RESETVALUE                     0x00000000UL                     /**< Default value for ADC_IFC */
#define _ADC_IFC_MASK                           0x00000303UL                     /**< Mask for ADC_IFC */
#define ADC_IFC_SINGLE                          (0x1UL << 0)                     /**< Single Conversion Complete Interrupt Flag Clear */
#define _ADC_IFC_SINGLE_SHIFT                   0                                /**< Shift value for ADC_SINGLE */
#define _ADC_IFC_SINGLE_MASK                    0x1UL                            /**< Bit mask for ADC_SINGLE */
#define _ADC_IFC_SINGLE_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SINGLE_DEFAULT                  (_ADC_IFC_SINGLE_DEFAULT << 0)   /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCAN                            (0x1UL << 1)                     /**< Scan Conversion Complete Interrupt Flag Clear */
#define _ADC_IFC_SCAN_SHIFT                     1                                /**< Shift value for ADC_SCAN */
#define _ADC_IFC_SCAN_MASK                      0x2UL                            /**< Bit mask for ADC_SCAN */
#define _ADC_IFC_SCAN_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCAN_DEFAULT                    (_ADC_IFC_SCAN_DEFAULT << 1)     /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_SINGLEOF                        (0x1UL << 8)                     /**< Single Result Overflow Interrupt Flag Clear */
#define _ADC_IFC_SINGLEOF_SHIFT                 8                                /**< Shift value for ADC_SINGLEOF */
#define _ADC_IFC_SINGLEOF_MASK                  0x100UL                          /**< Bit mask for ADC_SINGLEOF */
#define _ADC_IFC_SINGLEOF_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SINGLEOF_DEFAULT                (_ADC_IFC_SINGLEOF_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCANOF                          (0x1UL << 9)                     /**< Scan Result Overflow Interrupt Flag Clear */
#define _ADC_IFC_SCANOF_SHIFT                   9                                /**< Shift value for ADC_SCANOF */
#define _ADC_IFC_SCANOF_MASK                    0x200UL                          /**< Bit mask for ADC_SCANOF */
#define _ADC_IFC_SCANOF_DEFAULT                 0x00000000UL                     /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCANOF_DEFAULT                  (_ADC_IFC_SCANOF_DEFAULT << 9)   /**< Shifted mode DEFAULT for ADC_IFC */

/* Bit fields for ADC SINGLEDATA */
#define _ADC_SINGLEDATA_RESETVALUE              0x00000000UL                        /**< Default value for ADC_SINGLEDATA */
#define _ADC_SINGLEDATA_MASK                    0xFFFFFFFFUL                        /**< Mask for ADC_SINGLEDATA */
#define _ADC_SINGLEDATA_DATA_SHIFT              0                                   /**< Shift value for ADC_DATA */
#define _ADC_SINGLEDATA_DATA_MASK               0xFFFFFFFFUL                        /**< Bit mask for ADC_DATA */
#define _ADC_SINGLEDATA_DATA_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for ADC_SINGLEDATA */
#define ADC_SINGLEDATA_DATA_DEFAULT             (_ADC_SINGLEDATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SINGLEDATA */

/* Bit fields for ADC SCANDATA */
#define _ADC_SCANDATA_RESETVALUE                0x00000000UL                      /**< Default value for ADC_SCANDATA */
#define _ADC_SCANDATA_MASK                      0xFFFFFFFFUL                      /**< Mask for ADC_SCANDATA */
#define _ADC_SCANDATA_DATA_SHIFT                0                                 /**< Shift value for ADC_DATA */
#define _ADC_SCANDATA_DATA_MASK                 0xFFFFFFFFUL                      /**< Bit mask for ADC_DATA */
#define _ADC_SCANDATA_DATA_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for ADC_SCANDATA */
#define ADC_SCANDATA_DATA_DEFAULT               (_ADC_SCANDATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SCANDATA */

/* Bit fields for ADC SINGLEDATAP */
#define _ADC_SINGLEDATAP_RESETVALUE             0x00000000UL                          /**< Default value for ADC_SINGLEDATAP */
#define _ADC_SINGLEDATAP_MASK                   0xFFFFFFFFUL                          /**< Mask for ADC_SINGLEDATAP */
#define _ADC_SINGLEDATAP_DATAP_SHIFT            0                                     /**< Shift value for ADC_DATAP */
#define _ADC_SINGLEDATAP_DATAP_MASK             0xFFFFFFFFUL                          /**< Bit mask for ADC_DATAP */
#define _ADC_SINGLEDATAP_DATAP_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for ADC_SINGLEDATAP */
#define ADC_SINGLEDATAP_DATAP_DEFAULT           (_ADC_SINGLEDATAP_DATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SINGLEDATAP */

/* Bit fields for ADC SCANDATAP */
#define _ADC_SCANDATAP_RESETVALUE               0x00000000UL                        /**< Default value for ADC_SCANDATAP */
#define _ADC_SCANDATAP_MASK                     0xFFFFFFFFUL                        /**< Mask for ADC_SCANDATAP */
#define _ADC_SCANDATAP_DATAP_SHIFT              0                                   /**< Shift value for ADC_DATAP */
#define _ADC_SCANDATAP_DATAP_MASK               0xFFFFFFFFUL                        /**< Bit mask for ADC_DATAP */
#define _ADC_SCANDATAP_DATAP_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for ADC_SCANDATAP */
#define ADC_SCANDATAP_DATAP_DEFAULT             (_ADC_SCANDATAP_DATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SCANDATAP */

/* Bit fields for ADC CAL */
#define _ADC_CAL_RESETVALUE                     0x3F003F00UL                         /**< Default value for ADC_CAL */
#define _ADC_CAL_MASK                           0x7F7F7F7FUL                         /**< Mask for ADC_CAL */
#define _ADC_CAL_SINGLEOFFSET_SHIFT             0                                    /**< Shift value for ADC_SINGLEOFFSET */
#define _ADC_CAL_SINGLEOFFSET_MASK              0x7FUL                               /**< Bit mask for ADC_SINGLEOFFSET */
#define _ADC_CAL_SINGLEOFFSET_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SINGLEOFFSET_DEFAULT            (_ADC_CAL_SINGLEOFFSET_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_CAL */
#define _ADC_CAL_SINGLEGAIN_SHIFT               8                                    /**< Shift value for ADC_SINGLEGAIN */
#define _ADC_CAL_SINGLEGAIN_MASK                0x7F00UL                             /**< Bit mask for ADC_SINGLEGAIN */
#define _ADC_CAL_SINGLEGAIN_DEFAULT             0x0000003FUL                         /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SINGLEGAIN_DEFAULT              (_ADC_CAL_SINGLEGAIN_DEFAULT << 8)   /**< Shifted mode DEFAULT for ADC_CAL */
#define _ADC_CAL_SCANOFFSET_SHIFT               16                                   /**< Shift value for ADC_SCANOFFSET */
#define _ADC_CAL_SCANOFFSET_MASK                0x7F0000UL                           /**< Bit mask for ADC_SCANOFFSET */
#define _ADC_CAL_SCANOFFSET_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SCANOFFSET_DEFAULT              (_ADC_CAL_SCANOFFSET_DEFAULT << 16)  /**< Shifted mode DEFAULT for ADC_CAL */
#define _ADC_CAL_SCANGAIN_SHIFT                 24                                   /**< Shift value for ADC_SCANGAIN */
#define _ADC_CAL_SCANGAIN_MASK                  0x7F000000UL                         /**< Bit mask for ADC_SCANGAIN */
#define _ADC_CAL_SCANGAIN_DEFAULT               0x0000003FUL                         /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SCANGAIN_DEFAULT                (_ADC_CAL_SCANGAIN_DEFAULT << 24)    /**< Shifted mode DEFAULT for ADC_CAL */

/* Bit fields for ADC BIASPROG */
#define _ADC_BIASPROG_RESETVALUE                0x00000747UL                          /**< Default value for ADC_BIASPROG */
#define _ADC_BIASPROG_MASK                      0x00000F4FUL                          /**< Mask for ADC_BIASPROG */
#define _ADC_BIASPROG_BIASPROG_SHIFT            0                                     /**< Shift value for ADC_BIASPROG */
#define _ADC_BIASPROG_BIASPROG_MASK             0xFUL                                 /**< Bit mask for ADC_BIASPROG */
#define _ADC_BIASPROG_BIASPROG_DEFAULT          0x00000007UL                          /**< Mode DEFAULT for ADC_BIASPROG */
#define ADC_BIASPROG_BIASPROG_DEFAULT           (_ADC_BIASPROG_BIASPROG_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_BIASPROG */
#define ADC_BIASPROG_HALFBIAS                   (0x1UL << 6)                          /**< Half Bias Current */
#define _ADC_BIASPROG_HALFBIAS_SHIFT            6                                     /**< Shift value for ADC_HALFBIAS */
#define _ADC_BIASPROG_HALFBIAS_MASK             0x40UL                                /**< Bit mask for ADC_HALFBIAS */
#define _ADC_BIASPROG_HALFBIAS_DEFAULT          0x00000001UL                          /**< Mode DEFAULT for ADC_BIASPROG */
#define ADC_BIASPROG_HALFBIAS_DEFAULT           (_ADC_BIASPROG_HALFBIAS_DEFAULT << 6) /**< Shifted mode DEFAULT for ADC_BIASPROG */
#define _ADC_BIASPROG_COMPBIAS_SHIFT            8                                     /**< Shift value for ADC_COMPBIAS */
#define _ADC_BIASPROG_COMPBIAS_MASK             0xF00UL                               /**< Bit mask for ADC_COMPBIAS */
#define _ADC_BIASPROG_COMPBIAS_DEFAULT          0x00000007UL                          /**< Mode DEFAULT for ADC_BIASPROG */
#define ADC_BIASPROG_COMPBIAS_DEFAULT           (_ADC_BIASPROG_COMPBIAS_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_BIASPROG */

/** @} End of group EFM32WG_ADC */
/** @} End of group Parts */

