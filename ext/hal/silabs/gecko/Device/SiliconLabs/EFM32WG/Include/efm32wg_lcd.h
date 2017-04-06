/**************************************************************************//**
 * @file efm32wg_lcd.h
 * @brief EFM32WG_LCD register and bit field definitions
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
 * @defgroup EFM32WG_LCD
 * @{
 * @brief EFM32WG_LCD Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;          /**< Control Register  */
  __IOM uint32_t DISPCTRL;      /**< Display Control Register  */
  __IOM uint32_t SEGEN;         /**< Segment Enable Register  */
  __IOM uint32_t BACTRL;        /**< Blink and Animation Control Register  */
  __IM uint32_t  STATUS;        /**< Status Register  */
  __IOM uint32_t AREGA;         /**< Animation Register A  */
  __IOM uint32_t AREGB;         /**< Animation Register B  */
  __IM uint32_t  IF;            /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;           /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;           /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;           /**< Interrupt Enable Register  */

  uint32_t       RESERVED0[5];  /**< Reserved for future use **/
  __IOM uint32_t SEGD0L;        /**< Segment Data Low Register 0  */
  __IOM uint32_t SEGD1L;        /**< Segment Data Low Register 1  */
  __IOM uint32_t SEGD2L;        /**< Segment Data Low Register 2  */
  __IOM uint32_t SEGD3L;        /**< Segment Data Low Register 3  */
  __IOM uint32_t SEGD0H;        /**< Segment Data High Register 0  */
  __IOM uint32_t SEGD1H;        /**< Segment Data High Register 1  */
  __IOM uint32_t SEGD2H;        /**< Segment Data High Register 2  */
  __IOM uint32_t SEGD3H;        /**< Segment Data High Register 3  */

  __IOM uint32_t FREEZE;        /**< Freeze Register  */
  __IM uint32_t  SYNCBUSY;      /**< Synchronization Busy Register  */

  uint32_t       RESERVED1[19]; /**< Reserved for future use **/
  __IOM uint32_t SEGD4H;        /**< Segment Data High Register 4  */
  __IOM uint32_t SEGD5H;        /**< Segment Data High Register 5  */
  __IOM uint32_t SEGD6H;        /**< Segment Data High Register 6  */
  __IOM uint32_t SEGD7H;        /**< Segment Data High Register 7  */
  uint32_t       RESERVED2[2];  /**< Reserved for future use **/
  __IOM uint32_t SEGD4L;        /**< Segment Data Low Register 4  */
  __IOM uint32_t SEGD5L;        /**< Segment Data Low Register 5  */
  __IOM uint32_t SEGD6L;        /**< Segment Data Low Register 6  */
  __IOM uint32_t SEGD7L;        /**< Segment Data Low Register 7  */
} LCD_TypeDef;                  /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_LCD_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for LCD CTRL */
#define _LCD_CTRL_RESETVALUE               0x00000000UL                       /**< Default value for LCD_CTRL */
#define _LCD_CTRL_MASK                     0x00800007UL                       /**< Mask for LCD_CTRL */
#define LCD_CTRL_EN                        (0x1UL << 0)                       /**< LCD Enable */
#define _LCD_CTRL_EN_SHIFT                 0                                  /**< Shift value for LCD_EN */
#define _LCD_CTRL_EN_MASK                  0x1UL                              /**< Bit mask for LCD_EN */
#define _LCD_CTRL_EN_DEFAULT               0x00000000UL                       /**< Mode DEFAULT for LCD_CTRL */
#define LCD_CTRL_EN_DEFAULT                (_LCD_CTRL_EN_DEFAULT << 0)        /**< Shifted mode DEFAULT for LCD_CTRL */
#define _LCD_CTRL_UDCTRL_SHIFT             1                                  /**< Shift value for LCD_UDCTRL */
#define _LCD_CTRL_UDCTRL_MASK              0x6UL                              /**< Bit mask for LCD_UDCTRL */
#define _LCD_CTRL_UDCTRL_DEFAULT           0x00000000UL                       /**< Mode DEFAULT for LCD_CTRL */
#define _LCD_CTRL_UDCTRL_REGULAR           0x00000000UL                       /**< Mode REGULAR for LCD_CTRL */
#define _LCD_CTRL_UDCTRL_FCEVENT           0x00000001UL                       /**< Mode FCEVENT for LCD_CTRL */
#define _LCD_CTRL_UDCTRL_FRAMESTART        0x00000002UL                       /**< Mode FRAMESTART for LCD_CTRL */
#define LCD_CTRL_UDCTRL_DEFAULT            (_LCD_CTRL_UDCTRL_DEFAULT << 1)    /**< Shifted mode DEFAULT for LCD_CTRL */
#define LCD_CTRL_UDCTRL_REGULAR            (_LCD_CTRL_UDCTRL_REGULAR << 1)    /**< Shifted mode REGULAR for LCD_CTRL */
#define LCD_CTRL_UDCTRL_FCEVENT            (_LCD_CTRL_UDCTRL_FCEVENT << 1)    /**< Shifted mode FCEVENT for LCD_CTRL */
#define LCD_CTRL_UDCTRL_FRAMESTART         (_LCD_CTRL_UDCTRL_FRAMESTART << 1) /**< Shifted mode FRAMESTART for LCD_CTRL */
#define LCD_CTRL_DSC                       (0x1UL << 23)                      /**< Direct Segment Control */
#define _LCD_CTRL_DSC_SHIFT                23                                 /**< Shift value for LCD_DSC */
#define _LCD_CTRL_DSC_MASK                 0x800000UL                         /**< Bit mask for LCD_DSC */
#define _LCD_CTRL_DSC_DEFAULT              0x00000000UL                       /**< Mode DEFAULT for LCD_CTRL */
#define LCD_CTRL_DSC_DEFAULT               (_LCD_CTRL_DSC_DEFAULT << 23)      /**< Shifted mode DEFAULT for LCD_CTRL */

