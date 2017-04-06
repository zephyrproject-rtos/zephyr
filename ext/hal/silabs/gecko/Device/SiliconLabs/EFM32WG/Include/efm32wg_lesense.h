/**************************************************************************//**
 * @file efm32wg_lesense.h
 * @brief EFM32WG_LESENSE register and bit field definitions
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
 * @defgroup EFM32WG_LESENSE
 * @{
 * @brief EFM32WG_LESENSE Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t      CTRL;           /**< Control Register  */
  __IOM uint32_t      TIMCTRL;        /**< Timing Control Register  */
  __IOM uint32_t      PERCTRL;        /**< Peripheral Control Register  */
  __IOM uint32_t      DECCTRL;        /**< Decoder control Register  */
  __IOM uint32_t      BIASCTRL;       /**< Bias Control Register  */
  __IOM uint32_t      CMD;            /**< Command Register  */
  __IOM uint32_t      CHEN;           /**< Channel enable Register  */
  __IM uint32_t       SCANRES;        /**< Scan result register  */
  __IM uint32_t       STATUS;         /**< Status Register  */
  __IM uint32_t       PTR;            /**< Result buffer pointers  */
  __IM uint32_t       BUFDATA;        /**< Result buffer data register  */
  __IM uint32_t       CURCH;          /**< Current channel index  */
  __IOM uint32_t      DECSTATE;       /**< Current decoder state  */
  __IOM uint32_t      SENSORSTATE;    /**< Decoder input register  */
  __IOM uint32_t      IDLECONF;       /**< GPIO Idle phase configuration  */
  __IOM uint32_t      ALTEXCONF;      /**< Alternative excite pin configuration  */
  __IM uint32_t       IF;             /**< Interrupt Flag Register  */
  __IOM uint32_t      IFC;            /**< Interrupt Flag Clear Register  */
  __IOM uint32_t      IFS;            /**< Interrupt Flag Set Register  */
  __IOM uint32_t      IEN;            /**< Interrupt Enable Register  */
  __IM uint32_t       SYNCBUSY;       /**< Synchronization Busy Register  */
  __IOM uint32_t      ROUTE;          /**< I/O Routing Register  */
  __IOM uint32_t      POWERDOWN;      /**< LESENSE RAM power-down register  */

  uint32_t            RESERVED0[105]; /**< Reserved registers */
  LESENSE_ST_TypeDef  ST[16];         /**< Decoding states */

  LESENSE_BUF_TypeDef BUF[16];        /**< Scanresult */

  LESENSE_CH_TypeDef  CH[16];         /**< Scanconfig */
} LESENSE_TypeDef;                    /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_LESENSE_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for LESENSE CTRL */
#define _LESENSE_CTRL_RESETVALUE                       0x00000000UL                             /**< Default value for LESENSE_CTRL */
#define _LESENSE_CTRL_MASK                             0x00772EFFUL                             /**< Mask for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANMODE_SHIFT                   0                                        /**< Shift value for LESENSE_SCANMODE */
#define _LESENSE_CTRL_SCANMODE_MASK                    0x3UL                                    /**< Bit mask for LESENSE_SCANMODE */
#define _LESENSE_CTRL_SCANMODE_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANMODE_PERIODIC                0x00000000UL                             /**< Mode PERIODIC for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANMODE_ONESHOT                 0x00000001UL                             /**< Mode ONESHOT for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANMODE_PRS                     0x00000002UL                             /**< Mode PRS for LESENSE_CTRL */
#define LESENSE_CTRL_SCANMODE_DEFAULT                  (_LESENSE_CTRL_SCANMODE_DEFAULT << 0)    /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_SCANMODE_PERIODIC                 (_LESENSE_CTRL_SCANMODE_PERIODIC << 0)   /**< Shifted mode PERIODIC for LESENSE_CTRL */
#define LESENSE_CTRL_SCANMODE_ONESHOT                  (_LESENSE_CTRL_SCANMODE_ONESHOT << 0)    /**< Shifted mode ONESHOT for LESENSE_CTRL */
#define LESENSE_CTRL_SCANMODE_PRS                      (_LESENSE_CTRL_SCANMODE_PRS << 0)        /**< Shifted mode PRS for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_SHIFT                     2                                        /**< Shift value for LESENSE_PRSSEL */
#define _LESENSE_CTRL_PRSSEL_MASK                      0x3CUL                                   /**< Bit mask for LESENSE_PRSSEL */
#define _LESENSE_CTRL_PRSSEL_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH0                    0x00000000UL                             /**< Mode PRSCH0 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH1                    0x00000001UL                             /**< Mode PRSCH1 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH2                    0x00000002UL                             /**< Mode PRSCH2 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH3                    0x00000003UL                             /**< Mode PRSCH3 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH4                    0x00000004UL                             /**< Mode PRSCH4 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH5                    0x00000005UL                             /**< Mode PRSCH5 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH6                    0x00000006UL                             /**< Mode PRSCH6 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH7                    0x00000007UL                             /**< Mode PRSCH7 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH8                    0x00000008UL                             /**< Mode PRSCH8 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH9                    0x00000009UL                             /**< Mode PRSCH9 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH10                   0x0000000AUL                             /**< Mode PRSCH10 for LESENSE_CTRL */
#define _LESENSE_CTRL_PRSSEL_PRSCH11                   0x0000000BUL                             /**< Mode PRSCH11 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_DEFAULT                    (_LESENSE_CTRL_PRSSEL_DEFAULT << 2)      /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH0                     (_LESENSE_CTRL_PRSSEL_PRSCH0 << 2)       /**< Shifted mode PRSCH0 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH1                     (_LESENSE_CTRL_PRSSEL_PRSCH1 << 2)       /**< Shifted mode PRSCH1 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH2                     (_LESENSE_CTRL_PRSSEL_PRSCH2 << 2)       /**< Shifted mode PRSCH2 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH3                     (_LESENSE_CTRL_PRSSEL_PRSCH3 << 2)       /**< Shifted mode PRSCH3 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH4                     (_LESENSE_CTRL_PRSSEL_PRSCH4 << 2)       /**< Shifted mode PRSCH4 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH5                     (_LESENSE_CTRL_PRSSEL_PRSCH5 << 2)       /**< Shifted mode PRSCH5 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH6                     (_LESENSE_CTRL_PRSSEL_PRSCH6 << 2)       /**< Shifted mode PRSCH6 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH7                     (_LESENSE_CTRL_PRSSEL_PRSCH7 << 2)       /**< Shifted mode PRSCH7 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH8                     (_LESENSE_CTRL_PRSSEL_PRSCH8 << 2)       /**< Shifted mode PRSCH8 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH9                     (_LESENSE_CTRL_PRSSEL_PRSCH9 << 2)       /**< Shifted mode PRSCH9 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH10                    (_LESENSE_CTRL_PRSSEL_PRSCH10 << 2)      /**< Shifted mode PRSCH10 for LESENSE_CTRL */
#define LESENSE_CTRL_PRSSEL_PRSCH11                    (_LESENSE_CTRL_PRSSEL_PRSCH11 << 2)      /**< Shifted mode PRSCH11 for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_SHIFT                   6                                        /**< Shift value for LESENSE_SCANCONF */
#define _LESENSE_CTRL_SCANCONF_MASK                    0xC0UL                                   /**< Bit mask for LESENSE_SCANCONF */
#define _LESENSE_CTRL_SCANCONF_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_DIRMAP                  0x00000000UL                             /**< Mode DIRMAP for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_INVMAP                  0x00000001UL                             /**< Mode INVMAP for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_TOGGLE                  0x00000002UL                             /**< Mode TOGGLE for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_DECDEF                  0x00000003UL                             /**< Mode DECDEF for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_DEFAULT                  (_LESENSE_CTRL_SCANCONF_DEFAULT << 6)    /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_DIRMAP                   (_LESENSE_CTRL_SCANCONF_DIRMAP << 6)     /**< Shifted mode DIRMAP for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_INVMAP                   (_LESENSE_CTRL_SCANCONF_INVMAP << 6)     /**< Shifted mode INVMAP for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_TOGGLE                   (_LESENSE_CTRL_SCANCONF_TOGGLE << 6)     /**< Shifted mode TOGGLE for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_DECDEF                   (_LESENSE_CTRL_SCANCONF_DECDEF << 6)     /**< Shifted mode DECDEF for LESENSE_CTRL */
#define LESENSE_CTRL_ACMP0INV                          (0x1UL << 9)                             /**< Invert analog comparator 0 output */
#define _LESENSE_CTRL_ACMP0INV_SHIFT                   9                                        /**< Shift value for LESENSE_ACMP0INV */
#define _LESENSE_CTRL_ACMP0INV_MASK                    0x200UL                                  /**< Bit mask for LESENSE_ACMP0INV */
#define _LESENSE_CTRL_ACMP0INV_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_ACMP0INV_DEFAULT                  (_LESENSE_CTRL_ACMP0INV_DEFAULT << 9)    /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_ACMP1INV                          (0x1UL << 10)                            /**< Invert analog comparator 1 output */
#define _LESENSE_CTRL_ACMP1INV_SHIFT                   10                                       /**< Shift value for LESENSE_ACMP1INV */
#define _LESENSE_CTRL_ACMP1INV_MASK                    0x400UL                                  /**< Bit mask for LESENSE_ACMP1INV */
#define _LESENSE_CTRL_ACMP1INV_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_ACMP1INV_DEFAULT                  (_LESENSE_CTRL_ACMP1INV_DEFAULT << 10)   /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_ALTEXMAP                          (0x1UL << 11)                            /**< Alternative excitation map */
#define _LESENSE_CTRL_ALTEXMAP_SHIFT                   11                                       /**< Shift value for LESENSE_ALTEXMAP */
#define _LESENSE_CTRL_ALTEXMAP_MASK                    0x800UL                                  /**< Bit mask for LESENSE_ALTEXMAP */
#define _LESENSE_CTRL_ALTEXMAP_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_ALTEXMAP_ALTEX                   0x00000000UL                             /**< Mode ALTEX for LESENSE_CTRL */
#define _LESENSE_CTRL_ALTEXMAP_ACMP                    0x00000001UL                             /**< Mode ACMP for LESENSE_CTRL */
#define LESENSE_CTRL_ALTEXMAP_DEFAULT                  (_LESENSE_CTRL_ALTEXMAP_DEFAULT << 11)   /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_ALTEXMAP_ALTEX                    (_LESENSE_CTRL_ALTEXMAP_ALTEX << 11)     /**< Shifted mode ALTEX for LESENSE_CTRL */
#define LESENSE_CTRL_ALTEXMAP_ACMP                     (_LESENSE_CTRL_ALTEXMAP_ACMP << 11)      /**< Shifted mode ACMP for LESENSE_CTRL */
#define LESENSE_CTRL_DUALSAMPLE                        (0x1UL << 13)                            /**< Enable dual sample mode */
#define _LESENSE_CTRL_DUALSAMPLE_SHIFT                 13                                       /**< Shift value for LESENSE_DUALSAMPLE */
#define _LESENSE_CTRL_DUALSAMPLE_MASK                  0x2000UL                                 /**< Bit mask for LESENSE_DUALSAMPLE */
#define _LESENSE_CTRL_DUALSAMPLE_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_DUALSAMPLE_DEFAULT                (_LESENSE_CTRL_DUALSAMPLE_DEFAULT << 13) /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_BUFOW                             (0x1UL << 16)                            /**< Result buffer overwrite */
#define _LESENSE_CTRL_BUFOW_SHIFT                      16                                       /**< Shift value for LESENSE_BUFOW */
#define _LESENSE_CTRL_BUFOW_MASK                       0x10000UL                                /**< Bit mask for LESENSE_BUFOW */
#define _LESENSE_CTRL_BUFOW_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_BUFOW_DEFAULT                     (_LESENSE_CTRL_BUFOW_DEFAULT << 16)      /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_STRSCANRES                        (0x1UL << 17)                            /**< Enable storing of SCANRES */
#define _LESENSE_CTRL_STRSCANRES_SHIFT                 17                                       /**< Shift value for LESENSE_STRSCANRES */
#define _LESENSE_CTRL_STRSCANRES_MASK                  0x20000UL                                /**< Bit mask for LESENSE_STRSCANRES */
#define _LESENSE_CTRL_STRSCANRES_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_STRSCANRES_DEFAULT                (_LESENSE_CTRL_STRSCANRES_DEFAULT << 17) /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_BUFIDL                            (0x1UL << 18)                            /**< Result buffer interrupt and DMA trigger level */
#define _LESENSE_CTRL_BUFIDL_SHIFT                     18                                       /**< Shift value for LESENSE_BUFIDL */
#define _LESENSE_CTRL_BUFIDL_MASK                      0x40000UL                                /**< Bit mask for LESENSE_BUFIDL */
#define _LESENSE_CTRL_BUFIDL_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_BUFIDL_HALFFULL                  0x00000000UL                             /**< Mode HALFFULL for LESENSE_CTRL */
#define _LESENSE_CTRL_BUFIDL_FULL                      0x00000001UL                             /**< Mode FULL for LESENSE_CTRL */
#define LESENSE_CTRL_BUFIDL_DEFAULT                    (_LESENSE_CTRL_BUFIDL_DEFAULT << 18)     /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_BUFIDL_HALFFULL                   (_LESENSE_CTRL_BUFIDL_HALFFULL << 18)    /**< Shifted mode HALFFULL for LESENSE_CTRL */
#define LESENSE_CTRL_BUFIDL_FULL                       (_LESENSE_CTRL_BUFIDL_FULL << 18)        /**< Shifted mode FULL for LESENSE_CTRL */
#define _LESENSE_CTRL_DMAWU_SHIFT                      20                                       /**< Shift value for LESENSE_DMAWU */
#define _LESENSE_CTRL_DMAWU_MASK                       0x300000UL                               /**< Bit mask for LESENSE_DMAWU */
#define _LESENSE_CTRL_DMAWU_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_DMAWU_DISABLE                    0x00000000UL                             /**< Mode DISABLE for LESENSE_CTRL */
#define _LESENSE_CTRL_DMAWU_BUFDATAV                   0x00000001UL                             /**< Mode BUFDATAV for LESENSE_CTRL */
#define _LESENSE_CTRL_DMAWU_BUFLEVEL                   0x00000002UL                             /**< Mode BUFLEVEL for LESENSE_CTRL */
#define LESENSE_CTRL_DMAWU_DEFAULT                     (_LESENSE_CTRL_DMAWU_DEFAULT << 20)      /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_DMAWU_DISABLE                     (_LESENSE_CTRL_DMAWU_DISABLE << 20)      /**< Shifted mode DISABLE for LESENSE_CTRL */
#define LESENSE_CTRL_DMAWU_BUFDATAV                    (_LESENSE_CTRL_DMAWU_BUFDATAV << 20)     /**< Shifted mode BUFDATAV for LESENSE_CTRL */
#define LESENSE_CTRL_DMAWU_BUFLEVEL                    (_LESENSE_CTRL_DMAWU_BUFLEVEL << 20)     /**< Shifted mode BUFLEVEL for LESENSE_CTRL */
#define LESENSE_CTRL_DEBUGRUN                          (0x1UL << 22)                            /**< Debug Mode Run Enable */
#define _LESENSE_CTRL_DEBUGRUN_SHIFT                   22                                       /**< Shift value for LESENSE_DEBUGRUN */
#define _LESENSE_CTRL_DEBUGRUN_MASK                    0x400000UL                               /**< Bit mask for LESENSE_DEBUGRUN */
#define _LESENSE_CTRL_DEBUGRUN_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_DEBUGRUN_DEFAULT                  (_LESENSE_CTRL_DEBUGRUN_DEFAULT << 22)   /**< Shifted mode DEFAULT for LESENSE_CTRL */

/* Bit fields for LESENSE TIMCTRL */
#define _LESENSE_TIMCTRL_RESETVALUE                    0x00000000UL                              /**< Default value for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_MASK                          0x00CFF773UL                              /**< Mask for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_SHIFT                0                                         /**< Shift value for LESENSE_AUXPRESC */
#define _LESENSE_TIMCTRL_AUXPRESC_MASK                 0x3UL                                     /**< Bit mask for LESENSE_AUXPRESC */
#define _LESENSE_TIMCTRL_AUXPRESC_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_DIV1                 0x00000000UL                              /**< Mode DIV1 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_DIV2                 0x00000001UL                              /**< Mode DIV2 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_DIV4                 0x00000002UL                              /**< Mode DIV4 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_DIV8                 0x00000003UL                              /**< Mode DIV8 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DEFAULT               (_LESENSE_TIMCTRL_AUXPRESC_DEFAULT << 0)  /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DIV1                  (_LESENSE_TIMCTRL_AUXPRESC_DIV1 << 0)     /**< Shifted mode DIV1 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DIV2                  (_LESENSE_TIMCTRL_AUXPRESC_DIV2 << 0)     /**< Shifted mode DIV2 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DIV4                  (_LESENSE_TIMCTRL_AUXPRESC_DIV4 << 0)     /**< Shifted mode DIV4 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DIV8                  (_LESENSE_TIMCTRL_AUXPRESC_DIV8 << 0)     /**< Shifted mode DIV8 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_SHIFT                 4                                         /**< Shift value for LESENSE_LFPRESC */
#define _LESENSE_TIMCTRL_LFPRESC_MASK                  0x70UL                                    /**< Bit mask for LESENSE_LFPRESC */
#define _LESENSE_TIMCTRL_LFPRESC_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV1                  0x00000000UL                              /**< Mode DIV1 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV2                  0x00000001UL                              /**< Mode DIV2 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV4                  0x00000002UL                              /**< Mode DIV4 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV8                  0x00000003UL                              /**< Mode DIV8 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV16                 0x00000004UL                              /**< Mode DIV16 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV32                 0x00000005UL                              /**< Mode DIV32 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV64                 0x00000006UL                              /**< Mode DIV64 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV128                0x00000007UL                              /**< Mode DIV128 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DEFAULT                (_LESENSE_TIMCTRL_LFPRESC_DEFAULT << 4)   /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV1                   (_LESENSE_TIMCTRL_LFPRESC_DIV1 << 4)      /**< Shifted mode DIV1 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV2                   (_LESENSE_TIMCTRL_LFPRESC_DIV2 << 4)      /**< Shifted mode DIV2 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV4                   (_LESENSE_TIMCTRL_LFPRESC_DIV4 << 4)      /**< Shifted mode DIV4 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV8                   (_LESENSE_TIMCTRL_LFPRESC_DIV8 << 4)      /**< Shifted mode DIV8 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV16                  (_LESENSE_TIMCTRL_LFPRESC_DIV16 << 4)     /**< Shifted mode DIV16 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV32                  (_LESENSE_TIMCTRL_LFPRESC_DIV32 << 4)     /**< Shifted mode DIV32 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV64                  (_LESENSE_TIMCTRL_LFPRESC_DIV64 << 4)     /**< Shifted mode DIV64 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV128                 (_LESENSE_TIMCTRL_LFPRESC_DIV128 << 4)    /**< Shifted mode DIV128 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_SHIFT                 8                                         /**< Shift value for LESENSE_PCPRESC */
#define _LESENSE_TIMCTRL_PCPRESC_MASK                  0x700UL                                   /**< Bit mask for LESENSE_PCPRESC */
#define _LESENSE_TIMCTRL_PCPRESC_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV1                  0x00000000UL                              /**< Mode DIV1 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV2                  0x00000001UL                              /**< Mode DIV2 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV4                  0x00000002UL                              /**< Mode DIV4 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV8                  0x00000003UL                              /**< Mode DIV8 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV16                 0x00000004UL                              /**< Mode DIV16 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV32                 0x00000005UL                              /**< Mode DIV32 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV64                 0x00000006UL                              /**< Mode DIV64 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV128                0x00000007UL                              /**< Mode DIV128 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DEFAULT                (_LESENSE_TIMCTRL_PCPRESC_DEFAULT << 8)   /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV1                   (_LESENSE_TIMCTRL_PCPRESC_DIV1 << 8)      /**< Shifted mode DIV1 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV2                   (_LESENSE_TIMCTRL_PCPRESC_DIV2 << 8)      /**< Shifted mode DIV2 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV4                   (_LESENSE_TIMCTRL_PCPRESC_DIV4 << 8)      /**< Shifted mode DIV4 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV8                   (_LESENSE_TIMCTRL_PCPRESC_DIV8 << 8)      /**< Shifted mode DIV8 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV16                  (_LESENSE_TIMCTRL_PCPRESC_DIV16 << 8)     /**< Shifted mode DIV16 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV32                  (_LESENSE_TIMCTRL_PCPRESC_DIV32 << 8)     /**< Shifted mode DIV32 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV64                  (_LESENSE_TIMCTRL_PCPRESC_DIV64 << 8)     /**< Shifted mode DIV64 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV128                 (_LESENSE_TIMCTRL_PCPRESC_DIV128 << 8)    /**< Shifted mode DIV128 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCTOP_SHIFT                   12                                        /**< Shift value for LESENSE_PCTOP */
#define _LESENSE_TIMCTRL_PCTOP_MASK                    0xFF000UL                                 /**< Bit mask for LESENSE_PCTOP */
#define _LESENSE_TIMCTRL_PCTOP_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCTOP_DEFAULT                  (_LESENSE_TIMCTRL_PCTOP_DEFAULT << 12)    /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_STARTDLY_SHIFT                22                                        /**< Shift value for LESENSE_STARTDLY */
#define _LESENSE_TIMCTRL_STARTDLY_MASK                 0xC00000UL                                /**< Bit mask for LESENSE_STARTDLY */
#define _LESENSE_TIMCTRL_STARTDLY_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_STARTDLY_DEFAULT               (_LESENSE_TIMCTRL_STARTDLY_DEFAULT << 22) /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */

