/**************************************************************************//**
 * @file efm32wg_dac.h
 * @brief EFM32WG_DAC register and bit field definitions
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
 * @defgroup EFM32WG_DAC
 * @{
 * @brief EFM32WG_DAC Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;         /**< Control Register  */
  __IM uint32_t  STATUS;       /**< Status Register  */
  __IOM uint32_t CH0CTRL;      /**< Channel 0 Control Register  */
  __IOM uint32_t CH1CTRL;      /**< Channel 1 Control Register  */
  __IOM uint32_t IEN;          /**< Interrupt Enable Register  */
  __IM uint32_t  IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t CH0DATA;      /**< Channel 0 Data Register  */
  __IOM uint32_t CH1DATA;      /**< Channel 1 Data Register  */
  __IOM uint32_t COMBDATA;     /**< Combined Data Register  */
  __IOM uint32_t CAL;          /**< Calibration Register  */
  __IOM uint32_t BIASPROG;     /**< Bias Programming Register  */
  uint32_t       RESERVED0[8]; /**< Reserved for future use **/
  __IOM uint32_t OPACTRL;      /**< Operational Amplifier Control Register  */
  __IOM uint32_t OPAOFFSET;    /**< Operational Amplifier Offset Register  */
  __IOM uint32_t OPA0MUX;      /**< Operational Amplifier Mux Configuration Register  */
  __IOM uint32_t OPA1MUX;      /**< Operational Amplifier Mux Configuration Register  */
  __IOM uint32_t OPA2MUX;      /**< Operational Amplifier Mux Configuration Register  */
} DAC_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_DAC_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for DAC CTRL */
#define _DAC_CTRL_RESETVALUE                  0x00000010UL                         /**< Default value for DAC_CTRL */
#define _DAC_CTRL_MASK                        0x003703FFUL                         /**< Mask for DAC_CTRL */
#define DAC_CTRL_DIFF                         (0x1UL << 0)                         /**< Differential Mode */
#define _DAC_CTRL_DIFF_SHIFT                  0                                    /**< Shift value for DAC_DIFF */
#define _DAC_CTRL_DIFF_MASK                   0x1UL                                /**< Bit mask for DAC_DIFF */
#define _DAC_CTRL_DIFF_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_DIFF_DEFAULT                 (_DAC_CTRL_DIFF_DEFAULT << 0)        /**< Shifted mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_SINEMODE                     (0x1UL << 1)                         /**< Sine Mode */
#define _DAC_CTRL_SINEMODE_SHIFT              1                                    /**< Shift value for DAC_SINEMODE */
#define _DAC_CTRL_SINEMODE_MASK               0x2UL                                /**< Bit mask for DAC_SINEMODE */
#define _DAC_CTRL_SINEMODE_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_SINEMODE_DEFAULT             (_DAC_CTRL_SINEMODE_DEFAULT << 1)    /**< Shifted mode DEFAULT for DAC_CTRL */
#define _DAC_CTRL_CONVMODE_SHIFT              2                                    /**< Shift value for DAC_CONVMODE */
#define _DAC_CTRL_CONVMODE_MASK               0xCUL                                /**< Bit mask for DAC_CONVMODE */
#define _DAC_CTRL_CONVMODE_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for DAC_CTRL */
#define _DAC_CTRL_CONVMODE_CONTINUOUS         0x00000000UL                         /**< Mode CONTINUOUS for DAC_CTRL */
#define _DAC_CTRL_CONVMODE_SAMPLEHOLD         0x00000001UL                         /**< Mode SAMPLEHOLD for DAC_CTRL */
#define _DAC_CTRL_CONVMODE_SAMPLEOFF          0x00000002UL                         /**< Mode SAMPLEOFF for DAC_CTRL */
#define DAC_CTRL_CONVMODE_DEFAULT             (_DAC_CTRL_CONVMODE_DEFAULT << 2)    /**< Shifted mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_CONVMODE_CONTINUOUS          (_DAC_CTRL_CONVMODE_CONTINUOUS << 2) /**< Shifted mode CONTINUOUS for DAC_CTRL */
#define DAC_CTRL_CONVMODE_SAMPLEHOLD          (_DAC_CTRL_CONVMODE_SAMPLEHOLD << 2) /**< Shifted mode SAMPLEHOLD for DAC_CTRL */
#define DAC_CTRL_CONVMODE_SAMPLEOFF           (_DAC_CTRL_CONVMODE_SAMPLEOFF << 2)  /**< Shifted mode SAMPLEOFF for DAC_CTRL */
#define _DAC_CTRL_OUTMODE_SHIFT               4                                    /**< Shift value for DAC_OUTMODE */
#define _DAC_CTRL_OUTMODE_MASK                0x30UL                               /**< Bit mask for DAC_OUTMODE */
#define _DAC_CTRL_OUTMODE_DISABLE             0x00000000UL                         /**< Mode DISABLE for DAC_CTRL */
#define _DAC_CTRL_OUTMODE_DEFAULT             0x00000001UL                         /**< Mode DEFAULT for DAC_CTRL */
#define _DAC_CTRL_OUTMODE_PIN                 0x00000001UL                         /**< Mode PIN for DAC_CTRL */
#define _DAC_CTRL_OUTMODE_ADC                 0x00000002UL                         /**< Mode ADC for DAC_CTRL */
#define _DAC_CTRL_OUTMODE_PINADC              0x00000003UL                         /**< Mode PINADC for DAC_CTRL */
#define DAC_CTRL_OUTMODE_DISABLE              (_DAC_CTRL_OUTMODE_DISABLE << 4)     /**< Shifted mode DISABLE for DAC_CTRL */
#define DAC_CTRL_OUTMODE_DEFAULT              (_DAC_CTRL_OUTMODE_DEFAULT << 4)     /**< Shifted mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_OUTMODE_PIN                  (_DAC_CTRL_OUTMODE_PIN << 4)         /**< Shifted mode PIN for DAC_CTRL */
#define DAC_CTRL_OUTMODE_ADC                  (_DAC_CTRL_OUTMODE_ADC << 4)         /**< Shifted mode ADC for DAC_CTRL */
#define DAC_CTRL_OUTMODE_PINADC               (_DAC_CTRL_OUTMODE_PINADC << 4)      /**< Shifted mode PINADC for DAC_CTRL */
#define DAC_CTRL_OUTENPRS                     (0x1UL << 6)                         /**< PRS Controlled Output Enable */
#define _DAC_CTRL_OUTENPRS_SHIFT              6                                    /**< Shift value for DAC_OUTENPRS */
#define _DAC_CTRL_OUTENPRS_MASK               0x40UL                               /**< Bit mask for DAC_OUTENPRS */
#define _DAC_CTRL_OUTENPRS_DEFAULT            0x00000000UL                         /**< Mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_OUTENPRS_DEFAULT             (_DAC_CTRL_OUTENPRS_DEFAULT << 6)    /**< Shifted mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_CH0PRESCRST                  (0x1UL << 7)                         /**< Channel 0 Start Reset Prescaler */
#define _DAC_CTRL_CH0PRESCRST_SHIFT           7                                    /**< Shift value for DAC_CH0PRESCRST */
#define _DAC_CTRL_CH0PRESCRST_MASK            0x80UL                               /**< Bit mask for DAC_CH0PRESCRST */
#define _DAC_CTRL_CH0PRESCRST_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_CH0PRESCRST_DEFAULT          (_DAC_CTRL_CH0PRESCRST_DEFAULT << 7) /**< Shifted mode DEFAULT for DAC_CTRL */
#define _DAC_CTRL_REFSEL_SHIFT                8                                    /**< Shift value for DAC_REFSEL */
#define _DAC_CTRL_REFSEL_MASK                 0x300UL                              /**< Bit mask for DAC_REFSEL */
#define _DAC_CTRL_REFSEL_DEFAULT              0x00000000UL                         /**< Mode DEFAULT for DAC_CTRL */
#define _DAC_CTRL_REFSEL_1V25                 0x00000000UL                         /**< Mode 1V25 for DAC_CTRL */
#define _DAC_CTRL_REFSEL_2V5                  0x00000001UL                         /**< Mode 2V5 for DAC_CTRL */
#define _DAC_CTRL_REFSEL_VDD                  0x00000002UL                         /**< Mode VDD for DAC_CTRL */
#define DAC_CTRL_REFSEL_DEFAULT               (_DAC_CTRL_REFSEL_DEFAULT << 8)      /**< Shifted mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_REFSEL_1V25                  (_DAC_CTRL_REFSEL_1V25 << 8)         /**< Shifted mode 1V25 for DAC_CTRL */
#define DAC_CTRL_REFSEL_2V5                   (_DAC_CTRL_REFSEL_2V5 << 8)          /**< Shifted mode 2V5 for DAC_CTRL */
#define DAC_CTRL_REFSEL_VDD                   (_DAC_CTRL_REFSEL_VDD << 8)          /**< Shifted mode VDD for DAC_CTRL */
#define _DAC_CTRL_PRESC_SHIFT                 16                                   /**< Shift value for DAC_PRESC */
#define _DAC_CTRL_PRESC_MASK                  0x70000UL                            /**< Bit mask for DAC_PRESC */
#define _DAC_CTRL_PRESC_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for DAC_CTRL */
#define _DAC_CTRL_PRESC_NODIVISION            0x00000000UL                         /**< Mode NODIVISION for DAC_CTRL */
#define DAC_CTRL_PRESC_DEFAULT                (_DAC_CTRL_PRESC_DEFAULT << 16)      /**< Shifted mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_PRESC_NODIVISION             (_DAC_CTRL_PRESC_NODIVISION << 16)   /**< Shifted mode NODIVISION for DAC_CTRL */
#define _DAC_CTRL_REFRSEL_SHIFT               20                                   /**< Shift value for DAC_REFRSEL */
#define _DAC_CTRL_REFRSEL_MASK                0x300000UL                           /**< Bit mask for DAC_REFRSEL */
#define _DAC_CTRL_REFRSEL_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for DAC_CTRL */
#define _DAC_CTRL_REFRSEL_8CYCLES             0x00000000UL                         /**< Mode 8CYCLES for DAC_CTRL */
#define _DAC_CTRL_REFRSEL_16CYCLES            0x00000001UL                         /**< Mode 16CYCLES for DAC_CTRL */
#define _DAC_CTRL_REFRSEL_32CYCLES            0x00000002UL                         /**< Mode 32CYCLES for DAC_CTRL */
#define _DAC_CTRL_REFRSEL_64CYCLES            0x00000003UL                         /**< Mode 64CYCLES for DAC_CTRL */
#define DAC_CTRL_REFRSEL_DEFAULT              (_DAC_CTRL_REFRSEL_DEFAULT << 20)    /**< Shifted mode DEFAULT for DAC_CTRL */
#define DAC_CTRL_REFRSEL_8CYCLES              (_DAC_CTRL_REFRSEL_8CYCLES << 20)    /**< Shifted mode 8CYCLES for DAC_CTRL */
#define DAC_CTRL_REFRSEL_16CYCLES             (_DAC_CTRL_REFRSEL_16CYCLES << 20)   /**< Shifted mode 16CYCLES for DAC_CTRL */
#define DAC_CTRL_REFRSEL_32CYCLES             (_DAC_CTRL_REFRSEL_32CYCLES << 20)   /**< Shifted mode 32CYCLES for DAC_CTRL */
#define DAC_CTRL_REFRSEL_64CYCLES             (_DAC_CTRL_REFRSEL_64CYCLES << 20)   /**< Shifted mode 64CYCLES for DAC_CTRL */

