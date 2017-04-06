/**************************************************************************//**
 * @file efm32wg_cmu.h
 * @brief EFM32WG_CMU register and bit field definitions
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
 * @defgroup EFM32WG_CMU
 * @{
 * @brief EFM32WG_CMU Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;         /**< CMU Control Register  */
  __IOM uint32_t HFCORECLKDIV; /**< High Frequency Core Clock Division Register  */
  __IOM uint32_t HFPERCLKDIV;  /**< High Frequency Peripheral Clock Division Register  */
  __IOM uint32_t HFRCOCTRL;    /**< HFRCO Control Register  */
  __IOM uint32_t LFRCOCTRL;    /**< LFRCO Control Register  */
  __IOM uint32_t AUXHFRCOCTRL; /**< AUXHFRCO Control Register  */
  __IOM uint32_t CALCTRL;      /**< Calibration Control Register  */
  __IOM uint32_t CALCNT;       /**< Calibration Counter Register  */
  __IOM uint32_t OSCENCMD;     /**< Oscillator Enable/Disable Command Register  */
  __IOM uint32_t CMD;          /**< Command Register  */
  __IOM uint32_t LFCLKSEL;     /**< Low Frequency Clock Select Register  */
  __IM uint32_t  STATUS;       /**< Status Register  */
  __IM uint32_t  IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;          /**< Interrupt Enable Register  */
  __IOM uint32_t HFCORECLKEN0; /**< High Frequency Core Clock Enable Register 0  */
  __IOM uint32_t HFPERCLKEN0;  /**< High Frequency Peripheral Clock Enable Register 0  */
  uint32_t       RESERVED0[2]; /**< Reserved for future use **/
  __IM uint32_t  SYNCBUSY;     /**< Synchronization Busy Register  */
  __IOM uint32_t FREEZE;       /**< Freeze Register  */
  __IOM uint32_t LFACLKEN0;    /**< Low Frequency A Clock Enable Register 0  (Async Reg)  */
  uint32_t       RESERVED1[1]; /**< Reserved for future use **/
  __IOM uint32_t LFBCLKEN0;    /**< Low Frequency B Clock Enable Register 0 (Async Reg)  */

  uint32_t       RESERVED2[1]; /**< Reserved for future use **/
  __IOM uint32_t LFAPRESC0;    /**< Low Frequency A Prescaler Register 0 (Async Reg)  */
  uint32_t       RESERVED3[1]; /**< Reserved for future use **/
  __IOM uint32_t LFBPRESC0;    /**< Low Frequency B Prescaler Register 0  (Async Reg)  */
  uint32_t       RESERVED4[1]; /**< Reserved for future use **/
  __IOM uint32_t PCNTCTRL;     /**< PCNT Control Register  */
  __IOM uint32_t LCDCTRL;      /**< LCD Control Register  */
  __IOM uint32_t ROUTE;        /**< I/O Routing Register  */
  __IOM uint32_t LOCK;         /**< Configuration Lock Register  */
} CMU_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_CMU_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for CMU CTRL */
#define _CMU_CTRL_RESETVALUE                        0x000C262CUL                                /**< Default value for CMU_CTRL */
#define _CMU_CTRL_MASK                              0x57FFFEEFUL                                /**< Mask for CMU_CTRL */
#define _CMU_CTRL_HFXOMODE_SHIFT                    0                                           /**< Shift value for CMU_HFXOMODE */
#define _CMU_CTRL_HFXOMODE_MASK                     0x3UL                                       /**< Bit mask for CMU_HFXOMODE */
#define _CMU_CTRL_HFXOMODE_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOMODE_XTAL                     0x00000000UL                                /**< Mode XTAL for CMU_CTRL */
#define _CMU_CTRL_HFXOMODE_BUFEXTCLK                0x00000001UL                                /**< Mode BUFEXTCLK for CMU_CTRL */
#define _CMU_CTRL_HFXOMODE_DIGEXTCLK                0x00000002UL                                /**< Mode DIGEXTCLK for CMU_CTRL */
#define CMU_CTRL_HFXOMODE_DEFAULT                   (_CMU_CTRL_HFXOMODE_DEFAULT << 0)           /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOMODE_XTAL                      (_CMU_CTRL_HFXOMODE_XTAL << 0)              /**< Shifted mode XTAL for CMU_CTRL */
#define CMU_CTRL_HFXOMODE_BUFEXTCLK                 (_CMU_CTRL_HFXOMODE_BUFEXTCLK << 0)         /**< Shifted mode BUFEXTCLK for CMU_CTRL */
#define CMU_CTRL_HFXOMODE_DIGEXTCLK                 (_CMU_CTRL_HFXOMODE_DIGEXTCLK << 0)         /**< Shifted mode DIGEXTCLK for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_SHIFT                   2                                           /**< Shift value for CMU_HFXOBOOST */
#define _CMU_CTRL_HFXOBOOST_MASK                    0xCUL                                       /**< Bit mask for CMU_HFXOBOOST */
#define _CMU_CTRL_HFXOBOOST_50PCENT                 0x00000000UL                                /**< Mode 50PCENT for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_70PCENT                 0x00000001UL                                /**< Mode 70PCENT for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_80PCENT                 0x00000002UL                                /**< Mode 80PCENT for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_DEFAULT                 0x00000003UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOBOOST_100PCENT                0x00000003UL                                /**< Mode 100PCENT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_50PCENT                  (_CMU_CTRL_HFXOBOOST_50PCENT << 2)          /**< Shifted mode 50PCENT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_70PCENT                  (_CMU_CTRL_HFXOBOOST_70PCENT << 2)          /**< Shifted mode 70PCENT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_80PCENT                  (_CMU_CTRL_HFXOBOOST_80PCENT << 2)          /**< Shifted mode 80PCENT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_DEFAULT                  (_CMU_CTRL_HFXOBOOST_DEFAULT << 2)          /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOBOOST_100PCENT                 (_CMU_CTRL_HFXOBOOST_100PCENT << 2)         /**< Shifted mode 100PCENT for CMU_CTRL */
#define _CMU_CTRL_HFXOBUFCUR_SHIFT                  5                                           /**< Shift value for CMU_HFXOBUFCUR */
#define _CMU_CTRL_HFXOBUFCUR_MASK                   0x60UL                                      /**< Bit mask for CMU_HFXOBUFCUR */
#define _CMU_CTRL_HFXOBUFCUR_DEFAULT                0x00000001UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOBUFCUR_BOOSTUPTO32MHZ         0x00000001UL                                /**< Mode BOOSTUPTO32MHZ for CMU_CTRL */
#define _CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ        0x00000003UL                                /**< Mode BOOSTABOVE32MHZ for CMU_CTRL */
#define CMU_CTRL_HFXOBUFCUR_DEFAULT                 (_CMU_CTRL_HFXOBUFCUR_DEFAULT << 5)         /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOBUFCUR_BOOSTUPTO32MHZ          (_CMU_CTRL_HFXOBUFCUR_BOOSTUPTO32MHZ << 5)  /**< Shifted mode BOOSTUPTO32MHZ for CMU_CTRL */
#define CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ         (_CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ << 5) /**< Shifted mode BOOSTABOVE32MHZ for CMU_CTRL */
#define CMU_CTRL_HFXOGLITCHDETEN                    (0x1UL << 7)                                /**< HFXO Glitch Detector Enable */
#define _CMU_CTRL_HFXOGLITCHDETEN_SHIFT             7                                           /**< Shift value for CMU_HFXOGLITCHDETEN */
#define _CMU_CTRL_HFXOGLITCHDETEN_MASK              0x80UL                                      /**< Bit mask for CMU_HFXOGLITCHDETEN */
#define _CMU_CTRL_HFXOGLITCHDETEN_DEFAULT           0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOGLITCHDETEN_DEFAULT            (_CMU_CTRL_HFXOGLITCHDETEN_DEFAULT << 7)    /**< Shifted mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_SHIFT                 9                                           /**< Shift value for CMU_HFXOTIMEOUT */
#define _CMU_CTRL_HFXOTIMEOUT_MASK                  0x600UL                                     /**< Bit mask for CMU_HFXOTIMEOUT */
#define _CMU_CTRL_HFXOTIMEOUT_8CYCLES               0x00000000UL                                /**< Mode 8CYCLES for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_256CYCLES             0x00000001UL                                /**< Mode 256CYCLES for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_1KCYCLES              0x00000002UL                                /**< Mode 1KCYCLES for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_DEFAULT               0x00000003UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_HFXOTIMEOUT_16KCYCLES             0x00000003UL                                /**< Mode 16KCYCLES for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_8CYCLES                (_CMU_CTRL_HFXOTIMEOUT_8CYCLES << 9)        /**< Shifted mode 8CYCLES for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_256CYCLES              (_CMU_CTRL_HFXOTIMEOUT_256CYCLES << 9)      /**< Shifted mode 256CYCLES for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_1KCYCLES               (_CMU_CTRL_HFXOTIMEOUT_1KCYCLES << 9)       /**< Shifted mode 1KCYCLES for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_DEFAULT                (_CMU_CTRL_HFXOTIMEOUT_DEFAULT << 9)        /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFXOTIMEOUT_16KCYCLES              (_CMU_CTRL_HFXOTIMEOUT_16KCYCLES << 9)      /**< Shifted mode 16KCYCLES for CMU_CTRL */
#define _CMU_CTRL_LFXOMODE_SHIFT                    11                                          /**< Shift value for CMU_LFXOMODE */
#define _CMU_CTRL_LFXOMODE_MASK                     0x1800UL                                    /**< Bit mask for CMU_LFXOMODE */
#define _CMU_CTRL_LFXOMODE_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_LFXOMODE_XTAL                     0x00000000UL                                /**< Mode XTAL for CMU_CTRL */
#define _CMU_CTRL_LFXOMODE_BUFEXTCLK                0x00000001UL                                /**< Mode BUFEXTCLK for CMU_CTRL */
#define _CMU_CTRL_LFXOMODE_DIGEXTCLK                0x00000002UL                                /**< Mode DIGEXTCLK for CMU_CTRL */
#define CMU_CTRL_LFXOMODE_DEFAULT                   (_CMU_CTRL_LFXOMODE_DEFAULT << 11)          /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOMODE_XTAL                      (_CMU_CTRL_LFXOMODE_XTAL << 11)             /**< Shifted mode XTAL for CMU_CTRL */
#define CMU_CTRL_LFXOMODE_BUFEXTCLK                 (_CMU_CTRL_LFXOMODE_BUFEXTCLK << 11)        /**< Shifted mode BUFEXTCLK for CMU_CTRL */
#define CMU_CTRL_LFXOMODE_DIGEXTCLK                 (_CMU_CTRL_LFXOMODE_DIGEXTCLK << 11)        /**< Shifted mode DIGEXTCLK for CMU_CTRL */
#define CMU_CTRL_LFXOBOOST                          (0x1UL << 13)                               /**< LFXO Start-up Boost Current */
#define _CMU_CTRL_LFXOBOOST_SHIFT                   13                                          /**< Shift value for CMU_LFXOBOOST */
#define _CMU_CTRL_LFXOBOOST_MASK                    0x2000UL                                    /**< Bit mask for CMU_LFXOBOOST */
#define _CMU_CTRL_LFXOBOOST_70PCENT                 0x00000000UL                                /**< Mode 70PCENT for CMU_CTRL */
#define _CMU_CTRL_LFXOBOOST_DEFAULT                 0x00000001UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_LFXOBOOST_100PCENT                0x00000001UL                                /**< Mode 100PCENT for CMU_CTRL */
#define CMU_CTRL_LFXOBOOST_70PCENT                  (_CMU_CTRL_LFXOBOOST_70PCENT << 13)         /**< Shifted mode 70PCENT for CMU_CTRL */
#define CMU_CTRL_LFXOBOOST_DEFAULT                  (_CMU_CTRL_LFXOBOOST_DEFAULT << 13)         /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOBOOST_100PCENT                 (_CMU_CTRL_LFXOBOOST_100PCENT << 13)        /**< Shifted mode 100PCENT for CMU_CTRL */
#define _CMU_CTRL_HFCLKDIV_SHIFT                    14                                          /**< Shift value for CMU_HFCLKDIV */
#define _CMU_CTRL_HFCLKDIV_MASK                     0x1C000UL                                   /**< Bit mask for CMU_HFCLKDIV */
#define _CMU_CTRL_HFCLKDIV_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFCLKDIV_DEFAULT                   (_CMU_CTRL_HFCLKDIV_DEFAULT << 14)          /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOBUFCUR                         (0x1UL << 17)                               /**< LFXO Boost Buffer Current */
#define _CMU_CTRL_LFXOBUFCUR_SHIFT                  17                                          /**< Shift value for CMU_LFXOBUFCUR */
#define _CMU_CTRL_LFXOBUFCUR_MASK                   0x20000UL                                   /**< Bit mask for CMU_LFXOBUFCUR */
#define _CMU_CTRL_LFXOBUFCUR_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOBUFCUR_DEFAULT                 (_CMU_CTRL_LFXOBUFCUR_DEFAULT << 17)        /**< Shifted mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_SHIFT                 18                                          /**< Shift value for CMU_LFXOTIMEOUT */
#define _CMU_CTRL_LFXOTIMEOUT_MASK                  0xC0000UL                                   /**< Bit mask for CMU_LFXOTIMEOUT */
#define _CMU_CTRL_LFXOTIMEOUT_8CYCLES               0x00000000UL                                /**< Mode 8CYCLES for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_1KCYCLES              0x00000001UL                                /**< Mode 1KCYCLES for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_16KCYCLES             0x00000002UL                                /**< Mode 16KCYCLES for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_DEFAULT               0x00000003UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_LFXOTIMEOUT_32KCYCLES             0x00000003UL                                /**< Mode 32KCYCLES for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_8CYCLES                (_CMU_CTRL_LFXOTIMEOUT_8CYCLES << 18)       /**< Shifted mode 8CYCLES for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_1KCYCLES               (_CMU_CTRL_LFXOTIMEOUT_1KCYCLES << 18)      /**< Shifted mode 1KCYCLES for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_16KCYCLES              (_CMU_CTRL_LFXOTIMEOUT_16KCYCLES << 18)     /**< Shifted mode 16KCYCLES for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_DEFAULT                (_CMU_CTRL_LFXOTIMEOUT_DEFAULT << 18)       /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_LFXOTIMEOUT_32KCYCLES              (_CMU_CTRL_LFXOTIMEOUT_32KCYCLES << 18)     /**< Shifted mode 32KCYCLES for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_SHIFT                  20                                          /**< Shift value for CMU_CLKOUTSEL0 */
#define _CMU_CTRL_CLKOUTSEL0_MASK                   0x700000UL                                  /**< Bit mask for CMU_CLKOUTSEL0 */
#define _CMU_CTRL_CLKOUTSEL0_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFRCO                  0x00000000UL                                /**< Mode HFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFXO                   0x00000001UL                                /**< Mode HFXO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFCLK2                 0x00000002UL                                /**< Mode HFCLK2 for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFCLK4                 0x00000003UL                                /**< Mode HFCLK4 for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFCLK8                 0x00000004UL                                /**< Mode HFCLK8 for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_HFCLK16                0x00000005UL                                /**< Mode HFCLK16 for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_ULFRCO                 0x00000006UL                                /**< Mode ULFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL0_AUXHFRCO               0x00000007UL                                /**< Mode AUXHFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_DEFAULT                 (_CMU_CTRL_CLKOUTSEL0_DEFAULT << 20)        /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFRCO                   (_CMU_CTRL_CLKOUTSEL0_HFRCO << 20)          /**< Shifted mode HFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFXO                    (_CMU_CTRL_CLKOUTSEL0_HFXO << 20)           /**< Shifted mode HFXO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFCLK2                  (_CMU_CTRL_CLKOUTSEL0_HFCLK2 << 20)         /**< Shifted mode HFCLK2 for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFCLK4                  (_CMU_CTRL_CLKOUTSEL0_HFCLK4 << 20)         /**< Shifted mode HFCLK4 for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFCLK8                  (_CMU_CTRL_CLKOUTSEL0_HFCLK8 << 20)         /**< Shifted mode HFCLK8 for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_HFCLK16                 (_CMU_CTRL_CLKOUTSEL0_HFCLK16 << 20)        /**< Shifted mode HFCLK16 for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_ULFRCO                  (_CMU_CTRL_CLKOUTSEL0_ULFRCO << 20)         /**< Shifted mode ULFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL0_AUXHFRCO                (_CMU_CTRL_CLKOUTSEL0_AUXHFRCO << 20)       /**< Shifted mode AUXHFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_SHIFT                  23                                          /**< Shift value for CMU_CLKOUTSEL1 */
#define _CMU_CTRL_CLKOUTSEL1_MASK                   0x7800000UL                                 /**< Bit mask for CMU_CLKOUTSEL1 */
#define _CMU_CTRL_CLKOUTSEL1_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFRCO                  0x00000000UL                                /**< Mode LFRCO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFXO                   0x00000001UL                                /**< Mode LFXO for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFCLK                  0x00000002UL                                /**< Mode HFCLK for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFXOQ                  0x00000003UL                                /**< Mode LFXOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFXOQ                  0x00000004UL                                /**< Mode HFXOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_LFRCOQ                 0x00000005UL                                /**< Mode LFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_HFRCOQ                 0x00000006UL                                /**< Mode HFRCOQ for CMU_CTRL */
#define _CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ              0x00000007UL                                /**< Mode AUXHFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_DEFAULT                 (_CMU_CTRL_CLKOUTSEL1_DEFAULT << 23)        /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFRCO                   (_CMU_CTRL_CLKOUTSEL1_LFRCO << 23)          /**< Shifted mode LFRCO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFXO                    (_CMU_CTRL_CLKOUTSEL1_LFXO << 23)           /**< Shifted mode LFXO for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFCLK                   (_CMU_CTRL_CLKOUTSEL1_HFCLK << 23)          /**< Shifted mode HFCLK for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFXOQ                   (_CMU_CTRL_CLKOUTSEL1_LFXOQ << 23)          /**< Shifted mode LFXOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFXOQ                   (_CMU_CTRL_CLKOUTSEL1_HFXOQ << 23)          /**< Shifted mode HFXOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_LFRCOQ                  (_CMU_CTRL_CLKOUTSEL1_LFRCOQ << 23)         /**< Shifted mode LFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_HFRCOQ                  (_CMU_CTRL_CLKOUTSEL1_HFRCOQ << 23)         /**< Shifted mode HFRCOQ for CMU_CTRL */
#define CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ               (_CMU_CTRL_CLKOUTSEL1_AUXHFRCOQ << 23)      /**< Shifted mode AUXHFRCOQ for CMU_CTRL */
#define CMU_CTRL_DBGCLK                             (0x1UL << 28)                               /**< Debug Clock */
#define _CMU_CTRL_DBGCLK_SHIFT                      28                                          /**< Shift value for CMU_DBGCLK */
#define _CMU_CTRL_DBGCLK_MASK                       0x10000000UL                                /**< Bit mask for CMU_DBGCLK */
#define _CMU_CTRL_DBGCLK_DEFAULT                    0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define _CMU_CTRL_DBGCLK_AUXHFRCO                   0x00000000UL                                /**< Mode AUXHFRCO for CMU_CTRL */
#define _CMU_CTRL_DBGCLK_HFCLK                      0x00000001UL                                /**< Mode HFCLK for CMU_CTRL */
#define CMU_CTRL_DBGCLK_DEFAULT                     (_CMU_CTRL_DBGCLK_DEFAULT << 28)            /**< Shifted mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_DBGCLK_AUXHFRCO                    (_CMU_CTRL_DBGCLK_AUXHFRCO << 28)           /**< Shifted mode AUXHFRCO for CMU_CTRL */
#define CMU_CTRL_DBGCLK_HFCLK                       (_CMU_CTRL_DBGCLK_HFCLK << 28)              /**< Shifted mode HFCLK for CMU_CTRL */
#define CMU_CTRL_HFLE                               (0x1UL << 30)                               /**< High-Frequency LE Interface */
#define _CMU_CTRL_HFLE_SHIFT                        30                                          /**< Shift value for CMU_HFLE */
#define _CMU_CTRL_HFLE_MASK                         0x40000000UL                                /**< Bit mask for CMU_HFLE */
#define _CMU_CTRL_HFLE_DEFAULT                      0x00000000UL                                /**< Mode DEFAULT for CMU_CTRL */
#define CMU_CTRL_HFLE_DEFAULT                       (_CMU_CTRL_HFLE_DEFAULT << 30)              /**< Shifted mode DEFAULT for CMU_CTRL */

