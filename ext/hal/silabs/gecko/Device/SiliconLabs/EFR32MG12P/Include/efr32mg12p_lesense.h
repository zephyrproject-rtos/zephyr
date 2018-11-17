/**************************************************************************//**
 * @file efr32mg12p_lesense.h
 * @brief EFR32MG12P_LESENSE register and bit field definitions
 * @version 5.6.0
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
 * @defgroup EFR32MG12P_LESENSE LESENSE
 * @{
 * @brief EFR32MG12P_LESENSE Register Declaration
 *****************************************************************************/
/** LESENSE Register Declaration */
typedef struct {
  __IOM uint32_t      CTRL;           /**< Control Register  */
  __IOM uint32_t      TIMCTRL;        /**< Timing Control Register  */
  __IOM uint32_t      PERCTRL;        /**< Peripheral Control Register  */
  __IOM uint32_t      DECCTRL;        /**< Decoder Control Register  */
  __IOM uint32_t      BIASCTRL;       /**< Bias Control Register  */
  __IOM uint32_t      EVALCTRL;       /**< LESENSE Evaluation Control  */
  __IOM uint32_t      PRSCTRL;        /**< PRS Control Register  */
  __IOM uint32_t      CMD;            /**< Command Register  */
  __IOM uint32_t      CHEN;           /**< Channel Enable Register  */
  __IOM uint32_t      SCANRES;        /**< Scan Result Register  */
  __IM uint32_t       STATUS;         /**< Status Register  */
  __IM uint32_t       PTR;            /**< Result Buffer Pointers  */
  __IM uint32_t       BUFDATA;        /**< Result Buffer Data Register  */
  __IM uint32_t       CURCH;          /**< Current Channel Index  */
  __IOM uint32_t      DECSTATE;       /**< Current Decoder State  */
  __IOM uint32_t      SENSORSTATE;    /**< Decoder Input Register  */
  __IOM uint32_t      IDLECONF;       /**< GPIO Idle Phase Configuration  */
  __IOM uint32_t      ALTEXCONF;      /**< Alternative Excite Pin Configuration  */
  uint32_t            RESERVED0[2U];  /**< Reserved for future use **/
  __IM uint32_t       IF;             /**< Interrupt Flag Register  */
  __IOM uint32_t      IFS;            /**< Interrupt Flag Set Register  */
  __IOM uint32_t      IFC;            /**< Interrupt Flag Clear Register  */
  __IOM uint32_t      IEN;            /**< Interrupt Enable Register  */
  __IM uint32_t       SYNCBUSY;       /**< Synchronization Busy Register  */
  __IOM uint32_t      ROUTEPEN;       /**< I/O Routing Register  */

  uint32_t            RESERVED1[38U]; /**< Reserved registers */
  LESENSE_ST_TypeDef  ST[32U];        /**< Decoding states */

  LESENSE_BUF_TypeDef BUF[16U];       /**< Scanresult */

  LESENSE_CH_TypeDef  CH[16U];        /**< Scanconfig */
} LESENSE_TypeDef;                    /** @} */

/**************************************************************************//**
 * @addtogroup EFR32MG12P_LESENSE
 * @{
 * @defgroup EFR32MG12P_LESENSE_BitFields  LESENSE Bit Fields
 * @{
 *****************************************************************************/

/* Bit fields for LESENSE CTRL */
#define _LESENSE_CTRL_RESETVALUE                       0x00000000UL                             /**< Default value for LESENSE_CTRL */
#define _LESENSE_CTRL_MASK                             0x007B29BFUL                             /**< Mask for LESENSE_CTRL */
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
#define _LESENSE_CTRL_SCANCONF_SHIFT                   7                                        /**< Shift value for LESENSE_SCANCONF */
#define _LESENSE_CTRL_SCANCONF_MASK                    0x180UL                                  /**< Bit mask for LESENSE_SCANCONF */
#define _LESENSE_CTRL_SCANCONF_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_DIRMAP                  0x00000000UL                             /**< Mode DIRMAP for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_INVMAP                  0x00000001UL                             /**< Mode INVMAP for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_TOGGLE                  0x00000002UL                             /**< Mode TOGGLE for LESENSE_CTRL */
#define _LESENSE_CTRL_SCANCONF_DECDEF                  0x00000003UL                             /**< Mode DECDEF for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_DEFAULT                  (_LESENSE_CTRL_SCANCONF_DEFAULT << 7)    /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_DIRMAP                   (_LESENSE_CTRL_SCANCONF_DIRMAP << 7)     /**< Shifted mode DIRMAP for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_INVMAP                   (_LESENSE_CTRL_SCANCONF_INVMAP << 7)     /**< Shifted mode INVMAP for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_TOGGLE                   (_LESENSE_CTRL_SCANCONF_TOGGLE << 7)     /**< Shifted mode TOGGLE for LESENSE_CTRL */
#define LESENSE_CTRL_SCANCONF_DECDEF                   (_LESENSE_CTRL_SCANCONF_DECDEF << 7)     /**< Shifted mode DECDEF for LESENSE_CTRL */
#define LESENSE_CTRL_ALTEXMAP                          (0x1UL << 11)                            /**< Alternative Excitation Map */
#define _LESENSE_CTRL_ALTEXMAP_SHIFT                   11                                       /**< Shift value for LESENSE_ALTEXMAP */
#define _LESENSE_CTRL_ALTEXMAP_MASK                    0x800UL                                  /**< Bit mask for LESENSE_ALTEXMAP */
#define _LESENSE_CTRL_ALTEXMAP_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_ALTEXMAP_ALTEX                   0x00000000UL                             /**< Mode ALTEX for LESENSE_CTRL */
#define _LESENSE_CTRL_ALTEXMAP_CH                      0x00000001UL                             /**< Mode CH for LESENSE_CTRL */
#define LESENSE_CTRL_ALTEXMAP_DEFAULT                  (_LESENSE_CTRL_ALTEXMAP_DEFAULT << 11)   /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_ALTEXMAP_ALTEX                    (_LESENSE_CTRL_ALTEXMAP_ALTEX << 11)     /**< Shifted mode ALTEX for LESENSE_CTRL */
#define LESENSE_CTRL_ALTEXMAP_CH                       (_LESENSE_CTRL_ALTEXMAP_CH << 11)        /**< Shifted mode CH for LESENSE_CTRL */
#define LESENSE_CTRL_DUALSAMPLE                        (0x1UL << 13)                            /**< Enable Dual Sample Mode */
#define _LESENSE_CTRL_DUALSAMPLE_SHIFT                 13                                       /**< Shift value for LESENSE_DUALSAMPLE */
#define _LESENSE_CTRL_DUALSAMPLE_MASK                  0x2000UL                                 /**< Bit mask for LESENSE_DUALSAMPLE */
#define _LESENSE_CTRL_DUALSAMPLE_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_DUALSAMPLE_DEFAULT                (_LESENSE_CTRL_DUALSAMPLE_DEFAULT << 13) /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_BUFOW                             (0x1UL << 16)                            /**< Result Buffer Overwrite */
#define _LESENSE_CTRL_BUFOW_SHIFT                      16                                       /**< Shift value for LESENSE_BUFOW */
#define _LESENSE_CTRL_BUFOW_MASK                       0x10000UL                                /**< Bit mask for LESENSE_BUFOW */
#define _LESENSE_CTRL_BUFOW_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_BUFOW_DEFAULT                     (_LESENSE_CTRL_BUFOW_DEFAULT << 16)      /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_STRSCANRES                        (0x1UL << 17)                            /**< Enable Storing of SCANRES */
#define _LESENSE_CTRL_STRSCANRES_SHIFT                 17                                       /**< Shift value for LESENSE_STRSCANRES */
#define _LESENSE_CTRL_STRSCANRES_MASK                  0x20000UL                                /**< Bit mask for LESENSE_STRSCANRES */
#define _LESENSE_CTRL_STRSCANRES_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_STRSCANRES_DEFAULT                (_LESENSE_CTRL_STRSCANRES_DEFAULT << 17) /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_BUFIDL                            (0x1UL << 19)                            /**< Result Buffer Interrupt and DMA Trigger Level */
#define _LESENSE_CTRL_BUFIDL_SHIFT                     19                                       /**< Shift value for LESENSE_BUFIDL */
#define _LESENSE_CTRL_BUFIDL_MASK                      0x80000UL                                /**< Bit mask for LESENSE_BUFIDL */
#define _LESENSE_CTRL_BUFIDL_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for LESENSE_CTRL */
#define _LESENSE_CTRL_BUFIDL_HALFFULL                  0x00000000UL                             /**< Mode HALFFULL for LESENSE_CTRL */
#define _LESENSE_CTRL_BUFIDL_FULL                      0x00000001UL                             /**< Mode FULL for LESENSE_CTRL */
#define LESENSE_CTRL_BUFIDL_DEFAULT                    (_LESENSE_CTRL_BUFIDL_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_CTRL */
#define LESENSE_CTRL_BUFIDL_HALFFULL                   (_LESENSE_CTRL_BUFIDL_HALFFULL << 19)    /**< Shifted mode HALFFULL for LESENSE_CTRL */
#define LESENSE_CTRL_BUFIDL_FULL                       (_LESENSE_CTRL_BUFIDL_FULL << 19)        /**< Shifted mode FULL for LESENSE_CTRL */
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
#define _LESENSE_TIMCTRL_RESETVALUE                    0x00000000UL                                  /**< Default value for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_MASK                          0x10CFF773UL                                  /**< Mask for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_SHIFT                0                                             /**< Shift value for LESENSE_AUXPRESC */
#define _LESENSE_TIMCTRL_AUXPRESC_MASK                 0x3UL                                         /**< Bit mask for LESENSE_AUXPRESC */
#define _LESENSE_TIMCTRL_AUXPRESC_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_DIV1                 0x00000000UL                                  /**< Mode DIV1 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_DIV2                 0x00000001UL                                  /**< Mode DIV2 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_DIV4                 0x00000002UL                                  /**< Mode DIV4 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXPRESC_DIV8                 0x00000003UL                                  /**< Mode DIV8 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DEFAULT               (_LESENSE_TIMCTRL_AUXPRESC_DEFAULT << 0)      /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DIV1                  (_LESENSE_TIMCTRL_AUXPRESC_DIV1 << 0)         /**< Shifted mode DIV1 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DIV2                  (_LESENSE_TIMCTRL_AUXPRESC_DIV2 << 0)         /**< Shifted mode DIV2 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DIV4                  (_LESENSE_TIMCTRL_AUXPRESC_DIV4 << 0)         /**< Shifted mode DIV4 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXPRESC_DIV8                  (_LESENSE_TIMCTRL_AUXPRESC_DIV8 << 0)         /**< Shifted mode DIV8 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_SHIFT                 4                                             /**< Shift value for LESENSE_LFPRESC */
#define _LESENSE_TIMCTRL_LFPRESC_MASK                  0x70UL                                        /**< Bit mask for LESENSE_LFPRESC */
#define _LESENSE_TIMCTRL_LFPRESC_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV1                  0x00000000UL                                  /**< Mode DIV1 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV2                  0x00000001UL                                  /**< Mode DIV2 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV4                  0x00000002UL                                  /**< Mode DIV4 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV8                  0x00000003UL                                  /**< Mode DIV8 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV16                 0x00000004UL                                  /**< Mode DIV16 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV32                 0x00000005UL                                  /**< Mode DIV32 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV64                 0x00000006UL                                  /**< Mode DIV64 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_LFPRESC_DIV128                0x00000007UL                                  /**< Mode DIV128 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DEFAULT                (_LESENSE_TIMCTRL_LFPRESC_DEFAULT << 4)       /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV1                   (_LESENSE_TIMCTRL_LFPRESC_DIV1 << 4)          /**< Shifted mode DIV1 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV2                   (_LESENSE_TIMCTRL_LFPRESC_DIV2 << 4)          /**< Shifted mode DIV2 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV4                   (_LESENSE_TIMCTRL_LFPRESC_DIV4 << 4)          /**< Shifted mode DIV4 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV8                   (_LESENSE_TIMCTRL_LFPRESC_DIV8 << 4)          /**< Shifted mode DIV8 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV16                  (_LESENSE_TIMCTRL_LFPRESC_DIV16 << 4)         /**< Shifted mode DIV16 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV32                  (_LESENSE_TIMCTRL_LFPRESC_DIV32 << 4)         /**< Shifted mode DIV32 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV64                  (_LESENSE_TIMCTRL_LFPRESC_DIV64 << 4)         /**< Shifted mode DIV64 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_LFPRESC_DIV128                 (_LESENSE_TIMCTRL_LFPRESC_DIV128 << 4)        /**< Shifted mode DIV128 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_SHIFT                 8                                             /**< Shift value for LESENSE_PCPRESC */
#define _LESENSE_TIMCTRL_PCPRESC_MASK                  0x700UL                                       /**< Bit mask for LESENSE_PCPRESC */
#define _LESENSE_TIMCTRL_PCPRESC_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV1                  0x00000000UL                                  /**< Mode DIV1 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV2                  0x00000001UL                                  /**< Mode DIV2 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV4                  0x00000002UL                                  /**< Mode DIV4 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV8                  0x00000003UL                                  /**< Mode DIV8 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV16                 0x00000004UL                                  /**< Mode DIV16 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV32                 0x00000005UL                                  /**< Mode DIV32 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV64                 0x00000006UL                                  /**< Mode DIV64 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCPRESC_DIV128                0x00000007UL                                  /**< Mode DIV128 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DEFAULT                (_LESENSE_TIMCTRL_PCPRESC_DEFAULT << 8)       /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV1                   (_LESENSE_TIMCTRL_PCPRESC_DIV1 << 8)          /**< Shifted mode DIV1 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV2                   (_LESENSE_TIMCTRL_PCPRESC_DIV2 << 8)          /**< Shifted mode DIV2 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV4                   (_LESENSE_TIMCTRL_PCPRESC_DIV4 << 8)          /**< Shifted mode DIV4 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV8                   (_LESENSE_TIMCTRL_PCPRESC_DIV8 << 8)          /**< Shifted mode DIV8 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV16                  (_LESENSE_TIMCTRL_PCPRESC_DIV16 << 8)         /**< Shifted mode DIV16 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV32                  (_LESENSE_TIMCTRL_PCPRESC_DIV32 << 8)         /**< Shifted mode DIV32 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV64                  (_LESENSE_TIMCTRL_PCPRESC_DIV64 << 8)         /**< Shifted mode DIV64 for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCPRESC_DIV128                 (_LESENSE_TIMCTRL_PCPRESC_DIV128 << 8)        /**< Shifted mode DIV128 for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_PCTOP_SHIFT                   12                                            /**< Shift value for LESENSE_PCTOP */
#define _LESENSE_TIMCTRL_PCTOP_MASK                    0xFF000UL                                     /**< Bit mask for LESENSE_PCTOP */
#define _LESENSE_TIMCTRL_PCTOP_DEFAULT                 0x00000000UL                                  /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_PCTOP_DEFAULT                  (_LESENSE_TIMCTRL_PCTOP_DEFAULT << 12)        /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_STARTDLY_SHIFT                22                                            /**< Shift value for LESENSE_STARTDLY */
#define _LESENSE_TIMCTRL_STARTDLY_MASK                 0xC00000UL                                    /**< Bit mask for LESENSE_STARTDLY */
#define _LESENSE_TIMCTRL_STARTDLY_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_STARTDLY_DEFAULT               (_LESENSE_TIMCTRL_STARTDLY_DEFAULT << 22)     /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXSTARTUP                     (0x1UL << 28)                                 /**< AUXHFRCO Startup Configuration */
#define _LESENSE_TIMCTRL_AUXSTARTUP_SHIFT              28                                            /**< Shift value for LESENSE_AUXSTARTUP */
#define _LESENSE_TIMCTRL_AUXSTARTUP_MASK               0x10000000UL                                  /**< Bit mask for LESENSE_AUXSTARTUP */
#define _LESENSE_TIMCTRL_AUXSTARTUP_DEFAULT            0x00000000UL                                  /**< Mode DEFAULT for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXSTARTUP_PREDEMAND          0x00000000UL                                  /**< Mode PREDEMAND for LESENSE_TIMCTRL */
#define _LESENSE_TIMCTRL_AUXSTARTUP_ONDEMAND           0x00000001UL                                  /**< Mode ONDEMAND for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXSTARTUP_DEFAULT             (_LESENSE_TIMCTRL_AUXSTARTUP_DEFAULT << 28)   /**< Shifted mode DEFAULT for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXSTARTUP_PREDEMAND           (_LESENSE_TIMCTRL_AUXSTARTUP_PREDEMAND << 28) /**< Shifted mode PREDEMAND for LESENSE_TIMCTRL */
#define LESENSE_TIMCTRL_AUXSTARTUP_ONDEMAND            (_LESENSE_TIMCTRL_AUXSTARTUP_ONDEMAND << 28)  /**< Shifted mode ONDEMAND for LESENSE_TIMCTRL */