/* Bit fields for DAC STATUS */
#define _DAC_STATUS_RESETVALUE                0x00000000UL                     /**< Default value for DAC_STATUS */
#define _DAC_STATUS_MASK                      0x00000003UL                     /**< Mask for DAC_STATUS */
#define DAC_STATUS_CH0DV                      (0x1UL << 0)                     /**< Channel 0 Data Valid */
#define _DAC_STATUS_CH0DV_SHIFT               0                                /**< Shift value for DAC_CH0DV */
#define _DAC_STATUS_CH0DV_MASK                0x1UL                            /**< Bit mask for DAC_CH0DV */
#define _DAC_STATUS_CH0DV_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for DAC_STATUS */
#define DAC_STATUS_CH0DV_DEFAULT              (_DAC_STATUS_CH0DV_DEFAULT << 0) /**< Shifted mode DEFAULT for DAC_STATUS */
#define DAC_STATUS_CH1DV                      (0x1UL << 1)                     /**< Channel 1 Data Valid */
#define _DAC_STATUS_CH1DV_SHIFT               1                                /**< Shift value for DAC_CH1DV */
#define _DAC_STATUS_CH1DV_MASK                0x2UL                            /**< Bit mask for DAC_CH1DV */
#define _DAC_STATUS_CH1DV_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for DAC_STATUS */
#define DAC_STATUS_CH1DV_DEFAULT              (_DAC_STATUS_CH1DV_DEFAULT << 1) /**< Shifted mode DEFAULT for DAC_STATUS */

/* Bit fields for DAC CH0CTRL */
#define _DAC_CH0CTRL_RESETVALUE               0x00000000UL                       /**< Default value for DAC_CH0CTRL */
#define _DAC_CH0CTRL_MASK                     0x000000F7UL                       /**< Mask for DAC_CH0CTRL */
#define DAC_CH0CTRL_EN                        (0x1UL << 0)                       /**< Channel 0 Enable */
#define _DAC_CH0CTRL_EN_SHIFT                 0                                  /**< Shift value for DAC_EN */
#define _DAC_CH0CTRL_EN_MASK                  0x1UL                              /**< Bit mask for DAC_EN */
#define _DAC_CH0CTRL_EN_DEFAULT               0x00000000UL                       /**< Mode DEFAULT for DAC_CH0CTRL */
#define DAC_CH0CTRL_EN_DEFAULT                (_DAC_CH0CTRL_EN_DEFAULT << 0)     /**< Shifted mode DEFAULT for DAC_CH0CTRL */
#define DAC_CH0CTRL_REFREN                    (0x1UL << 1)                       /**< Channel 0 Automatic Refresh Enable */
#define _DAC_CH0CTRL_REFREN_SHIFT             1                                  /**< Shift value for DAC_REFREN */
#define _DAC_CH0CTRL_REFREN_MASK              0x2UL                              /**< Bit mask for DAC_REFREN */
#define _DAC_CH0CTRL_REFREN_DEFAULT           0x00000000UL                       /**< Mode DEFAULT for DAC_CH0CTRL */
#define DAC_CH0CTRL_REFREN_DEFAULT            (_DAC_CH0CTRL_REFREN_DEFAULT << 1) /**< Shifted mode DEFAULT for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSEN                     (0x1UL << 2)                       /**< Channel 0 PRS Trigger Enable */
#define _DAC_CH0CTRL_PRSEN_SHIFT              2                                  /**< Shift value for DAC_PRSEN */
#define _DAC_CH0CTRL_PRSEN_MASK               0x4UL                              /**< Bit mask for DAC_PRSEN */
#define _DAC_CH0CTRL_PRSEN_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSEN_DEFAULT             (_DAC_CH0CTRL_PRSEN_DEFAULT << 2)  /**< Shifted mode DEFAULT for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_SHIFT             4                                  /**< Shift value for DAC_PRSSEL */
#define _DAC_CH0CTRL_PRSSEL_MASK              0xF0UL                             /**< Bit mask for DAC_PRSSEL */
#define _DAC_CH0CTRL_PRSSEL_DEFAULT           0x00000000UL                       /**< Mode DEFAULT for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH0            0x00000000UL                       /**< Mode PRSCH0 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH1            0x00000001UL                       /**< Mode PRSCH1 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH2            0x00000002UL                       /**< Mode PRSCH2 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH3            0x00000003UL                       /**< Mode PRSCH3 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH4            0x00000004UL                       /**< Mode PRSCH4 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH5            0x00000005UL                       /**< Mode PRSCH5 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH6            0x00000006UL                       /**< Mode PRSCH6 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH7            0x00000007UL                       /**< Mode PRSCH7 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH8            0x00000008UL                       /**< Mode PRSCH8 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH9            0x00000009UL                       /**< Mode PRSCH9 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH10           0x0000000AUL                       /**< Mode PRSCH10 for DAC_CH0CTRL */
#define _DAC_CH0CTRL_PRSSEL_PRSCH11           0x0000000BUL                       /**< Mode PRSCH11 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_DEFAULT            (_DAC_CH0CTRL_PRSSEL_DEFAULT << 4) /**< Shifted mode DEFAULT for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH0             (_DAC_CH0CTRL_PRSSEL_PRSCH0 << 4)  /**< Shifted mode PRSCH0 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH1             (_DAC_CH0CTRL_PRSSEL_PRSCH1 << 4)  /**< Shifted mode PRSCH1 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH2             (_DAC_CH0CTRL_PRSSEL_PRSCH2 << 4)  /**< Shifted mode PRSCH2 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH3             (_DAC_CH0CTRL_PRSSEL_PRSCH3 << 4)  /**< Shifted mode PRSCH3 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH4             (_DAC_CH0CTRL_PRSSEL_PRSCH4 << 4)  /**< Shifted mode PRSCH4 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH5             (_DAC_CH0CTRL_PRSSEL_PRSCH5 << 4)  /**< Shifted mode PRSCH5 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH6             (_DAC_CH0CTRL_PRSSEL_PRSCH6 << 4)  /**< Shifted mode PRSCH6 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH7             (_DAC_CH0CTRL_PRSSEL_PRSCH7 << 4)  /**< Shifted mode PRSCH7 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH8             (_DAC_CH0CTRL_PRSSEL_PRSCH8 << 4)  /**< Shifted mode PRSCH8 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH9             (_DAC_CH0CTRL_PRSSEL_PRSCH9 << 4)  /**< Shifted mode PRSCH9 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH10            (_DAC_CH0CTRL_PRSSEL_PRSCH10 << 4) /**< Shifted mode PRSCH10 for DAC_CH0CTRL */
#define DAC_CH0CTRL_PRSSEL_PRSCH11            (_DAC_CH0CTRL_PRSSEL_PRSCH11 << 4) /**< Shifted mode PRSCH11 for DAC_CH0CTRL */

/* Bit fields for DAC CH1CTRL */
#define _DAC_CH1CTRL_RESETVALUE               0x00000000UL                       /**< Default value for DAC_CH1CTRL */
#define _DAC_CH1CTRL_MASK                     0x000000F7UL                       /**< Mask for DAC_CH1CTRL */
#define DAC_CH1CTRL_EN                        (0x1UL << 0)                       /**< Channel 1 Enable */
#define _DAC_CH1CTRL_EN_SHIFT                 0                                  /**< Shift value for DAC_EN */
#define _DAC_CH1CTRL_EN_MASK                  0x1UL                              /**< Bit mask for DAC_EN */
#define _DAC_CH1CTRL_EN_DEFAULT               0x00000000UL                       /**< Mode DEFAULT for DAC_CH1CTRL */
#define DAC_CH1CTRL_EN_DEFAULT                (_DAC_CH1CTRL_EN_DEFAULT << 0)     /**< Shifted mode DEFAULT for DAC_CH1CTRL */
#define DAC_CH1CTRL_REFREN                    (0x1UL << 1)                       /**< Channel 1 Automatic Refresh Enable */
#define _DAC_CH1CTRL_REFREN_SHIFT             1                                  /**< Shift value for DAC_REFREN */
#define _DAC_CH1CTRL_REFREN_MASK              0x2UL                              /**< Bit mask for DAC_REFREN */
#define _DAC_CH1CTRL_REFREN_DEFAULT           0x00000000UL                       /**< Mode DEFAULT for DAC_CH1CTRL */
#define DAC_CH1CTRL_REFREN_DEFAULT            (_DAC_CH1CTRL_REFREN_DEFAULT << 1) /**< Shifted mode DEFAULT for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSEN                     (0x1UL << 2)                       /**< Channel 1 PRS Trigger Enable */
#define _DAC_CH1CTRL_PRSEN_SHIFT              2                                  /**< Shift value for DAC_PRSEN */
#define _DAC_CH1CTRL_PRSEN_MASK               0x4UL                              /**< Bit mask for DAC_PRSEN */
#define _DAC_CH1CTRL_PRSEN_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSEN_DEFAULT             (_DAC_CH1CTRL_PRSEN_DEFAULT << 2)  /**< Shifted mode DEFAULT for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_SHIFT             4                                  /**< Shift value for DAC_PRSSEL */
#define _DAC_CH1CTRL_PRSSEL_MASK              0xF0UL                             /**< Bit mask for DAC_PRSSEL */
#define _DAC_CH1CTRL_PRSSEL_DEFAULT           0x00000000UL                       /**< Mode DEFAULT for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH0            0x00000000UL                       /**< Mode PRSCH0 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH1            0x00000001UL                       /**< Mode PRSCH1 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH2            0x00000002UL                       /**< Mode PRSCH2 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH3            0x00000003UL                       /**< Mode PRSCH3 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH4            0x00000004UL                       /**< Mode PRSCH4 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH5            0x00000005UL                       /**< Mode PRSCH5 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH6            0x00000006UL                       /**< Mode PRSCH6 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH7            0x00000007UL                       /**< Mode PRSCH7 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH8            0x00000008UL                       /**< Mode PRSCH8 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH9            0x00000009UL                       /**< Mode PRSCH9 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH10           0x0000000AUL                       /**< Mode PRSCH10 for DAC_CH1CTRL */
#define _DAC_CH1CTRL_PRSSEL_PRSCH11           0x0000000BUL                       /**< Mode PRSCH11 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_DEFAULT            (_DAC_CH1CTRL_PRSSEL_DEFAULT << 4) /**< Shifted mode DEFAULT for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH0             (_DAC_CH1CTRL_PRSSEL_PRSCH0 << 4)  /**< Shifted mode PRSCH0 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH1             (_DAC_CH1CTRL_PRSSEL_PRSCH1 << 4)  /**< Shifted mode PRSCH1 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH2             (_DAC_CH1CTRL_PRSSEL_PRSCH2 << 4)  /**< Shifted mode PRSCH2 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH3             (_DAC_CH1CTRL_PRSSEL_PRSCH3 << 4)  /**< Shifted mode PRSCH3 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH4             (_DAC_CH1CTRL_PRSSEL_PRSCH4 << 4)  /**< Shifted mode PRSCH4 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH5             (_DAC_CH1CTRL_PRSSEL_PRSCH5 << 4)  /**< Shifted mode PRSCH5 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH6             (_DAC_CH1CTRL_PRSSEL_PRSCH6 << 4)  /**< Shifted mode PRSCH6 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH7             (_DAC_CH1CTRL_PRSSEL_PRSCH7 << 4)  /**< Shifted mode PRSCH7 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH8             (_DAC_CH1CTRL_PRSSEL_PRSCH8 << 4)  /**< Shifted mode PRSCH8 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH9             (_DAC_CH1CTRL_PRSSEL_PRSCH9 << 4)  /**< Shifted mode PRSCH9 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH10            (_DAC_CH1CTRL_PRSSEL_PRSCH10 << 4) /**< Shifted mode PRSCH10 for DAC_CH1CTRL */
#define DAC_CH1CTRL_PRSSEL_PRSCH11            (_DAC_CH1CTRL_PRSSEL_PRSCH11 << 4) /**< Shifted mode PRSCH11 for DAC_CH1CTRL */