/* Bit fields for CMU HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_RESETVALUE                0x00000000UL                                    /**< Default value for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_MASK                      0x0000010FUL                                    /**< Mask for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_SHIFT        0                                               /**< Shift value for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_MASK         0xFUL                                           /**< Bit mask for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_DEFAULT      0x00000000UL                                    /**< Mode DEFAULT for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK        0x00000000UL                                    /**< Mode HFCLK for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK2       0x00000001UL                                    /**< Mode HFCLK2 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK4       0x00000002UL                                    /**< Mode HFCLK4 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK8       0x00000003UL                                    /**< Mode HFCLK8 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK16      0x00000004UL                                    /**< Mode HFCLK16 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK32      0x00000005UL                                    /**< Mode HFCLK32 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK64      0x00000006UL                                    /**< Mode HFCLK64 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK128     0x00000007UL                                    /**< Mode HFCLK128 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK256     0x00000008UL                                    /**< Mode HFCLK256 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK512     0x00000009UL                                    /**< Mode HFCLK512 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_DEFAULT       (_CMU_HFCORECLKDIV_HFCORECLKDIV_DEFAULT << 0)   /**< Shifted mode DEFAULT for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK         (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK << 0)     /**< Shifted mode HFCLK for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK2        (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK2 << 0)    /**< Shifted mode HFCLK2 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK4        (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK4 << 0)    /**< Shifted mode HFCLK4 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK8        (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK8 << 0)    /**< Shifted mode HFCLK8 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK16       (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK16 << 0)   /**< Shifted mode HFCLK16 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK32       (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK32 << 0)   /**< Shifted mode HFCLK32 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK64       (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK64 << 0)   /**< Shifted mode HFCLK64 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK128      (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK128 << 0)  /**< Shifted mode HFCLK128 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK256      (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK256 << 0)  /**< Shifted mode HFCLK256 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK512      (_CMU_HFCORECLKDIV_HFCORECLKDIV_HFCLK512 << 0)  /**< Shifted mode HFCLK512 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKLEDIV             (0x1UL << 8)                                    /**< Additional Division Factor For HFCORECLKLE */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_SHIFT      8                                               /**< Shift value for CMU_HFCORECLKLEDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_MASK       0x100UL                                         /**< Bit mask for CMU_HFCORECLKLEDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_DEFAULT    0x00000000UL                                    /**< Mode DEFAULT for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV2       0x00000000UL                                    /**< Mode DIV2 for CMU_HFCORECLKDIV */
#define _CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV4       0x00000001UL                                    /**< Mode DIV4 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKLEDIV_DEFAULT     (_CMU_HFCORECLKDIV_HFCORECLKLEDIV_DEFAULT << 8) /**< Shifted mode DEFAULT for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV2        (_CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV2 << 8)    /**< Shifted mode DIV2 for CMU_HFCORECLKDIV */
#define CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV4        (_CMU_HFCORECLKDIV_HFCORECLKLEDIV_DIV4 << 8)    /**< Shifted mode DIV4 for CMU_HFCORECLKDIV */

/* Bit fields for CMU HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_RESETVALUE                 0x00000100UL                                 /**< Default value for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_MASK                       0x0000010FUL                                 /**< Mask for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_SHIFT          0                                            /**< Shift value for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_MASK           0xFUL                                        /**< Bit mask for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_DEFAULT        0x00000000UL                                 /**< Mode DEFAULT for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK          0x00000000UL                                 /**< Mode HFCLK for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK2         0x00000001UL                                 /**< Mode HFCLK2 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK4         0x00000002UL                                 /**< Mode HFCLK4 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK8         0x00000003UL                                 /**< Mode HFCLK8 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK16        0x00000004UL                                 /**< Mode HFCLK16 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK32        0x00000005UL                                 /**< Mode HFCLK32 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK64        0x00000006UL                                 /**< Mode HFCLK64 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK128       0x00000007UL                                 /**< Mode HFCLK128 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK256       0x00000008UL                                 /**< Mode HFCLK256 for CMU_HFPERCLKDIV */
#define _CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK512       0x00000009UL                                 /**< Mode HFCLK512 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_DEFAULT         (_CMU_HFPERCLKDIV_HFPERCLKDIV_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK           (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK << 0)    /**< Shifted mode HFCLK for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK2          (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK2 << 0)   /**< Shifted mode HFCLK2 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK4          (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK4 << 0)   /**< Shifted mode HFCLK4 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK8          (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK8 << 0)   /**< Shifted mode HFCLK8 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK16         (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK16 << 0)  /**< Shifted mode HFCLK16 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK32         (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK32 << 0)  /**< Shifted mode HFCLK32 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK64         (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK64 << 0)  /**< Shifted mode HFCLK64 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK128        (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK128 << 0) /**< Shifted mode HFCLK128 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK256        (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK256 << 0) /**< Shifted mode HFCLK256 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK512        (_CMU_HFPERCLKDIV_HFPERCLKDIV_HFCLK512 << 0) /**< Shifted mode HFCLK512 for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKEN                  (0x1UL << 8)                                 /**< HFPERCLK Enable */
#define _CMU_HFPERCLKDIV_HFPERCLKEN_SHIFT           8                                            /**< Shift value for CMU_HFPERCLKEN */
#define _CMU_HFPERCLKDIV_HFPERCLKEN_MASK            0x100UL                                      /**< Bit mask for CMU_HFPERCLKEN */
#define _CMU_HFPERCLKDIV_HFPERCLKEN_DEFAULT         0x00000001UL                                 /**< Mode DEFAULT for CMU_HFPERCLKDIV */
#define CMU_HFPERCLKDIV_HFPERCLKEN_DEFAULT          (_CMU_HFPERCLKDIV_HFPERCLKEN_DEFAULT << 8)   /**< Shifted mode DEFAULT for CMU_HFPERCLKDIV */

/* Bit fields for CMU HFRCOCTRL */
#define _CMU_HFRCOCTRL_RESETVALUE                   0x00000380UL                           /**< Default value for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_MASK                         0x0001F7FFUL                           /**< Mask for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_TUNING_SHIFT                 0                                      /**< Shift value for CMU_TUNING */
#define _CMU_HFRCOCTRL_TUNING_MASK                  0xFFUL                                 /**< Bit mask for CMU_TUNING */
#define _CMU_HFRCOCTRL_TUNING_DEFAULT               0x00000080UL                           /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_TUNING_DEFAULT                (_CMU_HFRCOCTRL_TUNING_DEFAULT << 0)   /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_SHIFT                   8                                      /**< Shift value for CMU_BAND */
#define _CMU_HFRCOCTRL_BAND_MASK                    0x700UL                                /**< Bit mask for CMU_BAND */
#define _CMU_HFRCOCTRL_BAND_1MHZ                    0x00000000UL                           /**< Mode 1MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_7MHZ                    0x00000001UL                           /**< Mode 7MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_11MHZ                   0x00000002UL                           /**< Mode 11MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_DEFAULT                 0x00000003UL                           /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_14MHZ                   0x00000003UL                           /**< Mode 14MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_21MHZ                   0x00000004UL                           /**< Mode 21MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_BAND_28MHZ                   0x00000005UL                           /**< Mode 28MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_1MHZ                     (_CMU_HFRCOCTRL_BAND_1MHZ << 8)        /**< Shifted mode 1MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_7MHZ                     (_CMU_HFRCOCTRL_BAND_7MHZ << 8)        /**< Shifted mode 7MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_11MHZ                    (_CMU_HFRCOCTRL_BAND_11MHZ << 8)       /**< Shifted mode 11MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_DEFAULT                  (_CMU_HFRCOCTRL_BAND_DEFAULT << 8)     /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_14MHZ                    (_CMU_HFRCOCTRL_BAND_14MHZ << 8)       /**< Shifted mode 14MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_21MHZ                    (_CMU_HFRCOCTRL_BAND_21MHZ << 8)       /**< Shifted mode 21MHZ for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_BAND_28MHZ                    (_CMU_HFRCOCTRL_BAND_28MHZ << 8)       /**< Shifted mode 28MHZ for CMU_HFRCOCTRL */
#define _CMU_HFRCOCTRL_SUDELAY_SHIFT                12                                     /**< Shift value for CMU_SUDELAY */
#define _CMU_HFRCOCTRL_SUDELAY_MASK                 0x1F000UL                              /**< Bit mask for CMU_SUDELAY */
#define _CMU_HFRCOCTRL_SUDELAY_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFRCOCTRL */
#define CMU_HFRCOCTRL_SUDELAY_DEFAULT               (_CMU_HFRCOCTRL_SUDELAY_DEFAULT << 12) /**< Shifted mode DEFAULT for CMU_HFRCOCTRL */

/* Bit fields for CMU LFRCOCTRL */
#define _CMU_LFRCOCTRL_RESETVALUE                   0x00000040UL                         /**< Default value for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_MASK                         0x0000007FUL                         /**< Mask for CMU_LFRCOCTRL */
#define _CMU_LFRCOCTRL_TUNING_SHIFT                 0                                    /**< Shift value for CMU_TUNING */
#define _CMU_LFRCOCTRL_TUNING_MASK                  0x7FUL                               /**< Bit mask for CMU_TUNING */
#define _CMU_LFRCOCTRL_TUNING_DEFAULT               0x00000040UL                         /**< Mode DEFAULT for CMU_LFRCOCTRL */
#define CMU_LFRCOCTRL_TUNING_DEFAULT                (_CMU_LFRCOCTRL_TUNING_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_LFRCOCTRL */