/* Bit fields for LESENSE PERCTRL */
#define _LESENSE_PERCTRL_RESETVALUE                    0x00000000UL                                        /**< Default value for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_MASK                          0x3FF0014FUL                                        /**< Mask for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0EN                       (0x1UL << 0)                                        /**< VDAC CH0 Enable */
#define _LESENSE_PERCTRL_DACCH0EN_SHIFT                0                                                   /**< Shift value for LESENSE_DACCH0EN */
#define _LESENSE_PERCTRL_DACCH0EN_MASK                 0x1UL                                               /**< Bit mask for LESENSE_DACCH0EN */
#define _LESENSE_PERCTRL_DACCH0EN_DEFAULT              0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0EN_DEFAULT               (_LESENSE_PERCTRL_DACCH0EN_DEFAULT << 0)            /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1EN                       (0x1UL << 1)                                        /**< VDAC CH1 Enable */
#define _LESENSE_PERCTRL_DACCH1EN_SHIFT                1                                                   /**< Shift value for LESENSE_DACCH1EN */
#define _LESENSE_PERCTRL_DACCH1EN_MASK                 0x2UL                                               /**< Bit mask for LESENSE_DACCH1EN */
#define _LESENSE_PERCTRL_DACCH1EN_DEFAULT              0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1EN_DEFAULT               (_LESENSE_PERCTRL_DACCH1EN_DEFAULT << 1)            /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0DATA                     (0x1UL << 2)                                        /**< VDAC CH0 Data Selection */
#define _LESENSE_PERCTRL_DACCH0DATA_SHIFT              2                                                   /**< Shift value for LESENSE_DACCH0DATA */
#define _LESENSE_PERCTRL_DACCH0DATA_MASK               0x4UL                                               /**< Bit mask for LESENSE_DACCH0DATA */
#define _LESENSE_PERCTRL_DACCH0DATA_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0DATA_DACDATA            0x00000000UL                                        /**< Mode DACDATA for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH0DATA_THRES              0x00000001UL                                        /**< Mode THRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0DATA_DEFAULT             (_LESENSE_PERCTRL_DACCH0DATA_DEFAULT << 2)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0DATA_DACDATA             (_LESENSE_PERCTRL_DACCH0DATA_DACDATA << 2)          /**< Shifted mode DACDATA for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH0DATA_THRES               (_LESENSE_PERCTRL_DACCH0DATA_THRES << 2)            /**< Shifted mode THRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1DATA                     (0x1UL << 3)                                        /**< VDAC CH1 Data Selection */
#define _LESENSE_PERCTRL_DACCH1DATA_SHIFT              3                                                   /**< Shift value for LESENSE_DACCH1DATA */
#define _LESENSE_PERCTRL_DACCH1DATA_MASK               0x8UL                                               /**< Bit mask for LESENSE_DACCH1DATA */
#define _LESENSE_PERCTRL_DACCH1DATA_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1DATA_DACDATA            0x00000000UL                                        /**< Mode DACDATA for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCH1DATA_THRES              0x00000001UL                                        /**< Mode THRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1DATA_DEFAULT             (_LESENSE_PERCTRL_DACCH1DATA_DEFAULT << 3)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1DATA_DACDATA             (_LESENSE_PERCTRL_DACCH1DATA_DACDATA << 3)          /**< Shifted mode DACDATA for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCH1DATA_THRES               (_LESENSE_PERCTRL_DACCH1DATA_THRES << 3)            /**< Shifted mode THRES for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACSTARTUP                     (0x1UL << 6)                                        /**< VDAC Startup Configuration */
#define _LESENSE_PERCTRL_DACSTARTUP_SHIFT              6                                                   /**< Shift value for LESENSE_DACSTARTUP */
#define _LESENSE_PERCTRL_DACSTARTUP_MASK               0x40UL                                              /**< Bit mask for LESENSE_DACSTARTUP */
#define _LESENSE_PERCTRL_DACSTARTUP_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACSTARTUP_FULLCYCLE          0x00000000UL                                        /**< Mode FULLCYCLE for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACSTARTUP_HALFCYCLE          0x00000001UL                                        /**< Mode HALFCYCLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACSTARTUP_DEFAULT             (_LESENSE_PERCTRL_DACSTARTUP_DEFAULT << 6)          /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACSTARTUP_FULLCYCLE           (_LESENSE_PERCTRL_DACSTARTUP_FULLCYCLE << 6)        /**< Shifted mode FULLCYCLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACSTARTUP_HALFCYCLE           (_LESENSE_PERCTRL_DACSTARTUP_HALFCYCLE << 6)        /**< Shifted mode HALFCYCLE for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCONVTRIG                    (0x1UL << 8)                                        /**< VDAC Conversion Trigger Configuration */
#define _LESENSE_PERCTRL_DACCONVTRIG_SHIFT             8                                                   /**< Shift value for LESENSE_DACCONVTRIG */
#define _LESENSE_PERCTRL_DACCONVTRIG_MASK              0x100UL                                             /**< Bit mask for LESENSE_DACCONVTRIG */
#define _LESENSE_PERCTRL_DACCONVTRIG_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCONVTRIG_CHANNELSTART      0x00000000UL                                        /**< Mode CHANNELSTART for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_DACCONVTRIG_SCANSTART         0x00000001UL                                        /**< Mode SCANSTART for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCONVTRIG_DEFAULT            (_LESENSE_PERCTRL_DACCONVTRIG_DEFAULT << 8)         /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCONVTRIG_CHANNELSTART       (_LESENSE_PERCTRL_DACCONVTRIG_CHANNELSTART << 8)    /**< Shifted mode CHANNELSTART for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_DACCONVTRIG_SCANSTART          (_LESENSE_PERCTRL_DACCONVTRIG_SCANSTART << 8)       /**< Shifted mode SCANSTART for LESENSE_PERCTRL */
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
#define LESENSE_PERCTRL_ACMP0INV                       (0x1UL << 24)                                       /**< Invert Analog Comparator 0 Output */
#define _LESENSE_PERCTRL_ACMP0INV_SHIFT                24                                                  /**< Shift value for LESENSE_ACMP0INV */
#define _LESENSE_PERCTRL_ACMP0INV_MASK                 0x1000000UL                                         /**< Bit mask for LESENSE_ACMP0INV */
#define _LESENSE_PERCTRL_ACMP0INV_DEFAULT              0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP0INV_DEFAULT               (_LESENSE_PERCTRL_ACMP0INV_DEFAULT << 24)           /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP1INV                       (0x1UL << 25)                                       /**< Invert Analog Comparator 1 Output */
#define _LESENSE_PERCTRL_ACMP1INV_SHIFT                25                                                  /**< Shift value for LESENSE_ACMP1INV */
#define _LESENSE_PERCTRL_ACMP1INV_MASK                 0x2000000UL                                         /**< Bit mask for LESENSE_ACMP1INV */
#define _LESENSE_PERCTRL_ACMP1INV_DEFAULT              0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP1INV_DEFAULT               (_LESENSE_PERCTRL_ACMP1INV_DEFAULT << 25)           /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP0HYSTEN                    (0x1UL << 26)                                       /**< ACMP0 Hysteresis Enable */
#define _LESENSE_PERCTRL_ACMP0HYSTEN_SHIFT             26                                                  /**< Shift value for LESENSE_ACMP0HYSTEN */
#define _LESENSE_PERCTRL_ACMP0HYSTEN_MASK              0x4000000UL                                         /**< Bit mask for LESENSE_ACMP0HYSTEN */
#define _LESENSE_PERCTRL_ACMP0HYSTEN_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP0HYSTEN_DEFAULT            (_LESENSE_PERCTRL_ACMP0HYSTEN_DEFAULT << 26)        /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP1HYSTEN                    (0x1UL << 27)                                       /**< ACMP1 Hysteresis Enable */
#define _LESENSE_PERCTRL_ACMP1HYSTEN_SHIFT             27                                                  /**< Shift value for LESENSE_ACMP1HYSTEN */
#define _LESENSE_PERCTRL_ACMP1HYSTEN_MASK              0x8000000UL                                         /**< Bit mask for LESENSE_ACMP1HYSTEN */
#define _LESENSE_PERCTRL_ACMP1HYSTEN_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_ACMP1HYSTEN_DEFAULT            (_LESENSE_PERCTRL_ACMP1HYSTEN_DEFAULT << 27)        /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_SHIFT              28                                                  /**< Shift value for LESENSE_WARMUPMODE */
#define _LESENSE_PERCTRL_WARMUPMODE_MASK               0x30000000UL                                        /**< Bit mask for LESENSE_WARMUPMODE */
#define _LESENSE_PERCTRL_WARMUPMODE_DEFAULT            0x00000000UL                                        /**< Mode DEFAULT for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_NORMAL             0x00000000UL                                        /**< Mode NORMAL for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_KEEPACMPWARM       0x00000001UL                                        /**< Mode KEEPACMPWARM for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_KEEPDACWARM        0x00000002UL                                        /**< Mode KEEPDACWARM for LESENSE_PERCTRL */
#define _LESENSE_PERCTRL_WARMUPMODE_KEEPACMPDACWARM    0x00000003UL                                        /**< Mode KEEPACMPDACWARM for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_DEFAULT             (_LESENSE_PERCTRL_WARMUPMODE_DEFAULT << 28)         /**< Shifted mode DEFAULT for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_NORMAL              (_LESENSE_PERCTRL_WARMUPMODE_NORMAL << 28)          /**< Shifted mode NORMAL for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_KEEPACMPWARM        (_LESENSE_PERCTRL_WARMUPMODE_KEEPACMPWARM << 28)    /**< Shifted mode KEEPACMPWARM for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_KEEPDACWARM         (_LESENSE_PERCTRL_WARMUPMODE_KEEPDACWARM << 28)     /**< Shifted mode KEEPDACWARM for LESENSE_PERCTRL */
#define LESENSE_PERCTRL_WARMUPMODE_KEEPACMPDACWARM     (_LESENSE_PERCTRL_WARMUPMODE_KEEPACMPDACWARM << 28) /**< Shifted mode KEEPACMPDACWARM for LESENSE_PERCTRL */

/* Bit fields for LESENSE DECCTRL */
#define _LESENSE_DECCTRL_RESETVALUE                    0x00000000UL                              /**< Default value for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_MASK                          0x1EF7BDFFUL                              /**< Mask for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_DISABLE                        (0x1UL << 0)                              /**< Disable the Decoder */
#define _LESENSE_DECCTRL_DISABLE_SHIFT                 0                                         /**< Shift value for LESENSE_DISABLE */
#define _LESENSE_DECCTRL_DISABLE_MASK                  0x1UL                                     /**< Bit mask for LESENSE_DISABLE */
#define _LESENSE_DECCTRL_DISABLE_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_DISABLE_DEFAULT                (_LESENSE_DECCTRL_DISABLE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_ERRCHK                         (0x1UL << 1)                              /**< Enable Check of Current State */
#define _LESENSE_DECCTRL_ERRCHK_SHIFT                  1                                         /**< Shift value for LESENSE_ERRCHK */
#define _LESENSE_DECCTRL_ERRCHK_MASK                   0x2UL                                     /**< Bit mask for LESENSE_ERRCHK */
#define _LESENSE_DECCTRL_ERRCHK_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_ERRCHK_DEFAULT                 (_LESENSE_DECCTRL_ERRCHK_DEFAULT << 1)    /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INTMAP                         (0x1UL << 2)                              /**< Enable Decoder to Channel Interrupt Mapping */
#define _LESENSE_DECCTRL_INTMAP_SHIFT                  2                                         /**< Shift value for LESENSE_INTMAP */
#define _LESENSE_DECCTRL_INTMAP_MASK                   0x4UL                                     /**< Bit mask for LESENSE_INTMAP */
#define _LESENSE_DECCTRL_INTMAP_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INTMAP_DEFAULT                 (_LESENSE_DECCTRL_INTMAP_DEFAULT << 2)    /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS0                       (0x1UL << 3)                              /**< Enable Decoder Hysteresis on PRS0 Output */
#define _LESENSE_DECCTRL_HYSTPRS0_SHIFT                3                                         /**< Shift value for LESENSE_HYSTPRS0 */
#define _LESENSE_DECCTRL_HYSTPRS0_MASK                 0x8UL                                     /**< Bit mask for LESENSE_HYSTPRS0 */
#define _LESENSE_DECCTRL_HYSTPRS0_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS0_DEFAULT               (_LESENSE_DECCTRL_HYSTPRS0_DEFAULT << 3)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS1                       (0x1UL << 4)                              /**< Enable Decoder Hysteresis on PRS1 Output */
#define _LESENSE_DECCTRL_HYSTPRS1_SHIFT                4                                         /**< Shift value for LESENSE_HYSTPRS1 */
#define _LESENSE_DECCTRL_HYSTPRS1_MASK                 0x10UL                                    /**< Bit mask for LESENSE_HYSTPRS1 */
#define _LESENSE_DECCTRL_HYSTPRS1_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS1_DEFAULT               (_LESENSE_DECCTRL_HYSTPRS1_DEFAULT << 4)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS2                       (0x1UL << 5)                              /**< Enable Decoder Hysteresis on PRS2 Output */
#define _LESENSE_DECCTRL_HYSTPRS2_SHIFT                5                                         /**< Shift value for LESENSE_HYSTPRS2 */
#define _LESENSE_DECCTRL_HYSTPRS2_MASK                 0x20UL                                    /**< Bit mask for LESENSE_HYSTPRS2 */
#define _LESENSE_DECCTRL_HYSTPRS2_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTPRS2_DEFAULT               (_LESENSE_DECCTRL_HYSTPRS2_DEFAULT << 5)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTIRQ                        (0x1UL << 6)                              /**< Enable Decoder Hysteresis on Interrupt Requests */
#define _LESENSE_DECCTRL_HYSTIRQ_SHIFT                 6                                         /**< Shift value for LESENSE_HYSTIRQ */
#define _LESENSE_DECCTRL_HYSTIRQ_MASK                  0x40UL                                    /**< Bit mask for LESENSE_HYSTIRQ */
#define _LESENSE_DECCTRL_HYSTIRQ_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_HYSTIRQ_DEFAULT                (_LESENSE_DECCTRL_HYSTIRQ_DEFAULT << 6)   /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSCNT                         (0x1UL << 7)                              /**< Enable Count Mode on Decoder PRS Channels 0 and 1 */
#define _LESENSE_DECCTRL_PRSCNT_SHIFT                  7                                         /**< Shift value for LESENSE_PRSCNT */
#define _LESENSE_DECCTRL_PRSCNT_MASK                   0x80UL                                    /**< Bit mask for LESENSE_PRSCNT */
#define _LESENSE_DECCTRL_PRSCNT_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSCNT_DEFAULT                 (_LESENSE_DECCTRL_PRSCNT_DEFAULT << 7)    /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_INPUT                          (0x1UL << 8)                              /**< LESENSE Decoder Input Configuration */
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
#define _LESENSE_DECCTRL_PRSSEL1_SHIFT                 15                                        /**< Shift value for LESENSE_PRSSEL1 */
#define _LESENSE_DECCTRL_PRSSEL1_MASK                  0x78000UL                                 /**< Bit mask for LESENSE_PRSSEL1 */
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
#define LESENSE_DECCTRL_PRSSEL1_DEFAULT                (_LESENSE_DECCTRL_PRSSEL1_DEFAULT << 15)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH0                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH0 << 15)   /**< Shifted mode PRSCH0 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH1                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH1 << 15)   /**< Shifted mode PRSCH1 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH2                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH2 << 15)   /**< Shifted mode PRSCH2 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH3                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH3 << 15)   /**< Shifted mode PRSCH3 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH4                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH4 << 15)   /**< Shifted mode PRSCH4 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH5                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH5 << 15)   /**< Shifted mode PRSCH5 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH6                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH6 << 15)   /**< Shifted mode PRSCH6 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH7                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH7 << 15)   /**< Shifted mode PRSCH7 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH8                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH8 << 15)   /**< Shifted mode PRSCH8 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH9                 (_LESENSE_DECCTRL_PRSSEL1_PRSCH9 << 15)   /**< Shifted mode PRSCH9 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH10                (_LESENSE_DECCTRL_PRSSEL1_PRSCH10 << 15)  /**< Shifted mode PRSCH10 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL1_PRSCH11                (_LESENSE_DECCTRL_PRSSEL1_PRSCH11 << 15)  /**< Shifted mode PRSCH11 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL2_SHIFT                 20                                        /**< Shift value for LESENSE_PRSSEL2 */
#define _LESENSE_DECCTRL_PRSSEL2_MASK                  0xF00000UL                                /**< Bit mask for LESENSE_PRSSEL2 */
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
#define LESENSE_DECCTRL_PRSSEL2_DEFAULT                (_LESENSE_DECCTRL_PRSSEL2_DEFAULT << 20)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH0                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH0 << 20)   /**< Shifted mode PRSCH0 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH1                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH1 << 20)   /**< Shifted mode PRSCH1 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH2                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH2 << 20)   /**< Shifted mode PRSCH2 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH3                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH3 << 20)   /**< Shifted mode PRSCH3 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH4                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH4 << 20)   /**< Shifted mode PRSCH4 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH5                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH5 << 20)   /**< Shifted mode PRSCH5 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH6                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH6 << 20)   /**< Shifted mode PRSCH6 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH7                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH7 << 20)   /**< Shifted mode PRSCH7 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH8                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH8 << 20)   /**< Shifted mode PRSCH8 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH9                 (_LESENSE_DECCTRL_PRSSEL2_PRSCH9 << 20)   /**< Shifted mode PRSCH9 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH10                (_LESENSE_DECCTRL_PRSSEL2_PRSCH10 << 20)  /**< Shifted mode PRSCH10 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL2_PRSCH11                (_LESENSE_DECCTRL_PRSSEL2_PRSCH11 << 20)  /**< Shifted mode PRSCH11 for LESENSE_DECCTRL */
#define _LESENSE_DECCTRL_PRSSEL3_SHIFT                 25                                        /**< Shift value for LESENSE_PRSSEL3 */
#define _LESENSE_DECCTRL_PRSSEL3_MASK                  0x1E000000UL                              /**< Bit mask for LESENSE_PRSSEL3 */
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
#define LESENSE_DECCTRL_PRSSEL3_DEFAULT                (_LESENSE_DECCTRL_PRSSEL3_DEFAULT << 25)  /**< Shifted mode DEFAULT for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH0                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH0 << 25)   /**< Shifted mode PRSCH0 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH1                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH1 << 25)   /**< Shifted mode PRSCH1 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH2                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH2 << 25)   /**< Shifted mode PRSCH2 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH3                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH3 << 25)   /**< Shifted mode PRSCH3 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH4                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH4 << 25)   /**< Shifted mode PRSCH4 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH5                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH5 << 25)   /**< Shifted mode PRSCH5 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH6                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH6 << 25)   /**< Shifted mode PRSCH6 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH7                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH7 << 25)   /**< Shifted mode PRSCH7 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH8                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH8 << 25)   /**< Shifted mode PRSCH8 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH9                 (_LESENSE_DECCTRL_PRSSEL3_PRSCH9 << 25)   /**< Shifted mode PRSCH9 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH10                (_LESENSE_DECCTRL_PRSSEL3_PRSCH10 << 25)  /**< Shifted mode PRSCH10 for LESENSE_DECCTRL */
#define LESENSE_DECCTRL_PRSSEL3_PRSCH11                (_LESENSE_DECCTRL_PRSSEL3_PRSCH11 << 25)  /**< Shifted mode PRSCH11 for LESENSE_DECCTRL */