/* Bit fields for LESENSE PERCTRL */
#define _LESENSE_PERCTRL_RESETVALUE                    0x00000000UL                                        /**< Default value for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_MASK                          0x0CF47FFFUL                                        /**< Mask for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0DATA                     (0x1UL << 0)                                        /**< DAC CH0 data selection. */
#define _LESENSE_PERCTRL_DACCH0DATA_SHIFT              0                                                   /**< Shift value for LESENSE_DACCH0DATA */
#define _LESENSE_PERCTRL_DACCH0DATA_MASK               0x1UL                                               /**< Bit mask for LESENSE_DACCH0DATA */
#define _LESENSE_PERCTRL_DACCH0DATA_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0DATA_DACDATA            0x00000000UL                                        /**< Mode DACDATA for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0DATA_ACMPTHRES          0x00000001UL                                        /**< Mode ACMPTHRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0DATA_DEFAULT             (_LESENSE_PERCTRL_DACCH0DATA_DEFAULT << 0)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0DATA_DACDATA             (_LESENSE_PERCTRL_DACCH0DATA_DACDATA << 0)          /**< Shifted mode DACDATA for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0DATA_ACMPTHRES           (_LESENSE_PERCTRL_DACCH0DATA_ACMPTHRES << 0)        /**< Shifted mode ACMPTHRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1DATA                     (0x1UL << 1)                                        /**< DAC CH1 data selection. */
#define _LESENSE_PERCTRL_DACCH1DATA_SHIFT              1                                                   /**< Shift value for LESENSE_DACCH1DATA */
#define _LESENSE_PERCTRL_DACCH1DATA_MASK               0x2UL                                               /**< Bit mask for LESENSE_DACCH1DATA */
#define _LESENSE_PERCTRL_DACCH1DATA_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1DATA_DACDATA            0x00000000UL                                        /**< Mode DACDATA for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1DATA_ACMPTHRES          0x00000001UL                                        /**< Mode ACMPTHRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1DATA_DEFAULT             (_LESENSE_PERCTRL_DACCH1DATA_DEFAULT << 1)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1DATA_DACDATA             (_LESENSE_PERCTRL_DACCH1DATA_DACDATA << 1)          /**< Shifted mode DACDATA for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1DATA_ACMPTHRES           (_LESENSE_PERCTRL_DACCH1DATA_ACMPTHRES << 1)        /**< Shifted mode ACMPTHRES for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0CONV_SHIFT              2                                                   /**< Shift value for LESENSE_DACCH0CONV */
#define _LESENSE_PERCTRL_DACCH0CONV_MASK               0xCUL                                               /**< Bit mask for LESENSE_DACCH0CONV */
#define _LESENSE_PERCTRL_DACCH0CONV_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0CONV_DISABLE            0x00000000UL                                        /**< Mode DISABLE for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0CONV_CONTINUOUS         0x00000001UL                                        /**< Mode CONTINUOUS for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0CONV_SAMPLEHOLD         0x00000002UL                                        /**< Mode SAMPLEHOLD for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0CONV_SAMPLEOFF          0x00000003UL                                        /**< Mode SAMPLEOFF for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0CONV_DEFAULT             (_LESENSE_PERCTRL_DACCH0CONV_DEFAULT << 2)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0CONV_DISABLE             (_LESENSE_PERCTRL_DACCH0CONV_DISABLE << 2)          /**< Shifted mode DISABLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0CONV_CONTINUOUS          (_LESENSE_PERCTRL_DACCH0CONV_CONTINUOUS << 2)       /**< Shifted mode CONTINUOUS for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0CONV_SAMPLEHOLD          (_LESENSE_PERCTRL_DACCH0CONV_SAMPLEHOLD << 2)       /**< Shifted mode SAMPLEHOLD for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0CONV_SAMPLEOFF           (_LESENSE_PERCTRL_DACCH0CONV_SAMPLEOFF << 2)        /**< Shifted mode SAMPLEOFF for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1CONV_SHIFT              4                                                   /**< Shift value for LESENSE_DACCH1CONV */
#define _LESENSE_PERCTRL_DACCH1CONV_MASK               0x30UL                                              /**< Bit mask for LESENSE_DACCH1CONV */
#define _LESENSE_PERCTRL_DACCH1CONV_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1CONV_DISABLE            0x00000000UL                                        /**< Mode DISABLE for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1CONV_CONTINUOUS         0x00000001UL                                        /**< Mode CONTINUOUS for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1CONV_SAMPLEHOLD         0x00000002UL                                        /**< Mode SAMPLEHOLD for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1CONV_SAMPLEOFF          0x00000003UL                                        /**< Mode SAMPLEOFF for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1CONV_DEFAULT             (_LESENSE_PERCTRL_DACCH1CONV_DEFAULT << 4)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1CONV_DISABLE             (_LESENSE_PERCTRL_DACCH1CONV_DISABLE << 4)          /**< Shifted mode DISABLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1CONV_CONTINUOUS          (_LESENSE_PERCTRL_DACCH1CONV_CONTINUOUS << 4)       /**< Shifted mode CONTINUOUS for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1CONV_SAMPLEHOLD          (_LESENSE_PERCTRL_DACCH1CONV_SAMPLEHOLD << 4)       /**< Shifted mode SAMPLEHOLD for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1CONV_SAMPLEOFF           (_LESENSE_PERCTRL_DACCH1CONV_SAMPLEOFF << 4)        /**< Shifted mode SAMPLEOFF for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0OUT_SHIFT               6                                                   /**< Shift value for LESENSE_DACCH0OUT */
#define _LESENSE_PERCTRL_DACCH0OUT_MASK                0xC0UL                                              /**< Bit mask for LESENSE_DACCH0OUT */
#define _LESENSE_PERCTRL_DACCH0OUT_DEFAULT             0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0OUT_DISABLE             0x00000000UL                                        /**< Mode DISABLE for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0OUT_PIN                 0x00000001UL                                        /**< Mode PIN for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0OUT_ADCACMP             0x00000002UL                                        /**< Mode ADCACMP for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0OUT_PINADCACMP          0x00000003UL                                        /**< Mode PINADCACMP for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0OUT_DEFAULT              (_LESENSE_PERCTRL_DACCH0OUT_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0OUT_DISABLE              (_LESENSE_PERCTRL_DACCH0OUT_DISABLE << 6)           /**< Shifted mode DISABLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0OUT_PIN                  (_LESENSE_PERCTRL_DACCH0OUT_PIN << 6)               /**< Shifted mode PIN for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0OUT_ADCACMP              (_LESENSE_PERCTRL_DACCH0OUT_ADCACMP << 6)           /**< Shifted mode ADCACMP for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0OUT_PINADCACMP           (_LESENSE_PERCTRL_DACCH0OUT_PINADCACMP << 6)        /**< Shifted mode PINADCACMP for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1OUT_SHIFT               8                                                   /**< Shift value for LESENSE_DACCH1OUT */
#define _LESENSE_PERCTRL_DACCH1OUT_MASK                0x300UL                                             /**< Bit mask for LESENSE_DACCH1OUT */
#define _LESENSE_PERCTRL_DACCH1OUT_DEFAULT             0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1OUT_DISABLE             0x00000000UL                                        /**< Mode DISABLE for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1OUT_PIN                 0x00000001UL                                        /**< Mode PIN for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1OUT_ADCACMP             0x00000002UL                                        /**< Mode ADCACMP for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1OUT_PINADCACMP          0x00000003UL                                        /**< Mode PINADCACMP for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1OUT_DEFAULT              (_LESENSE_PERCTRL_DACCH1OUT_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1OUT_DISABLE              (_LESENSE_PERCTRL_DACCH1OUT_DISABLE << 8)           /**< Shifted mode DISABLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1OUT_PIN                  (_LESENSE_PERCTRL_DACCH1OUT_PIN << 8)               /**< Shifted mode PIN for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1OUT_ADCACMP              (_LESENSE_PERCTRL_DACCH1OUT_ADCACMP << 8)           /**< Shifted mode ADCACMP for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1OUT_PINADCACMP           (_LESENSE_PERCTRL_DACCH1OUT_PINADCACMP << 8)        /**< Shifted mode PINADCACMP for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACPRESC_SHIFT                10                                                  /**< Shift value for LESENSE_DACPRESC */
#define _LESENSE_PERCTRL_DACPRESC_MASK                 0x7C00UL                                            /**< Bit mask for LESENSE_DACPRESC */
#define _LESENSE_PERCTRL_DACPRESC_DEFAULT              0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACPRESC_DEFAULT               (_LESENSE_PERCTRL_DACPRESC_DEFAULT << 10)           /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACREF                         (0x1UL << 18)                                       /**< DAC bandgap reference used */
#define _LESENSE_PERCTRL_DACREF_SHIFT                  18                                                  /**< Shift value for LESENSE_DACREF */
#define _LESENSE_PERCTRL_DACREF_MASK                   0x40000UL                                           /**< Bit mask for LESENSE_DACREF */
#define _LESENSE_PERCTRL_DACREF_DEFAULT                0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACREF_VDD                    0x00000000UL                                        /**< Mode VDD for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACREF_BANDGAP                0x00000001UL                                        /**< Mode BANDGAP for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACREF_DEFAULT                 (_LESENSE_PERCTRL_DACREF_DEFAULT << 18)             /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACREF_VDD                     (_LESENSE_PERCTRL_DACREF_VDD << 18)                 /**< Shifted mode VDD for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACREF_BANDGAP                 (_LESENSE_PERCTRL_DACREF_BANDGAP << 18)             /**< Shifted mode BANDGAP for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_ACMP0MODE_SHIFT               20                                                  /**< Shift value for LESENSE_ACMP0MODE */
#define _LESENSE_PERCTRL_ACMP0MODE_MASK                0x300000UL                                          /**< Bit mask for LESENSE_ACMP0MODE */
#define _LESENSE_PERCTRL_ACMP0MODE_DEFAULT             0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_ACMP0MODE_DISABLE             0x00000000UL                                        /**< Mode DISABLE for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_ACMP0MODE_MUX                 0x00000001UL                                        /**< Mode MUX for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_ACMP0MODE_MUXTHRES            0x00000002UL                                        /**< Mode MUXTHRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP0MODE_DEFAULT              (_LESENSE_PERCTRL_ACMP0MODE_DEFAULT << 20)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP0MODE_DISABLE              (_LESENSE_PERCTRL_ACMP0MODE_DISABLE << 20)          /**< Shifted mode DISABLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP0MODE_MUX                  (_LESENSE_PERCTRL_ACMP0MODE_MUX << 20)              /**< Shifted mode MUX for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP0MODE_MUXTHRES             (_LESENSE_PERCTRL_ACMP0MODE_MUXTHRES << 20)         /**< Shifted mode MUXTHRES for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_ACMP1MODE_SHIFT               22                                                  /**< Shift value for LESENSE_ACMP1MODE */
#define _LESENSE_PERCTRL_ACMP1MODE_MASK                0xC00000UL                                          /**< Bit mask for LESENSE_ACMP1MODE */
#define _LESENSE_PERCTRL_ACMP1MODE_DEFAULT             0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_ACMP1MODE_DISABLE             0x00000000UL                                        /**< Mode DISABLE for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_ACMP1MODE_MUX                 0x00000001UL                                        /**< Mode MUX for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_ACMP1MODE_MUXTHRES            0x00000002UL                                        /**< Mode MUXTHRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP1MODE_DEFAULT              (_LESENSE_PERCTRL_ACMP1MODE_DEFAULT << 22)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP1MODE_DISABLE              (_LESENSE_PERCTRL_ACMP1MODE_DISABLE << 22)          /**< Shifted mode DISABLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP1MODE_MUX                  (_LESENSE_PERCTRL_ACMP1MODE_MUX << 22)              /**< Shifted mode MUX for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP1MODE_MUXTHRES             (_LESENSE_PERCTRL_ACMP1MODE_MUXTHRES << 22)         /**< Shifted mode MUXTHRES for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_SHIFT              26                                                  /**< Shift value for LESENSE_WARMUPMODE */
#define _LESENSE_PERCTRL_WARMUPMODE_MASK               0xC000000UL                                         /**< Bit mask for LESENSE_WARMUPMODE */
#define _LESENSE_PERCTRL_WARMUPMODE_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_NORMAL             0x00000000UL                                        /**< Mode NORMAL for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_KEEPACMPWARM       0x00000001UL                                        /**< Mode KEEPACMPWARM for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_KEEPDACWARM        0x00000002UL                                        /**< Mode KEEPDACWARM for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_KEEPACMPDACWARM    0x00000003UL                                        /**< Mode KEEPACMPDACWARM for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_DEFAULT             (_LESENSE_PERCTRL_WARMUPMODE_DEFAULT << 26)         /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_NORMAL              (_LESENSE_PERCTRL_WARMUPMODE_NORMAL << 26)          /**< Shifted mode NORMAL for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_KEEPACMPWARM        (_LESENSE_PERCTRL_WARMUPMODE_KEEPACMPWARM << 26)    /**< Shifted mode KEEPACMPWARM for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_KEEPDACWARM         (_LESENSE_PERCTRL_WARMUPMODE_KEEPDACWARM << 26)     /**< Shifted mode KEEPDACWARM for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_KEEPACMPDACWARM     (_LESENSE_PERCTRL_WARMUPMODE_KEEPACMPDACWARM << 26) /**< Shifted mode KEEPACMPDACWARM for LESENSE_PERCTRL */

/* Bit fields for LESENSE DECCTRL */
#define _LESENSE_DECCTRL_RESETVALUE                    0x00000000UL                              /**< Default value for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_MASK                          0x03FFFDFFUL                              /**< Mask for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_DISABLE                        (0x1UL << 0)                              /**< Disable the decoder */
#define _LESENSE_DECCTRL_DISABLE_SHIFT                 0                                         /**< Shift value for LESENSE_DISABLE */
#define _LESENSE_DECCTRL_DISABLE_MASK                  0x1UL                                     /**< Bit mask for LESENSE_DISABLE */
#define _LESENSE_DECCTRL_DISABLE_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_DISABLE_DEFAULT                (_LESENSE_DECCTRL_DISABLE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_ERRCHK                         (0x1UL << 1)                              /**< Enable check of current state */
#define _LESENSE_DECCTRL_ERRCHK_SHIFT                  1                                         /**< Shift value for LESENSE_ERRCHK */
#define _LESENSE_DECCTRL_ERRCHK_MASK                   0x2UL                                     /**< Bit mask for LESENSE_ERRCHK */
#define _LESENSE_DECCTRL_ERRCHK_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_ERRCHK_DEFAULT                 (_LESENSE_DECCTRL_ERRCHK_DEFAULT << 1)    /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INTMAP                         (0x1UL << 2)                              /**< Enable decoder to channel interrupt mapping */
#define _LESENSE_DECCTRL_INTMAP_SHIFT                  2                                         /**< Shift value for LESENSE_INTMAP */
#define _LESENSE_DECCTRL_INTMAP_MASK                   0x4UL                                     /**< Bit mask for LESENSE_INTMAP */
#define _LESENSE_DECCTRL_INTMAP_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INTMAP_DEFAULT                 (_LESENSE_DECCTRL_INTMAP_DEFAULT << 2)    /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS0                       (0x1UL << 3)                              /**< Enable decoder hysteresis on PRS0 output */
#define _LESENSE_DECCTRL_HYSTPRS0_SHIFT                3                                         /**< Shift value for LESENSE_HYSTPRS0 */
#define _LESENSE_DECCTRL_HYSTPRS0_MASK                 0x8UL                                     /**< Bit mask for LESENSE_HYSTPRS0 */
#define _LESENSE_DECCTRL_HYSTPRS0_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS0_DEFAULT               (_LESENSE_DECCTRL_HYSTPRS0_DEFAULT << 3)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS1                       (0x1UL << 4)                              /**< Enable decoder hysteresis on PRS1 output */
#define _LESENSE_DECCTRL_HYSTPRS1_SHIFT                4                                         /**< Shift value for LESENSE_HYSTPRS1 */
#define _LESENSE_DECCTRL_HYSTPRS1_MASK                 0x10UL                                    /**< Bit mask for LESENSE_HYSTPRS1 */
#define _LESENSE_DECCTRL_HYSTPRS1_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS1_DEFAULT               (_LESENSE_DECCTRL_HYSTPRS1_DEFAULT << 4)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS2                       (0x1UL << 5)                              /**< Enable decoder hysteresis on PRS2 output */
#define _LESENSE_DECCTRL_HYSTPRS2_SHIFT                5                                         /**< Shift value for LESENSE_HYSTPRS2 */
#define _LESENSE_DECCTRL_HYSTPRS2_MASK                 0x20UL                                    /**< Bit mask for LESENSE_HYSTPRS2 */
#define _LESENSE_DECCTRL_HYSTPRS2_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS2_DEFAULT               (_LESENSE_DECCTRL_HYSTPRS2_DEFAULT << 5)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTIRQ                        (0x1UL << 6)                              /**< Enable decoder hysteresis on interrupt requests */
#define _LESENSE_DECCTRL_HYSTIRQ_SHIFT                 6                                         /**< Shift value for LESENSE_HYSTIRQ */
#define _LESENSE_DECCTRL_HYSTIRQ_MASK                  0x40UL                                    /**< Bit mask for LESENSE_HYSTIRQ */
#define _LESENSE_DECCTRL_HYSTIRQ_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTIRQ_DEFAULT                (_LESENSE_DECCTRL_HYSTIRQ_DEFAULT << 6)   /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSCNT                         (0x1UL << 7)                              /**< Enable count mode on decoder PRS channels 0 and 1 */
#define _LESENSE_DECCTRL_PRSCNT_SHIFT                  7                                         /**< Shift value for LESENSE_PRSCNT */
#define _LESENSE_DECCTRL_PRSCNT_MASK                   0x80UL                                    /**< Bit mask for LESENSE_PRSCNT */
#define _LESENSE_DECCTRL_PRSCNT_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSCNT_DEFAULT                 (_LESENSE_DECCTRL_PRSCNT_DEFAULT << 7)    /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INPUT                          (0x1UL << 8)                              /**<  */
#define _LESENSE_DECCTRL_INPUT_SHIFT                   8                                         /**< Shift value for LESENSE_INPUT */
#define _LESENSE_DECCTRL_INPUT_MASK                    0x100UL                                   /**< Bit mask for LESENSE_INPUT */
#define _LESENSE_DECCTRL_INPUT_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_INPUT_SENSORSTATE             0x00000000UL                              /**< Mode SENSORSTATE for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_INPUT_PRS                     0x00000001UL                              /**< Mode PRS for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INPUT_DEFAULT                  (_LESENSE_DECCTRL_INPUT_DEFAULT << 8)     /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INPUT_SENSORSTATE              (_LESENSE_DECCTRL_INPUT_SENSORSTATE << 8) /**< Shifted mode SENSORSTATE for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INPUT_PRS                      (_LESENSE_DECCTRL_INPUT_PRS << 8)         /**< Shifted mode PRS for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_SHIFT                 10                                        /**< Shift value for LESENSE_PRSSEL0 */
#define _LESENSE_DECCTRL_PRSSEL0_MASK                  0x3C00UL                                  /**< Bit mask for LESENSE_PRSSEL0 */
#define _LESENSE_DECCTRL_PRSSEL0_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH0                0x00000000UL                              /**< Mode PRSCH0 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH1                0x00000001UL                              /**< Mode PRSCH1 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH2                0x00000002UL                              /**< Mode PRSCH2 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH3                0x00000003UL                              /**< Mode PRSCH3 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH4                0x00000004UL                              /**< Mode PRSCH4 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH5                0x00000005UL                              /**< Mode PRSCH5 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH6                0x00000006UL                              /**< Mode PRSCH6 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH7                0x00000007UL                              /**< Mode PRSCH7 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH8                0x00000008UL                              /**< Mode PRSCH8 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH9                0x00000009UL                              /**< Mode PRSCH9 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH10               0x0000000AUL                              /**< Mode PRSCH10 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL0_PRSCH11               0x0000000BUL                              /**< Mode PRSCH11 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_DEFAULT                (_LESENSE_DECCTRL_PRSSEL0_DEFAULT << 10)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH0                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH0 << 10)   /**< Shifted mode PRSCH0 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH1                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH1 << 10)   /**< Shifted mode PRSCH1 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH2                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH2 << 10)   /**< Shifted mode PRSCH2 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH3                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH3 << 10)   /**< Shifted mode PRSCH3 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH4                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH4 << 10)   /**< Shifted mode PRSCH4 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH5                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH5 << 10)   /**< Shifted mode PRSCH5 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH6                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH6 << 10)   /**< Shifted mode PRSCH6 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH7                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH7 << 10)   /**< Shifted mode PRSCH7 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH8                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH8 << 10)   /**< Shifted mode PRSCH8 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH9                 (_LESENSE_DECCTRL_PRSSEL0_PRSCH9 << 10)   /**< Shifted mode PRSCH9 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH10                (_LESENSE_DECCTRL_PRSSEL0_PRSCH10 << 10)  /**< Shifted mode PRSCH10 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL0_PRSCH11                (_LESENSE_DECCTRL_PRSSEL0_PRSCH11 << 10)  /**< Shifted mode PRSCH11 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_SHIFT                 14                                        /**< Shift value for LESENSE_PRSSEL1 */
#define _LESENSE_DECCTRL_PRSSEL1_MASK                  0x3C000UL                                 /**< Bit mask for LESENSE_PRSSEL1 */
#define _LESENSE_DECCTRL_PRSSEL1_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH0                0x00000000UL                              /**< Mode PRSCH0 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH1                0x00000001UL                              /**< Mode PRSCH1 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH2                0x00000002UL                              /**< Mode PRSCH2 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH3                0x00000003UL                              /**< Mode PRSCH3 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH4                0x00000004UL                              /**< Mode PRSCH4 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH5                0x00000005UL                              /**< Mode PRSCH5 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH6                0x00000006UL                              /**< Mode PRSCH6 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH7                0x00000007UL                              /**< Mode PRSCH7 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH8                0x00000008UL                              /**< Mode PRSCH8 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH9                0x00000009UL                              /**< Mode PRSCH9 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH10               0x0000000AUL                              /**< Mode PRSCH10 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL1_PRSCH11               0x0000000BUL                              /**< Mode PRSCH11 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_DEFAULT                (_LESENSE_DECCTRL_PRSSEL1_DEFAULT << 14)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH0                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH0 << 14)   /**< Shifted mode PRSCH0 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH1                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH1 << 14)   /**< Shifted mode PRSCH1 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH2                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH2 << 14)   /**< Shifted mode PRSCH2 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH3                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH3 << 14)   /**< Shifted mode PRSCH3 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH4                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH4 << 14)   /**< Shifted mode PRSCH4 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH5                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH5 << 14)   /**< Shifted mode PRSCH5 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH6                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH6 << 14)   /**< Shifted mode PRSCH6 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH7                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH7 << 14)   /**< Shifted mode PRSCH7 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH8                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH8 << 14)   /**< Shifted mode PRSCH8 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH9                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH9 << 14)   /**< Shifted mode PRSCH9 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH10                (_LESENSE_DECCTRL_PRSSEL1_PRSCH10 << 14)  /**< Shifted mode PRSCH10 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH11                (_LESENSE_DECCTRL_PRSSEL1_PRSCH11 << 14)  /**< Shifted mode PRSCH11 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_SHIFT                 18                                        /**< Shift value for LESENSE_PRSSEL2 */
#define _LESENSE_DECCTRL_PRSSEL2_MASK                  0x3C0000UL                                /**< Bit mask for LESENSE_PRSSEL2 */
#define _LESENSE_DECCTRL_PRSSEL2_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH0                0x00000000UL                              /**< Mode PRSCH0 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH1                0x00000001UL                              /**< Mode PRSCH1 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH2                0x00000002UL                              /**< Mode PRSCH2 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH3                0x00000003UL                              /**< Mode PRSCH3 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH4                0x00000004UL                              /**< Mode PRSCH4 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH5                0x00000005UL                              /**< Mode PRSCH5 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH6                0x00000006UL                              /**< Mode PRSCH6 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH7                0x00000007UL                              /**< Mode PRSCH7 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH8                0x00000008UL                              /**< Mode PRSCH8 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH9                0x00000009UL                              /**< Mode PRSCH9 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH10               0x0000000AUL                              /**< Mode PRSCH10 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_PRSCH11               0x0000000BUL                              /**< Mode PRSCH11 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_DEFAULT                (_LESENSE_DECCTRL_PRSSEL2_DEFAULT << 18)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH0                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH0 << 18)   /**< Shifted mode PRSCH0 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH1                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH1 << 18)   /**< Shifted mode PRSCH1 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH2                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH2 << 18)   /**< Shifted mode PRSCH2 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH3                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH3 << 18)   /**< Shifted mode PRSCH3 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH4                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH4 << 18)   /**< Shifted mode PRSCH4 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH5                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH5 << 18)   /**< Shifted mode PRSCH5 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH6                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH6 << 18)   /**< Shifted mode PRSCH6 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH7                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH7 << 18)   /**< Shifted mode PRSCH7 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH8                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH8 << 18)   /**< Shifted mode PRSCH8 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH9                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH9 << 18)   /**< Shifted mode PRSCH9 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH10                (_LESENSE_DECCTRL_PRSSEL2_PRSCH10 << 18)  /**< Shifted mode PRSCH10 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH11                (_LESENSE_DECCTRL_PRSSEL2_PRSCH11 << 18)  /**< Shifted mode PRSCH11 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_SHIFT                 22                                        /**< Shift value for LESENSE_PRSSEL3 */
#define _LESENSE_DECCTRL_PRSSEL3_MASK                  0x3C00000UL                               /**< Bit mask for LESENSE_PRSSEL3 */
#define _LESENSE_DECCTRL_PRSSEL3_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH0                0x00000000UL                              /**< Mode PRSCH0 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH1                0x00000001UL                              /**< Mode PRSCH1 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH2                0x00000002UL                              /**< Mode PRSCH2 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH3                0x00000003UL                              /**< Mode PRSCH3 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH4                0x00000004UL                              /**< Mode PRSCH4 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH5                0x00000005UL                              /**< Mode PRSCH5 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH6                0x00000006UL                              /**< Mode PRSCH6 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH7                0x00000007UL                              /**< Mode PRSCH7 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH8                0x00000008UL                              /**< Mode PRSCH8 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH9                0x00000009UL                              /**< Mode PRSCH9 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH10               0x0000000AUL                              /**< Mode PRSCH10 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_PRSCH11               0x0000000BUL                              /**< Mode PRSCH11 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_DEFAULT                (_LESENSE_DECCTRL_PRSSEL3_DEFAULT << 22)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH0                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH0 << 22)   /**< Shifted mode PRSCH0 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH1                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH1 << 22)   /**< Shifted mode PRSCH1 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH2                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH2 << 22)   /**< Shifted mode PRSCH2 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH3                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH3 << 22)   /**< Shifted mode PRSCH3 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH4                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH4 << 22)   /**< Shifted mode PRSCH4 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH5                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH5 << 22)   /**< Shifted mode PRSCH5 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH6                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH6 << 22)   /**< Shifted mode PRSCH6 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH7                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH7 << 22)   /**< Shifted mode PRSCH7 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH8                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH8 << 22)   /**< Shifted mode PRSCH8 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH9                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH9 << 22)   /**< Shifted mode PRSCH9 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH10                (_LESENSE_DECCTRL_PRSSEL3_PRSCH10 << 22)  /**< Shifted mode PRSCH10 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH11                (_LESENSE_DECCTRL_PRSSEL3_PRSCH11 << 22)  /**< Shifted mode PRSCH11 for LESENSE_DECCTRL */