/* Bit fields for CMU AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_RESETVALUE                0x00000080UL                            /**< Default value for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_MASK                      0x000007FFUL                            /**< Mask for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_TUNING_SHIFT              0                                       /**< Shift value for CMU_TUNING */
#define _CMU_AUXHFRCOCTRL_TUNING_MASK               0xFFUL                                  /**< Bit mask for CMU_TUNING */
#define _CMU_AUXHFRCOCTRL_TUNING_DEFAULT            0x00000080UL                            /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_TUNING_DEFAULT             (_CMU_AUXHFRCOCTRL_TUNING_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_SHIFT                8                                       /**< Shift value for CMU_BAND */
#define _CMU_AUXHFRCOCTRL_BAND_MASK                 0x700UL                                 /**< Bit mask for CMU_BAND */
#define _CMU_AUXHFRCOCTRL_BAND_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_14MHZ                0x00000000UL                            /**< Mode 14MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_11MHZ                0x00000001UL                            /**< Mode 11MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_7MHZ                 0x00000002UL                            /**< Mode 7MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_1MHZ                 0x00000003UL                            /**< Mode 1MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_28MHZ                0x00000006UL                            /**< Mode 28MHZ for CMU_AUXHFRCOCTRL */
#define _CMU_AUXHFRCOCTRL_BAND_21MHZ                0x00000007UL                            /**< Mode 21MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_DEFAULT               (_CMU_AUXHFRCOCTRL_BAND_DEFAULT << 8)   /**< Shifted mode DEFAULT for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_14MHZ                 (_CMU_AUXHFRCOCTRL_BAND_14MHZ << 8)     /**< Shifted mode 14MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_11MHZ                 (_CMU_AUXHFRCOCTRL_BAND_11MHZ << 8)     /**< Shifted mode 11MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_7MHZ                  (_CMU_AUXHFRCOCTRL_BAND_7MHZ << 8)      /**< Shifted mode 7MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_1MHZ                  (_CMU_AUXHFRCOCTRL_BAND_1MHZ << 8)      /**< Shifted mode 1MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_28MHZ                 (_CMU_AUXHFRCOCTRL_BAND_28MHZ << 8)     /**< Shifted mode 28MHZ for CMU_AUXHFRCOCTRL */
#define CMU_AUXHFRCOCTRL_BAND_21MHZ                 (_CMU_AUXHFRCOCTRL_BAND_21MHZ << 8)     /**< Shifted mode 21MHZ for CMU_AUXHFRCOCTRL */

/* Bit fields for CMU CALCTRL */
#define _CMU_CALCTRL_RESETVALUE                     0x00000000UL                         /**< Default value for CMU_CALCTRL */
#define _CMU_CALCTRL_MASK                           0x0000007FUL                         /**< Mask for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_SHIFT                    0                                    /**< Shift value for CMU_UPSEL */
#define _CMU_CALCTRL_UPSEL_MASK                     0x7UL                                /**< Bit mask for CMU_UPSEL */
#define _CMU_CALCTRL_UPSEL_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_HFXO                     0x00000000UL                         /**< Mode HFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_LFXO                     0x00000001UL                         /**< Mode LFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_HFRCO                    0x00000002UL                         /**< Mode HFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_LFRCO                    0x00000003UL                         /**< Mode LFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_UPSEL_AUXHFRCO                 0x00000004UL                         /**< Mode AUXHFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_DEFAULT                   (_CMU_CALCTRL_UPSEL_DEFAULT << 0)    /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_HFXO                      (_CMU_CALCTRL_UPSEL_HFXO << 0)       /**< Shifted mode HFXO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_LFXO                      (_CMU_CALCTRL_UPSEL_LFXO << 0)       /**< Shifted mode LFXO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_HFRCO                     (_CMU_CALCTRL_UPSEL_HFRCO << 0)      /**< Shifted mode HFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_LFRCO                     (_CMU_CALCTRL_UPSEL_LFRCO << 0)      /**< Shifted mode LFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_UPSEL_AUXHFRCO                  (_CMU_CALCTRL_UPSEL_AUXHFRCO << 0)   /**< Shifted mode AUXHFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_SHIFT                  3                                    /**< Shift value for CMU_DOWNSEL */
#define _CMU_CALCTRL_DOWNSEL_MASK                   0x38UL                               /**< Bit mask for CMU_DOWNSEL */
#define _CMU_CALCTRL_DOWNSEL_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFCLK                  0x00000000UL                         /**< Mode HFCLK for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFXO                   0x00000001UL                         /**< Mode HFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_LFXO                   0x00000002UL                         /**< Mode LFXO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_HFRCO                  0x00000003UL                         /**< Mode HFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_LFRCO                  0x00000004UL                         /**< Mode LFRCO for CMU_CALCTRL */
#define _CMU_CALCTRL_DOWNSEL_AUXHFRCO               0x00000005UL                         /**< Mode AUXHFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_DEFAULT                 (_CMU_CALCTRL_DOWNSEL_DEFAULT << 3)  /**< Shifted mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFCLK                   (_CMU_CALCTRL_DOWNSEL_HFCLK << 3)    /**< Shifted mode HFCLK for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFXO                    (_CMU_CALCTRL_DOWNSEL_HFXO << 3)     /**< Shifted mode HFXO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_LFXO                    (_CMU_CALCTRL_DOWNSEL_LFXO << 3)     /**< Shifted mode LFXO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_HFRCO                   (_CMU_CALCTRL_DOWNSEL_HFRCO << 3)    /**< Shifted mode HFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_LFRCO                   (_CMU_CALCTRL_DOWNSEL_LFRCO << 3)    /**< Shifted mode LFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_DOWNSEL_AUXHFRCO                (_CMU_CALCTRL_DOWNSEL_AUXHFRCO << 3) /**< Shifted mode AUXHFRCO for CMU_CALCTRL */
#define CMU_CALCTRL_CONT                            (0x1UL << 6)                         /**< Continuous Calibration */
#define _CMU_CALCTRL_CONT_SHIFT                     6                                    /**< Shift value for CMU_CONT */
#define _CMU_CALCTRL_CONT_MASK                      0x40UL                               /**< Bit mask for CMU_CONT */
#define _CMU_CALCTRL_CONT_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_CALCTRL */
#define CMU_CALCTRL_CONT_DEFAULT                    (_CMU_CALCTRL_CONT_DEFAULT << 6)     /**< Shifted mode DEFAULT for CMU_CALCTRL */

/* Bit fields for CMU CALCNT */
#define _CMU_CALCNT_RESETVALUE                      0x00000000UL                      /**< Default value for CMU_CALCNT */
#define _CMU_CALCNT_MASK                            0x000FFFFFUL                      /**< Mask for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_SHIFT                    0                                 /**< Shift value for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_MASK                     0xFFFFFUL                         /**< Bit mask for CMU_CALCNT */
#define _CMU_CALCNT_CALCNT_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for CMU_CALCNT */
#define CMU_CALCNT_CALCNT_DEFAULT                   (_CMU_CALCNT_CALCNT_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_CALCNT */

/* Bit fields for CMU OSCENCMD */
#define _CMU_OSCENCMD_RESETVALUE                    0x00000000UL                             /**< Default value for CMU_OSCENCMD */
#define _CMU_OSCENCMD_MASK                          0x000003FFUL                             /**< Mask for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCOEN                        (0x1UL << 0)                             /**< HFRCO Enable */
#define _CMU_OSCENCMD_HFRCOEN_SHIFT                 0                                        /**< Shift value for CMU_HFRCOEN */
#define _CMU_OSCENCMD_HFRCOEN_MASK                  0x1UL                                    /**< Bit mask for CMU_HFRCOEN */
#define _CMU_OSCENCMD_HFRCOEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCOEN_DEFAULT                (_CMU_OSCENCMD_HFRCOEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCODIS                       (0x1UL << 1)                             /**< HFRCO Disable */
#define _CMU_OSCENCMD_HFRCODIS_SHIFT                1                                        /**< Shift value for CMU_HFRCODIS */
#define _CMU_OSCENCMD_HFRCODIS_MASK                 0x2UL                                    /**< Bit mask for CMU_HFRCODIS */
#define _CMU_OSCENCMD_HFRCODIS_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFRCODIS_DEFAULT               (_CMU_OSCENCMD_HFRCODIS_DEFAULT << 1)    /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXOEN                         (0x1UL << 2)                             /**< HFXO Enable */
#define _CMU_OSCENCMD_HFXOEN_SHIFT                  2                                        /**< Shift value for CMU_HFXOEN */
#define _CMU_OSCENCMD_HFXOEN_MASK                   0x4UL                                    /**< Bit mask for CMU_HFXOEN */
#define _CMU_OSCENCMD_HFXOEN_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXOEN_DEFAULT                 (_CMU_OSCENCMD_HFXOEN_DEFAULT << 2)      /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXODIS                        (0x1UL << 3)                             /**< HFXO Disable */
#define _CMU_OSCENCMD_HFXODIS_SHIFT                 3                                        /**< Shift value for CMU_HFXODIS */
#define _CMU_OSCENCMD_HFXODIS_MASK                  0x8UL                                    /**< Bit mask for CMU_HFXODIS */
#define _CMU_OSCENCMD_HFXODIS_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_HFXODIS_DEFAULT                (_CMU_OSCENCMD_HFXODIS_DEFAULT << 3)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCOEN                     (0x1UL << 4)                             /**< AUXHFRCO Enable */
#define _CMU_OSCENCMD_AUXHFRCOEN_SHIFT              4                                        /**< Shift value for CMU_AUXHFRCOEN */
#define _CMU_OSCENCMD_AUXHFRCOEN_MASK               0x10UL                                   /**< Bit mask for CMU_AUXHFRCOEN */
#define _CMU_OSCENCMD_AUXHFRCOEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCOEN_DEFAULT             (_CMU_OSCENCMD_AUXHFRCOEN_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCODIS                    (0x1UL << 5)                             /**< AUXHFRCO Disable */
#define _CMU_OSCENCMD_AUXHFRCODIS_SHIFT             5                                        /**< Shift value for CMU_AUXHFRCODIS */
#define _CMU_OSCENCMD_AUXHFRCODIS_MASK              0x20UL                                   /**< Bit mask for CMU_AUXHFRCODIS */
#define _CMU_OSCENCMD_AUXHFRCODIS_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_AUXHFRCODIS_DEFAULT            (_CMU_OSCENCMD_AUXHFRCODIS_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCOEN                        (0x1UL << 6)                             /**< LFRCO Enable */
#define _CMU_OSCENCMD_LFRCOEN_SHIFT                 6                                        /**< Shift value for CMU_LFRCOEN */
#define _CMU_OSCENCMD_LFRCOEN_MASK                  0x40UL                                   /**< Bit mask for CMU_LFRCOEN */
#define _CMU_OSCENCMD_LFRCOEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCOEN_DEFAULT                (_CMU_OSCENCMD_LFRCOEN_DEFAULT << 6)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCODIS                       (0x1UL << 7)                             /**< LFRCO Disable */
#define _CMU_OSCENCMD_LFRCODIS_SHIFT                7                                        /**< Shift value for CMU_LFRCODIS */
#define _CMU_OSCENCMD_LFRCODIS_MASK                 0x80UL                                   /**< Bit mask for CMU_LFRCODIS */
#define _CMU_OSCENCMD_LFRCODIS_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFRCODIS_DEFAULT               (_CMU_OSCENCMD_LFRCODIS_DEFAULT << 7)    /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXOEN                         (0x1UL << 8)                             /**< LFXO Enable */
#define _CMU_OSCENCMD_LFXOEN_SHIFT                  8                                        /**< Shift value for CMU_LFXOEN */
#define _CMU_OSCENCMD_LFXOEN_MASK                   0x100UL                                  /**< Bit mask for CMU_LFXOEN */
#define _CMU_OSCENCMD_LFXOEN_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXOEN_DEFAULT                 (_CMU_OSCENCMD_LFXOEN_DEFAULT << 8)      /**< Shifted mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXODIS                        (0x1UL << 9)                             /**< LFXO Disable */
#define _CMU_OSCENCMD_LFXODIS_SHIFT                 9                                        /**< Shift value for CMU_LFXODIS */
#define _CMU_OSCENCMD_LFXODIS_MASK                  0x200UL                                  /**< Bit mask for CMU_LFXODIS */
#define _CMU_OSCENCMD_LFXODIS_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for CMU_OSCENCMD */
#define CMU_OSCENCMD_LFXODIS_DEFAULT                (_CMU_OSCENCMD_LFXODIS_DEFAULT << 9)     /**< Shifted mode DEFAULT for CMU_OSCENCMD */

/* Bit fields for CMU CMD */
#define _CMU_CMD_RESETVALUE                         0x00000000UL                          /**< Default value for CMU_CMD */
#define _CMU_CMD_MASK                               0x000000FFUL                          /**< Mask for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_SHIFT                     0                                     /**< Shift value for CMU_HFCLKSEL */
#define _CMU_CMD_HFCLKSEL_MASK                      0x7UL                                 /**< Bit mask for CMU_HFCLKSEL */
#define _CMU_CMD_HFCLKSEL_DEFAULT                   0x00000000UL                          /**< Mode DEFAULT for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_HFRCO                     0x00000001UL                          /**< Mode HFRCO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_HFXO                      0x00000002UL                          /**< Mode HFXO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_LFRCO                     0x00000003UL                          /**< Mode LFRCO for CMU_CMD */
#define _CMU_CMD_HFCLKSEL_LFXO                      0x00000004UL                          /**< Mode LFXO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_DEFAULT                    (_CMU_CMD_HFCLKSEL_DEFAULT << 0)      /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_HFCLKSEL_HFRCO                      (_CMU_CMD_HFCLKSEL_HFRCO << 0)        /**< Shifted mode HFRCO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_HFXO                       (_CMU_CMD_HFCLKSEL_HFXO << 0)         /**< Shifted mode HFXO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_LFRCO                      (_CMU_CMD_HFCLKSEL_LFRCO << 0)        /**< Shifted mode LFRCO for CMU_CMD */
#define CMU_CMD_HFCLKSEL_LFXO                       (_CMU_CMD_HFCLKSEL_LFXO << 0)         /**< Shifted mode LFXO for CMU_CMD */
#define CMU_CMD_CALSTART                            (0x1UL << 3)                          /**< Calibration Start */
#define _CMU_CMD_CALSTART_SHIFT                     3                                     /**< Shift value for CMU_CALSTART */
#define _CMU_CMD_CALSTART_MASK                      0x8UL                                 /**< Bit mask for CMU_CALSTART */
#define _CMU_CMD_CALSTART_DEFAULT                   0x00000000UL                          /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTART_DEFAULT                    (_CMU_CMD_CALSTART_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTOP                             (0x1UL << 4)                          /**< Calibration Stop */
#define _CMU_CMD_CALSTOP_SHIFT                      4                                     /**< Shift value for CMU_CALSTOP */
#define _CMU_CMD_CALSTOP_MASK                       0x10UL                                /**< Bit mask for CMU_CALSTOP */
#define _CMU_CMD_CALSTOP_DEFAULT                    0x00000000UL                          /**< Mode DEFAULT for CMU_CMD */
#define CMU_CMD_CALSTOP_DEFAULT                     (_CMU_CMD_CALSTOP_DEFAULT << 4)       /**< Shifted mode DEFAULT for CMU_CMD */
#define _CMU_CMD_USBCCLKSEL_SHIFT                   5                                     /**< Shift value for CMU_USBCCLKSEL */
#define _CMU_CMD_USBCCLKSEL_MASK                    0xE0UL                                /**< Bit mask for CMU_USBCCLKSEL */
#define _CMU_CMD_USBCCLKSEL_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for CMU_CMD */
#define _CMU_CMD_USBCCLKSEL_HFCLKNODIV              0x00000001UL                          /**< Mode HFCLKNODIV for CMU_CMD */
#define _CMU_CMD_USBCCLKSEL_LFXO                    0x00000002UL                          /**< Mode LFXO for CMU_CMD */
#define _CMU_CMD_USBCCLKSEL_LFRCO                   0x00000003UL                          /**< Mode LFRCO for CMU_CMD */
#define CMU_CMD_USBCCLKSEL_DEFAULT                  (_CMU_CMD_USBCCLKSEL_DEFAULT << 5)    /**< Shifted mode DEFAULT for CMU_CMD */
#define CMU_CMD_USBCCLKSEL_HFCLKNODIV               (_CMU_CMD_USBCCLKSEL_HFCLKNODIV << 5) /**< Shifted mode HFCLKNODIV for CMU_CMD */
#define CMU_CMD_USBCCLKSEL_LFXO                     (_CMU_CMD_USBCCLKSEL_LFXO << 5)       /**< Shifted mode LFXO for CMU_CMD */
#define CMU_CMD_USBCCLKSEL_LFRCO                    (_CMU_CMD_USBCCLKSEL_LFRCO << 5)      /**< Shifted mode LFRCO for CMU_CMD */