/* Bit fields for DAC IEN */
#define _DAC_IEN_RESETVALUE                   0x00000000UL                  /**< Default value for DAC_IEN */
#define _DAC_IEN_MASK                         0x00000033UL                  /**< Mask for DAC_IEN */
#define DAC_IEN_CH0                           (0x1UL << 0)                  /**< Channel 0 Conversion Complete Interrupt Enable */
#define _DAC_IEN_CH0_SHIFT                    0                             /**< Shift value for DAC_CH0 */
#define _DAC_IEN_CH0_MASK                     0x1UL                         /**< Bit mask for DAC_CH0 */
#define _DAC_IEN_CH0_DEFAULT                  0x00000000UL                  /**< Mode DEFAULT for DAC_IEN */
#define DAC_IEN_CH0_DEFAULT                   (_DAC_IEN_CH0_DEFAULT << 0)   /**< Shifted mode DEFAULT for DAC_IEN */
#define DAC_IEN_CH1                           (0x1UL << 1)                  /**< Channel 1 Conversion Complete Interrupt Enable */
#define _DAC_IEN_CH1_SHIFT                    1                             /**< Shift value for DAC_CH1 */
#define _DAC_IEN_CH1_MASK                     0x2UL                         /**< Bit mask for DAC_CH1 */
#define _DAC_IEN_CH1_DEFAULT                  0x00000000UL                  /**< Mode DEFAULT for DAC_IEN */
#define DAC_IEN_CH1_DEFAULT                   (_DAC_IEN_CH1_DEFAULT << 1)   /**< Shifted mode DEFAULT for DAC_IEN */
#define DAC_IEN_CH0UF                         (0x1UL << 4)                  /**< Channel 0 Conversion Data Underflow Interrupt Enable */
#define _DAC_IEN_CH0UF_SHIFT                  4                             /**< Shift value for DAC_CH0UF */
#define _DAC_IEN_CH0UF_MASK                   0x10UL                        /**< Bit mask for DAC_CH0UF */
#define _DAC_IEN_CH0UF_DEFAULT                0x00000000UL                  /**< Mode DEFAULT for DAC_IEN */
#define DAC_IEN_CH0UF_DEFAULT                 (_DAC_IEN_CH0UF_DEFAULT << 4) /**< Shifted mode DEFAULT for DAC_IEN */
#define DAC_IEN_CH1UF                         (0x1UL << 5)                  /**< Channel 1 Conversion Data Underflow Interrupt Enable */
#define _DAC_IEN_CH1UF_SHIFT                  5                             /**< Shift value for DAC_CH1UF */
#define _DAC_IEN_CH1UF_MASK                   0x20UL                        /**< Bit mask for DAC_CH1UF */
#define _DAC_IEN_CH1UF_DEFAULT                0x00000000UL                  /**< Mode DEFAULT for DAC_IEN */
#define DAC_IEN_CH1UF_DEFAULT                 (_DAC_IEN_CH1UF_DEFAULT << 5) /**< Shifted mode DEFAULT for DAC_IEN */

/* Bit fields for DAC IF */
#define _DAC_IF_RESETVALUE                    0x00000000UL                 /**< Default value for DAC_IF */
#define _DAC_IF_MASK                          0x00000033UL                 /**< Mask for DAC_IF */
#define DAC_IF_CH0                            (0x1UL << 0)                 /**< Channel 0 Conversion Complete Interrupt Flag */
#define _DAC_IF_CH0_SHIFT                     0                            /**< Shift value for DAC_CH0 */
#define _DAC_IF_CH0_MASK                      0x1UL                        /**< Bit mask for DAC_CH0 */
#define _DAC_IF_CH0_DEFAULT                   0x00000000UL                 /**< Mode DEFAULT for DAC_IF */
#define DAC_IF_CH0_DEFAULT                    (_DAC_IF_CH0_DEFAULT << 0)   /**< Shifted mode DEFAULT for DAC_IF */
#define DAC_IF_CH1                            (0x1UL << 1)                 /**< Channel 1 Conversion Complete Interrupt Flag */
#define _DAC_IF_CH1_SHIFT                     1                            /**< Shift value for DAC_CH1 */
#define _DAC_IF_CH1_MASK                      0x2UL                        /**< Bit mask for DAC_CH1 */
#define _DAC_IF_CH1_DEFAULT                   0x00000000UL                 /**< Mode DEFAULT for DAC_IF */
#define DAC_IF_CH1_DEFAULT                    (_DAC_IF_CH1_DEFAULT << 1)   /**< Shifted mode DEFAULT for DAC_IF */
#define DAC_IF_CH0UF                          (0x1UL << 4)                 /**< Channel 0 Data Underflow Interrupt Flag */
#define _DAC_IF_CH0UF_SHIFT                   4                            /**< Shift value for DAC_CH0UF */
#define _DAC_IF_CH0UF_MASK                    0x10UL                       /**< Bit mask for DAC_CH0UF */
#define _DAC_IF_CH0UF_DEFAULT                 0x00000000UL                 /**< Mode DEFAULT for DAC_IF */
#define DAC_IF_CH0UF_DEFAULT                  (_DAC_IF_CH0UF_DEFAULT << 4) /**< Shifted mode DEFAULT for DAC_IF */
#define DAC_IF_CH1UF                          (0x1UL << 5)                 /**< Channel 1 Data Underflow Interrupt Flag */
#define _DAC_IF_CH1UF_SHIFT                   5                            /**< Shift value for DAC_CH1UF */
#define _DAC_IF_CH1UF_MASK                    0x20UL                       /**< Bit mask for DAC_CH1UF */
#define _DAC_IF_CH1UF_DEFAULT                 0x00000000UL                 /**< Mode DEFAULT for DAC_IF */
#define DAC_IF_CH1UF_DEFAULT                  (_DAC_IF_CH1UF_DEFAULT << 5) /**< Shifted mode DEFAULT for DAC_IF */

/* Bit fields for DAC IFS */
#define _DAC_IFS_RESETVALUE                   0x00000000UL                  /**< Default value for DAC_IFS */
#define _DAC_IFS_MASK                         0x00000033UL                  /**< Mask for DAC_IFS */
#define DAC_IFS_CH0                           (0x1UL << 0)                  /**< Channel 0 Conversion Complete Interrupt Flag Set */
#define _DAC_IFS_CH0_SHIFT                    0                             /**< Shift value for DAC_CH0 */
#define _DAC_IFS_CH0_MASK                     0x1UL                         /**< Bit mask for DAC_CH0 */
#define _DAC_IFS_CH0_DEFAULT                  0x00000000UL                  /**< Mode DEFAULT for DAC_IFS */
#define DAC_IFS_CH0_DEFAULT                   (_DAC_IFS_CH0_DEFAULT << 0)   /**< Shifted mode DEFAULT for DAC_IFS */
#define DAC_IFS_CH1                           (0x1UL << 1)                  /**< Channel 1 Conversion Complete Interrupt Flag Set */
#define _DAC_IFS_CH1_SHIFT                    1                             /**< Shift value for DAC_CH1 */
#define _DAC_IFS_CH1_MASK                     0x2UL                         /**< Bit mask for DAC_CH1 */
#define _DAC_IFS_CH1_DEFAULT                  0x00000000UL                  /**< Mode DEFAULT for DAC_IFS */
#define DAC_IFS_CH1_DEFAULT                   (_DAC_IFS_CH1_DEFAULT << 1)   /**< Shifted mode DEFAULT for DAC_IFS */
#define DAC_IFS_CH0UF                         (0x1UL << 4)                  /**< Channel 0 Data Underflow Interrupt Flag Set */
#define _DAC_IFS_CH0UF_SHIFT                  4                             /**< Shift value for DAC_CH0UF */
#define _DAC_IFS_CH0UF_MASK                   0x10UL                        /**< Bit mask for DAC_CH0UF */
#define _DAC_IFS_CH0UF_DEFAULT                0x00000000UL                  /**< Mode DEFAULT for DAC_IFS */
#define DAC_IFS_CH0UF_DEFAULT                 (_DAC_IFS_CH0UF_DEFAULT << 4) /**< Shifted mode DEFAULT for DAC_IFS */
#define DAC_IFS_CH1UF                         (0x1UL << 5)                  /**< Channel 1 Data Underflow Interrupt Flag Set */
#define _DAC_IFS_CH1UF_SHIFT                  5                             /**< Shift value for DAC_CH1UF */
#define _DAC_IFS_CH1UF_MASK                   0x20UL                        /**< Bit mask for DAC_CH1UF */
#define _DAC_IFS_CH1UF_DEFAULT                0x00000000UL                  /**< Mode DEFAULT for DAC_IFS */
#define DAC_IFS_CH1UF_DEFAULT                 (_DAC_IFS_CH1UF_DEFAULT << 5) /**< Shifted mode DEFAULT for DAC_IFS */