/* Bit fields for LESENSE BIASCTRL */
#define _LESENSE_BIASCTRL_RESETVALUE                   0x00000000UL                                /**< Default value for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_MASK                         0x00000003UL                                /**< Mask for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_BIASMODE_SHIFT               0                                           /**< Shift value for LESENSE_BIASMODE */
#define _LESENSE_BIASCTRL_BIASMODE_MASK                0x3UL                                       /**< Bit mask for LESENSE_BIASMODE */
#define _LESENSE_BIASCTRL_BIASMODE_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_BIASMODE_DUTYCYCLE           0x00000000UL                                /**< Mode DUTYCYCLE for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_BIASMODE_HIGHACC             0x00000001UL                                /**< Mode HIGHACC for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_BIASMODE_DONTTOUCH           0x00000002UL                                /**< Mode DONTTOUCH for LESENSE_BIASCTRL */
#define LESENSE_BIASCTRL_BIASMODE_DEFAULT              (_LESENSE_BIASCTRL_BIASMODE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_BIASCTRL */
#define LESENSE_BIASCTRL_BIASMODE_DUTYCYCLE            (_LESENSE_BIASCTRL_BIASMODE_DUTYCYCLE << 0) /**< Shifted mode DUTYCYCLE for LESENSE_BIASCTRL */
#define LESENSE_BIASCTRL_BIASMODE_HIGHACC              (_LESENSE_BIASCTRL_BIASMODE_HIGHACC << 0)   /**< Shifted mode HIGHACC for LESENSE_BIASCTRL */
#define LESENSE_BIASCTRL_BIASMODE_DONTTOUCH            (_LESENSE_BIASCTRL_BIASMODE_DONTTOUCH << 0) /**< Shifted mode DONTTOUCH for LESENSE_BIASCTRL */

/* Bit fields for LESENSE CMD */
#define _LESENSE_CMD_RESETVALUE                        0x00000000UL                         /**< Default value for LESENSE_CMD */
#define _LESENSE_CMD_MASK                              0x0000000FUL                         /**< Mask for LESENSE_CMD */
#define LESENSE_CMD_START                              (0x1UL << 0)                         /**< Start scanning of sensors. */
#define _LESENSE_CMD_START_SHIFT                       0                                    /**< Shift value for LESENSE_START */
#define _LESENSE_CMD_START_MASK                        0x1UL                                /**< Bit mask for LESENSE_START */
#define _LESENSE_CMD_START_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_START_DEFAULT                      (_LESENSE_CMD_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_STOP                               (0x1UL << 1)                         /**< Stop scanning of sensors */
#define _LESENSE_CMD_STOP_SHIFT                        1                                    /**< Shift value for LESENSE_STOP */
#define _LESENSE_CMD_STOP_MASK                         0x2UL                                /**< Bit mask for LESENSE_STOP */
#define _LESENSE_CMD_STOP_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_STOP_DEFAULT                       (_LESENSE_CMD_STOP_DEFAULT << 1)     /**< Shifted mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_DECODE                             (0x1UL << 2)                         /**< Start decoder */
#define _LESENSE_CMD_DECODE_SHIFT                      2                                    /**< Shift value for LESENSE_DECODE */
#define _LESENSE_CMD_DECODE_MASK                       0x4UL                                /**< Bit mask for LESENSE_DECODE */
#define _LESENSE_CMD_DECODE_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_DECODE_DEFAULT                     (_LESENSE_CMD_DECODE_DEFAULT << 2)   /**< Shifted mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_CLEARBUF                           (0x1UL << 3)                         /**< Clear result buffer */
#define _LESENSE_CMD_CLEARBUF_SHIFT                    3                                    /**< Shift value for LESENSE_CLEARBUF */
#define _LESENSE_CMD_CLEARBUF_MASK                     0x8UL                                /**< Bit mask for LESENSE_CLEARBUF */
#define _LESENSE_CMD_CLEARBUF_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_CLEARBUF_DEFAULT                   (_LESENSE_CMD_CLEARBUF_DEFAULT << 3) /**< Shifted mode DEFAULT for LESENSE_CMD */

/* Bit fields for LESENSE CHEN */
#define _LESENSE_CHEN_RESETVALUE                       0x00000000UL                      /**< Default value for LESENSE_CHEN */
#define _LESENSE_CHEN_MASK                             0x0000FFFFUL                      /**< Mask for LESENSE_CHEN */
#define _LESENSE_CHEN_CHEN_SHIFT                       0                                 /**< Shift value for LESENSE_CHEN */
#define _LESENSE_CHEN_CHEN_MASK                        0xFFFFUL                          /**< Bit mask for LESENSE_CHEN */
#define _LESENSE_CHEN_CHEN_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for LESENSE_CHEN */
#define LESENSE_CHEN_CHEN_DEFAULT                      (_LESENSE_CHEN_CHEN_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_CHEN */

/* Bit fields for LESENSE SCANRES */
#define _LESENSE_SCANRES_RESETVALUE                    0x00000000UL                            /**< Default value for LESENSE_SCANRES */
#define _LESENSE_SCANRES_MASK                          0x0000FFFFUL                            /**< Mask for LESENSE_SCANRES */
#define _LESENSE_SCANRES_SCANRES_SHIFT                 0                                       /**< Shift value for LESENSE_SCANRES */
#define _LESENSE_SCANRES_SCANRES_MASK                  0xFFFFUL                                /**< Bit mask for LESENSE_SCANRES */
#define _LESENSE_SCANRES_SCANRES_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for LESENSE_SCANRES */
#define LESENSE_SCANRES_SCANRES_DEFAULT                (_LESENSE_SCANRES_SCANRES_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_SCANRES */

/* Bit fields for LESENSE STATUS */
#define _LESENSE_STATUS_RESETVALUE                     0x00000000UL                               /**< Default value for LESENSE_STATUS */
#define _LESENSE_STATUS_MASK                           0x0000003FUL                               /**< Mask for LESENSE_STATUS */
#define LESENSE_STATUS_BUFDATAV                        (0x1UL << 0)                               /**< Result data valid */
#define _LESENSE_STATUS_BUFDATAV_SHIFT                 0                                          /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_STATUS_BUFDATAV_MASK                  0x1UL                                      /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_STATUS_BUFDATAV_DEFAULT               0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFDATAV_DEFAULT                (_LESENSE_STATUS_BUFDATAV_DEFAULT << 0)    /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFHALFFULL                     (0x1UL << 1)                               /**< Result buffer half full */
#define _LESENSE_STATUS_BUFHALFFULL_SHIFT              1                                          /**< Shift value for LESENSE_BUFHALFFULL */
#define _LESENSE_STATUS_BUFHALFFULL_MASK               0x2UL                                      /**< Bit mask for LESENSE_BUFHALFFULL */
#define _LESENSE_STATUS_BUFHALFFULL_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFHALFFULL_DEFAULT             (_LESENSE_STATUS_BUFHALFFULL_DEFAULT << 1) /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFFULL                         (0x1UL << 2)                               /**< Result buffer full */
#define _LESENSE_STATUS_BUFFULL_SHIFT                  2                                          /**< Shift value for LESENSE_BUFFULL */
#define _LESENSE_STATUS_BUFFULL_MASK                   0x4UL                                      /**< Bit mask for LESENSE_BUFFULL */
#define _LESENSE_STATUS_BUFFULL_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFFULL_DEFAULT                 (_LESENSE_STATUS_BUFFULL_DEFAULT << 2)     /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_RUNNING                         (0x1UL << 3)                               /**< LESENSE is active */
#define _LESENSE_STATUS_RUNNING_SHIFT                  3                                          /**< Shift value for LESENSE_RUNNING */
#define _LESENSE_STATUS_RUNNING_MASK                   0x8UL                                      /**< Bit mask for LESENSE_RUNNING */
#define _LESENSE_STATUS_RUNNING_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_RUNNING_DEFAULT                 (_LESENSE_STATUS_RUNNING_DEFAULT << 3)     /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_SCANACTIVE                      (0x1UL << 4)                               /**< LESENSE is currently interfacing sensors. */
#define _LESENSE_STATUS_SCANACTIVE_SHIFT               4                                          /**< Shift value for LESENSE_SCANACTIVE */
#define _LESENSE_STATUS_SCANACTIVE_MASK                0x10UL                                     /**< Bit mask for LESENSE_SCANACTIVE */
#define _LESENSE_STATUS_SCANACTIVE_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_SCANACTIVE_DEFAULT              (_LESENSE_STATUS_SCANACTIVE_DEFAULT << 4)  /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_DACACTIVE                       (0x1UL << 5)                               /**< LESENSE DAC interface is active */
#define _LESENSE_STATUS_DACACTIVE_SHIFT                5                                          /**< Shift value for LESENSE_DACACTIVE */
#define _LESENSE_STATUS_DACACTIVE_MASK                 0x20UL                                     /**< Bit mask for LESENSE_DACACTIVE */
#define _LESENSE_STATUS_DACACTIVE_DEFAULT              0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_DACACTIVE_DEFAULT               (_LESENSE_STATUS_DACACTIVE_DEFAULT << 5)   /**< Shifted mode DEFAULT for LESENSE_STATUS */

/* Bit fields for LESENSE PTR */
#define _LESENSE_PTR_RESETVALUE                        0x00000000UL                   /**< Default value for LESENSE_PTR */
#define _LESENSE_PTR_MASK                              0x000001EFUL                   /**< Mask for LESENSE_PTR */
#define _LESENSE_PTR_RD_SHIFT                          0                              /**< Shift value for LESENSE_RD */
#define _LESENSE_PTR_RD_MASK                           0xFUL                          /**< Bit mask for LESENSE_RD */
#define _LESENSE_PTR_RD_DEFAULT                        0x00000000UL                   /**< Mode DEFAULT for LESENSE_PTR */
#define LESENSE_PTR_RD_DEFAULT                         (_LESENSE_PTR_RD_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_PTR */
#define _LESENSE_PTR_WR_SHIFT                          5                              /**< Shift value for LESENSE_WR */
#define _LESENSE_PTR_WR_MASK                           0x1E0UL                        /**< Bit mask for LESENSE_WR */
#define _LESENSE_PTR_WR_DEFAULT                        0x00000000UL                   /**< Mode DEFAULT for LESENSE_PTR */
#define LESENSE_PTR_WR_DEFAULT                         (_LESENSE_PTR_WR_DEFAULT << 5) /**< Shifted mode DEFAULT for LESENSE_PTR */

/* Bit fields for LESENSE BUFDATA */
#define _LESENSE_BUFDATA_RESETVALUE                    0x00000000UL                            /**< Default value for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_MASK                          0x0000FFFFUL                            /**< Mask for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_BUFDATA_SHIFT                 0                                       /**< Shift value for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_BUFDATA_MASK                  0xFFFFUL                                /**< Bit mask for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_BUFDATA_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for LESENSE_BUFDATA */
#define LESENSE_BUFDATA_BUFDATA_DEFAULT                (_LESENSE_BUFDATA_BUFDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_BUFDATA */

/* Bit fields for LESENSE CURCH */
#define _LESENSE_CURCH_RESETVALUE                      0x00000000UL                        /**< Default value for LESENSE_CURCH */
#define _LESENSE_CURCH_MASK                            0x0000000FUL                        /**< Mask for LESENSE_CURCH */
#define _LESENSE_CURCH_CURCH_SHIFT                     0                                   /**< Shift value for LESENSE_CURCH */
#define _LESENSE_CURCH_CURCH_MASK                      0xFUL                               /**< Bit mask for LESENSE_CURCH */
#define _LESENSE_CURCH_CURCH_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for LESENSE_CURCH */
#define LESENSE_CURCH_CURCH_DEFAULT                    (_LESENSE_CURCH_CURCH_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_CURCH */

/* Bit fields for LESENSE DECSTATE */
#define _LESENSE_DECSTATE_RESETVALUE                   0x00000000UL                              /**< Default value for LESENSE_DECSTATE */
#define _LESENSE_DECSTATE_MASK                         0x0000000FUL                              /**< Mask for LESENSE_DECSTATE */
#define _LESENSE_DECSTATE_DECSTATE_SHIFT               0                                         /**< Shift value for LESENSE_DECSTATE */
#define _LESENSE_DECSTATE_DECSTATE_MASK                0xFUL                                     /**< Bit mask for LESENSE_DECSTATE */
#define _LESENSE_DECSTATE_DECSTATE_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECSTATE */
#define LESENSE_DECSTATE_DECSTATE_DEFAULT              (_LESENSE_DECSTATE_DECSTATE_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_DECSTATE */

/* Bit fields for LESENSE SENSORSTATE */
#define _LESENSE_SENSORSTATE_RESETVALUE                0x00000000UL                                    /**< Default value for LESENSE_SENSORSTATE */
#define _LESENSE_SENSORSTATE_MASK                      0x0000000FUL                                    /**< Mask for LESENSE_SENSORSTATE */
#define _LESENSE_SENSORSTATE_SENSORSTATE_SHIFT         0                                               /**< Shift value for LESENSE_SENSORSTATE */
#define _LESENSE_SENSORSTATE_SENSORSTATE_MASK          0xFUL                                           /**< Bit mask for LESENSE_SENSORSTATE */
#define _LESENSE_SENSORSTATE_SENSORSTATE_DEFAULT       0x00000000UL                                    /**< Mode DEFAULT for LESENSE_SENSORSTATE */
#define LESENSE_SENSORSTATE_SENSORSTATE_DEFAULT        (_LESENSE_SENSORSTATE_SENSORSTATE_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_SENSORSTATE */