/* Bit fields for CMU LFCLKSEL */
#define _CMU_LFCLKSEL_RESETVALUE                    0x00000005UL                             /**< Default value for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_MASK                          0x0011000FUL                             /**< Mask for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_SHIFT                     0                                        /**< Shift value for CMU_LFA */
#define _CMU_LFCLKSEL_LFA_MASK                      0x3UL                                    /**< Bit mask for CMU_LFA */
#define _CMU_LFCLKSEL_LFA_DISABLED                  0x00000000UL                             /**< Mode DISABLED for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_DEFAULT                   0x00000001UL                             /**< Mode DEFAULT for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_LFRCO                     0x00000001UL                             /**< Mode LFRCO for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_LFXO                      0x00000002UL                             /**< Mode LFXO for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2           0x00000003UL                             /**< Mode HFCORECLKLEDIV2 for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_DISABLED                   (_CMU_LFCLKSEL_LFA_DISABLED << 0)        /**< Shifted mode DISABLED for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_DEFAULT                    (_CMU_LFCLKSEL_LFA_DEFAULT << 0)         /**< Shifted mode DEFAULT for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_LFRCO                      (_CMU_LFCLKSEL_LFA_LFRCO << 0)           /**< Shifted mode LFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_LFXO                       (_CMU_LFCLKSEL_LFA_LFXO << 0)            /**< Shifted mode LFXO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2            (_CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2 << 0) /**< Shifted mode HFCORECLKLEDIV2 for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_SHIFT                     2                                        /**< Shift value for CMU_LFB */
#define _CMU_LFCLKSEL_LFB_MASK                      0xCUL                                    /**< Bit mask for CMU_LFB */
#define _CMU_LFCLKSEL_LFB_DISABLED                  0x00000000UL                             /**< Mode DISABLED for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_DEFAULT                   0x00000001UL                             /**< Mode DEFAULT for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_LFRCO                     0x00000001UL                             /**< Mode LFRCO for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_LFXO                      0x00000002UL                             /**< Mode LFXO for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFB_HFCORECLKLEDIV2           0x00000003UL                             /**< Mode HFCORECLKLEDIV2 for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_DISABLED                   (_CMU_LFCLKSEL_LFB_DISABLED << 2)        /**< Shifted mode DISABLED for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_DEFAULT                    (_CMU_LFCLKSEL_LFB_DEFAULT << 2)         /**< Shifted mode DEFAULT for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_LFRCO                      (_CMU_LFCLKSEL_LFB_LFRCO << 2)           /**< Shifted mode LFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_LFXO                       (_CMU_LFCLKSEL_LFB_LFXO << 2)            /**< Shifted mode LFXO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFB_HFCORECLKLEDIV2            (_CMU_LFCLKSEL_LFB_HFCORECLKLEDIV2 << 2) /**< Shifted mode HFCORECLKLEDIV2 for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFAE                           (0x1UL << 16)                            /**< Clock Select for LFA Extended */
#define _CMU_LFCLKSEL_LFAE_SHIFT                    16                                       /**< Shift value for CMU_LFAE */
#define _CMU_LFCLKSEL_LFAE_MASK                     0x10000UL                                /**< Bit mask for CMU_LFAE */
#define _CMU_LFCLKSEL_LFAE_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFAE_DISABLED                 0x00000000UL                             /**< Mode DISABLED for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFAE_ULFRCO                   0x00000001UL                             /**< Mode ULFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFAE_DEFAULT                   (_CMU_LFCLKSEL_LFAE_DEFAULT << 16)       /**< Shifted mode DEFAULT for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFAE_DISABLED                  (_CMU_LFCLKSEL_LFAE_DISABLED << 16)      /**< Shifted mode DISABLED for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFAE_ULFRCO                    (_CMU_LFCLKSEL_LFAE_ULFRCO << 16)        /**< Shifted mode ULFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFBE                           (0x1UL << 20)                            /**< Clock Select for LFB Extended */
#define _CMU_LFCLKSEL_LFBE_SHIFT                    20                                       /**< Shift value for CMU_LFBE */
#define _CMU_LFCLKSEL_LFBE_MASK                     0x100000UL                               /**< Bit mask for CMU_LFBE */
#define _CMU_LFCLKSEL_LFBE_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFBE_DISABLED                 0x00000000UL                             /**< Mode DISABLED for CMU_LFCLKSEL */
#define _CMU_LFCLKSEL_LFBE_ULFRCO                   0x00000001UL                             /**< Mode ULFRCO for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFBE_DEFAULT                   (_CMU_LFCLKSEL_LFBE_DEFAULT << 20)       /**< Shifted mode DEFAULT for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFBE_DISABLED                  (_CMU_LFCLKSEL_LFBE_DISABLED << 20)      /**< Shifted mode DISABLED for CMU_LFCLKSEL */
#define CMU_LFCLKSEL_LFBE_ULFRCO                    (_CMU_LFCLKSEL_LFBE_ULFRCO << 20)        /**< Shifted mode ULFRCO for CMU_LFCLKSEL */

/* Bit fields for CMU STATUS */
#define _CMU_STATUS_RESETVALUE                      0x00000403UL                             /**< Default value for CMU_STATUS */
#define _CMU_STATUS_MASK                            0x0003FFFFUL                             /**< Mask for CMU_STATUS */
#define CMU_STATUS_HFRCOENS                         (0x1UL << 0)                             /**< HFRCO Enable Status */
#define _CMU_STATUS_HFRCOENS_SHIFT                  0                                        /**< Shift value for CMU_HFRCOENS */
#define _CMU_STATUS_HFRCOENS_MASK                   0x1UL                                    /**< Bit mask for CMU_HFRCOENS */
#define _CMU_STATUS_HFRCOENS_DEFAULT                0x00000001UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOENS_DEFAULT                 (_CMU_STATUS_HFRCOENS_DEFAULT << 0)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCORDY                         (0x1UL << 1)                             /**< HFRCO Ready */
#define _CMU_STATUS_HFRCORDY_SHIFT                  1                                        /**< Shift value for CMU_HFRCORDY */
#define _CMU_STATUS_HFRCORDY_MASK                   0x2UL                                    /**< Bit mask for CMU_HFRCORDY */
#define _CMU_STATUS_HFRCORDY_DEFAULT                0x00000001UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCORDY_DEFAULT                 (_CMU_STATUS_HFRCORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOENS                          (0x1UL << 2)                             /**< HFXO Enable Status */
#define _CMU_STATUS_HFXOENS_SHIFT                   2                                        /**< Shift value for CMU_HFXOENS */
#define _CMU_STATUS_HFXOENS_MASK                    0x4UL                                    /**< Bit mask for CMU_HFXOENS */
#define _CMU_STATUS_HFXOENS_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOENS_DEFAULT                  (_CMU_STATUS_HFXOENS_DEFAULT << 2)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXORDY                          (0x1UL << 3)                             /**< HFXO Ready */
#define _CMU_STATUS_HFXORDY_SHIFT                   3                                        /**< Shift value for CMU_HFXORDY */
#define _CMU_STATUS_HFXORDY_MASK                    0x8UL                                    /**< Bit mask for CMU_HFXORDY */
#define _CMU_STATUS_HFXORDY_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXORDY_DEFAULT                  (_CMU_STATUS_HFXORDY_DEFAULT << 3)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCOENS                      (0x1UL << 4)                             /**< AUXHFRCO Enable Status */
#define _CMU_STATUS_AUXHFRCOENS_SHIFT               4                                        /**< Shift value for CMU_AUXHFRCOENS */
#define _CMU_STATUS_AUXHFRCOENS_MASK                0x10UL                                   /**< Bit mask for CMU_AUXHFRCOENS */
#define _CMU_STATUS_AUXHFRCOENS_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCOENS_DEFAULT              (_CMU_STATUS_AUXHFRCOENS_DEFAULT << 4)   /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCORDY                      (0x1UL << 5)                             /**< AUXHFRCO Ready */
#define _CMU_STATUS_AUXHFRCORDY_SHIFT               5                                        /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_STATUS_AUXHFRCORDY_MASK                0x20UL                                   /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_STATUS_AUXHFRCORDY_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_AUXHFRCORDY_DEFAULT              (_CMU_STATUS_AUXHFRCORDY_DEFAULT << 5)   /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOENS                         (0x1UL << 6)                             /**< LFRCO Enable Status */
#define _CMU_STATUS_LFRCOENS_SHIFT                  6                                        /**< Shift value for CMU_LFRCOENS */
#define _CMU_STATUS_LFRCOENS_MASK                   0x40UL                                   /**< Bit mask for CMU_LFRCOENS */
#define _CMU_STATUS_LFRCOENS_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOENS_DEFAULT                 (_CMU_STATUS_LFRCOENS_DEFAULT << 6)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCORDY                         (0x1UL << 7)                             /**< LFRCO Ready */
#define _CMU_STATUS_LFRCORDY_SHIFT                  7                                        /**< Shift value for CMU_LFRCORDY */
#define _CMU_STATUS_LFRCORDY_MASK                   0x80UL                                   /**< Bit mask for CMU_LFRCORDY */
#define _CMU_STATUS_LFRCORDY_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCORDY_DEFAULT                 (_CMU_STATUS_LFRCORDY_DEFAULT << 7)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOENS                          (0x1UL << 8)                             /**< LFXO Enable Status */
#define _CMU_STATUS_LFXOENS_SHIFT                   8                                        /**< Shift value for CMU_LFXOENS */
#define _CMU_STATUS_LFXOENS_MASK                    0x100UL                                  /**< Bit mask for CMU_LFXOENS */
#define _CMU_STATUS_LFXOENS_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOENS_DEFAULT                  (_CMU_STATUS_LFXOENS_DEFAULT << 8)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXORDY                          (0x1UL << 9)                             /**< LFXO Ready */
#define _CMU_STATUS_LFXORDY_SHIFT                   9                                        /**< Shift value for CMU_LFXORDY */
#define _CMU_STATUS_LFXORDY_MASK                    0x200UL                                  /**< Bit mask for CMU_LFXORDY */
#define _CMU_STATUS_LFXORDY_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXORDY_DEFAULT                  (_CMU_STATUS_LFXORDY_DEFAULT << 9)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOSEL                         (0x1UL << 10)                            /**< HFRCO Selected */
#define _CMU_STATUS_HFRCOSEL_SHIFT                  10                                       /**< Shift value for CMU_HFRCOSEL */
#define _CMU_STATUS_HFRCOSEL_MASK                   0x400UL                                  /**< Bit mask for CMU_HFRCOSEL */
#define _CMU_STATUS_HFRCOSEL_DEFAULT                0x00000001UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFRCOSEL_DEFAULT                 (_CMU_STATUS_HFRCOSEL_DEFAULT << 10)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOSEL                          (0x1UL << 11)                            /**< HFXO Selected */
#define _CMU_STATUS_HFXOSEL_SHIFT                   11                                       /**< Shift value for CMU_HFXOSEL */
#define _CMU_STATUS_HFXOSEL_MASK                    0x800UL                                  /**< Bit mask for CMU_HFXOSEL */
#define _CMU_STATUS_HFXOSEL_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_HFXOSEL_DEFAULT                  (_CMU_STATUS_HFXOSEL_DEFAULT << 11)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOSEL                         (0x1UL << 12)                            /**< LFRCO Selected */
#define _CMU_STATUS_LFRCOSEL_SHIFT                  12                                       /**< Shift value for CMU_LFRCOSEL */
#define _CMU_STATUS_LFRCOSEL_MASK                   0x1000UL                                 /**< Bit mask for CMU_LFRCOSEL */
#define _CMU_STATUS_LFRCOSEL_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFRCOSEL_DEFAULT                 (_CMU_STATUS_LFRCOSEL_DEFAULT << 12)     /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOSEL                          (0x1UL << 13)                            /**< LFXO Selected */
#define _CMU_STATUS_LFXOSEL_SHIFT                   13                                       /**< Shift value for CMU_LFXOSEL */
#define _CMU_STATUS_LFXOSEL_MASK                    0x2000UL                                 /**< Bit mask for CMU_LFXOSEL */
#define _CMU_STATUS_LFXOSEL_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_LFXOSEL_DEFAULT                  (_CMU_STATUS_LFXOSEL_DEFAULT << 13)      /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_CALBSY                           (0x1UL << 14)                            /**< Calibration Busy */
#define _CMU_STATUS_CALBSY_SHIFT                    14                                       /**< Shift value for CMU_CALBSY */
#define _CMU_STATUS_CALBSY_MASK                     0x4000UL                                 /**< Bit mask for CMU_CALBSY */
#define _CMU_STATUS_CALBSY_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_CALBSY_DEFAULT                   (_CMU_STATUS_CALBSY_DEFAULT << 14)       /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCHFCLKSEL                     (0x1UL << 15)                            /**< USBC HFCLK Selected */
#define _CMU_STATUS_USBCHFCLKSEL_SHIFT              15                                       /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_STATUS_USBCHFCLKSEL_MASK               0x8000UL                                 /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_STATUS_USBCHFCLKSEL_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCHFCLKSEL_DEFAULT             (_CMU_STATUS_USBCHFCLKSEL_DEFAULT << 15) /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCLFXOSEL                      (0x1UL << 16)                            /**< USBC LFXO Selected */
#define _CMU_STATUS_USBCLFXOSEL_SHIFT               16                                       /**< Shift value for CMU_USBCLFXOSEL */
#define _CMU_STATUS_USBCLFXOSEL_MASK                0x10000UL                                /**< Bit mask for CMU_USBCLFXOSEL */
#define _CMU_STATUS_USBCLFXOSEL_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCLFXOSEL_DEFAULT              (_CMU_STATUS_USBCLFXOSEL_DEFAULT << 16)  /**< Shifted mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCLFRCOSEL                     (0x1UL << 17)                            /**< USBC LFRCO Selected */
#define _CMU_STATUS_USBCLFRCOSEL_SHIFT              17                                       /**< Shift value for CMU_USBCLFRCOSEL */
#define _CMU_STATUS_USBCLFRCOSEL_MASK               0x20000UL                                /**< Bit mask for CMU_USBCLFRCOSEL */
#define _CMU_STATUS_USBCLFRCOSEL_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_STATUS */
#define CMU_STATUS_USBCLFRCOSEL_DEFAULT             (_CMU_STATUS_USBCLFRCOSEL_DEFAULT << 17) /**< Shifted mode DEFAULT for CMU_STATUS */