/* Bit fields for DAC IFC */
#define _DAC_IFC_RESETVALUE                   0x00000000UL                  /**< Default value for DAC_IFC */
#define _DAC_IFC_MASK                         0x00000033UL                  /**< Mask for DAC_IFC */
#define DAC_IFC_CH0                           (0x1UL << 0)                  /**< Channel 0 Conversion Complete Interrupt Flag Clear */
#define _DAC_IFC_CH0_SHIFT                    0                             /**< Shift value for DAC_CH0 */
#define _DAC_IFC_CH0_MASK                     0x1UL                         /**< Bit mask for DAC_CH0 */
#define _DAC_IFC_CH0_DEFAULT                  0x00000000UL                  /**< Mode DEFAULT for DAC_IFC */
#define DAC_IFC_CH0_DEFAULT                   (_DAC_IFC_CH0_DEFAULT << 0)   /**< Shifted mode DEFAULT for DAC_IFC */
#define DAC_IFC_CH1                           (0x1UL << 1)                  /**< Channel 1 Conversion Complete Interrupt Flag Clear */
#define _DAC_IFC_CH1_SHIFT                    1                             /**< Shift value for DAC_CH1 */
#define _DAC_IFC_CH1_MASK                     0x2UL                         /**< Bit mask for DAC_CH1 */
#define _DAC_IFC_CH1_DEFAULT                  0x00000000UL                  /**< Mode DEFAULT for DAC_IFC */
#define DAC_IFC_CH1_DEFAULT                   (_DAC_IFC_CH1_DEFAULT << 1)   /**< Shifted mode DEFAULT for DAC_IFC */
#define DAC_IFC_CH0UF                         (0x1UL << 4)                  /**< Channel 0 Data Underflow Interrupt Flag Clear */
#define _DAC_IFC_CH0UF_SHIFT                  4                             /**< Shift value for DAC_CH0UF */
#define _DAC_IFC_CH0UF_MASK                   0x10UL                        /**< Bit mask for DAC_CH0UF */
#define _DAC_IFC_CH0UF_DEFAULT                0x00000000UL                  /**< Mode DEFAULT for DAC_IFC */
#define DAC_IFC_CH0UF_DEFAULT                 (_DAC_IFC_CH0UF_DEFAULT << 4) /**< Shifted mode DEFAULT for DAC_IFC */
#define DAC_IFC_CH1UF                         (0x1UL << 5)                  /**< Channel 1 Data Underflow Interrupt Flag Clear */
#define _DAC_IFC_CH1UF_SHIFT                  5                             /**< Shift value for DAC_CH1UF */
#define _DAC_IFC_CH1UF_MASK                   0x20UL                        /**< Bit mask for DAC_CH1UF */
#define _DAC_IFC_CH1UF_DEFAULT                0x00000000UL                  /**< Mode DEFAULT for DAC_IFC */
#define DAC_IFC_CH1UF_DEFAULT                 (_DAC_IFC_CH1UF_DEFAULT << 5) /**< Shifted mode DEFAULT for DAC_IFC */

/* Bit fields for DAC CH0DATA */
#define _DAC_CH0DATA_RESETVALUE               0x00000000UL                     /**< Default value for DAC_CH0DATA */
#define _DAC_CH0DATA_MASK                     0x00000FFFUL                     /**< Mask for DAC_CH0DATA */
#define _DAC_CH0DATA_DATA_SHIFT               0                                /**< Shift value for DAC_DATA */
#define _DAC_CH0DATA_DATA_MASK                0xFFFUL                          /**< Bit mask for DAC_DATA */
#define _DAC_CH0DATA_DATA_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for DAC_CH0DATA */
#define DAC_CH0DATA_DATA_DEFAULT              (_DAC_CH0DATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for DAC_CH0DATA */

/* Bit fields for DAC CH1DATA */
#define _DAC_CH1DATA_RESETVALUE               0x00000000UL                     /**< Default value for DAC_CH1DATA */
#define _DAC_CH1DATA_MASK                     0x00000FFFUL                     /**< Mask for DAC_CH1DATA */
#define _DAC_CH1DATA_DATA_SHIFT               0                                /**< Shift value for DAC_DATA */
#define _DAC_CH1DATA_DATA_MASK                0xFFFUL                          /**< Bit mask for DAC_DATA */
#define _DAC_CH1DATA_DATA_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for DAC_CH1DATA */
#define DAC_CH1DATA_DATA_DEFAULT              (_DAC_CH1DATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for DAC_CH1DATA */

/* Bit fields for DAC COMBDATA */
#define _DAC_COMBDATA_RESETVALUE              0x00000000UL                          /**< Default value for DAC_COMBDATA */
#define _DAC_COMBDATA_MASK                    0x0FFF0FFFUL                          /**< Mask for DAC_COMBDATA */
#define _DAC_COMBDATA_CH0DATA_SHIFT           0                                     /**< Shift value for DAC_CH0DATA */
#define _DAC_COMBDATA_CH0DATA_MASK            0xFFFUL                               /**< Bit mask for DAC_CH0DATA */
#define _DAC_COMBDATA_CH0DATA_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for DAC_COMBDATA */
#define DAC_COMBDATA_CH0DATA_DEFAULT          (_DAC_COMBDATA_CH0DATA_DEFAULT << 0)  /**< Shifted mode DEFAULT for DAC_COMBDATA */
#define _DAC_COMBDATA_CH1DATA_SHIFT           16                                    /**< Shift value for DAC_CH1DATA */
#define _DAC_COMBDATA_CH1DATA_MASK            0xFFF0000UL                           /**< Bit mask for DAC_CH1DATA */
#define _DAC_COMBDATA_CH1DATA_DEFAULT         0x00000000UL                          /**< Mode DEFAULT for DAC_COMBDATA */
#define DAC_COMBDATA_CH1DATA_DEFAULT          (_DAC_COMBDATA_CH1DATA_DEFAULT << 16) /**< Shifted mode DEFAULT for DAC_COMBDATA */

/* Bit fields for DAC CAL */
#define _DAC_CAL_RESETVALUE                   0x00400000UL                      /**< Default value for DAC_CAL */
#define _DAC_CAL_MASK                         0x007F3F3FUL                      /**< Mask for DAC_CAL */
#define _DAC_CAL_CH0OFFSET_SHIFT              0                                 /**< Shift value for DAC_CH0OFFSET */
#define _DAC_CAL_CH0OFFSET_MASK               0x3FUL                            /**< Bit mask for DAC_CH0OFFSET */
#define _DAC_CAL_CH0OFFSET_DEFAULT            0x00000000UL                      /**< Mode DEFAULT for DAC_CAL */
#define DAC_CAL_CH0OFFSET_DEFAULT             (_DAC_CAL_CH0OFFSET_DEFAULT << 0) /**< Shifted mode DEFAULT for DAC_CAL */
#define _DAC_CAL_CH1OFFSET_SHIFT              8                                 /**< Shift value for DAC_CH1OFFSET */
#define _DAC_CAL_CH1OFFSET_MASK               0x3F00UL                          /**< Bit mask for DAC_CH1OFFSET */
#define _DAC_CAL_CH1OFFSET_DEFAULT            0x00000000UL                      /**< Mode DEFAULT for DAC_CAL */
#define DAC_CAL_CH1OFFSET_DEFAULT             (_DAC_CAL_CH1OFFSET_DEFAULT << 8) /**< Shifted mode DEFAULT for DAC_CAL */
#define _DAC_CAL_GAIN_SHIFT                   16                                /**< Shift value for DAC_GAIN */
#define _DAC_CAL_GAIN_MASK                    0x7F0000UL                        /**< Bit mask for DAC_GAIN */
#define _DAC_CAL_GAIN_DEFAULT                 0x00000040UL                      /**< Mode DEFAULT for DAC_CAL */
#define DAC_CAL_GAIN_DEFAULT                  (_DAC_CAL_GAIN_DEFAULT << 16)     /**< Shifted mode DEFAULT for DAC_CAL */

/* Bit fields for DAC BIASPROG */
#define _DAC_BIASPROG_RESETVALUE              0x00004747UL                               /**< Default value for DAC_BIASPROG */
#define _DAC_BIASPROG_MASK                    0x00004F4FUL                               /**< Mask for DAC_BIASPROG */
#define _DAC_BIASPROG_BIASPROG_SHIFT          0                                          /**< Shift value for DAC_BIASPROG */
#define _DAC_BIASPROG_BIASPROG_MASK           0xFUL                                      /**< Bit mask for DAC_BIASPROG */
#define _DAC_BIASPROG_BIASPROG_DEFAULT        0x00000007UL                               /**< Mode DEFAULT for DAC_BIASPROG */
#define DAC_BIASPROG_BIASPROG_DEFAULT         (_DAC_BIASPROG_BIASPROG_DEFAULT << 0)      /**< Shifted mode DEFAULT for DAC_BIASPROG */
#define DAC_BIASPROG_HALFBIAS                 (0x1UL << 6)                               /**< Half Bias Current */
#define _DAC_BIASPROG_HALFBIAS_SHIFT          6                                          /**< Shift value for DAC_HALFBIAS */
#define _DAC_BIASPROG_HALFBIAS_MASK           0x40UL                                     /**< Bit mask for DAC_HALFBIAS */
#define _DAC_BIASPROG_HALFBIAS_DEFAULT        0x00000001UL                               /**< Mode DEFAULT for DAC_BIASPROG */
#define DAC_BIASPROG_HALFBIAS_DEFAULT         (_DAC_BIASPROG_HALFBIAS_DEFAULT << 6)      /**< Shifted mode DEFAULT for DAC_BIASPROG */
#define _DAC_BIASPROG_OPA2BIASPROG_SHIFT      8                                          /**< Shift value for DAC_OPA2BIASPROG */
#define _DAC_BIASPROG_OPA2BIASPROG_MASK       0xF00UL                                    /**< Bit mask for DAC_OPA2BIASPROG */
#define _DAC_BIASPROG_OPA2BIASPROG_DEFAULT    0x00000007UL                               /**< Mode DEFAULT for DAC_BIASPROG */
#define DAC_BIASPROG_OPA2BIASPROG_DEFAULT     (_DAC_BIASPROG_OPA2BIASPROG_DEFAULT << 8)  /**< Shifted mode DEFAULT for DAC_BIASPROG */
#define DAC_BIASPROG_OPA2HALFBIAS             (0x1UL << 14)                              /**< Half Bias Current */
#define _DAC_BIASPROG_OPA2HALFBIAS_SHIFT      14                                         /**< Shift value for DAC_OPA2HALFBIAS */
#define _DAC_BIASPROG_OPA2HALFBIAS_MASK       0x4000UL                                   /**< Bit mask for DAC_OPA2HALFBIAS */
#define _DAC_BIASPROG_OPA2HALFBIAS_DEFAULT    0x00000001UL                               /**< Mode DEFAULT for DAC_BIASPROG */
#define DAC_BIASPROG_OPA2HALFBIAS_DEFAULT     (_DAC_BIASPROG_OPA2HALFBIAS_DEFAULT << 14) /**< Shifted mode DEFAULT for DAC_BIASPROG */

