/**************************************************************************//**
 * @file efr32fg1p_cmu.h
 * @brief EFR32FG1P_CMU register and bit field definitions
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
 * @defgroup EFR32FG1P_CMU
 * @{
 * @brief EFR32FG1P_CMU Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;                /**< CMU Control Register  */

  uint32_t       RESERVED0[3];        /**< Reserved for future use **/
  __IOM uint32_t HFRCOCTRL;           /**< HFRCO Control Register  */

  uint32_t       RESERVED1[1];        /**< Reserved for future use **/
  __IOM uint32_t AUXHFRCOCTRL;        /**< AUXHFRCO Control Register  */

  uint32_t       RESERVED2[1];        /**< Reserved for future use **/
  __IOM uint32_t LFRCOCTRL;           /**< LFRCO Control Register  */
  __IOM uint32_t HFXOCTRL;            /**< HFXO Control Register  */
  __IOM uint32_t HFXOCTRL1;           /**< HFXO Control 1  */
  __IOM uint32_t HFXOSTARTUPCTRL;     /**< HFXO Startup Control  */
  __IOM uint32_t HFXOSTEADYSTATECTRL; /**< HFXO Steady State control  */
  __IOM uint32_t HFXOTIMEOUTCTRL;     /**< HFXO Timeout Control  */
  __IOM uint32_t LFXOCTRL;            /**< LFXO Control Register  */
  __IOM uint32_t ULFRCOCTRL;          /**< ULFRCO Control Register  */

  uint32_t       RESERVED3[4];        /**< Reserved for future use **/
  __IOM uint32_t CALCTRL;             /**< Calibration Control Register  */
  __IOM uint32_t CALCNT;              /**< Calibration Counter Register  */
  uint32_t       RESERVED4[2];        /**< Reserved for future use **/
  __IOM uint32_t OSCENCMD;            /**< Oscillator Enable/Disable Command Register  */
  __IOM uint32_t CMD;                 /**< Command Register  */
  uint32_t       RESERVED5[2];        /**< Reserved for future use **/
  __IOM uint32_t DBGCLKSEL;           /**< Debug Trace Clock Select  */
  __IOM uint32_t HFCLKSEL;            /**< High Frequency Clock Select Command Register  */
  uint32_t       RESERVED6[2];        /**< Reserved for future use **/
  __IOM uint32_t LFACLKSEL;           /**< Low Frequency A Clock Select Register  */
  __IOM uint32_t LFBCLKSEL;           /**< Low Frequency B Clock Select Register  */
  __IOM uint32_t LFECLKSEL;           /**< Low Frequency E Clock Select Register  */

  uint32_t       RESERVED7[1];        /**< Reserved for future use **/
  __IM uint32_t  STATUS;              /**< Status Register  */
  __IM uint32_t  HFCLKSTATUS;         /**< HFCLK Status Register  */
  uint32_t       RESERVED8[1];        /**< Reserved for future use **/
  __IM uint32_t  HFXOTRIMSTATUS;      /**< HFXO Trim Status  */
  __IM uint32_t  IF;                  /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;                 /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;                 /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;                 /**< Interrupt Enable Register  */
  __IOM uint32_t HFBUSCLKEN0;         /**< High Frequency Bus Clock Enable Register 0  */

  uint32_t       RESERVED9[3];        /**< Reserved for future use **/
  __IOM uint32_t HFPERCLKEN0;         /**< High Frequency Peripheral Clock Enable Register 0  */

  uint32_t       RESERVED10[7];       /**< Reserved for future use **/
  __IOM uint32_t LFACLKEN0;           /**< Low Frequency A Clock Enable Register 0  (Async Reg)  */
  uint32_t       RESERVED11[1];       /**< Reserved for future use **/
  __IOM uint32_t LFBCLKEN0;           /**< Low Frequency B Clock Enable Register 0 (Async Reg)  */

  uint32_t       RESERVED12[1];       /**< Reserved for future use **/
  __IOM uint32_t LFECLKEN0;           /**< Low Frequency E Clock Enable Register 0 (Async Reg)  */
  uint32_t       RESERVED13[3];       /**< Reserved for future use **/
  __IOM uint32_t HFPRESC;             /**< High Frequency Clock Prescaler Register  */

  uint32_t       RESERVED14[1];       /**< Reserved for future use **/
  __IOM uint32_t HFCOREPRESC;         /**< High Frequency Core Clock Prescaler Register  */
  __IOM uint32_t HFPERPRESC;          /**< High Frequency Peripheral Clock Prescaler Register  */

  uint32_t       RESERVED15[1];       /**< Reserved for future use **/
  __IOM uint32_t HFEXPPRESC;          /**< High Frequency Export Clock Prescaler Register  */

  uint32_t       RESERVED16[2];       /**< Reserved for future use **/
  __IOM uint32_t LFAPRESC0;           /**< Low Frequency A Prescaler Register 0 (Async Reg)  */
  uint32_t       RESERVED17[1];       /**< Reserved for future use **/
  __IOM uint32_t LFBPRESC0;           /**< Low Frequency B Prescaler Register 0  (Async Reg)  */
  uint32_t       RESERVED18[1];       /**< Reserved for future use **/
  __IOM uint32_t LFEPRESC0;           /**< Low Frequency E Prescaler Register 0  (Async Reg).  When waking up from EM4 make sure EM4UNLATCH in EMU_CMD is set for this to take effect  */

  uint32_t       RESERVED19[3];       /**< Reserved for future use **/
  __IM uint32_t  SYNCBUSY;            /**< Synchronization Busy Register  */
  __IOM uint32_t FREEZE;              /**< Freeze Register  */
  uint32_t       RESERVED20[2];       /**< Reserved for future use **/
  __IOM uint32_t PCNTCTRL;            /**< PCNT Control Register  */

  uint32_t       RESERVED21[2];       /**< Reserved for future use **/
  __IOM uint32_t ADCCTRL;             /**< ADC Control Register  */

  uint32_t       RESERVED22[4];       /**< Reserved for future use **/
  __IOM uint32_t ROUTEPEN;            /**< I/O Routing Pin Enable Register  */
  __IOM uint32_t ROUTELOC0;           /**< I/O Routing Location Register  */

  uint32_t       RESERVED23[2];       /**< Reserved for future use **/
  __IOM uint32_t LOCK;                /**< Configuration Lock Register  */
} CMU_TypeDef;                        /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_CMU_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for CMU CTRL */
#define _CMU_CTRL_RESETVALUE                              0x00300000UL                          /**< Default value for CMU_CTRL */
#define _CMU_CTRL_MASK                                    0x001101EFUL                          /**< Mask for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_SHIFT                        0                                     /**< Shift value for CMU_CLKOUTSEL0 */
#define _CMU_CTRL_CLKOUTSEL0_MASK                         0xFUL                                 /**< Bit mask for CMU_CLKOUTSEL0 */
#define _CMU_CTRL_CLKOUTSEL0_DEFAULT                      0x00000000UL                          /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_DISABLED                     0x00000000UL                          /**< Mode DISABLED for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_ULFRCO                       0x00000001UL                          /**< Mode ULFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_LFRCO                        0x00000002UL                          /**< Mode LFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_LFXO                         0x00000003UL                          /**< Mode LFXO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFXO                         0x00000006UL                          /**< Mode HFXO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFEXPCLK                     0x00000007UL                          /**< Mode HFEXPCLK for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_ULFRCOQ                      0x00000009UL                          /**< Mode ULFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_LFRCOQ                       0x0000000AUL                          /**< Mode LFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_LFXOQ                        0x0000000BUL                          /**< Mode LFXOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFRCOQ                       0x0000000CUL                          /**< Mode HFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_AUXHFRCOQ                    0x0000000DUL                          /**< Mode AUXHFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFXOQ                        0x0000000EUL                          /**< Mode HFXOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFSRCCLK                     0x0000000FUL                          /**< Mode HFSRCCLK for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_DEFAULT                       (_CMU_CTRL_CLKOUTSEL0_DEFAULT << 0)   /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_DISABLED                      (_CMU_CTRL_CLKOUTSEL0_DISABLED << 0)  /**< Shifted mode DISABLED for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_ULFRCO                        (_CMU_CTRL_CLKOUTSEL0_ULFRCO << 0)    /**< Shifted mode ULFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_LFRCO                         (_CMU_CTRL_CLKOUTSEL0_LFRCO << 0)     /**< Shifted mode LFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_LFXO                          (_CMU_CTRL_CLKOUTSEL0_LFXO << 0)      /**< Shifted mode LFXO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFXO                          (_CMU_CTRL_CLKOUTSEL0_HFXO << 0)      /**< Shifted mode HFXO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFEXPCLK                      (_CMU_CTRL_CLKOUTSEL0_HFEXPCLK << 0)  /**< Shifted mode HFEXPCLK for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_ULFRCOQ                       (_CMU_CTRL_CLKOUTSEL0_ULFRCOQ << 0)   /**< Shifted mode ULFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_LFRCOQ                        (_CMU_CTRL_CLKOUTSEL0_LFRCOQ << 0)    /**< Shifted mode LFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_LFXOQ                         (_CMU_CTRL_CLKOUTSEL0_LFXOQ << 0)     /**< Shifted mode LFXOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFRCOQ                        (_CMU_CTRL_CLKOUTSEL0_HFRCOQ << 0)    /**< Shifted mode HFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_AUXHFRCOQ                     (_CMU_CTRL_CLKOUTSEL0_AUXHFRCOQ << 0) /**< Shifted mode AUXHFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFXOQ                         (_CMU_CTRL_CLKOUTSEL0_HFXOQ << 0)     /**< Shifted mode HFXOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFSRCCLK                      (_CMU_CTRL_CLKOUTSEL0_HFSRCCLK << 0)  /**< Shifted mode HFSRCCLK for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_SHIFT                        5                                     /**< Shift value for CMU_CLKOUTSEL1 */
#define _CMU_CTRL_CLKOUTSEL1_MASK                         0x1E0UL                               /**< Bit mask for CMU_CLKOUTSEL1 */
#define _CMU_CTRL_CLKOUTSEL1_DEFAULT                      0x00000000UL                          /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_DISABLED                     0x00000000UL                          /**< Mode DISABLED for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_ULFRCO                       0x00000001UL                          /**< Mode ULFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFRCO                        0x00000002UL                          /**< Mode LFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFXO                         0x00000003UL                          /**< Mode LFXO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFXO                         0x00000006UL                          /**< Mode HFXO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFEXPCLK                     0x00000007UL                          /**< Mode HFEXPCLK for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_ULFRCOQ                      0x00000009UL                          /**< Mode ULFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFRCOQ                       0x0000000AUL                          /**< Mode LFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFXOQ                        0x0000000BUL                          /**< Mode LFXOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFRCOQ                       0x0000000CUL                          /**< Mode HFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ                    0x0000000DUL                          /**< Mode AUXHFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFXOQ                        0x0000000EUL                          /**< Mode HFXOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFSRCCLK                     0x0000000FUL                          /**< Mode HFSRCCLK for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_DEFAULT                       (_CMU_CTRL_CLKOUTSEL1_DEFAULT << 5)   /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_DISABLED                      (_CMU_CTRL_CLKOUTSEL1_DISABLED << 5)  /**< Shifted mode DISABLED for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_ULFRCO                        (_CMU_CTRL_CLKOUTSEL1_ULFRCO << 5)    /**< Shifted mode ULFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFRCO                         (_CMU_CTRL_CLKOUTSEL1_LFRCO << 5)     /**< Shifted mode LFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFXO                          (_CMU_CTRL_CLKOUTSEL1_LFXO << 5)      /**< Shifted mode LFXO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFXO                          (_CMU_CTRL_CLKOUTSEL1_HFXO << 5)      /**< Shifted mode HFXO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFEXPCLK                      (_CMU_CTRL_CLKOUTSEL1_HFEXPCLK << 5)  /**< Shifted mode HFEXPCLK for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_ULFRCOQ                       (_CMU_CTRL_CLKOUTSEL1_ULFRCOQ << 5)   /**< Shifted mode ULFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFRCOQ                        (_CMU_CTRL_CLKOUTSEL1_LFRCOQ << 5)    /**< Shifted mode LFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFXOQ                         (_CMU_CTRL_CLKOUTSEL1_LFXOQ << 5)     /**< Shifted mode LFXOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFRCOQ                        (_CMU_CTRL_CLKOUTSEL1_HFRCOQ << 5)    /**< Shifted mode HFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ                     (_CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ << 5) /**< Shifted mode AUXHFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFXOQ                         (_CMU_CTRL_CLKOUTSEL1_HFXOQ << 5)     /**< Shifted mode HFXOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFSRCCLK                      (_CMU_CTRL_CLKOUTSEL1_HFSRCCLK << 5)  /**< Shifted mode HFSRCCLK for CMU_CTRL */
#define CMU_CTRL_WSHFLE                                   (0x1UL << 16)                         /**< Wait State for High-Frequency LE Interface */
#define _CMU_CTRL_WSHFLE_SHIFT                            16                                    /**< Shift value for CMU_WSHFLE */
#define _CMU_CTRL_WSHFLE_MASK                             0x10000UL                             /**< Bit mask for CMU_WSHFLE */
#define _CMU_CTRL_WSHFLE_DEFAULT                          0x00000000UL                          /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_WSHFLE_DEFAULT                           (_CMU_CTRL_WSHFLE_DEFAULT << 16)      /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFPERCLKEN                               (0x1UL << 20)                         /**< HFPERCLK Enable */
#define _CMU_CTRL_HFPERCLKEN_SHIFT                        20                                    /**< Shift value for CMU_HFPERCLKEN */
#define _CMU_CTRL_HFPERCLKEN_MASK                         0x100000UL                            /**< Bit mask for CMU_HFPERCLKEN */
#define _CMU_CTRL_HFPERCLKEN_DEFAULT                      0x00000001UL                          /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFPERCLKEN_DEFAULT                       (_CMU_CTRL_HFPERCLKEN_DEFAULT << 20)  /**< Shifted mode DEFAULT for CMU_CTRL */

/* Bit fields for CMU HFRCOCTRL */
#define _CMU_HFRCOCTRL_RESETVALUE                         0xB1481F3CUL                                /**< Default value for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_MASK                               0xFFFF3F7FUL                                /**< Mask for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_TUNING_SHIFT                       0                                           /**< Shift value for CMU_TUNING */
#define _CMU_HFRCOCTRL_TUNING_MASK                        0x7FUL                                      /**< Bit mask for CMU_TUNING */
#define _CMU_HFRCOCTRL_TUNING_DEFAULT                     0x0000003CUL                                /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_TUNING_DEFAULT                      (_CMU_HFRCOCTRL_TUNING_DEFAULT << 0)        /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_FINETUNING_SHIFT                   8                                           /**< Shift value for CMU_FINETUNING */
#define _CMU_HFRCOCTRL_FINETUNING_MASK                    0x3F00UL                                    /**< Bit mask for CMU_FINETUNING */
#define _CMU_HFRCOCTRL_FINETUNING_DEFAULT                 0x0000001FUL                                /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_FINETUNING_DEFAULT                  (_CMU_HFRCOCTRL_FINETUNING_DEFAULT << 8)    /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_FREQRANGE_SHIFT                    16                                          /**< Shift value for CMU_FREQRANGE */
#define _CMU_HFRCOCTRL_FREQRANGE_MASK                     0x1F0000UL                                  /**< Bit mask for CMU_FREQRANGE */
#define _CMU_HFRCOCTRL_FREQRANGE_DEFAULT                  0x00000008UL                                /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_FREQRANGE_DEFAULT                   (_CMU_HFRCOCTRL_FREQRANGE_DEFAULT << 16)    /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_CMPBIAS_SHIFT                      21                                          /**< Shift value for CMU_CMPBIAS */
#define _CMU_HFRCOCTRL_CMPBIAS_MASK                       0xE00000UL                                  /**< Bit mask for CMU_CMPBIAS */
#define _CMU_HFRCOCTRL_CMPBIAS_DEFAULT                    0x00000002UL                                /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_CMPBIAS_DEFAULT                     (_CMU_HFRCOCTRL_CMPBIAS_DEFAULT << 21)      /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_LDOHP                               (0x1UL << 24)                               /**< HFRCO LDO High Power Mode */
#define _CMU_HFRCOCTRL_LDOHP_SHIFT                        24                                          /**< Shift value for CMU_LDOHP */
#define _CMU_HFRCOCTRL_LDOHP_MASK                         0x1000000UL                                 /**< Bit mask for CMU_LDOHP */
#define _CMU_HFRCOCTRL_LDOHP_DEFAULT                      0x00000001UL                                /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_LDOHP_DEFAULT                       (_CMU_HFRCOCTRL_LDOHP_DEFAULT << 24)        /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_CLKDIV_SHIFT                       25                                          /**< Shift value for CMU_CLKDIV */
#define _CMU_HFRCOCTRL_CLKDIV_MASK                        0x6000000UL                                 /**< Bit mask for CMU_CLKDIV */
#define _CMU_HFRCOCTRL_CLKDIV_DEFAULT                     0x00000000UL                                /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_CLKDIV_DIV1                        0x00000000UL                                /**< Mode DIV1 for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_CLKDIV_DIV2                        0x00000001UL                                /**< Mode DIV2 for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_CLKDIV_DIV4                        0x00000002UL                                /**< Mode DIV4 for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_CLKDIV_DEFAULT                      (_CMU_HFRCOCTRL_CLKDIV_DEFAULT << 25)       /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_CLKDIV_DIV1                         (_CMU_HFRCOCTRL_CLKDIV_DIV1 << 25)          /**< Shifted mode DIV1 for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_CLKDIV_DIV2                         (_CMU_HFRCOCTRL_CLKDIV_DIV2 << 25)          /**< Shifted mode DIV2 for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_CLKDIV_DIV4                         (_CMU_HFRCOCTRL_CLKDIV_DIV4 << 25)          /**< Shifted mode DIV4 for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_FINETUNINGEN                        (0x1UL << 27)                               /**< Enable reference for fine tuning */
#define _CMU_HFRCOCTRL_FINETUNINGEN_SHIFT                 27                                          /**< Shift value for CMU_FINETUNINGEN */
#define _CMU_HFRCOCTRL_FINETUNINGEN_MASK                  0x8000000UL                                 /**< Bit mask for CMU_FINETUNINGEN */
#define _CMU_HFRCOCTRL_FINETUNINGEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_FINETUNINGEN_DEFAULT                (_CMU_HFRCOCTRL_FINETUNINGEN_DEFAULT << 27) /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_VREFTC_SHIFT                       28                                          /**< Shift value for CMU_VREFTC */
#define _CMU_HFRCOCTRL_VREFTC_MASK                        0xF0000000UL                                /**< Bit mask for CMU_VREFTC */
#define _CMU_HFRCOCTRL_VREFTC_DEFAULT                     0x0000000BUL                                /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_VREFTC_DEFAULT                      (_CMU_HFRCOCTRL_VREFTC_DEFAULT << 28)       /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */

/* Bit fields for CMU AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_RESETVALUE                      0xB1481F3CUL                                   /**< Default value for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_MASK                            0xFFFF3F7FUL                                   /**< Mask for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_TUNING_SHIFT                    0                                              /**< Shift value for CMU_TUNING */
#define _CMU_AUXHFRCOCTRL_TUNING_MASK                     0x7FUL                                         /**< Bit mask for CMU_TUNING */
#define _CMU_AUXHFRCOCTRL_TUNING_DEFAULT                  0x0000003CUL                                   /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_TUNING_DEFAULT                   (_CMU_AUXHFRCOCTRL_TUNING_DEFAULT << 0)        /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_FINETUNING_SHIFT                8                                              /**< Shift value for CMU_FINETUNING */
#define _CMU_AUXHFRCOCTRL_FINETUNING_MASK                 0x3F00UL                                       /**< Bit mask for CMU_FINETUNING */
#define _CMU_AUXHFRCOCTRL_FINETUNING_DEFAULT              0x0000001FUL                                   /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_FINETUNING_DEFAULT               (_CMU_AUXHFRCOCTRL_FINETUNING_DEFAULT << 8)    /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_FREQRANGE_SHIFT                 16                                             /**< Shift value for CMU_FREQRANGE */
#define _CMU_AUXHFRCOCTRL_FREQRANGE_MASK                  0x1F0000UL                                     /**< Bit mask for CMU_FREQRANGE */
#define _CMU_AUXHFRCOCTRL_FREQRANGE_DEFAULT               0x00000008UL                                   /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_FREQRANGE_DEFAULT                (_CMU_AUXHFRCOCTRL_FREQRANGE_DEFAULT << 16)    /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_CMPBIAS_SHIFT                   21                                             /**< Shift value for CMU_CMPBIAS */
#define _CMU_AUXHFRCOCTRL_CMPBIAS_MASK                    0xE00000UL                                     /**< Bit mask for CMU_CMPBIAS */
#define _CMU_AUXHFRCOCTRL_CMPBIAS_DEFAULT                 0x00000002UL                                   /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_CMPBIAS_DEFAULT                  (_CMU_AUXHFRCOCTRL_CMPBIAS_DEFAULT << 21)      /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_LDOHP                            (0x1UL << 24)                                  /**< AUXHFRCO LDO High Power Mode */
#define _CMU_AUXHFRCOCTRL_LDOHP_SHIFT                     24                                             /**< Shift value for CMU_LDOHP */
#define _CMU_AUXHFRCOCTRL_LDOHP_MASK                      0x1000000UL                                    /**< Bit mask for CMU_LDOHP */
#define _CMU_AUXHFRCOCTRL_LDOHP_DEFAULT                   0x00000001UL                                   /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_LDOHP_DEFAULT                    (_CMU_AUXHFRCOCTRL_LDOHP_DEFAULT << 24)        /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_CLKDIV_SHIFT                    25                                             /**< Shift value for CMU_CLKDIV */
#define _CMU_AUXHFRCOCTRL_CLKDIV_MASK                     0x6000000UL                                    /**< Bit mask for CMU_CLKDIV */
#define _CMU_AUXHFRCOCTRL_CLKDIV_DEFAULT                  0x00000000UL                                   /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_CLKDIV_DIV1                     0x00000000UL                                   /**< Mode DIV1 for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_CLKDIV_DIV2                     0x00000001UL                                   /**< Mode DIV2 for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_CLKDIV_DIV4                     0x00000002UL                                   /**< Mode DIV4 for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_CLKDIV_DEFAULT                   (_CMU_AUXHFRCOCTRL_CLKDIV_DEFAULT << 25)       /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_CLKDIV_DIV1                      (_CMU_AUXHFRCOCTRL_CLKDIV_DIV1 << 25)          /**< Shifted mode DIV1 for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_CLKDIV_DIV2                      (_CMU_AUXHFRCOCTRL_CLKDIV_DIV2 << 25)          /**< Shifted mode DIV2 for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_CLKDIV_DIV4                      (_CMU_AUXHFRCOCTRL_CLKDIV_DIV4 << 25)          /**< Shifted mode DIV4 for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_FINETUNINGEN                     (0x1UL << 27)                                  /**< Enable reference for fine tuning */
#define _CMU_AUXHFRCOCTRL_FINETUNINGEN_SHIFT              27                                             /**< Shift value for CMU_FINETUNINGEN */
#define _CMU_AUXHFRCOCTRL_FINETUNINGEN_MASK               0x8000000UL                                    /**< Bit mask for CMU_FINETUNINGEN */
#define _CMU_AUXHFRCOCTRL_FINETUNINGEN_DEFAULT            0x00000000UL                                   /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_FINETUNINGEN_DEFAULT             (_CMU_AUXHFRCOCTRL_FINETUNINGEN_DEFAULT << 27) /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_VREFTC_SHIFT                    28                                             /**< Shift value for CMU_VREFTC */
#define _CMU_AUXHFRCOCTRL_VREFTC_MASK                     0xF0000000UL                                   /**< Bit mask for CMU_VREFTC */
#define _CMU_AUXHFRCOCTRL_VREFTC_DEFAULT                  0x0000000BUL                                   /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_VREFTC_DEFAULT                   (_CMU_AUXHFRCOCTRL_VREFTC_DEFAULT << 28)       /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */

/* Bit fields for CMU LFRCOCTRL */
#define _CMU_LFRCOCTRL_RESETVALUE                         0x81060100UL                              /**< Default value for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_MASK                               0xF30701FFUL                              /**< Mask for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_TUNING_SHIFT                       0                                         /**< Shift value for CMU_TUNING */
#define _CMU_LFRCOCTRL_TUNING_MASK                        0x1FFUL                                   /**< Bit mask for CMU_TUNING */
#define _CMU_LFRCOCTRL_TUNING_DEFAULT                     0x00000100UL                              /**< Mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_TUNING_DEFAULT                      (_CMU_LFRCOCTRL_TUNING_DEFAULT << 0)      /**< Shifted mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_ENVREF                              (0x1UL << 16)                             /**< Enable duty cycling of vref */
#define _CMU_LFRCOCTRL_ENVREF_SHIFT                       16                                        /**< Shift value for CMU_ENVREF */
#define _CMU_LFRCOCTRL_ENVREF_MASK                        0x10000UL                                 /**< Bit mask for CMU_ENVREF */
#define _CMU_LFRCOCTRL_ENVREF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_ENVREF_DEFAULT                      (_CMU_LFRCOCTRL_ENVREF_DEFAULT << 16)     /**< Shifted mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_ENCHOP                              (0x1UL << 17)                             /**< Enable comparator chopping */
#define _CMU_LFRCOCTRL_ENCHOP_SHIFT                       17                                        /**< Shift value for CMU_ENCHOP */
#define _CMU_LFRCOCTRL_ENCHOP_MASK                        0x20000UL                                 /**< Bit mask for CMU_ENCHOP */
#define _CMU_LFRCOCTRL_ENCHOP_DEFAULT                     0x00000001UL                              /**< Mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_ENCHOP_DEFAULT                      (_CMU_LFRCOCTRL_ENCHOP_DEFAULT << 17)     /**< Shifted mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_ENDEM                               (0x1UL << 18)                             /**< Enable dynamic element matching */
#define _CMU_LFRCOCTRL_ENDEM_SHIFT                        18                                        /**< Shift value for CMU_ENDEM */
#define _CMU_LFRCOCTRL_ENDEM_MASK                         0x40000UL                                 /**< Bit mask for CMU_ENDEM */
#define _CMU_LFRCOCTRL_ENDEM_DEFAULT                      0x00000001UL                              /**< Mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_ENDEM_DEFAULT                       (_CMU_LFRCOCTRL_ENDEM_DEFAULT << 18)      /**< Shifted mode DEFAULT for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_TIMEOUT_SHIFT                      24                                        /**< Shift value for CMU_TIMEOUT */
#define _CMU_LFRCOCTRL_TIMEOUT_MASK                       0x3000000UL                               /**< Bit mask for CMU_TIMEOUT */
#define _CMU_LFRCOCTRL_TIMEOUT_2CYCLES                    0x00000000UL                              /**< Mode 2CYCLES for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_TIMEOUT_DEFAULT                    0x00000001UL                              /**< Mode DEFAULT for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_TIMEOUT_16CYCLES                   0x00000001UL                              /**< Mode 16CYCLES for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_TIMEOUT_32CYCLES                   0x00000002UL                              /**< Mode 32CYCLES for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_TIMEOUT_2CYCLES                     (_CMU_LFRCOCTRL_TIMEOUT_2CYCLES << 24)    /**< Shifted mode 2CYCLES for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_TIMEOUT_DEFAULT                     (_CMU_LFRCOCTRL_TIMEOUT_DEFAULT << 24)    /**< Shifted mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_TIMEOUT_16CYCLES                    (_CMU_LFRCOCTRL_TIMEOUT_16CYCLES << 24)   /**< Shifted mode 16CYCLES for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_TIMEOUT_32CYCLES                    (_CMU_LFRCOCTRL_TIMEOUT_32CYCLES << 24)   /**< Shifted mode 32CYCLES for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_GMCCURTUNE_SHIFT                   28                                        /**< Shift value for CMU_GMCCURTUNE */
#define _CMU_LFRCOCTRL_GMCCURTUNE_MASK                    0xF0000000UL                              /**< Bit mask for CMU_GMCCURTUNE */
#define _CMU_LFRCOCTRL_GMCCURTUNE_DEFAULT                 0x00000008UL                              /**< Mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_GMCCURTUNE_DEFAULT                  (_CMU_LFRCOCTRL_GMCCURTUNE_DEFAULT << 28) /**< Shifted mode DEFAULT for CMU_LFRCOCTRL */

/* Bit fields for CMU HFXOCTRL */
#define _CMU_HFXOCTRL_RESETVALUE                          0x00000000UL                                     /**< Default value for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_MASK                                0x37000731UL                                     /**< Mask for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_MODE                                 (0x1UL << 0)                                     /**< HFXO Mode */
#define _CMU_HFXOCTRL_MODE_SHIFT                          0                                                /**< Shift value for CMU_MODE */
#define _CMU_HFXOCTRL_MODE_MASK                           0x1UL                                            /**< Bit mask for CMU_MODE */
#define _CMU_HFXOCTRL_MODE_DEFAULT                        0x00000000UL                                     /**< Mode DEFAULT for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_MODE_XTAL                           0x00000000UL                                     /**< Mode XTAL for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_MODE_EXTCLK                         0x00000001UL                                     /**< Mode EXTCLK for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_MODE_DEFAULT                         (_CMU_HFXOCTRL_MODE_DEFAULT << 0)                /**< Shifted mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_MODE_XTAL                            (_CMU_HFXOCTRL_MODE_XTAL << 0)                   /**< Shifted mode XTAL for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_MODE_EXTCLK                          (_CMU_HFXOCTRL_MODE_EXTCLK << 0)                 /**< Shifted mode EXTCLK for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_SHIFT           4                                                /**< Shift value for CMU_PEAKDETSHUNTOPTMODE */
#define _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK            0x30UL                                           /**< Bit mask for CMU_PEAKDETSHUNTOPTMODE */
#define _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_AUTOCMD         0x00000000UL                                     /**< Mode AUTOCMD for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_CMD             0x00000001UL                                     /**< Mode CMD for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MANUAL          0x00000002UL                                     /**< Mode MANUAL for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_DEFAULT          (_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_DEFAULT << 4) /**< Shifted mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_AUTOCMD          (_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_AUTOCMD << 4) /**< Shifted mode AUTOCMD for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_CMD              (_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_CMD << 4)     /**< Shifted mode CMD for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MANUAL           (_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MANUAL << 4)  /**< Shifted mode MANUAL for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LOWPOWER                             (0x1UL << 8)                                     /**< Low power mode control. PSR performance is reduced to enable low current consumption. */
#define _CMU_HFXOCTRL_LOWPOWER_SHIFT                      8                                                /**< Shift value for CMU_LOWPOWER */
#define _CMU_HFXOCTRL_LOWPOWER_MASK                       0x100UL                                          /**< Bit mask for CMU_LOWPOWER */
#define _CMU_HFXOCTRL_LOWPOWER_DEFAULT                    0x00000000UL                                     /**< Mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LOWPOWER_DEFAULT                     (_CMU_HFXOCTRL_LOWPOWER_DEFAULT << 8)            /**< Shifted mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_XTI2GND                              (0x1UL << 9)                                     /**< Clamp HFXTAL_N pin to ground when HFXO oscillator is off. */
#define _CMU_HFXOCTRL_XTI2GND_SHIFT                       9                                                /**< Shift value for CMU_XTI2GND */
#define _CMU_HFXOCTRL_XTI2GND_MASK                        0x200UL                                          /**< Bit mask for CMU_XTI2GND */
#define _CMU_HFXOCTRL_XTI2GND_DEFAULT                     0x00000000UL                                     /**< Mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_XTI2GND_DEFAULT                      (_CMU_HFXOCTRL_XTI2GND_DEFAULT << 9)             /**< Shifted mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_XTO2GND                              (0x1UL << 10)                                    /**< Clamp HFXTAL_P pin to ground when HFXO oscillator is off. */
#define _CMU_HFXOCTRL_XTO2GND_SHIFT                       10                                               /**< Shift value for CMU_XTO2GND */
#define _CMU_HFXOCTRL_XTO2GND_MASK                        0x400UL                                          /**< Bit mask for CMU_XTO2GND */
#define _CMU_HFXOCTRL_XTO2GND_DEFAULT                     0x00000000UL                                     /**< Mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_XTO2GND_DEFAULT                      (_CMU_HFXOCTRL_XTO2GND_DEFAULT << 10)            /**< Shifted mode DEFAULT for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_SHIFT                     24                                               /**< Shift value for CMU_LFTIMEOUT */
#define _CMU_HFXOCTRL_LFTIMEOUT_MASK                      0x7000000UL                                      /**< Bit mask for CMU_LFTIMEOUT */
#define _CMU_HFXOCTRL_LFTIMEOUT_DEFAULT                   0x00000000UL                                     /**< Mode DEFAULT for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_0CYCLES                   0x00000000UL                                     /**< Mode 0CYCLES for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_2CYCLES                   0x00000001UL                                     /**< Mode 2CYCLES for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_4CYCLES                   0x00000002UL                                     /**< Mode 4CYCLES for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_16CYCLES                  0x00000003UL                                     /**< Mode 16CYCLES for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_32CYCLES                  0x00000004UL                                     /**< Mode 32CYCLES for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_64CYCLES                  0x00000005UL                                     /**< Mode 64CYCLES for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_1KCYCLES                  0x00000006UL                                     /**< Mode 1KCYCLES for CMU_HFXOCTRL */
#define _CMU_HFXOCTRL_LFTIMEOUT_4KCYCLES                  0x00000007UL                                     /**< Mode 4KCYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_DEFAULT                    (_CMU_HFXOCTRL_LFTIMEOUT_DEFAULT << 24)          /**< Shifted mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_0CYCLES                    (_CMU_HFXOCTRL_LFTIMEOUT_0CYCLES << 24)          /**< Shifted mode 0CYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_2CYCLES                    (_CMU_HFXOCTRL_LFTIMEOUT_2CYCLES << 24)          /**< Shifted mode 2CYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_4CYCLES                    (_CMU_HFXOCTRL_LFTIMEOUT_4CYCLES << 24)          /**< Shifted mode 4CYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_16CYCLES                   (_CMU_HFXOCTRL_LFTIMEOUT_16CYCLES << 24)         /**< Shifted mode 16CYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_32CYCLES                   (_CMU_HFXOCTRL_LFTIMEOUT_32CYCLES << 24)         /**< Shifted mode 32CYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_64CYCLES                   (_CMU_HFXOCTRL_LFTIMEOUT_64CYCLES << 24)         /**< Shifted mode 64CYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_1KCYCLES                   (_CMU_HFXOCTRL_LFTIMEOUT_1KCYCLES << 24)         /**< Shifted mode 1KCYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_LFTIMEOUT_4KCYCLES                   (_CMU_HFXOCTRL_LFTIMEOUT_4KCYCLES << 24)         /**< Shifted mode 4KCYCLES for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_AUTOSTARTEM0EM1                      (0x1UL << 28)                                    /**< Automatically start of HFXO upon EM0/EM1 entry from EM2/EM3 */
#define _CMU_HFXOCTRL_AUTOSTARTEM0EM1_SHIFT               28                                               /**< Shift value for CMU_AUTOSTARTEM0EM1 */
#define _CMU_HFXOCTRL_AUTOSTARTEM0EM1_MASK                0x10000000UL                                     /**< Bit mask for CMU_AUTOSTARTEM0EM1 */
#define _CMU_HFXOCTRL_AUTOSTARTEM0EM1_DEFAULT             0x00000000UL                                     /**< Mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_AUTOSTARTEM0EM1_DEFAULT              (_CMU_HFXOCTRL_AUTOSTARTEM0EM1_DEFAULT << 28)    /**< Shifted mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_AUTOSTARTSELEM0EM1                   (0x1UL << 29)                                    /**< Automatically start and select of HFXO upon EM0/EM1 entry from EM2/EM3 */
#define _CMU_HFXOCTRL_AUTOSTARTSELEM0EM1_SHIFT            29                                               /**< Shift value for CMU_AUTOSTARTSELEM0EM1 */
#define _CMU_HFXOCTRL_AUTOSTARTSELEM0EM1_MASK             0x20000000UL                                     /**< Bit mask for CMU_AUTOSTARTSELEM0EM1 */
#define _CMU_HFXOCTRL_AUTOSTARTSELEM0EM1_DEFAULT          0x00000000UL                                     /**< Mode DEFAULT for CMU_HFXOCTRL */
#define CMU_HFXOCTRL_AUTOSTARTSELEM0EM1_DEFAULT           (_CMU_HFXOCTRL_AUTOSTARTSELEM0EM1_DEFAULT << 29) /**< Shifted mode DEFAULT for CMU_HFXOCTRL */

/* Bit fields for CMU HFXOCTRL1 */
#define _CMU_HFXOCTRL1_RESETVALUE                         0x00000240UL                             /**< Default value for CMU_HFXOCTRL1 */
#define _CMU_HFXOCTRL1_MASK                               0x00000277UL                             /**< Mask for CMU_HFXOCTRL1 */
#define _CMU_HFXOCTRL1_PEAKDETTHR_SHIFT                   0                                        /**< Shift value for CMU_PEAKDETTHR */
#define _CMU_HFXOCTRL1_PEAKDETTHR_MASK                    0x7UL                                    /**< Bit mask for CMU_PEAKDETTHR */
#define _CMU_HFXOCTRL1_PEAKDETTHR_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_HFXOCTRL1 */
#define CMU_HFXOCTRL1_PEAKDETTHR_DEFAULT                  (_CMU_HFXOCTRL1_PEAKDETTHR_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFXOCTRL1 */
#define _CMU_HFXOCTRL1_REGLVL_SHIFT                       4                                        /**< Shift value for CMU_REGLVL */
#define _CMU_HFXOCTRL1_REGLVL_MASK                        0x70UL                                   /**< Bit mask for CMU_REGLVL */
#define _CMU_HFXOCTRL1_REGLVL_DEFAULT                     0x00000004UL                             /**< Mode DEFAULT for CMU_HFXOCTRL1 */
#define CMU_HFXOCTRL1_REGLVL_DEFAULT                      (_CMU_HFXOCTRL1_REGLVL_DEFAULT << 4)     /**< Shifted mode DEFAULT for CMU_HFXOCTRL1 */
#define CMU_HFXOCTRL1_XTIBIASEN                           (0x1UL << 9)                             /**< Reserved for internal use. Do not change. */
#define _CMU_HFXOCTRL1_XTIBIASEN_SHIFT                    9                                        /**< Shift value for CMU_XTIBIASEN */
#define _CMU_HFXOCTRL1_XTIBIASEN_MASK                     0x200UL                                  /**< Bit mask for CMU_XTIBIASEN */
#define _CMU_HFXOCTRL1_XTIBIASEN_DEFAULT                  0x00000001UL                             /**< Mode DEFAULT for CMU_HFXOCTRL1 */
#define CMU_HFXOCTRL1_XTIBIASEN_DEFAULT                   (_CMU_HFXOCTRL1_XTIBIASEN_DEFAULT << 9)  /**< Shifted mode DEFAULT for CMU_HFXOCTRL1 */

/* Bit fields for CMU HFXOSTARTUPCTRL */
#define _CMU_HFXOSTARTUPCTRL_RESETVALUE                   0xA1250060UL                                     /**< Default value for CMU_HFXOSTARTUPCTRL */
#define _CMU_HFXOSTARTUPCTRL_MASK                         0xFFEFF87FUL                                     /**< Mask for CMU_HFXOSTARTUPCTRL */
#define _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_SHIFT           0                                                /**< Shift value for CMU_IBTRIMXOCORE */
#define _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_MASK            0x7FUL                                           /**< Bit mask for CMU_IBTRIMXOCORE */
#define _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_DEFAULT         0x00000060UL                                     /**< Mode DEFAULT for CMU_HFXOSTARTUPCTRL */
#define CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_DEFAULT          (_CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFXOSTARTUPCTRL */
#define _CMU_HFXOSTARTUPCTRL_CTUNE_SHIFT                  11                                               /**< Shift value for CMU_CTUNE */
#define _CMU_HFXOSTARTUPCTRL_CTUNE_MASK                   0xFF800UL                                        /**< Bit mask for CMU_CTUNE */
#define _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT                0x000000A0UL                                     /**< Mode DEFAULT for CMU_HFXOSTARTUPCTRL */
#define CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT                 (_CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT << 11)       /**< Shifted mode DEFAULT for CMU_HFXOSTARTUPCTRL */
#define _CMU_HFXOSTARTUPCTRL_RESERVED0_SHIFT              21                                               /**< Shift value for CMU_RESERVED0 */
#define _CMU_HFXOSTARTUPCTRL_RESERVED0_MASK               0xFE00000UL                                      /**< Bit mask for CMU_RESERVED0 */
#define _CMU_HFXOSTARTUPCTRL_RESERVED0_DEFAULT            0x00000009UL                                     /**< Mode DEFAULT for CMU_HFXOSTARTUPCTRL */
#define CMU_HFXOSTARTUPCTRL_RESERVED0_DEFAULT             (_CMU_HFXOSTARTUPCTRL_RESERVED0_DEFAULT << 21)   /**< Shifted mode DEFAULT for CMU_HFXOSTARTUPCTRL */
#define _CMU_HFXOSTARTUPCTRL_RESERVED1_SHIFT              28                                               /**< Shift value for CMU_RESERVED1 */
#define _CMU_HFXOSTARTUPCTRL_RESERVED1_MASK               0xF0000000UL                                     /**< Bit mask for CMU_RESERVED1 */
#define _CMU_HFXOSTARTUPCTRL_RESERVED1_DEFAULT            0x0000000AUL                                     /**< Mode DEFAULT for CMU_HFXOSTARTUPCTRL */
#define CMU_HFXOSTARTUPCTRL_RESERVED1_DEFAULT             (_CMU_HFXOSTARTUPCTRL_RESERVED1_DEFAULT << 28)   /**< Shifted mode DEFAULT for CMU_HFXOSTARTUPCTRL */

/* Bit fields for CMU HFXOSTEADYSTATECTRL */
#define _CMU_HFXOSTEADYSTATECTRL_RESETVALUE               0xA30AAD09UL                                         /**< Default value for CMU_HFXOSTEADYSTATECTRL */
#define _CMU_HFXOSTEADYSTATECTRL_MASK                     0xF70FFFFFUL                                         /**< Mask for CMU_HFXOSTEADYSTATECTRL */
#define _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_SHIFT       0                                                    /**< Shift value for CMU_IBTRIMXOCORE */
#define _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_MASK        0x7FUL                                               /**< Bit mask for CMU_IBTRIMXOCORE */
#define _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_DEFAULT     0x00000009UL                                         /**< Mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_DEFAULT      (_CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define _CMU_HFXOSTEADYSTATECTRL_REGISH_SHIFT             7                                                    /**< Shift value for CMU_REGISH */
#define _CMU_HFXOSTEADYSTATECTRL_REGISH_MASK              0x780UL                                              /**< Bit mask for CMU_REGISH */
#define _CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT           0x0000000AUL                                         /**< Mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT            (_CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT << 7)       /**< Shifted mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define _CMU_HFXOSTEADYSTATECTRL_CTUNE_SHIFT              11                                                   /**< Shift value for CMU_CTUNE */
#define _CMU_HFXOSTEADYSTATECTRL_CTUNE_MASK               0xFF800UL                                            /**< Bit mask for CMU_CTUNE */
#define _CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT            0x00000155UL                                         /**< Mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT             (_CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT << 11)       /**< Shifted mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define _CMU_HFXOSTEADYSTATECTRL_REGSELILOW_SHIFT         24                                                   /**< Shift value for CMU_REGSELILOW */
#define _CMU_HFXOSTEADYSTATECTRL_REGSELILOW_MASK          0x3000000UL                                          /**< Bit mask for CMU_REGSELILOW */
#define _CMU_HFXOSTEADYSTATECTRL_REGSELILOW_DEFAULT       0x00000003UL                                         /**< Mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define CMU_HFXOSTEADYSTATECTRL_REGSELILOW_DEFAULT        (_CMU_HFXOSTEADYSTATECTRL_REGSELILOW_DEFAULT << 24)  /**< Shifted mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define CMU_HFXOSTEADYSTATECTRL_PEAKDETEN                 (0x1UL << 26)                                        /**< Enables oscillator peak detectors */
#define _CMU_HFXOSTEADYSTATECTRL_PEAKDETEN_SHIFT          26                                                   /**< Shift value for CMU_PEAKDETEN */
#define _CMU_HFXOSTEADYSTATECTRL_PEAKDETEN_MASK           0x4000000UL                                          /**< Bit mask for CMU_PEAKDETEN */
#define _CMU_HFXOSTEADYSTATECTRL_PEAKDETEN_DEFAULT        0x00000000UL                                         /**< Mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define CMU_HFXOSTEADYSTATECTRL_PEAKDETEN_DEFAULT         (_CMU_HFXOSTEADYSTATECTRL_PEAKDETEN_DEFAULT << 26)   /**< Shifted mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define _CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_SHIFT        28                                                   /**< Shift value for CMU_REGISHUPPER */
#define _CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_MASK         0xF0000000UL                                         /**< Bit mask for CMU_REGISHUPPER */
#define _CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_DEFAULT      0x0000000AUL                                         /**< Mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */
#define CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_DEFAULT       (_CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_DEFAULT << 28) /**< Shifted mode DEFAULT for CMU_HFXOSTEADYSTATECTRL */

