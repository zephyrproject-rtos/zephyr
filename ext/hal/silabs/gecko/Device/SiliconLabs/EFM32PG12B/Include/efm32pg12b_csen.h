/**************************************************************************//**
 * @file efm32pg12b_csen.h
 * @brief EFM32PG12B_CSEN register and bit field definitions
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
 * @defgroup EFM32PG12B_CSEN CSEN
 * @{
 * @brief EFM32PG12B_CSEN Register Declaration
 *****************************************************************************/
/** CSEN Register Declaration */
typedef struct {
  __IOM uint32_t CTRL;          /**< Control  */
  __IOM uint32_t TIMCTRL;       /**< Timing Control  */
  __IOM uint32_t CMD;           /**< Command  */
  __IM uint32_t  STATUS;        /**< Status  */
  __IOM uint32_t PRSSEL;        /**< PRS Select  */
  __IOM uint32_t DATA;          /**< Output Data  */
  __IOM uint32_t SCANMASK0;     /**< Scan Channel Mask 0  */
  __IOM uint32_t SCANINPUTSEL0; /**< Scan Input Selection 0  */
  __IOM uint32_t SCANMASK1;     /**< Scan Channel Mask 1  */
  __IOM uint32_t SCANINPUTSEL1; /**< Scan Input Selection 1  */
  __IM uint32_t  APORTREQ;      /**< APORT Request Status  */
  __IM uint32_t  APORTCONFLICT; /**< APORT Request Conflict  */
  __IOM uint32_t CMPTHR;        /**< Comparator Threshold  */
  __IOM uint32_t EMA;           /**< Exponential Moving Average  */
  __IOM uint32_t EMACTRL;       /**< Exponential Moving Average Control  */
  __IOM uint32_t SINGLECTRL;    /**< Single Conversion Control  */
  __IOM uint32_t DMBASELINE;    /**< Delta Modulation Baseline  */
  __IOM uint32_t DMCFG;         /**< Delta Modulation Configuration  */
  __IOM uint32_t ANACTRL;       /**< Analog Control  */

  uint32_t       RESERVED0[2U]; /**< Reserved for future use **/
  __IM uint32_t  IF;            /**< Interrupt Flag  */
  __IOM uint32_t IFS;           /**< Interrupt Flag Set  */
  __IOM uint32_t IFC;           /**< Interrupt Flag Clear  */
  __IOM uint32_t IEN;           /**< Interrupt Enable  */
} CSEN_TypeDef;                 /** @} */

/**************************************************************************//**
 * @addtogroup EFM32PG12B_CSEN
 * @{
 * @defgroup EFM32PG12B_CSEN_BitFields  CSEN Bit Fields
 * @{
 *****************************************************************************/