/* Bit fields for LESENSE IDLECONF */
#define _LESENSE_IDLECONF_RESETVALUE                   0x00000000UL                           /**< Default value for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_MASK                         0xFFFFFFFFUL                           /**< Mask for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH0_SHIFT                    0                                      /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IDLECONF_CH0_MASK                     0x3UL                                  /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IDLECONF_CH0_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH0_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH0_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH0_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH0_DACCH0                   0x00000003UL                           /**< Mode DACCH0 for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_DEFAULT                   (_LESENSE_IDLECONF_CH0_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_DISABLE                   (_LESENSE_IDLECONF_CH0_DISABLE << 0)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_HIGH                      (_LESENSE_IDLECONF_CH0_HIGH << 0)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_LOW                       (_LESENSE_IDLECONF_CH0_LOW << 0)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_DACCH0                    (_LESENSE_IDLECONF_CH0_DACCH0 << 0)    /**< Shifted mode DACCH0 for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_SHIFT                    2                                      /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IDLECONF_CH1_MASK                     0xCUL                                  /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IDLECONF_CH1_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_DACCH0                   0x00000003UL                           /**< Mode DACCH0 for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_DEFAULT                   (_LESENSE_IDLECONF_CH1_DEFAULT << 2)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_DISABLE                   (_LESENSE_IDLECONF_CH1_DISABLE << 2)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_HIGH                      (_LESENSE_IDLECONF_CH1_HIGH << 2)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_LOW                       (_LESENSE_IDLECONF_CH1_LOW << 2)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_DACCH0                    (_LESENSE_IDLECONF_CH1_DACCH0 << 2)    /**< Shifted mode DACCH0 for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_SHIFT                    4                                      /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IDLECONF_CH2_MASK                     0x30UL                                 /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IDLECONF_CH2_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_DACCH0                   0x00000003UL                           /**< Mode DACCH0 for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_DEFAULT                   (_LESENSE_IDLECONF_CH2_DEFAULT << 4)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_DISABLE                   (_LESENSE_IDLECONF_CH2_DISABLE << 4)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_HIGH                      (_LESENSE_IDLECONF_CH2_HIGH << 4)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_LOW                       (_LESENSE_IDLECONF_CH2_LOW << 4)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_DACCH0                    (_LESENSE_IDLECONF_CH2_DACCH0 << 4)    /**< Shifted mode DACCH0 for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_SHIFT                    6                                      /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IDLECONF_CH3_MASK                     0xC0UL                                 /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IDLECONF_CH3_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_DACCH0                   0x00000003UL                           /**< Mode DACCH0 for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_DEFAULT                   (_LESENSE_IDLECONF_CH3_DEFAULT << 6)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_DISABLE                   (_LESENSE_IDLECONF_CH3_DISABLE << 6)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_HIGH                      (_LESENSE_IDLECONF_CH3_HIGH << 6)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_LOW                       (_LESENSE_IDLECONF_CH3_LOW << 6)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_DACCH0                    (_LESENSE_IDLECONF_CH3_DACCH0 << 6)    /**< Shifted mode DACCH0 for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_SHIFT                    8                                      /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IDLECONF_CH4_MASK                     0x300UL                                /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IDLECONF_CH4_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_DEFAULT                   (_LESENSE_IDLECONF_CH4_DEFAULT << 8)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_DISABLE                   (_LESENSE_IDLECONF_CH4_DISABLE << 8)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_HIGH                      (_LESENSE_IDLECONF_CH4_HIGH << 8)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_LOW                       (_LESENSE_IDLECONF_CH4_LOW << 8)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_SHIFT                    10                                     /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IDLECONF_CH5_MASK                     0xC00UL                                /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IDLECONF_CH5_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_DEFAULT                   (_LESENSE_IDLECONF_CH5_DEFAULT << 10)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_DISABLE                   (_LESENSE_IDLECONF_CH5_DISABLE << 10)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_HIGH                      (_LESENSE_IDLECONF_CH5_HIGH << 10)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_LOW                       (_LESENSE_IDLECONF_CH5_LOW << 10)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_SHIFT                    12                                     /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IDLECONF_CH6_MASK                     0x3000UL                               /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IDLECONF_CH6_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_DEFAULT                   (_LESENSE_IDLECONF_CH6_DEFAULT << 12)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_DISABLE                   (_LESENSE_IDLECONF_CH6_DISABLE << 12)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_HIGH                      (_LESENSE_IDLECONF_CH6_HIGH << 12)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_LOW                       (_LESENSE_IDLECONF_CH6_LOW << 12)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_SHIFT                    14                                     /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IDLECONF_CH7_MASK                     0xC000UL                               /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IDLECONF_CH7_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_DEFAULT                   (_LESENSE_IDLECONF_CH7_DEFAULT << 14)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_DISABLE                   (_LESENSE_IDLECONF_CH7_DISABLE << 14)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_HIGH                      (_LESENSE_IDLECONF_CH7_HIGH << 14)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_LOW                       (_LESENSE_IDLECONF_CH7_LOW << 14)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_SHIFT                    16                                     /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IDLECONF_CH8_MASK                     0x30000UL                              /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IDLECONF_CH8_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_DEFAULT                   (_LESENSE_IDLECONF_CH8_DEFAULT << 16)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_DISABLE                   (_LESENSE_IDLECONF_CH8_DISABLE << 16)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_HIGH                      (_LESENSE_IDLECONF_CH8_HIGH << 16)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_LOW                       (_LESENSE_IDLECONF_CH8_LOW << 16)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_SHIFT                    18                                     /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IDLECONF_CH9_MASK                     0xC0000UL                              /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IDLECONF_CH9_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_DEFAULT                   (_LESENSE_IDLECONF_CH9_DEFAULT << 18)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_DISABLE                   (_LESENSE_IDLECONF_CH9_DISABLE << 18)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_HIGH                      (_LESENSE_IDLECONF_CH9_HIGH << 18)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_LOW                       (_LESENSE_IDLECONF_CH9_LOW << 18)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_SHIFT                   20                                     /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IDLECONF_CH10_MASK                    0x300000UL                             /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IDLECONF_CH10_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_DEFAULT                  (_LESENSE_IDLECONF_CH10_DEFAULT << 20) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_DISABLE                  (_LESENSE_IDLECONF_CH10_DISABLE << 20) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_HIGH                     (_LESENSE_IDLECONF_CH10_HIGH << 20)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_LOW                      (_LESENSE_IDLECONF_CH10_LOW << 20)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_SHIFT                   22                                     /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IDLECONF_CH11_MASK                    0xC00000UL                             /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IDLECONF_CH11_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_DEFAULT                  (_LESENSE_IDLECONF_CH11_DEFAULT << 22) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_DISABLE                  (_LESENSE_IDLECONF_CH11_DISABLE << 22) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_HIGH                     (_LESENSE_IDLECONF_CH11_HIGH << 22)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_LOW                      (_LESENSE_IDLECONF_CH11_LOW << 22)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_SHIFT                   24                                     /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IDLECONF_CH12_MASK                    0x3000000UL                            /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IDLECONF_CH12_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_DACCH1                  0x00000003UL                           /**< Mode DACCH1 for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_DEFAULT                  (_LESENSE_IDLECONF_CH12_DEFAULT << 24) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_DISABLE                  (_LESENSE_IDLECONF_CH12_DISABLE << 24) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_HIGH                     (_LESENSE_IDLECONF_CH12_HIGH << 24)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_LOW                      (_LESENSE_IDLECONF_CH12_LOW << 24)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_DACCH1                   (_LESENSE_IDLECONF_CH12_DACCH1 << 24)  /**< Shifted mode DACCH1 for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_SHIFT                   26                                     /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IDLECONF_CH13_MASK                    0xC000000UL                            /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IDLECONF_CH13_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_DACCH1                  0x00000003UL                           /**< Mode DACCH1 for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_DEFAULT                  (_LESENSE_IDLECONF_CH13_DEFAULT << 26) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_DISABLE                  (_LESENSE_IDLECONF_CH13_DISABLE << 26) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_HIGH                     (_LESENSE_IDLECONF_CH13_HIGH << 26)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_LOW                      (_LESENSE_IDLECONF_CH13_LOW << 26)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_DACCH1                   (_LESENSE_IDLECONF_CH13_DACCH1 << 26)  /**< Shifted mode DACCH1 for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_SHIFT                   28                                     /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IDLECONF_CH14_MASK                    0x30000000UL                           /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IDLECONF_CH14_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_DACCH1                  0x00000003UL                           /**< Mode DACCH1 for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_DEFAULT                  (_LESENSE_IDLECONF_CH14_DEFAULT << 28) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_DISABLE                  (_LESENSE_IDLECONF_CH14_DISABLE << 28) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_HIGH                     (_LESENSE_IDLECONF_CH14_HIGH << 28)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_LOW                      (_LESENSE_IDLECONF_CH14_LOW << 28)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_DACCH1                   (_LESENSE_IDLECONF_CH14_DACCH1 << 28)  /**< Shifted mode DACCH1 for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_SHIFT                   30                                     /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IDLECONF_CH15_MASK                    0xC0000000UL                           /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IDLECONF_CH15_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_DACCH1                  0x00000003UL                           /**< Mode DACCH1 for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_DEFAULT                  (_LESENSE_IDLECONF_CH15_DEFAULT << 30) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_DISABLE                  (_LESENSE_IDLECONF_CH15_DISABLE << 30) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_HIGH                     (_LESENSE_IDLECONF_CH15_HIGH << 30)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_LOW                      (_LESENSE_IDLECONF_CH15_LOW << 30)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_DACCH1                   (_LESENSE_IDLECONF_CH15_DACCH1 << 30)  /**< Shifted mode DACCH1 for LESENSE_IDLECONF */

/* Bit fields for LESENSE ALTEXCONF */
#define _LESENSE_ALTEXCONF_RESETVALUE                  0x00000000UL                                 /**< Default value for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_MASK                        0x00FFFFFFUL                                 /**< Mask for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF0_SHIFT             0                                            /**< Shift value for LESENSE_IDLECONF0 */
#define _LESENSE_ALTEXCONF_IDLECONF0_MASK              0x3UL                                        /**< Bit mask for LESENSE_IDLECONF0 */
#define _LESENSE_ALTEXCONF_IDLECONF0_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF0_DISABLE           0x00000000UL                                 /**< Mode DISABLE for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF0_HIGH              0x00000001UL                                 /**< Mode HIGH for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF0_LOW               0x00000002UL                                 /**< Mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF0_DEFAULT            (_LESENSE_ALTEXCONF_IDLECONF0_DEFAULT << 0)  /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF0_DISABLE            (_LESENSE_ALTEXCONF_IDLECONF0_DISABLE << 0)  /**< Shifted mode DISABLE for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF0_HIGH               (_LESENSE_ALTEXCONF_IDLECONF0_HIGH << 0)     /**< Shifted mode HIGH for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF0_LOW                (_LESENSE_ALTEXCONF_IDLECONF0_LOW << 0)      /**< Shifted mode LOW for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF1_SHIFT             2                                            /**< Shift value for LESENSE_IDLECONF1 */
#define _LESENSE_ALTEXCONF_IDLECONF1_MASK              0xCUL                                        /**< Bit mask for LESENSE_IDLECONF1 */
#define _LESENSE_ALTEXCONF_IDLECONF1_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF1_DISABLE           0x00000000UL                                 /**< Mode DISABLE for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF1_HIGH              0x00000001UL                                 /**< Mode HIGH for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF1_LOW               0x00000002UL                                 /**< Mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF1_DEFAULT            (_LESENSE_ALTEXCONF_IDLECONF1_DEFAULT << 2)  /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF1_DISABLE            (_LESENSE_ALTEXCONF_IDLECONF1_DISABLE << 2)  /**< Shifted mode DISABLE for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF1_HIGH               (_LESENSE_ALTEXCONF_IDLECONF1_HIGH << 2)     /**< Shifted mode HIGH for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF1_LOW                (_LESENSE_ALTEXCONF_IDLECONF1_LOW << 2)      /**< Shifted mode LOW for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF2_SHIFT             4                                            /**< Shift value for LESENSE_IDLECONF2 */
#define _LESENSE_ALTEXCONF_IDLECONF2_MASK              0x30UL                                       /**< Bit mask for LESENSE_IDLECONF2 */
#define _LESENSE_ALTEXCONF_IDLECONF2_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF2_DISABLE           0x00000000UL                                 /**< Mode DISABLE for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF2_HIGH              0x00000001UL                                 /**< Mode HIGH for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF2_LOW               0x00000002UL                                 /**< Mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF2_DEFAULT            (_LESENSE_ALTEXCONF_IDLECONF2_DEFAULT << 4)  /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF2_DISABLE            (_LESENSE_ALTEXCONF_IDLECONF2_DISABLE << 4)  /**< Shifted mode DISABLE for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF2_HIGH               (_LESENSE_ALTEXCONF_IDLECONF2_HIGH << 4)     /**< Shifted mode HIGH for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF2_LOW                (_LESENSE_ALTEXCONF_IDLECONF2_LOW << 4)      /**< Shifted mode LOW for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF3_SHIFT             6                                            /**< Shift value for LESENSE_IDLECONF3 */
#define _LESENSE_ALTEXCONF_IDLECONF3_MASK              0xC0UL                                       /**< Bit mask for LESENSE_IDLECONF3 */
#define _LESENSE_ALTEXCONF_IDLECONF3_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF3_DISABLE           0x00000000UL                                 /**< Mode DISABLE for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF3_HIGH              0x00000001UL                                 /**< Mode HIGH for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF3_LOW               0x00000002UL                                 /**< Mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF3_DEFAULT            (_LESENSE_ALTEXCONF_IDLECONF3_DEFAULT << 6)  /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF3_DISABLE            (_LESENSE_ALTEXCONF_IDLECONF3_DISABLE << 6)  /**< Shifted mode DISABLE for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF3_HIGH               (_LESENSE_ALTEXCONF_IDLECONF3_HIGH << 6)     /**< Shifted mode HIGH for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF3_LOW                (_LESENSE_ALTEXCONF_IDLECONF3_LOW << 6)      /**< Shifted mode LOW for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF4_SHIFT             8                                            /**< Shift value for LESENSE_IDLECONF4 */
#define _LESENSE_ALTEXCONF_IDLECONF4_MASK              0x300UL                                      /**< Bit mask for LESENSE_IDLECONF4 */
#define _LESENSE_ALTEXCONF_IDLECONF4_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF4_DISABLE           0x00000000UL                                 /**< Mode DISABLE for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF4_HIGH              0x00000001UL                                 /**< Mode HIGH for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF4_LOW               0x00000002UL                                 /**< Mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF4_DEFAULT            (_LESENSE_ALTEXCONF_IDLECONF4_DEFAULT << 8)  /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF4_DISABLE            (_LESENSE_ALTEXCONF_IDLECONF4_DISABLE << 8)  /**< Shifted mode DISABLE for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF4_HIGH               (_LESENSE_ALTEXCONF_IDLECONF4_HIGH << 8)     /**< Shifted mode HIGH for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF4_LOW                (_LESENSE_ALTEXCONF_IDLECONF4_LOW << 8)      /**< Shifted mode LOW for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF5_SHIFT             10                                           /**< Shift value for LESENSE_IDLECONF5 */
#define _LESENSE_ALTEXCONF_IDLECONF5_MASK              0xC00UL                                      /**< Bit mask for LESENSE_IDLECONF5 */
#define _LESENSE_ALTEXCONF_IDLECONF5_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF5_DISABLE           0x00000000UL                                 /**< Mode DISABLE for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF5_HIGH              0x00000001UL                                 /**< Mode HIGH for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF5_LOW               0x00000002UL                                 /**< Mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF5_DEFAULT            (_LESENSE_ALTEXCONF_IDLECONF5_DEFAULT << 10) /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF5_DISABLE            (_LESENSE_ALTEXCONF_IDLECONF5_DISABLE << 10) /**< Shifted mode DISABLE for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF5_HIGH               (_LESENSE_ALTEXCONF_IDLECONF5_HIGH << 10)    /**< Shifted mode HIGH for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF5_LOW                (_LESENSE_ALTEXCONF_IDLECONF5_LOW << 10)     /**< Shifted mode LOW for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF6_SHIFT             12                                           /**< Shift value for LESENSE_IDLECONF6 */
#define _LESENSE_ALTEXCONF_IDLECONF6_MASK              0x3000UL                                     /**< Bit mask for LESENSE_IDLECONF6 */
#define _LESENSE_ALTEXCONF_IDLECONF6_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF6_DISABLE           0x00000000UL                                 /**< Mode DISABLE for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF6_HIGH              0x00000001UL                                 /**< Mode HIGH for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF6_LOW               0x00000002UL                                 /**< Mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF6_DEFAULT            (_LESENSE_ALTEXCONF_IDLECONF6_DEFAULT << 12) /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF6_DISABLE            (_LESENSE_ALTEXCONF_IDLECONF6_DISABLE << 12) /**< Shifted mode DISABLE for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF6_HIGH               (_LESENSE_ALTEXCONF_IDLECONF6_HIGH << 12)    /**< Shifted mode HIGH for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF6_LOW                (_LESENSE_ALTEXCONF_IDLECONF6_LOW << 12)     /**< Shifted mode LOW for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF7_SHIFT             14                                           /**< Shift value for LESENSE_IDLECONF7 */
#define _LESENSE_ALTEXCONF_IDLECONF7_MASK              0xC000UL                                     /**< Bit mask for LESENSE_IDLECONF7 */
#define _LESENSE_ALTEXCONF_IDLECONF7_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF7_DISABLE           0x00000000UL                                 /**< Mode DISABLE for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF7_HIGH              0x00000001UL                                 /**< Mode HIGH for LESENSE_ALTEXCONF */
#define _LESENSE_ALTEXCONF_IDLECONF7_LOW               0x00000002UL                                 /**< Mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF7_DEFAULT            (_LESENSE_ALTEXCONF_IDLECONF7_DEFAULT << 14) /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF7_DISABLE            (_LESENSE_ALTEXCONF_IDLECONF7_DISABLE << 14) /**< Shifted mode DISABLE for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF7_HIGH               (_LESENSE_ALTEXCONF_IDLECONF7_HIGH << 14)    /**< Shifted mode HIGH for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_IDLECONF7_LOW                (_LESENSE_ALTEXCONF_IDLECONF7_LOW << 14)     /**< Shifted mode LOW for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX0                         (0x1UL << 16)                                /**< ALTEX0 always excite enable */
#define _LESENSE_ALTEXCONF_AEX0_SHIFT                  16                                           /**< Shift value for LESENSE_AEX0 */
#define _LESENSE_ALTEXCONF_AEX0_MASK                   0x10000UL                                    /**< Bit mask for LESENSE_AEX0 */
#define _LESENSE_ALTEXCONF_AEX0_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX0_DEFAULT                 (_LESENSE_ALTEXCONF_AEX0_DEFAULT << 16)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX1                         (0x1UL << 17)                                /**< ALTEX1 always excite enable */
#define _LESENSE_ALTEXCONF_AEX1_SHIFT                  17                                           /**< Shift value for LESENSE_AEX1 */
#define _LESENSE_ALTEXCONF_AEX1_MASK                   0x20000UL                                    /**< Bit mask for LESENSE_AEX1 */
#define _LESENSE_ALTEXCONF_AEX1_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX1_DEFAULT                 (_LESENSE_ALTEXCONF_AEX1_DEFAULT << 17)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX2                         (0x1UL << 18)                                /**< ALTEX2 always excite enable */
#define _LESENSE_ALTEXCONF_AEX2_SHIFT                  18                                           /**< Shift value for LESENSE_AEX2 */
#define _LESENSE_ALTEXCONF_AEX2_MASK                   0x40000UL                                    /**< Bit mask for LESENSE_AEX2 */
#define _LESENSE_ALTEXCONF_AEX2_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX2_DEFAULT                 (_LESENSE_ALTEXCONF_AEX2_DEFAULT << 18)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX3                         (0x1UL << 19)                                /**< ALTEX3 always excite enable */
#define _LESENSE_ALTEXCONF_AEX3_SHIFT                  19                                           /**< Shift value for LESENSE_AEX3 */
#define _LESENSE_ALTEXCONF_AEX3_MASK                   0x80000UL                                    /**< Bit mask for LESENSE_AEX3 */
#define _LESENSE_ALTEXCONF_AEX3_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX3_DEFAULT                 (_LESENSE_ALTEXCONF_AEX3_DEFAULT << 19)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX4                         (0x1UL << 20)                                /**< ALTEX4 always excite enable */
#define _LESENSE_ALTEXCONF_AEX4_SHIFT                  20                                           /**< Shift value for LESENSE_AEX4 */
#define _LESENSE_ALTEXCONF_AEX4_MASK                   0x100000UL                                   /**< Bit mask for LESENSE_AEX4 */
#define _LESENSE_ALTEXCONF_AEX4_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX4_DEFAULT                 (_LESENSE_ALTEXCONF_AEX4_DEFAULT << 20)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX5                         (0x1UL << 21)                                /**< ALTEX5 always excite enable */
#define _LESENSE_ALTEXCONF_AEX5_SHIFT                  21                                           /**< Shift value for LESENSE_AEX5 */
#define _LESENSE_ALTEXCONF_AEX5_MASK                   0x200000UL                                   /**< Bit mask for LESENSE_AEX5 */
#define _LESENSE_ALTEXCONF_AEX5_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX5_DEFAULT                 (_LESENSE_ALTEXCONF_AEX5_DEFAULT << 21)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX6                         (0x1UL << 22)                                /**< ALTEX6 always excite enable */
#define _LESENSE_ALTEXCONF_AEX6_SHIFT                  22                                           /**< Shift value for LESENSE_AEX6 */
#define _LESENSE_ALTEXCONF_AEX6_MASK                   0x400000UL                                   /**< Bit mask for LESENSE_AEX6 */
#define _LESENSE_ALTEXCONF_AEX6_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX6_DEFAULT                 (_LESENSE_ALTEXCONF_AEX6_DEFAULT << 22)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX7                         (0x1UL << 23)                                /**< ALTEX7 always excite enable */
#define _LESENSE_ALTEXCONF_AEX7_SHIFT                  23                                           /**< Shift value for LESENSE_AEX7 */
#define _LESENSE_ALTEXCONF_AEX7_MASK                   0x800000UL                                   /**< Bit mask for LESENSE_AEX7 */
#define _LESENSE_ALTEXCONF_AEX7_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX7_DEFAULT                 (_LESENSE_ALTEXCONF_AEX7_DEFAULT << 23)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */

/* Bit fields for LESENSE IF */
#define _LESENSE_IF_RESETVALUE                         0x00000000UL                             /**< Default value for LESENSE_IF */
#define _LESENSE_IF_MASK                               0x007FFFFFUL                             /**< Mask for LESENSE_IF */
#define LESENSE_IF_CH0                                 (0x1UL << 0)                             /**<  */
#define _LESENSE_IF_CH0_SHIFT                          0                                        /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IF_CH0_MASK                           0x1UL                                    /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IF_CH0_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH0_DEFAULT                         (_LESENSE_IF_CH0_DEFAULT << 0)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH1                                 (0x1UL << 1)                             /**<  */
#define _LESENSE_IF_CH1_SHIFT                          1                                        /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IF_CH1_MASK                           0x2UL                                    /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IF_CH1_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH1_DEFAULT                         (_LESENSE_IF_CH1_DEFAULT << 1)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH2                                 (0x1UL << 2)                             /**<  */
#define _LESENSE_IF_CH2_SHIFT                          2                                        /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IF_CH2_MASK                           0x4UL                                    /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IF_CH2_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH2_DEFAULT                         (_LESENSE_IF_CH2_DEFAULT << 2)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH3                                 (0x1UL << 3)                             /**<  */
#define _LESENSE_IF_CH3_SHIFT                          3                                        /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IF_CH3_MASK                           0x8UL                                    /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IF_CH3_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH3_DEFAULT                         (_LESENSE_IF_CH3_DEFAULT << 3)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH4                                 (0x1UL << 4)                             /**<  */
#define _LESENSE_IF_CH4_SHIFT                          4                                        /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IF_CH4_MASK                           0x10UL                                   /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IF_CH4_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH4_DEFAULT                         (_LESENSE_IF_CH4_DEFAULT << 4)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH5                                 (0x1UL << 5)                             /**<  */
#define _LESENSE_IF_CH5_SHIFT                          5                                        /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IF_CH5_MASK                           0x20UL                                   /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IF_CH5_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH5_DEFAULT                         (_LESENSE_IF_CH5_DEFAULT << 5)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH6                                 (0x1UL << 6)                             /**<  */
#define _LESENSE_IF_CH6_SHIFT                          6                                        /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IF_CH6_MASK                           0x40UL                                   /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IF_CH6_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH6_DEFAULT                         (_LESENSE_IF_CH6_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH7                                 (0x1UL << 7)                             /**<  */
#define _LESENSE_IF_CH7_SHIFT                          7                                        /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IF_CH7_MASK                           0x80UL                                   /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IF_CH7_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH7_DEFAULT                         (_LESENSE_IF_CH7_DEFAULT << 7)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH8                                 (0x1UL << 8)                             /**<  */
#define _LESENSE_IF_CH8_SHIFT                          8                                        /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IF_CH8_MASK                           0x100UL                                  /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IF_CH8_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH8_DEFAULT                         (_LESENSE_IF_CH8_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH9                                 (0x1UL << 9)                             /**<  */
#define _LESENSE_IF_CH9_SHIFT                          9                                        /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IF_CH9_MASK                           0x200UL                                  /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IF_CH9_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH9_DEFAULT                         (_LESENSE_IF_CH9_DEFAULT << 9)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH10                                (0x1UL << 10)                            /**<  */
#define _LESENSE_IF_CH10_SHIFT                         10                                       /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IF_CH10_MASK                          0x400UL                                  /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IF_CH10_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH10_DEFAULT                        (_LESENSE_IF_CH10_DEFAULT << 10)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH11                                (0x1UL << 11)                            /**<  */
#define _LESENSE_IF_CH11_SHIFT                         11                                       /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IF_CH11_MASK                          0x800UL                                  /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IF_CH11_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH11_DEFAULT                        (_LESENSE_IF_CH11_DEFAULT << 11)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH12                                (0x1UL << 12)                            /**<  */
#define _LESENSE_IF_CH12_SHIFT                         12                                       /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IF_CH12_MASK                          0x1000UL                                 /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IF_CH12_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH12_DEFAULT                        (_LESENSE_IF_CH12_DEFAULT << 12)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH13                                (0x1UL << 13)                            /**<  */
#define _LESENSE_IF_CH13_SHIFT                         13                                       /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IF_CH13_MASK                          0x2000UL                                 /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IF_CH13_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH13_DEFAULT                        (_LESENSE_IF_CH13_DEFAULT << 13)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH14                                (0x1UL << 14)                            /**<  */
#define _LESENSE_IF_CH14_SHIFT                         14                                       /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IF_CH14_MASK                          0x4000UL                                 /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IF_CH14_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH14_DEFAULT                        (_LESENSE_IF_CH14_DEFAULT << 14)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH15                                (0x1UL << 15)                            /**<  */
#define _LESENSE_IF_CH15_SHIFT                         15                                       /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IF_CH15_MASK                          0x8000UL                                 /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IF_CH15_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH15_DEFAULT                        (_LESENSE_IF_CH15_DEFAULT << 15)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_SCANCOMPLETE                        (0x1UL << 16)                            /**<  */
#define _LESENSE_IF_SCANCOMPLETE_SHIFT                 16                                       /**< Shift value for LESENSE_SCANCOMPLETE */
#define _LESENSE_IF_SCANCOMPLETE_MASK                  0x10000UL                                /**< Bit mask for LESENSE_SCANCOMPLETE */
#define _LESENSE_IF_SCANCOMPLETE_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_SCANCOMPLETE_DEFAULT                (_LESENSE_IF_SCANCOMPLETE_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_DEC                                 (0x1UL << 17)                            /**<  */
#define _LESENSE_IF_DEC_SHIFT                          17                                       /**< Shift value for LESENSE_DEC */
#define _LESENSE_IF_DEC_MASK                           0x20000UL                                /**< Bit mask for LESENSE_DEC */
#define _LESENSE_IF_DEC_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_DEC_DEFAULT                         (_LESENSE_IF_DEC_DEFAULT << 17)          /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_DECERR                              (0x1UL << 18)                            /**<  */
#define _LESENSE_IF_DECERR_SHIFT                       18                                       /**< Shift value for LESENSE_DECERR */
#define _LESENSE_IF_DECERR_MASK                        0x40000UL                                /**< Bit mask for LESENSE_DECERR */
#define _LESENSE_IF_DECERR_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_DECERR_DEFAULT                      (_LESENSE_IF_DECERR_DEFAULT << 18)       /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFDATAV                            (0x1UL << 19)                            /**<  */
#define _LESENSE_IF_BUFDATAV_SHIFT                     19                                       /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_IF_BUFDATAV_MASK                      0x80000UL                                /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_IF_BUFDATAV_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFDATAV_DEFAULT                    (_LESENSE_IF_BUFDATAV_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFLEVEL                            (0x1UL << 20)                            /**<  */
#define _LESENSE_IF_BUFLEVEL_SHIFT                     20                                       /**< Shift value for LESENSE_BUFLEVEL */
#define _LESENSE_IF_BUFLEVEL_MASK                      0x100000UL                               /**< Bit mask for LESENSE_BUFLEVEL */
#define _LESENSE_IF_BUFLEVEL_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFLEVEL_DEFAULT                    (_LESENSE_IF_BUFLEVEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFOF                               (0x1UL << 21)                            /**<  */
#define _LESENSE_IF_BUFOF_SHIFT                        21                                       /**< Shift value for LESENSE_BUFOF */
#define _LESENSE_IF_BUFOF_MASK                         0x200000UL                               /**< Bit mask for LESENSE_BUFOF */
#define _LESENSE_IF_BUFOF_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFOF_DEFAULT                       (_LESENSE_IF_BUFOF_DEFAULT << 21)        /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CNTOF                               (0x1UL << 22)                            /**<  */
#define _LESENSE_IF_CNTOF_SHIFT                        22                                       /**< Shift value for LESENSE_CNTOF */
#define _LESENSE_IF_CNTOF_MASK                         0x400000UL                               /**< Bit mask for LESENSE_CNTOF */
#define _LESENSE_IF_CNTOF_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CNTOF_DEFAULT                       (_LESENSE_IF_CNTOF_DEFAULT << 22)        /**< Shifted mode DEFAULT for LESENSE_IF */

/* Bit fields for LESENSE IFC */
#define _LESENSE_IFC_RESETVALUE                        0x00000000UL                              /**< Default value for LESENSE_IFC */
#define _LESENSE_IFC_MASK                              0x007FFFFFUL                              /**< Mask for LESENSE_IFC */
#define LESENSE_IFC_CH0                                (0x1UL << 0)                              /**<  */
#define _LESENSE_IFC_CH0_SHIFT                         0                                         /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IFC_CH0_MASK                          0x1UL                                     /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IFC_CH0_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH0_DEFAULT                        (_LESENSE_IFC_CH0_DEFAULT << 0)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH1                                (0x1UL << 1)                              /**<  */
#define _LESENSE_IFC_CH1_SHIFT                         1                                         /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IFC_CH1_MASK                          0x2UL                                     /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IFC_CH1_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH1_DEFAULT                        (_LESENSE_IFC_CH1_DEFAULT << 1)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH2                                (0x1UL << 2)                              /**<  */
#define _LESENSE_IFC_CH2_SHIFT                         2                                         /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IFC_CH2_MASK                          0x4UL                                     /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IFC_CH2_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH2_DEFAULT                        (_LESENSE_IFC_CH2_DEFAULT << 2)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH3                                (0x1UL << 3)                              /**<  */
#define _LESENSE_IFC_CH3_SHIFT                         3                                         /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IFC_CH3_MASK                          0x8UL                                     /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IFC_CH3_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH3_DEFAULT                        (_LESENSE_IFC_CH3_DEFAULT << 3)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH4                                (0x1UL << 4)                              /**<  */
#define _LESENSE_IFC_CH4_SHIFT                         4                                         /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IFC_CH4_MASK                          0x10UL                                    /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IFC_CH4_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH4_DEFAULT                        (_LESENSE_IFC_CH4_DEFAULT << 4)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH5                                (0x1UL << 5)                              /**<  */
#define _LESENSE_IFC_CH5_SHIFT                         5                                         /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IFC_CH5_MASK                          0x20UL                                    /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IFC_CH5_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH5_DEFAULT                        (_LESENSE_IFC_CH5_DEFAULT << 5)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH6                                (0x1UL << 6)                              /**<  */
#define _LESENSE_IFC_CH6_SHIFT                         6                                         /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IFC_CH6_MASK                          0x40UL                                    /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IFC_CH6_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH6_DEFAULT                        (_LESENSE_IFC_CH6_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH7                                (0x1UL << 7)                              /**<  */
#define _LESENSE_IFC_CH7_SHIFT                         7                                         /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IFC_CH7_MASK                          0x80UL                                    /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IFC_CH7_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH7_DEFAULT                        (_LESENSE_IFC_CH7_DEFAULT << 7)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH8                                (0x1UL << 8)                              /**<  */
#define _LESENSE_IFC_CH8_SHIFT                         8                                         /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IFC_CH8_MASK                          0x100UL                                   /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IFC_CH8_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH8_DEFAULT                        (_LESENSE_IFC_CH8_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH9                                (0x1UL << 9)                              /**<  */
#define _LESENSE_IFC_CH9_SHIFT                         9                                         /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IFC_CH9_MASK                          0x200UL                                   /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IFC_CH9_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH9_DEFAULT                        (_LESENSE_IFC_CH9_DEFAULT << 9)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH10                               (0x1UL << 10)                             /**<  */
#define _LESENSE_IFC_CH10_SHIFT                        10                                        /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IFC_CH10_MASK                         0x400UL                                   /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IFC_CH10_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH10_DEFAULT                       (_LESENSE_IFC_CH10_DEFAULT << 10)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH11                               (0x1UL << 11)                             /**<  */
#define _LESENSE_IFC_CH11_SHIFT                        11                                        /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IFC_CH11_MASK                         0x800UL                                   /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IFC_CH11_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH11_DEFAULT                       (_LESENSE_IFC_CH11_DEFAULT << 11)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH12                               (0x1UL << 12)                             /**<  */
#define _LESENSE_IFC_CH12_SHIFT                        12                                        /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IFC_CH12_MASK                         0x1000UL                                  /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IFC_CH12_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH12_DEFAULT                       (_LESENSE_IFC_CH12_DEFAULT << 12)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH13                               (0x1UL << 13)                             /**<  */
#define _LESENSE_IFC_CH13_SHIFT                        13                                        /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IFC_CH13_MASK                         0x2000UL                                  /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IFC_CH13_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH13_DEFAULT                       (_LESENSE_IFC_CH13_DEFAULT << 13)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH14                               (0x1UL << 14)                             /**<  */
#define _LESENSE_IFC_CH14_SHIFT                        14                                        /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IFC_CH14_MASK                         0x4000UL                                  /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IFC_CH14_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH14_DEFAULT                       (_LESENSE_IFC_CH14_DEFAULT << 14)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH15                               (0x1UL << 15)                             /**<  */
#define _LESENSE_IFC_CH15_SHIFT                        15                                        /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IFC_CH15_MASK                         0x8000UL                                  /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IFC_CH15_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH15_DEFAULT                       (_LESENSE_IFC_CH15_DEFAULT << 15)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_SCANCOMPLETE                       (0x1UL << 16)                             /**<  */
#define _LESENSE_IFC_SCANCOMPLETE_SHIFT                16                                        /**< Shift value for LESENSE_SCANCOMPLETE */
#define _LESENSE_IFC_SCANCOMPLETE_MASK                 0x10000UL                                 /**< Bit mask for LESENSE_SCANCOMPLETE */
#define _LESENSE_IFC_SCANCOMPLETE_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_SCANCOMPLETE_DEFAULT               (_LESENSE_IFC_SCANCOMPLETE_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_DEC                                (0x1UL << 17)                             /**<  */
#define _LESENSE_IFC_DEC_SHIFT                         17                                        /**< Shift value for LESENSE_DEC */
#define _LESENSE_IFC_DEC_MASK                          0x20000UL                                 /**< Bit mask for LESENSE_DEC */
#define _LESENSE_IFC_DEC_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_DEC_DEFAULT                        (_LESENSE_IFC_DEC_DEFAULT << 17)          /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_DECERR                             (0x1UL << 18)                             /**<  */
#define _LESENSE_IFC_DECERR_SHIFT                      18                                        /**< Shift value for LESENSE_DECERR */
#define _LESENSE_IFC_DECERR_MASK                       0x40000UL                                 /**< Bit mask for LESENSE_DECERR */
#define _LESENSE_IFC_DECERR_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_DECERR_DEFAULT                     (_LESENSE_IFC_DECERR_DEFAULT << 18)       /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFDATAV                           (0x1UL << 19)                             /**<  */
#define _LESENSE_IFC_BUFDATAV_SHIFT                    19                                        /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_IFC_BUFDATAV_MASK                     0x80000UL                                 /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_IFC_BUFDATAV_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFDATAV_DEFAULT                   (_LESENSE_IFC_BUFDATAV_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFLEVEL                           (0x1UL << 20)                             /**<  */
#define _LESENSE_IFC_BUFLEVEL_SHIFT                    20                                        /**< Shift value for LESENSE_BUFLEVEL */
#define _LESENSE_IFC_BUFLEVEL_MASK                     0x100000UL                                /**< Bit mask for LESENSE_BUFLEVEL */
#define _LESENSE_IFC_BUFLEVEL_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFLEVEL_DEFAULT                   (_LESENSE_IFC_BUFLEVEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFOF                              (0x1UL << 21)                             /**<  */
#define _LESENSE_IFC_BUFOF_SHIFT                       21                                        /**< Shift value for LESENSE_BUFOF */
#define _LESENSE_IFC_BUFOF_MASK                        0x200000UL                                /**< Bit mask for LESENSE_BUFOF */
#define _LESENSE_IFC_BUFOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFOF_DEFAULT                      (_LESENSE_IFC_BUFOF_DEFAULT << 21)        /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CNTOF                              (0x1UL << 22)                             /**<  */
#define _LESENSE_IFC_CNTOF_SHIFT                       22                                        /**< Shift value for LESENSE_CNTOF */
#define _LESENSE_IFC_CNTOF_MASK                        0x400000UL                                /**< Bit mask for LESENSE_CNTOF */
#define _LESENSE_IFC_CNTOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CNTOF_DEFAULT                      (_LESENSE_IFC_CNTOF_DEFAULT << 22)        /**< Shifted mode DEFAULT for LESENSE_IFC */

/* Bit fields for LESENSE IFS */
#define _LESENSE_IFS_RESETVALUE                        0x00000000UL                              /**< Default value for LESENSE_IFS */
#define _LESENSE_IFS_MASK                              0x007FFFFFUL                              /**< Mask for LESENSE_IFS */
#define LESENSE_IFS_CH0                                (0x1UL << 0)                              /**<  */
#define _LESENSE_IFS_CH0_SHIFT                         0                                         /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IFS_CH0_MASK                          0x1UL                                     /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IFS_CH0_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH0_DEFAULT                        (_LESENSE_IFS_CH0_DEFAULT << 0)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH1                                (0x1UL << 1)                              /**<  */
#define _LESENSE_IFS_CH1_SHIFT                         1                                         /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IFS_CH1_MASK                          0x2UL                                     /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IFS_CH1_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH1_DEFAULT                        (_LESENSE_IFS_CH1_DEFAULT << 1)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH2                                (0x1UL << 2)                              /**<  */
#define _LESENSE_IFS_CH2_SHIFT                         2                                         /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IFS_CH2_MASK                          0x4UL                                     /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IFS_CH2_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH2_DEFAULT                        (_LESENSE_IFS_CH2_DEFAULT << 2)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH3                                (0x1UL << 3)                              /**<  */
#define _LESENSE_IFS_CH3_SHIFT                         3                                         /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IFS_CH3_MASK                          0x8UL                                     /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IFS_CH3_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH3_DEFAULT                        (_LESENSE_IFS_CH3_DEFAULT << 3)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH4                                (0x1UL << 4)                              /**<  */
#define _LESENSE_IFS_CH4_SHIFT                         4                                         /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IFS_CH4_MASK                          0x10UL                                    /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IFS_CH4_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH4_DEFAULT                        (_LESENSE_IFS_CH4_DEFAULT << 4)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH5                                (0x1UL << 5)                              /**<  */
#define _LESENSE_IFS_CH5_SHIFT                         5                                         /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IFS_CH5_MASK                          0x20UL                                    /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IFS_CH5_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH5_DEFAULT                        (_LESENSE_IFS_CH5_DEFAULT << 5)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH6                                (0x1UL << 6)                              /**<  */
#define _LESENSE_IFS_CH6_SHIFT                         6                                         /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IFS_CH6_MASK                          0x40UL                                    /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IFS_CH6_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH6_DEFAULT                        (_LESENSE_IFS_CH6_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH7                                (0x1UL << 7)                              /**<  */
#define _LESENSE_IFS_CH7_SHIFT                         7                                         /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IFS_CH7_MASK                          0x80UL                                    /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IFS_CH7_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH7_DEFAULT                        (_LESENSE_IFS_CH7_DEFAULT << 7)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH8                                (0x1UL << 8)                              /**<  */
#define _LESENSE_IFS_CH8_SHIFT                         8                                         /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IFS_CH8_MASK                          0x100UL                                   /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IFS_CH8_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH8_DEFAULT                        (_LESENSE_IFS_CH8_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH9                                (0x1UL << 9)                              /**<  */
#define _LESENSE_IFS_CH9_SHIFT                         9                                         /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IFS_CH9_MASK                          0x200UL                                   /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IFS_CH9_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH9_DEFAULT                        (_LESENSE_IFS_CH9_DEFAULT << 9)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH10                               (0x1UL << 10)                             /**<  */
#define _LESENSE_IFS_CH10_SHIFT                        10                                        /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IFS_CH10_MASK                         0x400UL                                   /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IFS_CH10_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH10_DEFAULT                       (_LESENSE_IFS_CH10_DEFAULT << 10)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH11                               (0x1UL << 11)                             /**<  */
#define _LESENSE_IFS_CH11_SHIFT                        11                                        /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IFS_CH11_MASK                         0x800UL                                   /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IFS_CH11_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH11_DEFAULT                       (_LESENSE_IFS_CH11_DEFAULT << 11)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH12                               (0x1UL << 12)                             /**<  */
#define _LESENSE_IFS_CH12_SHIFT                        12                                        /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IFS_CH12_MASK                         0x1000UL                                  /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IFS_CH12_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH12_DEFAULT                       (_LESENSE_IFS_CH12_DEFAULT << 12)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH13                               (0x1UL << 13)                             /**<  */
#define _LESENSE_IFS_CH13_SHIFT                        13                                        /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IFS_CH13_MASK                         0x2000UL                                  /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IFS_CH13_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH13_DEFAULT                       (_LESENSE_IFS_CH13_DEFAULT << 13)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH14                               (0x1UL << 14)                             /**<  */
#define _LESENSE_IFS_CH14_SHIFT                        14                                        /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IFS_CH14_MASK                         0x4000UL                                  /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IFS_CH14_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH14_DEFAULT                       (_LESENSE_IFS_CH14_DEFAULT << 14)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH15                               (0x1UL << 15)                             /**<  */
#define _LESENSE_IFS_CH15_SHIFT                        15                                        /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IFS_CH15_MASK                         0x8000UL                                  /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IFS_CH15_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH15_DEFAULT                       (_LESENSE_IFS_CH15_DEFAULT << 15)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_SCANCOMPLETE                       (0x1UL << 16)                             /**<  */
#define _LESENSE_IFS_SCANCOMPLETE_SHIFT                16                                        /**< Shift value for LESENSE_SCANCOMPLETE */
#define _LESENSE_IFS_SCANCOMPLETE_MASK                 0x10000UL                                 /**< Bit mask for LESENSE_SCANCOMPLETE */
#define _LESENSE_IFS_SCANCOMPLETE_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_SCANCOMPLETE_DEFAULT               (_LESENSE_IFS_SCANCOMPLETE_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_DEC                                (0x1UL << 17)                             /**<  */
#define _LESENSE_IFS_DEC_SHIFT                         17                                        /**< Shift value for LESENSE_DEC */
#define _LESENSE_IFS_DEC_MASK                          0x20000UL                                 /**< Bit mask for LESENSE_DEC */
#define _LESENSE_IFS_DEC_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_DEC_DEFAULT                        (_LESENSE_IFS_DEC_DEFAULT << 17)          /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_DECERR                             (0x1UL << 18)                             /**<  */
#define _LESENSE_IFS_DECERR_SHIFT                      18                                        /**< Shift value for LESENSE_DECERR */
#define _LESENSE_IFS_DECERR_MASK                       0x40000UL                                 /**< Bit mask for LESENSE_DECERR */
#define _LESENSE_IFS_DECERR_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_DECERR_DEFAULT                     (_LESENSE_IFS_DECERR_DEFAULT << 18)       /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFDATAV                           (0x1UL << 19)                             /**<  */
#define _LESENSE_IFS_BUFDATAV_SHIFT                    19                                        /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_IFS_BUFDATAV_MASK                     0x80000UL                                 /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_IFS_BUFDATAV_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFDATAV_DEFAULT                   (_LESENSE_IFS_BUFDATAV_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFLEVEL                           (0x1UL << 20)                             /**<  */
#define _LESENSE_IFS_BUFLEVEL_SHIFT                    20                                        /**< Shift value for LESENSE_BUFLEVEL */
#define _LESENSE_IFS_BUFLEVEL_MASK                     0x100000UL                                /**< Bit mask for LESENSE_BUFLEVEL */
#define _LESENSE_IFS_BUFLEVEL_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFLEVEL_DEFAULT                   (_LESENSE_IFS_BUFLEVEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFOF                              (0x1UL << 21)                             /**<  */
#define _LESENSE_IFS_BUFOF_SHIFT                       21                                        /**< Shift value for LESENSE_BUFOF */
#define _LESENSE_IFS_BUFOF_MASK                        0x200000UL                                /**< Bit mask for LESENSE_BUFOF */
#define _LESENSE_IFS_BUFOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFOF_DEFAULT                      (_LESENSE_IFS_BUFOF_DEFAULT << 21)        /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CNTOF                              (0x1UL << 22)                             /**<  */
#define _LESENSE_IFS_CNTOF_SHIFT                       22                                        /**< Shift value for LESENSE_CNTOF */
#define _LESENSE_IFS_CNTOF_MASK                        0x400000UL                                /**< Bit mask for LESENSE_CNTOF */
#define _LESENSE_IFS_CNTOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CNTOF_DEFAULT                      (_LESENSE_IFS_CNTOF_DEFAULT << 22)        /**< Shifted mode DEFAULT for LESENSE_IFS */