/* Bit fields for LESENSE BIASCTRL */
#define _LESENSE_BIASCTRL_RESETVALUE                   0x00000000UL                                /**< Default value for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_MASK                         0x00000003UL                                /**< Mask for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_BIASMODE_SHIFT               0                                           /**< Shift value for LESENSE_BIASMODE */
#define _LESENSE_BIASCTRL_BIASMODE_MASK                0x3UL                                       /**< Bit mask for LESENSE_BIASMODE */
#define _LESENSE_BIASCTRL_BIASMODE_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_BIASMODE_DONTTOUCH           0x00000000UL                                /**< Mode DONTTOUCH for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_BIASMODE_DUTYCYCLE           0x00000001UL                                /**< Mode DUTYCYCLE for LESENSE_BIASCTRL */
#define _LESENSE_BIASCTRL_BIASMODE_HIGHACC             0x00000002UL                                /**< Mode HIGHACC for LESENSE_BIASCTRL */
#define LESENSE_BIASCTRL_BIASMODE_DEFAULT              (_LESENSE_BIASCTRL_BIASMODE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_BIASCTRL */
#define LESENSE_BIASCTRL_BIASMODE_DONTTOUCH            (_LESENSE_BIASCTRL_BIASMODE_DONTTOUCH << 0) /**< Shifted mode DONTTOUCH for LESENSE_BIASCTRL */
#define LESENSE_BIASCTRL_BIASMODE_DUTYCYCLE            (_LESENSE_BIASCTRL_BIASMODE_DUTYCYCLE << 0) /**< Shifted mode DUTYCYCLE for LESENSE_BIASCTRL */
#define LESENSE_BIASCTRL_BIASMODE_HIGHACC              (_LESENSE_BIASCTRL_BIASMODE_HIGHACC << 0)   /**< Shifted mode HIGHACC for LESENSE_BIASCTRL */

/* Bit fields for LESENSE EVALCTRL */
#define _LESENSE_EVALCTRL_RESETVALUE                   0x00000000UL                             /**< Default value for LESENSE_EVALCTRL */
#define _LESENSE_EVALCTRL_MASK                         0x0000FFFFUL                             /**< Mask for LESENSE_EVALCTRL */
#define _LESENSE_EVALCTRL_WINSIZE_SHIFT                0                                        /**< Shift value for LESENSE_WINSIZE */
#define _LESENSE_EVALCTRL_WINSIZE_MASK                 0xFFFFUL                                 /**< Bit mask for LESENSE_WINSIZE */
#define _LESENSE_EVALCTRL_WINSIZE_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for LESENSE_EVALCTRL */
#define LESENSE_EVALCTRL_WINSIZE_DEFAULT               (_LESENSE_EVALCTRL_WINSIZE_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_EVALCTRL */

/* Bit fields for LESENSE PRSCTRL */
#define _LESENSE_PRSCTRL_RESETVALUE                    0x00000000UL                               /**< Default value for LESENSE_PRSCTRL */
#define _LESENSE_PRSCTRL_MASK                          0x00011F1FUL                               /**< Mask for LESENSE_PRSCTRL */
#define _LESENSE_PRSCTRL_DECCMPVAL_SHIFT               0                                          /**< Shift value for LESENSE_DECCMPVAL */
#define _LESENSE_PRSCTRL_DECCMPVAL_MASK                0x1FUL                                     /**< Bit mask for LESENSE_DECCMPVAL */
#define _LESENSE_PRSCTRL_DECCMPVAL_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for LESENSE_PRSCTRL */
#define LESENSE_PRSCTRL_DECCMPVAL_DEFAULT              (_LESENSE_PRSCTRL_DECCMPVAL_DEFAULT << 0)  /**< Shifted mode DEFAULT for LESENSE_PRSCTRL */
#define _LESENSE_PRSCTRL_DECCMPMASK_SHIFT              8                                          /**< Shift value for LESENSE_DECCMPMASK */
#define _LESENSE_PRSCTRL_DECCMPMASK_MASK               0x1F00UL                                   /**< Bit mask for LESENSE_DECCMPMASK */
#define _LESENSE_PRSCTRL_DECCMPMASK_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for LESENSE_PRSCTRL */
#define LESENSE_PRSCTRL_DECCMPMASK_DEFAULT             (_LESENSE_PRSCTRL_DECCMPMASK_DEFAULT << 8) /**< Shifted mode DEFAULT for LESENSE_PRSCTRL */
#define LESENSE_PRSCTRL_DECCMPEN                       (0x1UL << 16)                              /**< Enable PRS Output DECCMP */
#define _LESENSE_PRSCTRL_DECCMPEN_SHIFT                16                                         /**< Shift value for LESENSE_DECCMPEN */
#define _LESENSE_PRSCTRL_DECCMPEN_MASK                 0x10000UL                                  /**< Bit mask for LESENSE_DECCMPEN */
#define _LESENSE_PRSCTRL_DECCMPEN_DEFAULT              0x00000000UL                               /**< Mode DEFAULT for LESENSE_PRSCTRL */
#define LESENSE_PRSCTRL_DECCMPEN_DEFAULT               (_LESENSE_PRSCTRL_DECCMPEN_DEFAULT << 16)  /**< Shifted mode DEFAULT for LESENSE_PRSCTRL */

/* Bit fields for LESENSE CMD */
#define _LESENSE_CMD_RESETVALUE                        0x00000000UL                         /**< Default value for LESENSE_CMD */
#define _LESENSE_CMD_MASK                              0x0000000FUL                         /**< Mask for LESENSE_CMD */
#define LESENSE_CMD_START                              (0x1UL << 0)                         /**< Start Scanning of Sensors */
#define _LESENSE_CMD_START_SHIFT                       0                                    /**< Shift value for LESENSE_START */
#define _LESENSE_CMD_START_MASK                        0x1UL                                /**< Bit mask for LESENSE_START */
#define _LESENSE_CMD_START_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_START_DEFAULT                      (_LESENSE_CMD_START_DEFAULT << 0)    /**< Shifted mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_STOP                               (0x1UL << 1)                         /**< Stop Scanning of Sensors */
#define _LESENSE_CMD_STOP_SHIFT                        1                                    /**< Shift value for LESENSE_STOP */
#define _LESENSE_CMD_STOP_MASK                         0x2UL                                /**< Bit mask for LESENSE_STOP */
#define _LESENSE_CMD_STOP_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_STOP_DEFAULT                       (_LESENSE_CMD_STOP_DEFAULT << 1)     /**< Shifted mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_DECODE                             (0x1UL << 2)                         /**< Start Decoder */
#define _LESENSE_CMD_DECODE_SHIFT                      2                                    /**< Shift value for LESENSE_DECODE */
#define _LESENSE_CMD_DECODE_MASK                       0x4UL                                /**< Bit mask for LESENSE_DECODE */
#define _LESENSE_CMD_DECODE_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_DECODE_DEFAULT                     (_LESENSE_CMD_DECODE_DEFAULT << 2)   /**< Shifted mode DEFAULT for LESENSE_CMD */
#define LESENSE_CMD_CLEARBUF                           (0x1UL << 3)                         /**< Clear Result Buffer */
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
#define _LESENSE_SCANRES_RESETVALUE                    0x00000000UL                             /**< Default value for LESENSE_SCANRES */
#define _LESENSE_SCANRES_MASK                          0xFFFFFFFFUL                             /**< Mask for LESENSE_SCANRES */
#define _LESENSE_SCANRES_SCANRES_SHIFT                 0                                        /**< Shift value for LESENSE_SCANRES */
#define _LESENSE_SCANRES_SCANRES_MASK                  0xFFFFUL                                 /**< Bit mask for LESENSE_SCANRES */
#define _LESENSE_SCANRES_SCANRES_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_SCANRES */
#define LESENSE_SCANRES_SCANRES_DEFAULT                (_LESENSE_SCANRES_SCANRES_DEFAULT << 0)  /**< Shifted mode DEFAULT for LESENSE_SCANRES */
#define _LESENSE_SCANRES_STEPDIR_SHIFT                 16                                       /**< Shift value for LESENSE_STEPDIR */
#define _LESENSE_SCANRES_STEPDIR_MASK                  0xFFFF0000UL                             /**< Bit mask for LESENSE_STEPDIR */
#define _LESENSE_SCANRES_STEPDIR_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_SCANRES */
#define LESENSE_SCANRES_STEPDIR_DEFAULT                (_LESENSE_SCANRES_STEPDIR_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_SCANRES */

/* Bit fields for LESENSE STATUS */
#define _LESENSE_STATUS_RESETVALUE                     0x00000000UL                               /**< Default value for LESENSE_STATUS */
#define _LESENSE_STATUS_MASK                           0x0000003FUL                               /**< Mask for LESENSE_STATUS */
#define LESENSE_STATUS_BUFDATAV                        (0x1UL << 0)                               /**< Result Data Valid */
#define _LESENSE_STATUS_BUFDATAV_SHIFT                 0                                          /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_STATUS_BUFDATAV_MASK                  0x1UL                                      /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_STATUS_BUFDATAV_DEFAULT               0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFDATAV_DEFAULT                (_LESENSE_STATUS_BUFDATAV_DEFAULT << 0)    /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFHALFFULL                     (0x1UL << 1)                               /**< Result Buffer Half Full */
#define _LESENSE_STATUS_BUFHALFFULL_SHIFT              1                                          /**< Shift value for LESENSE_BUFHALFFULL */
#define _LESENSE_STATUS_BUFHALFFULL_MASK               0x2UL                                      /**< Bit mask for LESENSE_BUFHALFFULL */
#define _LESENSE_STATUS_BUFHALFFULL_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFHALFFULL_DEFAULT             (_LESENSE_STATUS_BUFHALFFULL_DEFAULT << 1) /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFFULL                         (0x1UL << 2)                               /**< Result Buffer Full */
#define _LESENSE_STATUS_BUFFULL_SHIFT                  2                                          /**< Shift value for LESENSE_BUFFULL */
#define _LESENSE_STATUS_BUFFULL_MASK                   0x4UL                                      /**< Bit mask for LESENSE_BUFFULL */
#define _LESENSE_STATUS_BUFFULL_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_BUFFULL_DEFAULT                 (_LESENSE_STATUS_BUFFULL_DEFAULT << 2)     /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_RUNNING                         (0x1UL << 3)                               /**< LESENSE Periodic Counter Running */
#define _LESENSE_STATUS_RUNNING_SHIFT                  3                                          /**< Shift value for LESENSE_RUNNING */
#define _LESENSE_STATUS_RUNNING_MASK                   0x8UL                                      /**< Bit mask for LESENSE_RUNNING */
#define _LESENSE_STATUS_RUNNING_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_RUNNING_DEFAULT                 (_LESENSE_STATUS_RUNNING_DEFAULT << 3)     /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_SCANACTIVE                      (0x1UL << 4)                               /**< LESENSE Scan Active */
#define _LESENSE_STATUS_SCANACTIVE_SHIFT               4                                          /**< Shift value for LESENSE_SCANACTIVE */
#define _LESENSE_STATUS_SCANACTIVE_MASK                0x10UL                                     /**< Bit mask for LESENSE_SCANACTIVE */
#define _LESENSE_STATUS_SCANACTIVE_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_SCANACTIVE_DEFAULT              (_LESENSE_STATUS_SCANACTIVE_DEFAULT << 4)  /**< Shifted mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_DACACTIVE                       (0x1UL << 5)                               /**< LESENSE VDAC Interface is Active */
#define _LESENSE_STATUS_DACACTIVE_SHIFT                5                                          /**< Shift value for LESENSE_DACACTIVE */
#define _LESENSE_STATUS_DACACTIVE_MASK                 0x20UL                                     /**< Bit mask for LESENSE_DACACTIVE */
#define _LESENSE_STATUS_DACACTIVE_DEFAULT              0x00000000UL                               /**< Mode DEFAULT for LESENSE_STATUS */
#define LESENSE_STATUS_DACACTIVE_DEFAULT               (_LESENSE_STATUS_DACACTIVE_DEFAULT << 5)   /**< Shifted mode DEFAULT for LESENSE_STATUS */

/* Bit fields for LESENSE PTR */
#define _LESENSE_PTR_RESETVALUE                        0x00000000UL                   /**< Default value for LESENSE_PTR */
#define _LESENSE_PTR_MASK                              0x000000FFUL                   /**< Mask for LESENSE_PTR */
#define _LESENSE_PTR_RD_SHIFT                          0                              /**< Shift value for LESENSE_RD */
#define _LESENSE_PTR_RD_MASK                           0xFUL                          /**< Bit mask for LESENSE_RD */
#define _LESENSE_PTR_RD_DEFAULT                        0x00000000UL                   /**< Mode DEFAULT for LESENSE_PTR */
#define LESENSE_PTR_RD_DEFAULT                         (_LESENSE_PTR_RD_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_PTR */
#define _LESENSE_PTR_WR_SHIFT                          4                              /**< Shift value for LESENSE_WR */
#define _LESENSE_PTR_WR_MASK                           0xF0UL                         /**< Bit mask for LESENSE_WR */
#define _LESENSE_PTR_WR_DEFAULT                        0x00000000UL                   /**< Mode DEFAULT for LESENSE_PTR */
#define LESENSE_PTR_WR_DEFAULT                         (_LESENSE_PTR_WR_DEFAULT << 4) /**< Shifted mode DEFAULT for LESENSE_PTR */

/* Bit fields for LESENSE BUFDATA */
#define _LESENSE_BUFDATA_RESETVALUE                    0x00000000UL                                /**< Default value for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_MASK                          0x000FFFFFUL                                /**< Mask for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_BUFDATA_SHIFT                 0                                           /**< Shift value for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_BUFDATA_MASK                  0xFFFFUL                                    /**< Bit mask for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_BUFDATA_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_BUFDATA */
#define LESENSE_BUFDATA_BUFDATA_DEFAULT                (_LESENSE_BUFDATA_BUFDATA_DEFAULT << 0)     /**< Shifted mode DEFAULT for LESENSE_BUFDATA */
#define _LESENSE_BUFDATA_BUFDATASRC_SHIFT              16                                          /**< Shift value for LESENSE_BUFDATASRC */
#define _LESENSE_BUFDATA_BUFDATASRC_MASK               0xF0000UL                                   /**< Bit mask for LESENSE_BUFDATASRC */
#define _LESENSE_BUFDATA_BUFDATASRC_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_BUFDATA */
#define LESENSE_BUFDATA_BUFDATASRC_DEFAULT             (_LESENSE_BUFDATA_BUFDATASRC_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_BUFDATA */

/* Bit fields for LESENSE CURCH */
#define _LESENSE_CURCH_RESETVALUE                      0x00000000UL                        /**< Default value for LESENSE_CURCH */
#define _LESENSE_CURCH_MASK                            0x0000000FUL                        /**< Mask for LESENSE_CURCH */
#define _LESENSE_CURCH_CURCH_SHIFT                     0                                   /**< Shift value for LESENSE_CURCH */
#define _LESENSE_CURCH_CURCH_MASK                      0xFUL                               /**< Bit mask for LESENSE_CURCH */
#define _LESENSE_CURCH_CURCH_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for LESENSE_CURCH */
#define LESENSE_CURCH_CURCH_DEFAULT                    (_LESENSE_CURCH_CURCH_DEFAULT << 0) /**< Shifted mode DEFAULT for LESENSE_CURCH */