/* Bit fields for CSEN CTRL */
#define _CSEN_CTRL_RESETVALUE                                0x00030000UL                               /**< Default value for CSEN_CTRL */
#define _CSEN_CTRL_MASK                                      0x1FFFF336UL                               /**< Mask for CSEN_CTRL */
#define CSEN_CTRL_EN                                         (0x1UL << 1)                               /**< CSEN Enable */
#define _CSEN_CTRL_EN_SHIFT                                  1                                          /**< Shift value for CSEN_EN */
#define _CSEN_CTRL_EN_MASK                                   0x2UL                                      /**< Bit mask for CSEN_EN */
#define _CSEN_CTRL_EN_DEFAULT                                0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_EN_DISABLE                                0x00000000UL                               /**< Mode DISABLE for CSEN_CTRL */
#define _CSEN_CTRL_EN_ENABLE                                 0x00000001UL                               /**< Mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_EN_DEFAULT                                 (_CSEN_CTRL_EN_DEFAULT << 1)               /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_EN_DISABLE                                 (_CSEN_CTRL_EN_DISABLE << 1)               /**< Shifted mode DISABLE for CSEN_CTRL */
#define CSEN_CTRL_EN_ENABLE                                  (_CSEN_CTRL_EN_ENABLE << 1)                /**< Shifted mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_CMPPOL                                     (0x1UL << 2)                               /**< CSEN Digital Comparator Polarity Select */
#define _CSEN_CTRL_CMPPOL_SHIFT                              2                                          /**< Shift value for CSEN_CMPPOL */
#define _CSEN_CTRL_CMPPOL_MASK                               0x4UL                                      /**< Bit mask for CSEN_CMPPOL */
#define _CSEN_CTRL_CMPPOL_DEFAULT                            0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_CMPPOL_GT                                 0x00000000UL                               /**< Mode GT for CSEN_CTRL */
#define _CSEN_CTRL_CMPPOL_LTE                                0x00000001UL                               /**< Mode LTE for CSEN_CTRL */
#define CSEN_CTRL_CMPPOL_DEFAULT                             (_CSEN_CTRL_CMPPOL_DEFAULT << 2)           /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_CMPPOL_GT                                  (_CSEN_CTRL_CMPPOL_GT << 2)                /**< Shifted mode GT for CSEN_CTRL */
#define CSEN_CTRL_CMPPOL_LTE                                 (_CSEN_CTRL_CMPPOL_LTE << 2)               /**< Shifted mode LTE for CSEN_CTRL */
#define _CSEN_CTRL_CM_SHIFT                                  4                                          /**< Shift value for CSEN_CM */
#define _CSEN_CTRL_CM_MASK                                   0x30UL                                     /**< Bit mask for CSEN_CM */
#define _CSEN_CTRL_CM_DEFAULT                                0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_CM_SGL                                    0x00000000UL                               /**< Mode SGL for CSEN_CTRL */
#define _CSEN_CTRL_CM_SCAN                                   0x00000001UL                               /**< Mode SCAN for CSEN_CTRL */
#define _CSEN_CTRL_CM_CONTSGL                                0x00000002UL                               /**< Mode CONTSGL for CSEN_CTRL */
#define _CSEN_CTRL_CM_CONTSCAN                               0x00000003UL                               /**< Mode CONTSCAN for CSEN_CTRL */
#define CSEN_CTRL_CM_DEFAULT                                 (_CSEN_CTRL_CM_DEFAULT << 4)               /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_CM_SGL                                     (_CSEN_CTRL_CM_SGL << 4)                   /**< Shifted mode SGL for CSEN_CTRL */
#define CSEN_CTRL_CM_SCAN                                    (_CSEN_CTRL_CM_SCAN << 4)                  /**< Shifted mode SCAN for CSEN_CTRL */
#define CSEN_CTRL_CM_CONTSGL                                 (_CSEN_CTRL_CM_CONTSGL << 4)               /**< Shifted mode CONTSGL for CSEN_CTRL */
#define CSEN_CTRL_CM_CONTSCAN                                (_CSEN_CTRL_CM_CONTSCAN << 4)              /**< Shifted mode CONTSCAN for CSEN_CTRL */
#define _CSEN_CTRL_SARCR_SHIFT                               8                                          /**< Shift value for CSEN_SARCR */
#define _CSEN_CTRL_SARCR_MASK                                0x300UL                                    /**< Bit mask for CSEN_SARCR */
#define _CSEN_CTRL_SARCR_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_SARCR_CLK10                               0x00000000UL                               /**< Mode CLK10 for CSEN_CTRL */
#define _CSEN_CTRL_SARCR_CLK12                               0x00000001UL                               /**< Mode CLK12 for CSEN_CTRL */
#define _CSEN_CTRL_SARCR_CLK14                               0x00000002UL                               /**< Mode CLK14 for CSEN_CTRL */
#define _CSEN_CTRL_SARCR_CLK16                               0x00000003UL                               /**< Mode CLK16 for CSEN_CTRL */
#define CSEN_CTRL_SARCR_DEFAULT                              (_CSEN_CTRL_SARCR_DEFAULT << 8)            /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_SARCR_CLK10                                (_CSEN_CTRL_SARCR_CLK10 << 8)              /**< Shifted mode CLK10 for CSEN_CTRL */
#define CSEN_CTRL_SARCR_CLK12                                (_CSEN_CTRL_SARCR_CLK12 << 8)              /**< Shifted mode CLK12 for CSEN_CTRL */
#define CSEN_CTRL_SARCR_CLK14                                (_CSEN_CTRL_SARCR_CLK14 << 8)              /**< Shifted mode CLK14 for CSEN_CTRL */
#define CSEN_CTRL_SARCR_CLK16                                (_CSEN_CTRL_SARCR_CLK16 << 8)              /**< Shifted mode CLK16 for CSEN_CTRL */
#define _CSEN_CTRL_ACU_SHIFT                                 12                                         /**< Shift value for CSEN_ACU */
#define _CSEN_CTRL_ACU_MASK                                  0x7000UL                                   /**< Bit mask for CSEN_ACU */
#define _CSEN_CTRL_ACU_DEFAULT                               0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_ACU_ACC1                                  0x00000000UL                               /**< Mode ACC1 for CSEN_CTRL */
#define _CSEN_CTRL_ACU_ACC2                                  0x00000001UL                               /**< Mode ACC2 for CSEN_CTRL */
#define _CSEN_CTRL_ACU_ACC4                                  0x00000002UL                               /**< Mode ACC4 for CSEN_CTRL */
#define _CSEN_CTRL_ACU_ACC8                                  0x00000003UL                               /**< Mode ACC8 for CSEN_CTRL */
#define _CSEN_CTRL_ACU_ACC16                                 0x00000004UL                               /**< Mode ACC16 for CSEN_CTRL */
#define _CSEN_CTRL_ACU_ACC32                                 0x00000005UL                               /**< Mode ACC32 for CSEN_CTRL */
#define _CSEN_CTRL_ACU_ACC64                                 0x00000006UL                               /**< Mode ACC64 for CSEN_CTRL */
#define CSEN_CTRL_ACU_DEFAULT                                (_CSEN_CTRL_ACU_DEFAULT << 12)             /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_ACU_ACC1                                   (_CSEN_CTRL_ACU_ACC1 << 12)                /**< Shifted mode ACC1 for CSEN_CTRL */
#define CSEN_CTRL_ACU_ACC2                                   (_CSEN_CTRL_ACU_ACC2 << 12)                /**< Shifted mode ACC2 for CSEN_CTRL */
#define CSEN_CTRL_ACU_ACC4                                   (_CSEN_CTRL_ACU_ACC4 << 12)                /**< Shifted mode ACC4 for CSEN_CTRL */
#define CSEN_CTRL_ACU_ACC8                                   (_CSEN_CTRL_ACU_ACC8 << 12)                /**< Shifted mode ACC8 for CSEN_CTRL */
#define CSEN_CTRL_ACU_ACC16                                  (_CSEN_CTRL_ACU_ACC16 << 12)               /**< Shifted mode ACC16 for CSEN_CTRL */
#define CSEN_CTRL_ACU_ACC32                                  (_CSEN_CTRL_ACU_ACC32 << 12)               /**< Shifted mode ACC32 for CSEN_CTRL */
#define CSEN_CTRL_ACU_ACC64                                  (_CSEN_CTRL_ACU_ACC64 << 12)               /**< Shifted mode ACC64 for CSEN_CTRL */
#define CSEN_CTRL_MCEN                                       (0x1UL << 15)                              /**< CSEN Multiple Channel Enable */
#define _CSEN_CTRL_MCEN_SHIFT                                15                                         /**< Shift value for CSEN_MCEN */
#define _CSEN_CTRL_MCEN_MASK                                 0x8000UL                                   /**< Bit mask for CSEN_MCEN */
#define _CSEN_CTRL_MCEN_DEFAULT                              0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_MCEN_DISABLE                              0x00000000UL                               /**< Mode DISABLE for CSEN_CTRL */
#define _CSEN_CTRL_MCEN_ENABLE                               0x00000001UL                               /**< Mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_MCEN_DEFAULT                               (_CSEN_CTRL_MCEN_DEFAULT << 15)            /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_MCEN_DISABLE                               (_CSEN_CTRL_MCEN_DISABLE << 15)            /**< Shifted mode DISABLE for CSEN_CTRL */
#define CSEN_CTRL_MCEN_ENABLE                                (_CSEN_CTRL_MCEN_ENABLE << 15)             /**< Shifted mode ENABLE for CSEN_CTRL */
#define _CSEN_CTRL_STM_SHIFT                                 16                                         /**< Shift value for CSEN_STM */
#define _CSEN_CTRL_STM_MASK                                  0x30000UL                                  /**< Bit mask for CSEN_STM */
#define _CSEN_CTRL_STM_PRS                                   0x00000000UL                               /**< Mode PRS for CSEN_CTRL */
#define _CSEN_CTRL_STM_TIMER                                 0x00000001UL                               /**< Mode TIMER for CSEN_CTRL */
#define _CSEN_CTRL_STM_START                                 0x00000002UL                               /**< Mode START for CSEN_CTRL */
#define _CSEN_CTRL_STM_DEFAULT                               0x00000003UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_STM_DEFAULT                               0x00000003UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_STM_PRS                                    (_CSEN_CTRL_STM_PRS << 16)                 /**< Shifted mode PRS for CSEN_CTRL */
#define CSEN_CTRL_STM_TIMER                                  (_CSEN_CTRL_STM_TIMER << 16)               /**< Shifted mode TIMER for CSEN_CTRL */
#define CSEN_CTRL_STM_START                                  (_CSEN_CTRL_STM_START << 16)               /**< Shifted mode START for CSEN_CTRL */
#define CSEN_CTRL_STM_DEFAULT                                (_CSEN_CTRL_STM_DEFAULT << 16)             /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_STM_DEFAULT                                (_CSEN_CTRL_STM_DEFAULT << 16)             /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_CMPEN                                      (0x1UL << 18)                              /**< CSEN Digital Comparator Enable */
#define _CSEN_CTRL_CMPEN_SHIFT                               18                                         /**< Shift value for CSEN_CMPEN */
#define _CSEN_CTRL_CMPEN_MASK                                0x40000UL                                  /**< Bit mask for CSEN_CMPEN */
#define _CSEN_CTRL_CMPEN_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_CMPEN_DISABLE                             0x00000000UL                               /**< Mode DISABLE for CSEN_CTRL */
#define _CSEN_CTRL_CMPEN_ENABLE                              0x00000001UL                               /**< Mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_CMPEN_DEFAULT                              (_CSEN_CTRL_CMPEN_DEFAULT << 18)           /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_CMPEN_DISABLE                              (_CSEN_CTRL_CMPEN_DISABLE << 18)           /**< Shifted mode DISABLE for CSEN_CTRL */
#define CSEN_CTRL_CMPEN_ENABLE                               (_CSEN_CTRL_CMPEN_ENABLE << 18)            /**< Shifted mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_DRSF                                       (0x1UL << 19)                              /**< CSEN Disable Right-Shift */
#define _CSEN_CTRL_DRSF_SHIFT                                19                                         /**< Shift value for CSEN_DRSF */
#define _CSEN_CTRL_DRSF_MASK                                 0x80000UL                                  /**< Bit mask for CSEN_DRSF */
#define _CSEN_CTRL_DRSF_DEFAULT                              0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_DRSF_DISABLE                              0x00000000UL                               /**< Mode DISABLE for CSEN_CTRL */
#define _CSEN_CTRL_DRSF_ENABLE                               0x00000001UL                               /**< Mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_DRSF_DEFAULT                               (_CSEN_CTRL_DRSF_DEFAULT << 19)            /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_DRSF_DISABLE                               (_CSEN_CTRL_DRSF_DISABLE << 19)            /**< Shifted mode DISABLE for CSEN_CTRL */
#define CSEN_CTRL_DRSF_ENABLE                                (_CSEN_CTRL_DRSF_ENABLE << 19)             /**< Shifted mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_DMAEN                                      (0x1UL << 20)                              /**< CSEN DMA Enable Bit */
#define _CSEN_CTRL_DMAEN_SHIFT                               20                                         /**< Shift value for CSEN_DMAEN */
#define _CSEN_CTRL_DMAEN_MASK                                0x100000UL                                 /**< Bit mask for CSEN_DMAEN */
#define _CSEN_CTRL_DMAEN_DEFAULT                             0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_DMAEN_DISABLE                             0x00000000UL                               /**< Mode DISABLE for CSEN_CTRL */
#define _CSEN_CTRL_DMAEN_ENABLE                              0x00000001UL                               /**< Mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_DMAEN_DEFAULT                              (_CSEN_CTRL_DMAEN_DEFAULT << 20)           /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_DMAEN_DISABLE                              (_CSEN_CTRL_DMAEN_DISABLE << 20)           /**< Shifted mode DISABLE for CSEN_CTRL */
#define CSEN_CTRL_DMAEN_ENABLE                               (_CSEN_CTRL_DMAEN_ENABLE << 20)            /**< Shifted mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_CONVSEL                                    (0x1UL << 21)                              /**< CSEN Converter Select */
#define _CSEN_CTRL_CONVSEL_SHIFT                             21                                         /**< Shift value for CSEN_CONVSEL */
#define _CSEN_CTRL_CONVSEL_MASK                              0x200000UL                                 /**< Bit mask for CSEN_CONVSEL */
#define _CSEN_CTRL_CONVSEL_DEFAULT                           0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_CONVSEL_SAR                               0x00000000UL                               /**< Mode SAR for CSEN_CTRL */
#define _CSEN_CTRL_CONVSEL_DM                                0x00000001UL                               /**< Mode DM for CSEN_CTRL */
#define CSEN_CTRL_CONVSEL_DEFAULT                            (_CSEN_CTRL_CONVSEL_DEFAULT << 21)         /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_CONVSEL_SAR                                (_CSEN_CTRL_CONVSEL_SAR << 21)             /**< Shifted mode SAR for CSEN_CTRL */
#define CSEN_CTRL_CONVSEL_DM                                 (_CSEN_CTRL_CONVSEL_DM << 21)              /**< Shifted mode DM for CSEN_CTRL */
#define CSEN_CTRL_CHOPEN                                     (0x1UL << 22)                              /**< CSEN Chop Enable */
#define _CSEN_CTRL_CHOPEN_SHIFT                              22                                         /**< Shift value for CSEN_CHOPEN */
#define _CSEN_CTRL_CHOPEN_MASK                               0x400000UL                                 /**< Bit mask for CSEN_CHOPEN */
#define _CSEN_CTRL_CHOPEN_DEFAULT                            0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_CHOPEN_DISABLE                            0x00000000UL                               /**< Mode DISABLE for CSEN_CTRL */
#define _CSEN_CTRL_CHOPEN_ENABLE                             0x00000001UL                               /**< Mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_CHOPEN_DEFAULT                             (_CSEN_CTRL_CHOPEN_DEFAULT << 22)          /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_CHOPEN_DISABLE                             (_CSEN_CTRL_CHOPEN_DISABLE << 22)          /**< Shifted mode DISABLE for CSEN_CTRL */
#define CSEN_CTRL_CHOPEN_ENABLE                              (_CSEN_CTRL_CHOPEN_ENABLE << 22)           /**< Shifted mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_AUTOGND                                    (0x1UL << 23)                              /**< CSEN Automatic Ground Enable */
#define _CSEN_CTRL_AUTOGND_SHIFT                             23                                         /**< Shift value for CSEN_AUTOGND */
#define _CSEN_CTRL_AUTOGND_MASK                              0x800000UL                                 /**< Bit mask for CSEN_AUTOGND */
#define _CSEN_CTRL_AUTOGND_DEFAULT                           0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_AUTOGND_DISABLE                           0x00000000UL                               /**< Mode DISABLE for CSEN_CTRL */
#define _CSEN_CTRL_AUTOGND_ENABLE                            0x00000001UL                               /**< Mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_AUTOGND_DEFAULT                            (_CSEN_CTRL_AUTOGND_DEFAULT << 23)         /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_AUTOGND_DISABLE                            (_CSEN_CTRL_AUTOGND_DISABLE << 23)         /**< Shifted mode DISABLE for CSEN_CTRL */
#define CSEN_CTRL_AUTOGND_ENABLE                             (_CSEN_CTRL_AUTOGND_ENABLE << 23)          /**< Shifted mode ENABLE for CSEN_CTRL */
#define CSEN_CTRL_MXUC                                       (0x1UL << 24)                              /**< CSEN Mux Disconnect */
#define _CSEN_CTRL_MXUC_SHIFT                                24                                         /**< Shift value for CSEN_MXUC */
#define _CSEN_CTRL_MXUC_MASK                                 0x1000000UL                                /**< Bit mask for CSEN_MXUC */
#define _CSEN_CTRL_MXUC_DEFAULT                              0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_MXUC_CONN                                 0x00000000UL                               /**< Mode CONN for CSEN_CTRL */
#define _CSEN_CTRL_MXUC_UNC                                  0x00000001UL                               /**< Mode UNC for CSEN_CTRL */
#define CSEN_CTRL_MXUC_DEFAULT                               (_CSEN_CTRL_MXUC_DEFAULT << 24)            /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_MXUC_CONN                                  (_CSEN_CTRL_MXUC_CONN << 24)               /**< Shifted mode CONN for CSEN_CTRL */
#define CSEN_CTRL_MXUC_UNC                                   (_CSEN_CTRL_MXUC_UNC << 24)                /**< Shifted mode UNC for CSEN_CTRL */
#define CSEN_CTRL_EMACMPEN                                   (0x1UL << 25)                              /**< Greater and Less Than Comparison Using the Exponential Moving Average (EMA) is Enabled */
#define _CSEN_CTRL_EMACMPEN_SHIFT                            25                                         /**< Shift value for CSEN_EMACMPEN */
#define _CSEN_CTRL_EMACMPEN_MASK                             0x2000000UL                                /**< Bit mask for CSEN_EMACMPEN */
#define _CSEN_CTRL_EMACMPEN_DEFAULT                          0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_EMACMPEN_DEFAULT                           (_CSEN_CTRL_EMACMPEN_DEFAULT << 25)        /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_WARMUPMODE                                 (0x1UL << 26)                              /**< Select Warmup Mode for CSEN */
#define _CSEN_CTRL_WARMUPMODE_SHIFT                          26                                         /**< Shift value for CSEN_WARMUPMODE */
#define _CSEN_CTRL_WARMUPMODE_MASK                           0x4000000UL                                /**< Bit mask for CSEN_WARMUPMODE */
#define _CSEN_CTRL_WARMUPMODE_DEFAULT                        0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_WARMUPMODE_NORMAL                         0x00000000UL                               /**< Mode NORMAL for CSEN_CTRL */
#define _CSEN_CTRL_WARMUPMODE_KEEPCSENWARM                   0x00000001UL                               /**< Mode KEEPCSENWARM for CSEN_CTRL */
#define CSEN_CTRL_WARMUPMODE_DEFAULT                         (_CSEN_CTRL_WARMUPMODE_DEFAULT << 26)      /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_WARMUPMODE_NORMAL                          (_CSEN_CTRL_WARMUPMODE_NORMAL << 26)       /**< Shifted mode NORMAL for CSEN_CTRL */
#define CSEN_CTRL_WARMUPMODE_KEEPCSENWARM                    (_CSEN_CTRL_WARMUPMODE_KEEPCSENWARM << 26) /**< Shifted mode KEEPCSENWARM for CSEN_CTRL */
#define CSEN_CTRL_LOCALSENS                                  (0x1UL << 27)                              /**< Local Sensing Enable */
#define _CSEN_CTRL_LOCALSENS_SHIFT                           27                                         /**< Shift value for CSEN_LOCALSENS */
#define _CSEN_CTRL_LOCALSENS_MASK                            0x8000000UL                                /**< Bit mask for CSEN_LOCALSENS */
#define _CSEN_CTRL_LOCALSENS_DEFAULT                         0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_LOCALSENS_DEFAULT                          (_CSEN_CTRL_LOCALSENS_DEFAULT << 27)       /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_CPACCURACY                                 (0x1UL << 28)                              /**< Charge Pump Accuracy */
#define _CSEN_CTRL_CPACCURACY_SHIFT                          28                                         /**< Shift value for CSEN_CPACCURACY */
#define _CSEN_CTRL_CPACCURACY_MASK                           0x10000000UL                               /**< Bit mask for CSEN_CPACCURACY */
#define _CSEN_CTRL_CPACCURACY_DEFAULT                        0x00000000UL                               /**< Mode DEFAULT for CSEN_CTRL */
#define _CSEN_CTRL_CPACCURACY_LO                             0x00000000UL                               /**< Mode LO for CSEN_CTRL */
#define _CSEN_CTRL_CPACCURACY_HI                             0x00000001UL                               /**< Mode HI for CSEN_CTRL */
#define CSEN_CTRL_CPACCURACY_DEFAULT                         (_CSEN_CTRL_CPACCURACY_DEFAULT << 28)      /**< Shifted mode DEFAULT for CSEN_CTRL */
#define CSEN_CTRL_CPACCURACY_LO                              (_CSEN_CTRL_CPACCURACY_LO << 28)           /**< Shifted mode LO for CSEN_CTRL */
#define CSEN_CTRL_CPACCURACY_HI                              (_CSEN_CTRL_CPACCURACY_HI << 28)           /**< Shifted mode HI for CSEN_CTRL */