/* Bit fields for CMU IF */
#define _CMU_IF_RESETVALUE                          0x00000001UL                        /**< Default value for CMU_IF */
#define _CMU_IF_MASK                                0x000000FFUL                        /**< Mask for CMU_IF */
#define CMU_IF_HFRCORDY                             (0x1UL << 0)                        /**< HFRCO Ready Interrupt Flag */
#define _CMU_IF_HFRCORDY_SHIFT                      0                                   /**< Shift value for CMU_HFRCORDY */
#define _CMU_IF_HFRCORDY_MASK                       0x1UL                               /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IF_HFRCORDY_DEFAULT                    0x00000001UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFRCORDY_DEFAULT                     (_CMU_IF_HFRCORDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_HFXORDY                              (0x1UL << 1)                        /**< HFXO Ready Interrupt Flag */
#define _CMU_IF_HFXORDY_SHIFT                       1                                   /**< Shift value for CMU_HFXORDY */
#define _CMU_IF_HFXORDY_MASK                        0x2UL                               /**< Bit mask for CMU_HFXORDY */
#define _CMU_IF_HFXORDY_DEFAULT                     0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_HFXORDY_DEFAULT                      (_CMU_IF_HFXORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFRCORDY                             (0x1UL << 2)                        /**< LFRCO Ready Interrupt Flag */
#define _CMU_IF_LFRCORDY_SHIFT                      2                                   /**< Shift value for CMU_LFRCORDY */
#define _CMU_IF_LFRCORDY_MASK                       0x4UL                               /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IF_LFRCORDY_DEFAULT                    0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFRCORDY_DEFAULT                     (_CMU_IF_LFRCORDY_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_LFXORDY                              (0x1UL << 3)                        /**< LFXO Ready Interrupt Flag */
#define _CMU_IF_LFXORDY_SHIFT                       3                                   /**< Shift value for CMU_LFXORDY */
#define _CMU_IF_LFXORDY_MASK                        0x8UL                               /**< Bit mask for CMU_LFXORDY */
#define _CMU_IF_LFXORDY_DEFAULT                     0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_LFXORDY_DEFAULT                      (_CMU_IF_LFXORDY_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_AUXHFRCORDY                          (0x1UL << 4)                        /**< AUXHFRCO Ready Interrupt Flag */
#define _CMU_IF_AUXHFRCORDY_SHIFT                   4                                   /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IF_AUXHFRCORDY_MASK                    0x10UL                              /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IF_AUXHFRCORDY_DEFAULT                 0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_AUXHFRCORDY_DEFAULT                  (_CMU_IF_AUXHFRCORDY_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CALRDY                               (0x1UL << 5)                        /**< Calibration Ready Interrupt Flag */
#define _CMU_IF_CALRDY_SHIFT                        5                                   /**< Shift value for CMU_CALRDY */
#define _CMU_IF_CALRDY_MASK                         0x20UL                              /**< Bit mask for CMU_CALRDY */
#define _CMU_IF_CALRDY_DEFAULT                      0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CALRDY_DEFAULT                       (_CMU_IF_CALRDY_DEFAULT << 5)       /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_CALOF                                (0x1UL << 6)                        /**< Calibration Overflow Interrupt Flag */
#define _CMU_IF_CALOF_SHIFT                         6                                   /**< Shift value for CMU_CALOF */
#define _CMU_IF_CALOF_MASK                          0x40UL                              /**< Bit mask for CMU_CALOF */
#define _CMU_IF_CALOF_DEFAULT                       0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_CALOF_DEFAULT                        (_CMU_IF_CALOF_DEFAULT << 6)        /**< Shifted mode DEFAULT for CMU_IF */
#define CMU_IF_USBCHFCLKSEL                         (0x1UL << 7)                        /**< USBC HFCLK Selected Interrupt Flag */
#define _CMU_IF_USBCHFCLKSEL_SHIFT                  7                                   /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_IF_USBCHFCLKSEL_MASK                   0x80UL                              /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_IF_USBCHFCLKSEL_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for CMU_IF */
#define CMU_IF_USBCHFCLKSEL_DEFAULT                 (_CMU_IF_USBCHFCLKSEL_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_IF */

/* Bit fields for CMU IFS */
#define _CMU_IFS_RESETVALUE                         0x00000000UL                         /**< Default value for CMU_IFS */
#define _CMU_IFS_MASK                               0x000000FFUL                         /**< Mask for CMU_IFS */
#define CMU_IFS_HFRCORDY                            (0x1UL << 0)                         /**< HFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_HFRCORDY_SHIFT                     0                                    /**< Shift value for CMU_HFRCORDY */
#define _CMU_IFS_HFRCORDY_MASK                      0x1UL                                /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IFS_HFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFRCORDY_DEFAULT                    (_CMU_IFS_HFRCORDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXORDY                             (0x1UL << 1)                         /**< HFXO Ready Interrupt Flag Set */
#define _CMU_IFS_HFXORDY_SHIFT                      1                                    /**< Shift value for CMU_HFXORDY */
#define _CMU_IFS_HFXORDY_MASK                       0x2UL                                /**< Bit mask for CMU_HFXORDY */
#define _CMU_IFS_HFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_HFXORDY_DEFAULT                     (_CMU_IFS_HFXORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFRCORDY                            (0x1UL << 2)                         /**< LFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_LFRCORDY_SHIFT                     2                                    /**< Shift value for CMU_LFRCORDY */
#define _CMU_IFS_LFRCORDY_MASK                      0x4UL                                /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IFS_LFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFRCORDY_DEFAULT                    (_CMU_IFS_LFRCORDY_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFXORDY                             (0x1UL << 3)                         /**< LFXO Ready Interrupt Flag Set */
#define _CMU_IFS_LFXORDY_SHIFT                      3                                    /**< Shift value for CMU_LFXORDY */
#define _CMU_IFS_LFXORDY_MASK                       0x8UL                                /**< Bit mask for CMU_LFXORDY */
#define _CMU_IFS_LFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_LFXORDY_DEFAULT                     (_CMU_IFS_LFXORDY_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_AUXHFRCORDY                         (0x1UL << 4)                         /**< AUXHFRCO Ready Interrupt Flag Set */
#define _CMU_IFS_AUXHFRCORDY_SHIFT                  4                                    /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IFS_AUXHFRCORDY_MASK                   0x10UL                               /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IFS_AUXHFRCORDY_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_AUXHFRCORDY_DEFAULT                 (_CMU_IFS_AUXHFRCORDY_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALRDY                              (0x1UL << 5)                         /**< Calibration Ready Interrupt Flag Set */
#define _CMU_IFS_CALRDY_SHIFT                       5                                    /**< Shift value for CMU_CALRDY */
#define _CMU_IFS_CALRDY_MASK                        0x20UL                               /**< Bit mask for CMU_CALRDY */
#define _CMU_IFS_CALRDY_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALRDY_DEFAULT                      (_CMU_IFS_CALRDY_DEFAULT << 5)       /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALOF                               (0x1UL << 6)                         /**< Calibration Overflow Interrupt Flag Set */
#define _CMU_IFS_CALOF_SHIFT                        6                                    /**< Shift value for CMU_CALOF */
#define _CMU_IFS_CALOF_MASK                         0x40UL                               /**< Bit mask for CMU_CALOF */
#define _CMU_IFS_CALOF_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_CALOF_DEFAULT                       (_CMU_IFS_CALOF_DEFAULT << 6)        /**< Shifted mode DEFAULT for CMU_IFS */
#define CMU_IFS_USBCHFCLKSEL                        (0x1UL << 7)                         /**< USBC HFCLK Selected Interrupt Flag Set */
#define _CMU_IFS_USBCHFCLKSEL_SHIFT                 7                                    /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_IFS_USBCHFCLKSEL_MASK                  0x80UL                               /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_IFS_USBCHFCLKSEL_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_IFS */
#define CMU_IFS_USBCHFCLKSEL_DEFAULT                (_CMU_IFS_USBCHFCLKSEL_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_IFS */

/* Bit fields for CMU IFC */
#define _CMU_IFC_RESETVALUE                         0x00000000UL                         /**< Default value for CMU_IFC */
#define _CMU_IFC_MASK                               0x000000FFUL                         /**< Mask for CMU_IFC */
#define CMU_IFC_HFRCORDY                            (0x1UL << 0)                         /**< HFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_HFRCORDY_SHIFT                     0                                    /**< Shift value for CMU_HFRCORDY */
#define _CMU_IFC_HFRCORDY_MASK                      0x1UL                                /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IFC_HFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFRCORDY_DEFAULT                    (_CMU_IFC_HFRCORDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXORDY                             (0x1UL << 1)                         /**< HFXO Ready Interrupt Flag Clear */
#define _CMU_IFC_HFXORDY_SHIFT                      1                                    /**< Shift value for CMU_HFXORDY */
#define _CMU_IFC_HFXORDY_MASK                       0x2UL                                /**< Bit mask for CMU_HFXORDY */
#define _CMU_IFC_HFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_HFXORDY_DEFAULT                     (_CMU_IFC_HFXORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFRCORDY                            (0x1UL << 2)                         /**< LFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_LFRCORDY_SHIFT                     2                                    /**< Shift value for CMU_LFRCORDY */
#define _CMU_IFC_LFRCORDY_MASK                      0x4UL                                /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IFC_LFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFRCORDY_DEFAULT                    (_CMU_IFC_LFRCORDY_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFXORDY                             (0x1UL << 3)                         /**< LFXO Ready Interrupt Flag Clear */
#define _CMU_IFC_LFXORDY_SHIFT                      3                                    /**< Shift value for CMU_LFXORDY */
#define _CMU_IFC_LFXORDY_MASK                       0x8UL                                /**< Bit mask for CMU_LFXORDY */
#define _CMU_IFC_LFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_LFXORDY_DEFAULT                     (_CMU_IFC_LFXORDY_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_AUXHFRCORDY                         (0x1UL << 4)                         /**< AUXHFRCO Ready Interrupt Flag Clear */
#define _CMU_IFC_AUXHFRCORDY_SHIFT                  4                                    /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IFC_AUXHFRCORDY_MASK                   0x10UL                               /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IFC_AUXHFRCORDY_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_AUXHFRCORDY_DEFAULT                 (_CMU_IFC_AUXHFRCORDY_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALRDY                              (0x1UL << 5)                         /**< Calibration Ready Interrupt Flag Clear */
#define _CMU_IFC_CALRDY_SHIFT                       5                                    /**< Shift value for CMU_CALRDY */
#define _CMU_IFC_CALRDY_MASK                        0x20UL                               /**< Bit mask for CMU_CALRDY */
#define _CMU_IFC_CALRDY_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALRDY_DEFAULT                      (_CMU_IFC_CALRDY_DEFAULT << 5)       /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALOF                               (0x1UL << 6)                         /**< Calibration Overflow Interrupt Flag Clear */
#define _CMU_IFC_CALOF_SHIFT                        6                                    /**< Shift value for CMU_CALOF */
#define _CMU_IFC_CALOF_MASK                         0x40UL                               /**< Bit mask for CMU_CALOF */
#define _CMU_IFC_CALOF_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_CALOF_DEFAULT                       (_CMU_IFC_CALOF_DEFAULT << 6)        /**< Shifted mode DEFAULT for CMU_IFC */
#define CMU_IFC_USBCHFCLKSEL                        (0x1UL << 7)                         /**< USBC HFCLK Selected Interrupt Flag Clear */
#define _CMU_IFC_USBCHFCLKSEL_SHIFT                 7                                    /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_IFC_USBCHFCLKSEL_MASK                  0x80UL                               /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_IFC_USBCHFCLKSEL_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_IFC */
#define CMU_IFC_USBCHFCLKSEL_DEFAULT                (_CMU_IFC_USBCHFCLKSEL_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_IFC */

/* Bit fields for CMU IEN */
#define _CMU_IEN_RESETVALUE                         0x00000000UL                         /**< Default value for CMU_IEN */
#define _CMU_IEN_MASK                               0x000000FFUL                         /**< Mask for CMU_IEN */
#define CMU_IEN_HFRCORDY                            (0x1UL << 0)                         /**< HFRCO Ready Interrupt Enable */
#define _CMU_IEN_HFRCORDY_SHIFT                     0                                    /**< Shift value for CMU_HFRCORDY */
#define _CMU_IEN_HFRCORDY_MASK                      0x1UL                                /**< Bit mask for CMU_HFRCORDY */
#define _CMU_IEN_HFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFRCORDY_DEFAULT                    (_CMU_IEN_HFRCORDY_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXORDY                             (0x1UL << 1)                         /**< HFXO Ready Interrupt Enable */
#define _CMU_IEN_HFXORDY_SHIFT                      1                                    /**< Shift value for CMU_HFXORDY */
#define _CMU_IEN_HFXORDY_MASK                       0x2UL                                /**< Bit mask for CMU_HFXORDY */
#define _CMU_IEN_HFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_HFXORDY_DEFAULT                     (_CMU_IEN_HFXORDY_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFRCORDY                            (0x1UL << 2)                         /**< LFRCO Ready Interrupt Enable */
#define _CMU_IEN_LFRCORDY_SHIFT                     2                                    /**< Shift value for CMU_LFRCORDY */
#define _CMU_IEN_LFRCORDY_MASK                      0x4UL                                /**< Bit mask for CMU_LFRCORDY */
#define _CMU_IEN_LFRCORDY_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFRCORDY_DEFAULT                    (_CMU_IEN_LFRCORDY_DEFAULT << 2)     /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFXORDY                             (0x1UL << 3)                         /**< LFXO Ready Interrupt Enable */
#define _CMU_IEN_LFXORDY_SHIFT                      3                                    /**< Shift value for CMU_LFXORDY */
#define _CMU_IEN_LFXORDY_MASK                       0x8UL                                /**< Bit mask for CMU_LFXORDY */
#define _CMU_IEN_LFXORDY_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_LFXORDY_DEFAULT                     (_CMU_IEN_LFXORDY_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_AUXHFRCORDY                         (0x1UL << 4)                         /**< AUXHFRCO Ready Interrupt Enable */
#define _CMU_IEN_AUXHFRCORDY_SHIFT                  4                                    /**< Shift value for CMU_AUXHFRCORDY */
#define _CMU_IEN_AUXHFRCORDY_MASK                   0x10UL                               /**< Bit mask for CMU_AUXHFRCORDY */
#define _CMU_IEN_AUXHFRCORDY_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_AUXHFRCORDY_DEFAULT                 (_CMU_IEN_AUXHFRCORDY_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALRDY                              (0x1UL << 5)                         /**< Calibration Ready Interrupt Enable */
#define _CMU_IEN_CALRDY_SHIFT                       5                                    /**< Shift value for CMU_CALRDY */
#define _CMU_IEN_CALRDY_MASK                        0x20UL                               /**< Bit mask for CMU_CALRDY */
#define _CMU_IEN_CALRDY_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALRDY_DEFAULT                      (_CMU_IEN_CALRDY_DEFAULT << 5)       /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALOF                               (0x1UL << 6)                         /**< Calibration Overflow Interrupt Enable */
#define _CMU_IEN_CALOF_SHIFT                        6                                    /**< Shift value for CMU_CALOF */
#define _CMU_IEN_CALOF_MASK                         0x40UL                               /**< Bit mask for CMU_CALOF */
#define _CMU_IEN_CALOF_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_CALOF_DEFAULT                       (_CMU_IEN_CALOF_DEFAULT << 6)        /**< Shifted mode DEFAULT for CMU_IEN */
#define CMU_IEN_USBCHFCLKSEL                        (0x1UL << 7)                         /**< USBC HFCLK Selected Interrupt Enable */
#define _CMU_IEN_USBCHFCLKSEL_SHIFT                 7                                    /**< Shift value for CMU_USBCHFCLKSEL */
#define _CMU_IEN_USBCHFCLKSEL_MASK                  0x80UL                               /**< Bit mask for CMU_USBCHFCLKSEL */
#define _CMU_IEN_USBCHFCLKSEL_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_IEN */
#define CMU_IEN_USBCHFCLKSEL_DEFAULT                (_CMU_IEN_USBCHFCLKSEL_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_IEN */