/* Bit fields for CMU HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_RESETVALUE                   0x00026667UL                                           /**< Default value for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_MASK                         0x000FFFFFUL                                           /**< Mask for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_SHIFT         0                                                      /**< Shift value for CMU_STARTUPTIMEOUT */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_MASK          0xFUL                                                  /**< Bit mask for CMU_STARTUPTIMEOUT */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_2CYCLES       0x00000000UL                                           /**< Mode 2CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_4CYCLES       0x00000001UL                                           /**< Mode 4CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_16CYCLES      0x00000002UL                                           /**< Mode 16CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_32CYCLES      0x00000003UL                                           /**< Mode 32CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_256CYCLES     0x00000004UL                                           /**< Mode 256CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_1KCYCLES      0x00000005UL                                           /**< Mode 1KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_2KCYCLES      0x00000006UL                                           /**< Mode 2KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT       0x00000007UL                                           /**< Mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_4KCYCLES      0x00000007UL                                           /**< Mode 4KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_8KCYCLES      0x00000008UL                                           /**< Mode 8KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_16KCYCLES     0x00000009UL                                           /**< Mode 16KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_32KCYCLES     0x0000000AUL                                           /**< Mode 32KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_2CYCLES        (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_2CYCLES << 0)     /**< Shifted mode 2CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_4CYCLES        (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_4CYCLES << 0)     /**< Shifted mode 4CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_16CYCLES       (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_16CYCLES << 0)    /**< Shifted mode 16CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_32CYCLES       (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_32CYCLES << 0)    /**< Shifted mode 32CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_256CYCLES      (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_256CYCLES << 0)   /**< Shifted mode 256CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_1KCYCLES       (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_1KCYCLES << 0)    /**< Shifted mode 1KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_2KCYCLES       (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_2KCYCLES << 0)    /**< Shifted mode 2KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT        (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_4KCYCLES       (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_4KCYCLES << 0)    /**< Shifted mode 4KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_8KCYCLES       (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_8KCYCLES << 0)    /**< Shifted mode 8KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_16KCYCLES      (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_16KCYCLES << 0)   /**< Shifted mode 16KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_32KCYCLES      (_CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_32KCYCLES << 0)   /**< Shifted mode 32KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_SHIFT          4                                                      /**< Shift value for CMU_STEADYTIMEOUT */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_MASK           0xF0UL                                                 /**< Bit mask for CMU_STEADYTIMEOUT */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_2CYCLES        0x00000000UL                                           /**< Mode 2CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_4CYCLES        0x00000001UL                                           /**< Mode 4CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_16CYCLES       0x00000002UL                                           /**< Mode 16CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_32CYCLES       0x00000003UL                                           /**< Mode 32CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_256CYCLES      0x00000004UL                                           /**< Mode 256CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_1KCYCLES       0x00000005UL                                           /**< Mode 1KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_DEFAULT        0x00000006UL                                           /**< Mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_2KCYCLES       0x00000006UL                                           /**< Mode 2KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_4KCYCLES       0x00000007UL                                           /**< Mode 4KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_8KCYCLES       0x00000008UL                                           /**< Mode 8KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_16KCYCLES      0x00000009UL                                           /**< Mode 16KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_32KCYCLES      0x0000000AUL                                           /**< Mode 32KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_2CYCLES         (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_2CYCLES << 4)      /**< Shifted mode 2CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_4CYCLES         (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_4CYCLES << 4)      /**< Shifted mode 4CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_16CYCLES        (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_16CYCLES << 4)     /**< Shifted mode 16CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_32CYCLES        (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_32CYCLES << 4)     /**< Shifted mode 32CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_256CYCLES       (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_256CYCLES << 4)    /**< Shifted mode 256CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_1KCYCLES        (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_1KCYCLES << 4)     /**< Shifted mode 1KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_DEFAULT         (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_DEFAULT << 4)      /**< Shifted mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_2KCYCLES        (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_2KCYCLES << 4)     /**< Shifted mode 2KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_4KCYCLES        (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_4KCYCLES << 4)     /**< Shifted mode 4KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_8KCYCLES        (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_8KCYCLES << 4)     /**< Shifted mode 8KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_16KCYCLES       (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_16KCYCLES << 4)    /**< Shifted mode 16KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_32KCYCLES       (_CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_32KCYCLES << 4)    /**< Shifted mode 32KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_RESERVED2_SHIFT              8                                                      /**< Shift value for CMU_RESERVED2 */
#define _CMU_HFXOTIMEOUTCTRL_RESERVED2_MASK               0xF00UL                                                /**< Bit mask for CMU_RESERVED2 */
#define _CMU_HFXOTIMEOUTCTRL_RESERVED2_DEFAULT            0x00000006UL                                           /**< Mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_RESERVED2_DEFAULT             (_CMU_HFXOTIMEOUTCTRL_RESERVED2_DEFAULT << 8)          /**< Shifted mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_SHIFT         12                                                     /**< Shift value for CMU_PEAKDETTIMEOUT */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_MASK          0xF000UL                                               /**< Bit mask for CMU_PEAKDETTIMEOUT */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_2CYCLES       0x00000000UL                                           /**< Mode 2CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_4CYCLES       0x00000001UL                                           /**< Mode 4CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_16CYCLES      0x00000002UL                                           /**< Mode 16CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_32CYCLES      0x00000003UL                                           /**< Mode 32CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_256CYCLES     0x00000004UL                                           /**< Mode 256CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_1KCYCLES      0x00000005UL                                           /**< Mode 1KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_DEFAULT       0x00000006UL                                           /**< Mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_2KCYCLES      0x00000006UL                                           /**< Mode 2KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_4KCYCLES      0x00000007UL                                           /**< Mode 4KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_8KCYCLES      0x00000008UL                                           /**< Mode 8KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_16KCYCLES     0x00000009UL                                           /**< Mode 16KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_32KCYCLES     0x0000000AUL                                           /**< Mode 32KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_2CYCLES        (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_2CYCLES << 12)    /**< Shifted mode 2CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_4CYCLES        (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_4CYCLES << 12)    /**< Shifted mode 4CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_16CYCLES       (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_16CYCLES << 12)   /**< Shifted mode 16CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_32CYCLES       (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_32CYCLES << 12)   /**< Shifted mode 32CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_256CYCLES      (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_256CYCLES << 12)  /**< Shifted mode 256CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_1KCYCLES       (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_1KCYCLES << 12)   /**< Shifted mode 1KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_DEFAULT        (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_DEFAULT << 12)    /**< Shifted mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_2KCYCLES       (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_2KCYCLES << 12)   /**< Shifted mode 2KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_4KCYCLES       (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_4KCYCLES << 12)   /**< Shifted mode 4KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_8KCYCLES       (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_8KCYCLES << 12)   /**< Shifted mode 8KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_16KCYCLES      (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_16KCYCLES << 12)  /**< Shifted mode 16KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_32KCYCLES      (_CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_32KCYCLES << 12)  /**< Shifted mode 32KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_SHIFT        16                                                     /**< Shift value for CMU_SHUNTOPTTIMEOUT */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_MASK         0xF0000UL                                              /**< Bit mask for CMU_SHUNTOPTTIMEOUT */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_2CYCLES      0x00000000UL                                           /**< Mode 2CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_4CYCLES      0x00000001UL                                           /**< Mode 4CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT      0x00000002UL                                           /**< Mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_16CYCLES     0x00000002UL                                           /**< Mode 16CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_32CYCLES     0x00000003UL                                           /**< Mode 32CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_256CYCLES    0x00000004UL                                           /**< Mode 256CYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_1KCYCLES     0x00000005UL                                           /**< Mode 1KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_2KCYCLES     0x00000006UL                                           /**< Mode 2KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_4KCYCLES     0x00000007UL                                           /**< Mode 4KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_8KCYCLES     0x00000008UL                                           /**< Mode 8KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_16KCYCLES    0x00000009UL                                           /**< Mode 16KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_32KCYCLES    0x0000000AUL                                           /**< Mode 32KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_2CYCLES       (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_2CYCLES << 16)   /**< Shifted mode 2CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_4CYCLES       (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_4CYCLES << 16)   /**< Shifted mode 4CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT       (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT << 16)   /**< Shifted mode DEFAULT for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_16CYCLES      (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_16CYCLES << 16)  /**< Shifted mode 16CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_32CYCLES      (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_32CYCLES << 16)  /**< Shifted mode 32CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_256CYCLES     (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_256CYCLES << 16) /**< Shifted mode 256CYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_1KCYCLES      (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_1KCYCLES << 16)  /**< Shifted mode 1KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_2KCYCLES      (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_2KCYCLES << 16)  /**< Shifted mode 2KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_4KCYCLES      (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_4KCYCLES << 16)  /**< Shifted mode 4KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_8KCYCLES      (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_8KCYCLES << 16)  /**< Shifted mode 8KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_16KCYCLES     (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_16KCYCLES << 16) /**< Shifted mode 16KCYCLES for CMU_HFXOTIMEOUTCTRL */
#define CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_32KCYCLES     (_CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_32KCYCLES << 16) /**< Shifted mode 32KCYCLES for CMU_HFXOTIMEOUTCTRL */

/* Bit fields for CMU LFXOCTRL */
#define _CMU_LFXOCTRL_RESETVALUE                          0x07009000UL                            /**< Default value for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_MASK                                0x0713DB7FUL                            /**< Mask for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TUNING_SHIFT                        0                                       /**< Shift value for CMU_TUNING */
#define _CMU_LFXOCTRL_TUNING_MASK                         0x7FUL                                  /**< Bit mask for CMU_TUNING */
#define _CMU_LFXOCTRL_TUNING_DEFAULT                      0x00000000UL                            /**< Mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TUNING_DEFAULT                       (_CMU_LFXOCTRL_TUNING_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_MODE_SHIFT                          8                                       /**< Shift value for CMU_MODE */
#define _CMU_LFXOCTRL_MODE_MASK                           0x300UL                                 /**< Bit mask for CMU_MODE */
#define _CMU_LFXOCTRL_MODE_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_MODE_XTAL                           0x00000000UL                            /**< Mode XTAL for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_MODE_BUFEXTCLK                      0x00000001UL                            /**< Mode BUFEXTCLK for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_MODE_DIGEXTCLK                      0x00000002UL                            /**< Mode DIGEXTCLK for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_MODE_DEFAULT                         (_CMU_LFXOCTRL_MODE_DEFAULT << 8)       /**< Shifted mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_MODE_XTAL                            (_CMU_LFXOCTRL_MODE_XTAL << 8)          /**< Shifted mode XTAL for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_MODE_BUFEXTCLK                       (_CMU_LFXOCTRL_MODE_BUFEXTCLK << 8)     /**< Shifted mode BUFEXTCLK for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_MODE_DIGEXTCLK                       (_CMU_LFXOCTRL_MODE_DIGEXTCLK << 8)     /**< Shifted mode DIGEXTCLK for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_GAIN_SHIFT                          11                                      /**< Shift value for CMU_GAIN */
#define _CMU_LFXOCTRL_GAIN_MASK                           0x1800UL                                /**< Bit mask for CMU_GAIN */
#define _CMU_LFXOCTRL_GAIN_DEFAULT                        0x00000002UL                            /**< Mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_GAIN_DEFAULT                         (_CMU_LFXOCTRL_GAIN_DEFAULT << 11)      /**< Shifted mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_HIGHAMPL                             (0x1UL << 14)                           /**< LFXO High XTAL Oscillation Amplitude Enable */
#define _CMU_LFXOCTRL_HIGHAMPL_SHIFT                      14                                      /**< Shift value for CMU_HIGHAMPL */
#define _CMU_LFXOCTRL_HIGHAMPL_MASK                       0x4000UL                                /**< Bit mask for CMU_HIGHAMPL */
#define _CMU_LFXOCTRL_HIGHAMPL_DEFAULT                    0x00000000UL                            /**< Mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_HIGHAMPL_DEFAULT                     (_CMU_LFXOCTRL_HIGHAMPL_DEFAULT << 14)  /**< Shifted mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_AGC                                  (0x1UL << 15)                           /**< LFXO AGC Enable */
#define _CMU_LFXOCTRL_AGC_SHIFT                           15                                      /**< Shift value for CMU_AGC */
#define _CMU_LFXOCTRL_AGC_MASK                            0x8000UL                                /**< Bit mask for CMU_AGC */
#define _CMU_LFXOCTRL_AGC_DEFAULT                         0x00000001UL                            /**< Mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_AGC_DEFAULT                          (_CMU_LFXOCTRL_AGC_DEFAULT << 15)       /**< Shifted mode DEFAULT for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_CUR_SHIFT                           16                                      /**< Shift value for CMU_CUR */
#define _CMU_LFXOCTRL_CUR_MASK                            0x30000UL                               /**< Bit mask for CMU_CUR */
#define _CMU_LFXOCTRL_CUR_DEFAULT                         0x00000000UL                            /**< Mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_CUR_DEFAULT                          (_CMU_LFXOCTRL_CUR_DEFAULT << 16)       /**< Shifted mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_BUFCUR                               (0x1UL << 20)                           /**< LFXO Buffer Bias Current */
#define _CMU_LFXOCTRL_BUFCUR_SHIFT                        20                                      /**< Shift value for CMU_BUFCUR */
#define _CMU_LFXOCTRL_BUFCUR_MASK                         0x100000UL                              /**< Bit mask for CMU_BUFCUR */
#define _CMU_LFXOCTRL_BUFCUR_DEFAULT                      0x00000000UL                            /**< Mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_BUFCUR_DEFAULT                       (_CMU_LFXOCTRL_BUFCUR_DEFAULT << 20)    /**< Shifted mode DEFAULT for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_SHIFT                       24                                      /**< Shift value for CMU_TIMEOUT */
#define _CMU_LFXOCTRL_TIMEOUT_MASK                        0x7000000UL                             /**< Bit mask for CMU_TIMEOUT */
#define _CMU_LFXOCTRL_TIMEOUT_2CYCLES                     0x00000000UL                            /**< Mode 2CYCLES for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_256CYCLES                   0x00000001UL                            /**< Mode 256CYCLES for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_1KCYCLES                    0x00000002UL                            /**< Mode 1KCYCLES for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_2KCYCLES                    0x00000003UL                            /**< Mode 2KCYCLES for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_4KCYCLES                    0x00000004UL                            /**< Mode 4KCYCLES for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_8KCYCLES                    0x00000005UL                            /**< Mode 8KCYCLES for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_16KCYCLES                   0x00000006UL                            /**< Mode 16KCYCLES for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_DEFAULT                     0x00000007UL                            /**< Mode DEFAULT for CMU_LFXOCTRL */
#define _CMU_LFXOCTRL_TIMEOUT_32KCYCLES                   0x00000007UL                            /**< Mode 32KCYCLES for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_2CYCLES                      (_CMU_LFXOCTRL_TIMEOUT_2CYCLES << 24)   /**< Shifted mode 2CYCLES for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_256CYCLES                    (_CMU_LFXOCTRL_TIMEOUT_256CYCLES << 24) /**< Shifted mode 256CYCLES for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_1KCYCLES                     (_CMU_LFXOCTRL_TIMEOUT_1KCYCLES << 24)  /**< Shifted mode 1KCYCLES for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_2KCYCLES                     (_CMU_LFXOCTRL_TIMEOUT_2KCYCLES << 24)  /**< Shifted mode 2KCYCLES for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_4KCYCLES                     (_CMU_LFXOCTRL_TIMEOUT_4KCYCLES << 24)  /**< Shifted mode 4KCYCLES for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_8KCYCLES                     (_CMU_LFXOCTRL_TIMEOUT_8KCYCLES << 24)  /**< Shifted mode 8KCYCLES for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_16KCYCLES                    (_CMU_LFXOCTRL_TIMEOUT_16KCYCLES << 24) /**< Shifted mode 16KCYCLES for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_DEFAULT                      (_CMU_LFXOCTRL_TIMEOUT_DEFAULT << 24)   /**< Shifted mode DEFAULT for CMU_LFXOCTRL */
#define CMU_LFXOCTRL_TIMEOUT_32KCYCLES                    (_CMU_LFXOCTRL_TIMEOUT_32KCYCLES << 24) /**< Shifted mode 32KCYCLES for CMU_LFXOCTRL */

/* Bit fields for CMU ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_RESETVALUE                        0x00020020UL                            /**< Default value for CMU_ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_MASK                              0x00030C3FUL                            /**< Mask for CMU_ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_TUNING_SHIFT                      0                                       /**< Shift value for CMU_TUNING */
#define _CMU_ULFRCOCTRL_TUNING_MASK                       0x3FUL                                  /**< Bit mask for CMU_TUNING */
#define _CMU_ULFRCOCTRL_TUNING_DEFAULT                    0x00000020UL                            /**< Mode DEFAULT for CMU_ULFRCOCTRL */
#define CMU_ULFRCOCTRL_TUNING_DEFAULT                     (_CMU_ULFRCOCTRL_TUNING_DEFAULT << 0)   /**< Shifted mode DEFAULT for CMU_ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_MODE_SHIFT                        10                                      /**< Shift value for CMU_MODE */
#define _CMU_ULFRCOCTRL_MODE_MASK                         0xC00UL                                 /**< Bit mask for CMU_MODE */
#define _CMU_ULFRCOCTRL_MODE_DEFAULT                      0x00000000UL                            /**< Mode DEFAULT for CMU_ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_MODE_1KHZ                         0x00000000UL                            /**< Mode 1KHZ for CMU_ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_MODE_2KHZ                         0x00000001UL                            /**< Mode 2KHZ for CMU_ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_MODE_4KHZ                         0x00000002UL                            /**< Mode 4KHZ for CMU_ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_MODE_32KHZ                        0x00000003UL                            /**< Mode 32KHZ for CMU_ULFRCOCTRL */
#define CMU_ULFRCOCTRL_MODE_DEFAULT                       (_CMU_ULFRCOCTRL_MODE_DEFAULT << 10)    /**< Shifted mode DEFAULT for CMU_ULFRCOCTRL */
#define CMU_ULFRCOCTRL_MODE_1KHZ                          (_CMU_ULFRCOCTRL_MODE_1KHZ << 10)       /**< Shifted mode 1KHZ for CMU_ULFRCOCTRL */
#define CMU_ULFRCOCTRL_MODE_2KHZ                          (_CMU_ULFRCOCTRL_MODE_2KHZ << 10)       /**< Shifted mode 2KHZ for CMU_ULFRCOCTRL */
#define CMU_ULFRCOCTRL_MODE_4KHZ                          (_CMU_ULFRCOCTRL_MODE_4KHZ << 10)       /**< Shifted mode 4KHZ for CMU_ULFRCOCTRL */
#define CMU_ULFRCOCTRL_MODE_32KHZ                         (_CMU_ULFRCOCTRL_MODE_32KHZ << 10)      /**< Shifted mode 32KHZ for CMU_ULFRCOCTRL */
#define _CMU_ULFRCOCTRL_RESTRIM_SHIFT                     16                                      /**< Shift value for CMU_RESTRIM */
#define _CMU_ULFRCOCTRL_RESTRIM_MASK                      0x30000UL                               /**< Bit mask for CMU_RESTRIM */
#define _CMU_ULFRCOCTRL_RESTRIM_DEFAULT                   0x00000002UL                            /**< Mode DEFAULT for CMU_ULFRCOCTRL */
#define CMU_ULFRCOCTRL_RESTRIM_DEFAULT                    (_CMU_ULFRCOCTRL_RESTRIM_DEFAULT << 16) /**< Shifted mode DEFAULT for CMU_ULFRCOCTRL */

/* Bit fields for CMU CALCTRL */
#define _CMU_CALCTRL_RESETVALUE                           0x00000000UL                            /**< Default value for CMU_CALCTRL */
#define _CMU_CALCTRL_MASK                                 0x0F0F0177UL                            /**< Mask for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_SHIFT                          0                                       /**< Shift value for CMU_UPSEL */
#define _CMU_CALCTRL_UPSEL_MASK                           0x7UL                                   /**< Bit mask for CMU_UPSEL */
#define _CMU_CALCTRL_UPSEL_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_HFXO                           0x00000000UL                            /**< Mode HFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_LFXO                           0x00000001UL                            /**< Mode LFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_HFRCO                          0x00000002UL                            /**< Mode HFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_LFRCO                          0x00000003UL                            /**< Mode LFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_AUXHFRCO                       0x00000004UL                            /**< Mode AUXHFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_PRS                            0x00000005UL                            /**< Mode PRS for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_DEFAULT                         (_CMU_CALCTRL_UPSEL_DEFAULT << 0)       /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_HFXO                            (_CMU_CALCTRL_UPSEL_HFXO << 0)          /**< Shifted mode HFXO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_LFXO                            (_CMU_CALCTRL_UPSEL_LFXO << 0)          /**< Shifted mode LFXO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_HFRCO                           (_CMU_CALCTRL_UPSEL_HFRCO << 0)         /**< Shifted mode HFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_LFRCO                           (_CMU_CALCTRL_UPSEL_LFRCO << 0)         /**< Shifted mode LFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_AUXHFRCO                        (_CMU_CALCTRL_UPSEL_AUXHFRCO << 0)      /**< Shifted mode AUXHFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_PRS                             (_CMU_CALCTRL_UPSEL_PRS << 0)           /**< Shifted mode PRS for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_SHIFT                        4                                       /**< Shift value for CMU_DOWNSEL */
#define _CMU_CALCTRL_DOWNSEL_MASK                         0x70UL                                  /**< Bit mask for CMU_DOWNSEL */
#define _CMU_CALCTRL_DOWNSEL_DEFAULT                      0x00000000UL                            /**< Mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFCLK                        0x00000000UL                            /**< Mode HFCLK for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFXO                         0x00000001UL                            /**< Mode HFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_LFXO                         0x00000002UL                            /**< Mode LFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFRCO                        0x00000003UL                            /**< Mode HFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_LFRCO                        0x00000004UL                            /**< Mode LFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_AUXHFRCO                     0x00000005UL                            /**< Mode AUXHFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_PRS                          0x00000006UL                            /**< Mode PRS for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_DEFAULT                       (_CMU_CALCTRL_DOWNSEL_DEFAULT << 4)     /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFCLK                         (_CMU_CALCTRL_DOWNSEL_HFCLK << 4)       /**< Shifted mode HFCLK for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFXO                          (_CMU_CALCTRL_DOWNSEL_HFXO << 4)        /**< Shifted mode HFXO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_LFXO                          (_CMU_CALCTRL_DOWNSEL_LFXO << 4)        /**< Shifted mode LFXO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFRCO                         (_CMU_CALCTRL_DOWNSEL_HFRCO << 4)       /**< Shifted mode HFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_LFRCO                         (_CMU_CALCTRL_DOWNSEL_LFRCO << 4)       /**< Shifted mode LFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_AUXHFRCO                      (_CMU_CALCTRL_DOWNSEL_AUXHFRCO << 4)    /**< Shifted mode AUXHFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_PRS                           (_CMU_CALCTRL_DOWNSEL_PRS << 4)         /**< Shifted mode PRS for CMU_CALCTRL */
#define CMU_CALCTRL_CONT                                  (0x1UL << 8)                            /**< Continuous Calibration */
#define _CMU_CALCTRL_CONT_SHIFT                           8                                       /**< Shift value for CMU_CONT */
#define _CMU_CALCTRL_CONT_MASK                            0x100UL                                 /**< Bit mask for CMU_CONT */
#define _CMU_CALCTRL_CONT_DEFAULT                         0x00000000UL                            /**< Mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_CONT_DEFAULT                          (_CMU_CALCTRL_CONT_DEFAULT << 8)        /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_SHIFT                       16                                      /**< Shift value for CMU_PRSUPSEL */
#define _CMU_CALCTRL_PRSUPSEL_MASK                        0xF0000UL                               /**< Bit mask for CMU_PRSUPSEL */
#define _CMU_CALCTRL_PRSUPSEL_DEFAULT                     0x00000000UL                            /**< Mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH0                      0x00000000UL                            /**< Mode PRSCH0 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH1                      0x00000001UL                            /**< Mode PRSCH1 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH2                      0x00000002UL                            /**< Mode PRSCH2 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH3                      0x00000003UL                            /**< Mode PRSCH3 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH4                      0x00000004UL                            /**< Mode PRSCH4 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH5                      0x00000005UL                            /**< Mode PRSCH5 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH6                      0x00000006UL                            /**< Mode PRSCH6 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH7                      0x00000007UL                            /**< Mode PRSCH7 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH8                      0x00000008UL                            /**< Mode PRSCH8 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH9                      0x00000009UL                            /**< Mode PRSCH9 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH10                     0x0000000AUL                            /**< Mode PRSCH10 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSUPSEL_PRSCH11                     0x0000000BUL                            /**< Mode PRSCH11 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_DEFAULT                      (_CMU_CALCTRL_PRSUPSEL_DEFAULT << 16)   /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH0                       (_CMU_CALCTRL_PRSUPSEL_PRSCH0 << 16)    /**< Shifted mode PRSCH0 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH1                       (_CMU_CALCTRL_PRSUPSEL_PRSCH1 << 16)    /**< Shifted mode PRSCH1 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH2                       (_CMU_CALCTRL_PRSUPSEL_PRSCH2 << 16)    /**< Shifted mode PRSCH2 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH3                       (_CMU_CALCTRL_PRSUPSEL_PRSCH3 << 16)    /**< Shifted mode PRSCH3 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH4                       (_CMU_CALCTRL_PRSUPSEL_PRSCH4 << 16)    /**< Shifted mode PRSCH4 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH5                       (_CMU_CALCTRL_PRSUPSEL_PRSCH5 << 16)    /**< Shifted mode PRSCH5 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH6                       (_CMU_CALCTRL_PRSUPSEL_PRSCH6 << 16)    /**< Shifted mode PRSCH6 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH7                       (_CMU_CALCTRL_PRSUPSEL_PRSCH7 << 16)    /**< Shifted mode PRSCH7 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH8                       (_CMU_CALCTRL_PRSUPSEL_PRSCH8 << 16)    /**< Shifted mode PRSCH8 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH9                       (_CMU_CALCTRL_PRSUPSEL_PRSCH9 << 16)    /**< Shifted mode PRSCH9 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH10                      (_CMU_CALCTRL_PRSUPSEL_PRSCH10 << 16)   /**< Shifted mode PRSCH10 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSUPSEL_PRSCH11                      (_CMU_CALCTRL_PRSUPSEL_PRSCH11 << 16)   /**< Shifted mode PRSCH11 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_SHIFT                     24                                      /**< Shift value for CMU_PRSDOWNSEL */
#define _CMU_CALCTRL_PRSDOWNSEL_MASK                      0xF000000UL                             /**< Bit mask for CMU_PRSDOWNSEL */
#define _CMU_CALCTRL_PRSDOWNSEL_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH0                    0x00000000UL                            /**< Mode PRSCH0 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH1                    0x00000001UL                            /**< Mode PRSCH1 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH2                    0x00000002UL                            /**< Mode PRSCH2 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH3                    0x00000003UL                            /**< Mode PRSCH3 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH4                    0x00000004UL                            /**< Mode PRSCH4 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH5                    0x00000005UL                            /**< Mode PRSCH5 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH6                    0x00000006UL                            /**< Mode PRSCH6 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH7                    0x00000007UL                            /**< Mode PRSCH7 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH8                    0x00000008UL                            /**< Mode PRSCH8 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH9                    0x00000009UL                            /**< Mode PRSCH9 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH10                   0x0000000AUL                            /**< Mode PRSCH10 for CMU_CALCTRL */
#define _CMU_CALCTRL_PRSDOWNSEL_PRSCH11                   0x0000000BUL                            /**< Mode PRSCH11 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_DEFAULT                    (_CMU_CALCTRL_PRSDOWNSEL_DEFAULT << 24) /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH0                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH0 << 24)  /**< Shifted mode PRSCH0 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH1                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH1 << 24)  /**< Shifted mode PRSCH1 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH2                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH2 << 24)  /**< Shifted mode PRSCH2 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH3                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH3 << 24)  /**< Shifted mode PRSCH3 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH4                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH4 << 24)  /**< Shifted mode PRSCH4 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH5                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH5 << 24)  /**< Shifted mode PRSCH5 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH6                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH6 << 24)  /**< Shifted mode PRSCH6 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH7                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH7 << 24)  /**< Shifted mode PRSCH7 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH8                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH8 << 24)  /**< Shifted mode PRSCH8 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH9                     (_CMU_CALCTRL_PRSDOWNSEL_PRSCH9 << 24)  /**< Shifted mode PRSCH9 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH10                    (_CMU_CALCTRL_PRSDOWNSEL_PRSCH10 << 24) /**< Shifted mode PRSCH10 for CMU_CALCTRL */
#define CMU_CALCTRL_PRSDOWNSEL_PRSCH11                    (_CMU_CALCTRL_PRSDOWNSEL_PRSCH11 << 24) /**< Shifted mode PRSCH11 for CMU_CALCTRL */