/* Bit fields for CSEN TIMCTRL */
#define _CSEN_TIMCTRL_RESETVALUE                             0x00000000UL                            /**< Default value for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_MASK                                   0x0003FF07UL                            /**< Mask for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_SHIFT                          0                                       /**< Shift value for CSEN_PCPRESC */
#define _CSEN_TIMCTRL_PCPRESC_MASK                           0x7UL                                   /**< Bit mask for CSEN_PCPRESC */
#define _CSEN_TIMCTRL_PCPRESC_DEFAULT                        0x00000000UL                            /**< Mode DEFAULT for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_DIV1                           0x00000000UL                            /**< Mode DIV1 for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_DIV2                           0x00000001UL                            /**< Mode DIV2 for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_DIV4                           0x00000002UL                            /**< Mode DIV4 for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_DIV8                           0x00000003UL                            /**< Mode DIV8 for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_DIV16                          0x00000004UL                            /**< Mode DIV16 for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_DIV32                          0x00000005UL                            /**< Mode DIV32 for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_DIV64                          0x00000006UL                            /**< Mode DIV64 for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCPRESC_DIV128                         0x00000007UL                            /**< Mode DIV128 for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DEFAULT                         (_CSEN_TIMCTRL_PCPRESC_DEFAULT << 0)    /**< Shifted mode DEFAULT for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DIV1                            (_CSEN_TIMCTRL_PCPRESC_DIV1 << 0)       /**< Shifted mode DIV1 for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DIV2                            (_CSEN_TIMCTRL_PCPRESC_DIV2 << 0)       /**< Shifted mode DIV2 for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DIV4                            (_CSEN_TIMCTRL_PCPRESC_DIV4 << 0)       /**< Shifted mode DIV4 for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DIV8                            (_CSEN_TIMCTRL_PCPRESC_DIV8 << 0)       /**< Shifted mode DIV8 for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DIV16                           (_CSEN_TIMCTRL_PCPRESC_DIV16 << 0)      /**< Shifted mode DIV16 for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DIV32                           (_CSEN_TIMCTRL_PCPRESC_DIV32 << 0)      /**< Shifted mode DIV32 for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DIV64                           (_CSEN_TIMCTRL_PCPRESC_DIV64 << 0)      /**< Shifted mode DIV64 for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCPRESC_DIV128                          (_CSEN_TIMCTRL_PCPRESC_DIV128 << 0)     /**< Shifted mode DIV128 for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_PCTOP_SHIFT                            8                                       /**< Shift value for CSEN_PCTOP */
#define _CSEN_TIMCTRL_PCTOP_MASK                             0xFF00UL                                /**< Bit mask for CSEN_PCTOP */
#define _CSEN_TIMCTRL_PCTOP_DEFAULT                          0x00000000UL                            /**< Mode DEFAULT for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_PCTOP_DEFAULT                           (_CSEN_TIMCTRL_PCTOP_DEFAULT << 8)      /**< Shifted mode DEFAULT for CSEN_TIMCTRL */
#define _CSEN_TIMCTRL_WARMUPCNT_SHIFT                        16                                      /**< Shift value for CSEN_WARMUPCNT */
#define _CSEN_TIMCTRL_WARMUPCNT_MASK                         0x30000UL                               /**< Bit mask for CSEN_WARMUPCNT */
#define _CSEN_TIMCTRL_WARMUPCNT_DEFAULT                      0x00000000UL                            /**< Mode DEFAULT for CSEN_TIMCTRL */
#define CSEN_TIMCTRL_WARMUPCNT_DEFAULT                       (_CSEN_TIMCTRL_WARMUPCNT_DEFAULT << 16) /**< Shifted mode DEFAULT for CSEN_TIMCTRL */

/* Bit fields for CSEN CMD */
#define _CSEN_CMD_RESETVALUE                                 0x00000000UL                   /**< Default value for CSEN_CMD */
#define _CSEN_CMD_MASK                                       0x00000001UL                   /**< Mask for CSEN_CMD */
#define CSEN_CMD_START                                       (0x1UL << 0)                   /**< Start Software-Triggered Conversions */
#define _CSEN_CMD_START_SHIFT                                0                              /**< Shift value for CSEN_START */
#define _CSEN_CMD_START_MASK                                 0x1UL                          /**< Bit mask for CSEN_START */
#define _CSEN_CMD_START_DEFAULT                              0x00000000UL                   /**< Mode DEFAULT for CSEN_CMD */
#define CSEN_CMD_START_DEFAULT                               (_CSEN_CMD_START_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_CMD */

/* Bit fields for CSEN STATUS */
#define _CSEN_STATUS_RESETVALUE                              0x00000000UL                         /**< Default value for CSEN_STATUS */
#define _CSEN_STATUS_MASK                                    0x00000001UL                         /**< Mask for CSEN_STATUS */
#define CSEN_STATUS_CSENBUSY                                 (0x1UL << 0)                         /**< Busy Flag */
#define _CSEN_STATUS_CSENBUSY_SHIFT                          0                                    /**< Shift value for CSEN_CSENBUSY */
#define _CSEN_STATUS_CSENBUSY_MASK                           0x1UL                                /**< Bit mask for CSEN_CSENBUSY */
#define _CSEN_STATUS_CSENBUSY_DEFAULT                        0x00000000UL                         /**< Mode DEFAULT for CSEN_STATUS */
#define _CSEN_STATUS_CSENBUSY_IDLE                           0x00000000UL                         /**< Mode IDLE for CSEN_STATUS */
#define _CSEN_STATUS_CSENBUSY_BUSY                           0x00000001UL                         /**< Mode BUSY for CSEN_STATUS */
#define CSEN_STATUS_CSENBUSY_DEFAULT                         (_CSEN_STATUS_CSENBUSY_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_STATUS */
#define CSEN_STATUS_CSENBUSY_IDLE                            (_CSEN_STATUS_CSENBUSY_IDLE << 0)    /**< Shifted mode IDLE for CSEN_STATUS */
#define CSEN_STATUS_CSENBUSY_BUSY                            (_CSEN_STATUS_CSENBUSY_BUSY << 0)    /**< Shifted mode BUSY for CSEN_STATUS */

/* Bit fields for CSEN PRSSEL */
#define _CSEN_PRSSEL_RESETVALUE                              0x00000000UL                       /**< Default value for CSEN_PRSSEL */
#define _CSEN_PRSSEL_MASK                                    0x0000000FUL                       /**< Mask for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_SHIFT                            0                                  /**< Shift value for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_MASK                             0xFUL                              /**< Bit mask for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_DEFAULT                          0x00000000UL                       /**< Mode DEFAULT for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH0                           0x00000000UL                       /**< Mode PRSCH0 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH1                           0x00000001UL                       /**< Mode PRSCH1 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH2                           0x00000002UL                       /**< Mode PRSCH2 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH3                           0x00000003UL                       /**< Mode PRSCH3 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH4                           0x00000004UL                       /**< Mode PRSCH4 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH5                           0x00000005UL                       /**< Mode PRSCH5 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH6                           0x00000006UL                       /**< Mode PRSCH6 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH7                           0x00000007UL                       /**< Mode PRSCH7 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH8                           0x00000008UL                       /**< Mode PRSCH8 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH9                           0x00000009UL                       /**< Mode PRSCH9 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH10                          0x0000000AUL                       /**< Mode PRSCH10 for CSEN_PRSSEL */
#define _CSEN_PRSSEL_PRSSEL_PRSCH11                          0x0000000BUL                       /**< Mode PRSCH11 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_DEFAULT                           (_CSEN_PRSSEL_PRSSEL_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH0                            (_CSEN_PRSSEL_PRSSEL_PRSCH0 << 0)  /**< Shifted mode PRSCH0 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH1                            (_CSEN_PRSSEL_PRSSEL_PRSCH1 << 0)  /**< Shifted mode PRSCH1 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH2                            (_CSEN_PRSSEL_PRSSEL_PRSCH2 << 0)  /**< Shifted mode PRSCH2 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH3                            (_CSEN_PRSSEL_PRSSEL_PRSCH3 << 0)  /**< Shifted mode PRSCH3 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH4                            (_CSEN_PRSSEL_PRSSEL_PRSCH4 << 0)  /**< Shifted mode PRSCH4 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH5                            (_CSEN_PRSSEL_PRSSEL_PRSCH5 << 0)  /**< Shifted mode PRSCH5 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH6                            (_CSEN_PRSSEL_PRSSEL_PRSCH6 << 0)  /**< Shifted mode PRSCH6 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH7                            (_CSEN_PRSSEL_PRSSEL_PRSCH7 << 0)  /**< Shifted mode PRSCH7 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH8                            (_CSEN_PRSSEL_PRSSEL_PRSCH8 << 0)  /**< Shifted mode PRSCH8 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH9                            (_CSEN_PRSSEL_PRSSEL_PRSCH9 << 0)  /**< Shifted mode PRSCH9 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH10                           (_CSEN_PRSSEL_PRSSEL_PRSCH10 << 0) /**< Shifted mode PRSCH10 for CSEN_PRSSEL */
#define CSEN_PRSSEL_PRSSEL_PRSCH11                           (_CSEN_PRSSEL_PRSSEL_PRSCH11 << 0) /**< Shifted mode PRSCH11 for CSEN_PRSSEL */

/* Bit fields for CSEN DATA */
#define _CSEN_DATA_RESETVALUE                                0x00000000UL                   /**< Default value for CSEN_DATA */
#define _CSEN_DATA_MASK                                      0xFFFFFFFFUL                   /**< Mask for CSEN_DATA */
#define _CSEN_DATA_DATA_SHIFT                                0                              /**< Shift value for CSEN_DATA */
#define _CSEN_DATA_DATA_MASK                                 0xFFFFFFFFUL                   /**< Bit mask for CSEN_DATA */
#define _CSEN_DATA_DATA_DEFAULT                              0x00000000UL                   /**< Mode DEFAULT for CSEN_DATA */
#define CSEN_DATA_DATA_DEFAULT                               (_CSEN_DATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_DATA */

/* Bit fields for CSEN SCANMASK0 */
#define _CSEN_SCANMASK0_RESETVALUE                           0x00000000UL                               /**< Default value for CSEN_SCANMASK0 */
#define _CSEN_SCANMASK0_MASK                                 0xFFFFFFFFUL                               /**< Mask for CSEN_SCANMASK0 */
#define _CSEN_SCANMASK0_SCANINPUTEN_SHIFT                    0                                          /**< Shift value for CSEN_SCANINPUTEN */
#define _CSEN_SCANMASK0_SCANINPUTEN_MASK                     0xFFFFFFFFUL                               /**< Bit mask for CSEN_SCANINPUTEN */
#define _CSEN_SCANMASK0_SCANINPUTEN_DEFAULT                  0x00000000UL                               /**< Mode DEFAULT for CSEN_SCANMASK0 */
#define CSEN_SCANMASK0_SCANINPUTEN_DEFAULT                   (_CSEN_SCANMASK0_SCANINPUTEN_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_SCANMASK0 */