/* Bit fields for LCD DISPCTRL */
#define _LCD_DISPCTRL_RESETVALUE           0x000C1F00UL                            /**< Default value for LCD_DISPCTRL */
#define _LCD_DISPCTRL_MASK                 0x005D9F1FUL                            /**< Mask for LCD_DISPCTRL */
#define _LCD_DISPCTRL_MUX_SHIFT            0                                       /**< Shift value for LCD_MUX */
#define _LCD_DISPCTRL_MUX_MASK             0x3UL                                   /**< Bit mask for LCD_MUX */
#define _LCD_DISPCTRL_MUX_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for LCD_DISPCTRL */
#define _LCD_DISPCTRL_MUX_STATIC           0x00000000UL                            /**< Mode STATIC for LCD_DISPCTRL */
#define _LCD_DISPCTRL_MUX_DUPLEX           0x00000001UL                            /**< Mode DUPLEX for LCD_DISPCTRL */
#define _LCD_DISPCTRL_MUX_TRIPLEX          0x00000002UL                            /**< Mode TRIPLEX for LCD_DISPCTRL */
#define _LCD_DISPCTRL_MUX_QUADRUPLEX       0x00000003UL                            /**< Mode QUADRUPLEX for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUX_DEFAULT           (_LCD_DISPCTRL_MUX_DEFAULT << 0)        /**< Shifted mode DEFAULT for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUX_STATIC            (_LCD_DISPCTRL_MUX_STATIC << 0)         /**< Shifted mode STATIC for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUX_DUPLEX            (_LCD_DISPCTRL_MUX_DUPLEX << 0)         /**< Shifted mode DUPLEX for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUX_TRIPLEX           (_LCD_DISPCTRL_MUX_TRIPLEX << 0)        /**< Shifted mode TRIPLEX for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUX_QUADRUPLEX        (_LCD_DISPCTRL_MUX_QUADRUPLEX << 0)     /**< Shifted mode QUADRUPLEX for LCD_DISPCTRL */
#define _LCD_DISPCTRL_BIAS_SHIFT           2                                       /**< Shift value for LCD_BIAS */
#define _LCD_DISPCTRL_BIAS_MASK            0xCUL                                   /**< Bit mask for LCD_BIAS */
#define _LCD_DISPCTRL_BIAS_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for LCD_DISPCTRL */
#define _LCD_DISPCTRL_BIAS_STATIC          0x00000000UL                            /**< Mode STATIC for LCD_DISPCTRL */
#define _LCD_DISPCTRL_BIAS_ONEHALF         0x00000001UL                            /**< Mode ONEHALF for LCD_DISPCTRL */
#define _LCD_DISPCTRL_BIAS_ONETHIRD        0x00000002UL                            /**< Mode ONETHIRD for LCD_DISPCTRL */
#define _LCD_DISPCTRL_BIAS_ONEFOURTH       0x00000003UL                            /**< Mode ONEFOURTH for LCD_DISPCTRL */
#define LCD_DISPCTRL_BIAS_DEFAULT          (_LCD_DISPCTRL_BIAS_DEFAULT << 2)       /**< Shifted mode DEFAULT for LCD_DISPCTRL */
#define LCD_DISPCTRL_BIAS_STATIC           (_LCD_DISPCTRL_BIAS_STATIC << 2)        /**< Shifted mode STATIC for LCD_DISPCTRL */
#define LCD_DISPCTRL_BIAS_ONEHALF          (_LCD_DISPCTRL_BIAS_ONEHALF << 2)       /**< Shifted mode ONEHALF for LCD_DISPCTRL */
#define LCD_DISPCTRL_BIAS_ONETHIRD         (_LCD_DISPCTRL_BIAS_ONETHIRD << 2)      /**< Shifted mode ONETHIRD for LCD_DISPCTRL */
#define LCD_DISPCTRL_BIAS_ONEFOURTH        (_LCD_DISPCTRL_BIAS_ONEFOURTH << 2)     /**< Shifted mode ONEFOURTH for LCD_DISPCTRL */
#define LCD_DISPCTRL_WAVE                  (0x1UL << 4)                            /**< Waveform Selection */
#define _LCD_DISPCTRL_WAVE_SHIFT           4                                       /**< Shift value for LCD_WAVE */
#define _LCD_DISPCTRL_WAVE_MASK            0x10UL                                  /**< Bit mask for LCD_WAVE */
#define _LCD_DISPCTRL_WAVE_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for LCD_DISPCTRL */
#define _LCD_DISPCTRL_WAVE_LOWPOWER        0x00000000UL                            /**< Mode LOWPOWER for LCD_DISPCTRL */
#define _LCD_DISPCTRL_WAVE_NORMAL          0x00000001UL                            /**< Mode NORMAL for LCD_DISPCTRL */
#define LCD_DISPCTRL_WAVE_DEFAULT          (_LCD_DISPCTRL_WAVE_DEFAULT << 4)       /**< Shifted mode DEFAULT for LCD_DISPCTRL */
#define LCD_DISPCTRL_WAVE_LOWPOWER         (_LCD_DISPCTRL_WAVE_LOWPOWER << 4)      /**< Shifted mode LOWPOWER for LCD_DISPCTRL */
#define LCD_DISPCTRL_WAVE_NORMAL           (_LCD_DISPCTRL_WAVE_NORMAL << 4)        /**< Shifted mode NORMAL for LCD_DISPCTRL */
#define _LCD_DISPCTRL_CONLEV_SHIFT         8                                       /**< Shift value for LCD_CONLEV */
#define _LCD_DISPCTRL_CONLEV_MASK          0x1F00UL                                /**< Bit mask for LCD_CONLEV */
#define _LCD_DISPCTRL_CONLEV_MIN           0x00000000UL                            /**< Mode MIN for LCD_DISPCTRL */
#define _LCD_DISPCTRL_CONLEV_DEFAULT       0x0000001FUL                            /**< Mode DEFAULT for LCD_DISPCTRL */
#define _LCD_DISPCTRL_CONLEV_MAX           0x0000001FUL                            /**< Mode MAX for LCD_DISPCTRL */
#define LCD_DISPCTRL_CONLEV_MIN            (_LCD_DISPCTRL_CONLEV_MIN << 8)         /**< Shifted mode MIN for LCD_DISPCTRL */
#define LCD_DISPCTRL_CONLEV_DEFAULT        (_LCD_DISPCTRL_CONLEV_DEFAULT << 8)     /**< Shifted mode DEFAULT for LCD_DISPCTRL */
#define LCD_DISPCTRL_CONLEV_MAX            (_LCD_DISPCTRL_CONLEV_MAX << 8)         /**< Shifted mode MAX for LCD_DISPCTRL */
#define LCD_DISPCTRL_CONCONF               (0x1UL << 15)                           /**< Contrast Configuration */
#define _LCD_DISPCTRL_CONCONF_SHIFT        15                                      /**< Shift value for LCD_CONCONF */
#define _LCD_DISPCTRL_CONCONF_MASK         0x8000UL                                /**< Bit mask for LCD_CONCONF */
#define _LCD_DISPCTRL_CONCONF_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for LCD_DISPCTRL */
#define _LCD_DISPCTRL_CONCONF_VLCD         0x00000000UL                            /**< Mode VLCD for LCD_DISPCTRL */
#define _LCD_DISPCTRL_CONCONF_GND          0x00000001UL                            /**< Mode GND for LCD_DISPCTRL */
#define LCD_DISPCTRL_CONCONF_DEFAULT       (_LCD_DISPCTRL_CONCONF_DEFAULT << 15)   /**< Shifted mode DEFAULT for LCD_DISPCTRL */
#define LCD_DISPCTRL_CONCONF_VLCD          (_LCD_DISPCTRL_CONCONF_VLCD << 15)      /**< Shifted mode VLCD for LCD_DISPCTRL */
#define LCD_DISPCTRL_CONCONF_GND           (_LCD_DISPCTRL_CONCONF_GND << 15)       /**< Shifted mode GND for LCD_DISPCTRL */
#define LCD_DISPCTRL_VLCDSEL               (0x1UL << 16)                           /**< VLCD Selection */
#define _LCD_DISPCTRL_VLCDSEL_SHIFT        16                                      /**< Shift value for LCD_VLCDSEL */
#define _LCD_DISPCTRL_VLCDSEL_MASK         0x10000UL                               /**< Bit mask for LCD_VLCDSEL */
#define _LCD_DISPCTRL_VLCDSEL_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VLCDSEL_VDD          0x00000000UL                            /**< Mode VDD for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VLCDSEL_VEXTBOOST    0x00000001UL                            /**< Mode VEXTBOOST for LCD_DISPCTRL */
#define LCD_DISPCTRL_VLCDSEL_DEFAULT       (_LCD_DISPCTRL_VLCDSEL_DEFAULT << 16)   /**< Shifted mode DEFAULT for LCD_DISPCTRL */
#define LCD_DISPCTRL_VLCDSEL_VDD           (_LCD_DISPCTRL_VLCDSEL_VDD << 16)       /**< Shifted mode VDD for LCD_DISPCTRL */
#define LCD_DISPCTRL_VLCDSEL_VEXTBOOST     (_LCD_DISPCTRL_VLCDSEL_VEXTBOOST << 16) /**< Shifted mode VEXTBOOST for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_SHIFT          18                                      /**< Shift value for LCD_VBLEV */
#define _LCD_DISPCTRL_VBLEV_MASK           0x1C0000UL                              /**< Bit mask for LCD_VBLEV */
#define _LCD_DISPCTRL_VBLEV_LEVEL0         0x00000000UL                            /**< Mode LEVEL0 for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_LEVEL1         0x00000001UL                            /**< Mode LEVEL1 for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_LEVEL2         0x00000002UL                            /**< Mode LEVEL2 for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_DEFAULT        0x00000003UL                            /**< Mode DEFAULT for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_LEVEL3         0x00000003UL                            /**< Mode LEVEL3 for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_LEVEL4         0x00000004UL                            /**< Mode LEVEL4 for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_LEVEL5         0x00000005UL                            /**< Mode LEVEL5 for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_LEVEL6         0x00000006UL                            /**< Mode LEVEL6 for LCD_DISPCTRL */
#define _LCD_DISPCTRL_VBLEV_LEVEL7         0x00000007UL                            /**< Mode LEVEL7 for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_LEVEL0          (_LCD_DISPCTRL_VBLEV_LEVEL0 << 18)      /**< Shifted mode LEVEL0 for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_LEVEL1          (_LCD_DISPCTRL_VBLEV_LEVEL1 << 18)      /**< Shifted mode LEVEL1 for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_LEVEL2          (_LCD_DISPCTRL_VBLEV_LEVEL2 << 18)      /**< Shifted mode LEVEL2 for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_DEFAULT         (_LCD_DISPCTRL_VBLEV_DEFAULT << 18)     /**< Shifted mode DEFAULT for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_LEVEL3          (_LCD_DISPCTRL_VBLEV_LEVEL3 << 18)      /**< Shifted mode LEVEL3 for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_LEVEL4          (_LCD_DISPCTRL_VBLEV_LEVEL4 << 18)      /**< Shifted mode LEVEL4 for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_LEVEL5          (_LCD_DISPCTRL_VBLEV_LEVEL5 << 18)      /**< Shifted mode LEVEL5 for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_LEVEL6          (_LCD_DISPCTRL_VBLEV_LEVEL6 << 18)      /**< Shifted mode LEVEL6 for LCD_DISPCTRL */
#define LCD_DISPCTRL_VBLEV_LEVEL7          (_LCD_DISPCTRL_VBLEV_LEVEL7 << 18)      /**< Shifted mode LEVEL7 for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUXE                  (0x1UL << 22)                           /**< Extended Mux Configuration */
#define _LCD_DISPCTRL_MUXE_SHIFT           22                                      /**< Shift value for LCD_MUXE */
#define _LCD_DISPCTRL_MUXE_MASK            0x400000UL                              /**< Bit mask for LCD_MUXE */
#define _LCD_DISPCTRL_MUXE_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for LCD_DISPCTRL */
#define _LCD_DISPCTRL_MUXE_MUX             0x00000000UL                            /**< Mode MUX for LCD_DISPCTRL */
#define _LCD_DISPCTRL_MUXE_MUXE            0x00000001UL                            /**< Mode MUXE for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUXE_DEFAULT          (_LCD_DISPCTRL_MUXE_DEFAULT << 22)      /**< Shifted mode DEFAULT for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUXE_MUX              (_LCD_DISPCTRL_MUXE_MUX << 22)          /**< Shifted mode MUX for LCD_DISPCTRL */
#define LCD_DISPCTRL_MUXE_MUXE             (_LCD_DISPCTRL_MUXE_MUXE << 22)         /**< Shifted mode MUXE for LCD_DISPCTRL */