/* Bit fields for CMU CALCNT */
#define _CMU_CALCNT_RESETVALUE                            0x00000000UL                      /**< Default value for CMU_CALCNT */
#define _CMU_CALCNT_MASK                                  0x000FFFFFUL                      /**< Mask for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_SHIFT                          0                                 /**< Shift value for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_MASK                           0xFFFFFUL                         /**< Bit mask for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for CMU_CALCNT */
#define CMU_CALCNT_CALCNT_DEFAULT                         (_CMU_CALCNT_CALCNT_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_CALCNT */

/* Bit fields for CMU OSCENCMD */
#define _CMU_OSCENCMD_RESETVALUE                          0x00000000UL                             /**< Default value for CMU_OSCENCMD */
#define _CMU_OSCENCMD_MASK                                0x000003FFUL                             /**< Mask for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCOEN                              (0x1UL << 0)                             /**< HFRCO Enable */
#define _CMU_OSCENCMD_HFRCOEN_SHIFT                       0                                        /**< Shift value for CMU_HFRCOEN */
#define _CMU_OSCENCMD_HFRCOEN_MASK                        0x1UL                                    /**< Bit mask for CMU_HFRCOEN */
#define _CMU_OSCENCMD_HFRCOEN_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCOEN_DEFAULT                      (_CMU_OSCENCMD_HFRCOEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCODIS                             (0x1UL << 1)                             /**< HFRCO Disable */
#define _CMU_OSCENCMD_HFRCODIS_SHIFT                      1                                        /**< Shift value for CMU_HFRCODIS */
#define _CMU_OSCENCMD_HFRCODIS_MASK                       0x2UL                                    /**< Bit mask for CMU_HFRCODIS */
#define _CMU_OSCENCMD_HFRCODIS_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCODIS_DEFAULT                     (_CMU_OSCENCMD_HFRCODIS_DEFAULT << 1)    /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXOEN                               (0x1UL << 2)                             /**< HFXO Enable */
#define _CMU_OSCENCMD_HFXOEN_SHIFT                        2                                        /**< Shift value for CMU_HFXOEN */
#define _CMU_OSCENCMD_HFXOEN_MASK                         0x4UL                                    /**< Bit mask for CMU_HFXOEN */
#define _CMU_OSCENCMD_HFXOEN_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXOEN_DEFAULT                       (_CMU_OSCENCMD_HFXOEN_DEFAULT << 2)      /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXODIS                              (0x1UL << 3)                             /**< HFXO Disable */
#define _CMU_OSCENCMD_HFXODIS_SHIFT                       3                                        /**< Shift value for CMU_HFXODIS */
#define _CMU_OSCENCMD_HFXODIS_MASK                        0x8UL                                    /**< Bit mask for CMU_HFXODIS */
#define _CMU_OSCENCMD_HFXODIS_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXODIS_DEFAULT                      (_CMU_OSCENCMD_HFXODIS_DEFAULT << 3)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCOEN                           (0x1UL << 4)                             /**< AUXHFRCO Enable */
#define _CMU_OSCENCMD_AUXHFRCOEN_SHIFT                    4                                        /**< Shift value for CMU_AUXHFRCOEN */
#define _CMU_OSCENCMD_AUXHFRCOEN_MASK                     0x10UL                                   /**< Bit mask for CMU_AUXHFRCOEN */
#define _CMU_OSCENCMD_AUXHFRCOEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCOEN_DEFAULT                   (_CMU_OSCENCMD_AUXHFRCOEN_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCODIS                          (0x1UL << 5)                             /**< AUXHFRCO Disable */
#define _CMU_OSCENCMD_AUXHFRCODIS_SHIFT                   5                                        /**< Shift value for CMU_AUXHFRCODIS */
#define _CMU_OSCENCMD_AUXHFRCODIS_MASK                    0x20UL                                   /**< Bit mask for CMU_AUXHFRCODIS */
#define _CMU_OSCENCMD_AUXHFRCODIS_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCODIS_DEFAULT                  (_CMU_OSCENCMD_AUXHFRCODIS_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCOEN                              (0x1UL << 6)                             /**< LFRCO Enable */
#define _CMU_OSCENCMD_LFRCOEN_SHIFT                       6                                        /**< Shift value for CMU_LFRCOEN */
#define _CMU_OSCENCMD_LFRCOEN_MASK                        0x40UL                                   /**< Bit mask for CMU_LFRCOEN */
#define _CMU_OSCENCMD_LFRCOEN_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCOEN_DEFAULT                      (_CMU_OSCENCMD_LFRCOEN_DEFAULT << 6)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCODIS                             (0x1UL << 7)                             /**< LFRCO Disable */
#define _CMU_OSCENCMD_LFRCODIS_SHIFT                      7                                        /**< Shift value for CMU_LFRCODIS */
#define _CMU_OSCENCMD_LFRCODIS_MASK                       0x80UL                                   /**< Bit mask for CMU_LFRCODIS */
#define _CMU_OSCENCMD_LFRCODIS_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCODIS_DEFAULT                     (_CMU_OSCENCMD_LFRCODIS_DEFAULT << 7)    /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXOEN                               (0x1UL << 8)                             /**< LFXO Enable */
#define _CMU_OSCENCMD_LFXOEN_SHIFT                        8                                        /**< Shift value for CMU_LFXOEN */
#define _CMU_OSCENCMD_LFXOEN_MASK                         0x100UL                                  /**< Bit mask for CMU_LFXOEN */
#define _CMU_OSCENCMD_LFXOEN_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXOEN_DEFAULT                       (_CMU_OSCENCMD_LFXOEN_DEFAULT << 8)      /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXODIS                              (0x1UL << 9)                             /**< LFXO Disable */
#define _CMU_OSCENCMD_LFXODIS_SHIFT                       9                                        /**< Shift value for CMU_LFXODIS */
#define _CMU_OSCENCMD_LFXODIS_MASK                        0x200UL                                  /**< Bit mask for CMU_LFXODIS */
#define _CMU_OSCENCMD_LFXODIS_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXODIS_DEFAULT                      (_CMU_OSCENCMD_LFXODIS_DEFAULT << 9)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */

/* Bit fields for CMU CMD */
#define _CMU_CMD_RESETVALUE                               0x00000000UL                              /**< Default value for CMU_CMD */
#define _CMU_CMD_MASK                                     0x00000033UL                              /**< Mask for CMU_CMD */
#define CMU_CMD_CALSTART                                  (0x1UL << 0)                              /**< Calibration Start */
#define _CMU_CMD_CALSTART_SHIFT                           0                                         /**< Shift value for CMU_CALSTART */
#define _CMU_CMD_CALSTART_MASK                            0x1UL                                     /**< Bit mask for CMU_CALSTART */
#define _CMU_CMD_CALSTART_DEFAULT                         0x00000000UL                              /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTART_DEFAULT                          (_CMU_CMD_CALSTART_DEFAULT << 0)          /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTOP                                   (0x1UL << 1)                              /**< Calibration Stop */
#define _CMU_CMD_CALSTOP_SHIFT                            1                                         /**< Shift value for CMU_CALSTOP */
#define _CMU_CMD_CALSTOP_MASK                             0x2UL                                     /**< Bit mask for CMU_CALSTOP */
#define _CMU_CMD_CALSTOP_DEFAULT                          0x00000000UL                              /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTOP_DEFAULT                           (_CMU_CMD_CALSTOP_DEFAULT << 1)           /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_HFXOPEAKDETSTART                          (0x1UL << 4)                              /**< HFXO Peak Detection Start */
#define _CMU_CMD_HFXOPEAKDETSTART_SHIFT                   4                                         /**< Shift value for CMU_HFXOPEAKDETSTART */
#define _CMU_CMD_HFXOPEAKDETSTART_MASK                    0x10UL                                    /**< Bit mask for CMU_HFXOPEAKDETSTART */
#define _CMU_CMD_HFXOPEAKDETSTART_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_HFXOPEAKDETSTART_DEFAULT                  (_CMU_CMD_HFXOPEAKDETSTART_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_HFXOSHUNTOPTSTART                         (0x1UL << 5)                              /**< HFXO Shunt Current Optimization Start */
#define _CMU_CMD_HFXOSHUNTOPTSTART_SHIFT                  5                                         /**< Shift value for CMU_HFXOSHUNTOPTSTART */
#define _CMU_CMD_HFXOSHUNTOPTSTART_MASK                   0x20UL                                    /**< Bit mask for CMU_HFXOSHUNTOPTSTART */
#define _CMU_CMD_HFXOSHUNTOPTSTART_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_HFXOSHUNTOPTSTART_DEFAULT                 (_CMU_CMD_HFXOSHUNTOPTSTART_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_CMD */

/* Bit fields for CMU DBGCLKSEL */
#define _CMU_DBGCLKSEL_RESETVALUE                         0x00000000UL                       /**< Default value for CMU_DBGCLKSEL */
#define _CMU_DBGCLKSEL_MASK                               0x00000001UL                       /**< Mask for CMU_DBGCLKSEL */
#define _CMU_DBGCLKSEL_DBG_SHIFT                          0                                  /**< Shift value for CMU_DBG */
#define _CMU_DBGCLKSEL_DBG_MASK                           0x1UL                              /**< Bit mask for CMU_DBG */
#define _CMU_DBGCLKSEL_DBG_DEFAULT                        0x00000000UL                       /**< Mode DEFAULT for CMU_DBGCLKSEL */
#define _CMU_DBGCLKSEL_DBG_AUXHFRCO                       0x00000000UL                       /**< Mode AUXHFRCO for CMU_DBGCLKSEL */
#define _CMU_DBGCLKSEL_DBG_HFCLK                          0x00000001UL                       /**< Mode HFCLK for CMU_DBGCLKSEL */
#define CMU_DBGCLKSEL_DBG_DEFAULT                         (_CMU_DBGCLKSEL_DBG_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_DBGCLKSEL */
#define CMU_DBGCLKSEL_DBG_AUXHFRCO                        (_CMU_DBGCLKSEL_DBG_AUXHFRCO << 0) /**< Shifted mode AUXHFRCO for CMU_DBGCLKSEL */
#define CMU_DBGCLKSEL_DBG_HFCLK                           (_CMU_DBGCLKSEL_DBG_HFCLK << 0)    /**< Shifted mode HFCLK for CMU_DBGCLKSEL */

/* Bit fields for CMU HFCLKSEL */
#define _CMU_HFCLKSEL_RESETVALUE                          0x00000000UL                    /**< Default value for CMU_HFCLKSEL */
#define _CMU_HFCLKSEL_MASK                                0x00000007UL                    /**< Mask for CMU_HFCLKSEL */
#define _CMU_HFCLKSEL_HF_SHIFT                            0                               /**< Shift value for CMU_HF */
#define _CMU_HFCLKSEL_HF_MASK                             0x7UL                           /**< Bit mask for CMU_HF */
#define _CMU_HFCLKSEL_HF_DEFAULT                          0x00000000UL                    /**< Mode DEFAULT for CMU_HFCLKSEL */
#define _CMU_HFCLKSEL_HF_HFRCO                            0x00000001UL                    /**< Mode HFRCO for CMU_HFCLKSEL */
#define _CMU_HFCLKSEL_HF_HFXO                             0x00000002UL                    /**< Mode HFXO for CMU_HFCLKSEL */
#define _CMU_HFCLKSEL_HF_LFRCO                            0x00000003UL                    /**< Mode LFRCO for CMU_HFCLKSEL */
#define _CMU_HFCLKSEL_HF_LFXO                             0x00000004UL                    /**< Mode LFXO for CMU_HFCLKSEL */
#define CMU_HFCLKSEL_HF_DEFAULT                           (_CMU_HFCLKSEL_HF_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFCLKSEL */
#define CMU_HFCLKSEL_HF_HFRCO                             (_CMU_HFCLKSEL_HF_HFRCO << 0)   /**< Shifted mode HFRCO for CMU_HFCLKSEL */
#define CMU_HFCLKSEL_HF_HFXO                              (_CMU_HFCLKSEL_HF_HFXO << 0)    /**< Shifted mode HFXO for CMU_HFCLKSEL */
#define CMU_HFCLKSEL_HF_LFRCO                             (_CMU_HFCLKSEL_HF_LFRCO << 0)   /**< Shifted mode LFRCO for CMU_HFCLKSEL */
#define CMU_HFCLKSEL_HF_LFXO                              (_CMU_HFCLKSEL_HF_LFXO << 0)    /**< Shifted mode LFXO for CMU_HFCLKSEL */

/* Bit fields for CMU LFACLKSEL */
#define _CMU_LFACLKSEL_RESETVALUE                         0x00000000UL                       /**< Default value for CMU_LFACLKSEL */
#define _CMU_LFACLKSEL_MASK                               0x00000007UL                       /**< Mask for CMU_LFACLKSEL */
#define _CMU_LFACLKSEL_LFA_SHIFT                          0                                  /**< Shift value for CMU_LFA */
#define _CMU_LFACLKSEL_LFA_MASK                           0x7UL                              /**< Bit mask for CMU_LFA */
#define _CMU_LFACLKSEL_LFA_DEFAULT                        0x00000000UL                       /**< Mode DEFAULT for CMU_LFACLKSEL */
#define _CMU_LFACLKSEL_LFA_DISABLED                       0x00000000UL                       /**< Mode DISABLED for CMU_LFACLKSEL */
#define _CMU_LFACLKSEL_LFA_LFRCO                          0x00000001UL                       /**< Mode LFRCO for CMU_LFACLKSEL */
#define _CMU_LFACLKSEL_LFA_LFXO                           0x00000002UL                       /**< Mode LFXO for CMU_LFACLKSEL */
#define _CMU_LFACLKSEL_LFA_ULFRCO                         0x00000004UL                       /**< Mode ULFRCO for CMU_LFACLKSEL */
#define CMU_LFACLKSEL_LFA_DEFAULT                         (_CMU_LFACLKSEL_LFA_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_LFACLKSEL */
#define CMU_LFACLKSEL_LFA_DISABLED                        (_CMU_LFACLKSEL_LFA_DISABLED << 0) /**< Shifted mode DISABLED for CMU_LFACLKSEL */
#define CMU_LFACLKSEL_LFA_LFRCO                           (_CMU_LFACLKSEL_LFA_LFRCO << 0)    /**< Shifted mode LFRCO for CMU_LFACLKSEL */
#define CMU_LFACLKSEL_LFA_LFXO                            (_CMU_LFACLKSEL_LFA_LFXO << 0)     /**< Shifted mode LFXO for CMU_LFACLKSEL */
#define CMU_LFACLKSEL_LFA_ULFRCO                          (_CMU_LFACLKSEL_LFA_ULFRCO << 0)   /**< Shifted mode ULFRCO for CMU_LFACLKSEL */

/* Bit fields for CMU LFBCLKSEL */
#define _CMU_LFBCLKSEL_RESETVALUE                         0x00000000UL                       /**< Default value for CMU_LFBCLKSEL */
#define _CMU_LFBCLKSEL_MASK                               0x00000007UL                       /**< Mask for CMU_LFBCLKSEL */
#define _CMU_LFBCLKSEL_LFB_SHIFT                          0                                  /**< Shift value for CMU_LFB */
#define _CMU_LFBCLKSEL_LFB_MASK                           0x7UL                              /**< Bit mask for CMU_LFB */
#define _CMU_LFBCLKSEL_LFB_DEFAULT                        0x00000000UL                       /**< Mode DEFAULT for CMU_LFBCLKSEL */
#define _CMU_LFBCLKSEL_LFB_DISABLED                       0x00000000UL                       /**< Mode DISABLED for CMU_LFBCLKSEL */
#define _CMU_LFBCLKSEL_LFB_LFRCO                          0x00000001UL                       /**< Mode LFRCO for CMU_LFBCLKSEL */
#define _CMU_LFBCLKSEL_LFB_LFXO                           0x00000002UL                       /**< Mode LFXO for CMU_LFBCLKSEL */
#define _CMU_LFBCLKSEL_LFB_HFCLKLE                        0x00000003UL                       /**< Mode HFCLKLE for CMU_LFBCLKSEL */
#define _CMU_LFBCLKSEL_LFB_ULFRCO                         0x00000004UL                       /**< Mode ULFRCO for CMU_LFBCLKSEL */
#define CMU_LFBCLKSEL_LFB_DEFAULT                         (_CMU_LFBCLKSEL_LFB_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_LFBCLKSEL */
#define CMU_LFBCLKSEL_LFB_DISABLED                        (_CMU_LFBCLKSEL_LFB_DISABLED << 0) /**< Shifted mode DISABLED for CMU_LFBCLKSEL */
#define CMU_LFBCLKSEL_LFB_LFRCO                           (_CMU_LFBCLKSEL_LFB_LFRCO << 0)    /**< Shifted mode LFRCO for CMU_LFBCLKSEL */
#define CMU_LFBCLKSEL_LFB_LFXO                            (_CMU_LFBCLKSEL_LFB_LFXO << 0)     /**< Shifted mode LFXO for CMU_LFBCLKSEL */
#define CMU_LFBCLKSEL_LFB_HFCLKLE                         (_CMU_LFBCLKSEL_LFB_HFCLKLE << 0)  /**< Shifted mode HFCLKLE for CMU_LFBCLKSEL */
#define CMU_LFBCLKSEL_LFB_ULFRCO                          (_CMU_LFBCLKSEL_LFB_ULFRCO << 0)   /**< Shifted mode ULFRCO for CMU_LFBCLKSEL */

/* Bit fields for CMU LFECLKSEL */
#define _CMU_LFECLKSEL_RESETVALUE                         0x00000000UL                       /**< Default value for CMU_LFECLKSEL */
#define _CMU_LFECLKSEL_MASK                               0x00000007UL                       /**< Mask for CMU_LFECLKSEL */
#define _CMU_LFECLKSEL_LFE_SHIFT                          0                                  /**< Shift value for CMU_LFE */
#define _CMU_LFECLKSEL_LFE_MASK                           0x7UL                              /**< Bit mask for CMU_LFE */
#define _CMU_LFECLKSEL_LFE_DEFAULT                        0x00000000UL                       /**< Mode DEFAULT for CMU_LFECLKSEL */
#define _CMU_LFECLKSEL_LFE_DISABLED                       0x00000000UL                       /**< Mode DISABLED for CMU_LFECLKSEL */
#define _CMU_LFECLKSEL_LFE_LFRCO                          0x00000001UL                       /**< Mode LFRCO for CMU_LFECLKSEL */
#define _CMU_LFECLKSEL_LFE_LFXO                           0x00000002UL                       /**< Mode LFXO for CMU_LFECLKSEL */
#define _CMU_LFECLKSEL_LFE_ULFRCO                         0x00000004UL                       /**< Mode ULFRCO for CMU_LFECLKSEL */
#define CMU_LFECLKSEL_LFE_DEFAULT                         (_CMU_LFECLKSEL_LFE_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_LFECLKSEL */
#define CMU_LFECLKSEL_LFE_DISABLED                        (_CMU_LFECLKSEL_LFE_DISABLED << 0) /**< Shifted mode DISABLED for CMU_LFECLKSEL */
#define CMU_LFECLKSEL_LFE_LFRCO                           (_CMU_LFECLKSEL_LFE_LFRCO << 0)    /**< Shifted mode LFRCO for CMU_LFECLKSEL */
#define CMU_LFECLKSEL_LFE_LFXO                            (_CMU_LFECLKSEL_LFE_LFXO << 0)     /**< Shifted mode LFXO for CMU_LFECLKSEL */
#define CMU_LFECLKSEL_LFE_ULFRCO                          (_CMU_LFECLKSEL_LFE_ULFRCO << 0)   /**< Shifted mode ULFRCO for CMU_LFECLKSEL */