/* Bit fields for CSEN SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_RESETVALUE                       0x00000000UL                                              /**< Default value for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_MASK                             0x0F0F0F0FUL                                              /**< Mask for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_SHIFT               0                                                         /**< Shift value for CSEN_INPUT0TO7SEL */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_MASK                0xFUL                                                     /**< Bit mask for CSEN_INPUT0TO7SEL */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_DEFAULT             0x00000000UL                                              /**< Mode DEFAULT for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH0TO7        0x00000004UL                                              /**< Mode APORT1CH0TO7 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH8TO15       0x00000005UL                                              /**< Mode APORT1CH8TO15 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH16TO23      0x00000006UL                                              /**< Mode APORT1CH16TO23 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH24TO31      0x00000007UL                                              /**< Mode APORT1CH24TO31 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH0TO7        0x0000000CUL                                              /**< Mode APORT3CH0TO7 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH8TO15       0x0000000DUL                                              /**< Mode APORT3CH8TO15 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH16TO23      0x0000000EUL                                              /**< Mode APORT3CH16TO23 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH24TO31      0x0000000FUL                                              /**< Mode APORT3CH24TO31 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_DEFAULT              (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_DEFAULT << 0)           /**< Shifted mode DEFAULT for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH0TO7         (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH0TO7 << 0)      /**< Shifted mode APORT1CH0TO7 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH8TO15        (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH8TO15 << 0)     /**< Shifted mode APORT1CH8TO15 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH16TO23       (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH16TO23 << 0)    /**< Shifted mode APORT1CH16TO23 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH24TO31       (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH24TO31 << 0)    /**< Shifted mode APORT1CH24TO31 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH0TO7         (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH0TO7 << 0)      /**< Shifted mode APORT3CH0TO7 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH8TO15        (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH8TO15 << 0)     /**< Shifted mode APORT3CH8TO15 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH16TO23       (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH16TO23 << 0)    /**< Shifted mode APORT3CH16TO23 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH24TO31       (_CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH24TO31 << 0)    /**< Shifted mode APORT3CH24TO31 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_SHIFT              8                                                         /**< Shift value for CSEN_INPUT8TO15SEL */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_MASK               0xF00UL                                                   /**< Bit mask for CSEN_INPUT8TO15SEL */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_DEFAULT            0x00000000UL                                              /**< Mode DEFAULT for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH0TO7       0x00000004UL                                              /**< Mode APORT1CH0TO7 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH8TO15      0x00000005UL                                              /**< Mode APORT1CH8TO15 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH16TO23     0x00000006UL                                              /**< Mode APORT1CH16TO23 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH24TO31     0x00000007UL                                              /**< Mode APORT1CH24TO31 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH0TO7       0x0000000CUL                                              /**< Mode APORT3CH0TO7 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH8TO15      0x0000000DUL                                              /**< Mode APORT3CH8TO15 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH16TO23     0x0000000EUL                                              /**< Mode APORT3CH16TO23 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH24TO31     0x0000000FUL                                              /**< Mode APORT3CH24TO31 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_DEFAULT             (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_DEFAULT << 8)          /**< Shifted mode DEFAULT for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH0TO7        (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH0TO7 << 8)     /**< Shifted mode APORT1CH0TO7 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH8TO15       (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH8TO15 << 8)    /**< Shifted mode APORT1CH8TO15 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH16TO23      (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH16TO23 << 8)   /**< Shifted mode APORT1CH16TO23 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH24TO31      (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT1CH24TO31 << 8)   /**< Shifted mode APORT1CH24TO31 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH0TO7        (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH0TO7 << 8)     /**< Shifted mode APORT3CH0TO7 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH8TO15       (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH8TO15 << 8)    /**< Shifted mode APORT3CH8TO15 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH16TO23      (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH16TO23 << 8)   /**< Shifted mode APORT3CH16TO23 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH24TO31      (_CSEN_SCANINPUTSEL0_INPUT8TO15SEL_APORT3CH24TO31 << 8)   /**< Shifted mode APORT3CH24TO31 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_SHIFT             16                                                        /**< Shift value for CSEN_INPUT16TO23SEL */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_MASK              0xF0000UL                                                 /**< Bit mask for CSEN_INPUT16TO23SEL */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_DEFAULT           0x00000000UL                                              /**< Mode DEFAULT for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH0TO7      0x00000004UL                                              /**< Mode APORT1CH0TO7 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH8TO15     0x00000005UL                                              /**< Mode APORT1CH8TO15 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH16TO23    0x00000006UL                                              /**< Mode APORT1CH16TO23 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH24TO31    0x00000007UL                                              /**< Mode APORT1CH24TO31 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH0TO7      0x0000000CUL                                              /**< Mode APORT3CH0TO7 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH8TO15     0x0000000DUL                                              /**< Mode APORT3CH8TO15 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH16TO23    0x0000000EUL                                              /**< Mode APORT3CH16TO23 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH24TO31    0x0000000FUL                                              /**< Mode APORT3CH24TO31 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_DEFAULT            (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_DEFAULT << 16)        /**< Shifted mode DEFAULT for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH0TO7       (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH0TO7 << 16)   /**< Shifted mode APORT1CH0TO7 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH8TO15      (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH8TO15 << 16)  /**< Shifted mode APORT1CH8TO15 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH16TO23     (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH16TO23 << 16) /**< Shifted mode APORT1CH16TO23 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH24TO31     (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT1CH24TO31 << 16) /**< Shifted mode APORT1CH24TO31 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH0TO7       (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH0TO7 << 16)   /**< Shifted mode APORT3CH0TO7 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH8TO15      (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH8TO15 << 16)  /**< Shifted mode APORT3CH8TO15 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH16TO23     (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH16TO23 << 16) /**< Shifted mode APORT3CH16TO23 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH24TO31     (_CSEN_SCANINPUTSEL0_INPUT16TO23SEL_APORT3CH24TO31 << 16) /**< Shifted mode APORT3CH24TO31 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_SHIFT             24                                                        /**< Shift value for CSEN_INPUT24TO31SEL */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_MASK              0xF000000UL                                               /**< Bit mask for CSEN_INPUT24TO31SEL */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_DEFAULT           0x00000000UL                                              /**< Mode DEFAULT for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH0TO7      0x00000004UL                                              /**< Mode APORT1CH0TO7 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH8TO15     0x00000005UL                                              /**< Mode APORT1CH8TO15 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH16TO23    0x00000006UL                                              /**< Mode APORT1CH16TO23 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH24TO31    0x00000007UL                                              /**< Mode APORT1CH24TO31 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH0TO7      0x0000000CUL                                              /**< Mode APORT3CH0TO7 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH8TO15     0x0000000DUL                                              /**< Mode APORT3CH8TO15 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH16TO23    0x0000000EUL                                              /**< Mode APORT3CH16TO23 for CSEN_SCANINPUTSEL0 */
#define _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH24TO31    0x0000000FUL                                              /**< Mode APORT3CH24TO31 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_DEFAULT            (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_DEFAULT << 24)        /**< Shifted mode DEFAULT for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH0TO7       (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH0TO7 << 24)   /**< Shifted mode APORT1CH0TO7 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH8TO15      (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH8TO15 << 24)  /**< Shifted mode APORT1CH8TO15 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH16TO23     (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH16TO23 << 24) /**< Shifted mode APORT1CH16TO23 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH24TO31     (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT1CH24TO31 << 24) /**< Shifted mode APORT1CH24TO31 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH0TO7       (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH0TO7 << 24)   /**< Shifted mode APORT3CH0TO7 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH8TO15      (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH8TO15 << 24)  /**< Shifted mode APORT3CH8TO15 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH16TO23     (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH16TO23 << 24) /**< Shifted mode APORT3CH16TO23 for CSEN_SCANINPUTSEL0 */
#define CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH24TO31     (_CSEN_SCANINPUTSEL0_INPUT24TO31SEL_APORT3CH24TO31 << 24) /**< Shifted mode APORT3CH24TO31 for CSEN_SCANINPUTSEL0 */

/* Bit fields for CSEN SCANMASK1 */
#define _CSEN_SCANMASK1_RESETVALUE                           0x00000000UL                               /**< Default value for CSEN_SCANMASK1 */
#define _CSEN_SCANMASK1_MASK                                 0xFFFFFFFFUL                               /**< Mask for CSEN_SCANMASK1 */
#define _CSEN_SCANMASK1_SCANINPUTEN_SHIFT                    0                                          /**< Shift value for CSEN_SCANINPUTEN */
#define _CSEN_SCANMASK1_SCANINPUTEN_MASK                     0xFFFFFFFFUL                               /**< Bit mask for CSEN_SCANINPUTEN */
#define _CSEN_SCANMASK1_SCANINPUTEN_DEFAULT                  0x00000000UL                               /**< Mode DEFAULT for CSEN_SCANMASK1 */
#define CSEN_SCANMASK1_SCANINPUTEN_DEFAULT                   (_CSEN_SCANMASK1_SCANINPUTEN_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_SCANMASK1 */

/* Bit fields for CSEN SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_RESETVALUE                       0x00000000UL                                              /**< Default value for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_MASK                             0x0F0F0F0FUL                                              /**< Mask for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_SHIFT             0                                                         /**< Shift value for CSEN_INPUT32TO39SEL */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_MASK              0xFUL                                                     /**< Bit mask for CSEN_INPUT32TO39SEL */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_DEFAULT           0x00000000UL                                              /**< Mode DEFAULT for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH0TO7      0x00000004UL                                              /**< Mode APORT1CH0TO7 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH8TO15     0x00000005UL                                              /**< Mode APORT1CH8TO15 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH16TO23    0x00000006UL                                              /**< Mode APORT1CH16TO23 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH24TO31    0x00000007UL                                              /**< Mode APORT1CH24TO31 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH0TO7      0x0000000CUL                                              /**< Mode APORT3CH0TO7 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH8TO15     0x0000000DUL                                              /**< Mode APORT3CH8TO15 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH16TO23    0x0000000EUL                                              /**< Mode APORT3CH16TO23 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH24TO31    0x0000000FUL                                              /**< Mode APORT3CH24TO31 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_DEFAULT            (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_DEFAULT << 0)         /**< Shifted mode DEFAULT for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH0TO7       (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH0TO7 << 0)    /**< Shifted mode APORT1CH0TO7 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH8TO15      (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH8TO15 << 0)   /**< Shifted mode APORT1CH8TO15 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH16TO23     (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH16TO23 << 0)  /**< Shifted mode APORT1CH16TO23 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH24TO31     (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT1CH24TO31 << 0)  /**< Shifted mode APORT1CH24TO31 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH0TO7       (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH0TO7 << 0)    /**< Shifted mode APORT3CH0TO7 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH8TO15      (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH8TO15 << 0)   /**< Shifted mode APORT3CH8TO15 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH16TO23     (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH16TO23 << 0)  /**< Shifted mode APORT3CH16TO23 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH24TO31     (_CSEN_SCANINPUTSEL1_INPUT32TO39SEL_APORT3CH24TO31 << 0)  /**< Shifted mode APORT3CH24TO31 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_SHIFT             8                                                         /**< Shift value for CSEN_INPUT40TO47SEL */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_MASK              0xF00UL                                                   /**< Bit mask for CSEN_INPUT40TO47SEL */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_DEFAULT           0x00000000UL                                              /**< Mode DEFAULT for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH0TO7      0x00000004UL                                              /**< Mode APORT1CH0TO7 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH8TO15     0x00000005UL                                              /**< Mode APORT1CH8TO15 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH16TO23    0x00000006UL                                              /**< Mode APORT1CH16TO23 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH24TO31    0x00000007UL                                              /**< Mode APORT1CH24TO31 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH0TO7      0x0000000CUL                                              /**< Mode APORT3CH0TO7 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH8TO15     0x0000000DUL                                              /**< Mode APORT3CH8TO15 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH16TO23    0x0000000EUL                                              /**< Mode APORT3CH16TO23 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH24TO31    0x0000000FUL                                              /**< Mode APORT3CH24TO31 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_DEFAULT            (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_DEFAULT << 8)         /**< Shifted mode DEFAULT for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH0TO7       (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH0TO7 << 8)    /**< Shifted mode APORT1CH0TO7 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH8TO15      (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH8TO15 << 8)   /**< Shifted mode APORT1CH8TO15 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH16TO23     (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH16TO23 << 8)  /**< Shifted mode APORT1CH16TO23 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH24TO31     (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT1CH24TO31 << 8)  /**< Shifted mode APORT1CH24TO31 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH0TO7       (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH0TO7 << 8)    /**< Shifted mode APORT3CH0TO7 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH8TO15      (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH8TO15 << 8)   /**< Shifted mode APORT3CH8TO15 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH16TO23     (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH16TO23 << 8)  /**< Shifted mode APORT3CH16TO23 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH24TO31     (_CSEN_SCANINPUTSEL1_INPUT40TO47SEL_APORT3CH24TO31 << 8)  /**< Shifted mode APORT3CH24TO31 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_SHIFT             16                                                        /**< Shift value for CSEN_INPUT48TO55SEL */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_MASK              0xF0000UL                                                 /**< Bit mask for CSEN_INPUT48TO55SEL */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_DEFAULT           0x00000000UL                                              /**< Mode DEFAULT for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH0TO7      0x00000004UL                                              /**< Mode APORT1CH0TO7 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH8TO15     0x00000005UL                                              /**< Mode APORT1CH8TO15 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH16TO23    0x00000006UL                                              /**< Mode APORT1CH16TO23 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH24TO31    0x00000007UL                                              /**< Mode APORT1CH24TO31 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH0TO7      0x0000000CUL                                              /**< Mode APORT3CH0TO7 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH8TO15     0x0000000DUL                                              /**< Mode APORT3CH8TO15 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH16TO23    0x0000000EUL                                              /**< Mode APORT3CH16TO23 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH24TO31    0x0000000FUL                                              /**< Mode APORT3CH24TO31 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_DEFAULT            (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_DEFAULT << 16)        /**< Shifted mode DEFAULT for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH0TO7       (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH0TO7 << 16)   /**< Shifted mode APORT1CH0TO7 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH8TO15      (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH8TO15 << 16)  /**< Shifted mode APORT1CH8TO15 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH16TO23     (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH16TO23 << 16) /**< Shifted mode APORT1CH16TO23 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH24TO31     (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT1CH24TO31 << 16) /**< Shifted mode APORT1CH24TO31 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH0TO7       (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH0TO7 << 16)   /**< Shifted mode APORT3CH0TO7 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH8TO15      (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH8TO15 << 16)  /**< Shifted mode APORT3CH8TO15 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH16TO23     (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH16TO23 << 16) /**< Shifted mode APORT3CH16TO23 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH24TO31     (_CSEN_SCANINPUTSEL1_INPUT48TO55SEL_APORT3CH24TO31 << 16) /**< Shifted mode APORT3CH24TO31 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_SHIFT             24                                                        /**< Shift value for CSEN_INPUT56TO63SEL */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_MASK              0xF000000UL                                               /**< Bit mask for CSEN_INPUT56TO63SEL */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_DEFAULT           0x00000000UL                                              /**< Mode DEFAULT for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH0TO7      0x00000004UL                                              /**< Mode APORT1CH0TO7 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH8TO15     0x00000005UL                                              /**< Mode APORT1CH8TO15 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH16TO23    0x00000006UL                                              /**< Mode APORT1CH16TO23 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH24TO31    0x00000007UL                                              /**< Mode APORT1CH24TO31 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH0TO7      0x0000000CUL                                              /**< Mode APORT3CH0TO7 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH8TO15     0x0000000DUL                                              /**< Mode APORT3CH8TO15 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH16TO23    0x0000000EUL                                              /**< Mode APORT3CH16TO23 for CSEN_SCANINPUTSEL1 */
#define _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH24TO31    0x0000000FUL                                              /**< Mode APORT3CH24TO31 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_DEFAULT            (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_DEFAULT << 24)        /**< Shifted mode DEFAULT for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH0TO7       (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH0TO7 << 24)   /**< Shifted mode APORT1CH0TO7 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH8TO15      (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH8TO15 << 24)  /**< Shifted mode APORT1CH8TO15 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH16TO23     (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH16TO23 << 24) /**< Shifted mode APORT1CH16TO23 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH24TO31     (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT1CH24TO31 << 24) /**< Shifted mode APORT1CH24TO31 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH0TO7       (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH0TO7 << 24)   /**< Shifted mode APORT3CH0TO7 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH8TO15      (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH8TO15 << 24)  /**< Shifted mode APORT3CH8TO15 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH16TO23     (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH16TO23 << 24) /**< Shifted mode APORT3CH16TO23 for CSEN_SCANINPUTSEL1 */
#define CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH24TO31     (_CSEN_SCANINPUTSEL1_INPUT56TO63SEL_APORT3CH24TO31 << 24) /**< Shifted mode APORT3CH24TO31 for CSEN_SCANINPUTSEL1 */