/* Bit fields for DAC OPACTRL */
#define _DAC_OPACTRL_RESETVALUE               0x00000000UL                            /**< Default value for DAC_OPACTRL */
#define _DAC_OPACTRL_MASK                     0x01C3F1C7UL                            /**< Mask for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0EN                    (0x1UL << 0)                            /**< OPA0 Enable */
#define _DAC_OPACTRL_OPA0EN_SHIFT             0                                       /**< Shift value for DAC_OPA0EN */
#define _DAC_OPACTRL_OPA0EN_MASK              0x1UL                                   /**< Bit mask for DAC_OPA0EN */
#define _DAC_OPACTRL_OPA0EN_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0EN_DEFAULT            (_DAC_OPACTRL_OPA0EN_DEFAULT << 0)      /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1EN                    (0x1UL << 1)                            /**< OPA1 Enable */
#define _DAC_OPACTRL_OPA1EN_SHIFT             1                                       /**< Shift value for DAC_OPA1EN */
#define _DAC_OPACTRL_OPA1EN_MASK              0x2UL                                   /**< Bit mask for DAC_OPA1EN */
#define _DAC_OPACTRL_OPA1EN_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1EN_DEFAULT            (_DAC_OPACTRL_OPA1EN_DEFAULT << 1)      /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2EN                    (0x1UL << 2)                            /**< OPA2 Enable */
#define _DAC_OPACTRL_OPA2EN_SHIFT             2                                       /**< Shift value for DAC_OPA2EN */
#define _DAC_OPACTRL_OPA2EN_MASK              0x4UL                                   /**< Bit mask for DAC_OPA2EN */
#define _DAC_OPACTRL_OPA2EN_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2EN_DEFAULT            (_DAC_OPACTRL_OPA2EN_DEFAULT << 2)      /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0HCMDIS                (0x1UL << 6)                            /**< High Common Mode Disable. */
#define _DAC_OPACTRL_OPA0HCMDIS_SHIFT         6                                       /**< Shift value for DAC_OPA0HCMDIS */
#define _DAC_OPACTRL_OPA0HCMDIS_MASK          0x40UL                                  /**< Bit mask for DAC_OPA0HCMDIS */
#define _DAC_OPACTRL_OPA0HCMDIS_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0HCMDIS_DEFAULT        (_DAC_OPACTRL_OPA0HCMDIS_DEFAULT << 6)  /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1HCMDIS                (0x1UL << 7)                            /**< High Common Mode Disable. */
#define _DAC_OPACTRL_OPA1HCMDIS_SHIFT         7                                       /**< Shift value for DAC_OPA1HCMDIS */
#define _DAC_OPACTRL_OPA1HCMDIS_MASK          0x80UL                                  /**< Bit mask for DAC_OPA1HCMDIS */
#define _DAC_OPACTRL_OPA1HCMDIS_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1HCMDIS_DEFAULT        (_DAC_OPACTRL_OPA1HCMDIS_DEFAULT << 7)  /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2HCMDIS                (0x1UL << 8)                            /**< High Common Mode Disable. */
#define _DAC_OPACTRL_OPA2HCMDIS_SHIFT         8                                       /**< Shift value for DAC_OPA2HCMDIS */
#define _DAC_OPACTRL_OPA2HCMDIS_MASK          0x100UL                                 /**< Bit mask for DAC_OPA2HCMDIS */
#define _DAC_OPACTRL_OPA2HCMDIS_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2HCMDIS_DEFAULT        (_DAC_OPACTRL_OPA2HCMDIS_DEFAULT << 8)  /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA0LPFDIS_SHIFT         12                                      /**< Shift value for DAC_OPA0LPFDIS */
#define _DAC_OPACTRL_OPA0LPFDIS_MASK          0x3000UL                                /**< Bit mask for DAC_OPA0LPFDIS */
#define _DAC_OPACTRL_OPA0LPFDIS_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA0LPFDIS_PLPFDIS       0x00000001UL                            /**< Mode PLPFDIS for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA0LPFDIS_NLPFDIS       0x00000002UL                            /**< Mode NLPFDIS for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0LPFDIS_DEFAULT        (_DAC_OPACTRL_OPA0LPFDIS_DEFAULT << 12) /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0LPFDIS_PLPFDIS        (_DAC_OPACTRL_OPA0LPFDIS_PLPFDIS << 12) /**< Shifted mode PLPFDIS for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0LPFDIS_NLPFDIS        (_DAC_OPACTRL_OPA0LPFDIS_NLPFDIS << 12) /**< Shifted mode NLPFDIS for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA1LPFDIS_SHIFT         14                                      /**< Shift value for DAC_OPA1LPFDIS */
#define _DAC_OPACTRL_OPA1LPFDIS_MASK          0xC000UL                                /**< Bit mask for DAC_OPA1LPFDIS */
#define _DAC_OPACTRL_OPA1LPFDIS_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA1LPFDIS_PLPFDIS       0x00000001UL                            /**< Mode PLPFDIS for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA1LPFDIS_NLPFDIS       0x00000002UL                            /**< Mode NLPFDIS for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1LPFDIS_DEFAULT        (_DAC_OPACTRL_OPA1LPFDIS_DEFAULT << 14) /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1LPFDIS_PLPFDIS        (_DAC_OPACTRL_OPA1LPFDIS_PLPFDIS << 14) /**< Shifted mode PLPFDIS for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1LPFDIS_NLPFDIS        (_DAC_OPACTRL_OPA1LPFDIS_NLPFDIS << 14) /**< Shifted mode NLPFDIS for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA2LPFDIS_SHIFT         16                                      /**< Shift value for DAC_OPA2LPFDIS */
#define _DAC_OPACTRL_OPA2LPFDIS_MASK          0x30000UL                               /**< Bit mask for DAC_OPA2LPFDIS */
#define _DAC_OPACTRL_OPA2LPFDIS_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA2LPFDIS_PLPFDIS       0x00000001UL                            /**< Mode PLPFDIS for DAC_OPACTRL */
#define _DAC_OPACTRL_OPA2LPFDIS_NLPFDIS       0x00000002UL                            /**< Mode NLPFDIS for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2LPFDIS_DEFAULT        (_DAC_OPACTRL_OPA2LPFDIS_DEFAULT << 16) /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2LPFDIS_PLPFDIS        (_DAC_OPACTRL_OPA2LPFDIS_PLPFDIS << 16) /**< Shifted mode PLPFDIS for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2LPFDIS_NLPFDIS        (_DAC_OPACTRL_OPA2LPFDIS_NLPFDIS << 16) /**< Shifted mode NLPFDIS for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0SHORT                 (0x1UL << 22)                           /**< Short the non-inverting and inverting input. */
#define _DAC_OPACTRL_OPA0SHORT_SHIFT          22                                      /**< Shift value for DAC_OPA0SHORT */
#define _DAC_OPACTRL_OPA0SHORT_MASK           0x400000UL                              /**< Bit mask for DAC_OPA0SHORT */
#define _DAC_OPACTRL_OPA0SHORT_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA0SHORT_DEFAULT         (_DAC_OPACTRL_OPA0SHORT_DEFAULT << 22)  /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1SHORT                 (0x1UL << 23)                           /**< Short the non-inverting and inverting input. */
#define _DAC_OPACTRL_OPA1SHORT_SHIFT          23                                      /**< Shift value for DAC_OPA1SHORT */
#define _DAC_OPACTRL_OPA1SHORT_MASK           0x800000UL                              /**< Bit mask for DAC_OPA1SHORT */
#define _DAC_OPACTRL_OPA1SHORT_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA1SHORT_DEFAULT         (_DAC_OPACTRL_OPA1SHORT_DEFAULT << 23)  /**< Shifted mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2SHORT                 (0x1UL << 24)                           /**< Short the non-inverting and inverting input. */
#define _DAC_OPACTRL_OPA2SHORT_SHIFT          24                                      /**< Shift value for DAC_OPA2SHORT */
#define _DAC_OPACTRL_OPA2SHORT_MASK           0x1000000UL                             /**< Bit mask for DAC_OPA2SHORT */
#define _DAC_OPACTRL_OPA2SHORT_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for DAC_OPACTRL */
#define DAC_OPACTRL_OPA2SHORT_DEFAULT         (_DAC_OPACTRL_OPA2SHORT_DEFAULT << 24)  /**< Shifted mode DEFAULT for DAC_OPACTRL */

/* Bit fields for DAC OPAOFFSET */
#define _DAC_OPAOFFSET_RESETVALUE             0x00000020UL                             /**< Default value for DAC_OPAOFFSET */
#define _DAC_OPAOFFSET_MASK                   0x0000003FUL                             /**< Mask for DAC_OPAOFFSET */
#define _DAC_OPAOFFSET_OPA2OFFSET_SHIFT       0                                        /**< Shift value for DAC_OPA2OFFSET */
#define _DAC_OPAOFFSET_OPA2OFFSET_MASK        0x3FUL                                   /**< Bit mask for DAC_OPA2OFFSET */
#define _DAC_OPAOFFSET_OPA2OFFSET_DEFAULT     0x00000020UL                             /**< Mode DEFAULT for DAC_OPAOFFSET */
#define DAC_OPAOFFSET_OPA2OFFSET_DEFAULT      (_DAC_OPAOFFSET_OPA2OFFSET_DEFAULT << 0) /**< Shifted mode DEFAULT for DAC_OPAOFFSET */