/* Bit fields for CMU STATUS */
#define _CMU_STATUS_RESETVALUE                            0x00010003UL                                /**< Default value for CMU_STATUS */
#define _CMU_STATUS_MASK                                  0x07E103FFUL                                /**< Mask for CMU_STATUS */
#define CMU_STATUS_HFRCOENS                               (0x1UL << 0)                                /**< HFRCO Enable Status */
#define _CMU_STATUS_HFRCOENS_SHIFT                        0                                           /**< Shift value for CMU_HFRCOENS */
#define _CMU_STATUS_HFRCOENS_MASK                         0x1UL                                       /**< Bit mask for CMU_HFRCOENS */
#define _CMU_STATUS_HFRCOENS_DEFAULT                      0x00000001UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOENS_DEFAULT                       (_CMU_STATUS_HFRCOENS_DEFAULT << 0)         /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCORDY                               (0x1UL << 1)                                /**< HFRCO Ready */
#define _CMU_STATUS_HFRCORDY_SHIFT                        1                                           /**< Shift value for CMU_HFRCORDY */
#define _CMU_STATUS_HFRCORDY_MASK                         0x2UL                                       /**< Bit mask for CMU_HFRCORDY */
#define _CMU_STATUS_HFRCORDY_DEFAULT                      0x00000001UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCORDY_DEFAULT                       (_CMU_STATUS_HFRCORDY_DEFAULT << 1)         /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOENS                                (0x1UL << 2)                                /**< HFXO Enable Status */
#define _CMU_STATUS_HFXOENS_SHIFT                         2                                           /**< Shift value for CMU_HFXOENS */
#define _CMU_STATUS_HFXOENS_MASK                          0x4UL                                       /**< Bit mask for CMU_HFXOENS */
#define _CMU_STATUS_HFXOENS_DEFAULT                       0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOENS_DEFAULT                        (_CMU_STATUS_HFXOENS_DEFAULT << 2)          /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXORDY                                (0x1UL << 3)                                /**< HFXO Ready */
#define _CMU_STATUS_HFXORDY_SHIFT                         3                                           /**< Shift value for CMU_HFXORDY */
#define _CMU_STATUS_HFXORDY_MASK                          0x8UL                                       /**< Bit mask for CMU_HFXORDY */
#define _CMU_STATUS_HFXORDY_DEFAULT                       0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXORDY_DEFAULT                        (_CMU_STATUS_HFXORDY_DEFAULT << 3)          /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCOENS                            (0x1UL << 4)                                /**< AUXHFRCO Enable Status */
#define _CMU_STATUS_AUXHFRCOENS_SHIFT                     4                                           /**< Shift value for CMU_AUXHFRCOENS */
#define _CMU_STATUS_AUXHFRCOENS_MASK                      0x10UL                                      /**< Bit mask for CMU_AUXHFRCOENS */
#define _CMU_STATUS_AUXHFRCOENS_DEFAULT                   0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCOENS_DEFAULT                    (_CMU_STATUS_AUXHFRCOENS_DEFAULT << 4)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCORDY                            (0x1UL << 5)                                /**< AUXHFRCO Ready */
#define _CMU_STATUS_AUXHFRCORDY_SHIFT                     5                                           /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_STATUS_AUXHFRCORDY_MASK                      0x20UL                                      /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_STATUS_AUXHFRCORDY_DEFAULT                   0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCORDY_DEFAULT                    (_CMU_STATUS_AUXHFRCORDY_DEFAULT << 5)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOENS                               (0x1UL << 6)                                /**< LFRCO Enable Status */
#define _CMU_STATUS_LFRCOENS_SHIFT                        6                                           /**< Shift value for CMU_LFRCOENS */
#define _CMU_STATUS_LFRCOENS_MASK                         0x40UL                                      /**< Bit mask for CMU_LFRCOENS */
#define _CMU_STATUS_LFRCOENS_DEFAULT                      0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOENS_DEFAULT                       (_CMU_STATUS_LFRCOENS_DEFAULT << 6)         /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCORDY                               (0x1UL << 7)                                /**< LFRCO Ready */
#define _CMU_STATUS_LFRCORDY_SHIFT                        7                                           /**< Shift value for CMU_LFRCORDY */
#define _CMU_STATUS_LFRCORDY_MASK                         0x80UL                                      /**< Bit mask for CMU_LFRCORDY */
#define _CMU_STATUS_LFRCORDY_DEFAULT                      0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCORDY_DEFAULT                       (_CMU_STATUS_LFRCORDY_DEFAULT << 7)         /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOENS                                (0x1UL << 8)                                /**< LFXO Enable Status */
#define _CMU_STATUS_LFXOENS_SHIFT                         8                                           /**< Shift value for CMU_LFXOENS */
#define _CMU_STATUS_LFXOENS_MASK                          0x100UL                                     /**< Bit mask for CMU_LFXOENS */
#define _CMU_STATUS_LFXOENS_DEFAULT                       0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOENS_DEFAULT                        (_CMU_STATUS_LFXOENS_DEFAULT << 8)          /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXORDY                                (0x1UL << 9)                                /**< LFXO Ready */
#define _CMU_STATUS_LFXORDY_SHIFT                         9                                           /**< Shift value for CMU_LFXORDY */
#define _CMU_STATUS_LFXORDY_MASK                          0x200UL                                     /**< Bit mask for CMU_LFXORDY */
#define _CMU_STATUS_LFXORDY_DEFAULT                       0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXORDY_DEFAULT                        (_CMU_STATUS_LFXORDY_DEFAULT << 9)          /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_CALRDY                                 (0x1UL << 16)                               /**< Calibration Ready */
#define _CMU_STATUS_CALRDY_SHIFT                          16                                          /**< Shift value for CMU_CALRDY */
#define _CMU_STATUS_CALRDY_MASK                           0x10000UL                                   /**< Bit mask for CMU_CALRDY */
#define _CMU_STATUS_CALRDY_DEFAULT                        0x00000001UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_CALRDY_DEFAULT                         (_CMU_STATUS_CALRDY_DEFAULT << 16)          /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOREQ                                (0x1UL << 21)                               /**< HFXO is Required by Hardware (e.g. RAC) */
#define _CMU_STATUS_HFXOREQ_SHIFT                         21                                          /**< Shift value for CMU_HFXOREQ */
#define _CMU_STATUS_HFXOREQ_MASK                          0x200000UL                                  /**< Bit mask for CMU_HFXOREQ */
#define _CMU_STATUS_HFXOREQ_DEFAULT                       0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOREQ_DEFAULT                        (_CMU_STATUS_HFXOREQ_DEFAULT << 21)         /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOPEAKDETRDY                         (0x1UL << 22)                               /**< HFXO Peak Detection Ready */
#define _CMU_STATUS_HFXOPEAKDETRDY_SHIFT                  22                                          /**< Shift value for CMU_HFXOPEAKDETRDY */
#define _CMU_STATUS_HFXOPEAKDETRDY_MASK                   0x400000UL                                  /**< Bit mask for CMU_HFXOPEAKDETRDY */
#define _CMU_STATUS_HFXOPEAKDETRDY_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOPEAKDETRDY_DEFAULT                 (_CMU_STATUS_HFXOPEAKDETRDY_DEFAULT << 22)  /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOSHUNTOPTRDY                        (0x1UL << 23)                               /**< HFXO Shunt Current Optimization ready */
#define _CMU_STATUS_HFXOSHUNTOPTRDY_SHIFT                 23                                          /**< Shift value for CMU_HFXOSHUNTOPTRDY */
#define _CMU_STATUS_HFXOSHUNTOPTRDY_MASK                  0x800000UL                                  /**< Bit mask for CMU_HFXOSHUNTOPTRDY */
#define _CMU_STATUS_HFXOSHUNTOPTRDY_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOSHUNTOPTRDY_DEFAULT                (_CMU_STATUS_HFXOSHUNTOPTRDY_DEFAULT << 23) /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOAMPHIGH                            (0x1UL << 24)                               /**< HFXO oscillation amplitude is too high */
#define _CMU_STATUS_HFXOAMPHIGH_SHIFT                     24                                          /**< Shift value for CMU_HFXOAMPHIGH */
#define _CMU_STATUS_HFXOAMPHIGH_MASK                      0x1000000UL                                 /**< Bit mask for CMU_HFXOAMPHIGH */
#define _CMU_STATUS_HFXOAMPHIGH_DEFAULT                   0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOAMPHIGH_DEFAULT                    (_CMU_STATUS_HFXOAMPHIGH_DEFAULT << 24)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOAMPLOW                             (0x1UL << 25)                               /**< HFXO amplitude tuning value too low */
#define _CMU_STATUS_HFXOAMPLOW_SHIFT                      25                                          /**< Shift value for CMU_HFXOAMPLOW */
#define _CMU_STATUS_HFXOAMPLOW_MASK                       0x2000000UL                                 /**< Bit mask for CMU_HFXOAMPLOW */
#define _CMU_STATUS_HFXOAMPLOW_DEFAULT                    0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOAMPLOW_DEFAULT                     (_CMU_STATUS_HFXOAMPLOW_DEFAULT << 25)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOREGILOW                            (0x1UL << 26)                               /**< HFXO regulator shunt current too low */
#define _CMU_STATUS_HFXOREGILOW_SHIFT                     26                                          /**< Shift value for CMU_HFXOREGILOW */
#define _CMU_STATUS_HFXOREGILOW_MASK                      0x4000000UL                                 /**< Bit mask for CMU_HFXOREGILOW */
#define _CMU_STATUS_HFXOREGILOW_DEFAULT                   0x00000000UL                                /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOREGILOW_DEFAULT                    (_CMU_STATUS_HFXOREGILOW_DEFAULT << 26)     /**< Shifted mode DEFAULT for CMU_STATUS */

/* Bit fields for CMU HFCLKSTATUS */
#define _CMU_HFCLKSTATUS_RESETVALUE                       0x00000001UL                             /**< Default value for CMU_HFCLKSTATUS */
#define _CMU_HFCLKSTATUS_MASK                             0x00000007UL                             /**< Mask for CMU_HFCLKSTATUS */
#define _CMU_HFCLKSTATUS_SELECTED_SHIFT                   0                                        /**< Shift value for CMU_SELECTED */
#define _CMU_HFCLKSTATUS_SELECTED_MASK                    0x7UL                                    /**< Bit mask for CMU_SELECTED */
#define _CMU_HFCLKSTATUS_SELECTED_DEFAULT                 0x00000001UL                             /**< Mode DEFAULT for CMU_HFCLKSTATUS */
#define _CMU_HFCLKSTATUS_SELECTED_HFRCO                   0x00000001UL                             /**< Mode HFRCO for CMU_HFCLKSTATUS */
#define _CMU_HFCLKSTATUS_SELECTED_HFXO                    0x00000002UL                             /**< Mode HFXO for CMU_HFCLKSTATUS */
#define _CMU_HFCLKSTATUS_SELECTED_LFRCO                   0x00000003UL                             /**< Mode LFRCO for CMU_HFCLKSTATUS */
#define _CMU_HFCLKSTATUS_SELECTED_LFXO                    0x00000004UL                             /**< Mode LFXO for CMU_HFCLKSTATUS */
#define CMU_HFCLKSTATUS_SELECTED_DEFAULT                  (_CMU_HFCLKSTATUS_SELECTED_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFCLKSTATUS */
#define CMU_HFCLKSTATUS_SELECTED_HFRCO                    (_CMU_HFCLKSTATUS_SELECTED_HFRCO << 0)   /**< Shifted mode HFRCO for CMU_HFCLKSTATUS */
#define CMU_HFCLKSTATUS_SELECTED_HFXO                     (_CMU_HFCLKSTATUS_SELECTED_HFXO << 0)    /**< Shifted mode HFXO for CMU_HFCLKSTATUS */
#define CMU_HFCLKSTATUS_SELECTED_LFRCO                    (_CMU_HFCLKSTATUS_SELECTED_LFRCO << 0)   /**< Shifted mode LFRCO for CMU_HFCLKSTATUS */
#define CMU_HFCLKSTATUS_SELECTED_LFXO                     (_CMU_HFCLKSTATUS_SELECTED_LFXO << 0)    /**< Shifted mode LFXO for CMU_HFCLKSTATUS */

/* Bit fields for CMU HFXOTRIMSTATUS */
#define _CMU_HFXOTRIMSTATUS_RESETVALUE                    0x00000500UL                                    /**< Default value for CMU_HFXOTRIMSTATUS */
#define _CMU_HFXOTRIMSTATUS_MASK                          0x000007FFUL                                    /**< Mask for CMU_HFXOTRIMSTATUS */
#define _CMU_HFXOTRIMSTATUS_IBTRIMXOCORE_SHIFT            0                                               /**< Shift value for CMU_IBTRIMXOCORE */
#define _CMU_HFXOTRIMSTATUS_IBTRIMXOCORE_MASK             0x7FUL                                          /**< Bit mask for CMU_IBTRIMXOCORE */
#define _CMU_HFXOTRIMSTATUS_IBTRIMXOCORE_DEFAULT          0x00000000UL                                    /**< Mode DEFAULT for CMU_HFXOTRIMSTATUS */
#define CMU_HFXOTRIMSTATUS_IBTRIMXOCORE_DEFAULT           (_CMU_HFXOTRIMSTATUS_IBTRIMXOCORE_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFXOTRIMSTATUS */
#define _CMU_HFXOTRIMSTATUS_REGISH_SHIFT                  7                                               /**< Shift value for CMU_REGISH */
#define _CMU_HFXOTRIMSTATUS_REGISH_MASK                   0x780UL                                         /**< Bit mask for CMU_REGISH */
#define _CMU_HFXOTRIMSTATUS_REGISH_DEFAULT                0x0000000AUL                                    /**< Mode DEFAULT for CMU_HFXOTRIMSTATUS */
#define CMU_HFXOTRIMSTATUS_REGISH_DEFAULT                 (_CMU_HFXOTRIMSTATUS_REGISH_DEFAULT << 7)       /**< Shifted mode DEFAULT for CMU_HFXOTRIMSTATUS */

/* Bit fields for CMU IF */
#define _CMU_IF_RESETVALUE                                0x00000001UL                            /**< Default value for CMU_IF */
#define _CMU_IF_MASK                                      0x80007F7FUL                            /**< Mask for CMU_IF */
#define CMU_IF_HFRCORDY                                   (0x1UL << 0)                            /**< HFRCO Ready Interrupt Flag */
#define _CMU_IF_HFRCORDY_SHIFT                            0                                       /**< Shift value for CMU_HFRCORDY */
#define _CMU_IF_HFRCORDY_MASK                             0x1UL                                   /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IF_HFRCORDY_DEFAULT                          0x00000001UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFRCORDY_DEFAULT                           (_CMU_IF_HFRCORDY_DEFAULT << 0)         /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXORDY                                    (0x1UL << 1)                            /**< HFXO Ready Interrupt Flag */
#define _CMU_IF_HFXORDY_SHIFT                             1                                       /**< Shift value for CMU_HFXORDY */
#define _CMU_IF_HFXORDY_MASK                              0x2UL                                   /**< Bit mask for CMU_HFXORDY */
#define _CMU_IF_HFXORDY_DEFAULT                           0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXORDY_DEFAULT                            (_CMU_IF_HFXORDY_DEFAULT << 1)          /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFRCORDY                                   (0x1UL << 2)                            /**< LFRCO Ready Interrupt Flag */
#define _CMU_IF_LFRCORDY_SHIFT                            2                                       /**< Shift value for CMU_LFRCORDY */
#define _CMU_IF_LFRCORDY_MASK                             0x4UL                                   /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IF_LFRCORDY_DEFAULT                          0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFRCORDY_DEFAULT                           (_CMU_IF_LFRCORDY_DEFAULT << 2)         /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFXORDY                                    (0x1UL << 3)                            /**< LFXO Ready Interrupt Flag */
#define _CMU_IF_LFXORDY_SHIFT                             3                                       /**< Shift value for CMU_LFXORDY */
#define _CMU_IF_LFXORDY_MASK                              0x8UL                                   /**< Bit mask for CMU_LFXORDY */
#define _CMU_IF_LFXORDY_DEFAULT                           0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFXORDY_DEFAULT                            (_CMU_IF_LFXORDY_DEFAULT << 3)          /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_AUXHFRCORDY                                (0x1UL << 4)                            /**< AUXHFRCO Ready Interrupt Flag */
#define _CMU_IF_AUXHFRCORDY_SHIFT                         4                                       /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IF_AUXHFRCORDY_MASK                          0x10UL                                  /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IF_AUXHFRCORDY_DEFAULT                       0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_AUXHFRCORDY_DEFAULT                        (_CMU_IF_AUXHFRCORDY_DEFAULT << 4)      /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CALRDY                                     (0x1UL << 5)                            /**< Calibration Ready Interrupt Flag */
#define _CMU_IF_CALRDY_SHIFT                              5                                       /**< Shift value for CMU_CALRDY */
#define _CMU_IF_CALRDY_MASK                               0x20UL                                  /**< Bit mask for CMU_CALRDY */
#define _CMU_IF_CALRDY_DEFAULT                            0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CALRDY_DEFAULT                             (_CMU_IF_CALRDY_DEFAULT << 5)           /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CALOF                                      (0x1UL << 6)                            /**< Calibration Overflow Interrupt Flag */
#define _CMU_IF_CALOF_SHIFT                               6                                       /**< Shift value for CMU_CALOF */
#define _CMU_IF_CALOF_MASK                                0x40UL                                  /**< Bit mask for CMU_CALOF */
#define _CMU_IF_CALOF_DEFAULT                             0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CALOF_DEFAULT                              (_CMU_IF_CALOF_DEFAULT << 6)            /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXODISERR                                 (0x1UL << 8)                            /**< HFXO Disable Error Interrupt Flag */
#define _CMU_IF_HFXODISERR_SHIFT                          8                                       /**< Shift value for CMU_HFXODISERR */
#define _CMU_IF_HFXODISERR_MASK                           0x100UL                                 /**< Bit mask for CMU_HFXODISERR */
#define _CMU_IF_HFXODISERR_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXODISERR_DEFAULT                         (_CMU_IF_HFXODISERR_DEFAULT << 8)       /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXOAUTOSW                                 (0x1UL << 9)                            /**< HFXO Automatic Switch Interrupt Flag */
#define _CMU_IF_HFXOAUTOSW_SHIFT                          9                                       /**< Shift value for CMU_HFXOAUTOSW */
#define _CMU_IF_HFXOAUTOSW_MASK                           0x200UL                                 /**< Bit mask for CMU_HFXOAUTOSW */
#define _CMU_IF_HFXOAUTOSW_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXOAUTOSW_DEFAULT                         (_CMU_IF_HFXOAUTOSW_DEFAULT << 9)       /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXOPEAKDETERR                             (0x1UL << 10)                           /**< HFXO Automatic Peak Detection Error Interrupt Flag */
#define _CMU_IF_HFXOPEAKDETERR_SHIFT                      10                                      /**< Shift value for CMU_HFXOPEAKDETERR */
#define _CMU_IF_HFXOPEAKDETERR_MASK                       0x400UL                                 /**< Bit mask for CMU_HFXOPEAKDETERR */
#define _CMU_IF_HFXOPEAKDETERR_DEFAULT                    0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXOPEAKDETERR_DEFAULT                     (_CMU_IF_HFXOPEAKDETERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXOPEAKDETRDY                             (0x1UL << 11)                           /**< HFXO Automatic Peak Detection Ready Interrupt Flag */
#define _CMU_IF_HFXOPEAKDETRDY_SHIFT                      11                                      /**< Shift value for CMU_HFXOPEAKDETRDY */
#define _CMU_IF_HFXOPEAKDETRDY_MASK                       0x800UL                                 /**< Bit mask for CMU_HFXOPEAKDETRDY */
#define _CMU_IF_HFXOPEAKDETRDY_DEFAULT                    0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXOPEAKDETRDY_DEFAULT                     (_CMU_IF_HFXOPEAKDETRDY_DEFAULT << 11)  /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXOSHUNTOPTRDY                            (0x1UL << 12)                           /**< HFXO Automatic Shunt Current Optimization Ready Interrupt Flag */
#define _CMU_IF_HFXOSHUNTOPTRDY_SHIFT                     12                                      /**< Shift value for CMU_HFXOSHUNTOPTRDY */
#define _CMU_IF_HFXOSHUNTOPTRDY_MASK                      0x1000UL                                /**< Bit mask for CMU_HFXOSHUNTOPTRDY */
#define _CMU_IF_HFXOSHUNTOPTRDY_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXOSHUNTOPTRDY_DEFAULT                    (_CMU_IF_HFXOSHUNTOPTRDY_DEFAULT << 12) /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFRCODIS                                   (0x1UL << 13)                           /**< HFRCO Disable Interrupt Flag */
#define _CMU_IF_HFRCODIS_SHIFT                            13                                      /**< Shift value for CMU_HFRCODIS */
#define _CMU_IF_HFRCODIS_MASK                             0x2000UL                                /**< Bit mask for CMU_HFRCODIS */
#define _CMU_IF_HFRCODIS_DEFAULT                          0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFRCODIS_DEFAULT                           (_CMU_IF_HFRCODIS_DEFAULT << 13)        /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFTIMEOUTERR                               (0x1UL << 14)                           /**< Low Frequency Timeout Error Interrupt Flag */
#define _CMU_IF_LFTIMEOUTERR_SHIFT                        14                                      /**< Shift value for CMU_LFTIMEOUTERR */
#define _CMU_IF_LFTIMEOUTERR_MASK                         0x4000UL                                /**< Bit mask for CMU_LFTIMEOUTERR */
#define _CMU_IF_LFTIMEOUTERR_DEFAULT                      0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFTIMEOUTERR_DEFAULT                       (_CMU_IF_LFTIMEOUTERR_DEFAULT << 14)    /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CMUERR                                     (0x1UL << 31)                           /**< CMU Error Interrupt Flag */
#define _CMU_IF_CMUERR_SHIFT                              31                                      /**< Shift value for CMU_CMUERR */
#define _CMU_IF_CMUERR_MASK                               0x80000000UL                            /**< Bit mask for CMU_CMUERR */
#define _CMU_IF_CMUERR_DEFAULT                            0x00000000UL                            /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CMUERR_DEFAULT                             (_CMU_IF_CMUERR_DEFAULT << 31)          /**< Shifted mode DEFAULT for CMU_IF */