/* Bit fields for CSEN APORTREQ */
#define _CSEN_APORTREQ_RESETVALUE                            0x00000000UL                             /**< Default value for CSEN_APORTREQ */
#define _CSEN_APORTREQ_MASK                                  0x000003FCUL                             /**< Mask for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT1XREQ                             (0x1UL << 2)                             /**< 1 If the Bus Connected to APORT2X is Requested */
#define _CSEN_APORTREQ_APORT1XREQ_SHIFT                      2                                        /**< Shift value for CSEN_APORT1XREQ */
#define _CSEN_APORTREQ_APORT1XREQ_MASK                       0x4UL                                    /**< Bit mask for CSEN_APORT1XREQ */
#define _CSEN_APORTREQ_APORT1XREQ_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT1XREQ_DEFAULT                     (_CSEN_APORTREQ_APORT1XREQ_DEFAULT << 2) /**< Shifted mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT1YREQ                             (0x1UL << 3)                             /**< 1 If the Bus Connected to APORT1X is Requested */
#define _CSEN_APORTREQ_APORT1YREQ_SHIFT                      3                                        /**< Shift value for CSEN_APORT1YREQ */
#define _CSEN_APORTREQ_APORT1YREQ_MASK                       0x8UL                                    /**< Bit mask for CSEN_APORT1YREQ */
#define _CSEN_APORTREQ_APORT1YREQ_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT1YREQ_DEFAULT                     (_CSEN_APORTREQ_APORT1YREQ_DEFAULT << 3) /**< Shifted mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT2XREQ                             (0x1UL << 4)                             /**< 1 If the Bus Connected to APORT2X is Requested */
#define _CSEN_APORTREQ_APORT2XREQ_SHIFT                      4                                        /**< Shift value for CSEN_APORT2XREQ */
#define _CSEN_APORTREQ_APORT2XREQ_MASK                       0x10UL                                   /**< Bit mask for CSEN_APORT2XREQ */
#define _CSEN_APORTREQ_APORT2XREQ_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT2XREQ_DEFAULT                     (_CSEN_APORTREQ_APORT2XREQ_DEFAULT << 4) /**< Shifted mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT2YREQ                             (0x1UL << 5)                             /**< 1 If the Bus Connected to APORT2Y is Requested */
#define _CSEN_APORTREQ_APORT2YREQ_SHIFT                      5                                        /**< Shift value for CSEN_APORT2YREQ */
#define _CSEN_APORTREQ_APORT2YREQ_MASK                       0x20UL                                   /**< Bit mask for CSEN_APORT2YREQ */
#define _CSEN_APORTREQ_APORT2YREQ_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT2YREQ_DEFAULT                     (_CSEN_APORTREQ_APORT2YREQ_DEFAULT << 5) /**< Shifted mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT3XREQ                             (0x1UL << 6)                             /**< 1 If the Bus Connected to APORT3X is Requested */
#define _CSEN_APORTREQ_APORT3XREQ_SHIFT                      6                                        /**< Shift value for CSEN_APORT3XREQ */
#define _CSEN_APORTREQ_APORT3XREQ_MASK                       0x40UL                                   /**< Bit mask for CSEN_APORT3XREQ */
#define _CSEN_APORTREQ_APORT3XREQ_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT3XREQ_DEFAULT                     (_CSEN_APORTREQ_APORT3XREQ_DEFAULT << 6) /**< Shifted mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT3YREQ                             (0x1UL << 7)                             /**< 1 If the Bus Connected to APORT3Y is Requested */
#define _CSEN_APORTREQ_APORT3YREQ_SHIFT                      7                                        /**< Shift value for CSEN_APORT3YREQ */
#define _CSEN_APORTREQ_APORT3YREQ_MASK                       0x80UL                                   /**< Bit mask for CSEN_APORT3YREQ */
#define _CSEN_APORTREQ_APORT3YREQ_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT3YREQ_DEFAULT                     (_CSEN_APORTREQ_APORT3YREQ_DEFAULT << 7) /**< Shifted mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT4XREQ                             (0x1UL << 8)                             /**< 1 If the Bus Connected to APORT4X is Requested */
#define _CSEN_APORTREQ_APORT4XREQ_SHIFT                      8                                        /**< Shift value for CSEN_APORT4XREQ */
#define _CSEN_APORTREQ_APORT4XREQ_MASK                       0x100UL                                  /**< Bit mask for CSEN_APORT4XREQ */
#define _CSEN_APORTREQ_APORT4XREQ_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT4XREQ_DEFAULT                     (_CSEN_APORTREQ_APORT4XREQ_DEFAULT << 8) /**< Shifted mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT4YREQ                             (0x1UL << 9)                             /**< 1 If the Bus Connected to APORT4Y is Requested */
#define _CSEN_APORTREQ_APORT4YREQ_SHIFT                      9                                        /**< Shift value for CSEN_APORT4YREQ */
#define _CSEN_APORTREQ_APORT4YREQ_MASK                       0x200UL                                  /**< Bit mask for CSEN_APORT4YREQ */
#define _CSEN_APORTREQ_APORT4YREQ_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for CSEN_APORTREQ */
#define CSEN_APORTREQ_APORT4YREQ_DEFAULT                     (_CSEN_APORTREQ_APORT4YREQ_DEFAULT << 9) /**< Shifted mode DEFAULT for CSEN_APORTREQ */

/* Bit fields for CSEN APORTCONFLICT */
#define _CSEN_APORTCONFLICT_RESETVALUE                       0x00000000UL                                       /**< Default value for CSEN_APORTCONFLICT */
#define _CSEN_APORTCONFLICT_MASK                             0x000003FCUL                                       /**< Mask for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT1XCONFLICT                   (0x1UL << 2)                                       /**< 1 If the Bus Connected to APORT1X is in Conflict With Another Peripheral */
#define _CSEN_APORTCONFLICT_APORT1XCONFLICT_SHIFT            2                                                  /**< Shift value for CSEN_APORT1XCONFLICT */
#define _CSEN_APORTCONFLICT_APORT1XCONFLICT_MASK             0x4UL                                              /**< Bit mask for CSEN_APORT1XCONFLICT */
#define _CSEN_APORTCONFLICT_APORT1XCONFLICT_DEFAULT          0x00000000UL                                       /**< Mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT1XCONFLICT_DEFAULT           (_CSEN_APORTCONFLICT_APORT1XCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT1YCONFLICT                   (0x1UL << 3)                                       /**< 1 If the Bus Connected to APORT1Y is in Conflict With Another Peripheral */
#define _CSEN_APORTCONFLICT_APORT1YCONFLICT_SHIFT            3                                                  /**< Shift value for CSEN_APORT1YCONFLICT */
#define _CSEN_APORTCONFLICT_APORT1YCONFLICT_MASK             0x8UL                                              /**< Bit mask for CSEN_APORT1YCONFLICT */
#define _CSEN_APORTCONFLICT_APORT1YCONFLICT_DEFAULT          0x00000000UL                                       /**< Mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT1YCONFLICT_DEFAULT           (_CSEN_APORTCONFLICT_APORT1YCONFLICT_DEFAULT << 3) /**< Shifted mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT2XCONFLICT                   (0x1UL << 4)                                       /**< 1 If the Bus Connected to APORT2X is in Conflict With Another Peripheral */
#define _CSEN_APORTCONFLICT_APORT2XCONFLICT_SHIFT            4                                                  /**< Shift value for CSEN_APORT2XCONFLICT */
#define _CSEN_APORTCONFLICT_APORT2XCONFLICT_MASK             0x10UL                                             /**< Bit mask for CSEN_APORT2XCONFLICT */
#define _CSEN_APORTCONFLICT_APORT2XCONFLICT_DEFAULT          0x00000000UL                                       /**< Mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT2XCONFLICT_DEFAULT           (_CSEN_APORTCONFLICT_APORT2XCONFLICT_DEFAULT << 4) /**< Shifted mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT2YCONFLICT                   (0x1UL << 5)                                       /**< 1 If the Bus Connected to APORT2Y is in Conflict With Another Peripheral */
#define _CSEN_APORTCONFLICT_APORT2YCONFLICT_SHIFT            5                                                  /**< Shift value for CSEN_APORT2YCONFLICT */
#define _CSEN_APORTCONFLICT_APORT2YCONFLICT_MASK             0x20UL                                             /**< Bit mask for CSEN_APORT2YCONFLICT */
#define _CSEN_APORTCONFLICT_APORT2YCONFLICT_DEFAULT          0x00000000UL                                       /**< Mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT2YCONFLICT_DEFAULT           (_CSEN_APORTCONFLICT_APORT2YCONFLICT_DEFAULT << 5) /**< Shifted mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT3XCONFLICT                   (0x1UL << 6)                                       /**< 1 If the Bus Connected to APORT3X is in Conflict With Another Peripheral */
#define _CSEN_APORTCONFLICT_APORT3XCONFLICT_SHIFT            6                                                  /**< Shift value for CSEN_APORT3XCONFLICT */
#define _CSEN_APORTCONFLICT_APORT3XCONFLICT_MASK             0x40UL                                             /**< Bit mask for CSEN_APORT3XCONFLICT */
#define _CSEN_APORTCONFLICT_APORT3XCONFLICT_DEFAULT          0x00000000UL                                       /**< Mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT3XCONFLICT_DEFAULT           (_CSEN_APORTCONFLICT_APORT3XCONFLICT_DEFAULT << 6) /**< Shifted mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT3YCONFLICT                   (0x1UL << 7)                                       /**< 1 If the Bus Connected to APORT3Y is in Conflict With Another Peripheral */
#define _CSEN_APORTCONFLICT_APORT3YCONFLICT_SHIFT            7                                                  /**< Shift value for CSEN_APORT3YCONFLICT */
#define _CSEN_APORTCONFLICT_APORT3YCONFLICT_MASK             0x80UL                                             /**< Bit mask for CSEN_APORT3YCONFLICT */
#define _CSEN_APORTCONFLICT_APORT3YCONFLICT_DEFAULT          0x00000000UL                                       /**< Mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT3YCONFLICT_DEFAULT           (_CSEN_APORTCONFLICT_APORT3YCONFLICT_DEFAULT << 7) /**< Shifted mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT4XCONFLICT                   (0x1UL << 8)                                       /**< 1 If the Bus Connected to APORT4X is in Conflict With Another Peripheral */
#define _CSEN_APORTCONFLICT_APORT4XCONFLICT_SHIFT            8                                                  /**< Shift value for CSEN_APORT4XCONFLICT */
#define _CSEN_APORTCONFLICT_APORT4XCONFLICT_MASK             0x100UL                                            /**< Bit mask for CSEN_APORT4XCONFLICT */
#define _CSEN_APORTCONFLICT_APORT4XCONFLICT_DEFAULT          0x00000000UL                                       /**< Mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT4XCONFLICT_DEFAULT           (_CSEN_APORTCONFLICT_APORT4XCONFLICT_DEFAULT << 8) /**< Shifted mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT4YCONFLICT                   (0x1UL << 9)                                       /**< 1 If the Bus Connected to APORT4Y is in Conflict With Another Peripheral */
#define _CSEN_APORTCONFLICT_APORT4YCONFLICT_SHIFT            9                                                  /**< Shift value for CSEN_APORT4YCONFLICT */
#define _CSEN_APORTCONFLICT_APORT4YCONFLICT_MASK             0x200UL                                            /**< Bit mask for CSEN_APORT4YCONFLICT */
#define _CSEN_APORTCONFLICT_APORT4YCONFLICT_DEFAULT          0x00000000UL                                       /**< Mode DEFAULT for CSEN_APORTCONFLICT */
#define CSEN_APORTCONFLICT_APORT4YCONFLICT_DEFAULT           (_CSEN_APORTCONFLICT_APORT4YCONFLICT_DEFAULT << 9) /**< Shifted mode DEFAULT for CSEN_APORTCONFLICT */