/* Bit fields for LESENSE DECSTATE */
#define _LESENSE_DECSTATE_RESETVALUE                   0x00000000UL                              /**< Default value for LESENSE_DECSTATE */
#define _LESENSE_DECSTATE_MASK                         0x0000001FUL                              /**< Mask for LESENSE_DECSTATE */
#define _LESENSE_DECSTATE_DECSTATE_SHIFT               0                                         /**< Shift value for LESENSE_DECSTATE */
#define _LESENSE_DECSTATE_DECSTATE_MASK                0x1FUL                                    /**< Bit mask for LESENSE_DECSTATE */
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
#define _LESENSE_IDLECONF_CH0_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_DEFAULT                   (_LESENSE_IDLECONF_CH0_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_DISABLE                   (_LESENSE_IDLECONF_CH0_DISABLE << 0)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_HIGH                      (_LESENSE_IDLECONF_CH0_HIGH << 0)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_LOW                       (_LESENSE_IDLECONF_CH0_LOW << 0)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH0_DAC                       (_LESENSE_IDLECONF_CH0_DAC << 0)       /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_SHIFT                    2                                      /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IDLECONF_CH1_MASK                     0xCUL                                  /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IDLECONF_CH1_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH1_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_DEFAULT                   (_LESENSE_IDLECONF_CH1_DEFAULT << 2)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_DISABLE                   (_LESENSE_IDLECONF_CH1_DISABLE << 2)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_HIGH                      (_LESENSE_IDLECONF_CH1_HIGH << 2)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_LOW                       (_LESENSE_IDLECONF_CH1_LOW << 2)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH1_DAC                       (_LESENSE_IDLECONF_CH1_DAC << 2)       /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_SHIFT                    4                                      /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IDLECONF_CH2_MASK                     0x30UL                                 /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IDLECONF_CH2_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH2_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_DEFAULT                   (_LESENSE_IDLECONF_CH2_DEFAULT << 4)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_DISABLE                   (_LESENSE_IDLECONF_CH2_DISABLE << 4)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_HIGH                      (_LESENSE_IDLECONF_CH2_HIGH << 4)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_LOW                       (_LESENSE_IDLECONF_CH2_LOW << 4)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH2_DAC                       (_LESENSE_IDLECONF_CH2_DAC << 4)       /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_SHIFT                    6                                      /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IDLECONF_CH3_MASK                     0xC0UL                                 /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IDLECONF_CH3_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH3_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_DEFAULT                   (_LESENSE_IDLECONF_CH3_DEFAULT << 6)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_DISABLE                   (_LESENSE_IDLECONF_CH3_DISABLE << 6)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_HIGH                      (_LESENSE_IDLECONF_CH3_HIGH << 6)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_LOW                       (_LESENSE_IDLECONF_CH3_LOW << 6)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH3_DAC                       (_LESENSE_IDLECONF_CH3_DAC << 6)       /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_SHIFT                    8                                      /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IDLECONF_CH4_MASK                     0x300UL                                /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IDLECONF_CH4_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH4_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_DEFAULT                   (_LESENSE_IDLECONF_CH4_DEFAULT << 8)   /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_DISABLE                   (_LESENSE_IDLECONF_CH4_DISABLE << 8)   /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_HIGH                      (_LESENSE_IDLECONF_CH4_HIGH << 8)      /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_LOW                       (_LESENSE_IDLECONF_CH4_LOW << 8)       /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH4_DAC                       (_LESENSE_IDLECONF_CH4_DAC << 8)       /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_SHIFT                    10                                     /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IDLECONF_CH5_MASK                     0xC00UL                                /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IDLECONF_CH5_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH5_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_DEFAULT                   (_LESENSE_IDLECONF_CH5_DEFAULT << 10)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_DISABLE                   (_LESENSE_IDLECONF_CH5_DISABLE << 10)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_HIGH                      (_LESENSE_IDLECONF_CH5_HIGH << 10)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_LOW                       (_LESENSE_IDLECONF_CH5_LOW << 10)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH5_DAC                       (_LESENSE_IDLECONF_CH5_DAC << 10)      /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_SHIFT                    12                                     /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IDLECONF_CH6_MASK                     0x3000UL                               /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IDLECONF_CH6_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH6_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_DEFAULT                   (_LESENSE_IDLECONF_CH6_DEFAULT << 12)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_DISABLE                   (_LESENSE_IDLECONF_CH6_DISABLE << 12)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_HIGH                      (_LESENSE_IDLECONF_CH6_HIGH << 12)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_LOW                       (_LESENSE_IDLECONF_CH6_LOW << 12)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH6_DAC                       (_LESENSE_IDLECONF_CH6_DAC << 12)      /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_SHIFT                    14                                     /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IDLECONF_CH7_MASK                     0xC000UL                               /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IDLECONF_CH7_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH7_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_DEFAULT                   (_LESENSE_IDLECONF_CH7_DEFAULT << 14)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_DISABLE                   (_LESENSE_IDLECONF_CH7_DISABLE << 14)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_HIGH                      (_LESENSE_IDLECONF_CH7_HIGH << 14)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_LOW                       (_LESENSE_IDLECONF_CH7_LOW << 14)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH7_DAC                       (_LESENSE_IDLECONF_CH7_DAC << 14)      /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_SHIFT                    16                                     /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IDLECONF_CH8_MASK                     0x30000UL                              /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IDLECONF_CH8_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH8_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_DEFAULT                   (_LESENSE_IDLECONF_CH8_DEFAULT << 16)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_DISABLE                   (_LESENSE_IDLECONF_CH8_DISABLE << 16)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_HIGH                      (_LESENSE_IDLECONF_CH8_HIGH << 16)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_LOW                       (_LESENSE_IDLECONF_CH8_LOW << 16)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH8_DAC                       (_LESENSE_IDLECONF_CH8_DAC << 16)      /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_SHIFT                    18                                     /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IDLECONF_CH9_MASK                     0xC0000UL                              /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IDLECONF_CH9_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_DISABLE                  0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_HIGH                     0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_LOW                      0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH9_DAC                      0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_DEFAULT                   (_LESENSE_IDLECONF_CH9_DEFAULT << 18)  /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_DISABLE                   (_LESENSE_IDLECONF_CH9_DISABLE << 18)  /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_HIGH                      (_LESENSE_IDLECONF_CH9_HIGH << 18)     /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_LOW                       (_LESENSE_IDLECONF_CH9_LOW << 18)      /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH9_DAC                       (_LESENSE_IDLECONF_CH9_DAC << 18)      /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_SHIFT                   20                                     /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IDLECONF_CH10_MASK                    0x300000UL                             /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IDLECONF_CH10_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH10_DAC                     0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_DEFAULT                  (_LESENSE_IDLECONF_CH10_DEFAULT << 20) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_DISABLE                  (_LESENSE_IDLECONF_CH10_DISABLE << 20) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_HIGH                     (_LESENSE_IDLECONF_CH10_HIGH << 20)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_LOW                      (_LESENSE_IDLECONF_CH10_LOW << 20)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH10_DAC                      (_LESENSE_IDLECONF_CH10_DAC << 20)     /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_SHIFT                   22                                     /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IDLECONF_CH11_MASK                    0xC00000UL                             /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IDLECONF_CH11_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH11_DAC                     0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_DEFAULT                  (_LESENSE_IDLECONF_CH11_DEFAULT << 22) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_DISABLE                  (_LESENSE_IDLECONF_CH11_DISABLE << 22) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_HIGH                     (_LESENSE_IDLECONF_CH11_HIGH << 22)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_LOW                      (_LESENSE_IDLECONF_CH11_LOW << 22)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH11_DAC                      (_LESENSE_IDLECONF_CH11_DAC << 22)     /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_SHIFT                   24                                     /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IDLECONF_CH12_MASK                    0x3000000UL                            /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IDLECONF_CH12_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH12_DAC                     0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_DEFAULT                  (_LESENSE_IDLECONF_CH12_DEFAULT << 24) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_DISABLE                  (_LESENSE_IDLECONF_CH12_DISABLE << 24) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_HIGH                     (_LESENSE_IDLECONF_CH12_HIGH << 24)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_LOW                      (_LESENSE_IDLECONF_CH12_LOW << 24)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH12_DAC                      (_LESENSE_IDLECONF_CH12_DAC << 24)     /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_SHIFT                   26                                     /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IDLECONF_CH13_MASK                    0xC000000UL                            /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IDLECONF_CH13_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH13_DAC                     0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_DEFAULT                  (_LESENSE_IDLECONF_CH13_DEFAULT << 26) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_DISABLE                  (_LESENSE_IDLECONF_CH13_DISABLE << 26) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_HIGH                     (_LESENSE_IDLECONF_CH13_HIGH << 26)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_LOW                      (_LESENSE_IDLECONF_CH13_LOW << 26)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH13_DAC                      (_LESENSE_IDLECONF_CH13_DAC << 26)     /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_SHIFT                   28                                     /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IDLECONF_CH14_MASK                    0x30000000UL                           /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IDLECONF_CH14_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH14_DAC                     0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_DEFAULT                  (_LESENSE_IDLECONF_CH14_DEFAULT << 28) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_DISABLE                  (_LESENSE_IDLECONF_CH14_DISABLE << 28) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_HIGH                     (_LESENSE_IDLECONF_CH14_HIGH << 28)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_LOW                      (_LESENSE_IDLECONF_CH14_LOW << 28)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH14_DAC                      (_LESENSE_IDLECONF_CH14_DAC << 28)     /**< Shifted mode DAC for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_SHIFT                   30                                     /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IDLECONF_CH15_MASK                    0xC0000000UL                           /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IDLECONF_CH15_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_DISABLE                 0x00000000UL                           /**< Mode DISABLE for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_HIGH                    0x00000001UL                           /**< Mode HIGH for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_LOW                     0x00000002UL                           /**< Mode LOW for LESENSE_IDLECONF */
#define _LESENSE_IDLECONF_CH15_DAC                     0x00000003UL                           /**< Mode DAC for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_DEFAULT                  (_LESENSE_IDLECONF_CH15_DEFAULT << 30) /**< Shifted mode DEFAULT for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_DISABLE                  (_LESENSE_IDLECONF_CH15_DISABLE << 30) /**< Shifted mode DISABLE for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_HIGH                     (_LESENSE_IDLECONF_CH15_HIGH << 30)    /**< Shifted mode HIGH for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_LOW                      (_LESENSE_IDLECONF_CH15_LOW << 30)     /**< Shifted mode LOW for LESENSE_IDLECONF */
#define LESENSE_IDLECONF_CH15_DAC                      (_LESENSE_IDLECONF_CH15_DAC << 30)     /**< Shifted mode DAC for LESENSE_IDLECONF */

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
#define LESENSE_ALTEXCONF_AEX0                         (0x1UL << 16)                                /**< ALTEX0 Always Excite Enable */
#define _LESENSE_ALTEXCONF_AEX0_SHIFT                  16                                           /**< Shift value for LESENSE_AEX0 */
#define _LESENSE_ALTEXCONF_AEX0_MASK                   0x10000UL                                    /**< Bit mask for LESENSE_AEX0 */
#define _LESENSE_ALTEXCONF_AEX0_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX0_DEFAULT                 (_LESENSE_ALTEXCONF_AEX0_DEFAULT << 16)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX1                         (0x1UL << 17)                                /**< ALTEX1 Always Excite Enable */
#define _LESENSE_ALTEXCONF_AEX1_SHIFT                  17                                           /**< Shift value for LESENSE_AEX1 */
#define _LESENSE_ALTEXCONF_AEX1_MASK                   0x20000UL                                    /**< Bit mask for LESENSE_AEX1 */
#define _LESENSE_ALTEXCONF_AEX1_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX1_DEFAULT                 (_LESENSE_ALTEXCONF_AEX1_DEFAULT << 17)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX2                         (0x1UL << 18)                                /**< ALTEX2 Always Excite Enable */
#define _LESENSE_ALTEXCONF_AEX2_SHIFT                  18                                           /**< Shift value for LESENSE_AEX2 */
#define _LESENSE_ALTEXCONF_AEX2_MASK                   0x40000UL                                    /**< Bit mask for LESENSE_AEX2 */
#define _LESENSE_ALTEXCONF_AEX2_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX2_DEFAULT                 (_LESENSE_ALTEXCONF_AEX2_DEFAULT << 18)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX3                         (0x1UL << 19)                                /**< ALTEX3 Always Excite Enable */
#define _LESENSE_ALTEXCONF_AEX3_SHIFT                  19                                           /**< Shift value for LESENSE_AEX3 */
#define _LESENSE_ALTEXCONF_AEX3_MASK                   0x80000UL                                    /**< Bit mask for LESENSE_AEX3 */
#define _LESENSE_ALTEXCONF_AEX3_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX3_DEFAULT                 (_LESENSE_ALTEXCONF_AEX3_DEFAULT << 19)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX4                         (0x1UL << 20)                                /**< ALTEX4 Always Excite Enable */
#define _LESENSE_ALTEXCONF_AEX4_SHIFT                  20                                           /**< Shift value for LESENSE_AEX4 */
#define _LESENSE_ALTEXCONF_AEX4_MASK                   0x100000UL                                   /**< Bit mask for LESENSE_AEX4 */
#define _LESENSE_ALTEXCONF_AEX4_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX4_DEFAULT                 (_LESENSE_ALTEXCONF_AEX4_DEFAULT << 20)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX5                         (0x1UL << 21)                                /**< ALTEX5 Always Excite Enable */
#define _LESENSE_ALTEXCONF_AEX5_SHIFT                  21                                           /**< Shift value for LESENSE_AEX5 */
#define _LESENSE_ALTEXCONF_AEX5_MASK                   0x200000UL                                   /**< Bit mask for LESENSE_AEX5 */
#define _LESENSE_ALTEXCONF_AEX5_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX5_DEFAULT                 (_LESENSE_ALTEXCONF_AEX5_DEFAULT << 21)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX6                         (0x1UL << 22)                                /**< ALTEX6 Always Excite Enable */
#define _LESENSE_ALTEXCONF_AEX6_SHIFT                  22                                           /**< Shift value for LESENSE_AEX6 */
#define _LESENSE_ALTEXCONF_AEX6_MASK                   0x400000UL                                   /**< Bit mask for LESENSE_AEX6 */
#define _LESENSE_ALTEXCONF_AEX6_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX6_DEFAULT                 (_LESENSE_ALTEXCONF_AEX6_DEFAULT << 22)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX7                         (0x1UL << 23)                                /**< ALTEX7 Always Excite Enable */
#define _LESENSE_ALTEXCONF_AEX7_SHIFT                  23                                           /**< Shift value for LESENSE_AEX7 */
#define _LESENSE_ALTEXCONF_AEX7_MASK                   0x800000UL                                   /**< Bit mask for LESENSE_AEX7 */
#define _LESENSE_ALTEXCONF_AEX7_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for LESENSE_ALTEXCONF */
#define LESENSE_ALTEXCONF_AEX7_DEFAULT                 (_LESENSE_ALTEXCONF_AEX7_DEFAULT << 23)      /**< Shifted mode DEFAULT for LESENSE_ALTEXCONF */

/* Bit fields for LESENSE IF */
#define _LESENSE_IF_RESETVALUE                         0x00000000UL                             /**< Default value for LESENSE_IF */
#define _LESENSE_IF_MASK                               0x007FFFFFUL                             /**< Mask for LESENSE_IF */
#define LESENSE_IF_CH0                                 (0x1UL << 0)                             /**< CH0 Interrupt Flag */
#define _LESENSE_IF_CH0_SHIFT                          0                                        /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IF_CH0_MASK                           0x1UL                                    /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IF_CH0_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH0_DEFAULT                         (_LESENSE_IF_CH0_DEFAULT << 0)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH1                                 (0x1UL << 1)                             /**< CH1 Interrupt Flag */
#define _LESENSE_IF_CH1_SHIFT                          1                                        /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IF_CH1_MASK                           0x2UL                                    /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IF_CH1_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH1_DEFAULT                         (_LESENSE_IF_CH1_DEFAULT << 1)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH2                                 (0x1UL << 2)                             /**< CH2 Interrupt Flag */
#define _LESENSE_IF_CH2_SHIFT                          2                                        /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IF_CH2_MASK                           0x4UL                                    /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IF_CH2_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH2_DEFAULT                         (_LESENSE_IF_CH2_DEFAULT << 2)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH3                                 (0x1UL << 3)                             /**< CH3 Interrupt Flag */
#define _LESENSE_IF_CH3_SHIFT                          3                                        /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IF_CH3_MASK                           0x8UL                                    /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IF_CH3_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH3_DEFAULT                         (_LESENSE_IF_CH3_DEFAULT << 3)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH4                                 (0x1UL << 4)                             /**< CH4 Interrupt Flag */
#define _LESENSE_IF_CH4_SHIFT                          4                                        /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IF_CH4_MASK                           0x10UL                                   /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IF_CH4_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH4_DEFAULT                         (_LESENSE_IF_CH4_DEFAULT << 4)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH5                                 (0x1UL << 5)                             /**< CH5 Interrupt Flag */
#define _LESENSE_IF_CH5_SHIFT                          5                                        /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IF_CH5_MASK                           0x20UL                                   /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IF_CH5_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH5_DEFAULT                         (_LESENSE_IF_CH5_DEFAULT << 5)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH6                                 (0x1UL << 6)                             /**< CH6 Interrupt Flag */
#define _LESENSE_IF_CH6_SHIFT                          6                                        /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IF_CH6_MASK                           0x40UL                                   /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IF_CH6_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH6_DEFAULT                         (_LESENSE_IF_CH6_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH7                                 (0x1UL << 7)                             /**< CH7 Interrupt Flag */
#define _LESENSE_IF_CH7_SHIFT                          7                                        /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IF_CH7_MASK                           0x80UL                                   /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IF_CH7_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH7_DEFAULT                         (_LESENSE_IF_CH7_DEFAULT << 7)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH8                                 (0x1UL << 8)                             /**< CH8 Interrupt Flag */
#define _LESENSE_IF_CH8_SHIFT                          8                                        /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IF_CH8_MASK                           0x100UL                                  /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IF_CH8_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH8_DEFAULT                         (_LESENSE_IF_CH8_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH9                                 (0x1UL << 9)                             /**< CH9 Interrupt Flag */
#define _LESENSE_IF_CH9_SHIFT                          9                                        /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IF_CH9_MASK                           0x200UL                                  /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IF_CH9_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH9_DEFAULT                         (_LESENSE_IF_CH9_DEFAULT << 9)           /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH10                                (0x1UL << 10)                            /**< CH10 Interrupt Flag */
#define _LESENSE_IF_CH10_SHIFT                         10                                       /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IF_CH10_MASK                          0x400UL                                  /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IF_CH10_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH10_DEFAULT                        (_LESENSE_IF_CH10_DEFAULT << 10)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH11                                (0x1UL << 11)                            /**< CH11 Interrupt Flag */
#define _LESENSE_IF_CH11_SHIFT                         11                                       /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IF_CH11_MASK                          0x800UL                                  /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IF_CH11_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH11_DEFAULT                        (_LESENSE_IF_CH11_DEFAULT << 11)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH12                                (0x1UL << 12)                            /**< CH12 Interrupt Flag */
#define _LESENSE_IF_CH12_SHIFT                         12                                       /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IF_CH12_MASK                          0x1000UL                                 /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IF_CH12_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH12_DEFAULT                        (_LESENSE_IF_CH12_DEFAULT << 12)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH13                                (0x1UL << 13)                            /**< CH13 Interrupt Flag */
#define _LESENSE_IF_CH13_SHIFT                         13                                       /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IF_CH13_MASK                          0x2000UL                                 /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IF_CH13_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH13_DEFAULT                        (_LESENSE_IF_CH13_DEFAULT << 13)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH14                                (0x1UL << 14)                            /**< CH14 Interrupt Flag */
#define _LESENSE_IF_CH14_SHIFT                         14                                       /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IF_CH14_MASK                          0x4000UL                                 /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IF_CH14_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH14_DEFAULT                        (_LESENSE_IF_CH14_DEFAULT << 14)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH15                                (0x1UL << 15)                            /**< CH15 Interrupt Flag */
#define _LESENSE_IF_CH15_SHIFT                         15                                       /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IF_CH15_MASK                          0x8000UL                                 /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IF_CH15_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CH15_DEFAULT                        (_LESENSE_IF_CH15_DEFAULT << 15)         /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_SCANCOMPLETE                        (0x1UL << 16)                            /**< SCANCOMPLETE Interrupt Flag */
#define _LESENSE_IF_SCANCOMPLETE_SHIFT                 16                                       /**< Shift value for LESENSE_SCANCOMPLETE */
#define _LESENSE_IF_SCANCOMPLETE_MASK                  0x10000UL                                /**< Bit mask for LESENSE_SCANCOMPLETE */
#define _LESENSE_IF_SCANCOMPLETE_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_SCANCOMPLETE_DEFAULT                (_LESENSE_IF_SCANCOMPLETE_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_DEC                                 (0x1UL << 17)                            /**< DEC Interrupt Flag */
#define _LESENSE_IF_DEC_SHIFT                          17                                       /**< Shift value for LESENSE_DEC */
#define _LESENSE_IF_DEC_MASK                           0x20000UL                                /**< Bit mask for LESENSE_DEC */
#define _LESENSE_IF_DEC_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_DEC_DEFAULT                         (_LESENSE_IF_DEC_DEFAULT << 17)          /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_DECERR                              (0x1UL << 18)                            /**< DECERR Interrupt Flag */
#define _LESENSE_IF_DECERR_SHIFT                       18                                       /**< Shift value for LESENSE_DECERR */
#define _LESENSE_IF_DECERR_MASK                        0x40000UL                                /**< Bit mask for LESENSE_DECERR */
#define _LESENSE_IF_DECERR_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_DECERR_DEFAULT                      (_LESENSE_IF_DECERR_DEFAULT << 18)       /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFDATAV                            (0x1UL << 19)                            /**< BUFDATAV Interrupt Flag */
#define _LESENSE_IF_BUFDATAV_SHIFT                     19                                       /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_IF_BUFDATAV_MASK                      0x80000UL                                /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_IF_BUFDATAV_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFDATAV_DEFAULT                    (_LESENSE_IF_BUFDATAV_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFLEVEL                            (0x1UL << 20)                            /**< BUFLEVEL Interrupt Flag */
#define _LESENSE_IF_BUFLEVEL_SHIFT                     20                                       /**< Shift value for LESENSE_BUFLEVEL */
#define _LESENSE_IF_BUFLEVEL_MASK                      0x100000UL                               /**< Bit mask for LESENSE_BUFLEVEL */
#define _LESENSE_IF_BUFLEVEL_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFLEVEL_DEFAULT                    (_LESENSE_IF_BUFLEVEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFOF                               (0x1UL << 21)                            /**< BUFOF Interrupt Flag */
#define _LESENSE_IF_BUFOF_SHIFT                        21                                       /**< Shift value for LESENSE_BUFOF */
#define _LESENSE_IF_BUFOF_MASK                         0x200000UL                               /**< Bit mask for LESENSE_BUFOF */
#define _LESENSE_IF_BUFOF_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_BUFOF_DEFAULT                       (_LESENSE_IF_BUFOF_DEFAULT << 21)        /**< Shifted mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CNTOF                               (0x1UL << 22)                            /**< CNTOF Interrupt Flag */
#define _LESENSE_IF_CNTOF_SHIFT                        22                                       /**< Shift value for LESENSE_CNTOF */
#define _LESENSE_IF_CNTOF_MASK                         0x400000UL                               /**< Bit mask for LESENSE_CNTOF */
#define _LESENSE_IF_CNTOF_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for LESENSE_IF */
#define LESENSE_IF_CNTOF_DEFAULT                       (_LESENSE_IF_CNTOF_DEFAULT << 22)        /**< Shifted mode DEFAULT for LESENSE_IF */