/* Bit fields for CMU IFS */
#define _CMU_IFS_RESETVALUE                               0x00000000UL                             /**< Default value for CMU_IFS */
#define _CMU_IFS_MASK                                     0x80007F7FUL                             /**< Mask for CMU_IFS */
#define CMU_IFS_HFRCORDY                                  (0x1UL << 0)                             /**< Set HFRCORDY Interrupt Flag */
#define _CMU_IFS_HFRCORDY_SHIFT                           0                                        /**< Shift value for CMU_HFRCORDY */
#define _CMU_IFS_HFRCORDY_MASK                            0x1UL                                    /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IFS_HFRCORDY_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFRCORDY_DEFAULT                          (_CMU_IFS_HFRCORDY_DEFAULT << 0)         /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXORDY                                   (0x1UL << 1)                             /**< Set HFXORDY Interrupt Flag */
#define _CMU_IFS_HFXORDY_SHIFT                            1                                        /**< Shift value for CMU_HFXORDY */
#define _CMU_IFS_HFXORDY_MASK                             0x2UL                                    /**< Bit mask for CMU_HFXORDY */
#define _CMU_IFS_HFXORDY_DEFAULT                          0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXORDY_DEFAULT                           (_CMU_IFS_HFXORDY_DEFAULT << 1)          /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFRCORDY                                  (0x1UL << 2)                             /**< Set LFRCORDY Interrupt Flag */
#define _CMU_IFS_LFRCORDY_SHIFT                           2                                        /**< Shift value for CMU_LFRCORDY */
#define _CMU_IFS_LFRCORDY_MASK                            0x4UL                                    /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IFS_LFRCORDY_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFRCORDY_DEFAULT                          (_CMU_IFS_LFRCORDY_DEFAULT << 2)         /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFXORDY                                   (0x1UL << 3)                             /**< Set LFXORDY Interrupt Flag */
#define _CMU_IFS_LFXORDY_SHIFT                            3                                        /**< Shift value for CMU_LFXORDY */
#define _CMU_IFS_LFXORDY_MASK                             0x8UL                                    /**< Bit mask for CMU_LFXORDY */
#define _CMU_IFS_LFXORDY_DEFAULT                          0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFXORDY_DEFAULT                           (_CMU_IFS_LFXORDY_DEFAULT << 3)          /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_AUXHFRCORDY                               (0x1UL << 4)                             /**< Set AUXHFRCORDY Interrupt Flag */
#define _CMU_IFS_AUXHFRCORDY_SHIFT                        4                                        /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IFS_AUXHFRCORDY_MASK                         0x10UL                                   /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IFS_AUXHFRCORDY_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_AUXHFRCORDY_DEFAULT                       (_CMU_IFS_AUXHFRCORDY_DEFAULT << 4)      /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALRDY                                    (0x1UL << 5)                             /**< Set CALRDY Interrupt Flag */
#define _CMU_IFS_CALRDY_SHIFT                             5                                        /**< Shift value for CMU_CALRDY */
#define _CMU_IFS_CALRDY_MASK                              0x20UL                                   /**< Bit mask for CMU_CALRDY */
#define _CMU_IFS_CALRDY_DEFAULT                           0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALRDY_DEFAULT                            (_CMU_IFS_CALRDY_DEFAULT << 5)           /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALOF                                     (0x1UL << 6)                             /**< Set CALOF Interrupt Flag */
#define _CMU_IFS_CALOF_SHIFT                              6                                        /**< Shift value for CMU_CALOF */
#define _CMU_IFS_CALOF_MASK                               0x40UL                                   /**< Bit mask for CMU_CALOF */
#define _CMU_IFS_CALOF_DEFAULT                            0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALOF_DEFAULT                             (_CMU_IFS_CALOF_DEFAULT << 6)            /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXODISERR                                (0x1UL << 8)                             /**< Set HFXODISERR Interrupt Flag */
#define _CMU_IFS_HFXODISERR_SHIFT                         8                                        /**< Shift value for CMU_HFXODISERR */
#define _CMU_IFS_HFXODISERR_MASK                          0x100UL                                  /**< Bit mask for CMU_HFXODISERR */
#define _CMU_IFS_HFXODISERR_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXODISERR_DEFAULT                        (_CMU_IFS_HFXODISERR_DEFAULT << 8)       /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXOAUTOSW                                (0x1UL << 9)                             /**< Set HFXOAUTOSW Interrupt Flag */
#define _CMU_IFS_HFXOAUTOSW_SHIFT                         9                                        /**< Shift value for CMU_HFXOAUTOSW */
#define _CMU_IFS_HFXOAUTOSW_MASK                          0x200UL                                  /**< Bit mask for CMU_HFXOAUTOSW */
#define _CMU_IFS_HFXOAUTOSW_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXOAUTOSW_DEFAULT                        (_CMU_IFS_HFXOAUTOSW_DEFAULT << 9)       /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXOPEAKDETERR                            (0x1UL << 10)                            /**< Set HFXOPEAKDETERR Interrupt Flag */
#define _CMU_IFS_HFXOPEAKDETERR_SHIFT                     10                                       /**< Shift value for CMU_HFXOPEAKDETERR */
#define _CMU_IFS_HFXOPEAKDETERR_MASK                      0x400UL                                  /**< Bit mask for CMU_HFXOPEAKDETERR */
#define _CMU_IFS_HFXOPEAKDETERR_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXOPEAKDETERR_DEFAULT                    (_CMU_IFS_HFXOPEAKDETERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXOPEAKDETRDY                            (0x1UL << 11)                            /**< Set HFXOPEAKDETRDY Interrupt Flag */
#define _CMU_IFS_HFXOPEAKDETRDY_SHIFT                     11                                       /**< Shift value for CMU_HFXOPEAKDETRDY */
#define _CMU_IFS_HFXOPEAKDETRDY_MASK                      0x800UL                                  /**< Bit mask for CMU_HFXOPEAKDETRDY */
#define _CMU_IFS_HFXOPEAKDETRDY_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXOPEAKDETRDY_DEFAULT                    (_CMU_IFS_HFXOPEAKDETRDY_DEFAULT << 11)  /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXOSHUNTOPTRDY                           (0x1UL << 12)                            /**< Set HFXOSHUNTOPTRDY Interrupt Flag */
#define _CMU_IFS_HFXOSHUNTOPTRDY_SHIFT                    12                                       /**< Shift value for CMU_HFXOSHUNTOPTRDY */
#define _CMU_IFS_HFXOSHUNTOPTRDY_MASK                     0x1000UL                                 /**< Bit mask for CMU_HFXOSHUNTOPTRDY */
#define _CMU_IFS_HFXOSHUNTOPTRDY_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXOSHUNTOPTRDY_DEFAULT                   (_CMU_IFS_HFXOSHUNTOPTRDY_DEFAULT << 12) /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFRCODIS                                  (0x1UL << 13)                            /**< Set HFRCODIS Interrupt Flag */
#define _CMU_IFS_HFRCODIS_SHIFT                           13                                       /**< Shift value for CMU_HFRCODIS */
#define _CMU_IFS_HFRCODIS_MASK                            0x2000UL                                 /**< Bit mask for CMU_HFRCODIS */
#define _CMU_IFS_HFRCODIS_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFRCODIS_DEFAULT                          (_CMU_IFS_HFRCODIS_DEFAULT << 13)        /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFTIMEOUTERR                              (0x1UL << 14)                            /**< Set LFTIMEOUTERR Interrupt Flag */
#define _CMU_IFS_LFTIMEOUTERR_SHIFT                       14                                       /**< Shift value for CMU_LFTIMEOUTERR */
#define _CMU_IFS_LFTIMEOUTERR_MASK                        0x4000UL                                 /**< Bit mask for CMU_LFTIMEOUTERR */
#define _CMU_IFS_LFTIMEOUTERR_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFTIMEOUTERR_DEFAULT                      (_CMU_IFS_LFTIMEOUTERR_DEFAULT << 14)    /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CMUERR                                    (0x1UL << 31)                            /**< Set CMUERR Interrupt Flag */
#define _CMU_IFS_CMUERR_SHIFT                             31                                       /**< Shift value for CMU_CMUERR */
#define _CMU_IFS_CMUERR_MASK                              0x80000000UL                             /**< Bit mask for CMU_CMUERR */
#define _CMU_IFS_CMUERR_DEFAULT                           0x00000000UL                             /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CMUERR_DEFAULT                            (_CMU_IFS_CMUERR_DEFAULT << 31)          /**< Shifted mode DEFAULT for CMU_IFS */

/* Bit fields for CMU IFC */
#define _CMU_IFC_RESETVALUE                               0x00000000UL                             /**< Default value for CMU_IFC */
#define _CMU_IFC_MASK                                     0x80007F7FUL                             /**< Mask for CMU_IFC */
#define CMU_IFC_HFRCORDY                                  (0x1UL << 0)                             /**< Clear HFRCORDY Interrupt Flag */
#define _CMU_IFC_HFRCORDY_SHIFT                           0                                        /**< Shift value for CMU_HFRCORDY */
#define _CMU_IFC_HFRCORDY_MASK                            0x1UL                                    /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IFC_HFRCORDY_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFRCORDY_DEFAULT                          (_CMU_IFC_HFRCORDY_DEFAULT << 0)         /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXORDY                                   (0x1UL << 1)                             /**< Clear HFXORDY Interrupt Flag */
#define _CMU_IFC_HFXORDY_SHIFT                            1                                        /**< Shift value for CMU_HFXORDY */
#define _CMU_IFC_HFXORDY_MASK                             0x2UL                                    /**< Bit mask for CMU_HFXORDY */
#define _CMU_IFC_HFXORDY_DEFAULT                          0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXORDY_DEFAULT                           (_CMU_IFC_HFXORDY_DEFAULT << 1)          /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFRCORDY                                  (0x1UL << 2)                             /**< Clear LFRCORDY Interrupt Flag */
#define _CMU_IFC_LFRCORDY_SHIFT                           2                                        /**< Shift value for CMU_LFRCORDY */
#define _CMU_IFC_LFRCORDY_MASK                            0x4UL                                    /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IFC_LFRCORDY_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFRCORDY_DEFAULT                          (_CMU_IFC_LFRCORDY_DEFAULT << 2)         /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFXORDY                                   (0x1UL << 3)                             /**< Clear LFXORDY Interrupt Flag */
#define _CMU_IFC_LFXORDY_SHIFT                            3                                        /**< Shift value for CMU_LFXORDY */
#define _CMU_IFC_LFXORDY_MASK                             0x8UL                                    /**< Bit mask for CMU_LFXORDY */
#define _CMU_IFC_LFXORDY_DEFAULT                          0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFXORDY_DEFAULT                           (_CMU_IFC_LFXORDY_DEFAULT << 3)          /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_AUXHFRCORDY                               (0x1UL << 4)                             /**< Clear AUXHFRCORDY Interrupt Flag */
#define _CMU_IFC_AUXHFRCORDY_SHIFT                        4                                        /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IFC_AUXHFRCORDY_MASK                         0x10UL                                   /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IFC_AUXHFRCORDY_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_AUXHFRCORDY_DEFAULT                       (_CMU_IFC_AUXHFRCORDY_DEFAULT << 4)      /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALRDY                                    (0x1UL << 5)                             /**< Clear CALRDY Interrupt Flag */
#define _CMU_IFC_CALRDY_SHIFT                             5                                        /**< Shift value for CMU_CALRDY */
#define _CMU_IFC_CALRDY_MASK                              0x20UL                                   /**< Bit mask for CMU_CALRDY */
#define _CMU_IFC_CALRDY_DEFAULT                           0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALRDY_DEFAULT                            (_CMU_IFC_CALRDY_DEFAULT << 5)           /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALOF                                     (0x1UL << 6)                             /**< Clear CALOF Interrupt Flag */
#define _CMU_IFC_CALOF_SHIFT                              6                                        /**< Shift value for CMU_CALOF */
#define _CMU_IFC_CALOF_MASK                               0x40UL                                   /**< Bit mask for CMU_CALOF */
#define _CMU_IFC_CALOF_DEFAULT                            0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALOF_DEFAULT                             (_CMU_IFC_CALOF_DEFAULT << 6)            /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXODISERR                                (0x1UL << 8)                             /**< Clear HFXODISERR Interrupt Flag */
#define _CMU_IFC_HFXODISERR_SHIFT                         8                                        /**< Shift value for CMU_HFXODISERR */
#define _CMU_IFC_HFXODISERR_MASK                          0x100UL                                  /**< Bit mask for CMU_HFXODISERR */
#define _CMU_IFC_HFXODISERR_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXODISERR_DEFAULT                        (_CMU_IFC_HFXODISERR_DEFAULT << 8)       /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXOAUTOSW                                (0x1UL << 9)                             /**< Clear HFXOAUTOSW Interrupt Flag */
#define _CMU_IFC_HFXOAUTOSW_SHIFT                         9                                        /**< Shift value for CMU_HFXOAUTOSW */
#define _CMU_IFC_HFXOAUTOSW_MASK                          0x200UL                                  /**< Bit mask for CMU_HFXOAUTOSW */
#define _CMU_IFC_HFXOAUTOSW_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXOAUTOSW_DEFAULT                        (_CMU_IFC_HFXOAUTOSW_DEFAULT << 9)       /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXOPEAKDETERR                            (0x1UL << 10)                            /**< Clear HFXOPEAKDETERR Interrupt Flag */
#define _CMU_IFC_HFXOPEAKDETERR_SHIFT                     10                                       /**< Shift value for CMU_HFXOPEAKDETERR */
#define _CMU_IFC_HFXOPEAKDETERR_MASK                      0x400UL                                  /**< Bit mask for CMU_HFXOPEAKDETERR */
#define _CMU_IFC_HFXOPEAKDETERR_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXOPEAKDETERR_DEFAULT                    (_CMU_IFC_HFXOPEAKDETERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXOPEAKDETRDY                            (0x1UL << 11)                            /**< Clear HFXOPEAKDETRDY Interrupt Flag */
#define _CMU_IFC_HFXOPEAKDETRDY_SHIFT                     11                                       /**< Shift value for CMU_HFXOPEAKDETRDY */
#define _CMU_IFC_HFXOPEAKDETRDY_MASK                      0x800UL                                  /**< Bit mask for CMU_HFXOPEAKDETRDY */
#define _CMU_IFC_HFXOPEAKDETRDY_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXOPEAKDETRDY_DEFAULT                    (_CMU_IFC_HFXOPEAKDETRDY_DEFAULT << 11)  /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXOSHUNTOPTRDY                           (0x1UL << 12)                            /**< Clear HFXOSHUNTOPTRDY Interrupt Flag */
#define _CMU_IFC_HFXOSHUNTOPTRDY_SHIFT                    12                                       /**< Shift value for CMU_HFXOSHUNTOPTRDY */
#define _CMU_IFC_HFXOSHUNTOPTRDY_MASK                     0x1000UL                                 /**< Bit mask for CMU_HFXOSHUNTOPTRDY */
#define _CMU_IFC_HFXOSHUNTOPTRDY_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXOSHUNTOPTRDY_DEFAULT                   (_CMU_IFC_HFXOSHUNTOPTRDY_DEFAULT << 12) /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFRCODIS                                  (0x1UL << 13)                            /**< Clear HFRCODIS Interrupt Flag */
#define _CMU_IFC_HFRCODIS_SHIFT                           13                                       /**< Shift value for CMU_HFRCODIS */
#define _CMU_IFC_HFRCODIS_MASK                            0x2000UL                                 /**< Bit mask for CMU_HFRCODIS */
#define _CMU_IFC_HFRCODIS_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFRCODIS_DEFAULT                          (_CMU_IFC_HFRCODIS_DEFAULT << 13)        /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFTIMEOUTERR                              (0x1UL << 14)                            /**< Clear LFTIMEOUTERR Interrupt Flag */
#define _CMU_IFC_LFTIMEOUTERR_SHIFT                       14                                       /**< Shift value for CMU_LFTIMEOUTERR */
#define _CMU_IFC_LFTIMEOUTERR_MASK                        0x4000UL                                 /**< Bit mask for CMU_LFTIMEOUTERR */
#define _CMU_IFC_LFTIMEOUTERR_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFTIMEOUTERR_DEFAULT                      (_CMU_IFC_LFTIMEOUTERR_DEFAULT << 14)    /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CMUERR                                    (0x1UL << 31)                            /**< Clear CMUERR Interrupt Flag */
#define _CMU_IFC_CMUERR_SHIFT                             31                                       /**< Shift value for CMU_CMUERR */
#define _CMU_IFC_CMUERR_MASK                              0x80000000UL                             /**< Bit mask for CMU_CMUERR */
#define _CMU_IFC_CMUERR_DEFAULT                           0x00000000UL                             /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CMUERR_DEFAULT                            (_CMU_IFC_CMUERR_DEFAULT << 31)          /**< Shifted mode DEFAULT for CMU_IFC */

/* Bit fields for CMU IEN */
#define _CMU_IEN_RESETVALUE                               0x00000000UL                             /**< Default value for CMU_IEN */
#define _CMU_IEN_MASK                                     0x80007F7FUL                             /**< Mask for CMU_IEN */
#define CMU_IEN_HFRCORDY                                  (0x1UL << 0)                             /**< HFRCORDY Interrupt Enable */
#define _CMU_IEN_HFRCORDY_SHIFT                           0                                        /**< Shift value for CMU_HFRCORDY */
#define _CMU_IEN_HFRCORDY_MASK                            0x1UL                                    /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IEN_HFRCORDY_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFRCORDY_DEFAULT                          (_CMU_IEN_HFRCORDY_DEFAULT << 0)         /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXORDY                                   (0x1UL << 1)                             /**< HFXORDY Interrupt Enable */
#define _CMU_IEN_HFXORDY_SHIFT                            1                                        /**< Shift value for CMU_HFXORDY */
#define _CMU_IEN_HFXORDY_MASK                             0x2UL                                    /**< Bit mask for CMU_HFXORDY */
#define _CMU_IEN_HFXORDY_DEFAULT                          0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXORDY_DEFAULT                           (_CMU_IEN_HFXORDY_DEFAULT << 1)          /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFRCORDY                                  (0x1UL << 2)                             /**< LFRCORDY Interrupt Enable */
#define _CMU_IEN_LFRCORDY_SHIFT                           2                                        /**< Shift value for CMU_LFRCORDY */
#define _CMU_IEN_LFRCORDY_MASK                            0x4UL                                    /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IEN_LFRCORDY_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFRCORDY_DEFAULT                          (_CMU_IEN_LFRCORDY_DEFAULT << 2)         /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFXORDY                                   (0x1UL << 3)                             /**< LFXORDY Interrupt Enable */
#define _CMU_IEN_LFXORDY_SHIFT                            3                                        /**< Shift value for CMU_LFXORDY */
#define _CMU_IEN_LFXORDY_MASK                             0x8UL                                    /**< Bit mask for CMU_LFXORDY */
#define _CMU_IEN_LFXORDY_DEFAULT                          0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFXORDY_DEFAULT                           (_CMU_IEN_LFXORDY_DEFAULT << 3)          /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_AUXHFRCORDY                               (0x1UL << 4)                             /**< AUXHFRCORDY Interrupt Enable */
#define _CMU_IEN_AUXHFRCORDY_SHIFT                        4                                        /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IEN_AUXHFRCORDY_MASK                         0x10UL                                   /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IEN_AUXHFRCORDY_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_AUXHFRCORDY_DEFAULT                       (_CMU_IEN_AUXHFRCORDY_DEFAULT << 4)      /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALRDY                                    (0x1UL << 5)                             /**< CALRDY Interrupt Enable */
#define _CMU_IEN_CALRDY_SHIFT                             5                                        /**< Shift value for CMU_CALRDY */
#define _CMU_IEN_CALRDY_MASK                              0x20UL                                   /**< Bit mask for CMU_CALRDY */
#define _CMU_IEN_CALRDY_DEFAULT                           0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALRDY_DEFAULT                            (_CMU_IEN_CALRDY_DEFAULT << 5)           /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALOF                                     (0x1UL << 6)                             /**< CALOF Interrupt Enable */
#define _CMU_IEN_CALOF_SHIFT                              6                                        /**< Shift value for CMU_CALOF */
#define _CMU_IEN_CALOF_MASK                               0x40UL                                   /**< Bit mask for CMU_CALOF */
#define _CMU_IEN_CALOF_DEFAULT                            0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALOF_DEFAULT                             (_CMU_IEN_CALOF_DEFAULT << 6)            /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXODISERR                                (0x1UL << 8)                             /**< HFXODISERR Interrupt Enable */
#define _CMU_IEN_HFXODISERR_SHIFT                         8                                        /**< Shift value for CMU_HFXODISERR */
#define _CMU_IEN_HFXODISERR_MASK                          0x100UL                                  /**< Bit mask for CMU_HFXODISERR */
#define _CMU_IEN_HFXODISERR_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXODISERR_DEFAULT                        (_CMU_IEN_HFXODISERR_DEFAULT << 8)       /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXOAUTOSW                                (0x1UL << 9)                             /**< HFXOAUTOSW Interrupt Enable */
#define _CMU_IEN_HFXOAUTOSW_SHIFT                         9                                        /**< Shift value for CMU_HFXOAUTOSW */
#define _CMU_IEN_HFXOAUTOSW_MASK                          0x200UL                                  /**< Bit mask for CMU_HFXOAUTOSW */
#define _CMU_IEN_HFXOAUTOSW_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXOAUTOSW_DEFAULT                        (_CMU_IEN_HFXOAUTOSW_DEFAULT << 9)       /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXOPEAKDETERR                            (0x1UL << 10)                            /**< HFXOPEAKDETERR Interrupt Enable */
#define _CMU_IEN_HFXOPEAKDETERR_SHIFT                     10                                       /**< Shift value for CMU_HFXOPEAKDETERR */
#define _CMU_IEN_HFXOPEAKDETERR_MASK                      0x400UL                                  /**< Bit mask for CMU_HFXOPEAKDETERR */
#define _CMU_IEN_HFXOPEAKDETERR_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXOPEAKDETERR_DEFAULT                    (_CMU_IEN_HFXOPEAKDETERR_DEFAULT << 10)  /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXOPEAKDETRDY                            (0x1UL << 11)                            /**< HFXOPEAKDETRDY Interrupt Enable */
#define _CMU_IEN_HFXOPEAKDETRDY_SHIFT                     11                                       /**< Shift value for CMU_HFXOPEAKDETRDY */
#define _CMU_IEN_HFXOPEAKDETRDY_MASK                      0x800UL                                  /**< Bit mask for CMU_HFXOPEAKDETRDY */
#define _CMU_IEN_HFXOPEAKDETRDY_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXOPEAKDETRDY_DEFAULT                    (_CMU_IEN_HFXOPEAKDETRDY_DEFAULT << 11)  /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXOSHUNTOPTRDY                           (0x1UL << 12)                            /**< HFXOSHUNTOPTRDY Interrupt Enable */
#define _CMU_IEN_HFXOSHUNTOPTRDY_SHIFT                    12                                       /**< Shift value for CMU_HFXOSHUNTOPTRDY */
#define _CMU_IEN_HFXOSHUNTOPTRDY_MASK                     0x1000UL                                 /**< Bit mask for CMU_HFXOSHUNTOPTRDY */
#define _CMU_IEN_HFXOSHUNTOPTRDY_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXOSHUNTOPTRDY_DEFAULT                   (_CMU_IEN_HFXOSHUNTOPTRDY_DEFAULT << 12) /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFRCODIS                                  (0x1UL << 13)                            /**< HFRCODIS Interrupt Enable */
#define _CMU_IEN_HFRCODIS_SHIFT                           13                                       /**< Shift value for CMU_HFRCODIS */
#define _CMU_IEN_HFRCODIS_MASK                            0x2000UL                                 /**< Bit mask for CMU_HFRCODIS */
#define _CMU_IEN_HFRCODIS_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFRCODIS_DEFAULT                          (_CMU_IEN_HFRCODIS_DEFAULT << 13)        /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFTIMEOUTERR                              (0x1UL << 14)                            /**< LFTIMEOUTERR Interrupt Enable */
#define _CMU_IEN_LFTIMEOUTERR_SHIFT                       14                                       /**< Shift value for CMU_LFTIMEOUTERR */
#define _CMU_IEN_LFTIMEOUTERR_MASK                        0x4000UL                                 /**< Bit mask for CMU_LFTIMEOUTERR */
#define _CMU_IEN_LFTIMEOUTERR_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFTIMEOUTERR_DEFAULT                      (_CMU_IEN_LFTIMEOUTERR_DEFAULT << 14)    /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CMUERR                                    (0x1UL << 31)                            /**< CMUERR Interrupt Enable */
#define _CMU_IEN_CMUERR_SHIFT                             31                                       /**< Shift value for CMU_CMUERR */
#define _CMU_IEN_CMUERR_MASK                              0x80000000UL                             /**< Bit mask for CMU_CMUERR */
#define _CMU_IEN_CMUERR_DEFAULT                           0x00000000UL                             /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CMUERR_DEFAULT                            (_CMU_IEN_CMUERR_DEFAULT << 31)          /**< Shifted mode DEFAULT for CMU_IEN */