/* Bit fields for CSEN CMPTHR */
#define _CSEN_CMPTHR_RESETVALUE                              0x00000000UL                       /**< Default value for CSEN_CMPTHR */
#define _CSEN_CMPTHR_MASK                                    0x0000FFFFUL                       /**< Mask for CSEN_CMPTHR */
#define _CSEN_CMPTHR_CMPTHR_SHIFT                            0                                  /**< Shift value for CSEN_CMPTHR */
#define _CSEN_CMPTHR_CMPTHR_MASK                             0xFFFFUL                           /**< Bit mask for CSEN_CMPTHR */
#define _CSEN_CMPTHR_CMPTHR_DEFAULT                          0x00000000UL                       /**< Mode DEFAULT for CSEN_CMPTHR */
#define CSEN_CMPTHR_CMPTHR_DEFAULT                           (_CSEN_CMPTHR_CMPTHR_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_CMPTHR */

/* Bit fields for CSEN EMA */
#define _CSEN_EMA_RESETVALUE                                 0x00000000UL                 /**< Default value for CSEN_EMA */
#define _CSEN_EMA_MASK                                       0x003FFFFFUL                 /**< Mask for CSEN_EMA */
#define _CSEN_EMA_EMA_SHIFT                                  0                            /**< Shift value for CSEN_EMA */
#define _CSEN_EMA_EMA_MASK                                   0x3FFFFFUL                   /**< Bit mask for CSEN_EMA */
#define _CSEN_EMA_EMA_DEFAULT                                0x00000000UL                 /**< Mode DEFAULT for CSEN_EMA */
#define CSEN_EMA_EMA_DEFAULT                                 (_CSEN_EMA_EMA_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_EMA */

/* Bit fields for CSEN EMACTRL */
#define _CSEN_EMACTRL_RESETVALUE                             0x00000000UL                           /**< Default value for CSEN_EMACTRL */
#define _CSEN_EMACTRL_MASK                                   0x00000007UL                           /**< Mask for CSEN_EMACTRL */
#define _CSEN_EMACTRL_EMASAMPLE_SHIFT                        0                                      /**< Shift value for CSEN_EMASAMPLE */
#define _CSEN_EMACTRL_EMASAMPLE_MASK                         0x7UL                                  /**< Bit mask for CSEN_EMASAMPLE */
#define _CSEN_EMACTRL_EMASAMPLE_DEFAULT                      0x00000000UL                           /**< Mode DEFAULT for CSEN_EMACTRL */
#define _CSEN_EMACTRL_EMASAMPLE_W1                           0x00000000UL                           /**< Mode W1 for CSEN_EMACTRL */
#define _CSEN_EMACTRL_EMASAMPLE_W2                           0x00000001UL                           /**< Mode W2 for CSEN_EMACTRL */
#define _CSEN_EMACTRL_EMASAMPLE_W4                           0x00000002UL                           /**< Mode W4 for CSEN_EMACTRL */
#define _CSEN_EMACTRL_EMASAMPLE_W8                           0x00000003UL                           /**< Mode W8 for CSEN_EMACTRL */
#define _CSEN_EMACTRL_EMASAMPLE_W16                          0x00000004UL                           /**< Mode W16 for CSEN_EMACTRL */
#define _CSEN_EMACTRL_EMASAMPLE_W32                          0x00000005UL                           /**< Mode W32 for CSEN_EMACTRL */
#define _CSEN_EMACTRL_EMASAMPLE_W64                          0x00000006UL                           /**< Mode W64 for CSEN_EMACTRL */
#define CSEN_EMACTRL_EMASAMPLE_DEFAULT                       (_CSEN_EMACTRL_EMASAMPLE_DEFAULT << 0) /**< Shifted mode DEFAULT for CSEN_EMACTRL */
#define CSEN_EMACTRL_EMASAMPLE_W1                            (_CSEN_EMACTRL_EMASAMPLE_W1 << 0)      /**< Shifted mode W1 for CSEN_EMACTRL */
#define CSEN_EMACTRL_EMASAMPLE_W2                            (_CSEN_EMACTRL_EMASAMPLE_W2 << 0)      /**< Shifted mode W2 for CSEN_EMACTRL */
#define CSEN_EMACTRL_EMASAMPLE_W4                            (_CSEN_EMACTRL_EMASAMPLE_W4 << 0)      /**< Shifted mode W4 for CSEN_EMACTRL */
#define CSEN_EMACTRL_EMASAMPLE_W8                            (_CSEN_EMACTRL_EMASAMPLE_W8 << 0)      /**< Shifted mode W8 for CSEN_EMACTRL */
#define CSEN_EMACTRL_EMASAMPLE_W16                           (_CSEN_EMACTRL_EMASAMPLE_W16 << 0)     /**< Shifted mode W16 for CSEN_EMACTRL */
#define CSEN_EMACTRL_EMASAMPLE_W32                           (_CSEN_EMACTRL_EMASAMPLE_W32 << 0)     /**< Shifted mode W32 for CSEN_EMACTRL */
#define CSEN_EMACTRL_EMASAMPLE_W64                           (_CSEN_EMACTRL_EMASAMPLE_W64 << 0)     /**< Shifted mode W64 for CSEN_EMACTRL */

/* Bit fields for CSEN SINGLECTRL */
#define _CSEN_SINGLECTRL_RESETVALUE                          0x00000000UL                                  /**< Default value for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_MASK                                0x000007F0UL                                  /**< Mask for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_SHIFT                     4                                             /**< Shift value for CSEN_SINGLESEL */
#define _CSEN_SINGLECTRL_SINGLESEL_MASK                      0x7F0UL                                       /**< Bit mask for CSEN_SINGLESEL */
#define _CSEN_SINGLECTRL_SINGLESEL_DEFAULT                   0x00000000UL                                  /**< Mode DEFAULT for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH0                0x00000020UL                                  /**< Mode APORT1XCH0 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH1                0x00000021UL                                  /**< Mode APORT1YCH1 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH2                0x00000022UL                                  /**< Mode APORT1XCH2 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH3                0x00000023UL                                  /**< Mode APORT1YCH3 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH4                0x00000024UL                                  /**< Mode APORT1XCH4 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH5                0x00000025UL                                  /**< Mode APORT1YCH5 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH6                0x00000026UL                                  /**< Mode APORT1XCH6 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH7                0x00000027UL                                  /**< Mode APORT1YCH7 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH8                0x00000028UL                                  /**< Mode APORT1XCH8 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH9                0x00000029UL                                  /**< Mode APORT1YCH9 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH10               0x0000002AUL                                  /**< Mode APORT1XCH10 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH11               0x0000002BUL                                  /**< Mode APORT1YCH11 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH12               0x0000002CUL                                  /**< Mode APORT1XCH12 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH13               0x0000002DUL                                  /**< Mode APORT1YCH13 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH14               0x0000002EUL                                  /**< Mode APORT1XCH14 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH15               0x0000002FUL                                  /**< Mode APORT1YCH15 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH16               0x00000030UL                                  /**< Mode APORT1XCH16 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH17               0x00000031UL                                  /**< Mode APORT1YCH17 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH18               0x00000032UL                                  /**< Mode APORT1XCH18 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH19               0x00000033UL                                  /**< Mode APORT1YCH19 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH20               0x00000034UL                                  /**< Mode APORT1XCH20 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH21               0x00000035UL                                  /**< Mode APORT1YCH21 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH22               0x00000036UL                                  /**< Mode APORT1XCH22 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH23               0x00000037UL                                  /**< Mode APORT1YCH23 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH24               0x00000038UL                                  /**< Mode APORT1XCH24 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH25               0x00000039UL                                  /**< Mode APORT1YCH25 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH26               0x0000003AUL                                  /**< Mode APORT1XCH26 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH27               0x0000003BUL                                  /**< Mode APORT1YCH27 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH28               0x0000003CUL                                  /**< Mode APORT1XCH28 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH29               0x0000003DUL                                  /**< Mode APORT1YCH29 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH30               0x0000003EUL                                  /**< Mode APORT1XCH30 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH31               0x0000003FUL                                  /**< Mode APORT1YCH31 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH0                0x00000060UL                                  /**< Mode APORT3XCH0 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH1                0x00000061UL                                  /**< Mode APORT3YCH1 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH2                0x00000062UL                                  /**< Mode APORT3XCH2 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH3                0x00000063UL                                  /**< Mode APORT3YCH3 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH4                0x00000064UL                                  /**< Mode APORT3XCH4 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH5                0x00000065UL                                  /**< Mode APORT3YCH5 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH6                0x00000066UL                                  /**< Mode APORT3XCH6 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH7                0x00000067UL                                  /**< Mode APORT3YCH7 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH8                0x00000068UL                                  /**< Mode APORT3XCH8 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH9                0x00000069UL                                  /**< Mode APORT3YCH9 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH10               0x0000006AUL                                  /**< Mode APORT3XCH10 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH11               0x0000006BUL                                  /**< Mode APORT3YCH11 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH12               0x0000006CUL                                  /**< Mode APORT3XCH12 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH13               0x0000006DUL                                  /**< Mode APORT3YCH13 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH14               0x0000006EUL                                  /**< Mode APORT3XCH14 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH15               0x0000006FUL                                  /**< Mode APORT3YCH15 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH16               0x00000070UL                                  /**< Mode APORT3XCH16 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH17               0x00000071UL                                  /**< Mode APORT3YCH17 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH18               0x00000072UL                                  /**< Mode APORT3XCH18 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH19               0x00000073UL                                  /**< Mode APORT3YCH19 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH20               0x00000074UL                                  /**< Mode APORT3XCH20 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH21               0x00000075UL                                  /**< Mode APORT3YCH21 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH22               0x00000076UL                                  /**< Mode APORT3XCH22 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH23               0x00000077UL                                  /**< Mode APORT3YCH23 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH24               0x00000078UL                                  /**< Mode APORT3XCH24 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH25               0x00000079UL                                  /**< Mode APORT3YCH25 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH26               0x0000007AUL                                  /**< Mode APORT3XCH26 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH27               0x0000007BUL                                  /**< Mode APORT3YCH27 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH28               0x0000007CUL                                  /**< Mode APORT3XCH28 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH29               0x0000007DUL                                  /**< Mode APORT3YCH29 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH30               0x0000007EUL                                  /**< Mode APORT3XCH30 for CSEN_SINGLECTRL */
#define _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH31               0x0000007FUL                                  /**< Mode APORT3YCH31 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_DEFAULT                    (_CSEN_SINGLECTRL_SINGLESEL_DEFAULT << 4)     /**< Shifted mode DEFAULT for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH0                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH0 << 4)  /**< Shifted mode APORT1XCH0 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH1                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH1 << 4)  /**< Shifted mode APORT1YCH1 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH2                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH2 << 4)  /**< Shifted mode APORT1XCH2 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH3                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH3 << 4)  /**< Shifted mode APORT1YCH3 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH4                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH4 << 4)  /**< Shifted mode APORT1XCH4 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH5                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH5 << 4)  /**< Shifted mode APORT1YCH5 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH6                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH6 << 4)  /**< Shifted mode APORT1XCH6 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH7                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH7 << 4)  /**< Shifted mode APORT1YCH7 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH8                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH8 << 4)  /**< Shifted mode APORT1XCH8 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH9                 (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH9 << 4)  /**< Shifted mode APORT1YCH9 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH10                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH10 << 4) /**< Shifted mode APORT1XCH10 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH11                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH11 << 4) /**< Shifted mode APORT1YCH11 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH12                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH12 << 4) /**< Shifted mode APORT1XCH12 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH13                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH13 << 4) /**< Shifted mode APORT1YCH13 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH14                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH14 << 4) /**< Shifted mode APORT1XCH14 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH15                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH15 << 4) /**< Shifted mode APORT1YCH15 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH16                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH16 << 4) /**< Shifted mode APORT1XCH16 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH17                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH17 << 4) /**< Shifted mode APORT1YCH17 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH18                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH18 << 4) /**< Shifted mode APORT1XCH18 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH19                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH19 << 4) /**< Shifted mode APORT1YCH19 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH20                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH20 << 4) /**< Shifted mode APORT1XCH20 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH21                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH21 << 4) /**< Shifted mode APORT1YCH21 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH22                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH22 << 4) /**< Shifted mode APORT1XCH22 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH23                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH23 << 4) /**< Shifted mode APORT1YCH23 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH24                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH24 << 4) /**< Shifted mode APORT1XCH24 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH25                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH25 << 4) /**< Shifted mode APORT1YCH25 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH26                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH26 << 4) /**< Shifted mode APORT1XCH26 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH27                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH27 << 4) /**< Shifted mode APORT1YCH27 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH28                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH28 << 4) /**< Shifted mode APORT1XCH28 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH29                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH29 << 4) /**< Shifted mode APORT1YCH29 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1XCH30                (_CSEN_SINGLECTRL_SINGLESEL_APORT1XCH30 << 4) /**< Shifted mode APORT1XCH30 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT1YCH31                (_CSEN_SINGLECTRL_SINGLESEL_APORT1YCH31 << 4) /**< Shifted mode APORT1YCH31 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH0                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH0 << 4)  /**< Shifted mode APORT3XCH0 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH1                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH1 << 4)  /**< Shifted mode APORT3YCH1 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH2                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH2 << 4)  /**< Shifted mode APORT3XCH2 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH3                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH3 << 4)  /**< Shifted mode APORT3YCH3 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH4                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH4 << 4)  /**< Shifted mode APORT3XCH4 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH5                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH5 << 4)  /**< Shifted mode APORT3YCH5 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH6                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH6 << 4)  /**< Shifted mode APORT3XCH6 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH7                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH7 << 4)  /**< Shifted mode APORT3YCH7 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH8                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH8 << 4)  /**< Shifted mode APORT3XCH8 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH9                 (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH9 << 4)  /**< Shifted mode APORT3YCH9 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH10                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH10 << 4) /**< Shifted mode APORT3XCH10 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH11                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH11 << 4) /**< Shifted mode APORT3YCH11 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH12                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH12 << 4) /**< Shifted mode APORT3XCH12 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH13                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH13 << 4) /**< Shifted mode APORT3YCH13 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH14                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH14 << 4) /**< Shifted mode APORT3XCH14 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH15                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH15 << 4) /**< Shifted mode APORT3YCH15 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH16                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH16 << 4) /**< Shifted mode APORT3XCH16 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH17                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH17 << 4) /**< Shifted mode APORT3YCH17 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH18                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH18 << 4) /**< Shifted mode APORT3XCH18 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH19                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH19 << 4) /**< Shifted mode APORT3YCH19 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH20                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH20 << 4) /**< Shifted mode APORT3XCH20 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH21                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH21 << 4) /**< Shifted mode APORT3YCH21 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH22                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH22 << 4) /**< Shifted mode APORT3XCH22 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH23                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH23 << 4) /**< Shifted mode APORT3YCH23 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH24                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH24 << 4) /**< Shifted mode APORT3XCH24 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH25                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH25 << 4) /**< Shifted mode APORT3YCH25 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH26                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH26 << 4) /**< Shifted mode APORT3XCH26 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH27                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH27 << 4) /**< Shifted mode APORT3YCH27 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH28                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH28 << 4) /**< Shifted mode APORT3XCH28 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH29                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH29 << 4) /**< Shifted mode APORT3YCH29 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3XCH30                (_CSEN_SINGLECTRL_SINGLESEL_APORT3XCH30 << 4) /**< Shifted mode APORT3XCH30 for CSEN_SINGLECTRL */
#define CSEN_SINGLECTRL_SINGLESEL_APORT3YCH31                (_CSEN_SINGLECTRL_SINGLESEL_APORT3YCH31 << 4) /**< Shifted mode APORT3YCH31 for CSEN_SINGLECTRL */