/* Bit fields for LESENSE IFS */
#define _LESENSE_IFS_RESETVALUE                        0x00000000UL                              /**< Default value for LESENSE_IFS */
#define _LESENSE_IFS_MASK                              0x007FFFFFUL                              /**< Mask for LESENSE_IFS */
#define LESENSE_IFS_CH0                                (0x1UL << 0)                              /**< Set CH0 Interrupt Flag */
#define _LESENSE_IFS_CH0_SHIFT                         0                                         /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IFS_CH0_MASK                          0x1UL                                     /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IFS_CH0_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH0_DEFAULT                        (_LESENSE_IFS_CH0_DEFAULT << 0)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH1                                (0x1UL << 1)                              /**< Set CH1 Interrupt Flag */
#define _LESENSE_IFS_CH1_SHIFT                         1                                         /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IFS_CH1_MASK                          0x2UL                                     /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IFS_CH1_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH1_DEFAULT                        (_LESENSE_IFS_CH1_DEFAULT << 1)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH2                                (0x1UL << 2)                              /**< Set CH2 Interrupt Flag */
#define _LESENSE_IFS_CH2_SHIFT                         2                                         /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IFS_CH2_MASK                          0x4UL                                     /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IFS_CH2_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH2_DEFAULT                        (_LESENSE_IFS_CH2_DEFAULT << 2)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH3                                (0x1UL << 3)                              /**< Set CH3 Interrupt Flag */
#define _LESENSE_IFS_CH3_SHIFT                         3                                         /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IFS_CH3_MASK                          0x8UL                                     /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IFS_CH3_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH3_DEFAULT                        (_LESENSE_IFS_CH3_DEFAULT << 3)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH4                                (0x1UL << 4)                              /**< Set CH4 Interrupt Flag */
#define _LESENSE_IFS_CH4_SHIFT                         4                                         /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IFS_CH4_MASK                          0x10UL                                    /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IFS_CH4_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH4_DEFAULT                        (_LESENSE_IFS_CH4_DEFAULT << 4)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH5                                (0x1UL << 5)                              /**< Set CH5 Interrupt Flag */
#define _LESENSE_IFS_CH5_SHIFT                         5                                         /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IFS_CH5_MASK                          0x20UL                                    /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IFS_CH5_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH5_DEFAULT                        (_LESENSE_IFS_CH5_DEFAULT << 5)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH6                                (0x1UL << 6)                              /**< Set CH6 Interrupt Flag */
#define _LESENSE_IFS_CH6_SHIFT                         6                                         /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IFS_CH6_MASK                          0x40UL                                    /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IFS_CH6_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH6_DEFAULT                        (_LESENSE_IFS_CH6_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH7                                (0x1UL << 7)                              /**< Set CH7 Interrupt Flag */
#define _LESENSE_IFS_CH7_SHIFT                         7                                         /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IFS_CH7_MASK                          0x80UL                                    /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IFS_CH7_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH7_DEFAULT                        (_LESENSE_IFS_CH7_DEFAULT << 7)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH8                                (0x1UL << 8)                              /**< Set CH8 Interrupt Flag */
#define _LESENSE_IFS_CH8_SHIFT                         8                                         /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IFS_CH8_MASK                          0x100UL                                   /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IFS_CH8_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH8_DEFAULT                        (_LESENSE_IFS_CH8_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH9                                (0x1UL << 9)                              /**< Set CH9 Interrupt Flag */
#define _LESENSE_IFS_CH9_SHIFT                         9                                         /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IFS_CH9_MASK                          0x200UL                                   /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IFS_CH9_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH9_DEFAULT                        (_LESENSE_IFS_CH9_DEFAULT << 9)           /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH10                               (0x1UL << 10)                             /**< Set CH10 Interrupt Flag */
#define _LESENSE_IFS_CH10_SHIFT                        10                                        /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IFS_CH10_MASK                         0x400UL                                   /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IFS_CH10_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH10_DEFAULT                       (_LESENSE_IFS_CH10_DEFAULT << 10)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH11                               (0x1UL << 11)                             /**< Set CH11 Interrupt Flag */
#define _LESENSE_IFS_CH11_SHIFT                        11                                        /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IFS_CH11_MASK                         0x800UL                                   /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IFS_CH11_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH11_DEFAULT                       (_LESENSE_IFS_CH11_DEFAULT << 11)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH12                               (0x1UL << 12)                             /**< Set CH12 Interrupt Flag */
#define _LESENSE_IFS_CH12_SHIFT                        12                                        /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IFS_CH12_MASK                         0x1000UL                                  /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IFS_CH12_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH12_DEFAULT                       (_LESENSE_IFS_CH12_DEFAULT << 12)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH13                               (0x1UL << 13)                             /**< Set CH13 Interrupt Flag */
#define _LESENSE_IFS_CH13_SHIFT                        13                                        /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IFS_CH13_MASK                         0x2000UL                                  /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IFS_CH13_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH13_DEFAULT                       (_LESENSE_IFS_CH13_DEFAULT << 13)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH14                               (0x1UL << 14)                             /**< Set CH14 Interrupt Flag */
#define _LESENSE_IFS_CH14_SHIFT                        14                                        /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IFS_CH14_MASK                         0x4000UL                                  /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IFS_CH14_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH14_DEFAULT                       (_LESENSE_IFS_CH14_DEFAULT << 14)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH15                               (0x1UL << 15)                             /**< Set CH15 Interrupt Flag */
#define _LESENSE_IFS_CH15_SHIFT                        15                                        /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IFS_CH15_MASK                         0x8000UL                                  /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IFS_CH15_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CH15_DEFAULT                       (_LESENSE_IFS_CH15_DEFAULT << 15)         /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_SCANCOMPLETE                       (0x1UL << 16)                             /**< Set SCANCOMPLETE Interrupt Flag */
#define _LESENSE_IFS_SCANCOMPLETE_SHIFT                16                                        /**< Shift value for LESENSE_SCANCOMPLETE */
#define _LESENSE_IFS_SCANCOMPLETE_MASK                 0x10000UL                                 /**< Bit mask for LESENSE_SCANCOMPLETE */
#define _LESENSE_IFS_SCANCOMPLETE_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_SCANCOMPLETE_DEFAULT               (_LESENSE_IFS_SCANCOMPLETE_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_DEC                                (0x1UL << 17)                             /**< Set DEC Interrupt Flag */
#define _LESENSE_IFS_DEC_SHIFT                         17                                        /**< Shift value for LESENSE_DEC */
#define _LESENSE_IFS_DEC_MASK                          0x20000UL                                 /**< Bit mask for LESENSE_DEC */
#define _LESENSE_IFS_DEC_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_DEC_DEFAULT                        (_LESENSE_IFS_DEC_DEFAULT << 17)          /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_DECERR                             (0x1UL << 18)                             /**< Set DECERR Interrupt Flag */
#define _LESENSE_IFS_DECERR_SHIFT                      18                                        /**< Shift value for LESENSE_DECERR */
#define _LESENSE_IFS_DECERR_MASK                       0x40000UL                                 /**< Bit mask for LESENSE_DECERR */
#define _LESENSE_IFS_DECERR_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_DECERR_DEFAULT                     (_LESENSE_IFS_DECERR_DEFAULT << 18)       /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFDATAV                           (0x1UL << 19)                             /**< Set BUFDATAV Interrupt Flag */
#define _LESENSE_IFS_BUFDATAV_SHIFT                    19                                        /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_IFS_BUFDATAV_MASK                     0x80000UL                                 /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_IFS_BUFDATAV_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFDATAV_DEFAULT                   (_LESENSE_IFS_BUFDATAV_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFLEVEL                           (0x1UL << 20)                             /**< Set BUFLEVEL Interrupt Flag */
#define _LESENSE_IFS_BUFLEVEL_SHIFT                    20                                        /**< Shift value for LESENSE_BUFLEVEL */
#define _LESENSE_IFS_BUFLEVEL_MASK                     0x100000UL                                /**< Bit mask for LESENSE_BUFLEVEL */
#define _LESENSE_IFS_BUFLEVEL_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFLEVEL_DEFAULT                   (_LESENSE_IFS_BUFLEVEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFOF                              (0x1UL << 21)                             /**< Set BUFOF Interrupt Flag */
#define _LESENSE_IFS_BUFOF_SHIFT                       21                                        /**< Shift value for LESENSE_BUFOF */
#define _LESENSE_IFS_BUFOF_MASK                        0x200000UL                                /**< Bit mask for LESENSE_BUFOF */
#define _LESENSE_IFS_BUFOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_BUFOF_DEFAULT                      (_LESENSE_IFS_BUFOF_DEFAULT << 21)        /**< Shifted mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CNTOF                              (0x1UL << 22)                             /**< Set CNTOF Interrupt Flag */
#define _LESENSE_IFS_CNTOF_SHIFT                       22                                        /**< Shift value for LESENSE_CNTOF */
#define _LESENSE_IFS_CNTOF_MASK                        0x400000UL                                /**< Bit mask for LESENSE_CNTOF */
#define _LESENSE_IFS_CNTOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFS */
#define LESENSE_IFS_CNTOF_DEFAULT                      (_LESENSE_IFS_CNTOF_DEFAULT << 22)        /**< Shifted mode DEFAULT for LESENSE_IFS */

/* Bit fields for LESENSE IFC */
#define _LESENSE_IFC_RESETVALUE                        0x00000000UL                              /**< Default value for LESENSE_IFC */
#define _LESENSE_IFC_MASK                              0x007FFFFFUL                              /**< Mask for LESENSE_IFC */
#define LESENSE_IFC_CH0                                (0x1UL << 0)                              /**< Clear CH0 Interrupt Flag */
#define _LESENSE_IFC_CH0_SHIFT                         0                                         /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IFC_CH0_MASK                          0x1UL                                     /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IFC_CH0_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH0_DEFAULT                        (_LESENSE_IFC_CH0_DEFAULT << 0)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH1                                (0x1UL << 1)                              /**< Clear CH1 Interrupt Flag */
#define _LESENSE_IFC_CH1_SHIFT                         1                                         /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IFC_CH1_MASK                          0x2UL                                     /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IFC_CH1_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH1_DEFAULT                        (_LESENSE_IFC_CH1_DEFAULT << 1)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH2                                (0x1UL << 2)                              /**< Clear CH2 Interrupt Flag */
#define _LESENSE_IFC_CH2_SHIFT                         2                                         /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IFC_CH2_MASK                          0x4UL                                     /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IFC_CH2_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH2_DEFAULT                        (_LESENSE_IFC_CH2_DEFAULT << 2)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH3                                (0x1UL << 3)                              /**< Clear CH3 Interrupt Flag */
#define _LESENSE_IFC_CH3_SHIFT                         3                                         /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IFC_CH3_MASK                          0x8UL                                     /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IFC_CH3_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH3_DEFAULT                        (_LESENSE_IFC_CH3_DEFAULT << 3)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH4                                (0x1UL << 4)                              /**< Clear CH4 Interrupt Flag */
#define _LESENSE_IFC_CH4_SHIFT                         4                                         /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IFC_CH4_MASK                          0x10UL                                    /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IFC_CH4_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH4_DEFAULT                        (_LESENSE_IFC_CH4_DEFAULT << 4)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH5                                (0x1UL << 5)                              /**< Clear CH5 Interrupt Flag */
#define _LESENSE_IFC_CH5_SHIFT                         5                                         /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IFC_CH5_MASK                          0x20UL                                    /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IFC_CH5_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH5_DEFAULT                        (_LESENSE_IFC_CH5_DEFAULT << 5)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH6                                (0x1UL << 6)                              /**< Clear CH6 Interrupt Flag */
#define _LESENSE_IFC_CH6_SHIFT                         6                                         /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IFC_CH6_MASK                          0x40UL                                    /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IFC_CH6_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH6_DEFAULT                        (_LESENSE_IFC_CH6_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH7                                (0x1UL << 7)                              /**< Clear CH7 Interrupt Flag */
#define _LESENSE_IFC_CH7_SHIFT                         7                                         /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IFC_CH7_MASK                          0x80UL                                    /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IFC_CH7_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH7_DEFAULT                        (_LESENSE_IFC_CH7_DEFAULT << 7)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH8                                (0x1UL << 8)                              /**< Clear CH8 Interrupt Flag */
#define _LESENSE_IFC_CH8_SHIFT                         8                                         /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IFC_CH8_MASK                          0x100UL                                   /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IFC_CH8_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH8_DEFAULT                        (_LESENSE_IFC_CH8_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH9                                (0x1UL << 9)                              /**< Clear CH9 Interrupt Flag */
#define _LESENSE_IFC_CH9_SHIFT                         9                                         /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IFC_CH9_MASK                          0x200UL                                   /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IFC_CH9_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH9_DEFAULT                        (_LESENSE_IFC_CH9_DEFAULT << 9)           /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH10                               (0x1UL << 10)                             /**< Clear CH10 Interrupt Flag */
#define _LESENSE_IFC_CH10_SHIFT                        10                                        /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IFC_CH10_MASK                         0x400UL                                   /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IFC_CH10_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH10_DEFAULT                       (_LESENSE_IFC_CH10_DEFAULT << 10)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH11                               (0x1UL << 11)                             /**< Clear CH11 Interrupt Flag */
#define _LESENSE_IFC_CH11_SHIFT                        11                                        /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IFC_CH11_MASK                         0x800UL                                   /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IFC_CH11_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH11_DEFAULT                       (_LESENSE_IFC_CH11_DEFAULT << 11)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH12                               (0x1UL << 12)                             /**< Clear CH12 Interrupt Flag */
#define _LESENSE_IFC_CH12_SHIFT                        12                                        /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IFC_CH12_MASK                         0x1000UL                                  /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IFC_CH12_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH12_DEFAULT                       (_LESENSE_IFC_CH12_DEFAULT << 12)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH13                               (0x1UL << 13)                             /**< Clear CH13 Interrupt Flag */
#define _LESENSE_IFC_CH13_SHIFT                        13                                        /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IFC_CH13_MASK                         0x2000UL                                  /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IFC_CH13_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH13_DEFAULT                       (_LESENSE_IFC_CH13_DEFAULT << 13)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH14                               (0x1UL << 14)                             /**< Clear CH14 Interrupt Flag */
#define _LESENSE_IFC_CH14_SHIFT                        14                                        /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IFC_CH14_MASK                         0x4000UL                                  /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IFC_CH14_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH14_DEFAULT                       (_LESENSE_IFC_CH14_DEFAULT << 14)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH15                               (0x1UL << 15)                             /**< Clear CH15 Interrupt Flag */
#define _LESENSE_IFC_CH15_SHIFT                        15                                        /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IFC_CH15_MASK                         0x8000UL                                  /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IFC_CH15_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CH15_DEFAULT                       (_LESENSE_IFC_CH15_DEFAULT << 15)         /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_SCANCOMPLETE                       (0x1UL << 16)                             /**< Clear SCANCOMPLETE Interrupt Flag */
#define _LESENSE_IFC_SCANCOMPLETE_SHIFT                16                                        /**< Shift value for LESENSE_SCANCOMPLETE */
#define _LESENSE_IFC_SCANCOMPLETE_MASK                 0x10000UL                                 /**< Bit mask for LESENSE_SCANCOMPLETE */
#define _LESENSE_IFC_SCANCOMPLETE_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_SCANCOMPLETE_DEFAULT               (_LESENSE_IFC_SCANCOMPLETE_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_DEC                                (0x1UL << 17)                             /**< Clear DEC Interrupt Flag */
#define _LESENSE_IFC_DEC_SHIFT                         17                                        /**< Shift value for LESENSE_DEC */
#define _LESENSE_IFC_DEC_MASK                          0x20000UL                                 /**< Bit mask for LESENSE_DEC */
#define _LESENSE_IFC_DEC_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_DEC_DEFAULT                        (_LESENSE_IFC_DEC_DEFAULT << 17)          /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_DECERR                             (0x1UL << 18)                             /**< Clear DECERR Interrupt Flag */
#define _LESENSE_IFC_DECERR_SHIFT                      18                                        /**< Shift value for LESENSE_DECERR */
#define _LESENSE_IFC_DECERR_MASK                       0x40000UL                                 /**< Bit mask for LESENSE_DECERR */
#define _LESENSE_IFC_DECERR_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_DECERR_DEFAULT                     (_LESENSE_IFC_DECERR_DEFAULT << 18)       /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFDATAV                           (0x1UL << 19)                             /**< Clear BUFDATAV Interrupt Flag */
#define _LESENSE_IFC_BUFDATAV_SHIFT                    19                                        /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_IFC_BUFDATAV_MASK                     0x80000UL                                 /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_IFC_BUFDATAV_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFDATAV_DEFAULT                   (_LESENSE_IFC_BUFDATAV_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFLEVEL                           (0x1UL << 20)                             /**< Clear BUFLEVEL Interrupt Flag */
#define _LESENSE_IFC_BUFLEVEL_SHIFT                    20                                        /**< Shift value for LESENSE_BUFLEVEL */
#define _LESENSE_IFC_BUFLEVEL_MASK                     0x100000UL                                /**< Bit mask for LESENSE_BUFLEVEL */
#define _LESENSE_IFC_BUFLEVEL_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFLEVEL_DEFAULT                   (_LESENSE_IFC_BUFLEVEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFOF                              (0x1UL << 21)                             /**< Clear BUFOF Interrupt Flag */
#define _LESENSE_IFC_BUFOF_SHIFT                       21                                        /**< Shift value for LESENSE_BUFOF */
#define _LESENSE_IFC_BUFOF_MASK                        0x200000UL                                /**< Bit mask for LESENSE_BUFOF */
#define _LESENSE_IFC_BUFOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_BUFOF_DEFAULT                      (_LESENSE_IFC_BUFOF_DEFAULT << 21)        /**< Shifted mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CNTOF                              (0x1UL << 22)                             /**< Clear CNTOF Interrupt Flag */
#define _LESENSE_IFC_CNTOF_SHIFT                       22                                        /**< Shift value for LESENSE_CNTOF */
#define _LESENSE_IFC_CNTOF_MASK                        0x400000UL                                /**< Bit mask for LESENSE_CNTOF */
#define _LESENSE_IFC_CNTOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IFC */
#define LESENSE_IFC_CNTOF_DEFAULT                      (_LESENSE_IFC_CNTOF_DEFAULT << 22)        /**< Shifted mode DEFAULT for LESENSE_IFC */