/* Bit fields for CMU HFCORECLKEN0 */
#define _CMU_HFCORECLKEN0_RESETVALUE                0x00000000UL                          /**< Default value for CMU_HFCORECLKEN0 */
#define _CMU_HFCORECLKEN0_MASK                      0x0000003FUL                          /**< Mask for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_DMA                        (0x1UL << 0)                          /**< Direct Memory Access Controller Clock Enable */
#define _CMU_HFCORECLKEN0_DMA_SHIFT                 0                                     /**< Shift value for CMU_DMA */
#define _CMU_HFCORECLKEN0_DMA_MASK                  0x1UL                                 /**< Bit mask for CMU_DMA */
#define _CMU_HFCORECLKEN0_DMA_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_DMA_DEFAULT                (_CMU_HFCORECLKEN0_DMA_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_AES                        (0x1UL << 1)                          /**< Advanced Encryption Standard Accelerator Clock Enable */
#define _CMU_HFCORECLKEN0_AES_SHIFT                 1                                     /**< Shift value for CMU_AES */
#define _CMU_HFCORECLKEN0_AES_MASK                  0x2UL                                 /**< Bit mask for CMU_AES */
#define _CMU_HFCORECLKEN0_AES_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_AES_DEFAULT                (_CMU_HFCORECLKEN0_AES_DEFAULT << 1)  /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_USBC                       (0x1UL << 2)                          /**< Universal Serial Bus Interface Core Clock Enable */
#define _CMU_HFCORECLKEN0_USBC_SHIFT                2                                     /**< Shift value for CMU_USBC */
#define _CMU_HFCORECLKEN0_USBC_MASK                 0x4UL                                 /**< Bit mask for CMU_USBC */
#define _CMU_HFCORECLKEN0_USBC_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_USBC_DEFAULT               (_CMU_HFCORECLKEN0_USBC_DEFAULT << 2) /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_USB                        (0x1UL << 3)                          /**< Universal Serial Bus Interface Clock Enable */
#define _CMU_HFCORECLKEN0_USB_SHIFT                 3                                     /**< Shift value for CMU_USB */
#define _CMU_HFCORECLKEN0_USB_MASK                  0x8UL                                 /**< Bit mask for CMU_USB */
#define _CMU_HFCORECLKEN0_USB_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_USB_DEFAULT                (_CMU_HFCORECLKEN0_USB_DEFAULT << 3)  /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_LE                         (0x1UL << 4)                          /**< Low Energy Peripheral Interface Clock Enable */
#define _CMU_HFCORECLKEN0_LE_SHIFT                  4                                     /**< Shift value for CMU_LE */
#define _CMU_HFCORECLKEN0_LE_MASK                   0x10UL                                /**< Bit mask for CMU_LE */
#define _CMU_HFCORECLKEN0_LE_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_LE_DEFAULT                 (_CMU_HFCORECLKEN0_LE_DEFAULT << 4)   /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_EBI                        (0x1UL << 5)                          /**< External Bus Interface Clock Enable */
#define _CMU_HFCORECLKEN0_EBI_SHIFT                 5                                     /**< Shift value for CMU_EBI */
#define _CMU_HFCORECLKEN0_EBI_MASK                  0x20UL                                /**< Bit mask for CMU_EBI */
#define _CMU_HFCORECLKEN0_EBI_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for CMU_HFCORECLKEN0 */
#define CMU_HFCORECLKEN0_EBI_DEFAULT                (_CMU_HFCORECLKEN0_EBI_DEFAULT << 5)  /**< Shifted mode DEFAULT for CMU_HFCORECLKEN0 */

/* Bit fields for CMU HFPERCLKEN0 */
#define _CMU_HFPERCLKEN0_RESETVALUE                 0x00000000UL                           /**< Default value for CMU_HFPERCLKEN0 */
#define _CMU_HFPERCLKEN0_MASK                       0x0003FFFFUL                           /**< Mask for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART0                      (0x1UL << 0)                           /**< Universal Synchronous/Asynchronous Receiver/Transmitter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_USART0_SHIFT               0                                      /**< Shift value for CMU_USART0 */
#define _CMU_HFPERCLKEN0_USART0_MASK                0x1UL                                  /**< Bit mask for CMU_USART0 */
#define _CMU_HFPERCLKEN0_USART0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART0_DEFAULT              (_CMU_HFPERCLKEN0_USART0_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART1                      (0x1UL << 1)                           /**< Universal Synchronous/Asynchronous Receiver/Transmitter 1 Clock Enable */
#define _CMU_HFPERCLKEN0_USART1_SHIFT               1                                      /**< Shift value for CMU_USART1 */
#define _CMU_HFPERCLKEN0_USART1_MASK                0x2UL                                  /**< Bit mask for CMU_USART1 */
#define _CMU_HFPERCLKEN0_USART1_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART1_DEFAULT              (_CMU_HFPERCLKEN0_USART1_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART2                      (0x1UL << 2)                           /**< Universal Synchronous/Asynchronous Receiver/Transmitter 2 Clock Enable */
#define _CMU_HFPERCLKEN0_USART2_SHIFT               2                                      /**< Shift value for CMU_USART2 */
#define _CMU_HFPERCLKEN0_USART2_MASK                0x4UL                                  /**< Bit mask for CMU_USART2 */
#define _CMU_HFPERCLKEN0_USART2_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_USART2_DEFAULT              (_CMU_HFPERCLKEN0_USART2_DEFAULT << 2) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_UART0                       (0x1UL << 3)                           /**< Universal Asynchronous Receiver/Transmitter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_UART0_SHIFT                3                                      /**< Shift value for CMU_UART0 */
#define _CMU_HFPERCLKEN0_UART0_MASK                 0x8UL                                  /**< Bit mask for CMU_UART0 */
#define _CMU_HFPERCLKEN0_UART0_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_UART0_DEFAULT               (_CMU_HFPERCLKEN0_UART0_DEFAULT << 3)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_UART1                       (0x1UL << 4)                           /**< Universal Asynchronous Receiver/Transmitter 1 Clock Enable */
#define _CMU_HFPERCLKEN0_UART1_SHIFT                4                                      /**< Shift value for CMU_UART1 */
#define _CMU_HFPERCLKEN0_UART1_MASK                 0x10UL                                 /**< Bit mask for CMU_UART1 */
#define _CMU_HFPERCLKEN0_UART1_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_UART1_DEFAULT               (_CMU_HFPERCLKEN0_UART1_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER0                      (0x1UL << 5)                           /**< Timer 0 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER0_SHIFT               5                                      /**< Shift value for CMU_TIMER0 */
#define _CMU_HFPERCLKEN0_TIMER0_MASK                0x20UL                                 /**< Bit mask for CMU_TIMER0 */
#define _CMU_HFPERCLKEN0_TIMER0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER0_DEFAULT              (_CMU_HFPERCLKEN0_TIMER0_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER1                      (0x1UL << 6)                           /**< Timer 1 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER1_SHIFT               6                                      /**< Shift value for CMU_TIMER1 */
#define _CMU_HFPERCLKEN0_TIMER1_MASK                0x40UL                                 /**< Bit mask for CMU_TIMER1 */
#define _CMU_HFPERCLKEN0_TIMER1_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER1_DEFAULT              (_CMU_HFPERCLKEN0_TIMER1_DEFAULT << 6) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER2                      (0x1UL << 7)                           /**< Timer 2 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER2_SHIFT               7                                      /**< Shift value for CMU_TIMER2 */
#define _CMU_HFPERCLKEN0_TIMER2_MASK                0x80UL                                 /**< Bit mask for CMU_TIMER2 */
#define _CMU_HFPERCLKEN0_TIMER2_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER2_DEFAULT              (_CMU_HFPERCLKEN0_TIMER2_DEFAULT << 7) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER3                      (0x1UL << 8)                           /**< Timer 3 Clock Enable */
#define _CMU_HFPERCLKEN0_TIMER3_SHIFT               8                                      /**< Shift value for CMU_TIMER3 */
#define _CMU_HFPERCLKEN0_TIMER3_MASK                0x100UL                                /**< Bit mask for CMU_TIMER3 */
#define _CMU_HFPERCLKEN0_TIMER3_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_TIMER3_DEFAULT              (_CMU_HFPERCLKEN0_TIMER3_DEFAULT << 8) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP0                       (0x1UL << 9)                           /**< Analog Comparator 0 Clock Enable */
#define _CMU_HFPERCLKEN0_ACMP0_SHIFT                9                                      /**< Shift value for CMU_ACMP0 */
#define _CMU_HFPERCLKEN0_ACMP0_MASK                 0x200UL                                /**< Bit mask for CMU_ACMP0 */
#define _CMU_HFPERCLKEN0_ACMP0_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP0_DEFAULT               (_CMU_HFPERCLKEN0_ACMP0_DEFAULT << 9)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP1                       (0x1UL << 10)                          /**< Analog Comparator 1 Clock Enable */
#define _CMU_HFPERCLKEN0_ACMP1_SHIFT                10                                     /**< Shift value for CMU_ACMP1 */
#define _CMU_HFPERCLKEN0_ACMP1_MASK                 0x400UL                                /**< Bit mask for CMU_ACMP1 */
#define _CMU_HFPERCLKEN0_ACMP1_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ACMP1_DEFAULT               (_CMU_HFPERCLKEN0_ACMP1_DEFAULT << 10) /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C0                        (0x1UL << 11)                          /**< I2C 0 Clock Enable */
#define _CMU_HFPERCLKEN0_I2C0_SHIFT                 11                                     /**< Shift value for CMU_I2C0 */
#define _CMU_HFPERCLKEN0_I2C0_MASK                  0x800UL                                /**< Bit mask for CMU_I2C0 */
#define _CMU_HFPERCLKEN0_I2C0_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C0_DEFAULT                (_CMU_HFPERCLKEN0_I2C0_DEFAULT << 11)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C1                        (0x1UL << 12)                          /**< I2C 1 Clock Enable */
#define _CMU_HFPERCLKEN0_I2C1_SHIFT                 12                                     /**< Shift value for CMU_I2C1 */
#define _CMU_HFPERCLKEN0_I2C1_MASK                  0x1000UL                               /**< Bit mask for CMU_I2C1 */
#define _CMU_HFPERCLKEN0_I2C1_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_I2C1_DEFAULT                (_CMU_HFPERCLKEN0_I2C1_DEFAULT << 12)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_GPIO                        (0x1UL << 13)                          /**< General purpose Input/Output Clock Enable */
#define _CMU_HFPERCLKEN0_GPIO_SHIFT                 13                                     /**< Shift value for CMU_GPIO */
#define _CMU_HFPERCLKEN0_GPIO_MASK                  0x2000UL                               /**< Bit mask for CMU_GPIO */
#define _CMU_HFPERCLKEN0_GPIO_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_GPIO_DEFAULT                (_CMU_HFPERCLKEN0_GPIO_DEFAULT << 13)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_VCMP                        (0x1UL << 14)                          /**< Voltage Comparator Clock Enable */
#define _CMU_HFPERCLKEN0_VCMP_SHIFT                 14                                     /**< Shift value for CMU_VCMP */
#define _CMU_HFPERCLKEN0_VCMP_MASK                  0x4000UL                               /**< Bit mask for CMU_VCMP */
#define _CMU_HFPERCLKEN0_VCMP_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_VCMP_DEFAULT                (_CMU_HFPERCLKEN0_VCMP_DEFAULT << 14)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_PRS                         (0x1UL << 15)                          /**< Peripheral Reflex System Clock Enable */
#define _CMU_HFPERCLKEN0_PRS_SHIFT                  15                                     /**< Shift value for CMU_PRS */
#define _CMU_HFPERCLKEN0_PRS_MASK                   0x8000UL                               /**< Bit mask for CMU_PRS */
#define _CMU_HFPERCLKEN0_PRS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_PRS_DEFAULT                 (_CMU_HFPERCLKEN0_PRS_DEFAULT << 15)   /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ADC0                        (0x1UL << 16)                          /**< Analog to Digital Converter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_ADC0_SHIFT                 16                                     /**< Shift value for CMU_ADC0 */
#define _CMU_HFPERCLKEN0_ADC0_MASK                  0x10000UL                              /**< Bit mask for CMU_ADC0 */
#define _CMU_HFPERCLKEN0_ADC0_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_ADC0_DEFAULT                (_CMU_HFPERCLKEN0_ADC0_DEFAULT << 16)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_DAC0                        (0x1UL << 17)                          /**< Digital to Analog Converter 0 Clock Enable */
#define _CMU_HFPERCLKEN0_DAC0_SHIFT                 17                                     /**< Shift value for CMU_DAC0 */
#define _CMU_HFPERCLKEN0_DAC0_MASK                  0x20000UL                              /**< Bit mask for CMU_DAC0 */
#define _CMU_HFPERCLKEN0_DAC0_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for CMU_HFPERCLKEN0 */
#define CMU_HFPERCLKEN0_DAC0_DEFAULT                (_CMU_HFPERCLKEN0_DAC0_DEFAULT << 17)  /**< Shifted mode DEFAULT for CMU_HFPERCLKEN0 */

/* Bit fields for CMU SYNCBUSY */
#define _CMU_SYNCBUSY_RESETVALUE                    0x00000000UL                           /**< Default value for CMU_SYNCBUSY */
#define _CMU_SYNCBUSY_MASK                          0x00000055UL                           /**< Mask for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFACLKEN0                      (0x1UL << 0)                           /**< Low Frequency A Clock Enable 0 Busy */
#define _CMU_SYNCBUSY_LFACLKEN0_SHIFT               0                                      /**< Shift value for CMU_LFACLKEN0 */
#define _CMU_SYNCBUSY_LFACLKEN0_MASK                0x1UL                                  /**< Bit mask for CMU_LFACLKEN0 */
#define _CMU_SYNCBUSY_LFACLKEN0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFACLKEN0_DEFAULT              (_CMU_SYNCBUSY_LFACLKEN0_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFAPRESC0                      (0x1UL << 2)                           /**< Low Frequency A Prescaler 0 Busy */
#define _CMU_SYNCBUSY_LFAPRESC0_SHIFT               2                                      /**< Shift value for CMU_LFAPRESC0 */
#define _CMU_SYNCBUSY_LFAPRESC0_MASK                0x4UL                                  /**< Bit mask for CMU_LFAPRESC0 */
#define _CMU_SYNCBUSY_LFAPRESC0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFAPRESC0_DEFAULT              (_CMU_SYNCBUSY_LFAPRESC0_DEFAULT << 2) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBCLKEN0                      (0x1UL << 4)                           /**< Low Frequency B Clock Enable 0 Busy */
#define _CMU_SYNCBUSY_LFBCLKEN0_SHIFT               4                                      /**< Shift value for CMU_LFBCLKEN0 */
#define _CMU_SYNCBUSY_LFBCLKEN0_MASK                0x10UL                                 /**< Bit mask for CMU_LFBCLKEN0 */
#define _CMU_SYNCBUSY_LFBCLKEN0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBCLKEN0_DEFAULT              (_CMU_SYNCBUSY_LFBCLKEN0_DEFAULT << 4) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBPRESC0                      (0x1UL << 6)                           /**< Low Frequency B Prescaler 0 Busy */
#define _CMU_SYNCBUSY_LFBPRESC0_SHIFT               6                                      /**< Shift value for CMU_LFBPRESC0 */
#define _CMU_SYNCBUSY_LFBPRESC0_MASK                0x40UL                                 /**< Bit mask for CMU_LFBPRESC0 */
#define _CMU_SYNCBUSY_LFBPRESC0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_SYNCBUSY */
#define CMU_SYNCBUSY_LFBPRESC0_DEFAULT              (_CMU_SYNCBUSY_LFBPRESC0_DEFAULT << 6) /**< Shifted mode DEFAULT for CMU_SYNCBUSY */