/* Bit fields for CSEN DMBASELINE */
#define _CSEN_DMBASELINE_RESETVALUE                          0x00000000UL                                /**< Default value for CSEN_DMBASELINE */
#define _CSEN_DMBASELINE_MASK                                0xFFFFFFFFUL                                /**< Mask for CSEN_DMBASELINE */
#define _CSEN_DMBASELINE_BASELINEUP_SHIFT                    0                                           /**< Shift value for CSEN_BASELINEUP */
#define _CSEN_DMBASELINE_BASELINEUP_MASK                     0xFFFFUL                                    /**< Bit mask for CSEN_BASELINEUP */
#define _CSEN_DMBASELINE_BASELINEUP_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CSEN_DMBASELINE */
#define CSEN_DMBASELINE_BASELINEUP_DEFAULT                   (_CSEN_DMBASELINE_BASELINEUP_DEFAULT << 0)  /**< Shifted mode DEFAULT for CSEN_DMBASELINE */
#define _CSEN_DMBASELINE_BASELINEDN_SHIFT                    16                                          /**< Shift value for CSEN_BASELINEDN */
#define _CSEN_DMBASELINE_BASELINEDN_MASK                     0xFFFF0000UL                                /**< Bit mask for CSEN_BASELINEDN */
#define _CSEN_DMBASELINE_BASELINEDN_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CSEN_DMBASELINE */
#define CSEN_DMBASELINE_BASELINEDN_DEFAULT                   (_CSEN_DMBASELINE_BASELINEDN_DEFAULT << 16) /**< Shifted mode DEFAULT for CSEN_DMBASELINE */

/* Bit fields for CSEN DMCFG */
#define _CSEN_DMCFG_RESETVALUE                               0x00000000UL                        /**< Default value for CSEN_DMCFG */
#define _CSEN_DMCFG_MASK                                     0x103F0FFFUL                        /**< Mask for CSEN_DMCFG */
#define _CSEN_DMCFG_DMG_SHIFT                                0                                   /**< Shift value for CSEN_DMG */
#define _CSEN_DMCFG_DMG_MASK                                 0xFFUL                              /**< Bit mask for CSEN_DMG */
#define _CSEN_DMCFG_DMG_DEFAULT                              0x00000000UL                        /**< Mode DEFAULT for CSEN_DMCFG */
#define CSEN_DMCFG_DMG_DEFAULT                               (_CSEN_DMCFG_DMG_DEFAULT << 0)      /**< Shifted mode DEFAULT for CSEN_DMCFG */
#define _CSEN_DMCFG_DMR_SHIFT                                8                                   /**< Shift value for CSEN_DMR */
#define _CSEN_DMCFG_DMR_MASK                                 0xF00UL                             /**< Bit mask for CSEN_DMR */
#define _CSEN_DMCFG_DMR_DEFAULT                              0x00000000UL                        /**< Mode DEFAULT for CSEN_DMCFG */
#define CSEN_DMCFG_DMR_DEFAULT                               (_CSEN_DMCFG_DMR_DEFAULT << 8)      /**< Shifted mode DEFAULT for CSEN_DMCFG */
#define _CSEN_DMCFG_DMCR_SHIFT                               16                                  /**< Shift value for CSEN_DMCR */
#define _CSEN_DMCFG_DMCR_MASK                                0xF0000UL                           /**< Bit mask for CSEN_DMCR */
#define _CSEN_DMCFG_DMCR_DEFAULT                             0x00000000UL                        /**< Mode DEFAULT for CSEN_DMCFG */
#define CSEN_DMCFG_DMCR_DEFAULT                              (_CSEN_DMCFG_DMCR_DEFAULT << 16)    /**< Shifted mode DEFAULT for CSEN_DMCFG */
#define _CSEN_DMCFG_CRMODE_SHIFT                             20                                  /**< Shift value for CSEN_CRMODE */
#define _CSEN_DMCFG_CRMODE_MASK                              0x300000UL                          /**< Bit mask for CSEN_CRMODE */
#define _CSEN_DMCFG_CRMODE_DEFAULT                           0x00000000UL                        /**< Mode DEFAULT for CSEN_DMCFG */
#define _CSEN_DMCFG_CRMODE_DM10                              0x00000000UL                        /**< Mode DM10 for CSEN_DMCFG */
#define _CSEN_DMCFG_CRMODE_DM12                              0x00000001UL                        /**< Mode DM12 for CSEN_DMCFG */
#define _CSEN_DMCFG_CRMODE_DM14                              0x00000002UL                        /**< Mode DM14 for CSEN_DMCFG */
#define _CSEN_DMCFG_CRMODE_DM16                              0x00000003UL                        /**< Mode DM16 for CSEN_DMCFG */
#define CSEN_DMCFG_CRMODE_DEFAULT                            (_CSEN_DMCFG_CRMODE_DEFAULT << 20)  /**< Shifted mode DEFAULT for CSEN_DMCFG */
#define CSEN_DMCFG_CRMODE_DM10                               (_CSEN_DMCFG_CRMODE_DM10 << 20)     /**< Shifted mode DM10 for CSEN_DMCFG */
#define CSEN_DMCFG_CRMODE_DM12                               (_CSEN_DMCFG_CRMODE_DM12 << 20)     /**< Shifted mode DM12 for CSEN_DMCFG */
#define CSEN_DMCFG_CRMODE_DM14                               (_CSEN_DMCFG_CRMODE_DM14 << 20)     /**< Shifted mode DM14 for CSEN_DMCFG */
#define CSEN_DMCFG_CRMODE_DM16                               (_CSEN_DMCFG_CRMODE_DM16 << 20)     /**< Shifted mode DM16 for CSEN_DMCFG */
#define CSEN_DMCFG_DMGRDIS                                   (0x1UL << 28)                       /**< Delta Modulation Gain Step Reduction Disable */
#define _CSEN_DMCFG_DMGRDIS_SHIFT                            28                                  /**< Shift value for CSEN_DMGRDIS */
#define _CSEN_DMCFG_DMGRDIS_MASK                             0x10000000UL                        /**< Bit mask for CSEN_DMGRDIS */
#define _CSEN_DMCFG_DMGRDIS_DEFAULT                          0x00000000UL                        /**< Mode DEFAULT for CSEN_DMCFG */
#define CSEN_DMCFG_DMGRDIS_DEFAULT                           (_CSEN_DMCFG_DMGRDIS_DEFAULT << 28) /**< Shifted mode DEFAULT for CSEN_DMCFG */

/* Bit fields for CSEN ANACTRL */
#define _CSEN_ANACTRL_RESETVALUE                             0x00000070UL                           /**< Default value for CSEN_ANACTRL */
#define _CSEN_ANACTRL_MASK                                   0x00700770UL                           /**< Mask for CSEN_ANACTRL */
#define _CSEN_ANACTRL_IREFPROG_SHIFT                         4                                      /**< Shift value for CSEN_IREFPROG */
#define _CSEN_ANACTRL_IREFPROG_MASK                          0x70UL                                 /**< Bit mask for CSEN_IREFPROG */
#define _CSEN_ANACTRL_IREFPROG_DEFAULT                       0x00000007UL                           /**< Mode DEFAULT for CSEN_ANACTRL */
#define CSEN_ANACTRL_IREFPROG_DEFAULT                        (_CSEN_ANACTRL_IREFPROG_DEFAULT << 4)  /**< Shifted mode DEFAULT for CSEN_ANACTRL */
#define _CSEN_ANACTRL_IDACIREFS_SHIFT                        8                                      /**< Shift value for CSEN_IDACIREFS */
#define _CSEN_ANACTRL_IDACIREFS_MASK                         0x700UL                                /**< Bit mask for CSEN_IDACIREFS */
#define _CSEN_ANACTRL_IDACIREFS_DEFAULT                      0x00000000UL                           /**< Mode DEFAULT for CSEN_ANACTRL */
#define CSEN_ANACTRL_IDACIREFS_DEFAULT                       (_CSEN_ANACTRL_IDACIREFS_DEFAULT << 8) /**< Shifted mode DEFAULT for CSEN_ANACTRL */
#define _CSEN_ANACTRL_TRSTPROG_SHIFT                         20                                     /**< Shift value for CSEN_TRSTPROG */
#define _CSEN_ANACTRL_TRSTPROG_MASK                          0x700000UL                             /**< Bit mask for CSEN_TRSTPROG */
#define _CSEN_ANACTRL_TRSTPROG_DEFAULT                       0x00000000UL                           /**< Mode DEFAULT for CSEN_ANACTRL */
#define CSEN_ANACTRL_TRSTPROG_DEFAULT                        (_CSEN_ANACTRL_TRSTPROG_DEFAULT << 20) /**< Shifted mode DEFAULT for CSEN_ANACTRL */