/* Bit fields for LCD SEGEN */
#define _LCD_SEGEN_RESETVALUE              0x00000000UL                    /**< Default value for LCD_SEGEN */
#define _LCD_SEGEN_MASK                    0x000003FFUL                    /**< Mask for LCD_SEGEN */
#define _LCD_SEGEN_SEGEN_SHIFT             0                               /**< Shift value for LCD_SEGEN */
#define _LCD_SEGEN_SEGEN_MASK              0x3FFUL                         /**< Bit mask for LCD_SEGEN */
#define _LCD_SEGEN_SEGEN_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for LCD_SEGEN */
#define LCD_SEGEN_SEGEN_DEFAULT            (_LCD_SEGEN_SEGEN_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGEN */

/* Bit fields for LCD BACTRL */
#define _LCD_BACTRL_RESETVALUE             0x00000000UL                          /**< Default value for LCD_BACTRL */
#define _LCD_BACTRL_MASK                   0x10FF01FFUL                          /**< Mask for LCD_BACTRL */
#define LCD_BACTRL_BLINKEN                 (0x1UL << 0)                          /**< Blink Enable */
#define _LCD_BACTRL_BLINKEN_SHIFT          0                                     /**< Shift value for LCD_BLINKEN */
#define _LCD_BACTRL_BLINKEN_MASK           0x1UL                                 /**< Bit mask for LCD_BLINKEN */
#define _LCD_BACTRL_BLINKEN_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_BLINKEN_DEFAULT         (_LCD_BACTRL_BLINKEN_DEFAULT << 0)    /**< Shifted mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_BLANK                   (0x1UL << 1)                          /**< Blank Display */
#define _LCD_BACTRL_BLANK_SHIFT            1                                     /**< Shift value for LCD_BLANK */
#define _LCD_BACTRL_BLANK_MASK             0x2UL                                 /**< Bit mask for LCD_BLANK */
#define _LCD_BACTRL_BLANK_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_BLANK_DEFAULT           (_LCD_BACTRL_BLANK_DEFAULT << 1)      /**< Shifted mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_AEN                     (0x1UL << 2)                          /**< Animation Enable */
#define _LCD_BACTRL_AEN_SHIFT              2                                     /**< Shift value for LCD_AEN */
#define _LCD_BACTRL_AEN_MASK               0x4UL                                 /**< Bit mask for LCD_AEN */
#define _LCD_BACTRL_AEN_DEFAULT            0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_AEN_DEFAULT             (_LCD_BACTRL_AEN_DEFAULT << 2)        /**< Shifted mode DEFAULT for LCD_BACTRL */
#define _LCD_BACTRL_AREGASC_SHIFT          3                                     /**< Shift value for LCD_AREGASC */
#define _LCD_BACTRL_AREGASC_MASK           0x18UL                                /**< Bit mask for LCD_AREGASC */
#define _LCD_BACTRL_AREGASC_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define _LCD_BACTRL_AREGASC_NOSHIFT        0x00000000UL                          /**< Mode NOSHIFT for LCD_BACTRL */
#define _LCD_BACTRL_AREGASC_SHIFTLEFT      0x00000001UL                          /**< Mode SHIFTLEFT for LCD_BACTRL */
#define _LCD_BACTRL_AREGASC_SHIFTRIGHT     0x00000002UL                          /**< Mode SHIFTRIGHT for LCD_BACTRL */
#define LCD_BACTRL_AREGASC_DEFAULT         (_LCD_BACTRL_AREGASC_DEFAULT << 3)    /**< Shifted mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_AREGASC_NOSHIFT         (_LCD_BACTRL_AREGASC_NOSHIFT << 3)    /**< Shifted mode NOSHIFT for LCD_BACTRL */
#define LCD_BACTRL_AREGASC_SHIFTLEFT       (_LCD_BACTRL_AREGASC_SHIFTLEFT << 3)  /**< Shifted mode SHIFTLEFT for LCD_BACTRL */
#define LCD_BACTRL_AREGASC_SHIFTRIGHT      (_LCD_BACTRL_AREGASC_SHIFTRIGHT << 3) /**< Shifted mode SHIFTRIGHT for LCD_BACTRL */
#define _LCD_BACTRL_AREGBSC_SHIFT          5                                     /**< Shift value for LCD_AREGBSC */
#define _LCD_BACTRL_AREGBSC_MASK           0x60UL                                /**< Bit mask for LCD_AREGBSC */
#define _LCD_BACTRL_AREGBSC_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define _LCD_BACTRL_AREGBSC_NOSHIFT        0x00000000UL                          /**< Mode NOSHIFT for LCD_BACTRL */
#define _LCD_BACTRL_AREGBSC_SHIFTLEFT      0x00000001UL                          /**< Mode SHIFTLEFT for LCD_BACTRL */
#define _LCD_BACTRL_AREGBSC_SHIFTRIGHT     0x00000002UL                          /**< Mode SHIFTRIGHT for LCD_BACTRL */
#define LCD_BACTRL_AREGBSC_DEFAULT         (_LCD_BACTRL_AREGBSC_DEFAULT << 5)    /**< Shifted mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_AREGBSC_NOSHIFT         (_LCD_BACTRL_AREGBSC_NOSHIFT << 5)    /**< Shifted mode NOSHIFT for LCD_BACTRL */
#define LCD_BACTRL_AREGBSC_SHIFTLEFT       (_LCD_BACTRL_AREGBSC_SHIFTLEFT << 5)  /**< Shifted mode SHIFTLEFT for LCD_BACTRL */
#define LCD_BACTRL_AREGBSC_SHIFTRIGHT      (_LCD_BACTRL_AREGBSC_SHIFTRIGHT << 5) /**< Shifted mode SHIFTRIGHT for LCD_BACTRL */
#define LCD_BACTRL_ALOGSEL                 (0x1UL << 7)                          /**< Animate Logic Function Select */
#define _LCD_BACTRL_ALOGSEL_SHIFT          7                                     /**< Shift value for LCD_ALOGSEL */
#define _LCD_BACTRL_ALOGSEL_MASK           0x80UL                                /**< Bit mask for LCD_ALOGSEL */
#define _LCD_BACTRL_ALOGSEL_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define _LCD_BACTRL_ALOGSEL_AND            0x00000000UL                          /**< Mode AND for LCD_BACTRL */
#define _LCD_BACTRL_ALOGSEL_OR             0x00000001UL                          /**< Mode OR for LCD_BACTRL */
#define LCD_BACTRL_ALOGSEL_DEFAULT         (_LCD_BACTRL_ALOGSEL_DEFAULT << 7)    /**< Shifted mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_ALOGSEL_AND             (_LCD_BACTRL_ALOGSEL_AND << 7)        /**< Shifted mode AND for LCD_BACTRL */
#define LCD_BACTRL_ALOGSEL_OR              (_LCD_BACTRL_ALOGSEL_OR << 7)         /**< Shifted mode OR for LCD_BACTRL */
#define LCD_BACTRL_FCEN                    (0x1UL << 8)                          /**< Frame Counter Enable */
#define _LCD_BACTRL_FCEN_SHIFT             8                                     /**< Shift value for LCD_FCEN */
#define _LCD_BACTRL_FCEN_MASK              0x100UL                               /**< Bit mask for LCD_FCEN */
#define _LCD_BACTRL_FCEN_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_FCEN_DEFAULT            (_LCD_BACTRL_FCEN_DEFAULT << 8)       /**< Shifted mode DEFAULT for LCD_BACTRL */
#define _LCD_BACTRL_FCPRESC_SHIFT          16                                    /**< Shift value for LCD_FCPRESC */
#define _LCD_BACTRL_FCPRESC_MASK           0x30000UL                             /**< Bit mask for LCD_FCPRESC */
#define _LCD_BACTRL_FCPRESC_DEFAULT        0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define _LCD_BACTRL_FCPRESC_DIV1           0x00000000UL                          /**< Mode DIV1 for LCD_BACTRL */
#define _LCD_BACTRL_FCPRESC_DIV2           0x00000001UL                          /**< Mode DIV2 for LCD_BACTRL */
#define _LCD_BACTRL_FCPRESC_DIV4           0x00000002UL                          /**< Mode DIV4 for LCD_BACTRL */
#define _LCD_BACTRL_FCPRESC_DIV8           0x00000003UL                          /**< Mode DIV8 for LCD_BACTRL */
#define LCD_BACTRL_FCPRESC_DEFAULT         (_LCD_BACTRL_FCPRESC_DEFAULT << 16)   /**< Shifted mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_FCPRESC_DIV1            (_LCD_BACTRL_FCPRESC_DIV1 << 16)      /**< Shifted mode DIV1 for LCD_BACTRL */
#define LCD_BACTRL_FCPRESC_DIV2            (_LCD_BACTRL_FCPRESC_DIV2 << 16)      /**< Shifted mode DIV2 for LCD_BACTRL */
#define LCD_BACTRL_FCPRESC_DIV4            (_LCD_BACTRL_FCPRESC_DIV4 << 16)      /**< Shifted mode DIV4 for LCD_BACTRL */
#define LCD_BACTRL_FCPRESC_DIV8            (_LCD_BACTRL_FCPRESC_DIV8 << 16)      /**< Shifted mode DIV8 for LCD_BACTRL */
#define _LCD_BACTRL_FCTOP_SHIFT            18                                    /**< Shift value for LCD_FCTOP */
#define _LCD_BACTRL_FCTOP_MASK             0xFC0000UL                            /**< Bit mask for LCD_FCTOP */
#define _LCD_BACTRL_FCTOP_DEFAULT          0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_FCTOP_DEFAULT           (_LCD_BACTRL_FCTOP_DEFAULT << 18)     /**< Shifted mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_ALOC                    (0x1UL << 28)                         /**< Animation Location */
#define _LCD_BACTRL_ALOC_SHIFT             28                                    /**< Shift value for LCD_ALOC */
#define _LCD_BACTRL_ALOC_MASK              0x10000000UL                          /**< Bit mask for LCD_ALOC */
#define _LCD_BACTRL_ALOC_DEFAULT           0x00000000UL                          /**< Mode DEFAULT for LCD_BACTRL */
#define _LCD_BACTRL_ALOC_SEG0TO7           0x00000000UL                          /**< Mode SEG0TO7 for LCD_BACTRL */
#define _LCD_BACTRL_ALOC_SEG8TO15          0x00000001UL                          /**< Mode SEG8TO15 for LCD_BACTRL */
#define LCD_BACTRL_ALOC_DEFAULT            (_LCD_BACTRL_ALOC_DEFAULT << 28)      /**< Shifted mode DEFAULT for LCD_BACTRL */
#define LCD_BACTRL_ALOC_SEG0TO7            (_LCD_BACTRL_ALOC_SEG0TO7 << 28)      /**< Shifted mode SEG0TO7 for LCD_BACTRL */
#define LCD_BACTRL_ALOC_SEG8TO15           (_LCD_BACTRL_ALOC_SEG8TO15 << 28)     /**< Shifted mode SEG8TO15 for LCD_BACTRL */