/* Bit fields for CMU FREEZE */
#define _CMU_FREEZE_RESETVALUE                      0x00000000UL                         /**< Default value for CMU_FREEZE */
#define _CMU_FREEZE_MASK                            0x00000001UL                         /**< Mask for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE                        (0x1UL << 0)                         /**< Register Update Freeze */
#define _CMU_FREEZE_REGFREEZE_SHIFT                 0                                    /**< Shift value for CMU_REGFREEZE */
#define _CMU_FREEZE_REGFREEZE_MASK                  0x1UL                                /**< Bit mask for CMU_REGFREEZE */
#define _CMU_FREEZE_REGFREEZE_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_FREEZE */
#define _CMU_FREEZE_REGFREEZE_UPDATE                0x00000000UL                         /**< Mode UPDATE for CMU_FREEZE */
#define _CMU_FREEZE_REGFREEZE_FREEZE                0x00000001UL                         /**< Mode FREEZE for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_DEFAULT                (_CMU_FREEZE_REGFREEZE_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_UPDATE                 (_CMU_FREEZE_REGFREEZE_UPDATE << 0)  /**< Shifted mode UPDATE for CMU_FREEZE */
#define CMU_FREEZE_REGFREEZE_FREEZE                 (_CMU_FREEZE_REGFREEZE_FREEZE << 0)  /**< Shifted mode FREEZE for CMU_FREEZE */

/* Bit fields for CMU LFACLKEN0 */
#define _CMU_LFACLKEN0_RESETVALUE                   0x00000000UL                           /**< Default value for CMU_LFACLKEN0 */
#define _CMU_LFACLKEN0_MASK                         0x0000000FUL                           /**< Mask for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LESENSE                       (0x1UL << 0)                           /**< Low Energy Sensor Interface Clock Enable */
#define _CMU_LFACLKEN0_LESENSE_SHIFT                0                                      /**< Shift value for CMU_LESENSE */
#define _CMU_LFACLKEN0_LESENSE_MASK                 0x1UL                                  /**< Bit mask for CMU_LESENSE */
#define _CMU_LFACLKEN0_LESENSE_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LESENSE_DEFAULT               (_CMU_LFACLKEN0_LESENSE_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_RTC                           (0x1UL << 1)                           /**< Real-Time Counter Clock Enable */
#define _CMU_LFACLKEN0_RTC_SHIFT                    1                                      /**< Shift value for CMU_RTC */
#define _CMU_LFACLKEN0_RTC_MASK                     0x2UL                                  /**< Bit mask for CMU_RTC */
#define _CMU_LFACLKEN0_RTC_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_RTC_DEFAULT                   (_CMU_LFACLKEN0_RTC_DEFAULT << 1)      /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LETIMER0                      (0x1UL << 2)                           /**< Low Energy Timer 0 Clock Enable */
#define _CMU_LFACLKEN0_LETIMER0_SHIFT               2                                      /**< Shift value for CMU_LETIMER0 */
#define _CMU_LFACLKEN0_LETIMER0_MASK                0x4UL                                  /**< Bit mask for CMU_LETIMER0 */
#define _CMU_LFACLKEN0_LETIMER0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LETIMER0_DEFAULT              (_CMU_LFACLKEN0_LETIMER0_DEFAULT << 2) /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LCD                           (0x1UL << 3)                           /**< Liquid Crystal Display Controller Clock Enable */
#define _CMU_LFACLKEN0_LCD_SHIFT                    3                                      /**< Shift value for CMU_LCD */
#define _CMU_LFACLKEN0_LCD_MASK                     0x8UL                                  /**< Bit mask for CMU_LCD */
#define _CMU_LFACLKEN0_LCD_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for CMU_LFACLKEN0 */
#define CMU_LFACLKEN0_LCD_DEFAULT                   (_CMU_LFACLKEN0_LCD_DEFAULT << 3)      /**< Shifted mode DEFAULT for CMU_LFACLKEN0 */

/* Bit fields for CMU LFBCLKEN0 */
#define _CMU_LFBCLKEN0_RESETVALUE                   0x00000000UL                          /**< Default value for CMU_LFBCLKEN0 */
#define _CMU_LFBCLKEN0_MASK                         0x00000003UL                          /**< Mask for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART0                       (0x1UL << 0)                          /**< Low Energy UART 0 Clock Enable */
#define _CMU_LFBCLKEN0_LEUART0_SHIFT                0                                     /**< Shift value for CMU_LEUART0 */
#define _CMU_LFBCLKEN0_LEUART0_MASK                 0x1UL                                 /**< Bit mask for CMU_LEUART0 */
#define _CMU_LFBCLKEN0_LEUART0_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART0_DEFAULT               (_CMU_LFBCLKEN0_LEUART0_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART1                       (0x1UL << 1)                          /**< Low Energy UART 1 Clock Enable */
#define _CMU_LFBCLKEN0_LEUART1_SHIFT                1                                     /**< Shift value for CMU_LEUART1 */
#define _CMU_LFBCLKEN0_LEUART1_MASK                 0x2UL                                 /**< Bit mask for CMU_LEUART1 */
#define _CMU_LFBCLKEN0_LEUART1_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for CMU_LFBCLKEN0 */
#define CMU_LFBCLKEN0_LEUART1_DEFAULT               (_CMU_LFBCLKEN0_LEUART1_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_LFBCLKEN0 */

/* Bit fields for CMU LFAPRESC0 */
#define _CMU_LFAPRESC0_RESETVALUE                   0x00000000UL                            /**< Default value for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_MASK                         0x00003FF3UL                            /**< Mask for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LESENSE_SHIFT                0                                       /**< Shift value for CMU_LESENSE */
#define _CMU_LFAPRESC0_LESENSE_MASK                 0x3UL                                   /**< Bit mask for CMU_LESENSE */
#define _CMU_LFAPRESC0_LESENSE_DIV1                 0x00000000UL                            /**< Mode DIV1 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LESENSE_DIV2                 0x00000001UL                            /**< Mode DIV2 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LESENSE_DIV4                 0x00000002UL                            /**< Mode DIV4 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LESENSE_DIV8                 0x00000003UL                            /**< Mode DIV8 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LESENSE_DIV1                  (_CMU_LFAPRESC0_LESENSE_DIV1 << 0)      /**< Shifted mode DIV1 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LESENSE_DIV2                  (_CMU_LFAPRESC0_LESENSE_DIV2 << 0)      /**< Shifted mode DIV2 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LESENSE_DIV4                  (_CMU_LFAPRESC0_LESENSE_DIV4 << 0)      /**< Shifted mode DIV4 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LESENSE_DIV8                  (_CMU_LFAPRESC0_LESENSE_DIV8 << 0)      /**< Shifted mode DIV8 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_SHIFT                    4                                       /**< Shift value for CMU_RTC */
#define _CMU_LFAPRESC0_RTC_MASK                     0xF0UL                                  /**< Bit mask for CMU_RTC */
#define _CMU_LFAPRESC0_RTC_DIV1                     0x00000000UL                            /**< Mode DIV1 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV2                     0x00000001UL                            /**< Mode DIV2 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV4                     0x00000002UL                            /**< Mode DIV4 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV8                     0x00000003UL                            /**< Mode DIV8 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV16                    0x00000004UL                            /**< Mode DIV16 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV32                    0x00000005UL                            /**< Mode DIV32 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV64                    0x00000006UL                            /**< Mode DIV64 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV128                   0x00000007UL                            /**< Mode DIV128 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV256                   0x00000008UL                            /**< Mode DIV256 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV512                   0x00000009UL                            /**< Mode DIV512 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV1024                  0x0000000AUL                            /**< Mode DIV1024 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV2048                  0x0000000BUL                            /**< Mode DIV2048 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV4096                  0x0000000CUL                            /**< Mode DIV4096 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV8192                  0x0000000DUL                            /**< Mode DIV8192 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV16384                 0x0000000EUL                            /**< Mode DIV16384 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_RTC_DIV32768                 0x0000000FUL                            /**< Mode DIV32768 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV1                      (_CMU_LFAPRESC0_RTC_DIV1 << 4)          /**< Shifted mode DIV1 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV2                      (_CMU_LFAPRESC0_RTC_DIV2 << 4)          /**< Shifted mode DIV2 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV4                      (_CMU_LFAPRESC0_RTC_DIV4 << 4)          /**< Shifted mode DIV4 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV8                      (_CMU_LFAPRESC0_RTC_DIV8 << 4)          /**< Shifted mode DIV8 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV16                     (_CMU_LFAPRESC0_RTC_DIV16 << 4)         /**< Shifted mode DIV16 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV32                     (_CMU_LFAPRESC0_RTC_DIV32 << 4)         /**< Shifted mode DIV32 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV64                     (_CMU_LFAPRESC0_RTC_DIV64 << 4)         /**< Shifted mode DIV64 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV128                    (_CMU_LFAPRESC0_RTC_DIV128 << 4)        /**< Shifted mode DIV128 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV256                    (_CMU_LFAPRESC0_RTC_DIV256 << 4)        /**< Shifted mode DIV256 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV512                    (_CMU_LFAPRESC0_RTC_DIV512 << 4)        /**< Shifted mode DIV512 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV1024                   (_CMU_LFAPRESC0_RTC_DIV1024 << 4)       /**< Shifted mode DIV1024 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV2048                   (_CMU_LFAPRESC0_RTC_DIV2048 << 4)       /**< Shifted mode DIV2048 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV4096                   (_CMU_LFAPRESC0_RTC_DIV4096 << 4)       /**< Shifted mode DIV4096 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV8192                   (_CMU_LFAPRESC0_RTC_DIV8192 << 4)       /**< Shifted mode DIV8192 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV16384                  (_CMU_LFAPRESC0_RTC_DIV16384 << 4)      /**< Shifted mode DIV16384 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_RTC_DIV32768                  (_CMU_LFAPRESC0_RTC_DIV32768 << 4)      /**< Shifted mode DIV32768 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_SHIFT               8                                       /**< Shift value for CMU_LETIMER0 */
#define _CMU_LFAPRESC0_LETIMER0_MASK                0xF00UL                                 /**< Bit mask for CMU_LETIMER0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV1                0x00000000UL                            /**< Mode DIV1 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV2                0x00000001UL                            /**< Mode DIV2 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV4                0x00000002UL                            /**< Mode DIV4 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV8                0x00000003UL                            /**< Mode DIV8 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV16               0x00000004UL                            /**< Mode DIV16 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV32               0x00000005UL                            /**< Mode DIV32 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV64               0x00000006UL                            /**< Mode DIV64 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV128              0x00000007UL                            /**< Mode DIV128 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV256              0x00000008UL                            /**< Mode DIV256 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV512              0x00000009UL                            /**< Mode DIV512 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV1024             0x0000000AUL                            /**< Mode DIV1024 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV2048             0x0000000BUL                            /**< Mode DIV2048 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV4096             0x0000000CUL                            /**< Mode DIV4096 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV8192             0x0000000DUL                            /**< Mode DIV8192 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV16384            0x0000000EUL                            /**< Mode DIV16384 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LETIMER0_DIV32768            0x0000000FUL                            /**< Mode DIV32768 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV1                 (_CMU_LFAPRESC0_LETIMER0_DIV1 << 8)     /**< Shifted mode DIV1 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV2                 (_CMU_LFAPRESC0_LETIMER0_DIV2 << 8)     /**< Shifted mode DIV2 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV4                 (_CMU_LFAPRESC0_LETIMER0_DIV4 << 8)     /**< Shifted mode DIV4 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV8                 (_CMU_LFAPRESC0_LETIMER0_DIV8 << 8)     /**< Shifted mode DIV8 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV16                (_CMU_LFAPRESC0_LETIMER0_DIV16 << 8)    /**< Shifted mode DIV16 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV32                (_CMU_LFAPRESC0_LETIMER0_DIV32 << 8)    /**< Shifted mode DIV32 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV64                (_CMU_LFAPRESC0_LETIMER0_DIV64 << 8)    /**< Shifted mode DIV64 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV128               (_CMU_LFAPRESC0_LETIMER0_DIV128 << 8)   /**< Shifted mode DIV128 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV256               (_CMU_LFAPRESC0_LETIMER0_DIV256 << 8)   /**< Shifted mode DIV256 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV512               (_CMU_LFAPRESC0_LETIMER0_DIV512 << 8)   /**< Shifted mode DIV512 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV1024              (_CMU_LFAPRESC0_LETIMER0_DIV1024 << 8)  /**< Shifted mode DIV1024 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV2048              (_CMU_LFAPRESC0_LETIMER0_DIV2048 << 8)  /**< Shifted mode DIV2048 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV4096              (_CMU_LFAPRESC0_LETIMER0_DIV4096 << 8)  /**< Shifted mode DIV4096 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV8192              (_CMU_LFAPRESC0_LETIMER0_DIV8192 << 8)  /**< Shifted mode DIV8192 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV16384             (_CMU_LFAPRESC0_LETIMER0_DIV16384 << 8) /**< Shifted mode DIV16384 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LETIMER0_DIV32768             (_CMU_LFAPRESC0_LETIMER0_DIV32768 << 8) /**< Shifted mode DIV32768 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LCD_SHIFT                    12                                      /**< Shift value for CMU_LCD */
#define _CMU_LFAPRESC0_LCD_MASK                     0x3000UL                                /**< Bit mask for CMU_LCD */
#define _CMU_LFAPRESC0_LCD_DIV16                    0x00000000UL                            /**< Mode DIV16 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LCD_DIV32                    0x00000001UL                            /**< Mode DIV32 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LCD_DIV64                    0x00000002UL                            /**< Mode DIV64 for CMU_LFAPRESC0 */
#define _CMU_LFAPRESC0_LCD_DIV128                   0x00000003UL                            /**< Mode DIV128 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LCD_DIV16                     (_CMU_LFAPRESC0_LCD_DIV16 << 12)        /**< Shifted mode DIV16 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LCD_DIV32                     (_CMU_LFAPRESC0_LCD_DIV32 << 12)        /**< Shifted mode DIV32 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LCD_DIV64                     (_CMU_LFAPRESC0_LCD_DIV64 << 12)        /**< Shifted mode DIV64 for CMU_LFAPRESC0 */
#define CMU_LFAPRESC0_LCD_DIV128                    (_CMU_LFAPRESC0_LCD_DIV128 << 12)       /**< Shifted mode DIV128 for CMU_LFAPRESC0 */