/* Bit fields for CMU HFBUSCLKEN0 */
#define _CMU_HFBUSCLKEN0_RESETVALUE                       0x00000000UL                           /**< Default value for CMU_HFBUSCLKEN0 */
#define _CMU_HFBUSCLKEN0_MASK                             0x0000003FUL                           /**< Mask for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_LE                                (0x1UL << 0)                           /**< Low Energy Peripheral Interface Clock Enable */
#define _CMU_HFBUSCLKEN0_LE_SHIFT                         0                                      /**< Shift value for CMU_LE */
#define _CMU_HFBUSCLKEN0_LE_MASK                          0x1UL                                  /**< Bit mask for CMU_LE */
#define _CMU_HFBUSCLKEN0_LE_DEFAULT                       0x00000000UL                           /**< Mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_LE_DEFAULT                        (_CMU_HFBUSCLKEN0_LE_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_CRYPTO                            (0x1UL << 1)                           /**< Advanced Encryption Standard Accelerator Clock Enable */
#define _CMU_HFBUSCLKEN0_CRYPTO_SHIFT                     1                                      /**< Shift value for CMU_CRYPTO */
#define _CMU_HFBUSCLKEN0_CRYPTO_MASK                      0x2UL                                  /**< Bit mask for CMU_CRYPTO */
#define _CMU_HFBUSCLKEN0_CRYPTO_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_CRYPTO_DEFAULT                    (_CMU_HFBUSCLKEN0_CRYPTO_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_GPIO                              (0x1UL << 2)                           /**< General purpose Input/Output Clock Enable */
#define _CMU_HFBUSCLKEN0_GPIO_SHIFT                       2                                      /**< Shift value for CMU_GPIO */
#define _CMU_HFBUSCLKEN0_GPIO_MASK                        0x4UL                                  /**< Bit mask for CMU_GPIO */
#define _CMU_HFBUSCLKEN0_GPIO_DEFAULT                     0x00000000UL                           /**< Mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_GPIO_DEFAULT                      (_CMU_HFBUSCLKEN0_GPIO_DEFAULT << 2)   /**< Shifted mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_PRS                               (0x1UL << 3)                           /**< Peripheral Reflex System Clock Enable */
#define _CMU_HFBUSCLKEN0_PRS_SHIFT                        3                                      /**< Shift value for CMU_PRS */
#define _CMU_HFBUSCLKEN0_PRS_MASK                         0x8UL                                  /**< Bit mask for CMU_PRS */
#define _CMU_HFBUSCLKEN0_PRS_DEFAULT                      0x00000000UL                           /**< Mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_PRS_DEFAULT                       (_CMU_HFBUSCLKEN0_PRS_DEFAULT << 3)    /**< Shifted mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_LDMA                              (0x1UL << 4)                           /**< Linked Direct Memory Access Controller Clock Enable */
#define _CMU_HFBUSCLKEN0_LDMA_SHIFT                       4                                      /**< Shift value for CMU_LDMA */
#define _CMU_HFBUSCLKEN0_LDMA_MASK                        0x10UL                                 /**< Bit mask for CMU_LDMA */
#define _CMU_HFBUSCLKEN0_LDMA_DEFAULT                     0x00000000UL                           /**< Mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_LDMA_DEFAULT                      (_CMU_HFBUSCLKEN0_LDMA_DEFAULT << 4)   /**< Shifted mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_GPCRC                             (0x1UL << 5)                           /**< General Purpose CRC Clock Enable */
#define _CMU_HFBUSCLKEN0_GPCRC_SHIFT                      5                                      /**< Shift value for CMU_GPCRC */
#define _CMU_HFBUSCLKEN0_GPCRC_MASK                       0x20UL                                 /**< Bit mask for CMU_GPCRC */
#define _CMU_HFBUSCLKEN0_GPCRC_DEFAULT                    0x00000000UL                           /**< Mode DEFAULT for CMU_HFBUSCLKEN0 */
#define CMU_HFBUSCLKEN0_GPCRC_DEFAULT                     (_CMU_HFBUSCLKEN0_GPCRC_DEFAULT << 5)  /**< Shifted mode DEFAULT for CMU_HFBUSCLKEN0 */

/* Bit fields for CMU HFPERCLKEN0 */
#define _CMU_HFPERCLKEN0_RESETVALUE                       0x00000000UL                              /**< Default value for CMU_HFPERCLKEN0 */
#define _CMU_HFPERCLKEN0_MASK                             0x000003FFUL                              /**< Mask for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER0                            (0x1UL << 0)                              /**< Timer 0 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER0_SHIFT                     0                                         /**< Shift value for CMU_TIMER0 */
#define _CMU_HFPERCLKEN0_TIMER0_MASK                      0x1UL                                     /**< Bit mask for CMU_TIMER0 */
#define _CMU_HFPERCLKEN0_TIMER0_DEFAULT                   0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER0_DEFAULT                    (_CMU_HFPERCLKEN0_TIMER0_DEFAULT << 0)    /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER1                            (0x1UL << 1)                              /**< Timer 1 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER1_SHIFT                     1                                         /**< Shift value for CMU_TIMER1 */
#define _CMU_HFPERCLKEN0_TIMER1_MASK                      0x2UL                                     /**< Bit mask for CMU_TIMER1 */
#define _CMU_HFPERCLKEN0_TIMER1_DEFAULT                   0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER1_DEFAULT                    (_CMU_HFPERCLKEN0_TIMER1_DEFAULT << 1)    /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART0                            (0x1UL << 2)                              /**< Universal Synchronous/Asynchronous Receiver/Transmitter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_USART0_SHIFT                     2                                         /**< Shift value for CMU_USART0 */
#define _CMU_HFPERCLKEN0_USART0_MASK                      0x4UL                                     /**< Bit mask for CMU_USART0 */
#define _CMU_HFPERCLKEN0_USART0_DEFAULT                   0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART0_DEFAULT                    (_CMU_HFPERCLKEN0_USART0_DEFAULT << 2)    /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART1                            (0x1UL << 3)                              /**< Universal Synchronous/Asynchronous Receiver/Transmitter 1 Clock Enable */
#define _CMU_HFPERCLKEN0_USART1_SHIFT                     3                                         /**< Shift value for CMU_USART1 */
#define _CMU_HFPERCLKEN0_USART1_MASK                      0x8UL                                     /**< Bit mask for CMU_USART1 */
#define _CMU_HFPERCLKEN0_USART1_DEFAULT                   0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART1_DEFAULT                    (_CMU_HFPERCLKEN0_USART1_DEFAULT << 3)    /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP0                             (0x1UL << 4)                              /**< Analog Comparator 0 Clock Enable */
#define _CMU_HFPERCLKEN0_ACMP0_SHIFT                      4                                         /**< Shift value for CMU_ACMP0 */
#define _CMU_HFPERCLKEN0_ACMP0_MASK                       0x10UL                                    /**< Bit mask for CMU_ACMP0 */
#define _CMU_HFPERCLKEN0_ACMP0_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP0_DEFAULT                     (_CMU_HFPERCLKEN0_ACMP0_DEFAULT << 4)     /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP1                             (0x1UL << 5)                              /**< Analog Comparator 1 Clock Enable */
#define _CMU_HFPERCLKEN0_ACMP1_SHIFT                      5                                         /**< Shift value for CMU_ACMP1 */
#define _CMU_HFPERCLKEN0_ACMP1_MASK                       0x20UL                                    /**< Bit mask for CMU_ACMP1 */
#define _CMU_HFPERCLKEN0_ACMP1_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP1_DEFAULT                     (_CMU_HFPERCLKEN0_ACMP1_DEFAULT << 5)     /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_CRYOTIMER                         (0x1UL << 6)                              /**< CryoTimer Clock Enable */
#define _CMU_HFPERCLKEN0_CRYOTIMER_SHIFT                  6                                         /**< Shift value for CMU_CRYOTIMER */
#define _CMU_HFPERCLKEN0_CRYOTIMER_MASK                   0x40UL                                    /**< Bit mask for CMU_CRYOTIMER */
#define _CMU_HFPERCLKEN0_CRYOTIMER_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_CRYOTIMER_DEFAULT                 (_CMU_HFPERCLKEN0_CRYOTIMER_DEFAULT << 6) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C0                              (0x1UL << 7)                              /**< I2C 0 Clock Enable */
#define _CMU_HFPERCLKEN0_I2C0_SHIFT                       7                                         /**< Shift value for CMU_I2C0 */
#define _CMU_HFPERCLKEN0_I2C0_MASK                        0x80UL                                    /**< Bit mask for CMU_I2C0 */
#define _CMU_HFPERCLKEN0_I2C0_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C0_DEFAULT                      (_CMU_HFPERCLKEN0_I2C0_DEFAULT << 7)      /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ADC0                              (0x1UL << 8)                              /**< Analog to Digital Converter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_ADC0_SHIFT                       8                                         /**< Shift value for CMU_ADC0 */
#define _CMU_HFPERCLKEN0_ADC0_MASK                        0x100UL                                   /**< Bit mask for CMU_ADC0 */
#define _CMU_HFPERCLKEN0_ADC0_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ADC0_DEFAULT                      (_CMU_HFPERCLKEN0_ADC0_DEFAULT << 8)      /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_IDAC0                             (0x1UL << 9)                              /**< Current Digital to Analog Converter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_IDAC0_SHIFT                      9                                         /**< Shift value for CMU_IDAC0 */
#define _CMU_HFPERCLKEN0_IDAC0_MASK                       0x200UL                                   /**< Bit mask for CMU_IDAC0 */
#define _CMU_HFPERCLKEN0_IDAC0_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_IDAC0_DEFAULT                     (_CMU_HFPERCLKEN0_IDAC0_DEFAULT << 9)     /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */

/* Bit fields for CMU LFACLKEN0 */
#define _CMU_LFACLKEN0_RESETVALUE                         0x00000000UL                           /**< Default value for CMU_LFACLKEN0 */
#define _CMU_LFACLKEN0_MASK                               0x00000001UL                           /**< Mask for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LETIMER0                            (0x1UL << 0)                           /**< Low Energy Timer 0 Clock Enable */
#define _CMU_LFACLKEN0_LETIMER0_SHIFT                     0                                      /**< Shift value for CMU_LETIMER0 */
#define _CMU_LFACLKEN0_LETIMER0_MASK                      0x1UL                                  /**< Bit mask for CMU_LETIMER0 */
#define _CMU_LFACLKEN0_LETIMER0_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LETIMER0_DEFAULT                    (_CMU_LFACLKEN0_LETIMER0_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */

/* Bit fields for CMU LFBCLKEN0 */
#define _CMU_LFBCLKEN0_RESETVALUE                         0x00000000UL                          /**< Default value for CMU_LFBCLKEN0 */
#define _CMU_LFBCLKEN0_MASK                               0x00000001UL                          /**< Mask for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART0                             (0x1UL << 0)                          /**< Low Energy UART 0 Clock Enable */
#define _CMU_LFBCLKEN0_LEUART0_SHIFT                      0                                     /**< Shift value for CMU_LEUART0 */
#define _CMU_LFBCLKEN0_LEUART0_MASK                       0x1UL                                 /**< Bit mask for CMU_LEUART0 */
#define _CMU_LFBCLKEN0_LEUART0_DEFAULT                    0x00000000UL                          /**< Mode DEFAULT for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART0_DEFAULT                     (_CMU_LFBCLKEN0_LEUART0_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_LFBCLKEN0 */

/* Bit fields for CMU LFECLKEN0 */
#define _CMU_LFECLKEN0_RESETVALUE                         0x00000000UL                       /**< Default value for CMU_LFECLKEN0 */
#define _CMU_LFECLKEN0_MASK                               0x00000001UL                       /**< Mask for CMU_LFECLKEN0 */
#define CMU_LFECLKEN0_RTCC                                (0x1UL << 0)                       /**< Real-Time Counter and Calendar Clock Enable */
#define _CMU_LFECLKEN0_RTCC_SHIFT                         0                                  /**< Shift value for CMU_RTCC */
#define _CMU_LFECLKEN0_RTCC_MASK                          0x1UL                              /**< Bit mask for CMU_RTCC */
#define _CMU_LFECLKEN0_RTCC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for CMU_LFECLKEN0 */
#define CMU_LFECLKEN0_RTCC_DEFAULT                        (_CMU_LFECLKEN0_RTCC_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_LFECLKEN0 */

/* Bit fields for CMU HFPRESC */
#define _CMU_HFPRESC_RESETVALUE                           0x00000000UL                              /**< Default value for CMU_HFPRESC */
#define _CMU_HFPRESC_MASK                                 0x01001F00UL                              /**< Mask for CMU_HFPRESC */
#define _CMU_HFPRESC_PRESC_SHIFT                          8                                         /**< Shift value for CMU_PRESC */
#define _CMU_HFPRESC_PRESC_MASK                           0x1F00UL                                  /**< Bit mask for CMU_PRESC */
#define _CMU_HFPRESC_PRESC_DEFAULT                        0x00000000UL                              /**< Mode DEFAULT for CMU_HFPRESC */
#define _CMU_HFPRESC_PRESC_NODIVISION                     0x00000000UL                              /**< Mode NODIVISION for CMU_HFPRESC */
#define CMU_HFPRESC_PRESC_DEFAULT                         (_CMU_HFPRESC_PRESC_DEFAULT << 8)         /**< Shifted mode DEFAULT for CMU_HFPRESC */
#define CMU_HFPRESC_PRESC_NODIVISION                      (_CMU_HFPRESC_PRESC_NODIVISION << 8)      /**< Shifted mode NODIVISION for CMU_HFPRESC */
#define _CMU_HFPRESC_HFCLKLEPRESC_SHIFT                   24                                        /**< Shift value for CMU_HFCLKLEPRESC */
#define _CMU_HFPRESC_HFCLKLEPRESC_MASK                    0x1000000UL                               /**< Bit mask for CMU_HFCLKLEPRESC */
#define _CMU_HFPRESC_HFCLKLEPRESC_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for CMU_HFPRESC */
#define _CMU_HFPRESC_HFCLKLEPRESC_DIV2                    0x00000000UL                              /**< Mode DIV2 for CMU_HFPRESC */
#define _CMU_HFPRESC_HFCLKLEPRESC_DIV4                    0x00000001UL                              /**< Mode DIV4 for CMU_HFPRESC */
#define CMU_HFPRESC_HFCLKLEPRESC_DEFAULT                  (_CMU_HFPRESC_HFCLKLEPRESC_DEFAULT << 24) /**< Shifted mode DEFAULT for CMU_HFPRESC */
#define CMU_HFPRESC_HFCLKLEPRESC_DIV2                     (_CMU_HFPRESC_HFCLKLEPRESC_DIV2 << 24)    /**< Shifted mode DIV2 for CMU_HFPRESC */
#define CMU_HFPRESC_HFCLKLEPRESC_DIV4                     (_CMU_HFPRESC_HFCLKLEPRESC_DIV4 << 24)    /**< Shifted mode DIV4 for CMU_HFPRESC */

/* Bit fields for CMU HFCOREPRESC */
#define _CMU_HFCOREPRESC_RESETVALUE                       0x00000000UL                             /**< Default value for CMU_HFCOREPRESC */
#define _CMU_HFCOREPRESC_MASK                             0x0001FF00UL                             /**< Mask for CMU_HFCOREPRESC */
#define _CMU_HFCOREPRESC_PRESC_SHIFT                      8                                        /**< Shift value for CMU_PRESC */
#define _CMU_HFCOREPRESC_PRESC_MASK                       0x1FF00UL                                /**< Bit mask for CMU_PRESC */
#define _CMU_HFCOREPRESC_PRESC_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CMU_HFCOREPRESC */
#define _CMU_HFCOREPRESC_PRESC_NODIVISION                 0x00000000UL                             /**< Mode NODIVISION for CMU_HFCOREPRESC */
#define CMU_HFCOREPRESC_PRESC_DEFAULT                     (_CMU_HFCOREPRESC_PRESC_DEFAULT << 8)    /**< Shifted mode DEFAULT for CMU_HFCOREPRESC */
#define CMU_HFCOREPRESC_PRESC_NODIVISION                  (_CMU_HFCOREPRESC_PRESC_NODIVISION << 8) /**< Shifted mode NODIVISION for CMU_HFCOREPRESC */

/* Bit fields for CMU HFPERPRESC */
#define _CMU_HFPERPRESC_RESETVALUE                        0x00000000UL                            /**< Default value for CMU_HFPERPRESC */
#define _CMU_HFPERPRESC_MASK                              0x0001FF00UL                            /**< Mask for CMU_HFPERPRESC */
#define _CMU_HFPERPRESC_PRESC_SHIFT                       8                                       /**< Shift value for CMU_PRESC */
#define _CMU_HFPERPRESC_PRESC_MASK                        0x1FF00UL                               /**< Bit mask for CMU_PRESC */
#define _CMU_HFPERPRESC_PRESC_DEFAULT                     0x00000000UL                            /**< Mode DEFAULT for CMU_HFPERPRESC */
#define _CMU_HFPERPRESC_PRESC_NODIVISION                  0x00000000UL                            /**< Mode NODIVISION for CMU_HFPERPRESC */
#define CMU_HFPERPRESC_PRESC_DEFAULT                      (_CMU_HFPERPRESC_PRESC_DEFAULT << 8)    /**< Shifted mode DEFAULT for CMU_HFPERPRESC */
#define CMU_HFPERPRESC_PRESC_NODIVISION                   (_CMU_HFPERPRESC_PRESC_NODIVISION << 8) /**< Shifted mode NODIVISION for CMU_HFPERPRESC */

/* Bit fields for CMU HFEXPPRESC */
#define _CMU_HFEXPPRESC_RESETVALUE                        0x00000000UL                            /**< Default value for CMU_HFEXPPRESC */
#define _CMU_HFEXPPRESC_MASK                              0x00001F00UL                            /**< Mask for CMU_HFEXPPRESC */
#define _CMU_HFEXPPRESC_PRESC_SHIFT                       8                                       /**< Shift value for CMU_PRESC */
#define _CMU_HFEXPPRESC_PRESC_MASK                        0x1F00UL                                /**< Bit mask for CMU_PRESC */
#define _CMU_HFEXPPRESC_PRESC_DEFAULT                     0x00000000UL                            /**< Mode DEFAULT for CMU_HFEXPPRESC */
#define _CMU_HFEXPPRESC_PRESC_NODIVISION                  0x00000000UL                            /**< Mode NODIVISION for CMU_HFEXPPRESC */
#define CMU_HFEXPPRESC_PRESC_DEFAULT                      (_CMU_HFEXPPRESC_PRESC_DEFAULT << 8)    /**< Shifted mode DEFAULT for CMU_HFEXPPRESC */
#define CMU_HFEXPPRESC_PRESC_NODIVISION                   (_CMU_HFEXPPRESC_PRESC_NODIVISION << 8) /**< Shifted mode NODIVISION for CMU_HFEXPPRESC */

/* Bit fields for CMU LFAPRESC0 */
#define _CMU_LFAPRESC0_RESETVALUE                         0x00000000UL                            /**< Default value for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_MASK                               0x0000000FUL                            /**< Mask for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_SHIFT                     0                                       /**< Shift value for CMU_LETIMER0 */
#define _CMU_LFAPRESC0_LETIMER0_MASK                      0xFUL                                   /**< Bit mask for CMU_LETIMER0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV1                      0x00000000UL                            /**< Mode DIV1 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV2                      0x00000001UL                            /**< Mode DIV2 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV4                      0x00000002UL                            /**< Mode DIV4 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV8                      0x00000003UL                            /**< Mode DIV8 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV16                     0x00000004UL                            /**< Mode DIV16 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV32                     0x00000005UL                            /**< Mode DIV32 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV64                     0x00000006UL                            /**< Mode DIV64 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV128                    0x00000007UL                            /**< Mode DIV128 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV256                    0x00000008UL                            /**< Mode DIV256 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV512                    0x00000009UL                            /**< Mode DIV512 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV1024                   0x0000000AUL                            /**< Mode DIV1024 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV2048                   0x0000000BUL                            /**< Mode DIV2048 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV4096                   0x0000000CUL                            /**< Mode DIV4096 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV8192                   0x0000000DUL                            /**< Mode DIV8192 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV16384                  0x0000000EUL                            /**< Mode DIV16384 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV32768                  0x0000000FUL                            /**< Mode DIV32768 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV1                       (_CMU_LFAPRESC0_LETIMER0_DIV1 << 0)     /**< Shifted mode DIV1 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV2                       (_CMU_LFAPRESC0_LETIMER0_DIV2 << 0)     /**< Shifted mode DIV2 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV4                       (_CMU_LFAPRESC0_LETIMER0_DIV4 << 0)     /**< Shifted mode DIV4 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV8                       (_CMU_LFAPRESC0_LETIMER0_DIV8 << 0)     /**< Shifted mode DIV8 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV16                      (_CMU_LFAPRESC0_LETIMER0_DIV16 << 0)    /**< Shifted mode DIV16 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV32                      (_CMU_LFAPRESC0_LETIMER0_DIV32 << 0)    /**< Shifted mode DIV32 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV64                      (_CMU_LFAPRESC0_LETIMER0_DIV64 << 0)    /**< Shifted mode DIV64 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV128                     (_CMU_LFAPRESC0_LETIMER0_DIV128 << 0)   /**< Shifted mode DIV128 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV256                     (_CMU_LFAPRESC0_LETIMER0_DIV256 << 0)   /**< Shifted mode DIV256 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV512                     (_CMU_LFAPRESC0_LETIMER0_DIV512 << 0)   /**< Shifted mode DIV512 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV1024                    (_CMU_LFAPRESC0_LETIMER0_DIV1024 << 0)  /**< Shifted mode DIV1024 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV2048                    (_CMU_LFAPRESC0_LETIMER0_DIV2048 << 0)  /**< Shifted mode DIV2048 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV4096                    (_CMU_LFAPRESC0_LETIMER0_DIV4096 << 0)  /**< Shifted mode DIV4096 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV8192                    (_CMU_LFAPRESC0_LETIMER0_DIV8192 << 0)  /**< Shifted mode DIV8192 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV16384                   (_CMU_LFAPRESC0_LETIMER0_DIV16384 << 0) /**< Shifted mode DIV16384 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV32768                   (_CMU_LFAPRESC0_LETIMER0_DIV32768 << 0) /**< Shifted mode DIV32768 for CMU_LFAPRESC0 */