/* Bit fields for LCD STATUS */
#define _LCD_STATUS_RESETVALUE             0x00000000UL                      /**< Default value for LCD_STATUS */
#define _LCD_STATUS_MASK                   0x0000010FUL                      /**< Mask for LCD_STATUS */
#define _LCD_STATUS_ASTATE_SHIFT           0                                 /**< Shift value for LCD_ASTATE */
#define _LCD_STATUS_ASTATE_MASK            0xFUL                             /**< Bit mask for LCD_ASTATE */
#define _LCD_STATUS_ASTATE_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_STATUS */
#define LCD_STATUS_ASTATE_DEFAULT          (_LCD_STATUS_ASTATE_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_STATUS */
#define LCD_STATUS_BLINK                   (0x1UL << 8)                      /**< Blink State */
#define _LCD_STATUS_BLINK_SHIFT            8                                 /**< Shift value for LCD_BLINK */
#define _LCD_STATUS_BLINK_MASK             0x100UL                           /**< Bit mask for LCD_BLINK */
#define _LCD_STATUS_BLINK_DEFAULT          0x00000000UL                      /**< Mode DEFAULT for LCD_STATUS */
#define LCD_STATUS_BLINK_DEFAULT           (_LCD_STATUS_BLINK_DEFAULT << 8)  /**< Shifted mode DEFAULT for LCD_STATUS */