/* Bit fields for DAC OPA0MUX */
#define _DAC_OPA0MUX_RESETVALUE               0x00400000UL                         /**< Default value for DAC_OPA0MUX */
#define _DAC_OPA0MUX_MASK                     0x74C7F737UL                         /**< Mask for DAC_OPA0MUX */
#define _DAC_OPA0MUX_POSSEL_SHIFT             0                                    /**< Shift value for DAC_POSSEL */
#define _DAC_OPA0MUX_POSSEL_MASK              0x7UL                                /**< Bit mask for DAC_POSSEL */
#define _DAC_OPA0MUX_POSSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_POSSEL_DISABLE           0x00000000UL                         /**< Mode DISABLE for DAC_OPA0MUX */
#define _DAC_OPA0MUX_POSSEL_DAC               0x00000001UL                         /**< Mode DAC for DAC_OPA0MUX */
#define _DAC_OPA0MUX_POSSEL_POSPAD            0x00000002UL                         /**< Mode POSPAD for DAC_OPA0MUX */
#define _DAC_OPA0MUX_POSSEL_OPA0INP           0x00000003UL                         /**< Mode OPA0INP for DAC_OPA0MUX */
#define _DAC_OPA0MUX_POSSEL_OPATAP            0x00000004UL                         /**< Mode OPATAP for DAC_OPA0MUX */
#define DAC_OPA0MUX_POSSEL_DEFAULT            (_DAC_OPA0MUX_POSSEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_POSSEL_DISABLE            (_DAC_OPA0MUX_POSSEL_DISABLE << 0)   /**< Shifted mode DISABLE for DAC_OPA0MUX */
#define DAC_OPA0MUX_POSSEL_DAC                (_DAC_OPA0MUX_POSSEL_DAC << 0)       /**< Shifted mode DAC for DAC_OPA0MUX */
#define DAC_OPA0MUX_POSSEL_POSPAD             (_DAC_OPA0MUX_POSSEL_POSPAD << 0)    /**< Shifted mode POSPAD for DAC_OPA0MUX */
#define DAC_OPA0MUX_POSSEL_OPA0INP            (_DAC_OPA0MUX_POSSEL_OPA0INP << 0)   /**< Shifted mode OPA0INP for DAC_OPA0MUX */
#define DAC_OPA0MUX_POSSEL_OPATAP             (_DAC_OPA0MUX_POSSEL_OPATAP << 0)    /**< Shifted mode OPATAP for DAC_OPA0MUX */
#define _DAC_OPA0MUX_NEGSEL_SHIFT             4                                    /**< Shift value for DAC_NEGSEL */
#define _DAC_OPA0MUX_NEGSEL_MASK              0x30UL                               /**< Bit mask for DAC_NEGSEL */
#define _DAC_OPA0MUX_NEGSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_NEGSEL_DISABLE           0x00000000UL                         /**< Mode DISABLE for DAC_OPA0MUX */
#define _DAC_OPA0MUX_NEGSEL_UG                0x00000001UL                         /**< Mode UG for DAC_OPA0MUX */
#define _DAC_OPA0MUX_NEGSEL_OPATAP            0x00000002UL                         /**< Mode OPATAP for DAC_OPA0MUX */
#define _DAC_OPA0MUX_NEGSEL_NEGPAD            0x00000003UL                         /**< Mode NEGPAD for DAC_OPA0MUX */
#define DAC_OPA0MUX_NEGSEL_DEFAULT            (_DAC_OPA0MUX_NEGSEL_DEFAULT << 4)   /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_NEGSEL_DISABLE            (_DAC_OPA0MUX_NEGSEL_DISABLE << 4)   /**< Shifted mode DISABLE for DAC_OPA0MUX */
#define DAC_OPA0MUX_NEGSEL_UG                 (_DAC_OPA0MUX_NEGSEL_UG << 4)        /**< Shifted mode UG for DAC_OPA0MUX */
#define DAC_OPA0MUX_NEGSEL_OPATAP             (_DAC_OPA0MUX_NEGSEL_OPATAP << 4)    /**< Shifted mode OPATAP for DAC_OPA0MUX */
#define DAC_OPA0MUX_NEGSEL_NEGPAD             (_DAC_OPA0MUX_NEGSEL_NEGPAD << 4)    /**< Shifted mode NEGPAD for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESINMUX_SHIFT           8                                    /**< Shift value for DAC_RESINMUX */
#define _DAC_OPA0MUX_RESINMUX_MASK            0x700UL                              /**< Bit mask for DAC_RESINMUX */
#define _DAC_OPA0MUX_RESINMUX_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESINMUX_DISABLE         0x00000000UL                         /**< Mode DISABLE for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESINMUX_OPA0INP         0x00000001UL                         /**< Mode OPA0INP for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESINMUX_NEGPAD          0x00000002UL                         /**< Mode NEGPAD for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESINMUX_POSPAD          0x00000003UL                         /**< Mode POSPAD for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESINMUX_VSS             0x00000004UL                         /**< Mode VSS for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESINMUX_DEFAULT          (_DAC_OPA0MUX_RESINMUX_DEFAULT << 8) /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESINMUX_DISABLE          (_DAC_OPA0MUX_RESINMUX_DISABLE << 8) /**< Shifted mode DISABLE for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESINMUX_OPA0INP          (_DAC_OPA0MUX_RESINMUX_OPA0INP << 8) /**< Shifted mode OPA0INP for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESINMUX_NEGPAD           (_DAC_OPA0MUX_RESINMUX_NEGPAD << 8)  /**< Shifted mode NEGPAD for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESINMUX_POSPAD           (_DAC_OPA0MUX_RESINMUX_POSPAD << 8)  /**< Shifted mode POSPAD for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESINMUX_VSS              (_DAC_OPA0MUX_RESINMUX_VSS << 8)     /**< Shifted mode VSS for DAC_OPA0MUX */
#define DAC_OPA0MUX_PPEN                      (0x1UL << 12)                        /**< OPA0 Positive Pad Input Enable */
#define _DAC_OPA0MUX_PPEN_SHIFT               12                                   /**< Shift value for DAC_PPEN */
#define _DAC_OPA0MUX_PPEN_MASK                0x1000UL                             /**< Bit mask for DAC_PPEN */
#define _DAC_OPA0MUX_PPEN_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_PPEN_DEFAULT              (_DAC_OPA0MUX_PPEN_DEFAULT << 12)    /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_NPEN                      (0x1UL << 13)                        /**< OPA0 Negative Pad Input Enable */
#define _DAC_OPA0MUX_NPEN_SHIFT               13                                   /**< Shift value for DAC_NPEN */
#define _DAC_OPA0MUX_NPEN_MASK                0x2000UL                             /**< Bit mask for DAC_NPEN */
#define _DAC_OPA0MUX_NPEN_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_NPEN_DEFAULT              (_DAC_OPA0MUX_NPEN_DEFAULT << 13)    /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTPEN_SHIFT             14                                   /**< Shift value for DAC_OUTPEN */
#define _DAC_OPA0MUX_OUTPEN_MASK              0x7C000UL                            /**< Bit mask for DAC_OUTPEN */
#define _DAC_OPA0MUX_OUTPEN_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTPEN_OUT0              0x00000001UL                         /**< Mode OUT0 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTPEN_OUT1              0x00000002UL                         /**< Mode OUT1 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTPEN_OUT2              0x00000004UL                         /**< Mode OUT2 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTPEN_OUT3              0x00000008UL                         /**< Mode OUT3 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTPEN_OUT4              0x00000010UL                         /**< Mode OUT4 for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTPEN_DEFAULT            (_DAC_OPA0MUX_OUTPEN_DEFAULT << 14)  /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTPEN_OUT0               (_DAC_OPA0MUX_OUTPEN_OUT0 << 14)     /**< Shifted mode OUT0 for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTPEN_OUT1               (_DAC_OPA0MUX_OUTPEN_OUT1 << 14)     /**< Shifted mode OUT1 for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTPEN_OUT2               (_DAC_OPA0MUX_OUTPEN_OUT2 << 14)     /**< Shifted mode OUT2 for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTPEN_OUT3               (_DAC_OPA0MUX_OUTPEN_OUT3 << 14)     /**< Shifted mode OUT3 for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTPEN_OUT4               (_DAC_OPA0MUX_OUTPEN_OUT4 << 14)     /**< Shifted mode OUT4 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTMODE_SHIFT            22                                   /**< Shift value for DAC_OUTMODE */
#define _DAC_OPA0MUX_OUTMODE_MASK             0xC00000UL                           /**< Bit mask for DAC_OUTMODE */
#define _DAC_OPA0MUX_OUTMODE_DISABLE          0x00000000UL                         /**< Mode DISABLE for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTMODE_DEFAULT          0x00000001UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTMODE_MAIN             0x00000001UL                         /**< Mode MAIN for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTMODE_ALT              0x00000002UL                         /**< Mode ALT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_OUTMODE_ALL              0x00000003UL                         /**< Mode ALL for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTMODE_DISABLE           (_DAC_OPA0MUX_OUTMODE_DISABLE << 22) /**< Shifted mode DISABLE for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTMODE_DEFAULT           (_DAC_OPA0MUX_OUTMODE_DEFAULT << 22) /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTMODE_MAIN              (_DAC_OPA0MUX_OUTMODE_MAIN << 22)    /**< Shifted mode MAIN for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTMODE_ALT               (_DAC_OPA0MUX_OUTMODE_ALT << 22)     /**< Shifted mode ALT for DAC_OPA0MUX */
#define DAC_OPA0MUX_OUTMODE_ALL               (_DAC_OPA0MUX_OUTMODE_ALL << 22)     /**< Shifted mode ALL for DAC_OPA0MUX */
#define DAC_OPA0MUX_NEXTOUT                   (0x1UL << 26)                        /**< OPA0 Next Enable */
#define _DAC_OPA0MUX_NEXTOUT_SHIFT            26                                   /**< Shift value for DAC_NEXTOUT */
#define _DAC_OPA0MUX_NEXTOUT_MASK             0x4000000UL                          /**< Bit mask for DAC_NEXTOUT */
#define _DAC_OPA0MUX_NEXTOUT_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_NEXTOUT_DEFAULT           (_DAC_OPA0MUX_NEXTOUT_DEFAULT << 26) /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_SHIFT             28                                   /**< Shift value for DAC_RESSEL */
#define _DAC_OPA0MUX_RESSEL_MASK              0x70000000UL                         /**< Bit mask for DAC_RESSEL */
#define _DAC_OPA0MUX_RESSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_RES0              0x00000000UL                         /**< Mode RES0 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_RES1              0x00000001UL                         /**< Mode RES1 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_RES2              0x00000002UL                         /**< Mode RES2 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_RES3              0x00000003UL                         /**< Mode RES3 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_RES4              0x00000004UL                         /**< Mode RES4 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_RES5              0x00000005UL                         /**< Mode RES5 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_RES6              0x00000006UL                         /**< Mode RES6 for DAC_OPA0MUX */
#define _DAC_OPA0MUX_RESSEL_RES7              0x00000007UL                         /**< Mode RES7 for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_DEFAULT            (_DAC_OPA0MUX_RESSEL_DEFAULT << 28)  /**< Shifted mode DEFAULT for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_RES0               (_DAC_OPA0MUX_RESSEL_RES0 << 28)     /**< Shifted mode RES0 for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_RES1               (_DAC_OPA0MUX_RESSEL_RES1 << 28)     /**< Shifted mode RES1 for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_RES2               (_DAC_OPA0MUX_RESSEL_RES2 << 28)     /**< Shifted mode RES2 for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_RES3               (_DAC_OPA0MUX_RESSEL_RES3 << 28)     /**< Shifted mode RES3 for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_RES4               (_DAC_OPA0MUX_RESSEL_RES4 << 28)     /**< Shifted mode RES4 for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_RES5               (_DAC_OPA0MUX_RESSEL_RES5 << 28)     /**< Shifted mode RES5 for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_RES6               (_DAC_OPA0MUX_RESSEL_RES6 << 28)     /**< Shifted mode RES6 for DAC_OPA0MUX */
#define DAC_OPA0MUX_RESSEL_RES7               (_DAC_OPA0MUX_RESSEL_RES7 << 28)     /**< Shifted mode RES7 for DAC_OPA0MUX */