/* Bit fields for CMU LFBPRESC0 */
#define _CMU_LFBPRESC0_RESETVALUE                   0x00000000UL                       /**< Default value for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_MASK                         0x00000033UL                       /**< Mask for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_SHIFT                0                                  /**< Shift value for CMU_LEUART0 */
#define _CMU_LFBPRESC0_LEUART0_MASK                 0x3UL                              /**< Bit mask for CMU_LEUART0 */
#define _CMU_LFBPRESC0_LEUART0_DIV1                 0x00000000UL                       /**< Mode DIV1 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV2                 0x00000001UL                       /**< Mode DIV2 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV4                 0x00000002UL                       /**< Mode DIV4 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART0_DIV8                 0x00000003UL                       /**< Mode DIV8 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV1                  (_CMU_LFBPRESC0_LEUART0_DIV1 << 0) /**< Shifted mode DIV1 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV2                  (_CMU_LFBPRESC0_LEUART0_DIV2 << 0) /**< Shifted mode DIV2 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV4                  (_CMU_LFBPRESC0_LEUART0_DIV4 << 0) /**< Shifted mode DIV4 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART0_DIV8                  (_CMU_LFBPRESC0_LEUART0_DIV8 << 0) /**< Shifted mode DIV8 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART1_SHIFT                4                                  /**< Shift value for CMU_LEUART1 */
#define _CMU_LFBPRESC0_LEUART1_MASK                 0x30UL                             /**< Bit mask for CMU_LEUART1 */
#define _CMU_LFBPRESC0_LEUART1_DIV1                 0x00000000UL                       /**< Mode DIV1 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART1_DIV2                 0x00000001UL                       /**< Mode DIV2 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART1_DIV4                 0x00000002UL                       /**< Mode DIV4 for CMU_LFBPRESC0 */
#define _CMU_LFBPRESC0_LEUART1_DIV8                 0x00000003UL                       /**< Mode DIV8 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART1_DIV1                  (_CMU_LFBPRESC0_LEUART1_DIV1 << 4) /**< Shifted mode DIV1 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART1_DIV2                  (_CMU_LFBPRESC0_LEUART1_DIV2 << 4) /**< Shifted mode DIV2 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART1_DIV4                  (_CMU_LFBPRESC0_LEUART1_DIV4 << 4) /**< Shifted mode DIV4 for CMU_LFBPRESC0 */
#define CMU_LFBPRESC0_LEUART1_DIV8                  (_CMU_LFBPRESC0_LEUART1_DIV8 << 4) /**< Shifted mode DIV8 for CMU_LFBPRESC0 */

/* Bit fields for CMU PCNTCTRL */
#define _CMU_PCNTCTRL_RESETVALUE                    0x00000000UL                             /**< Default value for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_MASK                          0x0000003FUL                             /**< Mask for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKEN                     (0x1UL << 0)                             /**< PCNT0 Clock Enable */
#define _CMU_PCNTCTRL_PCNT0CLKEN_SHIFT              0                                        /**< Shift value for CMU_PCNT0CLKEN */
#define _CMU_PCNTCTRL_PCNT0CLKEN_MASK               0x1UL                                    /**< Bit mask for CMU_PCNT0CLKEN */
#define _CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT             (_CMU_PCNTCTRL_PCNT0CLKEN_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL                    (0x1UL << 1)                             /**< PCNT0 Clock Select */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_SHIFT             1                                        /**< Shift value for CMU_PCNT0CLKSEL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_MASK              0x2UL                                    /**< Bit mask for CMU_PCNT0CLKSEL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK            0x00000000UL                             /**< Mode LFACLK for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0           0x00000001UL                             /**< Mode PCNT0S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT            (_CMU_PCNTCTRL_PCNT0CLKSEL_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK             (_CMU_PCNTCTRL_PCNT0CLKSEL_LFACLK << 1)  /**< Shifted mode LFACLK for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0            (_CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0 << 1) /**< Shifted mode PCNT0S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKEN                     (0x1UL << 2)                             /**< PCNT1 Clock Enable */
#define _CMU_PCNTCTRL_PCNT1CLKEN_SHIFT              2                                        /**< Shift value for CMU_PCNT1CLKEN */
#define _CMU_PCNTCTRL_PCNT1CLKEN_MASK               0x4UL                                    /**< Bit mask for CMU_PCNT1CLKEN */
#define _CMU_PCNTCTRL_PCNT1CLKEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKEN_DEFAULT             (_CMU_PCNTCTRL_PCNT1CLKEN_DEFAULT << 2)  /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKSEL                    (0x1UL << 3)                             /**< PCNT1 Clock Select */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_SHIFT             3                                        /**< Shift value for CMU_PCNT1CLKSEL */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_MASK              0x8UL                                    /**< Bit mask for CMU_PCNT1CLKSEL */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_LFACLK            0x00000000UL                             /**< Mode LFACLK for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT1CLKSEL_PCNT1S0           0x00000001UL                             /**< Mode PCNT1S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKSEL_DEFAULT            (_CMU_PCNTCTRL_PCNT1CLKSEL_DEFAULT << 3) /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKSEL_LFACLK             (_CMU_PCNTCTRL_PCNT1CLKSEL_LFACLK << 3)  /**< Shifted mode LFACLK for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT1CLKSEL_PCNT1S0            (_CMU_PCNTCTRL_PCNT1CLKSEL_PCNT1S0 << 3) /**< Shifted mode PCNT1S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKEN                     (0x1UL << 4)                             /**< PCNT2 Clock Enable */
#define _CMU_PCNTCTRL_PCNT2CLKEN_SHIFT              4                                        /**< Shift value for CMU_PCNT2CLKEN */
#define _CMU_PCNTCTRL_PCNT2CLKEN_MASK               0x10UL                                   /**< Bit mask for CMU_PCNT2CLKEN */
#define _CMU_PCNTCTRL_PCNT2CLKEN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKEN_DEFAULT             (_CMU_PCNTCTRL_PCNT2CLKEN_DEFAULT << 4)  /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKSEL                    (0x1UL << 5)                             /**< PCNT2 Clock Select */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_SHIFT             5                                        /**< Shift value for CMU_PCNT2CLKSEL */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_MASK              0x20UL                                   /**< Bit mask for CMU_PCNT2CLKSEL */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_LFACLK            0x00000000UL                             /**< Mode LFACLK for CMU_PCNTCTRL */
#define _CMU_PCNTCTRL_PCNT2CLKSEL_PCNT2S0           0x00000001UL                             /**< Mode PCNT2S0 for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKSEL_DEFAULT            (_CMU_PCNTCTRL_PCNT2CLKSEL_DEFAULT << 5) /**< Shifted mode DEFAULT for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKSEL_LFACLK             (_CMU_PCNTCTRL_PCNT2CLKSEL_LFACLK << 5)  /**< Shifted mode LFACLK for CMU_PCNTCTRL */
#define CMU_PCNTCTRL_PCNT2CLKSEL_PCNT2S0            (_CMU_PCNTCTRL_PCNT2CLKSEL_PCNT2S0 << 5) /**< Shifted mode PCNT2S0 for CMU_PCNTCTRL */

/* Bit fields for CMU LCDCTRL */
#define _CMU_LCDCTRL_RESETVALUE                     0x00000020UL                         /**< Default value for CMU_LCDCTRL */
#define _CMU_LCDCTRL_MASK                           0x0000007FUL                         /**< Mask for CMU_LCDCTRL */
#define _CMU_LCDCTRL_FDIV_SHIFT                     0                                    /**< Shift value for CMU_FDIV */
#define _CMU_LCDCTRL_FDIV_MASK                      0x7UL                                /**< Bit mask for CMU_FDIV */
#define _CMU_LCDCTRL_FDIV_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for CMU_LCDCTRL */
#define CMU_LCDCTRL_FDIV_DEFAULT                    (_CMU_LCDCTRL_FDIV_DEFAULT << 0)     /**< Shifted mode DEFAULT for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBOOSTEN                        (0x1UL << 3)                         /**< Voltage Boost Enable */
#define _CMU_LCDCTRL_VBOOSTEN_SHIFT                 3                                    /**< Shift value for CMU_VBOOSTEN */
#define _CMU_LCDCTRL_VBOOSTEN_MASK                  0x8UL                                /**< Bit mask for CMU_VBOOSTEN */
#define _CMU_LCDCTRL_VBOOSTEN_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBOOSTEN_DEFAULT                (_CMU_LCDCTRL_VBOOSTEN_DEFAULT << 3) /**< Shifted mode DEFAULT for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_SHIFT                   4                                    /**< Shift value for CMU_VBFDIV */
#define _CMU_LCDCTRL_VBFDIV_MASK                    0x70UL                               /**< Bit mask for CMU_VBFDIV */
#define _CMU_LCDCTRL_VBFDIV_DIV1                    0x00000000UL                         /**< Mode DIV1 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV2                    0x00000001UL                         /**< Mode DIV2 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DEFAULT                 0x00000002UL                         /**< Mode DEFAULT for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV4                    0x00000002UL                         /**< Mode DIV4 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV8                    0x00000003UL                         /**< Mode DIV8 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV16                   0x00000004UL                         /**< Mode DIV16 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV32                   0x00000005UL                         /**< Mode DIV32 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV64                   0x00000006UL                         /**< Mode DIV64 for CMU_LCDCTRL */
#define _CMU_LCDCTRL_VBFDIV_DIV128                  0x00000007UL                         /**< Mode DIV128 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV1                     (_CMU_LCDCTRL_VBFDIV_DIV1 << 4)      /**< Shifted mode DIV1 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV2                     (_CMU_LCDCTRL_VBFDIV_DIV2 << 4)      /**< Shifted mode DIV2 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DEFAULT                  (_CMU_LCDCTRL_VBFDIV_DEFAULT << 4)   /**< Shifted mode DEFAULT for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV4                     (_CMU_LCDCTRL_VBFDIV_DIV4 << 4)      /**< Shifted mode DIV4 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV8                     (_CMU_LCDCTRL_VBFDIV_DIV8 << 4)      /**< Shifted mode DIV8 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV16                    (_CMU_LCDCTRL_VBFDIV_DIV16 << 4)     /**< Shifted mode DIV16 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV32                    (_CMU_LCDCTRL_VBFDIV_DIV32 << 4)     /**< Shifted mode DIV32 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV64                    (_CMU_LCDCTRL_VBFDIV_DIV64 << 4)     /**< Shifted mode DIV64 for CMU_LCDCTRL */
#define CMU_LCDCTRL_VBFDIV_DIV128                   (_CMU_LCDCTRL_VBFDIV_DIV128 << 4)    /**< Shifted mode DIV128 for CMU_LCDCTRL */

/* Bit fields for CMU ROUTE */
#define _CMU_ROUTE_RESETVALUE                       0x00000000UL                         /**< Default value for CMU_ROUTE */
#define _CMU_ROUTE_MASK                             0x0000001FUL                         /**< Mask for CMU_ROUTE */
#define CMU_ROUTE_CLKOUT0PEN                        (0x1UL << 0)                         /**< CLKOUT0 Pin Enable */
#define _CMU_ROUTE_CLKOUT0PEN_SHIFT                 0                                    /**< Shift value for CMU_CLKOUT0PEN */
#define _CMU_ROUTE_CLKOUT0PEN_MASK                  0x1UL                                /**< Bit mask for CMU_CLKOUT0PEN */
#define _CMU_ROUTE_CLKOUT0PEN_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_ROUTE */
#define CMU_ROUTE_CLKOUT0PEN_DEFAULT                (_CMU_ROUTE_CLKOUT0PEN_DEFAULT << 0) /**< Shifted mode DEFAULT for CMU_ROUTE */
#define CMU_ROUTE_CLKOUT1PEN                        (0x1UL << 1)                         /**< CLKOUT1 Pin Enable */
#define _CMU_ROUTE_CLKOUT1PEN_SHIFT                 1                                    /**< Shift value for CMU_CLKOUT1PEN */
#define _CMU_ROUTE_CLKOUT1PEN_MASK                  0x2UL                                /**< Bit mask for CMU_CLKOUT1PEN */
#define _CMU_ROUTE_CLKOUT1PEN_DEFAULT               0x00000000UL                         /**< Mode DEFAULT for CMU_ROUTE */
#define CMU_ROUTE_CLKOUT1PEN_DEFAULT                (_CMU_ROUTE_CLKOUT1PEN_DEFAULT << 1) /**< Shifted mode DEFAULT for CMU_ROUTE */
#define _CMU_ROUTE_LOCATION_SHIFT                   2                                    /**< Shift value for CMU_LOCATION */
#define _CMU_ROUTE_LOCATION_MASK                    0x1CUL                               /**< Bit mask for CMU_LOCATION */
#define _CMU_ROUTE_LOCATION_LOC0                    0x00000000UL                         /**< Mode LOC0 for CMU_ROUTE */
#define _CMU_ROUTE_LOCATION_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CMU_ROUTE */
#define _CMU_ROUTE_LOCATION_LOC1                    0x00000001UL                         /**< Mode LOC1 for CMU_ROUTE */
#define _CMU_ROUTE_LOCATION_LOC2                    0x00000002UL                         /**< Mode LOC2 for CMU_ROUTE */
#define CMU_ROUTE_LOCATION_LOC0                     (_CMU_ROUTE_LOCATION_LOC0 << 2)      /**< Shifted mode LOC0 for CMU_ROUTE */
#define CMU_ROUTE_LOCATION_DEFAULT                  (_CMU_ROUTE_LOCATION_DEFAULT << 2)   /**< Shifted mode DEFAULT for CMU_ROUTE */
#define CMU_ROUTE_LOCATION_LOC1                     (_CMU_ROUTE_LOCATION_LOC1 << 2)      /**< Shifted mode LOC1 for CMU_ROUTE */
#define CMU_ROUTE_LOCATION_LOC2                     (_CMU_ROUTE_LOCATION_LOC2 << 2)      /**< Shifted mode LOC2 for CMU_ROUTE */

/* Bit fields for CMU LOCK */
#define _CMU_LOCK_RESETVALUE                        0x00000000UL                      /**< Default value for CMU_LOCK */
#define _CMU_LOCK_MASK                              0x0000FFFFUL                      /**< Mask for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_SHIFT                     0                                 /**< Shift value for CMU_LOCKKEY */
#define _CMU_LOCK_LOCKKEY_MASK                      0xFFFFUL                          /**< Bit mask for CMU_LOCKKEY */
#define _CMU_LOCK_LOCKKEY_DEFAULT                   0x00000000UL                      /**< Mode DEFAULT for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_LOCK                      0x00000000UL                      /**< Mode LOCK for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_UNLOCKED                  0x00000000UL                      /**< Mode UNLOCKED for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_LOCKED                    0x00000001UL                      /**< Mode LOCKED for CMU_LOCK */
#define _CMU_LOCK_LOCKKEY_UNLOCK                    0x0000580EUL                      /**< Mode UNLOCK for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_DEFAULT                    (_CMU_LOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_LOCK                       (_CMU_LOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_UNLOCKED                   (_CMU_LOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_LOCKED                     (_CMU_LOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for CMU_LOCK */
#define CMU_LOCK_LOCKKEY_UNLOCK                     (_CMU_LOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for CMU_LOCK */

/** @} End of group EFM32WG_CMU */
/** @} End of group Parts */