/* Bit fields for LCD AREGA */
#define _LCD_AREGA_RESETVALUE              0x00000000UL                    /**< Default value for LCD_AREGA */
#define _LCD_AREGA_MASK                    0x000000FFUL                    /**< Mask for LCD_AREGA */
#define _LCD_AREGA_AREGA_SHIFT             0                               /**< Shift value for LCD_AREGA */
#define _LCD_AREGA_AREGA_MASK              0xFFUL                          /**< Bit mask for LCD_AREGA */
#define _LCD_AREGA_AREGA_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for LCD_AREGA */
#define LCD_AREGA_AREGA_DEFAULT            (_LCD_AREGA_AREGA_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_AREGA */

/* Bit fields for LCD AREGB */
#define _LCD_AREGB_RESETVALUE              0x00000000UL                    /**< Default value for LCD_AREGB */
#define _LCD_AREGB_MASK                    0x000000FFUL                    /**< Mask for LCD_AREGB */
#define _LCD_AREGB_AREGB_SHIFT             0                               /**< Shift value for LCD_AREGB */
#define _LCD_AREGB_AREGB_MASK              0xFFUL                          /**< Bit mask for LCD_AREGB */
#define _LCD_AREGB_AREGB_DEFAULT           0x00000000UL                    /**< Mode DEFAULT for LCD_AREGB */
#define LCD_AREGB_AREGB_DEFAULT            (_LCD_AREGB_AREGB_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_AREGB */

/* Bit fields for LCD IF */
#define _LCD_IF_RESETVALUE                 0x00000000UL              /**< Default value for LCD_IF */
#define _LCD_IF_MASK                       0x00000001UL              /**< Mask for LCD_IF */
#define LCD_IF_FC                          (0x1UL << 0)              /**< Frame Counter Interrupt Flag */
#define _LCD_IF_FC_SHIFT                   0                         /**< Shift value for LCD_FC */
#define _LCD_IF_FC_MASK                    0x1UL                     /**< Bit mask for LCD_FC */
#define _LCD_IF_FC_DEFAULT                 0x00000000UL              /**< Mode DEFAULT for LCD_IF */
#define LCD_IF_FC_DEFAULT                  (_LCD_IF_FC_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_IF */

/* Bit fields for LCD IFS */
#define _LCD_IFS_RESETVALUE                0x00000000UL               /**< Default value for LCD_IFS */
#define _LCD_IFS_MASK                      0x00000001UL               /**< Mask for LCD_IFS */
#define LCD_IFS_FC                         (0x1UL << 0)               /**< Frame Counter Interrupt Flag Set */
#define _LCD_IFS_FC_SHIFT                  0                          /**< Shift value for LCD_FC */
#define _LCD_IFS_FC_MASK                   0x1UL                      /**< Bit mask for LCD_FC */
#define _LCD_IFS_FC_DEFAULT                0x00000000UL               /**< Mode DEFAULT for LCD_IFS */
#define LCD_IFS_FC_DEFAULT                 (_LCD_IFS_FC_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_IFS */

/* Bit fields for LCD IFC */
#define _LCD_IFC_RESETVALUE                0x00000000UL               /**< Default value for LCD_IFC */
#define _LCD_IFC_MASK                      0x00000001UL               /**< Mask for LCD_IFC */
#define LCD_IFC_FC                         (0x1UL << 0)               /**< Frame Counter Interrupt Flag Clear */
#define _LCD_IFC_FC_SHIFT                  0                          /**< Shift value for LCD_FC */
#define _LCD_IFC_FC_MASK                   0x1UL                      /**< Bit mask for LCD_FC */
#define _LCD_IFC_FC_DEFAULT                0x00000000UL               /**< Mode DEFAULT for LCD_IFC */
#define LCD_IFC_FC_DEFAULT                 (_LCD_IFC_FC_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_IFC */

/* Bit fields for LCD IEN */
#define _LCD_IEN_RESETVALUE                0x00000000UL               /**< Default value for LCD_IEN */
#define _LCD_IEN_MASK                      0x00000001UL               /**< Mask for LCD_IEN */
#define LCD_IEN_FC                         (0x1UL << 0)               /**< Frame Counter Interrupt Enable */
#define _LCD_IEN_FC_SHIFT                  0                          /**< Shift value for LCD_FC */
#define _LCD_IEN_FC_MASK                   0x1UL                      /**< Bit mask for LCD_FC */
#define _LCD_IEN_FC_DEFAULT                0x00000000UL               /**< Mode DEFAULT for LCD_IEN */
#define LCD_IEN_FC_DEFAULT                 (_LCD_IEN_FC_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_IEN */

/* Bit fields for LCD SEGD0L */
#define _LCD_SEGD0L_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD0L */
#define _LCD_SEGD0L_MASK                   0xFFFFFFFFUL                      /**< Mask for LCD_SEGD0L */
#define _LCD_SEGD0L_SEGD0L_SHIFT           0                                 /**< Shift value for LCD_SEGD0L */
#define _LCD_SEGD0L_SEGD0L_MASK            0xFFFFFFFFUL                      /**< Bit mask for LCD_SEGD0L */
#define _LCD_SEGD0L_SEGD0L_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD0L */
#define LCD_SEGD0L_SEGD0L_DEFAULT          (_LCD_SEGD0L_SEGD0L_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD0L */

/* Bit fields for LCD SEGD1L */
#define _LCD_SEGD1L_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD1L */
#define _LCD_SEGD1L_MASK                   0xFFFFFFFFUL                      /**< Mask for LCD_SEGD1L */
#define _LCD_SEGD1L_SEGD1L_SHIFT           0                                 /**< Shift value for LCD_SEGD1L */
#define _LCD_SEGD1L_SEGD1L_MASK            0xFFFFFFFFUL                      /**< Bit mask for LCD_SEGD1L */
#define _LCD_SEGD1L_SEGD1L_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD1L */
#define LCD_SEGD1L_SEGD1L_DEFAULT          (_LCD_SEGD1L_SEGD1L_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD1L */