/* Bit fields for LESENSE IEN */
#define _LESENSE_IEN_RESETVALUE                        0x00000000UL                              /**< Default value for LESENSE_IEN */
#define _LESENSE_IEN_MASK                              0x007FFFFFUL                              /**< Mask for LESENSE_IEN */
#define LESENSE_IEN_CH0                                (0x1UL << 0)                              /**<  */
#define _LESENSE_IEN_CH0_SHIFT                         0                                         /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IEN_CH0_MASK                          0x1UL                                     /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IEN_CH0_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH0_DEFAULT                        (_LESENSE_IEN_CH0_DEFAULT << 0)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH1                                (0x1UL << 1)                              /**<  */
#define _LESENSE_IEN_CH1_SHIFT                         1                                         /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IEN_CH1_MASK                          0x2UL                                     /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IEN_CH1_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH1_DEFAULT                        (_LESENSE_IEN_CH1_DEFAULT << 1)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH2                                (0x1UL << 2)                              /**<  */
#define _LESENSE_IEN_CH2_SHIFT                         2                                         /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IEN_CH2_MASK                          0x4UL                                     /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IEN_CH2_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH2_DEFAULT                        (_LESENSE_IEN_CH2_DEFAULT << 2)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH3                                (0x1UL << 3)                              /**<  */
#define _LESENSE_IEN_CH3_SHIFT                         3                                         /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IEN_CH3_MASK                          0x8UL                                     /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IEN_CH3_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH3_DEFAULT                        (_LESENSE_IEN_CH3_DEFAULT << 3)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH4                                (0x1UL << 4)                              /**<  */
#define _LESENSE_IEN_CH4_SHIFT                         4                                         /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IEN_CH4_MASK                          0x10UL                                    /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IEN_CH4_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH4_DEFAULT                        (_LESENSE_IEN_CH4_DEFAULT << 4)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH5                                (0x1UL << 5)                              /**<  */
#define _LESENSE_IEN_CH5_SHIFT                         5                                         /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IEN_CH5_MASK                          0x20UL                                    /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IEN_CH5_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH5_DEFAULT                        (_LESENSE_IEN_CH5_DEFAULT << 5)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH6                                (0x1UL << 6)                              /**<  */
#define _LESENSE_IEN_CH6_SHIFT                         6                                         /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IEN_CH6_MASK                          0x40UL                                    /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IEN_CH6_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH6_DEFAULT                        (_LESENSE_IEN_CH6_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH7                                (0x1UL << 7)                              /**<  */
#define _LESENSE_IEN_CH7_SHIFT                         7                                         /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IEN_CH7_MASK                          0x80UL                                    /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IEN_CH7_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH7_DEFAULT                        (_LESENSE_IEN_CH7_DEFAULT << 7)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH8                                (0x1UL << 8)                              /**<  */
#define _LESENSE_IEN_CH8_SHIFT                         8                                         /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IEN_CH8_MASK                          0x100UL                                   /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IEN_CH8_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH8_DEFAULT                        (_LESENSE_IEN_CH8_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH9                                (0x1UL << 9)                              /**<  */
#define _LESENSE_IEN_CH9_SHIFT                         9                                         /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IEN_CH9_MASK                          0x200UL                                   /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IEN_CH9_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH9_DEFAULT                        (_LESENSE_IEN_CH9_DEFAULT << 9)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH10                               (0x1UL << 10)                             /**<  */
#define _LESENSE_IEN_CH10_SHIFT                        10                                        /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IEN_CH10_MASK                         0x400UL                                   /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IEN_CH10_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH10_DEFAULT                       (_LESENSE_IEN_CH10_DEFAULT << 10)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH11                               (0x1UL << 11)                             /**<  */
#define _LESENSE_IEN_CH11_SHIFT                        11                                        /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IEN_CH11_MASK                         0x800UL                                   /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IEN_CH11_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH11_DEFAULT                       (_LESENSE_IEN_CH11_DEFAULT << 11)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH12                               (0x1UL << 12)                             /**<  */
#define _LESENSE_IEN_CH12_SHIFT                        12                                        /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IEN_CH12_MASK                         0x1000UL                                  /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IEN_CH12_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH12_DEFAULT                       (_LESENSE_IEN_CH12_DEFAULT << 12)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH13                               (0x1UL << 13)                             /**<  */
#define _LESENSE_IEN_CH13_SHIFT                        13                                        /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IEN_CH13_MASK                         0x2000UL                                  /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IEN_CH13_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH13_DEFAULT                       (_LESENSE_IEN_CH13_DEFAULT << 13)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH14                               (0x1UL << 14)                             /**<  */
#define _LESENSE_IEN_CH14_SHIFT                        14                                        /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IEN_CH14_MASK                         0x4000UL                                  /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IEN_CH14_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH14_DEFAULT                       (_LESENSE_IEN_CH14_DEFAULT << 14)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH15                               (0x1UL << 15)                             /**<  */
#define _LESENSE_IEN_CH15_SHIFT                        15                                        /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IEN_CH15_MASK                         0x8000UL                                  /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IEN_CH15_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH15_DEFAULT                       (_LESENSE_IEN_CH15_DEFAULT << 15)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_SCANCOMPLETE                       (0x1UL << 16)                             /**<  */
#define _LESENSE_IEN_SCANCOMPLETE_SHIFT                16                                        /**< Shift value for LESENSE_SCANCOMPLETE */
#define _LESENSE_IEN_SCANCOMPLETE_MASK                 0x10000UL                                 /**< Bit mask for LESENSE_SCANCOMPLETE */
#define _LESENSE_IEN_SCANCOMPLETE_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_SCANCOMPLETE_DEFAULT               (_LESENSE_IEN_SCANCOMPLETE_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_DEC                                (0x1UL << 17)                             /**<  */
#define _LESENSE_IEN_DEC_SHIFT                         17                                        /**< Shift value for LESENSE_DEC */
#define _LESENSE_IEN_DEC_MASK                          0x20000UL                                 /**< Bit mask for LESENSE_DEC */
#define _LESENSE_IEN_DEC_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_DEC_DEFAULT                        (_LESENSE_IEN_DEC_DEFAULT << 17)          /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_DECERR                             (0x1UL << 18)                             /**<  */
#define _LESENSE_IEN_DECERR_SHIFT                      18                                        /**< Shift value for LESENSE_DECERR */
#define _LESENSE_IEN_DECERR_MASK                       0x40000UL                                 /**< Bit mask for LESENSE_DECERR */
#define _LESENSE_IEN_DECERR_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_DECERR_DEFAULT                     (_LESENSE_IEN_DECERR_DEFAULT << 18)       /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFDATAV                           (0x1UL << 19)                             /**<  */
#define _LESENSE_IEN_BUFDATAV_SHIFT                    19                                        /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_IEN_BUFDATAV_MASK                     0x80000UL                                 /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_IEN_BUFDATAV_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFDATAV_DEFAULT                   (_LESENSE_IEN_BUFDATAV_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFLEVEL                           (0x1UL << 20)                             /**<  */
#define _LESENSE_IEN_BUFLEVEL_SHIFT                    20                                        /**< Shift value for LESENSE_BUFLEVEL */
#define _LESENSE_IEN_BUFLEVEL_MASK                     0x100000UL                                /**< Bit mask for LESENSE_BUFLEVEL */
#define _LESENSE_IEN_BUFLEVEL_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFLEVEL_DEFAULT                   (_LESENSE_IEN_BUFLEVEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFOF                              (0x1UL << 21)                             /**<  */
#define _LESENSE_IEN_BUFOF_SHIFT                       21                                        /**< Shift value for LESENSE_BUFOF */
#define _LESENSE_IEN_BUFOF_MASK                        0x200000UL                                /**< Bit mask for LESENSE_BUFOF */
#define _LESENSE_IEN_BUFOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFOF_DEFAULT                      (_LESENSE_IEN_BUFOF_DEFAULT << 21)        /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CNTOF                              (0x1UL << 22)                             /**<  */
#define _LESENSE_IEN_CNTOF_SHIFT                       22                                        /**< Shift value for LESENSE_CNTOF */
#define _LESENSE_IEN_CNTOF_MASK                        0x400000UL                                /**< Bit mask for LESENSE_CNTOF */
#define _LESENSE_IEN_CNTOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CNTOF_DEFAULT                      (_LESENSE_IEN_CNTOF_DEFAULT << 22)        /**< Shifted mode DEFAULT for LESENSE_IEN */

/* Bit fields for LESENSE SYNCBUSY */
#define _LESENSE_SYNCBUSY_RESETVALUE                   0x00000000UL                                  /**< Default value for LESENSE_SYNCBUSY */
#define _LESENSE_SYNCBUSY_MASK                         0x07E3FFFFUL                                  /**< Mask for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CTRL                          (0x1UL << 0)                                  /**< LESENSE_CTRL Register Busy */
#define _LESENSE_SYNCBUSY_CTRL_SHIFT                   0                                             /**< Shift value for LESENSE_CTRL */
#define _LESENSE_SYNCBUSY_CTRL_MASK                    0x1UL                                         /**< Bit mask for LESENSE_CTRL */
#define _LESENSE_SYNCBUSY_CTRL_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CTRL_DEFAULT                  (_LESENSE_SYNCBUSY_CTRL_DEFAULT << 0)         /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_TIMCTRL                       (0x1UL << 1)                                  /**< LESENSE_TIMCTRL Register Busy */
#define _LESENSE_SYNCBUSY_TIMCTRL_SHIFT                1                                             /**< Shift value for LESENSE_TIMCTRL */
#define _LESENSE_SYNCBUSY_TIMCTRL_MASK                 0x2UL                                         /**< Bit mask for LESENSE_TIMCTRL */
#define _LESENSE_SYNCBUSY_TIMCTRL_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_TIMCTRL_DEFAULT               (_LESENSE_SYNCBUSY_TIMCTRL_DEFAULT << 1)      /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_PERCTRL                       (0x1UL << 2)                                  /**< LESENSE_PERCTRL Register Busy */
#define _LESENSE_SYNCBUSY_PERCTRL_SHIFT                2                                             /**< Shift value for LESENSE_PERCTRL */
#define _LESENSE_SYNCBUSY_PERCTRL_MASK                 0x4UL                                         /**< Bit mask for LESENSE_PERCTRL */
#define _LESENSE_SYNCBUSY_PERCTRL_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_PERCTRL_DEFAULT               (_LESENSE_SYNCBUSY_PERCTRL_DEFAULT << 2)      /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_DECCTRL                       (0x1UL << 3)                                  /**< LESENSE_DECCTRL Register Busy */
#define _LESENSE_SYNCBUSY_DECCTRL_SHIFT                3                                             /**< Shift value for LESENSE_DECCTRL */
#define _LESENSE_SYNCBUSY_DECCTRL_MASK                 0x8UL                                         /**< Bit mask for LESENSE_DECCTRL */
#define _LESENSE_SYNCBUSY_DECCTRL_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_DECCTRL_DEFAULT               (_LESENSE_SYNCBUSY_DECCTRL_DEFAULT << 3)      /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_BIASCTRL                      (0x1UL << 4)                                  /**< LESENSE_BIASCTRL Register Busy */
#define _LESENSE_SYNCBUSY_BIASCTRL_SHIFT               4                                             /**< Shift value for LESENSE_BIASCTRL */
#define _LESENSE_SYNCBUSY_BIASCTRL_MASK                0x10UL                                        /**< Bit mask for LESENSE_BIASCTRL */
#define _LESENSE_SYNCBUSY_BIASCTRL_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_BIASCTRL_DEFAULT              (_LESENSE_SYNCBUSY_BIASCTRL_DEFAULT << 4)     /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CMD                           (0x1UL << 5)                                  /**< LESENSE_CMD Register Busy */
#define _LESENSE_SYNCBUSY_CMD_SHIFT                    5                                             /**< Shift value for LESENSE_CMD */
#define _LESENSE_SYNCBUSY_CMD_MASK                     0x20UL                                        /**< Bit mask for LESENSE_CMD */
#define _LESENSE_SYNCBUSY_CMD_DEFAULT                  0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CMD_DEFAULT                   (_LESENSE_SYNCBUSY_CMD_DEFAULT << 5)          /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CHEN                          (0x1UL << 6)                                  /**< LESENSE_CHEN Register Busy */
#define _LESENSE_SYNCBUSY_CHEN_SHIFT                   6                                             /**< Shift value for LESENSE_CHEN */
#define _LESENSE_SYNCBUSY_CHEN_MASK                    0x40UL                                        /**< Bit mask for LESENSE_CHEN */
#define _LESENSE_SYNCBUSY_CHEN_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CHEN_DEFAULT                  (_LESENSE_SYNCBUSY_CHEN_DEFAULT << 6)         /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_SCANRES                       (0x1UL << 7)                                  /**< LESENSE_SCANRES Register Busy */
#define _LESENSE_SYNCBUSY_SCANRES_SHIFT                7                                             /**< Shift value for LESENSE_SCANRES */
#define _LESENSE_SYNCBUSY_SCANRES_MASK                 0x80UL                                        /**< Bit mask for LESENSE_SCANRES */
#define _LESENSE_SYNCBUSY_SCANRES_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_SCANRES_DEFAULT               (_LESENSE_SYNCBUSY_SCANRES_DEFAULT << 7)      /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_STATUS                        (0x1UL << 8)                                  /**< LESENSE_STATUS Register Busy */
#define _LESENSE_SYNCBUSY_STATUS_SHIFT                 8                                             /**< Shift value for LESENSE_STATUS */
#define _LESENSE_SYNCBUSY_STATUS_MASK                  0x100UL                                       /**< Bit mask for LESENSE_STATUS */
#define _LESENSE_SYNCBUSY_STATUS_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_STATUS_DEFAULT                (_LESENSE_SYNCBUSY_STATUS_DEFAULT << 8)       /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_PTR                           (0x1UL << 9)                                  /**< LESENSE_PTR Register Busy */
#define _LESENSE_SYNCBUSY_PTR_SHIFT                    9                                             /**< Shift value for LESENSE_PTR */
#define _LESENSE_SYNCBUSY_PTR_MASK                     0x200UL                                       /**< Bit mask for LESENSE_PTR */
#define _LESENSE_SYNCBUSY_PTR_DEFAULT                  0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_PTR_DEFAULT                   (_LESENSE_SYNCBUSY_PTR_DEFAULT << 9)          /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_BUFDATA                       (0x1UL << 10)                                 /**< LESENSE_BUFDATA Register Busy */
#define _LESENSE_SYNCBUSY_BUFDATA_SHIFT                10                                            /**< Shift value for LESENSE_BUFDATA */
#define _LESENSE_SYNCBUSY_BUFDATA_MASK                 0x400UL                                       /**< Bit mask for LESENSE_BUFDATA */
#define _LESENSE_SYNCBUSY_BUFDATA_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_BUFDATA_DEFAULT               (_LESENSE_SYNCBUSY_BUFDATA_DEFAULT << 10)     /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CURCH                         (0x1UL << 11)                                 /**< LESENSE_CURCH Register Busy */
#define _LESENSE_SYNCBUSY_CURCH_SHIFT                  11                                            /**< Shift value for LESENSE_CURCH */
#define _LESENSE_SYNCBUSY_CURCH_MASK                   0x800UL                                       /**< Bit mask for LESENSE_CURCH */
#define _LESENSE_SYNCBUSY_CURCH_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CURCH_DEFAULT                 (_LESENSE_SYNCBUSY_CURCH_DEFAULT << 11)       /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_DECSTATE                      (0x1UL << 12)                                 /**< LESENSE_DECSTATE Register Busy */
#define _LESENSE_SYNCBUSY_DECSTATE_SHIFT               12                                            /**< Shift value for LESENSE_DECSTATE */
#define _LESENSE_SYNCBUSY_DECSTATE_MASK                0x1000UL                                      /**< Bit mask for LESENSE_DECSTATE */
#define _LESENSE_SYNCBUSY_DECSTATE_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_DECSTATE_DEFAULT              (_LESENSE_SYNCBUSY_DECSTATE_DEFAULT << 12)    /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_SENSORSTATE                   (0x1UL << 13)                                 /**< LESENSE_SENSORSTATE Register Busy */
#define _LESENSE_SYNCBUSY_SENSORSTATE_SHIFT            13                                            /**< Shift value for LESENSE_SENSORSTATE */
#define _LESENSE_SYNCBUSY_SENSORSTATE_MASK             0x2000UL                                      /**< Bit mask for LESENSE_SENSORSTATE */
#define _LESENSE_SYNCBUSY_SENSORSTATE_DEFAULT          0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_SENSORSTATE_DEFAULT           (_LESENSE_SYNCBUSY_SENSORSTATE_DEFAULT << 13) /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_IDLECONF                      (0x1UL << 14)                                 /**< LESENSE_IDLECONF Register Busy */
#define _LESENSE_SYNCBUSY_IDLECONF_SHIFT               14                                            /**< Shift value for LESENSE_IDLECONF */
#define _LESENSE_SYNCBUSY_IDLECONF_MASK                0x4000UL                                      /**< Bit mask for LESENSE_IDLECONF */
#define _LESENSE_SYNCBUSY_IDLECONF_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_IDLECONF_DEFAULT              (_LESENSE_SYNCBUSY_IDLECONF_DEFAULT << 14)    /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_ALTEXCONF                     (0x1UL << 15)                                 /**< LESENSE_ALTEXCONF Register Busy */
#define _LESENSE_SYNCBUSY_ALTEXCONF_SHIFT              15                                            /**< Shift value for LESENSE_ALTEXCONF */
#define _LESENSE_SYNCBUSY_ALTEXCONF_MASK               0x8000UL                                      /**< Bit mask for LESENSE_ALTEXCONF */
#define _LESENSE_SYNCBUSY_ALTEXCONF_DEFAULT            0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_ALTEXCONF_DEFAULT             (_LESENSE_SYNCBUSY_ALTEXCONF_DEFAULT << 15)   /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_ROUTE                         (0x1UL << 16)                                 /**< LESENSE_ROUTE Register Busy */
#define _LESENSE_SYNCBUSY_ROUTE_SHIFT                  16                                            /**< Shift value for LESENSE_ROUTE */
#define _LESENSE_SYNCBUSY_ROUTE_MASK                   0x10000UL                                     /**< Bit mask for LESENSE_ROUTE */
#define _LESENSE_SYNCBUSY_ROUTE_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_ROUTE_DEFAULT                 (_LESENSE_SYNCBUSY_ROUTE_DEFAULT << 16)       /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_POWERDOWN                     (0x1UL << 17)                                 /**< LESENSE_POWERDOWN Register Busy */
#define _LESENSE_SYNCBUSY_POWERDOWN_SHIFT              17                                            /**< Shift value for LESENSE_POWERDOWN */
#define _LESENSE_SYNCBUSY_POWERDOWN_MASK               0x20000UL                                     /**< Bit mask for LESENSE_POWERDOWN */
#define _LESENSE_SYNCBUSY_POWERDOWN_DEFAULT            0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_POWERDOWN_DEFAULT             (_LESENSE_SYNCBUSY_POWERDOWN_DEFAULT << 17)   /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_TCONFA                        (0x1UL << 21)                                 /**< LESENSE_STx_TCONFA Register Busy */
#define _LESENSE_SYNCBUSY_TCONFA_SHIFT                 21                                            /**< Shift value for LESENSE_TCONFA */
#define _LESENSE_SYNCBUSY_TCONFA_MASK                  0x200000UL                                    /**< Bit mask for LESENSE_TCONFA */
#define _LESENSE_SYNCBUSY_TCONFA_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_TCONFA_DEFAULT                (_LESENSE_SYNCBUSY_TCONFA_DEFAULT << 21)      /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_TCONFB                        (0x1UL << 22)                                 /**< LESENSE_STx_TCONFB Register Busy */
#define _LESENSE_SYNCBUSY_TCONFB_SHIFT                 22                                            /**< Shift value for LESENSE_TCONFB */
#define _LESENSE_SYNCBUSY_TCONFB_MASK                  0x400000UL                                    /**< Bit mask for LESENSE_TCONFB */
#define _LESENSE_SYNCBUSY_TCONFB_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_TCONFB_DEFAULT                (_LESENSE_SYNCBUSY_TCONFB_DEFAULT << 22)      /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_DATA                          (0x1UL << 23)                                 /**< LESENSE_BUFx_DATA Register Busy */
#define _LESENSE_SYNCBUSY_DATA_SHIFT                   23                                            /**< Shift value for LESENSE_DATA */
#define _LESENSE_SYNCBUSY_DATA_MASK                    0x800000UL                                    /**< Bit mask for LESENSE_DATA */
#define _LESENSE_SYNCBUSY_DATA_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_DATA_DEFAULT                  (_LESENSE_SYNCBUSY_DATA_DEFAULT << 23)        /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_TIMING                        (0x1UL << 24)                                 /**< LESENSE_CHx_TIMING Register Busy */
#define _LESENSE_SYNCBUSY_TIMING_SHIFT                 24                                            /**< Shift value for LESENSE_TIMING */
#define _LESENSE_SYNCBUSY_TIMING_MASK                  0x1000000UL                                   /**< Bit mask for LESENSE_TIMING */
#define _LESENSE_SYNCBUSY_TIMING_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_TIMING_DEFAULT                (_LESENSE_SYNCBUSY_TIMING_DEFAULT << 24)      /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_INTERACT                      (0x1UL << 25)                                 /**< LESENSE_CHx_INTERACT Register Busy */
#define _LESENSE_SYNCBUSY_INTERACT_SHIFT               25                                            /**< Shift value for LESENSE_INTERACT */
#define _LESENSE_SYNCBUSY_INTERACT_MASK                0x2000000UL                                   /**< Bit mask for LESENSE_INTERACT */
#define _LESENSE_SYNCBUSY_INTERACT_DEFAULT             0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_INTERACT_DEFAULT              (_LESENSE_SYNCBUSY_INTERACT_DEFAULT << 25)    /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_EVAL                          (0x1UL << 26)                                 /**< LESENSE_CHx_EVAL Register Busy */
#define _LESENSE_SYNCBUSY_EVAL_SHIFT                   26                                            /**< Shift value for LESENSE_EVAL */
#define _LESENSE_SYNCBUSY_EVAL_MASK                    0x4000000UL                                   /**< Bit mask for LESENSE_EVAL */
#define _LESENSE_SYNCBUSY_EVAL_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_EVAL_DEFAULT                  (_LESENSE_SYNCBUSY_EVAL_DEFAULT << 26)        /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */

/* Bit fields for LESENSE ROUTE */
#define _LESENSE_ROUTE_RESETVALUE                      0x00000000UL                             /**< Default value for LESENSE_ROUTE */
#define _LESENSE_ROUTE_MASK                            0x00FFFFFFUL                             /**< Mask for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH0PEN                           (0x1UL << 0)                             /**< CH0 Pin Enable */
#define _LESENSE_ROUTE_CH0PEN_SHIFT                    0                                        /**< Shift value for LESENSE_CH0PEN */
#define _LESENSE_ROUTE_CH0PEN_MASK                     0x1UL                                    /**< Bit mask for LESENSE_CH0PEN */
#define _LESENSE_ROUTE_CH0PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH0PEN_DEFAULT                   (_LESENSE_ROUTE_CH0PEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH1PEN                           (0x1UL << 1)                             /**< CH0 Pin Enable */
#define _LESENSE_ROUTE_CH1PEN_SHIFT                    1                                        /**< Shift value for LESENSE_CH1PEN */
#define _LESENSE_ROUTE_CH1PEN_MASK                     0x2UL                                    /**< Bit mask for LESENSE_CH1PEN */
#define _LESENSE_ROUTE_CH1PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH1PEN_DEFAULT                   (_LESENSE_ROUTE_CH1PEN_DEFAULT << 1)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH2PEN                           (0x1UL << 2)                             /**< CH2 Pin Enable */
#define _LESENSE_ROUTE_CH2PEN_SHIFT                    2                                        /**< Shift value for LESENSE_CH2PEN */
#define _LESENSE_ROUTE_CH2PEN_MASK                     0x4UL                                    /**< Bit mask for LESENSE_CH2PEN */
#define _LESENSE_ROUTE_CH2PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH2PEN_DEFAULT                   (_LESENSE_ROUTE_CH2PEN_DEFAULT << 2)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH3PEN                           (0x1UL << 3)                             /**< CH3 Pin Enable */
#define _LESENSE_ROUTE_CH3PEN_SHIFT                    3                                        /**< Shift value for LESENSE_CH3PEN */
#define _LESENSE_ROUTE_CH3PEN_MASK                     0x8UL                                    /**< Bit mask for LESENSE_CH3PEN */
#define _LESENSE_ROUTE_CH3PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH3PEN_DEFAULT                   (_LESENSE_ROUTE_CH3PEN_DEFAULT << 3)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH4PEN                           (0x1UL << 4)                             /**< CH4 Pin Enable */
#define _LESENSE_ROUTE_CH4PEN_SHIFT                    4                                        /**< Shift value for LESENSE_CH4PEN */
#define _LESENSE_ROUTE_CH4PEN_MASK                     0x10UL                                   /**< Bit mask for LESENSE_CH4PEN */
#define _LESENSE_ROUTE_CH4PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH4PEN_DEFAULT                   (_LESENSE_ROUTE_CH4PEN_DEFAULT << 4)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH5PEN                           (0x1UL << 5)                             /**< CH5 Pin Enable */
#define _LESENSE_ROUTE_CH5PEN_SHIFT                    5                                        /**< Shift value for LESENSE_CH5PEN */
#define _LESENSE_ROUTE_CH5PEN_MASK                     0x20UL                                   /**< Bit mask for LESENSE_CH5PEN */
#define _LESENSE_ROUTE_CH5PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH5PEN_DEFAULT                   (_LESENSE_ROUTE_CH5PEN_DEFAULT << 5)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH6PEN                           (0x1UL << 6)                             /**< CH6 Pin Enable */
#define _LESENSE_ROUTE_CH6PEN_SHIFT                    6                                        /**< Shift value for LESENSE_CH6PEN */
#define _LESENSE_ROUTE_CH6PEN_MASK                     0x40UL                                   /**< Bit mask for LESENSE_CH6PEN */
#define _LESENSE_ROUTE_CH6PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH6PEN_DEFAULT                   (_LESENSE_ROUTE_CH6PEN_DEFAULT << 6)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH7PEN                           (0x1UL << 7)                             /**< CH7 Pin Enable */
#define _LESENSE_ROUTE_CH7PEN_SHIFT                    7                                        /**< Shift value for LESENSE_CH7PEN */
#define _LESENSE_ROUTE_CH7PEN_MASK                     0x80UL                                   /**< Bit mask for LESENSE_CH7PEN */
#define _LESENSE_ROUTE_CH7PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH7PEN_DEFAULT                   (_LESENSE_ROUTE_CH7PEN_DEFAULT << 7)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH8PEN                           (0x1UL << 8)                             /**< CH8 Pin Enable */
#define _LESENSE_ROUTE_CH8PEN_SHIFT                    8                                        /**< Shift value for LESENSE_CH8PEN */
#define _LESENSE_ROUTE_CH8PEN_MASK                     0x100UL                                  /**< Bit mask for LESENSE_CH8PEN */
#define _LESENSE_ROUTE_CH8PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH8PEN_DEFAULT                   (_LESENSE_ROUTE_CH8PEN_DEFAULT << 8)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH9PEN                           (0x1UL << 9)                             /**< CH9 Pin Enable */
#define _LESENSE_ROUTE_CH9PEN_SHIFT                    9                                        /**< Shift value for LESENSE_CH9PEN */
#define _LESENSE_ROUTE_CH9PEN_MASK                     0x200UL                                  /**< Bit mask for LESENSE_CH9PEN */
#define _LESENSE_ROUTE_CH9PEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH9PEN_DEFAULT                   (_LESENSE_ROUTE_CH9PEN_DEFAULT << 9)     /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH10PEN                          (0x1UL << 10)                            /**< CH10 Pin Enable */
#define _LESENSE_ROUTE_CH10PEN_SHIFT                   10                                       /**< Shift value for LESENSE_CH10PEN */
#define _LESENSE_ROUTE_CH10PEN_MASK                    0x400UL                                  /**< Bit mask for LESENSE_CH10PEN */
#define _LESENSE_ROUTE_CH10PEN_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH10PEN_DEFAULT                  (_LESENSE_ROUTE_CH10PEN_DEFAULT << 10)   /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH11PEN                          (0x1UL << 11)                            /**< CH11 Pin Enable */
#define _LESENSE_ROUTE_CH11PEN_SHIFT                   11                                       /**< Shift value for LESENSE_CH11PEN */
#define _LESENSE_ROUTE_CH11PEN_MASK                    0x800UL                                  /**< Bit mask for LESENSE_CH11PEN */
#define _LESENSE_ROUTE_CH11PEN_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH11PEN_DEFAULT                  (_LESENSE_ROUTE_CH11PEN_DEFAULT << 11)   /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH12PEN                          (0x1UL << 12)                            /**< CH12 Pin Enable */
#define _LESENSE_ROUTE_CH12PEN_SHIFT                   12                                       /**< Shift value for LESENSE_CH12PEN */
#define _LESENSE_ROUTE_CH12PEN_MASK                    0x1000UL                                 /**< Bit mask for LESENSE_CH12PEN */
#define _LESENSE_ROUTE_CH12PEN_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH12PEN_DEFAULT                  (_LESENSE_ROUTE_CH12PEN_DEFAULT << 12)   /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH13PEN                          (0x1UL << 13)                            /**< CH13 Pin Enable */
#define _LESENSE_ROUTE_CH13PEN_SHIFT                   13                                       /**< Shift value for LESENSE_CH13PEN */
#define _LESENSE_ROUTE_CH13PEN_MASK                    0x2000UL                                 /**< Bit mask for LESENSE_CH13PEN */
#define _LESENSE_ROUTE_CH13PEN_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH13PEN_DEFAULT                  (_LESENSE_ROUTE_CH13PEN_DEFAULT << 13)   /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH14PEN                          (0x1UL << 14)                            /**< CH14 Pin Enable */
#define _LESENSE_ROUTE_CH14PEN_SHIFT                   14                                       /**< Shift value for LESENSE_CH14PEN */
#define _LESENSE_ROUTE_CH14PEN_MASK                    0x4000UL                                 /**< Bit mask for LESENSE_CH14PEN */
#define _LESENSE_ROUTE_CH14PEN_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH14PEN_DEFAULT                  (_LESENSE_ROUTE_CH14PEN_DEFAULT << 14)   /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH15PEN                          (0x1UL << 15)                            /**< CH15 Pin Enable */
#define _LESENSE_ROUTE_CH15PEN_SHIFT                   15                                       /**< Shift value for LESENSE_CH15PEN */
#define _LESENSE_ROUTE_CH15PEN_MASK                    0x8000UL                                 /**< Bit mask for LESENSE_CH15PEN */
#define _LESENSE_ROUTE_CH15PEN_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_CH15PEN_DEFAULT                  (_LESENSE_ROUTE_CH15PEN_DEFAULT << 15)   /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX0PEN                        (0x1UL << 16)                            /**< ALTEX0 Pin Enable */
#define _LESENSE_ROUTE_ALTEX0PEN_SHIFT                 16                                       /**< Shift value for LESENSE_ALTEX0PEN */
#define _LESENSE_ROUTE_ALTEX0PEN_MASK                  0x10000UL                                /**< Bit mask for LESENSE_ALTEX0PEN */
#define _LESENSE_ROUTE_ALTEX0PEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX0PEN_DEFAULT                (_LESENSE_ROUTE_ALTEX0PEN_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX1PEN                        (0x1UL << 17)                            /**< ALTEX1 Pin Enable */
#define _LESENSE_ROUTE_ALTEX1PEN_SHIFT                 17                                       /**< Shift value for LESENSE_ALTEX1PEN */
#define _LESENSE_ROUTE_ALTEX1PEN_MASK                  0x20000UL                                /**< Bit mask for LESENSE_ALTEX1PEN */
#define _LESENSE_ROUTE_ALTEX1PEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX1PEN_DEFAULT                (_LESENSE_ROUTE_ALTEX1PEN_DEFAULT << 17) /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX2PEN                        (0x1UL << 18)                            /**< ALTEX2 Pin Enable */
#define _LESENSE_ROUTE_ALTEX2PEN_SHIFT                 18                                       /**< Shift value for LESENSE_ALTEX2PEN */
#define _LESENSE_ROUTE_ALTEX2PEN_MASK                  0x40000UL                                /**< Bit mask for LESENSE_ALTEX2PEN */
#define _LESENSE_ROUTE_ALTEX2PEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX2PEN_DEFAULT                (_LESENSE_ROUTE_ALTEX2PEN_DEFAULT << 18) /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX3PEN                        (0x1UL << 19)                            /**< ALTEX3 Pin Enable */
#define _LESENSE_ROUTE_ALTEX3PEN_SHIFT                 19                                       /**< Shift value for LESENSE_ALTEX3PEN */
#define _LESENSE_ROUTE_ALTEX3PEN_MASK                  0x80000UL                                /**< Bit mask for LESENSE_ALTEX3PEN */
#define _LESENSE_ROUTE_ALTEX3PEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX3PEN_DEFAULT                (_LESENSE_ROUTE_ALTEX3PEN_DEFAULT << 19) /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX4PEN                        (0x1UL << 20)                            /**< ALTEX4 Pin Enable */
#define _LESENSE_ROUTE_ALTEX4PEN_SHIFT                 20                                       /**< Shift value for LESENSE_ALTEX4PEN */
#define _LESENSE_ROUTE_ALTEX4PEN_MASK                  0x100000UL                               /**< Bit mask for LESENSE_ALTEX4PEN */
#define _LESENSE_ROUTE_ALTEX4PEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX4PEN_DEFAULT                (_LESENSE_ROUTE_ALTEX4PEN_DEFAULT << 20) /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX5PEN                        (0x1UL << 21)                            /**< ALTEX5 Pin Enable */
#define _LESENSE_ROUTE_ALTEX5PEN_SHIFT                 21                                       /**< Shift value for LESENSE_ALTEX5PEN */
#define _LESENSE_ROUTE_ALTEX5PEN_MASK                  0x200000UL                               /**< Bit mask for LESENSE_ALTEX5PEN */
#define _LESENSE_ROUTE_ALTEX5PEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX5PEN_DEFAULT                (_LESENSE_ROUTE_ALTEX5PEN_DEFAULT << 21) /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX6PEN                        (0x1UL << 22)                            /**< ALTEX6 Pin Enable */
#define _LESENSE_ROUTE_ALTEX6PEN_SHIFT                 22                                       /**< Shift value for LESENSE_ALTEX6PEN */
#define _LESENSE_ROUTE_ALTEX6PEN_MASK                  0x400000UL                               /**< Bit mask for LESENSE_ALTEX6PEN */
#define _LESENSE_ROUTE_ALTEX6PEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX6PEN_DEFAULT                (_LESENSE_ROUTE_ALTEX6PEN_DEFAULT << 22) /**< Shifted mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX7PEN                        (0x1UL << 23)                            /**< ALTEX7 Pin Enable */
#define _LESENSE_ROUTE_ALTEX7PEN_SHIFT                 23                                       /**< Shift value for LESENSE_ALTEX7PEN */
#define _LESENSE_ROUTE_ALTEX7PEN_MASK                  0x800000UL                               /**< Bit mask for LESENSE_ALTEX7PEN */
#define _LESENSE_ROUTE_ALTEX7PEN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_ROUTE */
#define LESENSE_ROUTE_ALTEX7PEN_DEFAULT                (_LESENSE_ROUTE_ALTEX7PEN_DEFAULT << 23) /**< Shifted mode DEFAULT for LESENSE_ROUTE */

/* Bit fields for LESENSE POWERDOWN */
#define _LESENSE_POWERDOWN_RESETVALUE                  0x00000000UL                          /**< Default value for LESENSE_POWERDOWN */
#define _LESENSE_POWERDOWN_MASK                        0x00000001UL                          /**< Mask for LESENSE_POWERDOWN */
#define LESENSE_POWERDOWN_RAM                          (0x1UL << 0)                          /**< LESENSE RAM power-down */
#define _LESENSE_POWERDOWN_RAM_SHIFT                   0                                     /**< Shift value for LESENSE_RAM */
#define _LESENSE_POWERDOWN_RAM_MASK                    0x1UL                                 /**< Bit mask for LESENSE_RAM */
#define _LESENSE_POWERDOWN_RAM_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for LESENSE_POWERDOWN */
#define LESENSE_POWERDOWN_RAM_DEFAULT                  (_LESENSE_POWERDOWN_RAM_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_POWERDOWN */