/* Bit fields for DAC OPA1MUX */
#define _DAC_OPA1MUX_RESETVALUE               0x00000000UL                         /**< Default value for DAC_OPA1MUX */
#define _DAC_OPA1MUX_MASK                     0x74C7F737UL                         /**< Mask for DAC_OPA1MUX */
#define _DAC_OPA1MUX_POSSEL_SHIFT             0                                    /**< Shift value for DAC_POSSEL */
#define _DAC_OPA1MUX_POSSEL_MASK              0x7UL                                /**< Bit mask for DAC_POSSEL */
#define _DAC_OPA1MUX_POSSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_POSSEL_DISABLE           0x00000000UL                         /**< Mode DISABLE for DAC_OPA1MUX */
#define _DAC_OPA1MUX_POSSEL_DAC               0x00000001UL                         /**< Mode DAC for DAC_OPA1MUX */
#define _DAC_OPA1MUX_POSSEL_POSPAD            0x00000002UL                         /**< Mode POSPAD for DAC_OPA1MUX */
#define _DAC_OPA1MUX_POSSEL_OPA0INP           0x00000003UL                         /**< Mode OPA0INP for DAC_OPA1MUX */
#define _DAC_OPA1MUX_POSSEL_OPATAP            0x00000004UL                         /**< Mode OPATAP for DAC_OPA1MUX */
#define DAC_OPA1MUX_POSSEL_DEFAULT            (_DAC_OPA1MUX_POSSEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_POSSEL_DISABLE            (_DAC_OPA1MUX_POSSEL_DISABLE << 0)   /**< Shifted mode DISABLE for DAC_OPA1MUX */
#define DAC_OPA1MUX_POSSEL_DAC                (_DAC_OPA1MUX_POSSEL_DAC << 0)       /**< Shifted mode DAC for DAC_OPA1MUX */
#define DAC_OPA1MUX_POSSEL_POSPAD             (_DAC_OPA1MUX_POSSEL_POSPAD << 0)    /**< Shifted mode POSPAD for DAC_OPA1MUX */
#define DAC_OPA1MUX_POSSEL_OPA0INP            (_DAC_OPA1MUX_POSSEL_OPA0INP << 0)   /**< Shifted mode OPA0INP for DAC_OPA1MUX */
#define DAC_OPA1MUX_POSSEL_OPATAP             (_DAC_OPA1MUX_POSSEL_OPATAP << 0)    /**< Shifted mode OPATAP for DAC_OPA1MUX */
#define _DAC_OPA1MUX_NEGSEL_SHIFT             4                                    /**< Shift value for DAC_NEGSEL */
#define _DAC_OPA1MUX_NEGSEL_MASK              0x30UL                               /**< Bit mask for DAC_NEGSEL */
#define _DAC_OPA1MUX_NEGSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_NEGSEL_DISABLE           0x00000000UL                         /**< Mode DISABLE for DAC_OPA1MUX */
#define _DAC_OPA1MUX_NEGSEL_UG                0x00000001UL                         /**< Mode UG for DAC_OPA1MUX */
#define _DAC_OPA1MUX_NEGSEL_OPATAP            0x00000002UL                         /**< Mode OPATAP for DAC_OPA1MUX */
#define _DAC_OPA1MUX_NEGSEL_NEGPAD            0x00000003UL                         /**< Mode NEGPAD for DAC_OPA1MUX */
#define DAC_OPA1MUX_NEGSEL_DEFAULT            (_DAC_OPA1MUX_NEGSEL_DEFAULT << 4)   /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_NEGSEL_DISABLE            (_DAC_OPA1MUX_NEGSEL_DISABLE << 4)   /**< Shifted mode DISABLE for DAC_OPA1MUX */
#define DAC_OPA1MUX_NEGSEL_UG                 (_DAC_OPA1MUX_NEGSEL_UG << 4)        /**< Shifted mode UG for DAC_OPA1MUX */
#define DAC_OPA1MUX_NEGSEL_OPATAP             (_DAC_OPA1MUX_NEGSEL_OPATAP << 4)    /**< Shifted mode OPATAP for DAC_OPA1MUX */
#define DAC_OPA1MUX_NEGSEL_NEGPAD             (_DAC_OPA1MUX_NEGSEL_NEGPAD << 4)    /**< Shifted mode NEGPAD for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESINMUX_SHIFT           8                                    /**< Shift value for DAC_RESINMUX */
#define _DAC_OPA1MUX_RESINMUX_MASK            0x700UL                              /**< Bit mask for DAC_RESINMUX */
#define _DAC_OPA1MUX_RESINMUX_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESINMUX_DISABLE         0x00000000UL                         /**< Mode DISABLE for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESINMUX_OPA0INP         0x00000001UL                         /**< Mode OPA0INP for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESINMUX_NEGPAD          0x00000002UL                         /**< Mode NEGPAD for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESINMUX_POSPAD          0x00000003UL                         /**< Mode POSPAD for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESINMUX_VSS             0x00000004UL                         /**< Mode VSS for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESINMUX_DEFAULT          (_DAC_OPA1MUX_RESINMUX_DEFAULT << 8) /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESINMUX_DISABLE          (_DAC_OPA1MUX_RESINMUX_DISABLE << 8) /**< Shifted mode DISABLE for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESINMUX_OPA0INP          (_DAC_OPA1MUX_RESINMUX_OPA0INP << 8) /**< Shifted mode OPA0INP for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESINMUX_NEGPAD           (_DAC_OPA1MUX_RESINMUX_NEGPAD << 8)  /**< Shifted mode NEGPAD for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESINMUX_POSPAD           (_DAC_OPA1MUX_RESINMUX_POSPAD << 8)  /**< Shifted mode POSPAD for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESINMUX_VSS              (_DAC_OPA1MUX_RESINMUX_VSS << 8)     /**< Shifted mode VSS for DAC_OPA1MUX */
#define DAC_OPA1MUX_PPEN                      (0x1UL << 12)                        /**< OPA1 Positive Pad Input Enable */
#define _DAC_OPA1MUX_PPEN_SHIFT               12                                   /**< Shift value for DAC_PPEN */
#define _DAC_OPA1MUX_PPEN_MASK                0x1000UL                             /**< Bit mask for DAC_PPEN */
#define _DAC_OPA1MUX_PPEN_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_PPEN_DEFAULT              (_DAC_OPA1MUX_PPEN_DEFAULT << 12)    /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_NPEN                      (0x1UL << 13)                        /**< OPA1 Negative Pad Input Enable */
#define _DAC_OPA1MUX_NPEN_SHIFT               13                                   /**< Shift value for DAC_NPEN */
#define _DAC_OPA1MUX_NPEN_MASK                0x2000UL                             /**< Bit mask for DAC_NPEN */
#define _DAC_OPA1MUX_NPEN_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_NPEN_DEFAULT              (_DAC_OPA1MUX_NPEN_DEFAULT << 13)    /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTPEN_SHIFT             14                                   /**< Shift value for DAC_OUTPEN */
#define _DAC_OPA1MUX_OUTPEN_MASK              0x7C000UL                            /**< Bit mask for DAC_OUTPEN */
#define _DAC_OPA1MUX_OUTPEN_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTPEN_OUT0              0x00000001UL                         /**< Mode OUT0 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTPEN_OUT1              0x00000002UL                         /**< Mode OUT1 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTPEN_OUT2              0x00000004UL                         /**< Mode OUT2 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTPEN_OUT3              0x00000008UL                         /**< Mode OUT3 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTPEN_OUT4              0x00000010UL                         /**< Mode OUT4 for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTPEN_DEFAULT            (_DAC_OPA1MUX_OUTPEN_DEFAULT << 14)  /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTPEN_OUT0               (_DAC_OPA1MUX_OUTPEN_OUT0 << 14)     /**< Shifted mode OUT0 for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTPEN_OUT1               (_DAC_OPA1MUX_OUTPEN_OUT1 << 14)     /**< Shifted mode OUT1 for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTPEN_OUT2               (_DAC_OPA1MUX_OUTPEN_OUT2 << 14)     /**< Shifted mode OUT2 for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTPEN_OUT3               (_DAC_OPA1MUX_OUTPEN_OUT3 << 14)     /**< Shifted mode OUT3 for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTPEN_OUT4               (_DAC_OPA1MUX_OUTPEN_OUT4 << 14)     /**< Shifted mode OUT4 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTMODE_SHIFT            22                                   /**< Shift value for DAC_OUTMODE */
#define _DAC_OPA1MUX_OUTMODE_MASK             0xC00000UL                           /**< Bit mask for DAC_OUTMODE */
#define _DAC_OPA1MUX_OUTMODE_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTMODE_DISABLE          0x00000000UL                         /**< Mode DISABLE for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTMODE_MAIN             0x00000001UL                         /**< Mode MAIN for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTMODE_ALT              0x00000002UL                         /**< Mode ALT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_OUTMODE_ALL              0x00000003UL                         /**< Mode ALL for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTMODE_DEFAULT           (_DAC_OPA1MUX_OUTMODE_DEFAULT << 22) /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTMODE_DISABLE           (_DAC_OPA1MUX_OUTMODE_DISABLE << 22) /**< Shifted mode DISABLE for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTMODE_MAIN              (_DAC_OPA1MUX_OUTMODE_MAIN << 22)    /**< Shifted mode MAIN for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTMODE_ALT               (_DAC_OPA1MUX_OUTMODE_ALT << 22)     /**< Shifted mode ALT for DAC_OPA1MUX */
#define DAC_OPA1MUX_OUTMODE_ALL               (_DAC_OPA1MUX_OUTMODE_ALL << 22)     /**< Shifted mode ALL for DAC_OPA1MUX */
#define DAC_OPA1MUX_NEXTOUT                   (0x1UL << 26)                        /**< OPA1 Next Enable */
#define _DAC_OPA1MUX_NEXTOUT_SHIFT            26                                   /**< Shift value for DAC_NEXTOUT */
#define _DAC_OPA1MUX_NEXTOUT_MASK             0x4000000UL                          /**< Bit mask for DAC_NEXTOUT */
#define _DAC_OPA1MUX_NEXTOUT_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_NEXTOUT_DEFAULT           (_DAC_OPA1MUX_NEXTOUT_DEFAULT << 26) /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_SHIFT             28                                   /**< Shift value for DAC_RESSEL */
#define _DAC_OPA1MUX_RESSEL_MASK              0x70000000UL                         /**< Bit mask for DAC_RESSEL */
#define _DAC_OPA1MUX_RESSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_RES0              0x00000000UL                         /**< Mode RES0 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_RES1              0x00000001UL                         /**< Mode RES1 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_RES2              0x00000002UL                         /**< Mode RES2 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_RES3              0x00000003UL                         /**< Mode RES3 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_RES4              0x00000004UL                         /**< Mode RES4 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_RES5              0x00000005UL                         /**< Mode RES5 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_RES6              0x00000006UL                         /**< Mode RES6 for DAC_OPA1MUX */
#define _DAC_OPA1MUX_RESSEL_RES7              0x00000007UL                         /**< Mode RES7 for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_DEFAULT            (_DAC_OPA1MUX_RESSEL_DEFAULT << 28)  /**< Shifted mode DEFAULT for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_RES0               (_DAC_OPA1MUX_RESSEL_RES0 << 28)     /**< Shifted mode RES0 for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_RES1               (_DAC_OPA1MUX_RESSEL_RES1 << 28)     /**< Shifted mode RES1 for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_RES2               (_DAC_OPA1MUX_RESSEL_RES2 << 28)     /**< Shifted mode RES2 for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_RES3               (_DAC_OPA1MUX_RESSEL_RES3 << 28)     /**< Shifted mode RES3 for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_RES4               (_DAC_OPA1MUX_RESSEL_RES4 << 28)     /**< Shifted mode RES4 for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_RES5               (_DAC_OPA1MUX_RESSEL_RES5 << 28)     /**< Shifted mode RES5 for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_RES6               (_DAC_OPA1MUX_RESSEL_RES6 << 28)     /**< Shifted mode RES6 for DAC_OPA1MUX */
#define DAC_OPA1MUX_RESSEL_RES7               (_DAC_OPA1MUX_RESSEL_RES7 << 28)     /**< Shifted mode RES7 for DAC_OPA1MUX */