/* Bit fields for LCD SEGD2L */
#define _LCD_SEGD2L_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD2L */
#define _LCD_SEGD2L_MASK                   0xFFFFFFFFUL                      /**< Mask for LCD_SEGD2L */
#define _LCD_SEGD2L_SEGD2L_SHIFT           0                                 /**< Shift value for LCD_SEGD2L */
#define _LCD_SEGD2L_SEGD2L_MASK            0xFFFFFFFFUL                      /**< Bit mask for LCD_SEGD2L */
#define _LCD_SEGD2L_SEGD2L_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD2L */
#define LCD_SEGD2L_SEGD2L_DEFAULT          (_LCD_SEGD2L_SEGD2L_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD2L */

/* Bit fields for LCD SEGD3L */
#define _LCD_SEGD3L_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD3L */
#define _LCD_SEGD3L_MASK                   0xFFFFFFFFUL                      /**< Mask for LCD_SEGD3L */
#define _LCD_SEGD3L_SEGD3L_SHIFT           0                                 /**< Shift value for LCD_SEGD3L */
#define _LCD_SEGD3L_SEGD3L_MASK            0xFFFFFFFFUL                      /**< Bit mask for LCD_SEGD3L */
#define _LCD_SEGD3L_SEGD3L_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD3L */
#define LCD_SEGD3L_SEGD3L_DEFAULT          (_LCD_SEGD3L_SEGD3L_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD3L */

/* Bit fields for LCD SEGD0H */
#define _LCD_SEGD0H_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD0H */
#define _LCD_SEGD0H_MASK                   0x000000FFUL                      /**< Mask for LCD_SEGD0H */
#define _LCD_SEGD0H_SEGD0H_SHIFT           0                                 /**< Shift value for LCD_SEGD0H */
#define _LCD_SEGD0H_SEGD0H_MASK            0xFFUL                            /**< Bit mask for LCD_SEGD0H */
#define _LCD_SEGD0H_SEGD0H_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD0H */
#define LCD_SEGD0H_SEGD0H_DEFAULT          (_LCD_SEGD0H_SEGD0H_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD0H */

/* Bit fields for LCD SEGD1H */
#define _LCD_SEGD1H_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD1H */
#define _LCD_SEGD1H_MASK                   0x000000FFUL                      /**< Mask for LCD_SEGD1H */
#define _LCD_SEGD1H_SEGD1H_SHIFT           0                                 /**< Shift value for LCD_SEGD1H */
#define _LCD_SEGD1H_SEGD1H_MASK            0xFFUL                            /**< Bit mask for LCD_SEGD1H */
#define _LCD_SEGD1H_SEGD1H_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD1H */
#define LCD_SEGD1H_SEGD1H_DEFAULT          (_LCD_SEGD1H_SEGD1H_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD1H */

/* Bit fields for LCD SEGD2H */
#define _LCD_SEGD2H_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD2H */
#define _LCD_SEGD2H_MASK                   0x000000FFUL                      /**< Mask for LCD_SEGD2H */
#define _LCD_SEGD2H_SEGD2H_SHIFT           0                                 /**< Shift value for LCD_SEGD2H */
#define _LCD_SEGD2H_SEGD2H_MASK            0xFFUL                            /**< Bit mask for LCD_SEGD2H */
#define _LCD_SEGD2H_SEGD2H_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD2H */
#define LCD_SEGD2H_SEGD2H_DEFAULT          (_LCD_SEGD2H_SEGD2H_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD2H */

/* Bit fields for LCD SEGD3H */
#define _LCD_SEGD3H_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD3H */
#define _LCD_SEGD3H_MASK                   0x000000FFUL                      /**< Mask for LCD_SEGD3H */
#define _LCD_SEGD3H_SEGD3H_SHIFT           0                                 /**< Shift value for LCD_SEGD3H */
#define _LCD_SEGD3H_SEGD3H_MASK            0xFFUL                            /**< Bit mask for LCD_SEGD3H */
#define _LCD_SEGD3H_SEGD3H_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD3H */
#define LCD_SEGD3H_SEGD3H_DEFAULT          (_LCD_SEGD3H_SEGD3H_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD3H */

/* Bit fields for LCD FREEZE */
#define _LCD_FREEZE_RESETVALUE             0x00000000UL                         /**< Default value for LCD_FREEZE */
#define _LCD_FREEZE_MASK                   0x00000001UL                         /**< Mask for LCD_FREEZE */
#define LCD_FREEZE_REGFREEZE               (0x1UL << 0)                         /**< Register Update Freeze */
#define _LCD_FREEZE_REGFREEZE_SHIFT        0                                    /**< Shift value for LCD_REGFREEZE */
#define _LCD_FREEZE_REGFREEZE_MASK         0x1UL                                /**< Bit mask for LCD_REGFREEZE */
#define _LCD_FREEZE_REGFREEZE_DEFAULT      0x00000000UL                         /**< Mode DEFAULT for LCD_FREEZE */
#define _LCD_FREEZE_REGFREEZE_UPDATE       0x00000000UL                         /**< Mode UPDATE for LCD_FREEZE */
#define _LCD_FREEZE_REGFREEZE_FREEZE       0x00000001UL                         /**< Mode FREEZE for LCD_FREEZE */
#define LCD_FREEZE_REGFREEZE_DEFAULT       (_LCD_FREEZE_REGFREEZE_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_FREEZE */
#define LCD_FREEZE_REGFREEZE_UPDATE        (_LCD_FREEZE_REGFREEZE_UPDATE << 0)  /**< Shifted mode UPDATE for LCD_FREEZE */
#define LCD_FREEZE_REGFREEZE_FREEZE        (_LCD_FREEZE_REGFREEZE_FREEZE << 0)  /**< Shifted mode FREEZE for LCD_FREEZE */