/* Bit fields for LESENSE IEN */
#define _LESENSE_IEN_RESETVALUE                        0x00000000UL                              /**< Default value for LESENSE_IEN */
#define _LESENSE_IEN_MASK                              0x007FFFFFUL                              /**< Mask for LESENSE_IEN */
#define LESENSE_IEN_CH0                                (0x1UL << 0)                              /**< CH0 Interrupt Enable */
#define _LESENSE_IEN_CH0_SHIFT                         0                                         /**< Shift value for LESENSE_CH0 */
#define _LESENSE_IEN_CH0_MASK                          0x1UL                                     /**< Bit mask for LESENSE_CH0 */
#define _LESENSE_IEN_CH0_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH0_DEFAULT                        (_LESENSE_IEN_CH0_DEFAULT << 0)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH1                                (0x1UL << 1)                              /**< CH1 Interrupt Enable */
#define _LESENSE_IEN_CH1_SHIFT                         1                                         /**< Shift value for LESENSE_CH1 */
#define _LESENSE_IEN_CH1_MASK                          0x2UL                                     /**< Bit mask for LESENSE_CH1 */
#define _LESENSE_IEN_CH1_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH1_DEFAULT                        (_LESENSE_IEN_CH1_DEFAULT << 1)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH2                                (0x1UL << 2)                              /**< CH2 Interrupt Enable */
#define _LESENSE_IEN_CH2_SHIFT                         2                                         /**< Shift value for LESENSE_CH2 */
#define _LESENSE_IEN_CH2_MASK                          0x4UL                                     /**< Bit mask for LESENSE_CH2 */
#define _LESENSE_IEN_CH2_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH2_DEFAULT                        (_LESENSE_IEN_CH2_DEFAULT << 2)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH3                                (0x1UL << 3)                              /**< CH3 Interrupt Enable */
#define _LESENSE_IEN_CH3_SHIFT                         3                                         /**< Shift value for LESENSE_CH3 */
#define _LESENSE_IEN_CH3_MASK                          0x8UL                                     /**< Bit mask for LESENSE_CH3 */
#define _LESENSE_IEN_CH3_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH3_DEFAULT                        (_LESENSE_IEN_CH3_DEFAULT << 3)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH4                                (0x1UL << 4)                              /**< CH4 Interrupt Enable */
#define _LESENSE_IEN_CH4_SHIFT                         4                                         /**< Shift value for LESENSE_CH4 */
#define _LESENSE_IEN_CH4_MASK                          0x10UL                                    /**< Bit mask for LESENSE_CH4 */
#define _LESENSE_IEN_CH4_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH4_DEFAULT                        (_LESENSE_IEN_CH4_DEFAULT << 4)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH5                                (0x1UL << 5)                              /**< CH5 Interrupt Enable */
#define _LESENSE_IEN_CH5_SHIFT                         5                                         /**< Shift value for LESENSE_CH5 */
#define _LESENSE_IEN_CH5_MASK                          0x20UL                                    /**< Bit mask for LESENSE_CH5 */
#define _LESENSE_IEN_CH5_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH5_DEFAULT                        (_LESENSE_IEN_CH5_DEFAULT << 5)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH6                                (0x1UL << 6)                              /**< CH6 Interrupt Enable */
#define _LESENSE_IEN_CH6_SHIFT                         6                                         /**< Shift value for LESENSE_CH6 */
#define _LESENSE_IEN_CH6_MASK                          0x40UL                                    /**< Bit mask for LESENSE_CH6 */
#define _LESENSE_IEN_CH6_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH6_DEFAULT                        (_LESENSE_IEN_CH6_DEFAULT << 6)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH7                                (0x1UL << 7)                              /**< CH7 Interrupt Enable */
#define _LESENSE_IEN_CH7_SHIFT                         7                                         /**< Shift value for LESENSE_CH7 */
#define _LESENSE_IEN_CH7_MASK                          0x80UL                                    /**< Bit mask for LESENSE_CH7 */
#define _LESENSE_IEN_CH7_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH7_DEFAULT                        (_LESENSE_IEN_CH7_DEFAULT << 7)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH8                                (0x1UL << 8)                              /**< CH8 Interrupt Enable */
#define _LESENSE_IEN_CH8_SHIFT                         8                                         /**< Shift value for LESENSE_CH8 */
#define _LESENSE_IEN_CH8_MASK                          0x100UL                                   /**< Bit mask for LESENSE_CH8 */
#define _LESENSE_IEN_CH8_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH8_DEFAULT                        (_LESENSE_IEN_CH8_DEFAULT << 8)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH9                                (0x1UL << 9)                              /**< CH9 Interrupt Enable */
#define _LESENSE_IEN_CH9_SHIFT                         9                                         /**< Shift value for LESENSE_CH9 */
#define _LESENSE_IEN_CH9_MASK                          0x200UL                                   /**< Bit mask for LESENSE_CH9 */
#define _LESENSE_IEN_CH9_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH9_DEFAULT                        (_LESENSE_IEN_CH9_DEFAULT << 9)           /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH10                               (0x1UL << 10)                             /**< CH10 Interrupt Enable */
#define _LESENSE_IEN_CH10_SHIFT                        10                                        /**< Shift value for LESENSE_CH10 */
#define _LESENSE_IEN_CH10_MASK                         0x400UL                                   /**< Bit mask for LESENSE_CH10 */
#define _LESENSE_IEN_CH10_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH10_DEFAULT                       (_LESENSE_IEN_CH10_DEFAULT << 10)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH11                               (0x1UL << 11)                             /**< CH11 Interrupt Enable */
#define _LESENSE_IEN_CH11_SHIFT                        11                                        /**< Shift value for LESENSE_CH11 */
#define _LESENSE_IEN_CH11_MASK                         0x800UL                                   /**< Bit mask for LESENSE_CH11 */
#define _LESENSE_IEN_CH11_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH11_DEFAULT                       (_LESENSE_IEN_CH11_DEFAULT << 11)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH12                               (0x1UL << 12)                             /**< CH12 Interrupt Enable */
#define _LESENSE_IEN_CH12_SHIFT                        12                                        /**< Shift value for LESENSE_CH12 */
#define _LESENSE_IEN_CH12_MASK                         0x1000UL                                  /**< Bit mask for LESENSE_CH12 */
#define _LESENSE_IEN_CH12_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH12_DEFAULT                       (_LESENSE_IEN_CH12_DEFAULT << 12)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH13                               (0x1UL << 13)                             /**< CH13 Interrupt Enable */
#define _LESENSE_IEN_CH13_SHIFT                        13                                        /**< Shift value for LESENSE_CH13 */
#define _LESENSE_IEN_CH13_MASK                         0x2000UL                                  /**< Bit mask for LESENSE_CH13 */
#define _LESENSE_IEN_CH13_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH13_DEFAULT                       (_LESENSE_IEN_CH13_DEFAULT << 13)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH14                               (0x1UL << 14)                             /**< CH14 Interrupt Enable */
#define _LESENSE_IEN_CH14_SHIFT                        14                                        /**< Shift value for LESENSE_CH14 */
#define _LESENSE_IEN_CH14_MASK                         0x4000UL                                  /**< Bit mask for LESENSE_CH14 */
#define _LESENSE_IEN_CH14_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH14_DEFAULT                       (_LESENSE_IEN_CH14_DEFAULT << 14)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH15                               (0x1UL << 15)                             /**< CH15 Interrupt Enable */
#define _LESENSE_IEN_CH15_SHIFT                        15                                        /**< Shift value for LESENSE_CH15 */
#define _LESENSE_IEN_CH15_MASK                         0x8000UL                                  /**< Bit mask for LESENSE_CH15 */
#define _LESENSE_IEN_CH15_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CH15_DEFAULT                       (_LESENSE_IEN_CH15_DEFAULT << 15)         /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_SCANCOMPLETE                       (0x1UL << 16)                             /**< SCANCOMPLETE Interrupt Enable */
#define _LESENSE_IEN_SCANCOMPLETE_SHIFT                16                                        /**< Shift value for LESENSE_SCANCOMPLETE */
#define _LESENSE_IEN_SCANCOMPLETE_MASK                 0x10000UL                                 /**< Bit mask for LESENSE_SCANCOMPLETE */
#define _LESENSE_IEN_SCANCOMPLETE_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_SCANCOMPLETE_DEFAULT               (_LESENSE_IEN_SCANCOMPLETE_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_DEC                                (0x1UL << 17)                             /**< DEC Interrupt Enable */
#define _LESENSE_IEN_DEC_SHIFT                         17                                        /**< Shift value for LESENSE_DEC */
#define _LESENSE_IEN_DEC_MASK                          0x20000UL                                 /**< Bit mask for LESENSE_DEC */
#define _LESENSE_IEN_DEC_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_DEC_DEFAULT                        (_LESENSE_IEN_DEC_DEFAULT << 17)          /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_DECERR                             (0x1UL << 18)                             /**< DECERR Interrupt Enable */
#define _LESENSE_IEN_DECERR_SHIFT                      18                                        /**< Shift value for LESENSE_DECERR */
#define _LESENSE_IEN_DECERR_MASK                       0x40000UL                                 /**< Bit mask for LESENSE_DECERR */
#define _LESENSE_IEN_DECERR_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_DECERR_DEFAULT                     (_LESENSE_IEN_DECERR_DEFAULT << 18)       /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFDATAV                           (0x1UL << 19)                             /**< BUFDATAV Interrupt Enable */
#define _LESENSE_IEN_BUFDATAV_SHIFT                    19                                        /**< Shift value for LESENSE_BUFDATAV */
#define _LESENSE_IEN_BUFDATAV_MASK                     0x80000UL                                 /**< Bit mask for LESENSE_BUFDATAV */
#define _LESENSE_IEN_BUFDATAV_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFDATAV_DEFAULT                   (_LESENSE_IEN_BUFDATAV_DEFAULT << 19)     /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFLEVEL                           (0x1UL << 20)                             /**< BUFLEVEL Interrupt Enable */
#define _LESENSE_IEN_BUFLEVEL_SHIFT                    20                                        /**< Shift value for LESENSE_BUFLEVEL */
#define _LESENSE_IEN_BUFLEVEL_MASK                     0x100000UL                                /**< Bit mask for LESENSE_BUFLEVEL */
#define _LESENSE_IEN_BUFLEVEL_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFLEVEL_DEFAULT                   (_LESENSE_IEN_BUFLEVEL_DEFAULT << 20)     /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFOF                              (0x1UL << 21)                             /**< BUFOF Interrupt Enable */
#define _LESENSE_IEN_BUFOF_SHIFT                       21                                        /**< Shift value for LESENSE_BUFOF */
#define _LESENSE_IEN_BUFOF_MASK                        0x200000UL                                /**< Bit mask for LESENSE_BUFOF */
#define _LESENSE_IEN_BUFOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_BUFOF_DEFAULT                      (_LESENSE_IEN_BUFOF_DEFAULT << 21)        /**< Shifted mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CNTOF                              (0x1UL << 22)                             /**< CNTOF Interrupt Enable */
#define _LESENSE_IEN_CNTOF_SHIFT                       22                                        /**< Shift value for LESENSE_CNTOF */
#define _LESENSE_IEN_CNTOF_MASK                        0x400000UL                                /**< Bit mask for LESENSE_CNTOF */
#define _LESENSE_IEN_CNTOF_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for LESENSE_IEN */
#define LESENSE_IEN_CNTOF_DEFAULT                      (_LESENSE_IEN_CNTOF_DEFAULT << 22)        /**< Shifted mode DEFAULT for LESENSE_IEN */

/* Bit fields for LESENSE SYNCBUSY */
#define _LESENSE_SYNCBUSY_RESETVALUE                   0x00000000UL                         /**< Default value for LESENSE_SYNCBUSY */
#define _LESENSE_SYNCBUSY_MASK                         0x00000080UL                         /**< Mask for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CMD                           (0x1UL << 7)                         /**< CMD Register Busy */
#define _LESENSE_SYNCBUSY_CMD_SHIFT                    7                                    /**< Shift value for LESENSE_CMD */
#define _LESENSE_SYNCBUSY_CMD_MASK                     0x80UL                               /**< Bit mask for LESENSE_CMD */
#define _LESENSE_SYNCBUSY_CMD_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for LESENSE_SYNCBUSY */
#define LESENSE_SYNCBUSY_CMD_DEFAULT                   (_LESENSE_SYNCBUSY_CMD_DEFAULT << 7) /**< Shifted mode DEFAULT for LESENSE_SYNCBUSY */