/* Bit fields for CSEN IF */
#define _CSEN_IF_RESETVALUE                                  0x00000000UL                          /**< Default value for CSEN_IF */
#define _CSEN_IF_MASK                                        0x0000001FUL                          /**< Mask for CSEN_IF */
#define CSEN_IF_CMP                                          (0x1UL << 0)                          /**< Digital Comparator Interrupt Flag */
#define _CSEN_IF_CMP_SHIFT                                   0                                     /**< Shift value for CSEN_CMP */
#define _CSEN_IF_CMP_MASK                                    0x1UL                                 /**< Bit mask for CSEN_CMP */
#define _CSEN_IF_CMP_DEFAULT                                 0x00000000UL                          /**< Mode DEFAULT for CSEN_IF */
#define CSEN_IF_CMP_DEFAULT                                  (_CSEN_IF_CMP_DEFAULT << 0)           /**< Shifted mode DEFAULT for CSEN_IF */
#define CSEN_IF_CONV                                         (0x1UL << 1)                          /**< Conversion Done Interrupt Flag */
#define _CSEN_IF_CONV_SHIFT                                  1                                     /**< Shift value for CSEN_CONV */
#define _CSEN_IF_CONV_MASK                                   0x2UL                                 /**< Bit mask for CSEN_CONV */
#define _CSEN_IF_CONV_DEFAULT                                0x00000000UL                          /**< Mode DEFAULT for CSEN_IF */
#define CSEN_IF_CONV_DEFAULT                                 (_CSEN_IF_CONV_DEFAULT << 1)          /**< Shifted mode DEFAULT for CSEN_IF */
#define CSEN_IF_EOS                                          (0x1UL << 2)                          /**< End of Scan Interrupt Flag. */
#define _CSEN_IF_EOS_SHIFT                                   2                                     /**< Shift value for CSEN_EOS */
#define _CSEN_IF_EOS_MASK                                    0x4UL                                 /**< Bit mask for CSEN_EOS */
#define _CSEN_IF_EOS_DEFAULT                                 0x00000000UL                          /**< Mode DEFAULT for CSEN_IF */
#define CSEN_IF_EOS_DEFAULT                                  (_CSEN_IF_EOS_DEFAULT << 2)           /**< Shifted mode DEFAULT for CSEN_IF */
#define CSEN_IF_DMAOF                                        (0x1UL << 3)                          /**< DMA Overflow Interrupt Flag. */
#define _CSEN_IF_DMAOF_SHIFT                                 3                                     /**< Shift value for CSEN_DMAOF */
#define _CSEN_IF_DMAOF_MASK                                  0x8UL                                 /**< Bit mask for CSEN_DMAOF */
#define _CSEN_IF_DMAOF_DEFAULT                               0x00000000UL                          /**< Mode DEFAULT for CSEN_IF */
#define CSEN_IF_DMAOF_DEFAULT                                (_CSEN_IF_DMAOF_DEFAULT << 3)         /**< Shifted mode DEFAULT for CSEN_IF */
#define CSEN_IF_APORTCONFLICT                                (0x1UL << 4)                          /**< APORT Conflict Interrupt Flag */
#define _CSEN_IF_APORTCONFLICT_SHIFT                         4                                     /**< Shift value for CSEN_APORTCONFLICT */
#define _CSEN_IF_APORTCONFLICT_MASK                          0x10UL                                /**< Bit mask for CSEN_APORTCONFLICT */
#define _CSEN_IF_APORTCONFLICT_DEFAULT                       0x00000000UL                          /**< Mode DEFAULT for CSEN_IF */
#define CSEN_IF_APORTCONFLICT_DEFAULT                        (_CSEN_IF_APORTCONFLICT_DEFAULT << 4) /**< Shifted mode DEFAULT for CSEN_IF */

/* Bit fields for CSEN IFS */
#define _CSEN_IFS_RESETVALUE                                 0x00000000UL                           /**< Default value for CSEN_IFS */
#define _CSEN_IFS_MASK                                       0x0000001FUL                           /**< Mask for CSEN_IFS */
#define CSEN_IFS_CMP                                         (0x1UL << 0)                           /**< Set CMP Interrupt Flag */
#define _CSEN_IFS_CMP_SHIFT                                  0                                      /**< Shift value for CSEN_CMP */
#define _CSEN_IFS_CMP_MASK                                   0x1UL                                  /**< Bit mask for CSEN_CMP */
#define _CSEN_IFS_CMP_DEFAULT                                0x00000000UL                           /**< Mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_CMP_DEFAULT                                 (_CSEN_IFS_CMP_DEFAULT << 0)           /**< Shifted mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_CONV                                        (0x1UL << 1)                           /**< Set CONV Interrupt Flag */
#define _CSEN_IFS_CONV_SHIFT                                 1                                      /**< Shift value for CSEN_CONV */
#define _CSEN_IFS_CONV_MASK                                  0x2UL                                  /**< Bit mask for CSEN_CONV */
#define _CSEN_IFS_CONV_DEFAULT                               0x00000000UL                           /**< Mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_CONV_DEFAULT                                (_CSEN_IFS_CONV_DEFAULT << 1)          /**< Shifted mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_EOS                                         (0x1UL << 2)                           /**< Set EOS Interrupt Flag */
#define _CSEN_IFS_EOS_SHIFT                                  2                                      /**< Shift value for CSEN_EOS */
#define _CSEN_IFS_EOS_MASK                                   0x4UL                                  /**< Bit mask for CSEN_EOS */
#define _CSEN_IFS_EOS_DEFAULT                                0x00000000UL                           /**< Mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_EOS_DEFAULT                                 (_CSEN_IFS_EOS_DEFAULT << 2)           /**< Shifted mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_DMAOF                                       (0x1UL << 3)                           /**< Set DMAOF Interrupt Flag */
#define _CSEN_IFS_DMAOF_SHIFT                                3                                      /**< Shift value for CSEN_DMAOF */
#define _CSEN_IFS_DMAOF_MASK                                 0x8UL                                  /**< Bit mask for CSEN_DMAOF */
#define _CSEN_IFS_DMAOF_DEFAULT                              0x00000000UL                           /**< Mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_DMAOF_DEFAULT                               (_CSEN_IFS_DMAOF_DEFAULT << 3)         /**< Shifted mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_APORTCONFLICT                               (0x1UL << 4)                           /**< Set APORTCONFLICT Interrupt Flag */
#define _CSEN_IFS_APORTCONFLICT_SHIFT                        4                                      /**< Shift value for CSEN_APORTCONFLICT */
#define _CSEN_IFS_APORTCONFLICT_MASK                         0x10UL                                 /**< Bit mask for CSEN_APORTCONFLICT */
#define _CSEN_IFS_APORTCONFLICT_DEFAULT                      0x00000000UL                           /**< Mode DEFAULT for CSEN_IFS */
#define CSEN_IFS_APORTCONFLICT_DEFAULT                       (_CSEN_IFS_APORTCONFLICT_DEFAULT << 4) /**< Shifted mode DEFAULT for CSEN_IFS */

/* Bit fields for CSEN IFC */
#define _CSEN_IFC_RESETVALUE                                 0x00000000UL                           /**< Default value for CSEN_IFC */
#define _CSEN_IFC_MASK                                       0x0000001FUL                           /**< Mask for CSEN_IFC */
#define CSEN_IFC_CMP                                         (0x1UL << 0)                           /**< Clear CMP Interrupt Flag */
#define _CSEN_IFC_CMP_SHIFT                                  0                                      /**< Shift value for CSEN_CMP */
#define _CSEN_IFC_CMP_MASK                                   0x1UL                                  /**< Bit mask for CSEN_CMP */
#define _CSEN_IFC_CMP_DEFAULT                                0x00000000UL                           /**< Mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_CMP_DEFAULT                                 (_CSEN_IFC_CMP_DEFAULT << 0)           /**< Shifted mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_CONV                                        (0x1UL << 1)                           /**< Clear CONV Interrupt Flag */
#define _CSEN_IFC_CONV_SHIFT                                 1                                      /**< Shift value for CSEN_CONV */
#define _CSEN_IFC_CONV_MASK                                  0x2UL                                  /**< Bit mask for CSEN_CONV */
#define _CSEN_IFC_CONV_DEFAULT                               0x00000000UL                           /**< Mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_CONV_DEFAULT                                (_CSEN_IFC_CONV_DEFAULT << 1)          /**< Shifted mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_EOS                                         (0x1UL << 2)                           /**< Clear EOS Interrupt Flag */
#define _CSEN_IFC_EOS_SHIFT                                  2                                      /**< Shift value for CSEN_EOS */
#define _CSEN_IFC_EOS_MASK                                   0x4UL                                  /**< Bit mask for CSEN_EOS */
#define _CSEN_IFC_EOS_DEFAULT                                0x00000000UL                           /**< Mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_EOS_DEFAULT                                 (_CSEN_IFC_EOS_DEFAULT << 2)           /**< Shifted mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_DMAOF                                       (0x1UL << 3)                           /**< Clear DMAOF Interrupt Flag */
#define _CSEN_IFC_DMAOF_SHIFT                                3                                      /**< Shift value for CSEN_DMAOF */
#define _CSEN_IFC_DMAOF_MASK                                 0x8UL                                  /**< Bit mask for CSEN_DMAOF */
#define _CSEN_IFC_DMAOF_DEFAULT                              0x00000000UL                           /**< Mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_DMAOF_DEFAULT                               (_CSEN_IFC_DMAOF_DEFAULT << 3)         /**< Shifted mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_APORTCONFLICT                               (0x1UL << 4)                           /**< Clear APORTCONFLICT Interrupt Flag */
#define _CSEN_IFC_APORTCONFLICT_SHIFT                        4                                      /**< Shift value for CSEN_APORTCONFLICT */
#define _CSEN_IFC_APORTCONFLICT_MASK                         0x10UL                                 /**< Bit mask for CSEN_APORTCONFLICT */
#define _CSEN_IFC_APORTCONFLICT_DEFAULT                      0x00000000UL                           /**< Mode DEFAULT for CSEN_IFC */
#define CSEN_IFC_APORTCONFLICT_DEFAULT                       (_CSEN_IFC_APORTCONFLICT_DEFAULT << 4) /**< Shifted mode DEFAULT for CSEN_IFC */

/* Bit fields for CSEN IEN */
#define _CSEN_IEN_RESETVALUE                                 0x00000000UL                           /**< Default value for CSEN_IEN */
#define _CSEN_IEN_MASK                                       0x0000001FUL                           /**< Mask for CSEN_IEN */
#define CSEN_IEN_CMP                                         (0x1UL << 0)                           /**< CMP Interrupt Enable */
#define _CSEN_IEN_CMP_SHIFT                                  0                                      /**< Shift value for CSEN_CMP */
#define _CSEN_IEN_CMP_MASK                                   0x1UL                                  /**< Bit mask for CSEN_CMP */
#define _CSEN_IEN_CMP_DEFAULT                                0x00000000UL                           /**< Mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_CMP_DEFAULT                                 (_CSEN_IEN_CMP_DEFAULT << 0)           /**< Shifted mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_CONV                                        (0x1UL << 1)                           /**< CONV Interrupt Enable */
#define _CSEN_IEN_CONV_SHIFT                                 1                                      /**< Shift value for CSEN_CONV */
#define _CSEN_IEN_CONV_MASK                                  0x2UL                                  /**< Bit mask for CSEN_CONV */
#define _CSEN_IEN_CONV_DEFAULT                               0x00000000UL                           /**< Mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_CONV_DEFAULT                                (_CSEN_IEN_CONV_DEFAULT << 1)          /**< Shifted mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_EOS                                         (0x1UL << 2)                           /**< EOS Interrupt Enable */
#define _CSEN_IEN_EOS_SHIFT                                  2                                      /**< Shift value for CSEN_EOS */
#define _CSEN_IEN_EOS_MASK                                   0x4UL                                  /**< Bit mask for CSEN_EOS */
#define _CSEN_IEN_EOS_DEFAULT                                0x00000000UL                           /**< Mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_EOS_DEFAULT                                 (_CSEN_IEN_EOS_DEFAULT << 2)           /**< Shifted mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_DMAOF                                       (0x1UL << 3)                           /**< DMAOF Interrupt Enable */
#define _CSEN_IEN_DMAOF_SHIFT                                3                                      /**< Shift value for CSEN_DMAOF */
#define _CSEN_IEN_DMAOF_MASK                                 0x8UL                                  /**< Bit mask for CSEN_DMAOF */
#define _CSEN_IEN_DMAOF_DEFAULT                              0x00000000UL                           /**< Mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_DMAOF_DEFAULT                               (_CSEN_IEN_DMAOF_DEFAULT << 3)         /**< Shifted mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_APORTCONFLICT                               (0x1UL << 4)                           /**< APORTCONFLICT Interrupt Enable */
#define _CSEN_IEN_APORTCONFLICT_SHIFT                        4                                      /**< Shift value for CSEN_APORTCONFLICT */
#define _CSEN_IEN_APORTCONFLICT_MASK                         0x10UL                                 /**< Bit mask for CSEN_APORTCONFLICT */
#define _CSEN_IEN_APORTCONFLICT_DEFAULT                      0x00000000UL                           /**< Mode DEFAULT for CSEN_IEN */
#define CSEN_IEN_APORTCONFLICT_DEFAULT                       (_CSEN_IEN_APORTCONFLICT_DEFAULT << 4) /**< Shifted mode DEFAULT for CSEN_IEN */

/** @} */
/** @} End of group EFM32PG12B_CSEN */
/** @} End of group Parts */