/* Bit fields for LCD SYNCBUSY */
#define _LCD_SYNCBUSY_RESETVALUE           0x00000000UL                         /**< Default value for LCD_SYNCBUSY */
#define _LCD_SYNCBUSY_MASK                 0x000FFFFFUL                         /**< Mask for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_CTRL                  (0x1UL << 0)                         /**< CTRL Register Busy */
#define _LCD_SYNCBUSY_CTRL_SHIFT           0                                    /**< Shift value for LCD_CTRL */
#define _LCD_SYNCBUSY_CTRL_MASK            0x1UL                                /**< Bit mask for LCD_CTRL */
#define _LCD_SYNCBUSY_CTRL_DEFAULT         0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_CTRL_DEFAULT          (_LCD_SYNCBUSY_CTRL_DEFAULT << 0)    /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_BACTRL                (0x1UL << 1)                         /**< BACTRL Register Busy */
#define _LCD_SYNCBUSY_BACTRL_SHIFT         1                                    /**< Shift value for LCD_BACTRL */
#define _LCD_SYNCBUSY_BACTRL_MASK          0x2UL                                /**< Bit mask for LCD_BACTRL */
#define _LCD_SYNCBUSY_BACTRL_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_BACTRL_DEFAULT        (_LCD_SYNCBUSY_BACTRL_DEFAULT << 1)  /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_AREGA                 (0x1UL << 2)                         /**< AREGA Register Busy */
#define _LCD_SYNCBUSY_AREGA_SHIFT          2                                    /**< Shift value for LCD_AREGA */
#define _LCD_SYNCBUSY_AREGA_MASK           0x4UL                                /**< Bit mask for LCD_AREGA */
#define _LCD_SYNCBUSY_AREGA_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_AREGA_DEFAULT         (_LCD_SYNCBUSY_AREGA_DEFAULT << 2)   /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_AREGB                 (0x1UL << 3)                         /**< AREGB Register Busy */
#define _LCD_SYNCBUSY_AREGB_SHIFT          3                                    /**< Shift value for LCD_AREGB */
#define _LCD_SYNCBUSY_AREGB_MASK           0x8UL                                /**< Bit mask for LCD_AREGB */
#define _LCD_SYNCBUSY_AREGB_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_AREGB_DEFAULT         (_LCD_SYNCBUSY_AREGB_DEFAULT << 3)   /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD0L                (0x1UL << 4)                         /**< SEGD0L Register Busy */
#define _LCD_SYNCBUSY_SEGD0L_SHIFT         4                                    /**< Shift value for LCD_SEGD0L */
#define _LCD_SYNCBUSY_SEGD0L_MASK          0x10UL                               /**< Bit mask for LCD_SEGD0L */
#define _LCD_SYNCBUSY_SEGD0L_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD0L_DEFAULT        (_LCD_SYNCBUSY_SEGD0L_DEFAULT << 4)  /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD1L                (0x1UL << 5)                         /**< SEGD1L Register Busy */
#define _LCD_SYNCBUSY_SEGD1L_SHIFT         5                                    /**< Shift value for LCD_SEGD1L */
#define _LCD_SYNCBUSY_SEGD1L_MASK          0x20UL                               /**< Bit mask for LCD_SEGD1L */
#define _LCD_SYNCBUSY_SEGD1L_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD1L_DEFAULT        (_LCD_SYNCBUSY_SEGD1L_DEFAULT << 5)  /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD2L                (0x1UL << 6)                         /**< SEGD2L Register Busy */
#define _LCD_SYNCBUSY_SEGD2L_SHIFT         6                                    /**< Shift value for LCD_SEGD2L */
#define _LCD_SYNCBUSY_SEGD2L_MASK          0x40UL                               /**< Bit mask for LCD_SEGD2L */
#define _LCD_SYNCBUSY_SEGD2L_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD2L_DEFAULT        (_LCD_SYNCBUSY_SEGD2L_DEFAULT << 6)  /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD3L                (0x1UL << 7)                         /**< SEGD3L Register Busy */
#define _LCD_SYNCBUSY_SEGD3L_SHIFT         7                                    /**< Shift value for LCD_SEGD3L */
#define _LCD_SYNCBUSY_SEGD3L_MASK          0x80UL                               /**< Bit mask for LCD_SEGD3L */
#define _LCD_SYNCBUSY_SEGD3L_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD3L_DEFAULT        (_LCD_SYNCBUSY_SEGD3L_DEFAULT << 7)  /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD0H                (0x1UL << 8)                         /**< SEGD0H Register Busy */
#define _LCD_SYNCBUSY_SEGD0H_SHIFT         8                                    /**< Shift value for LCD_SEGD0H */
#define _LCD_SYNCBUSY_SEGD0H_MASK          0x100UL                              /**< Bit mask for LCD_SEGD0H */
#define _LCD_SYNCBUSY_SEGD0H_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD0H_DEFAULT        (_LCD_SYNCBUSY_SEGD0H_DEFAULT << 8)  /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD1H                (0x1UL << 9)                         /**< SEGD1H Register Busy */
#define _LCD_SYNCBUSY_SEGD1H_SHIFT         9                                    /**< Shift value for LCD_SEGD1H */
#define _LCD_SYNCBUSY_SEGD1H_MASK          0x200UL                              /**< Bit mask for LCD_SEGD1H */
#define _LCD_SYNCBUSY_SEGD1H_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD1H_DEFAULT        (_LCD_SYNCBUSY_SEGD1H_DEFAULT << 9)  /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD2H                (0x1UL << 10)                        /**< SEGD2H Register Busy */
#define _LCD_SYNCBUSY_SEGD2H_SHIFT         10                                   /**< Shift value for LCD_SEGD2H */
#define _LCD_SYNCBUSY_SEGD2H_MASK          0x400UL                              /**< Bit mask for LCD_SEGD2H */
#define _LCD_SYNCBUSY_SEGD2H_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD2H_DEFAULT        (_LCD_SYNCBUSY_SEGD2H_DEFAULT << 10) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD3H                (0x1UL << 11)                        /**< SEGD3H Register Busy */
#define _LCD_SYNCBUSY_SEGD3H_SHIFT         11                                   /**< Shift value for LCD_SEGD3H */
#define _LCD_SYNCBUSY_SEGD3H_MASK          0x800UL                              /**< Bit mask for LCD_SEGD3H */
#define _LCD_SYNCBUSY_SEGD3H_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD3H_DEFAULT        (_LCD_SYNCBUSY_SEGD3H_DEFAULT << 11) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD4H                (0x1UL << 12)                        /**< SEGD4H Register Busy */
#define _LCD_SYNCBUSY_SEGD4H_SHIFT         12                                   /**< Shift value for LCD_SEGD4H */
#define _LCD_SYNCBUSY_SEGD4H_MASK          0x1000UL                             /**< Bit mask for LCD_SEGD4H */
#define _LCD_SYNCBUSY_SEGD4H_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD4H_DEFAULT        (_LCD_SYNCBUSY_SEGD4H_DEFAULT << 12) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD5H                (0x1UL << 13)                        /**< SEGD5H Register Busy */
#define _LCD_SYNCBUSY_SEGD5H_SHIFT         13                                   /**< Shift value for LCD_SEGD5H */
#define _LCD_SYNCBUSY_SEGD5H_MASK          0x2000UL                             /**< Bit mask for LCD_SEGD5H */
#define _LCD_SYNCBUSY_SEGD5H_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD5H_DEFAULT        (_LCD_SYNCBUSY_SEGD5H_DEFAULT << 13) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD6H                (0x1UL << 14)                        /**< SEGD6H Register Busy */
#define _LCD_SYNCBUSY_SEGD6H_SHIFT         14                                   /**< Shift value for LCD_SEGD6H */
#define _LCD_SYNCBUSY_SEGD6H_MASK          0x4000UL                             /**< Bit mask for LCD_SEGD6H */
#define _LCD_SYNCBUSY_SEGD6H_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD6H_DEFAULT        (_LCD_SYNCBUSY_SEGD6H_DEFAULT << 14) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD7H                (0x1UL << 15)                        /**< SEGD7H Register Busy */
#define _LCD_SYNCBUSY_SEGD7H_SHIFT         15                                   /**< Shift value for LCD_SEGD7H */
#define _LCD_SYNCBUSY_SEGD7H_MASK          0x8000UL                             /**< Bit mask for LCD_SEGD7H */
#define _LCD_SYNCBUSY_SEGD7H_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD7H_DEFAULT        (_LCD_SYNCBUSY_SEGD7H_DEFAULT << 15) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD4L                (0x1UL << 16)                        /**< SEGD4L Register Busy */
#define _LCD_SYNCBUSY_SEGD4L_SHIFT         16                                   /**< Shift value for LCD_SEGD4L */
#define _LCD_SYNCBUSY_SEGD4L_MASK          0x10000UL                            /**< Bit mask for LCD_SEGD4L */
#define _LCD_SYNCBUSY_SEGD4L_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD4L_DEFAULT        (_LCD_SYNCBUSY_SEGD4L_DEFAULT << 16) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD5L                (0x1UL << 17)                        /**< SEGD5L Register Busy */
#define _LCD_SYNCBUSY_SEGD5L_SHIFT         17                                   /**< Shift value for LCD_SEGD5L */
#define _LCD_SYNCBUSY_SEGD5L_MASK          0x20000UL                            /**< Bit mask for LCD_SEGD5L */
#define _LCD_SYNCBUSY_SEGD5L_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD5L_DEFAULT        (_LCD_SYNCBUSY_SEGD5L_DEFAULT << 17) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD6L                (0x1UL << 18)                        /**< SEGD6L Register Busy */
#define _LCD_SYNCBUSY_SEGD6L_SHIFT         18                                   /**< Shift value for LCD_SEGD6L */
#define _LCD_SYNCBUSY_SEGD6L_MASK          0x40000UL                            /**< Bit mask for LCD_SEGD6L */
#define _LCD_SYNCBUSY_SEGD6L_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD6L_DEFAULT        (_LCD_SYNCBUSY_SEGD6L_DEFAULT << 18) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD7L                (0x1UL << 19)                        /**< SEGD7L Register Busy */
#define _LCD_SYNCBUSY_SEGD7L_SHIFT         19                                   /**< Shift value for LCD_SEGD7L */
#define _LCD_SYNCBUSY_SEGD7L_MASK          0x80000UL                            /**< Bit mask for LCD_SEGD7L */
#define _LCD_SYNCBUSY_SEGD7L_DEFAULT       0x00000000UL                         /**< Mode DEFAULT for LCD_SYNCBUSY */
#define LCD_SYNCBUSY_SEGD7L_DEFAULT        (_LCD_SYNCBUSY_SEGD7L_DEFAULT << 19) /**< Shifted mode DEFAULT for LCD_SYNCBUSY */