/* Bit fields for LESENSE ROUTEPEN */
#define _LESENSE_ROUTEPEN_RESETVALUE                   0x00000000UL                                /**< Default value for LESENSE_ROUTEPEN */
#define _LESENSE_ROUTEPEN_MASK                         0x00FFFFFFUL                                /**< Mask for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH0PEN                        (0x1UL << 0)                                /**< CH0 Pin Enable */
#define _LESENSE_ROUTEPEN_CH0PEN_SHIFT                 0                                           /**< Shift value for LESENSE_CH0PEN */
#define _LESENSE_ROUTEPEN_CH0PEN_MASK                  0x1UL                                       /**< Bit mask for LESENSE_CH0PEN */
#define _LESENSE_ROUTEPEN_CH0PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH0PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH0PEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH1PEN                        (0x1UL << 1)                                /**< CH1 Pin Enable */
#define _LESENSE_ROUTEPEN_CH1PEN_SHIFT                 1                                           /**< Shift value for LESENSE_CH1PEN */
#define _LESENSE_ROUTEPEN_CH1PEN_MASK                  0x2UL                                       /**< Bit mask for LESENSE_CH1PEN */
#define _LESENSE_ROUTEPEN_CH1PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH1PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH1PEN_DEFAULT << 1)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH2PEN                        (0x1UL << 2)                                /**< CH2 Pin Enable */
#define _LESENSE_ROUTEPEN_CH2PEN_SHIFT                 2                                           /**< Shift value for LESENSE_CH2PEN */
#define _LESENSE_ROUTEPEN_CH2PEN_MASK                  0x4UL                                       /**< Bit mask for LESENSE_CH2PEN */
#define _LESENSE_ROUTEPEN_CH2PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH2PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH2PEN_DEFAULT << 2)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH3PEN                        (0x1UL << 3)                                /**< CH3 Pin Enable */
#define _LESENSE_ROUTEPEN_CH3PEN_SHIFT                 3                                           /**< Shift value for LESENSE_CH3PEN */
#define _LESENSE_ROUTEPEN_CH3PEN_MASK                  0x8UL                                       /**< Bit mask for LESENSE_CH3PEN */
#define _LESENSE_ROUTEPEN_CH3PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH3PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH3PEN_DEFAULT << 3)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH4PEN                        (0x1UL << 4)                                /**< CH4 Pin Enable */
#define _LESENSE_ROUTEPEN_CH4PEN_SHIFT                 4                                           /**< Shift value for LESENSE_CH4PEN */
#define _LESENSE_ROUTEPEN_CH4PEN_MASK                  0x10UL                                      /**< Bit mask for LESENSE_CH4PEN */
#define _LESENSE_ROUTEPEN_CH4PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH4PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH4PEN_DEFAULT << 4)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH5PEN                        (0x1UL << 5)                                /**< CH5 Pin Enable */
#define _LESENSE_ROUTEPEN_CH5PEN_SHIFT                 5                                           /**< Shift value for LESENSE_CH5PEN */
#define _LESENSE_ROUTEPEN_CH5PEN_MASK                  0x20UL                                      /**< Bit mask for LESENSE_CH5PEN */
#define _LESENSE_ROUTEPEN_CH5PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH5PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH5PEN_DEFAULT << 5)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH6PEN                        (0x1UL << 6)                                /**< CH6 Pin Enable */
#define _LESENSE_ROUTEPEN_CH6PEN_SHIFT                 6                                           /**< Shift value for LESENSE_CH6PEN */
#define _LESENSE_ROUTEPEN_CH6PEN_MASK                  0x40UL                                      /**< Bit mask for LESENSE_CH6PEN */
#define _LESENSE_ROUTEPEN_CH6PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH6PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH6PEN_DEFAULT << 6)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH7PEN                        (0x1UL << 7)                                /**< CH7 Pin Enable */
#define _LESENSE_ROUTEPEN_CH7PEN_SHIFT                 7                                           /**< Shift value for LESENSE_CH7PEN */
#define _LESENSE_ROUTEPEN_CH7PEN_MASK                  0x80UL                                      /**< Bit mask for LESENSE_CH7PEN */
#define _LESENSE_ROUTEPEN_CH7PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH7PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH7PEN_DEFAULT << 7)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH8PEN                        (0x1UL << 8)                                /**< CH8 Pin Enable */
#define _LESENSE_ROUTEPEN_CH8PEN_SHIFT                 8                                           /**< Shift value for LESENSE_CH8PEN */
#define _LESENSE_ROUTEPEN_CH8PEN_MASK                  0x100UL                                     /**< Bit mask for LESENSE_CH8PEN */
#define _LESENSE_ROUTEPEN_CH8PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH8PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH8PEN_DEFAULT << 8)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH9PEN                        (0x1UL << 9)                                /**< CH9 Pin Enable */
#define _LESENSE_ROUTEPEN_CH9PEN_SHIFT                 9                                           /**< Shift value for LESENSE_CH9PEN */
#define _LESENSE_ROUTEPEN_CH9PEN_MASK                  0x200UL                                     /**< Bit mask for LESENSE_CH9PEN */
#define _LESENSE_ROUTEPEN_CH9PEN_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH9PEN_DEFAULT                (_LESENSE_ROUTEPEN_CH9PEN_DEFAULT << 9)     /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH10PEN                       (0x1UL << 10)                               /**< CH10 Pin Enable */
#define _LESENSE_ROUTEPEN_CH10PEN_SHIFT                10                                          /**< Shift value for LESENSE_CH10PEN */
#define _LESENSE_ROUTEPEN_CH10PEN_MASK                 0x400UL                                     /**< Bit mask for LESENSE_CH10PEN */
#define _LESENSE_ROUTEPEN_CH10PEN_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH10PEN_DEFAULT               (_LESENSE_ROUTEPEN_CH10PEN_DEFAULT << 10)   /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH11PEN                       (0x1UL << 11)                               /**< CH11 Pin Enable */
#define _LESENSE_ROUTEPEN_CH11PEN_SHIFT                11                                          /**< Shift value for LESENSE_CH11PEN */
#define _LESENSE_ROUTEPEN_CH11PEN_MASK                 0x800UL                                     /**< Bit mask for LESENSE_CH11PEN */
#define _LESENSE_ROUTEPEN_CH11PEN_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH11PEN_DEFAULT               (_LESENSE_ROUTEPEN_CH11PEN_DEFAULT << 11)   /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH12PEN                       (0x1UL << 12)                               /**< CH12 Pin Enable */
#define _LESENSE_ROUTEPEN_CH12PEN_SHIFT                12                                          /**< Shift value for LESENSE_CH12PEN */
#define _LESENSE_ROUTEPEN_CH12PEN_MASK                 0x1000UL                                    /**< Bit mask for LESENSE_CH12PEN */
#define _LESENSE_ROUTEPEN_CH12PEN_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH12PEN_DEFAULT               (_LESENSE_ROUTEPEN_CH12PEN_DEFAULT << 12)   /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH13PEN                       (0x1UL << 13)                               /**< CH13 Pin Enable */
#define _LESENSE_ROUTEPEN_CH13PEN_SHIFT                13                                          /**< Shift value for LESENSE_CH13PEN */
#define _LESENSE_ROUTEPEN_CH13PEN_MASK                 0x2000UL                                    /**< Bit mask for LESENSE_CH13PEN */
#define _LESENSE_ROUTEPEN_CH13PEN_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH13PEN_DEFAULT               (_LESENSE_ROUTEPEN_CH13PEN_DEFAULT << 13)   /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH14PEN                       (0x1UL << 14)                               /**< CH14 Pin Enable */
#define _LESENSE_ROUTEPEN_CH14PEN_SHIFT                14                                          /**< Shift value for LESENSE_CH14PEN */
#define _LESENSE_ROUTEPEN_CH14PEN_MASK                 0x4000UL                                    /**< Bit mask for LESENSE_CH14PEN */
#define _LESENSE_ROUTEPEN_CH14PEN_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH14PEN_DEFAULT               (_LESENSE_ROUTEPEN_CH14PEN_DEFAULT << 14)   /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH15PEN                       (0x1UL << 15)                               /**< CH15 Pin Enable */
#define _LESENSE_ROUTEPEN_CH15PEN_SHIFT                15                                          /**< Shift value for LESENSE_CH15PEN */
#define _LESENSE_ROUTEPEN_CH15PEN_MASK                 0x8000UL                                    /**< Bit mask for LESENSE_CH15PEN */
#define _LESENSE_ROUTEPEN_CH15PEN_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_CH15PEN_DEFAULT               (_LESENSE_ROUTEPEN_CH15PEN_DEFAULT << 15)   /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX0PEN                     (0x1UL << 16)                               /**< ALTEX0 Pin Enable */
#define _LESENSE_ROUTEPEN_ALTEX0PEN_SHIFT              16                                          /**< Shift value for LESENSE_ALTEX0PEN */
#define _LESENSE_ROUTEPEN_ALTEX0PEN_MASK               0x10000UL                                   /**< Bit mask for LESENSE_ALTEX0PEN */
#define _LESENSE_ROUTEPEN_ALTEX0PEN_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX0PEN_DEFAULT             (_LESENSE_ROUTEPEN_ALTEX0PEN_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX1PEN                     (0x1UL << 17)                               /**< ALTEX1 Pin Enable */
#define _LESENSE_ROUTEPEN_ALTEX1PEN_SHIFT              17                                          /**< Shift value for LESENSE_ALTEX1PEN */
#define _LESENSE_ROUTEPEN_ALTEX1PEN_MASK               0x20000UL                                   /**< Bit mask for LESENSE_ALTEX1PEN */
#define _LESENSE_ROUTEPEN_ALTEX1PEN_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX1PEN_DEFAULT             (_LESENSE_ROUTEPEN_ALTEX1PEN_DEFAULT << 17) /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX2PEN                     (0x1UL << 18)                               /**< ALTEX2 Pin Enable */
#define _LESENSE_ROUTEPEN_ALTEX2PEN_SHIFT              18                                          /**< Shift value for LESENSE_ALTEX2PEN */
#define _LESENSE_ROUTEPEN_ALTEX2PEN_MASK               0x40000UL                                   /**< Bit mask for LESENSE_ALTEX2PEN */
#define _LESENSE_ROUTEPEN_ALTEX2PEN_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX2PEN_DEFAULT             (_LESENSE_ROUTEPEN_ALTEX2PEN_DEFAULT << 18) /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX3PEN                     (0x1UL << 19)                               /**< ALTEX3 Pin Enable */
#define _LESENSE_ROUTEPEN_ALTEX3PEN_SHIFT              19                                          /**< Shift value for LESENSE_ALTEX3PEN */
#define _LESENSE_ROUTEPEN_ALTEX3PEN_MASK               0x80000UL                                   /**< Bit mask for LESENSE_ALTEX3PEN */
#define _LESENSE_ROUTEPEN_ALTEX3PEN_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX3PEN_DEFAULT             (_LESENSE_ROUTEPEN_ALTEX3PEN_DEFAULT << 19) /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX4PEN                     (0x1UL << 20)                               /**< ALTEX4 Pin Enable */
#define _LESENSE_ROUTEPEN_ALTEX4PEN_SHIFT              20                                          /**< Shift value for LESENSE_ALTEX4PEN */
#define _LESENSE_ROUTEPEN_ALTEX4PEN_MASK               0x100000UL                                  /**< Bit mask for LESENSE_ALTEX4PEN */
#define _LESENSE_ROUTEPEN_ALTEX4PEN_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX4PEN_DEFAULT             (_LESENSE_ROUTEPEN_ALTEX4PEN_DEFAULT << 20) /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX5PEN                     (0x1UL << 21)                               /**< ALTEX5 Pin Enable */
#define _LESENSE_ROUTEPEN_ALTEX5PEN_SHIFT              21                                          /**< Shift value for LESENSE_ALTEX5PEN */
#define _LESENSE_ROUTEPEN_ALTEX5PEN_MASK               0x200000UL                                  /**< Bit mask for LESENSE_ALTEX5PEN */
#define _LESENSE_ROUTEPEN_ALTEX5PEN_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX5PEN_DEFAULT             (_LESENSE_ROUTEPEN_ALTEX5PEN_DEFAULT << 21) /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX6PEN                     (0x1UL << 22)                               /**< ALTEX6 Pin Enable */
#define _LESENSE_ROUTEPEN_ALTEX6PEN_SHIFT              22                                          /**< Shift value for LESENSE_ALTEX6PEN */
#define _LESENSE_ROUTEPEN_ALTEX6PEN_MASK               0x400000UL                                  /**< Bit mask for LESENSE_ALTEX6PEN */
#define _LESENSE_ROUTEPEN_ALTEX6PEN_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX6PEN_DEFAULT             (_LESENSE_ROUTEPEN_ALTEX6PEN_DEFAULT << 22) /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX7PEN                     (0x1UL << 23)                               /**< ALTEX7 Pin Enable */
#define _LESENSE_ROUTEPEN_ALTEX7PEN_SHIFT              23                                          /**< Shift value for LESENSE_ALTEX7PEN */
#define _LESENSE_ROUTEPEN_ALTEX7PEN_MASK               0x800000UL                                  /**< Bit mask for LESENSE_ALTEX7PEN */
#define _LESENSE_ROUTEPEN_ALTEX7PEN_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_ROUTEPEN */
#define LESENSE_ROUTEPEN_ALTEX7PEN_DEFAULT             (_LESENSE_ROUTEPEN_ALTEX7PEN_DEFAULT << 23) /**< Shifted mode DEFAULT for LESENSE_ROUTEPEN */

/* Bit fields for LESENSE ST_TCONFA */
#define _LESENSE_ST_TCONFA_RESETVALUE                  0x00000000UL                                  /**< Default value for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_MASK                        0x0007DFFFUL                                  /**< Mask for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_COMP_SHIFT                  0                                             /**< Shift value for LESENSE_COMP */
#define _LESENSE_ST_TCONFA_COMP_MASK                   0xFUL                                         /**< Bit mask for LESENSE_COMP */
#define _LESENSE_ST_TCONFA_COMP_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_COMP_DEFAULT                 (_LESENSE_ST_TCONFA_COMP_DEFAULT << 0)        /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_MASK_SHIFT                  4                                             /**< Shift value for LESENSE_MASK */
#define _LESENSE_ST_TCONFA_MASK_MASK                   0xF0UL                                        /**< Bit mask for LESENSE_MASK */
#define _LESENSE_ST_TCONFA_MASK_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_MASK_DEFAULT                 (_LESENSE_ST_TCONFA_MASK_DEFAULT << 4)        /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_NEXTSTATE_SHIFT             8                                             /**< Shift value for LESENSE_NEXTSTATE */
#define _LESENSE_ST_TCONFA_NEXTSTATE_MASK              0x1F00UL                                      /**< Bit mask for LESENSE_NEXTSTATE */
#define _LESENSE_ST_TCONFA_NEXTSTATE_DEFAULT           0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_NEXTSTATE_DEFAULT            (_LESENSE_ST_TCONFA_NEXTSTATE_DEFAULT << 8)   /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_CHAIN                        (0x1UL << 14)                                 /**< Enable State Descriptor Chaining */
#define _LESENSE_ST_TCONFA_CHAIN_SHIFT                 14                                            /**< Shift value for LESENSE_CHAIN */
#define _LESENSE_ST_TCONFA_CHAIN_MASK                  0x4000UL                                      /**< Bit mask for LESENSE_CHAIN */
#define _LESENSE_ST_TCONFA_CHAIN_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_CHAIN_DEFAULT                (_LESENSE_ST_TCONFA_CHAIN_DEFAULT << 14)      /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_SETIF                        (0x1UL << 15)                                 /**< Set Interrupt Flag Enable */
#define _LESENSE_ST_TCONFA_SETIF_SHIFT                 15                                            /**< Shift value for LESENSE_SETIF */
#define _LESENSE_ST_TCONFA_SETIF_MASK                  0x8000UL                                      /**< Bit mask for LESENSE_SETIF */
#define _LESENSE_ST_TCONFA_SETIF_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_SETIF_DEFAULT                (_LESENSE_ST_TCONFA_SETIF_DEFAULT << 15)      /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define _LESENSE_ST_TCONFA_PRSACT_SHIFT                16                                            /**< Shift value for LESENSE_PRSACT */
#define _LESENSE_ST_TCONFA_PRSACT_MASK                 0x70000UL                                     /**< Bit mask for LESENSE_PRSACT */
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
#define LESENSE_ST_TCONFA_PRSACT_DEFAULT               (_LESENSE_ST_TCONFA_PRSACT_DEFAULT << 16)     /**< Shifted mode DEFAULT for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_NONE                  (_LESENSE_ST_TCONFA_PRSACT_NONE << 16)        /**< Shifted mode NONE for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_UP                    (_LESENSE_ST_TCONFA_PRSACT_UP << 16)          /**< Shifted mode UP for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS0                  (_LESENSE_ST_TCONFA_PRSACT_PRS0 << 16)        /**< Shifted mode PRS0 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS1                  (_LESENSE_ST_TCONFA_PRSACT_PRS1 << 16)        /**< Shifted mode PRS1 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_DOWN                  (_LESENSE_ST_TCONFA_PRSACT_DOWN << 16)        /**< Shifted mode DOWN for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS01                 (_LESENSE_ST_TCONFA_PRSACT_PRS01 << 16)       /**< Shifted mode PRS01 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS2                  (_LESENSE_ST_TCONFA_PRSACT_PRS2 << 16)        /**< Shifted mode PRS2 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS02                 (_LESENSE_ST_TCONFA_PRSACT_PRS02 << 16)       /**< Shifted mode PRS02 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_UPANDPRS2             (_LESENSE_ST_TCONFA_PRSACT_UPANDPRS2 << 16)   /**< Shifted mode UPANDPRS2 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS12                 (_LESENSE_ST_TCONFA_PRSACT_PRS12 << 16)       /**< Shifted mode PRS12 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_DOWNANDPRS2           (_LESENSE_ST_TCONFA_PRSACT_DOWNANDPRS2 << 16) /**< Shifted mode DOWNANDPRS2 for LESENSE_ST_TCONFA */
#define LESENSE_ST_TCONFA_PRSACT_PRS012                (_LESENSE_ST_TCONFA_PRSACT_PRS012 << 16)      /**< Shifted mode PRS012 for LESENSE_ST_TCONFA */

/* Bit fields for LESENSE ST_TCONFB */
#define _LESENSE_ST_TCONFB_RESETVALUE                  0x00000000UL                                  /**< Default value for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_MASK                        0x00079FFFUL                                  /**< Mask for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_COMP_SHIFT                  0                                             /**< Shift value for LESENSE_COMP */
#define _LESENSE_ST_TCONFB_COMP_MASK                   0xFUL                                         /**< Bit mask for LESENSE_COMP */
#define _LESENSE_ST_TCONFB_COMP_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_COMP_DEFAULT                 (_LESENSE_ST_TCONFB_COMP_DEFAULT << 0)        /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_MASK_SHIFT                  4                                             /**< Shift value for LESENSE_MASK */
#define _LESENSE_ST_TCONFB_MASK_MASK                   0xF0UL                                        /**< Bit mask for LESENSE_MASK */
#define _LESENSE_ST_TCONFB_MASK_DEFAULT                0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_MASK_DEFAULT                 (_LESENSE_ST_TCONFB_MASK_DEFAULT << 4)        /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_NEXTSTATE_SHIFT             8                                             /**< Shift value for LESENSE_NEXTSTATE */
#define _LESENSE_ST_TCONFB_NEXTSTATE_MASK              0x1F00UL                                      /**< Bit mask for LESENSE_NEXTSTATE */
#define _LESENSE_ST_TCONFB_NEXTSTATE_DEFAULT           0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_NEXTSTATE_DEFAULT            (_LESENSE_ST_TCONFB_NEXTSTATE_DEFAULT << 8)   /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_SETIF                        (0x1UL << 15)                                 /**< Set Interrupt Flag */
#define _LESENSE_ST_TCONFB_SETIF_SHIFT                 15                                            /**< Shift value for LESENSE_SETIF */
#define _LESENSE_ST_TCONFB_SETIF_MASK                  0x8000UL                                      /**< Bit mask for LESENSE_SETIF */
#define _LESENSE_ST_TCONFB_SETIF_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_SETIF_DEFAULT                (_LESENSE_ST_TCONFB_SETIF_DEFAULT << 15)      /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define _LESENSE_ST_TCONFB_PRSACT_SHIFT                16                                            /**< Shift value for LESENSE_PRSACT */
#define _LESENSE_ST_TCONFB_PRSACT_MASK                 0x70000UL                                     /**< Bit mask for LESENSE_PRSACT */
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
#define LESENSE_ST_TCONFB_PRSACT_DEFAULT               (_LESENSE_ST_TCONFB_PRSACT_DEFAULT << 16)     /**< Shifted mode DEFAULT for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_NONE                  (_LESENSE_ST_TCONFB_PRSACT_NONE << 16)        /**< Shifted mode NONE for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_UP                    (_LESENSE_ST_TCONFB_PRSACT_UP << 16)          /**< Shifted mode UP for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS0                  (_LESENSE_ST_TCONFB_PRSACT_PRS0 << 16)        /**< Shifted mode PRS0 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS1                  (_LESENSE_ST_TCONFB_PRSACT_PRS1 << 16)        /**< Shifted mode PRS1 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_DOWN                  (_LESENSE_ST_TCONFB_PRSACT_DOWN << 16)        /**< Shifted mode DOWN for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS01                 (_LESENSE_ST_TCONFB_PRSACT_PRS01 << 16)       /**< Shifted mode PRS01 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS2                  (_LESENSE_ST_TCONFB_PRSACT_PRS2 << 16)        /**< Shifted mode PRS2 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS02                 (_LESENSE_ST_TCONFB_PRSACT_PRS02 << 16)       /**< Shifted mode PRS02 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_UPANDPRS2             (_LESENSE_ST_TCONFB_PRSACT_UPANDPRS2 << 16)   /**< Shifted mode UPANDPRS2 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS12                 (_LESENSE_ST_TCONFB_PRSACT_PRS12 << 16)       /**< Shifted mode PRS12 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_DOWNANDPRS2           (_LESENSE_ST_TCONFB_PRSACT_DOWNANDPRS2 << 16) /**< Shifted mode DOWNANDPRS2 for LESENSE_ST_TCONFB */
#define LESENSE_ST_TCONFB_PRSACT_PRS012                (_LESENSE_ST_TCONFB_PRSACT_PRS012 << 16)      /**< Shifted mode PRS012 for LESENSE_ST_TCONFB */