/* Bit fields for LESENSE ST_TCONFA */
#define _LESENSE_ST_TCONFA_RESETVALUE                  0x00000000UL                                  /**< Default value for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_MASK                        0x00057FFFUL                                  /**< Mask for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_COMP_SHIFT                  0                                             /**< Shift value for LESENSE_COMP */
#define _LESENSE_ST_TCONFA_COMP_MASK                   0xFUL                                         /**< Bit mask for LESENSE_COMP */
#define _LESENSE_ST_TCONFA_COMP_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_COMP_DEFAULT                 (_LESENSE_ST_TCONFA_COMP_DEFAULT << 0)        /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_MASK_SHIFT                  4                                             /**< Shift value for LESENSE_MASK */
#define _LESENSE_ST_TCONFA_MASK_MASK                   0xF0UL                                        /**< Bit mask for LESENSE_MASK */
#define _LESENSE_ST_TCONFA_MASK_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_MASK_DEFAULT                 (_LESENSE_ST_TCONFA_MASK_DEFAULT << 4)        /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_NEXTSTATE_SHIFT             8                                             /**< Shift value for LESENSE_NEXTSTATE */
#define _LESENSE_ST_TCONFA_NEXTSTATE_MASK              0xF00UL                                       /**< Bit mask for LESENSE_NEXTSTATE */
#define _LESENSE_ST_TCONFA_NEXTSTATE_DEFAULT           0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_NEXTSTATE_DEFAULT            (_LESENSE_ST_TCONFA_NEXTSTATE_DEFAULT << 8)   /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_SHIFT                12                                            /**< Shift value for LESENSE_PRSACT */
#define _LESENSE_ST_TCONFA_PRSACT_MASK                 0x7000UL                                      /**< Bit mask for LESENSE_PRSACT */
#define _LESENSE_ST_TCONFA_PRSACT_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_NONE                 0x00000000UL                                  /**< Mode NONE for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_UP                   0x00000001UL                                  /**< Mode UP for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_PRS0                 0x00000001UL                                  /**< Mode PRS0 for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_PRS1                 0x00000002UL                                  /**< Mode PRS1 for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_DOWN                 0x00000002UL                                  /**< Mode DOWN for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_PRS01                0x00000003UL                                  /**< Mode PRS01 for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_PRS2                 0x00000004UL                                  /**< Mode PRS2 for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_PRS02                0x00000005UL                                  /**< Mode PRS02 for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_UPANDPRS2            0x00000005UL                                  /**< Mode UPANDPRS2 for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_PRS12                0x00000006UL                                  /**< Mode PRS12 for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_DOWNANDPRS2          0x00000006UL                                  /**< Mode DOWNANDPRS2 for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_PRS012               0x00000007UL                                  /**< Mode PRS012 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_DEFAULT               (_LESENSE_ST_TCONFA_PRSACT_DEFAULT << 12)     /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_NONE                  (_LESENSE_ST_TCONFA_PRSACT_NONE << 12)        /**< Shifted mode NONE for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_UP                    (_LESENSE_ST_TCONFA_PRSACT_UP << 12)          /**< Shifted mode UP for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS0                  (_LESENSE_ST_TCONFA_PRSACT_PRS0 << 12)        /**< Shifted mode PRS0 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS1                  (_LESENSE_ST_TCONFA_PRSACT_PRS1 << 12)        /**< Shifted mode PRS1 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_DOWN                  (_LESENSE_ST_TCONFA_PRSACT_DOWN << 12)        /**< Shifted mode DOWN for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS01                 (_LESENSE_ST_TCONFA_PRSACT_PRS01 << 12)       /**< Shifted mode PRS01 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS2                  (_LESENSE_ST_TCONFA_PRSACT_PRS2 << 12)        /**< Shifted mode PRS2 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS02                 (_LESENSE_ST_TCONFA_PRSACT_PRS02 << 12)       /**< Shifted mode PRS02 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_UPANDPRS2             (_LESENSE_ST_TCONFA_PRSACT_UPANDPRS2 << 12)   /**< Shifted mode UPANDPRS2 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS12                 (_LESENSE_ST_TCONFA_PRSACT_PRS12 << 12)       /**< Shifted mode PRS12 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_DOWNANDPRS2           (_LESENSE_ST_TCONFA_PRSACT_DOWNANDPRS2 << 12) /**< Shifted mode DOWNANDPRS2 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS012                (_LESENSE_ST_TCONFA_PRSACT_PRS012 << 12)      /**< Shifted mode PRS012 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_SETIF                        (0x1UL << 16)                                 /**< Set interrupt flag enable */
#define _LESENSE_ST_TCONFA_SETIF_SHIFT                 16                                            /**< Shift value for LESENSE_SETIF */
#define _LESENSE_ST_TCONFA_SETIF_MASK                  0x10000UL                                     /**< Bit mask for LESENSE_SETIF */
#define _LESENSE_ST_TCONFA_SETIF_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_SETIF_DEFAULT                (_LESENSE_ST_TCONFA_SETIF_DEFAULT << 16)      /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_CHAIN                        (0x1UL << 18)                                 /**< Enable state descriptor chaining */
#define _LESENSE_ST_TCONFA_CHAIN_SHIFT                 18                                            /**< Shift value for LESENSE_CHAIN */
#define _LESENSE_ST_TCONFA_CHAIN_MASK                  0x40000UL                                     /**< Bit mask for LESENSE_CHAIN */
#define _LESENSE_ST_TCONFA_CHAIN_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_CHAIN_DEFAULT                (_LESENSE_ST_TCONFA_CHAIN_DEFAULT << 18)      /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */

/* Bit fields for LESENSE ST_TCONFB */
#define _LESENSE_ST_TCONFB_RESETVALUE                  0x00000000UL                                  /**< Default value for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_MASK                        0x00017FFFUL                                  /**< Mask for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_COMP_SHIFT                  0                                             /**< Shift value for LESENSE_COMP */
#define _LESENSE_ST_TCONFB_COMP_MASK                   0xFUL                                         /**< Bit mask for LESENSE_COMP */
#define _LESENSE_ST_TCONFB_COMP_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_COMP_DEFAULT                 (_LESENSE_ST_TCONFB_COMP_DEFAULT << 0)        /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_MASK_SHIFT                  4                                             /**< Shift value for LESENSE_MASK */
#define _LESENSE_ST_TCONFB_MASK_MASK                   0xF0UL                                        /**< Bit mask for LESENSE_MASK */
#define _LESENSE_ST_TCONFB_MASK_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_MASK_DEFAULT                 (_LESENSE_ST_TCONFB_MASK_DEFAULT << 4)        /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_NEXTSTATE_SHIFT             8                                             /**< Shift value for LESENSE_NEXTSTATE */
#define _LESENSE_ST_TCONFB_NEXTSTATE_MASK              0xF00UL                                       /**< Bit mask for LESENSE_NEXTSTATE */
#define _LESENSE_ST_TCONFB_NEXTSTATE_DEFAULT           0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_NEXTSTATE_DEFAULT            (_LESENSE_ST_TCONFB_NEXTSTATE_DEFAULT << 8)   /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_SHIFT                12                                            /**< Shift value for LESENSE_PRSACT */
#define _LESENSE_ST_TCONFB_PRSACT_MASK                 0x7000UL                                      /**< Bit mask for LESENSE_PRSACT */
#define _LESENSE_ST_TCONFB_PRSACT_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_NONE                 0x00000000UL                                  /**< Mode NONE for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_UP                   0x00000001UL                                  /**< Mode UP for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_PRS0                 0x00000001UL                                  /**< Mode PRS0 for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_PRS1                 0x00000002UL                                  /**< Mode PRS1 for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_DOWN                 0x00000002UL                                  /**< Mode DOWN for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_PRS01                0x00000003UL                                  /**< Mode PRS01 for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_PRS2                 0x00000004UL                                  /**< Mode PRS2 for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_PRS02                0x00000005UL                                  /**< Mode PRS02 for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_UPANDPRS2            0x00000005UL                                  /**< Mode UPANDPRS2 for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_PRS12                0x00000006UL                                  /**< Mode PRS12 for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_DOWNANDPRS2          0x00000006UL                                  /**< Mode DOWNANDPRS2 for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_PRS012               0x00000007UL                                  /**< Mode PRS012 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_DEFAULT               (_LESENSE_ST_TCONFB_PRSACT_DEFAULT << 12)     /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_NONE                  (_LESENSE_ST_TCONFB_PRSACT_NONE << 12)        /**< Shifted mode NONE for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_UP                    (_LESENSE_ST_TCONFB_PRSACT_UP << 12)          /**< Shifted mode UP for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS0                  (_LESENSE_ST_TCONFB_PRSACT_PRS0 << 12)        /**< Shifted mode PRS0 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS1                  (_LESENSE_ST_TCONFB_PRSACT_PRS1 << 12)        /**< Shifted mode PRS1 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_DOWN                  (_LESENSE_ST_TCONFB_PRSACT_DOWN << 12)        /**< Shifted mode DOWN for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS01                 (_LESENSE_ST_TCONFB_PRSACT_PRS01 << 12)       /**< Shifted mode PRS01 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS2                  (_LESENSE_ST_TCONFB_PRSACT_PRS2 << 12)        /**< Shifted mode PRS2 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS02                 (_LESENSE_ST_TCONFB_PRSACT_PRS02 << 12)       /**< Shifted mode PRS02 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_UPANDPRS2             (_LESENSE_ST_TCONFB_PRSACT_UPANDPRS2 << 12)   /**< Shifted mode UPANDPRS2 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS12                 (_LESENSE_ST_TCONFB_PRSACT_PRS12 << 12)       /**< Shifted mode PRS12 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_DOWNANDPRS2           (_LESENSE_ST_TCONFB_PRSACT_DOWNANDPRS2 << 12) /**< Shifted mode DOWNANDPRS2 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS012                (_LESENSE_ST_TCONFB_PRSACT_PRS012 << 12)      /**< Shifted mode PRS012 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_SETIF                        (0x1UL << 16)                                 /**< Set interrupt flag */
#define _LESENSE_ST_TCONFB_SETIF_SHIFT                 16                                            /**< Shift value for LESENSE_SETIF */
#define _LESENSE_ST_TCONFB_SETIF_MASK                  0x10000UL                                     /**< Bit mask for LESENSE_SETIF */
#define _LESENSE_ST_TCONFB_SETIF_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_SETIF_DEFAULT                (_LESENSE_ST_TCONFB_SETIF_DEFAULT << 16)      /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */

/* Bit fields for LESENSE BUF_DATA */
#define _LESENSE_BUF_DATA_RESETVALUE                   0x00000000UL                          /**< Default value for LESENSE_BUF_DATA */
#define _LESENSE_BUF_DATA_MASK                         0x0000FFFFUL                          /**< Mask for LESENSE_BUF_DATA */
#define _LESENSE_BUF_DATA_DATA_SHIFT                   0                                     /**< Shift value for LESENSE_DATA */
#define _LESENSE_BUF_DATA_DATA_MASK                    0xFFFFUL                              /**< Bit mask for LESENSE_DATA */
#define _LESENSE_BUF_DATA_DATA_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for LESENSE_BUF_DATA */
#define LESENSE_BUF_DATA_DATA_DEFAULT                  (_LESENSE_BUF_DATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_BUF_DATA */

/* Bit fields for LESENSE CH_TIMING */
#define _LESENSE_CH_TIMING_RESETVALUE                  0x00000000UL                                  /**< Default value for LESENSE_CH_TIMING */
#define _LESENSE_CH_TIMING_MASK                        0x000FFFFFUL                                  /**< Mask for LESENSE_CH_TIMING */
#define _LESENSE_CH_TIMING_EXTIME_SHIFT                0                                             /**< Shift value for LESENSE_EXTIME */
#define _LESENSE_CH_TIMING_EXTIME_MASK                 0x3FUL                                        /**< Bit mask for LESENSE_EXTIME */
#define _LESENSE_CH_TIMING_EXTIME_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_CH_TIMING */
#define LESENSE_CH_TIMING_EXTIME_DEFAULT               (_LESENSE_CH_TIMING_EXTIME_DEFAULT << 0)      /**< Shifted mode DEFAULT for LESENSE_CH_TIMING */
#define _LESENSE_CH_TIMING_SAMPLEDLY_SHIFT             6                                             /**< Shift value for LESENSE_SAMPLEDLY */
#define _LESENSE_CH_TIMING_SAMPLEDLY_MASK              0x1FC0UL                                      /**< Bit mask for LESENSE_SAMPLEDLY */
#define _LESENSE_CH_TIMING_SAMPLEDLY_DEFAULT           0x00000000UL                                  /**< Mode DEFAULT for LESENSE_CH_TIMING */
#define LESENSE_CH_TIMING_SAMPLEDLY_DEFAULT            (_LESENSE_CH_TIMING_SAMPLEDLY_DEFAULT << 6)   /**< Shifted mode DEFAULT for LESENSE_CH_TIMING */
#define _LESENSE_CH_TIMING_MEASUREDLY_SHIFT            13                                            /**< Shift value for LESENSE_MEASUREDLY */
#define _LESENSE_CH_TIMING_MEASUREDLY_MASK             0xFE000UL                                     /**< Bit mask for LESENSE_MEASUREDLY */
#define _LESENSE_CH_TIMING_MEASUREDLY_DEFAULT          0x00000000UL                                  /**< Mode DEFAULT for LESENSE_CH_TIMING */
#define LESENSE_CH_TIMING_MEASUREDLY_DEFAULT           (_LESENSE_CH_TIMING_MEASUREDLY_DEFAULT << 13) /**< Shifted mode DEFAULT for LESENSE_CH_TIMING */

/* Bit fields for LESENSE CH_INTERACT */
#define _LESENSE_CH_INTERACT_RESETVALUE                0x00000000UL                                    /**< Default value for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_MASK                      0x000FFFFFUL                                    /**< Mask for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_ACMPTHRES_SHIFT           0                                               /**< Shift value for LESENSE_ACMPTHRES */
#define _LESENSE_CH_INTERACT_ACMPTHRES_MASK            0xFFFUL                                         /**< Bit mask for LESENSE_ACMPTHRES */
#define _LESENSE_CH_INTERACT_ACMPTHRES_DEFAULT         0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_ACMPTHRES_DEFAULT          (_LESENSE_CH_INTERACT_ACMPTHRES_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE                     (0x1UL << 12)                                   /**< Select sample mode */
#define _LESENSE_CH_INTERACT_SAMPLE_SHIFT              12                                              /**< Shift value for LESENSE_SAMPLE */
#define _LESENSE_CH_INTERACT_SAMPLE_MASK               0x1000UL                                        /**< Bit mask for LESENSE_SAMPLE */
#define _LESENSE_CH_INTERACT_SAMPLE_DEFAULT            0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLE_COUNTER            0x00000000UL                                    /**< Mode COUNTER for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLE_ACMP               0x00000001UL                                    /**< Mode ACMP for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE_DEFAULT             (_LESENSE_CH_INTERACT_SAMPLE_DEFAULT << 12)     /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE_COUNTER             (_LESENSE_CH_INTERACT_SAMPLE_COUNTER << 12)     /**< Shifted mode COUNTER for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE_ACMP                (_LESENSE_CH_INTERACT_SAMPLE_ACMP << 12)        /**< Shifted mode ACMP for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_SHIFT               13                                              /**< Shift value for LESENSE_SETIF */
#define _LESENSE_CH_INTERACT_SETIF_MASK                0x6000UL                                        /**< Bit mask for LESENSE_SETIF */
#define _LESENSE_CH_INTERACT_SETIF_DEFAULT             0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_NONE                0x00000000UL                                    /**< Mode NONE for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_LEVEL               0x00000001UL                                    /**< Mode LEVEL for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_POSEDGE             0x00000002UL                                    /**< Mode POSEDGE for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_NEGEDGE             0x00000003UL                                    /**< Mode NEGEDGE for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_DEFAULT              (_LESENSE_CH_INTERACT_SETIF_DEFAULT << 13)      /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_NONE                 (_LESENSE_CH_INTERACT_SETIF_NONE << 13)         /**< Shifted mode NONE for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_LEVEL                (_LESENSE_CH_INTERACT_SETIF_LEVEL << 13)        /**< Shifted mode LEVEL for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_POSEDGE              (_LESENSE_CH_INTERACT_SETIF_POSEDGE << 13)      /**< Shifted mode POSEDGE for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_NEGEDGE              (_LESENSE_CH_INTERACT_SETIF_NEGEDGE << 13)      /**< Shifted mode NEGEDGE for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_SHIFT              15                                              /**< Shift value for LESENSE_EXMODE */
#define _LESENSE_CH_INTERACT_EXMODE_MASK               0x18000UL                                       /**< Bit mask for LESENSE_EXMODE */
#define _LESENSE_CH_INTERACT_EXMODE_DEFAULT            0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_DISABLE            0x00000000UL                                    /**< Mode DISABLE for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_HIGH               0x00000001UL                                    /**< Mode HIGH for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_LOW                0x00000002UL                                    /**< Mode LOW for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_DACOUT             0x00000003UL                                    /**< Mode DACOUT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_DEFAULT             (_LESENSE_CH_INTERACT_EXMODE_DEFAULT << 15)     /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_DISABLE             (_LESENSE_CH_INTERACT_EXMODE_DISABLE << 15)     /**< Shifted mode DISABLE for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_HIGH                (_LESENSE_CH_INTERACT_EXMODE_HIGH << 15)        /**< Shifted mode HIGH for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_LOW                 (_LESENSE_CH_INTERACT_EXMODE_LOW << 15)         /**< Shifted mode LOW for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_DACOUT              (_LESENSE_CH_INTERACT_EXMODE_DACOUT << 15)      /**< Shifted mode DACOUT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXCLK                      (0x1UL << 17)                                   /**< Select clock used for excitation timing */
#define _LESENSE_CH_INTERACT_EXCLK_SHIFT               17                                              /**< Shift value for LESENSE_EXCLK */
#define _LESENSE_CH_INTERACT_EXCLK_MASK                0x20000UL                                       /**< Bit mask for LESENSE_EXCLK */
#define _LESENSE_CH_INTERACT_EXCLK_DEFAULT             0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXCLK_LFACLK              0x00000000UL                                    /**< Mode LFACLK for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXCLK_AUXHFRCO            0x00000001UL                                    /**< Mode AUXHFRCO for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXCLK_DEFAULT              (_LESENSE_CH_INTERACT_EXCLK_DEFAULT << 17)      /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXCLK_LFACLK               (_LESENSE_CH_INTERACT_EXCLK_LFACLK << 17)       /**< Shifted mode LFACLK for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXCLK_AUXHFRCO             (_LESENSE_CH_INTERACT_EXCLK_AUXHFRCO << 17)     /**< Shifted mode AUXHFRCO for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLECLK                  (0x1UL << 18)                                   /**< Select clock used for timing of sample delay */
#define _LESENSE_CH_INTERACT_SAMPLECLK_SHIFT           18                                              /**< Shift value for LESENSE_SAMPLECLK */
#define _LESENSE_CH_INTERACT_SAMPLECLK_MASK            0x40000UL                                       /**< Bit mask for LESENSE_SAMPLECLK */
#define _LESENSE_CH_INTERACT_SAMPLECLK_DEFAULT         0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLECLK_LFACLK          0x00000000UL                                    /**< Mode LFACLK for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLECLK_AUXHFRCO        0x00000001UL                                    /**< Mode AUXHFRCO for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLECLK_DEFAULT          (_LESENSE_CH_INTERACT_SAMPLECLK_DEFAULT << 18)  /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLECLK_LFACLK           (_LESENSE_CH_INTERACT_SAMPLECLK_LFACLK << 18)   /**< Shifted mode LFACLK for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLECLK_AUXHFRCO         (_LESENSE_CH_INTERACT_SAMPLECLK_AUXHFRCO << 18) /**< Shifted mode AUXHFRCO for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_ALTEX                      (0x1UL << 19)                                   /**< Use alternative excite pin */
#define _LESENSE_CH_INTERACT_ALTEX_SHIFT               19                                              /**< Shift value for LESENSE_ALTEX */
#define _LESENSE_CH_INTERACT_ALTEX_MASK                0x80000UL                                       /**< Bit mask for LESENSE_ALTEX */
#define _LESENSE_CH_INTERACT_ALTEX_DEFAULT             0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_ALTEX_DEFAULT              (_LESENSE_CH_INTERACT_ALTEX_DEFAULT << 19)      /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */

/* Bit fields for LESENSE CH_EVAL */
#define _LESENSE_CH_EVAL_RESETVALUE                    0x00000000UL                                /**< Default value for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_MASK                          0x000FFFFFUL                                /**< Mask for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_COMPTHRES_SHIFT               0                                           /**< Shift value for LESENSE_COMPTHRES */
#define _LESENSE_CH_EVAL_COMPTHRES_MASK                0xFFFFUL                                    /**< Bit mask for LESENSE_COMPTHRES */
#define _LESENSE_CH_EVAL_COMPTHRES_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMPTHRES_DEFAULT              (_LESENSE_CH_EVAL_COMPTHRES_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMP                           (0x1UL << 16)                               /**< Select mode for counter comparison */
#define _LESENSE_CH_EVAL_COMP_SHIFT                    16                                          /**< Shift value for LESENSE_COMP */
#define _LESENSE_CH_EVAL_COMP_MASK                     0x10000UL                                   /**< Bit mask for LESENSE_COMP */
#define _LESENSE_CH_EVAL_COMP_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_COMP_LESS                     0x00000000UL                                /**< Mode LESS for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_COMP_GE                       0x00000001UL                                /**< Mode GE for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMP_DEFAULT                   (_LESENSE_CH_EVAL_COMP_DEFAULT << 16)       /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMP_LESS                      (_LESENSE_CH_EVAL_COMP_LESS << 16)          /**< Shifted mode LESS for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMP_GE                        (_LESENSE_CH_EVAL_COMP_GE << 16)            /**< Shifted mode GE for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_DECODE                         (0x1UL << 17)                               /**< Send result to decoder */
#define _LESENSE_CH_EVAL_DECODE_SHIFT                  17                                          /**< Shift value for LESENSE_DECODE */
#define _LESENSE_CH_EVAL_DECODE_MASK                   0x20000UL                                   /**< Bit mask for LESENSE_DECODE */
#define _LESENSE_CH_EVAL_DECODE_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_DECODE_DEFAULT                 (_LESENSE_CH_EVAL_DECODE_DEFAULT << 17)     /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_STRSAMPLE                      (0x1UL << 18)                               /**< Select if counter result should be stored */
#define _LESENSE_CH_EVAL_STRSAMPLE_SHIFT               18                                          /**< Shift value for LESENSE_STRSAMPLE */
#define _LESENSE_CH_EVAL_STRSAMPLE_MASK                0x40000UL                                   /**< Bit mask for LESENSE_STRSAMPLE */
#define _LESENSE_CH_EVAL_STRSAMPLE_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_STRSAMPLE_DEFAULT              (_LESENSE_CH_EVAL_STRSAMPLE_DEFAULT << 18)  /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_SCANRESINV                     (0x1UL << 19)                               /**< Enable inversion of result */
#define _LESENSE_CH_EVAL_SCANRESINV_SHIFT              19                                          /**< Shift value for LESENSE_SCANRESINV */
#define _LESENSE_CH_EVAL_SCANRESINV_MASK               0x80000UL                                   /**< Bit mask for LESENSE_SCANRESINV */
#define _LESENSE_CH_EVAL_SCANRESINV_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_SCANRESINV_DEFAULT             (_LESENSE_CH_EVAL_SCANRESINV_DEFAULT << 19) /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */

/** @} End of group EFM32WG_LESENSE */
/** @} End of group Parts */