/* Bit fields for LCD SEGD4H */
#define _LCD_SEGD4H_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD4H */
#define _LCD_SEGD4H_MASK                   0x000000FFUL                      /**< Mask for LCD_SEGD4H */
#define _LCD_SEGD4H_SEGD4H_SHIFT           0                                 /**< Shift value for LCD_SEGD4H */
#define _LCD_SEGD4H_SEGD4H_MASK            0xFFUL                            /**< Bit mask for LCD_SEGD4H */
#define _LCD_SEGD4H_SEGD4H_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD4H */
#define LCD_SEGD4H_SEGD4H_DEFAULT          (_LCD_SEGD4H_SEGD4H_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD4H */

/* Bit fields for LCD SEGD5H */
#define _LCD_SEGD5H_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD5H */
#define _LCD_SEGD5H_MASK                   0x000000FFUL                      /**< Mask for LCD_SEGD5H */
#define _LCD_SEGD5H_SEGD5H_SHIFT           0                                 /**< Shift value for LCD_SEGD5H */
#define _LCD_SEGD5H_SEGD5H_MASK            0xFFUL                            /**< Bit mask for LCD_SEGD5H */
#define _LCD_SEGD5H_SEGD5H_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD5H */
#define LCD_SEGD5H_SEGD5H_DEFAULT          (_LCD_SEGD5H_SEGD5H_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD5H */

/* Bit fields for LCD SEGD6H */
#define _LCD_SEGD6H_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD6H */
#define _LCD_SEGD6H_MASK                   0x000000FFUL                      /**< Mask for LCD_SEGD6H */
#define _LCD_SEGD6H_SEGD6H_SHIFT           0                                 /**< Shift value for LCD_SEGD6H */
#define _LCD_SEGD6H_SEGD6H_MASK            0xFFUL                            /**< Bit mask for LCD_SEGD6H */
#define _LCD_SEGD6H_SEGD6H_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD6H */
#define LCD_SEGD6H_SEGD6H_DEFAULT          (_LCD_SEGD6H_SEGD6H_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD6H */

/* Bit fields for LCD SEGD7H */
#define _LCD_SEGD7H_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD7H */
#define _LCD_SEGD7H_MASK                   0x000000FFUL                      /**< Mask for LCD_SEGD7H */
#define _LCD_SEGD7H_SEGD7H_SHIFT           0                                 /**< Shift value for LCD_SEGD7H */
#define _LCD_SEGD7H_SEGD7H_MASK            0xFFUL                            /**< Bit mask for LCD_SEGD7H */
#define _LCD_SEGD7H_SEGD7H_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD7H */
#define LCD_SEGD7H_SEGD7H_DEFAULT          (_LCD_SEGD7H_SEGD7H_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD7H */

/* Bit fields for LCD SEGD4L */
#define _LCD_SEGD4L_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD4L */
#define _LCD_SEGD4L_MASK                   0xFFFFFFFFUL                      /**< Mask for LCD_SEGD4L */
#define _LCD_SEGD4L_SEGD4L_SHIFT           0                                 /**< Shift value for LCD_SEGD4L */
#define _LCD_SEGD4L_SEGD4L_MASK            0xFFFFFFFFUL                      /**< Bit mask for LCD_SEGD4L */
#define _LCD_SEGD4L_SEGD4L_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD4L */
#define LCD_SEGD4L_SEGD4L_DEFAULT          (_LCD_SEGD4L_SEGD4L_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD4L */

/* Bit fields for LCD SEGD5L */
#define _LCD_SEGD5L_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD5L */
#define _LCD_SEGD5L_MASK                   0xFFFFFFFFUL                      /**< Mask for LCD_SEGD5L */
#define _LCD_SEGD5L_SEGD5L_SHIFT           0                                 /**< Shift value for LCD_SEGD5L */
#define _LCD_SEGD5L_SEGD5L_MASK            0xFFFFFFFFUL                      /**< Bit mask for LCD_SEGD5L */
#define _LCD_SEGD5L_SEGD5L_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD5L */
#define LCD_SEGD5L_SEGD5L_DEFAULT          (_LCD_SEGD5L_SEGD5L_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD5L */

/* Bit fields for LCD SEGD6L */
#define _LCD_SEGD6L_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD6L */
#define _LCD_SEGD6L_MASK                   0xFFFFFFFFUL                      /**< Mask for LCD_SEGD6L */
#define _LCD_SEGD6L_SEGD6L_SHIFT           0                                 /**< Shift value for LCD_SEGD6L */
#define _LCD_SEGD6L_SEGD6L_MASK            0xFFFFFFFFUL                      /**< Bit mask for LCD_SEGD6L */
#define _LCD_SEGD6L_SEGD6L_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD6L */
#define LCD_SEGD6L_SEGD6L_DEFAULT          (_LCD_SEGD6L_SEGD6L_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD6L */

/* Bit fields for LCD SEGD7L */
#define _LCD_SEGD7L_RESETVALUE             0x00000000UL                      /**< Default value for LCD_SEGD7L */
#define _LCD_SEGD7L_MASK                   0xFFFFFFFFUL                      /**< Mask for LCD_SEGD7L */
#define _LCD_SEGD7L_SEGD7L_SHIFT           0                                 /**< Shift value for LCD_SEGD7L */
#define _LCD_SEGD7L_SEGD7L_MASK            0xFFFFFFFFUL                      /**< Bit mask for LCD_SEGD7L */
#define _LCD_SEGD7L_SEGD7L_DEFAULT         0x00000000UL                      /**< Mode DEFAULT for LCD_SEGD7L */
#define LCD_SEGD7L_SEGD7L_DEFAULT          (_LCD_SEGD7L_SEGD7L_DEFAULT << 0) /**< Shifted mode DEFAULT for LCD_SEGD7L */

/** @} End of group EFM32WG_LCD */
/** @} End of group Parts */