/* Bit fields for LESENSE BUF_DATA */
#define _LESENSE_BUF_DATA_RESETVALUE                   0x00000000UL                              /**< Default value for LESENSE_BUF_DATA */
#define _LESENSE_BUF_DATA_MASK                         0x000FFFFFUL                              /**< Mask for LESENSE_BUF_DATA */
#define _LESENSE_BUF_DATA_DATA_SHIFT                   0                                         /**< Shift value for LESENSE_DATA */
#define _LESENSE_BUF_DATA_DATA_MASK                    0xFFFFUL                                  /**< Bit mask for LESENSE_DATA */
#define _LESENSE_BUF_DATA_DATA_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for LESENSE_BUF_DATA */
#define LESENSE_BUF_DATA_DATA_DEFAULT                  (_LESENSE_BUF_DATA_DATA_DEFAULT << 0)     /**< Shifted mode DEFAULT for LESENSE_BUF_DATA */
#define _LESENSE_BUF_DATA_DATASRC_SHIFT                16                                        /**< Shift value for LESENSE_DATASRC */
#define _LESENSE_BUF_DATA_DATASRC_MASK                 0xF0000UL                                 /**< Bit mask for LESENSE_DATASRC */
#define _LESENSE_BUF_DATA_DATASRC_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for LESENSE_BUF_DATA */
#define LESENSE_BUF_DATA_DATASRC_DEFAULT               (_LESENSE_BUF_DATA_DATASRC_DEFAULT << 16) /**< Shifted mode DEFAULT for LESENSE_BUF_DATA */

/* Bit fields for LESENSE CH_TIMING */
#define _LESENSE_CH_TIMING_RESETVALUE                  0x00000000UL                                  /**< Default value for LESENSE_CH_TIMING */
#define _LESENSE_CH_TIMING_MASK                        0x00FFFFFFUL                                  /**< Mask for LESENSE_CH_TIMING */
#define _LESENSE_CH_TIMING_EXTIME_SHIFT                0                                             /**< Shift value for LESENSE_EXTIME */
#define _LESENSE_CH_TIMING_EXTIME_MASK                 0x3FUL                                        /**< Bit mask for LESENSE_EXTIME */
#define _LESENSE_CH_TIMING_EXTIME_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for LESENSE_CH_TIMING */
#define LESENSE_CH_TIMING_EXTIME_DEFAULT               (_LESENSE_CH_TIMING_EXTIME_DEFAULT << 0)      /**< Shifted mode DEFAULT for LESENSE_CH_TIMING */
#define _LESENSE_CH_TIMING_SAMPLEDLY_SHIFT             6                                             /**< Shift value for LESENSE_SAMPLEDLY */
#define _LESENSE_CH_TIMING_SAMPLEDLY_MASK              0x3FC0UL                                      /**< Bit mask for LESENSE_SAMPLEDLY */
#define _LESENSE_CH_TIMING_SAMPLEDLY_DEFAULT           0x00000000UL                                  /**< Mode DEFAULT for LESENSE_CH_TIMING */
#define LESENSE_CH_TIMING_SAMPLEDLY_DEFAULT            (_LESENSE_CH_TIMING_SAMPLEDLY_DEFAULT << 6)   /**< Shifted mode DEFAULT for LESENSE_CH_TIMING */
#define _LESENSE_CH_TIMING_MEASUREDLY_SHIFT            14                                            /**< Shift value for LESENSE_MEASUREDLY */
#define _LESENSE_CH_TIMING_MEASUREDLY_MASK             0xFFC000UL                                    /**< Bit mask for LESENSE_MEASUREDLY */
#define _LESENSE_CH_TIMING_MEASUREDLY_DEFAULT          0x00000000UL                                  /**< Mode DEFAULT for LESENSE_CH_TIMING */
#define LESENSE_CH_TIMING_MEASUREDLY_DEFAULT           (_LESENSE_CH_TIMING_MEASUREDLY_DEFAULT << 14) /**< Shifted mode DEFAULT for LESENSE_CH_TIMING */

/* Bit fields for LESENSE CH_INTERACT */
#define _LESENSE_CH_INTERACT_RESETVALUE                0x00000000UL                                    /**< Default value for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_MASK                      0x003FFFFFUL                                    /**< Mask for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_THRES_SHIFT               0                                               /**< Shift value for LESENSE_THRES */
#define _LESENSE_CH_INTERACT_THRES_MASK                0xFFFUL                                         /**< Bit mask for LESENSE_THRES */
#define _LESENSE_CH_INTERACT_THRES_DEFAULT             0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_THRES_DEFAULT              (_LESENSE_CH_INTERACT_THRES_DEFAULT << 0)       /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLE_SHIFT              12                                              /**< Shift value for LESENSE_SAMPLE */
#define _LESENSE_CH_INTERACT_SAMPLE_MASK               0x3000UL                                        /**< Bit mask for LESENSE_SAMPLE */
#define _LESENSE_CH_INTERACT_SAMPLE_DEFAULT            0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLE_ACMPCOUNT          0x00000000UL                                    /**< Mode ACMPCOUNT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLE_ACMP               0x00000001UL                                    /**< Mode ACMP for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLE_ADC                0x00000002UL                                    /**< Mode ADC for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLE_ADCDIFF            0x00000003UL                                    /**< Mode ADCDIFF for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE_DEFAULT             (_LESENSE_CH_INTERACT_SAMPLE_DEFAULT << 12)     /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE_ACMPCOUNT           (_LESENSE_CH_INTERACT_SAMPLE_ACMPCOUNT << 12)   /**< Shifted mode ACMPCOUNT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE_ACMP                (_LESENSE_CH_INTERACT_SAMPLE_ACMP << 12)        /**< Shifted mode ACMP for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE_ADC                 (_LESENSE_CH_INTERACT_SAMPLE_ADC << 12)         /**< Shifted mode ADC for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLE_ADCDIFF             (_LESENSE_CH_INTERACT_SAMPLE_ADCDIFF << 12)     /**< Shifted mode ADCDIFF for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_SHIFT               14                                              /**< Shift value for LESENSE_SETIF */
#define _LESENSE_CH_INTERACT_SETIF_MASK                0x1C000UL                                       /**< Bit mask for LESENSE_SETIF */
#define _LESENSE_CH_INTERACT_SETIF_DEFAULT             0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_NONE                0x00000000UL                                    /**< Mode NONE for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_LEVEL               0x00000001UL                                    /**< Mode LEVEL for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_POSEDGE             0x00000002UL                                    /**< Mode POSEDGE for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_NEGEDGE             0x00000003UL                                    /**< Mode NEGEDGE for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SETIF_BOTHEDGES           0x00000004UL                                    /**< Mode BOTHEDGES for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_DEFAULT              (_LESENSE_CH_INTERACT_SETIF_DEFAULT << 14)      /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_NONE                 (_LESENSE_CH_INTERACT_SETIF_NONE << 14)         /**< Shifted mode NONE for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_LEVEL                (_LESENSE_CH_INTERACT_SETIF_LEVEL << 14)        /**< Shifted mode LEVEL for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_POSEDGE              (_LESENSE_CH_INTERACT_SETIF_POSEDGE << 14)      /**< Shifted mode POSEDGE for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_NEGEDGE              (_LESENSE_CH_INTERACT_SETIF_NEGEDGE << 14)      /**< Shifted mode NEGEDGE for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SETIF_BOTHEDGES            (_LESENSE_CH_INTERACT_SETIF_BOTHEDGES << 14)    /**< Shifted mode BOTHEDGES for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_SHIFT              17                                              /**< Shift value for LESENSE_EXMODE */
#define _LESENSE_CH_INTERACT_EXMODE_MASK               0x60000UL                                       /**< Bit mask for LESENSE_EXMODE */
#define _LESENSE_CH_INTERACT_EXMODE_DEFAULT            0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_DISABLE            0x00000000UL                                    /**< Mode DISABLE for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_HIGH               0x00000001UL                                    /**< Mode HIGH for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_LOW                0x00000002UL                                    /**< Mode LOW for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXMODE_DACOUT             0x00000003UL                                    /**< Mode DACOUT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_DEFAULT             (_LESENSE_CH_INTERACT_EXMODE_DEFAULT << 17)     /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_DISABLE             (_LESENSE_CH_INTERACT_EXMODE_DISABLE << 17)     /**< Shifted mode DISABLE for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_HIGH                (_LESENSE_CH_INTERACT_EXMODE_HIGH << 17)        /**< Shifted mode HIGH for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_LOW                 (_LESENSE_CH_INTERACT_EXMODE_LOW << 17)         /**< Shifted mode LOW for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXMODE_DACOUT              (_LESENSE_CH_INTERACT_EXMODE_DACOUT << 17)      /**< Shifted mode DACOUT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXCLK                      (0x1UL << 19)                                   /**< Select Clock Used for Excitation Timing */
#define _LESENSE_CH_INTERACT_EXCLK_SHIFT               19                                              /**< Shift value for LESENSE_EXCLK */
#define _LESENSE_CH_INTERACT_EXCLK_MASK                0x80000UL                                       /**< Bit mask for LESENSE_EXCLK */
#define _LESENSE_CH_INTERACT_EXCLK_DEFAULT             0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXCLK_LFACLK              0x00000000UL                                    /**< Mode LFACLK for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_EXCLK_AUXHFRCO            0x00000001UL                                    /**< Mode AUXHFRCO for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXCLK_DEFAULT              (_LESENSE_CH_INTERACT_EXCLK_DEFAULT << 19)      /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXCLK_LFACLK               (_LESENSE_CH_INTERACT_EXCLK_LFACLK << 19)       /**< Shifted mode LFACLK for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_EXCLK_AUXHFRCO             (_LESENSE_CH_INTERACT_EXCLK_AUXHFRCO << 19)     /**< Shifted mode AUXHFRCO for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLECLK                  (0x1UL << 20)                                   /**< Select Clock Used for Timing of Sample Delay */
#define _LESENSE_CH_INTERACT_SAMPLECLK_SHIFT           20                                              /**< Shift value for LESENSE_SAMPLECLK */
#define _LESENSE_CH_INTERACT_SAMPLECLK_MASK            0x100000UL                                      /**< Bit mask for LESENSE_SAMPLECLK */
#define _LESENSE_CH_INTERACT_SAMPLECLK_DEFAULT         0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLECLK_LFACLK          0x00000000UL                                    /**< Mode LFACLK for LESENSE_CH_INTERACT */
#define _LESENSE_CH_INTERACT_SAMPLECLK_AUXHFRCO        0x00000001UL                                    /**< Mode AUXHFRCO for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLECLK_DEFAULT          (_LESENSE_CH_INTERACT_SAMPLECLK_DEFAULT << 20)  /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLECLK_LFACLK           (_LESENSE_CH_INTERACT_SAMPLECLK_LFACLK << 20)   /**< Shifted mode LFACLK for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_SAMPLECLK_AUXHFRCO         (_LESENSE_CH_INTERACT_SAMPLECLK_AUXHFRCO << 20) /**< Shifted mode AUXHFRCO for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_ALTEX                      (0x1UL << 21)                                   /**< Use Alternative Excite Pin */
#define _LESENSE_CH_INTERACT_ALTEX_SHIFT               21                                              /**< Shift value for LESENSE_ALTEX */
#define _LESENSE_CH_INTERACT_ALTEX_MASK                0x200000UL                                      /**< Bit mask for LESENSE_ALTEX */
#define _LESENSE_CH_INTERACT_ALTEX_DEFAULT             0x00000000UL                                    /**< Mode DEFAULT for LESENSE_CH_INTERACT */
#define LESENSE_CH_INTERACT_ALTEX_DEFAULT              (_LESENSE_CH_INTERACT_ALTEX_DEFAULT << 21)      /**< Shifted mode DEFAULT for LESENSE_CH_INTERACT */

/* Bit fields for LESENSE CH_EVAL */
#define _LESENSE_CH_EVAL_RESETVALUE                    0x00000000UL                                /**< Default value for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_MASK                          0x007FFFFFUL                                /**< Mask for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_COMPTHRES_SHIFT               0                                           /**< Shift value for LESENSE_COMPTHRES */
#define _LESENSE_CH_EVAL_COMPTHRES_MASK                0xFFFFUL                                    /**< Bit mask for LESENSE_COMPTHRES */
#define _LESENSE_CH_EVAL_COMPTHRES_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMPTHRES_DEFAULT              (_LESENSE_CH_EVAL_COMPTHRES_DEFAULT << 0)   /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMP                           (0x1UL << 16)                               /**< Select Mode for Threshold Comparison */
#define _LESENSE_CH_EVAL_COMP_SHIFT                    16                                          /**< Shift value for LESENSE_COMP */
#define _LESENSE_CH_EVAL_COMP_MASK                     0x10000UL                                   /**< Bit mask for LESENSE_COMP */
#define _LESENSE_CH_EVAL_COMP_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_COMP_LESS                     0x00000000UL                                /**< Mode LESS for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_COMP_GE                       0x00000001UL                                /**< Mode GE for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMP_DEFAULT                   (_LESENSE_CH_EVAL_COMP_DEFAULT << 16)       /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMP_LESS                      (_LESENSE_CH_EVAL_COMP_LESS << 16)          /**< Shifted mode LESS for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_COMP_GE                        (_LESENSE_CH_EVAL_COMP_GE << 16)            /**< Shifted mode GE for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_DECODE                         (0x1UL << 17)                               /**< Send Result to Decoder */
#define _LESENSE_CH_EVAL_DECODE_SHIFT                  17                                          /**< Shift value for LESENSE_DECODE */
#define _LESENSE_CH_EVAL_DECODE_MASK                   0x20000UL                                   /**< Bit mask for LESENSE_DECODE */
#define _LESENSE_CH_EVAL_DECODE_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_DECODE_DEFAULT                 (_LESENSE_CH_EVAL_DECODE_DEFAULT << 17)     /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_STRSAMPLE_SHIFT               18                                          /**< Shift value for LESENSE_STRSAMPLE */
#define _LESENSE_CH_EVAL_STRSAMPLE_MASK                0xC0000UL                                   /**< Bit mask for LESENSE_STRSAMPLE */
#define _LESENSE_CH_EVAL_STRSAMPLE_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_STRSAMPLE_DISABLE             0x00000000UL                                /**< Mode DISABLE for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_STRSAMPLE_DATA                0x00000001UL                                /**< Mode DATA for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_STRSAMPLE_DATASRC             0x00000002UL                                /**< Mode DATASRC for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_STRSAMPLE_DEFAULT              (_LESENSE_CH_EVAL_STRSAMPLE_DEFAULT << 18)  /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_STRSAMPLE_DISABLE              (_LESENSE_CH_EVAL_STRSAMPLE_DISABLE << 18)  /**< Shifted mode DISABLE for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_STRSAMPLE_DATA                 (_LESENSE_CH_EVAL_STRSAMPLE_DATA << 18)     /**< Shifted mode DATA for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_STRSAMPLE_DATASRC              (_LESENSE_CH_EVAL_STRSAMPLE_DATASRC << 18)  /**< Shifted mode DATASRC for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_SCANRESINV                     (0x1UL << 20)                               /**< Enable Inversion of Result */
#define _LESENSE_CH_EVAL_SCANRESINV_SHIFT              20                                          /**< Shift value for LESENSE_SCANRESINV */
#define _LESENSE_CH_EVAL_SCANRESINV_MASK               0x100000UL                                  /**< Bit mask for LESENSE_SCANRESINV */
#define _LESENSE_CH_EVAL_SCANRESINV_DEFAULT            0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_SCANRESINV_DEFAULT             (_LESENSE_CH_EVAL_SCANRESINV_DEFAULT << 20) /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_MODE_SHIFT                    21                                          /**< Shift value for LESENSE_MODE */
#define _LESENSE_CH_EVAL_MODE_MASK                     0x600000UL                                  /**< Bit mask for LESENSE_MODE */
#define _LESENSE_CH_EVAL_MODE_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_MODE_THRES                    0x00000000UL                                /**< Mode THRES for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_MODE_SLIDINGWIN               0x00000001UL                                /**< Mode SLIDINGWIN for LESENSE_CH_EVAL */
#define _LESENSE_CH_EVAL_MODE_STEPDET                  0x00000002UL                                /**< Mode STEPDET for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_MODE_DEFAULT                   (_LESENSE_CH_EVAL_MODE_DEFAULT << 21)       /**< Shifted mode DEFAULT for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_MODE_THRES                     (_LESENSE_CH_EVAL_MODE_THRES << 21)         /**< Shifted mode THRES for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_MODE_SLIDINGWIN                (_LESENSE_CH_EVAL_MODE_SLIDINGWIN << 21)    /**< Shifted mode SLIDINGWIN for LESENSE_CH_EVAL */
#define LESENSE_CH_EVAL_MODE_STEPDET                   (_LESENSE_CH_EVAL_MODE_STEPDET << 21)       /**< Shifted mode STEPDET for LESENSE_CH_EVAL */

/** @} */
/** @} End of group EFR32MG12P_LESENSE */
/** @} End of group Parts */