/* Bit fields for CMU LFBPRESC0 */
#define _CMU_LFBPRESC0_RESETVALUE                         0x00000000UL                       /**< Default value for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_MASK                               0x00000003UL                       /**< Mask for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_SHIFT                      0                                  /**< Shift value for CMU_LEUART0 */
#define _CMU_LFBPRESC0_LEUART0_MASK                       0x3UL                              /**< Bit mask for CMU_LEUART0 */
#define _CMU_LFBPRESC0_LEUART0_DIV1                       0x00000000UL                       /**< Mode DIV1 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV2                       0x00000001UL                       /**< Mode DIV2 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV4                       0x00000002UL                       /**< Mode DIV4 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV8                       0x00000003UL                       /**< Mode DIV8 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV1                        (_CMU_LFBPRESC0_LEUART0_DIV1 << 0) /**< Shifted mode DIV1 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV2                        (_CMU_LFBPRESC0_LEUART0_DIV2 << 0) /**< Shifted mode DIV2 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV4                        (_CMU_LFBPRESC0_LEUART0_DIV4 << 0) /**< Shifted mode DIV4 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV8                        (_CMU_LFBPRESC0_LEUART0_DIV8 << 0) /**< Shifted mode DIV8 for CMU_LFBPRESC0 */

/* Bit fields for CMU LFEPRESC0 */
#define _CMU_LFEPRESC0_RESETVALUE                         0x00000000UL                    /**< Default value for CMU_LFEPRESC0 */
#define _CMU_LFEPRESC0_MASK                               0x0000000FUL                    /**< Mask for CMU_LFEPRESC0 */
#define _CMU_LFEPRESC0_RTCC_SHIFT                         0                               /**< Shift value for CMU_RTCC */
#define _CMU_LFEPRESC0_RTCC_MASK                          0xFUL                           /**< Bit mask for CMU_RTCC */
#define _CMU_LFEPRESC0_RTCC_DIV1                          0x00000000UL                    /**< Mode DIV1 for CMU_LFEPRESC0 */
#define CMU_LFEPRESC0_RTCC_DIV1                           (_CMU_LFEPRESC0_RTCC_DIV1 << 0) /**< Shifted mode DIV1 for CMU_LFEPRESC0 */

/* Bit fields for CMU SYNCBUSY */
#define _CMU_SYNCBUSY_RESETVALUE                          0x00000000UL                               /**< Default value for CMU_SYNCBUSY */
#define _CMU_SYNCBUSY_MASK                                0x3F050055UL                               /**< Mask for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFACLKEN0                            (0x1UL << 0)                               /**< Low Frequency A Clock Enable 0 Busy */
#define _CMU_SYNCBUSY_LFACLKEN0_SHIFT                     0                                          /**< Shift value for CMU_LFACLKEN0 */
#define _CMU_SYNCBUSY_LFACLKEN0_MASK                      0x1UL                                      /**< Bit mask for CMU_LFACLKEN0 */
#define _CMU_SYNCBUSY_LFACLKEN0_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFACLKEN0_DEFAULT                    (_CMU_SYNCBUSY_LFACLKEN0_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFAPRESC0                            (0x1UL << 2)                               /**< Low Frequency A Prescaler 0 Busy */
#define _CMU_SYNCBUSY_LFAPRESC0_SHIFT                     2                                          /**< Shift value for CMU_LFAPRESC0 */
#define _CMU_SYNCBUSY_LFAPRESC0_MASK                      0x4UL                                      /**< Bit mask for CMU_LFAPRESC0 */
#define _CMU_SYNCBUSY_LFAPRESC0_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFAPRESC0_DEFAULT                    (_CMU_SYNCBUSY_LFAPRESC0_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBCLKEN0                            (0x1UL << 4)                               /**< Low Frequency B Clock Enable 0 Busy */
#define _CMU_SYNCBUSY_LFBCLKEN0_SHIFT                     4                                          /**< Shift value for CMU_LFBCLKEN0 */
#define _CMU_SYNCBUSY_LFBCLKEN0_MASK                      0x10UL                                     /**< Bit mask for CMU_LFBCLKEN0 */
#define _CMU_SYNCBUSY_LFBCLKEN0_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBCLKEN0_DEFAULT                    (_CMU_SYNCBUSY_LFBCLKEN0_DEFAULT << 4)     /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBPRESC0                            (0x1UL << 6)                               /**< Low Frequency B Prescaler 0 Busy */
#define _CMU_SYNCBUSY_LFBPRESC0_SHIFT                     6                                          /**< Shift value for CMU_LFBPRESC0 */
#define _CMU_SYNCBUSY_LFBPRESC0_MASK                      0x40UL                                     /**< Bit mask for CMU_LFBPRESC0 */
#define _CMU_SYNCBUSY_LFBPRESC0_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBPRESC0_DEFAULT                    (_CMU_SYNCBUSY_LFBPRESC0_DEFAULT << 6)     /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFECLKEN0                            (0x1UL << 16)                              /**< Low Frequency E Clock Enable 0 Busy */
#define _CMU_SYNCBUSY_LFECLKEN0_SHIFT                     16                                         /**< Shift value for CMU_LFECLKEN0 */
#define _CMU_SYNCBUSY_LFECLKEN0_MASK                      0x10000UL                                  /**< Bit mask for CMU_LFECLKEN0 */
#define _CMU_SYNCBUSY_LFECLKEN0_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFECLKEN0_DEFAULT                    (_CMU_SYNCBUSY_LFECLKEN0_DEFAULT << 16)    /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFEPRESC0                            (0x1UL << 18)                              /**< Low Frequency E Prescaler 0 Busy */
#define _CMU_SYNCBUSY_LFEPRESC0_SHIFT                     18                                         /**< Shift value for CMU_LFEPRESC0 */
#define _CMU_SYNCBUSY_LFEPRESC0_MASK                      0x40000UL                                  /**< Bit mask for CMU_LFEPRESC0 */
#define _CMU_SYNCBUSY_LFEPRESC0_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFEPRESC0_DEFAULT                    (_CMU_SYNCBUSY_LFEPRESC0_DEFAULT << 18)    /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_HFRCOBSY                             (0x1UL << 24)                              /**< HFRCO Busy */
#define _CMU_SYNCBUSY_HFRCOBSY_SHIFT                      24                                         /**< Shift value for CMU_HFRCOBSY */
#define _CMU_SYNCBUSY_HFRCOBSY_MASK                       0x1000000UL                                /**< Bit mask for CMU_HFRCOBSY */
#define _CMU_SYNCBUSY_HFRCOBSY_DEFAULT                    0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_HFRCOBSY_DEFAULT                     (_CMU_SYNCBUSY_HFRCOBSY_DEFAULT << 24)     /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_AUXHFRCOBSY                          (0x1UL << 25)                              /**< AUXHFRCO Busy */
#define _CMU_SYNCBUSY_AUXHFRCOBSY_SHIFT                   25                                         /**< Shift value for CMU_AUXHFRCOBSY */
#define _CMU_SYNCBUSY_AUXHFRCOBSY_MASK                    0x2000000UL                                /**< Bit mask for CMU_AUXHFRCOBSY */
#define _CMU_SYNCBUSY_AUXHFRCOBSY_DEFAULT                 0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_AUXHFRCOBSY_DEFAULT                  (_CMU_SYNCBUSY_AUXHFRCOBSY_DEFAULT << 25)  /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFRCOBSY                             (0x1UL << 26)                              /**< LFRCO Busy */
#define _CMU_SYNCBUSY_LFRCOBSY_SHIFT                      26                                         /**< Shift value for CMU_LFRCOBSY */
#define _CMU_SYNCBUSY_LFRCOBSY_MASK                       0x4000000UL                                /**< Bit mask for CMU_LFRCOBSY */
#define _CMU_SYNCBUSY_LFRCOBSY_DEFAULT                    0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFRCOBSY_DEFAULT                     (_CMU_SYNCBUSY_LFRCOBSY_DEFAULT << 26)     /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFRCOVREFBSY                         (0x1UL << 27)                              /**< LFRCO VREF Busy */
#define _CMU_SYNCBUSY_LFRCOVREFBSY_SHIFT                  27                                         /**< Shift value for CMU_LFRCOVREFBSY */
#define _CMU_SYNCBUSY_LFRCOVREFBSY_MASK                   0x8000000UL                                /**< Bit mask for CMU_LFRCOVREFBSY */
#define _CMU_SYNCBUSY_LFRCOVREFBSY_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFRCOVREFBSY_DEFAULT                 (_CMU_SYNCBUSY_LFRCOVREFBSY_DEFAULT << 27) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_HFXOBSY                              (0x1UL << 28)                              /**< HFXO Busy */
#define _CMU_SYNCBUSY_HFXOBSY_SHIFT                       28                                         /**< Shift value for CMU_HFXOBSY */
#define _CMU_SYNCBUSY_HFXOBSY_MASK                        0x10000000UL                               /**< Bit mask for CMU_HFXOBSY */
#define _CMU_SYNCBUSY_HFXOBSY_DEFAULT                     0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_HFXOBSY_DEFAULT                      (_CMU_SYNCBUSY_HFXOBSY_DEFAULT << 28)      /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFXOBSY                              (0x1UL << 29)                              /**< LFXO Busy */
#define _CMU_SYNCBUSY_LFXOBSY_SHIFT                       29                                         /**< Shift value for CMU_LFXOBSY */
#define _CMU_SYNCBUSY_LFXOBSY_MASK                        0x20000000UL                               /**< Bit mask for CMU_LFXOBSY */
#define _CMU_SYNCBUSY_LFXOBSY_DEFAULT                     0x00000000UL                               /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFXOBSY_DEFAULT                      (_CMU_SYNCBUSY_LFXOBSY_DEFAULT << 29)      /**< Shifted mode DEFAULT for CMU_SYNCBUSY */

/* Bit fields for CMU FREEZE */
#define _CMU_FREEZE_RESETVALUE                            0x00000000UL                         /**< Default value for CMU_FREEZE */
#define _CMU_FREEZE_MASK                                  0x00000001UL                         /**< Mask for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE                              (0x1UL << 0)                         /**< Register Update Freeze */
#define _CMU_FREEZE_REGFREEZE_SHIFT                       0                                    /**< Shift value for CMU_REGFREEZE */
#define _CMU_FREEZE_REGFREEZE_MASK                        0x1UL                                /**< Bit mask for CMU_REGFREEZE */
#define _CMU_FREEZE_REGFREEZE_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for CMU_FREEZE */
#define _CMU_FREEZE_REGFREEZE_UPDATE                      0x00000000UL                         /**< Mode UPDATE for CMU_FREEZE */
#define _CMU_FREEZE_REGFREEZE_FREEZE                      0x00000001UL                         /**< Mode FREEZE for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_DEFAULT                      (_CMU_FREEZE_REGFREEZE_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_UPDATE                       (_CMU_FREEZE_REGFREEZE_UPDATE << 0)  /**< Shifted mode UPDATE for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_FREEZE                       (_CMU_FREEZE_REGFREEZE_FREEZE << 0)  /**< Shifted mode FREEZE for CMU_FREEZE */

/* Bit fields for CMU PCNTCTRL */
#define _CMU_PCNTCTRL_RESETVALUE                          0x00000000UL                             /**< Default value for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_MASK                                0x00000003UL                             /**< Mask for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKEN                           (0x1UL << 0)                             /**< PCNT0 Clock Enable */
#define _CMU_PCNTCTRL_PCNT0CLKEN_SHIFT                    0                                        /**< Shift value for CMU_PCNT0CLKEN */
#define _CMU_PCNTCTRL_PCNT0CLKEN_MASK                     0x1UL                                    /**< Bit mask for CMU_PCNT0CLKEN */
#define _CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT                   (_CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL                          (0x1UL << 1)                             /**< PCNT0 Clock Select */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_SHIFT                   1                                        /**< Shift value for CMU_PCNT0CLKSEL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_MASK                    0x2UL                                    /**< Bit mask for CMU_PCNT0CLKSEL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK                  0x00000000UL                             /**< Mode LFACLK for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0                 0x00000001UL                             /**< Mode PCNT0S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT                  (_CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK                   (_CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK << 1)  /**< Shifted mode LFACLK for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0                  (_CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0 << 1) /**< Shifted mode PCNT0S0 for CMU_PCNTCTRL */

/* Bit fields for CMU ADCCTRL */
#define _CMU_ADCCTRL_RESETVALUE                           0x00000000UL                            /**< Default value for CMU_ADCCTRL */
#define _CMU_ADCCTRL_MASK                                 0x00000130UL                            /**< Mask for CMU_ADCCTRL */
#define _CMU_ADCCTRL_ADC0CLKSEL_SHIFT                     4                                       /**< Shift value for CMU_ADC0CLKSEL */
#define _CMU_ADCCTRL_ADC0CLKSEL_MASK                      0x30UL                                  /**< Bit mask for CMU_ADC0CLKSEL */
#define _CMU_ADCCTRL_ADC0CLKSEL_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for CMU_ADCCTRL */
#define _CMU_ADCCTRL_ADC0CLKSEL_DISABLED                  0x00000000UL                            /**< Mode DISABLED for CMU_ADCCTRL */
#define _CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO                  0x00000001UL                            /**< Mode AUXHFRCO for CMU_ADCCTRL */
#define _CMU_ADCCTRL_ADC0CLKSEL_HFXO                      0x00000002UL                            /**< Mode HFXO for CMU_ADCCTRL */
#define _CMU_ADCCTRL_ADC0CLKSEL_HFSRCCLK                  0x00000003UL                            /**< Mode HFSRCCLK for CMU_ADCCTRL */
#define CMU_ADCCTRL_ADC0CLKSEL_DEFAULT                    (_CMU_ADCCTRL_ADC0CLKSEL_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_ADCCTRL */
#define CMU_ADCCTRL_ADC0CLKSEL_DISABLED                   (_CMU_ADCCTRL_ADC0CLKSEL_DISABLED << 4) /**< Shifted mode DISABLED for CMU_ADCCTRL */
#define CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO                   (_CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO << 4) /**< Shifted mode AUXHFRCO for CMU_ADCCTRL */
#define CMU_ADCCTRL_ADC0CLKSEL_HFXO                       (_CMU_ADCCTRL_ADC0CLKSEL_HFXO << 4)     /**< Shifted mode HFXO for CMU_ADCCTRL */
#define CMU_ADCCTRL_ADC0CLKSEL_HFSRCCLK                   (_CMU_ADCCTRL_ADC0CLKSEL_HFSRCCLK << 4) /**< Shifted mode HFSRCCLK for CMU_ADCCTRL */
#define CMU_ADCCTRL_ADC0CLKINV                            (0x1UL << 8)                            /**< Invert clock selected by ADC0CLKSEL */
#define _CMU_ADCCTRL_ADC0CLKINV_SHIFT                     8                                       /**< Shift value for CMU_ADC0CLKINV */
#define _CMU_ADCCTRL_ADC0CLKINV_MASK                      0x100UL                                 /**< Bit mask for CMU_ADC0CLKINV */
#define _CMU_ADCCTRL_ADC0CLKINV_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for CMU_ADCCTRL */
#define CMU_ADCCTRL_ADC0CLKINV_DEFAULT                    (_CMU_ADCCTRL_ADC0CLKINV_DEFAULT << 8)  /**< Shifted mode DEFAULT for CMU_ADCCTRL */

/* Bit fields for CMU ROUTEPEN */
#define _CMU_ROUTEPEN_RESETVALUE                          0x00000000UL                            /**< Default value for CMU_ROUTEPEN */
#define _CMU_ROUTEPEN_MASK                                0x00000003UL                            /**< Mask for CMU_ROUTEPEN */
#define CMU_ROUTEPEN_CLKOUT0PEN                           (0x1UL << 0)                            /**< CLKOUT0 Pin Enable */
#define _CMU_ROUTEPEN_CLKOUT0PEN_SHIFT                    0                                       /**< Shift value for CMU_CLKOUT0PEN */
#define _CMU_ROUTEPEN_CLKOUT0PEN_MASK                     0x1UL                                   /**< Bit mask for CMU_CLKOUT0PEN */
#define _CMU_ROUTEPEN_CLKOUT0PEN_DEFAULT                  0x00000000UL                            /**< Mode DEFAULT for CMU_ROUTEPEN */
#define CMU_ROUTEPEN_CLKOUT0PEN_DEFAULT                   (_CMU_ROUTEPEN_CLKOUT0PEN_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_ROUTEPEN */
#define CMU_ROUTEPEN_CLKOUT1PEN                           (0x1UL << 1)                            /**< CLKOUT1 Pin Enable */
#define _CMU_ROUTEPEN_CLKOUT1PEN_SHIFT                    1                                       /**< Shift value for CMU_CLKOUT1PEN */
#define _CMU_ROUTEPEN_CLKOUT1PEN_MASK                     0x2UL                                   /**< Bit mask for CMU_CLKOUT1PEN */
#define _CMU_ROUTEPEN_CLKOUT1PEN_DEFAULT                  0x00000000UL                            /**< Mode DEFAULT for CMU_ROUTEPEN */
#define CMU_ROUTEPEN_CLKOUT1PEN_DEFAULT                   (_CMU_ROUTEPEN_CLKOUT1PEN_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_ROUTEPEN */

/* Bit fields for CMU ROUTELOC0 */
#define _CMU_ROUTELOC0_RESETVALUE                         0x00000000UL                             /**< Default value for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_MASK                               0x00000707UL                             /**< Mask for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_SHIFT                   0                                        /**< Shift value for CMU_CLKOUT0LOC */
#define _CMU_ROUTELOC0_CLKOUT0LOC_MASK                    0x7UL                                    /**< Bit mask for CMU_CLKOUT0LOC */
#define _CMU_ROUTELOC0_CLKOUT0LOC_LOC0                    0x00000000UL                             /**< Mode LOC0 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_LOC1                    0x00000001UL                             /**< Mode LOC1 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_LOC2                    0x00000002UL                             /**< Mode LOC2 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_LOC3                    0x00000003UL                             /**< Mode LOC3 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_LOC4                    0x00000004UL                             /**< Mode LOC4 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_LOC5                    0x00000005UL                             /**< Mode LOC5 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_LOC6                    0x00000006UL                             /**< Mode LOC6 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT0LOC_LOC7                    0x00000007UL                             /**< Mode LOC7 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_LOC0                     (_CMU_ROUTELOC0_CLKOUT0LOC_LOC0 << 0)    /**< Shifted mode LOC0 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_DEFAULT                  (_CMU_ROUTELOC0_CLKOUT0LOC_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_LOC1                     (_CMU_ROUTELOC0_CLKOUT0LOC_LOC1 << 0)    /**< Shifted mode LOC1 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_LOC2                     (_CMU_ROUTELOC0_CLKOUT0LOC_LOC2 << 0)    /**< Shifted mode LOC2 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_LOC3                     (_CMU_ROUTELOC0_CLKOUT0LOC_LOC3 << 0)    /**< Shifted mode LOC3 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_LOC4                     (_CMU_ROUTELOC0_CLKOUT0LOC_LOC4 << 0)    /**< Shifted mode LOC4 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_LOC5                     (_CMU_ROUTELOC0_CLKOUT0LOC_LOC5 << 0)    /**< Shifted mode LOC5 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_LOC6                     (_CMU_ROUTELOC0_CLKOUT0LOC_LOC6 << 0)    /**< Shifted mode LOC6 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT0LOC_LOC7                     (_CMU_ROUTELOC0_CLKOUT0LOC_LOC7 << 0)    /**< Shifted mode LOC7 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_SHIFT                   8                                        /**< Shift value for CMU_CLKOUT1LOC */
#define _CMU_ROUTELOC0_CLKOUT1LOC_MASK                    0x700UL                                  /**< Bit mask for CMU_CLKOUT1LOC */
#define _CMU_ROUTELOC0_CLKOUT1LOC_LOC0                    0x00000000UL                             /**< Mode LOC0 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_LOC1                    0x00000001UL                             /**< Mode LOC1 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_LOC2                    0x00000002UL                             /**< Mode LOC2 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_LOC3                    0x00000003UL                             /**< Mode LOC3 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_LOC4                    0x00000004UL                             /**< Mode LOC4 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_LOC5                    0x00000005UL                             /**< Mode LOC5 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_LOC6                    0x00000006UL                             /**< Mode LOC6 for CMU_ROUTELOC0 */
#define _CMU_ROUTELOC0_CLKOUT1LOC_LOC7                    0x00000007UL                             /**< Mode LOC7 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_LOC0                     (_CMU_ROUTELOC0_CLKOUT1LOC_LOC0 << 8)    /**< Shifted mode LOC0 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_DEFAULT                  (_CMU_ROUTELOC0_CLKOUT1LOC_DEFAULT << 8) /**< Shifted mode DEFAULT for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_LOC1                     (_CMU_ROUTELOC0_CLKOUT1LOC_LOC1 << 8)    /**< Shifted mode LOC1 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_LOC2                     (_CMU_ROUTELOC0_CLKOUT1LOC_LOC2 << 8)    /**< Shifted mode LOC2 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_LOC3                     (_CMU_ROUTELOC0_CLKOUT1LOC_LOC3 << 8)    /**< Shifted mode LOC3 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_LOC4                     (_CMU_ROUTELOC0_CLKOUT1LOC_LOC4 << 8)    /**< Shifted mode LOC4 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_LOC5                     (_CMU_ROUTELOC0_CLKOUT1LOC_LOC5 << 8)    /**< Shifted mode LOC5 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_LOC6                     (_CMU_ROUTELOC0_CLKOUT1LOC_LOC6 << 8)    /**< Shifted mode LOC6 for CMU_ROUTELOC0 */
#define CMU_ROUTELOC0_CLKOUT1LOC_LOC7                     (_CMU_ROUTELOC0_CLKOUT1LOC_LOC7 << 8)    /**< Shifted mode LOC7 for CMU_ROUTELOC0 */

/* Bit fields for CMU LOCK */
#define _CMU_LOCK_RESETVALUE                              0x00000000UL                      /**< Default value for CMU_LOCK */
#define _CMU_LOCK_MASK                                    0x0000FFFFUL                      /**< Mask for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_SHIFT                           0                                 /**< Shift value for CMU_LOCKKEY */
#define _CMU_LOCK_LOCKKEY_MASK                            0xFFFFUL                          /**< Bit mask for CMU_LOCKKEY */
#define _CMU_LOCK_LOCKKEY_DEFAULT                         0x00000000UL                      /**< Mode DEFAULT for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_LOCK                            0x00000000UL                      /**< Mode LOCK for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_UNLOCKED                        0x00000000UL                      /**< Mode UNLOCKED for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_LOCKED                          0x00000001UL                      /**< Mode LOCKED for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_UNLOCK                          0x0000580EUL                      /**< Mode UNLOCK for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_DEFAULT                          (_CMU_LOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_LOCK                             (_CMU_LOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_UNLOCKED                         (_CMU_LOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_LOCKED                           (_CMU_LOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_UNLOCK                           (_CMU_LOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for CMU_LOCK */

/** @} End of group EFR32FG1P_CMU */
/** @} End of group Parts */