/* Bit fields for DAC OPA2MUX */
#define _DAC_OPA2MUX_RESETVALUE               0x00000000UL                         /**< Default value for DAC_OPA2MUX */
#define _DAC_OPA2MUX_MASK                     0x7440F737UL                         /**< Mask for DAC_OPA2MUX */
#define _DAC_OPA2MUX_POSSEL_SHIFT             0                                    /**< Shift value for DAC_POSSEL */
#define _DAC_OPA2MUX_POSSEL_MASK              0x7UL                                /**< Bit mask for DAC_POSSEL */
#define _DAC_OPA2MUX_POSSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define _DAC_OPA2MUX_POSSEL_DISABLE           0x00000000UL                         /**< Mode DISABLE for DAC_OPA2MUX */
#define _DAC_OPA2MUX_POSSEL_POSPAD            0x00000002UL                         /**< Mode POSPAD for DAC_OPA2MUX */
#define _DAC_OPA2MUX_POSSEL_OPA1INP           0x00000003UL                         /**< Mode OPA1INP for DAC_OPA2MUX */
#define _DAC_OPA2MUX_POSSEL_OPATAP            0x00000004UL                         /**< Mode OPATAP for DAC_OPA2MUX */
#define DAC_OPA2MUX_POSSEL_DEFAULT            (_DAC_OPA2MUX_POSSEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_POSSEL_DISABLE            (_DAC_OPA2MUX_POSSEL_DISABLE << 0)   /**< Shifted mode DISABLE for DAC_OPA2MUX */
#define DAC_OPA2MUX_POSSEL_POSPAD             (_DAC_OPA2MUX_POSSEL_POSPAD << 0)    /**< Shifted mode POSPAD for DAC_OPA2MUX */
#define DAC_OPA2MUX_POSSEL_OPA1INP            (_DAC_OPA2MUX_POSSEL_OPA1INP << 0)   /**< Shifted mode OPA1INP for DAC_OPA2MUX */
#define DAC_OPA2MUX_POSSEL_OPATAP             (_DAC_OPA2MUX_POSSEL_OPATAP << 0)    /**< Shifted mode OPATAP for DAC_OPA2MUX */
#define _DAC_OPA2MUX_NEGSEL_SHIFT             4                                    /**< Shift value for DAC_NEGSEL */
#define _DAC_OPA2MUX_NEGSEL_MASK              0x30UL                               /**< Bit mask for DAC_NEGSEL */
#define _DAC_OPA2MUX_NEGSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define _DAC_OPA2MUX_NEGSEL_DISABLE           0x00000000UL                         /**< Mode DISABLE for DAC_OPA2MUX */
#define _DAC_OPA2MUX_NEGSEL_UG                0x00000001UL                         /**< Mode UG for DAC_OPA2MUX */
#define _DAC_OPA2MUX_NEGSEL_OPATAP            0x00000002UL                         /**< Mode OPATAP for DAC_OPA2MUX */
#define _DAC_OPA2MUX_NEGSEL_NEGPAD            0x00000003UL                         /**< Mode NEGPAD for DAC_OPA2MUX */
#define DAC_OPA2MUX_NEGSEL_DEFAULT            (_DAC_OPA2MUX_NEGSEL_DEFAULT << 4)   /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_NEGSEL_DISABLE            (_DAC_OPA2MUX_NEGSEL_DISABLE << 4)   /**< Shifted mode DISABLE for DAC_OPA2MUX */
#define DAC_OPA2MUX_NEGSEL_UG                 (_DAC_OPA2MUX_NEGSEL_UG << 4)        /**< Shifted mode UG for DAC_OPA2MUX */
#define DAC_OPA2MUX_NEGSEL_OPATAP             (_DAC_OPA2MUX_NEGSEL_OPATAP << 4)    /**< Shifted mode OPATAP for DAC_OPA2MUX */
#define DAC_OPA2MUX_NEGSEL_NEGPAD             (_DAC_OPA2MUX_NEGSEL_NEGPAD << 4)    /**< Shifted mode NEGPAD for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESINMUX_SHIFT           8                                    /**< Shift value for DAC_RESINMUX */
#define _DAC_OPA2MUX_RESINMUX_MASK            0x700UL                              /**< Bit mask for DAC_RESINMUX */
#define _DAC_OPA2MUX_RESINMUX_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESINMUX_DISABLE         0x00000000UL                         /**< Mode DISABLE for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESINMUX_OPA1INP         0x00000001UL                         /**< Mode OPA1INP for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESINMUX_NEGPAD          0x00000002UL                         /**< Mode NEGPAD for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESINMUX_POSPAD          0x00000003UL                         /**< Mode POSPAD for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESINMUX_VSS             0x00000004UL                         /**< Mode VSS for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESINMUX_DEFAULT          (_DAC_OPA2MUX_RESINMUX_DEFAULT << 8) /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESINMUX_DISABLE          (_DAC_OPA2MUX_RESINMUX_DISABLE << 8) /**< Shifted mode DISABLE for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESINMUX_OPA1INP          (_DAC_OPA2MUX_RESINMUX_OPA1INP << 8) /**< Shifted mode OPA1INP for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESINMUX_NEGPAD           (_DAC_OPA2MUX_RESINMUX_NEGPAD << 8)  /**< Shifted mode NEGPAD for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESINMUX_POSPAD           (_DAC_OPA2MUX_RESINMUX_POSPAD << 8)  /**< Shifted mode POSPAD for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESINMUX_VSS              (_DAC_OPA2MUX_RESINMUX_VSS << 8)     /**< Shifted mode VSS for DAC_OPA2MUX */
#define DAC_OPA2MUX_PPEN                      (0x1UL << 12)                        /**< OPA2 Positive Pad Input Enable */
#define _DAC_OPA2MUX_PPEN_SHIFT               12                                   /**< Shift value for DAC_PPEN */
#define _DAC_OPA2MUX_PPEN_MASK                0x1000UL                             /**< Bit mask for DAC_PPEN */
#define _DAC_OPA2MUX_PPEN_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_PPEN_DEFAULT              (_DAC_OPA2MUX_PPEN_DEFAULT << 12)    /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_NPEN                      (0x1UL << 13)                        /**< OPA2 Negative Pad Input Enable */
#define _DAC_OPA2MUX_NPEN_SHIFT               13                                   /**< Shift value for DAC_NPEN */
#define _DAC_OPA2MUX_NPEN_MASK                0x2000UL                             /**< Bit mask for DAC_NPEN */
#define _DAC_OPA2MUX_NPEN_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_NPEN_DEFAULT              (_DAC_OPA2MUX_NPEN_DEFAULT << 13)    /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define _DAC_OPA2MUX_OUTPEN_SHIFT             14                                   /**< Shift value for DAC_OUTPEN */
#define _DAC_OPA2MUX_OUTPEN_MASK              0xC000UL                             /**< Bit mask for DAC_OUTPEN */
#define _DAC_OPA2MUX_OUTPEN_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define _DAC_OPA2MUX_OUTPEN_OUT0              0x00000001UL                         /**< Mode OUT0 for DAC_OPA2MUX */
#define _DAC_OPA2MUX_OUTPEN_OUT1              0x00000002UL                         /**< Mode OUT1 for DAC_OPA2MUX */
#define DAC_OPA2MUX_OUTPEN_DEFAULT            (_DAC_OPA2MUX_OUTPEN_DEFAULT << 14)  /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_OUTPEN_OUT0               (_DAC_OPA2MUX_OUTPEN_OUT0 << 14)     /**< Shifted mode OUT0 for DAC_OPA2MUX */
#define DAC_OPA2MUX_OUTPEN_OUT1               (_DAC_OPA2MUX_OUTPEN_OUT1 << 14)     /**< Shifted mode OUT1 for DAC_OPA2MUX */
#define DAC_OPA2MUX_OUTMODE                   (0x1UL << 22)                        /**< Output Select */
#define _DAC_OPA2MUX_OUTMODE_SHIFT            22                                   /**< Shift value for DAC_OUTMODE */
#define _DAC_OPA2MUX_OUTMODE_MASK             0x400000UL                           /**< Bit mask for DAC_OUTMODE */
#define _DAC_OPA2MUX_OUTMODE_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_OUTMODE_DEFAULT           (_DAC_OPA2MUX_OUTMODE_DEFAULT << 22) /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_NEXTOUT                   (0x1UL << 26)                        /**< OPA2 Next Enable */
#define _DAC_OPA2MUX_NEXTOUT_SHIFT            26                                   /**< Shift value for DAC_NEXTOUT */
#define _DAC_OPA2MUX_NEXTOUT_MASK             0x4000000UL                          /**< Bit mask for DAC_NEXTOUT */
#define _DAC_OPA2MUX_NEXTOUT_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_NEXTOUT_DEFAULT           (_DAC_OPA2MUX_NEXTOUT_DEFAULT << 26) /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_SHIFT             28                                   /**< Shift value for DAC_RESSEL */
#define _DAC_OPA2MUX_RESSEL_MASK              0x70000000UL                         /**< Bit mask for DAC_RESSEL */
#define _DAC_OPA2MUX_RESSEL_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_RES0              0x00000000UL                         /**< Mode RES0 for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_RES1              0x00000001UL                         /**< Mode RES1 for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_RES2              0x00000002UL                         /**< Mode RES2 for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_RES3              0x00000003UL                         /**< Mode RES3 for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_RES4              0x00000004UL                         /**< Mode RES4 for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_RES5              0x00000005UL                         /**< Mode RES5 for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_RES6              0x00000006UL                         /**< Mode RES6 for DAC_OPA2MUX */
#define _DAC_OPA2MUX_RESSEL_RES7              0x00000007UL                         /**< Mode RES7 for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_DEFAULT            (_DAC_OPA2MUX_RESSEL_DEFAULT << 28)  /**< Shifted mode DEFAULT for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_RES0               (_DAC_OPA2MUX_RESSEL_RES0 << 28)     /**< Shifted mode RES0 for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_RES1               (_DAC_OPA2MUX_RESSEL_RES1 << 28)     /**< Shifted mode RES1 for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_RES2               (_DAC_OPA2MUX_RESSEL_RES2 << 28)     /**< Shifted mode RES2 for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_RES3               (_DAC_OPA2MUX_RESSEL_RES3 << 28)     /**< Shifted mode RES3 for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_RES4               (_DAC_OPA2MUX_RESSEL_RES4 << 28)     /**< Shifted mode RES4 for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_RES5               (_DAC_OPA2MUX_RESSEL_RES5 << 28)     /**< Shifted mode RES5 for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_RES6               (_DAC_OPA2MUX_RESSEL_RES6 << 28)     /**< Shifted mode RES6 for DAC_OPA2MUX */
#define DAC_OPA2MUX_RESSEL_RES7               (_DAC_OPA2MUX_RESSEL_RES7 << 28)     /**< Shifted mode RES7 for DAC_OPA2MUX */

/** @} End of group EFM32WG_DAC */
/** @} End of group Parts */

