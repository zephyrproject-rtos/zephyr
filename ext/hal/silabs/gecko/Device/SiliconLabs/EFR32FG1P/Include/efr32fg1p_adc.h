/**************************************************************************//**
 * @file efr32fg1p_adc.h
 * @brief EFR32FG1P_ADC register and bit field definitions
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
 * @defgroup EFR32FG1P_ADC
 * @{
 * @brief EFR32FG1P_ADC Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;            /**< Control Register  */
  uint32_t       RESERVED0[1];    /**< Reserved for future use **/
  __IOM uint32_t CMD;             /**< Command Register  */
  __IM uint32_t  STATUS;          /**< Status Register  */
  __IOM uint32_t SINGLECTRL;      /**< Single Channel Control Register  */
  __IOM uint32_t SINGLECTRLX;     /**< Single Channel Control Register continued  */
  __IOM uint32_t SCANCTRL;        /**< Scan Control Register  */
  __IOM uint32_t SCANCTRLX;       /**< Scan Control Register continued  */
  __IOM uint32_t SCANMASK;        /**< Scan Sequence Input Mask Register  */
  __IOM uint32_t SCANINPUTSEL;    /**< Input Selection register for Scan mode  */
  __IOM uint32_t SCANNEGSEL;      /**< Negative Input select register for Scan  */
  __IOM uint32_t CMPTHR;          /**< Compare Threshold Register  */
  __IOM uint32_t BIASPROG;        /**< Bias Programming Register for various analog blocks used in ADC operation.  */
  __IOM uint32_t CAL;             /**< Calibration Register  */
  __IM uint32_t  IF;              /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;             /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;             /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;             /**< Interrupt Enable Register  */
  __IM uint32_t  SINGLEDATA;      /**< Single Conversion Result Data  */
  __IM uint32_t  SCANDATA;        /**< Scan Conversion Result Data  */
  __IM uint32_t  SINGLEDATAP;     /**< Single Conversion Result Data Peek Register  */
  __IM uint32_t  SCANDATAP;       /**< Scan Sequence Result Data Peek Register  */
  uint32_t       RESERVED1[4];    /**< Reserved for future use **/
  __IM uint32_t  SCANDATAX;       /**< Scan Sequence Result Data + Data Source Register  */
  __IM uint32_t  SCANDATAXP;      /**< Scan Sequence Result Data + Data Source Peek Register  */

  uint32_t       RESERVED2[3];    /**< Reserved for future use **/
  __IM uint32_t  APORTREQ;        /**< APORT Request Status Register  */
  __IM uint32_t  APORTCONFLICT;   /**< APORT Conflict Status Register  */
  __IM uint32_t  SINGLEFIFOCOUNT; /**< Single FIFO Count Register  */
  __IM uint32_t  SCANFIFOCOUNT;   /**< Scan FIFO Count Register  */
  __IOM uint32_t SINGLEFIFOCLEAR; /**< Single FIFO Clear Register  */
  __IOM uint32_t SCANFIFOCLEAR;   /**< Scan FIFO Clear Register  */
  __IOM uint32_t APORTMASTERDIS;  /**< APORT Bus Master Disable Register  */
} ADC_TypeDef;                    /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_ADC_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for ADC CTRL */
#define _ADC_CTRL_RESETVALUE                               0x001F0000UL                              /**< Default value for ADC_CTRL */
#define _ADC_CTRL_MASK                                     0x2F7F7FDFUL                              /**< Mask for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_SHIFT                         0                                         /**< Shift value for ADC_WARMUPMODE */
#define _ADC_CTRL_WARMUPMODE_MASK                          0x3UL                                     /**< Bit mask for ADC_WARMUPMODE */
#define _ADC_CTRL_WARMUPMODE_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_NORMAL                        0x00000000UL                              /**< Mode NORMAL for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_KEEPINSTANDBY                 0x00000001UL                              /**< Mode KEEPINSTANDBY for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_KEEPINSLOWACC                 0x00000002UL                              /**< Mode KEEPINSLOWACC for ADC_CTRL */
#define _ADC_CTRL_WARMUPMODE_KEEPADCWARM                   0x00000003UL                              /**< Mode KEEPADCWARM for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_DEFAULT                        (_ADC_CTRL_WARMUPMODE_DEFAULT << 0)       /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_NORMAL                         (_ADC_CTRL_WARMUPMODE_NORMAL << 0)        /**< Shifted mode NORMAL for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_KEEPINSTANDBY                  (_ADC_CTRL_WARMUPMODE_KEEPINSTANDBY << 0) /**< Shifted mode KEEPINSTANDBY for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_KEEPINSLOWACC                  (_ADC_CTRL_WARMUPMODE_KEEPINSLOWACC << 0) /**< Shifted mode KEEPINSLOWACC for ADC_CTRL */
#define ADC_CTRL_WARMUPMODE_KEEPADCWARM                    (_ADC_CTRL_WARMUPMODE_KEEPADCWARM << 0)   /**< Shifted mode KEEPADCWARM for ADC_CTRL */
#define ADC_CTRL_SINGLEDMAWU                               (0x1UL << 2)                              /**< SINGLEFIFO DMA Wakeup */
#define _ADC_CTRL_SINGLEDMAWU_SHIFT                        2                                         /**< Shift value for ADC_SINGLEDMAWU */
#define _ADC_CTRL_SINGLEDMAWU_MASK                         0x4UL                                     /**< Bit mask for ADC_SINGLEDMAWU */
#define _ADC_CTRL_SINGLEDMAWU_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_SINGLEDMAWU_DEFAULT                       (_ADC_CTRL_SINGLEDMAWU_DEFAULT << 2)      /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_SCANDMAWU                                 (0x1UL << 3)                              /**< SCANFIFO DMA Wakeup */
#define _ADC_CTRL_SCANDMAWU_SHIFT                          3                                         /**< Shift value for ADC_SCANDMAWU */
#define _ADC_CTRL_SCANDMAWU_MASK                           0x8UL                                     /**< Bit mask for ADC_SCANDMAWU */
#define _ADC_CTRL_SCANDMAWU_DEFAULT                        0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_SCANDMAWU_DEFAULT                         (_ADC_CTRL_SCANDMAWU_DEFAULT << 3)        /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_TAILGATE                                  (0x1UL << 4)                              /**< Conversion Tailgating */
#define _ADC_CTRL_TAILGATE_SHIFT                           4                                         /**< Shift value for ADC_TAILGATE */
#define _ADC_CTRL_TAILGATE_MASK                            0x10UL                                    /**< Bit mask for ADC_TAILGATE */
#define _ADC_CTRL_TAILGATE_DEFAULT                         0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_TAILGATE_DEFAULT                          (_ADC_CTRL_TAILGATE_DEFAULT << 4)         /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_ASYNCCLKEN                                (0x1UL << 6)                              /**< Selects ASYNC CLK enable mode when ADCCLKMODE=1 */
#define _ADC_CTRL_ASYNCCLKEN_SHIFT                         6                                         /**< Shift value for ADC_ASYNCCLKEN */
#define _ADC_CTRL_ASYNCCLKEN_MASK                          0x40UL                                    /**< Bit mask for ADC_ASYNCCLKEN */
#define _ADC_CTRL_ASYNCCLKEN_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_ASYNCCLKEN_ASNEEDED                      0x00000000UL                              /**< Mode ASNEEDED for ADC_CTRL */
#define _ADC_CTRL_ASYNCCLKEN_ALWAYSON                      0x00000001UL                              /**< Mode ALWAYSON for ADC_CTRL */
#define ADC_CTRL_ASYNCCLKEN_DEFAULT                        (_ADC_CTRL_ASYNCCLKEN_DEFAULT << 6)       /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_ASYNCCLKEN_ASNEEDED                       (_ADC_CTRL_ASYNCCLKEN_ASNEEDED << 6)      /**< Shifted mode ASNEEDED for ADC_CTRL */
#define ADC_CTRL_ASYNCCLKEN_ALWAYSON                       (_ADC_CTRL_ASYNCCLKEN_ALWAYSON << 6)      /**< Shifted mode ALWAYSON for ADC_CTRL */
#define ADC_CTRL_ADCCLKMODE                                (0x1UL << 7)                              /**< ADC Clock Mode */
#define _ADC_CTRL_ADCCLKMODE_SHIFT                         7                                         /**< Shift value for ADC_ADCCLKMODE */
#define _ADC_CTRL_ADCCLKMODE_MASK                          0x80UL                                    /**< Bit mask for ADC_ADCCLKMODE */
#define _ADC_CTRL_ADCCLKMODE_DEFAULT                       0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_ADCCLKMODE_SYNC                          0x00000000UL                              /**< Mode SYNC for ADC_CTRL */
#define _ADC_CTRL_ADCCLKMODE_ASYNC                         0x00000001UL                              /**< Mode ASYNC for ADC_CTRL */
#define ADC_CTRL_ADCCLKMODE_DEFAULT                        (_ADC_CTRL_ADCCLKMODE_DEFAULT << 7)       /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_ADCCLKMODE_SYNC                           (_ADC_CTRL_ADCCLKMODE_SYNC << 7)          /**< Shifted mode SYNC for ADC_CTRL */
#define ADC_CTRL_ADCCLKMODE_ASYNC                          (_ADC_CTRL_ADCCLKMODE_ASYNC << 7)         /**< Shifted mode ASYNC for ADC_CTRL */
#define _ADC_CTRL_PRESC_SHIFT                              8                                         /**< Shift value for ADC_PRESC */
#define _ADC_CTRL_PRESC_MASK                               0x7F00UL                                  /**< Bit mask for ADC_PRESC */
#define _ADC_CTRL_PRESC_DEFAULT                            0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_PRESC_NODIVISION                         0x00000000UL                              /**< Mode NODIVISION for ADC_CTRL */
#define ADC_CTRL_PRESC_DEFAULT                             (_ADC_CTRL_PRESC_DEFAULT << 8)            /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_PRESC_NODIVISION                          (_ADC_CTRL_PRESC_NODIVISION << 8)         /**< Shifted mode NODIVISION for ADC_CTRL */
#define _ADC_CTRL_TIMEBASE_SHIFT                           16                                        /**< Shift value for ADC_TIMEBASE */
#define _ADC_CTRL_TIMEBASE_MASK                            0x7F0000UL                                /**< Bit mask for ADC_TIMEBASE */
#define _ADC_CTRL_TIMEBASE_DEFAULT                         0x0000001FUL                              /**< Mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_TIMEBASE_DEFAULT                          (_ADC_CTRL_TIMEBASE_DEFAULT << 16)        /**< Shifted mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_SHIFT                            24                                        /**< Shift value for ADC_OVSRSEL */
#define _ADC_CTRL_OVSRSEL_MASK                             0xF000000UL                               /**< Bit mask for ADC_OVSRSEL */
#define _ADC_CTRL_OVSRSEL_DEFAULT                          0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X2                               0x00000000UL                              /**< Mode X2 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X4                               0x00000001UL                              /**< Mode X4 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X8                               0x00000002UL                              /**< Mode X8 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X16                              0x00000003UL                              /**< Mode X16 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X32                              0x00000004UL                              /**< Mode X32 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X64                              0x00000005UL                              /**< Mode X64 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X128                             0x00000006UL                              /**< Mode X128 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X256                             0x00000007UL                              /**< Mode X256 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X512                             0x00000008UL                              /**< Mode X512 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X1024                            0x00000009UL                              /**< Mode X1024 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X2048                            0x0000000AUL                              /**< Mode X2048 for ADC_CTRL */
#define _ADC_CTRL_OVSRSEL_X4096                            0x0000000BUL                              /**< Mode X4096 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_DEFAULT                           (_ADC_CTRL_OVSRSEL_DEFAULT << 24)         /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X2                                (_ADC_CTRL_OVSRSEL_X2 << 24)              /**< Shifted mode X2 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X4                                (_ADC_CTRL_OVSRSEL_X4 << 24)              /**< Shifted mode X4 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X8                                (_ADC_CTRL_OVSRSEL_X8 << 24)              /**< Shifted mode X8 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X16                               (_ADC_CTRL_OVSRSEL_X16 << 24)             /**< Shifted mode X16 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X32                               (_ADC_CTRL_OVSRSEL_X32 << 24)             /**< Shifted mode X32 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X64                               (_ADC_CTRL_OVSRSEL_X64 << 24)             /**< Shifted mode X64 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X128                              (_ADC_CTRL_OVSRSEL_X128 << 24)            /**< Shifted mode X128 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X256                              (_ADC_CTRL_OVSRSEL_X256 << 24)            /**< Shifted mode X256 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X512                              (_ADC_CTRL_OVSRSEL_X512 << 24)            /**< Shifted mode X512 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X1024                             (_ADC_CTRL_OVSRSEL_X1024 << 24)           /**< Shifted mode X1024 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X2048                             (_ADC_CTRL_OVSRSEL_X2048 << 24)           /**< Shifted mode X2048 for ADC_CTRL */
#define ADC_CTRL_OVSRSEL_X4096                             (_ADC_CTRL_OVSRSEL_X4096 << 24)           /**< Shifted mode X4096 for ADC_CTRL */
#define ADC_CTRL_CHCONMODE                                 (0x1UL << 29)                             /**< Channel Connect */
#define _ADC_CTRL_CHCONMODE_SHIFT                          29                                        /**< Shift value for ADC_CHCONMODE */
#define _ADC_CTRL_CHCONMODE_MASK                           0x20000000UL                              /**< Bit mask for ADC_CHCONMODE */
#define _ADC_CTRL_CHCONMODE_DEFAULT                        0x00000000UL                              /**< Mode DEFAULT for ADC_CTRL */
#define _ADC_CTRL_CHCONMODE_MAXSETTLE                      0x00000000UL                              /**< Mode MAXSETTLE for ADC_CTRL */
#define _ADC_CTRL_CHCONMODE_MAXRESP                        0x00000001UL                              /**< Mode MAXRESP for ADC_CTRL */
#define ADC_CTRL_CHCONMODE_DEFAULT                         (_ADC_CTRL_CHCONMODE_DEFAULT << 29)       /**< Shifted mode DEFAULT for ADC_CTRL */
#define ADC_CTRL_CHCONMODE_MAXSETTLE                       (_ADC_CTRL_CHCONMODE_MAXSETTLE << 29)     /**< Shifted mode MAXSETTLE for ADC_CTRL */
#define ADC_CTRL_CHCONMODE_MAXRESP                         (_ADC_CTRL_CHCONMODE_MAXRESP << 29)       /**< Shifted mode MAXRESP for ADC_CTRL */

/* Bit fields for ADC CMD */
#define _ADC_CMD_RESETVALUE                                0x00000000UL                        /**< Default value for ADC_CMD */
#define _ADC_CMD_MASK                                      0x0000000FUL                        /**< Mask for ADC_CMD */
#define ADC_CMD_SINGLESTART                                (0x1UL << 0)                        /**< Single Channel Conversion Start */
#define _ADC_CMD_SINGLESTART_SHIFT                         0                                   /**< Shift value for ADC_SINGLESTART */
#define _ADC_CMD_SINGLESTART_MASK                          0x1UL                               /**< Bit mask for ADC_SINGLESTART */
#define _ADC_CMD_SINGLESTART_DEFAULT                       0x00000000UL                        /**< Mode DEFAULT for ADC_CMD */
#define ADC_CMD_SINGLESTART_DEFAULT                        (_ADC_CMD_SINGLESTART_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_CMD */
#define ADC_CMD_SINGLESTOP                                 (0x1UL << 1)                        /**< Single Channel Conversion Stop */
#define _ADC_CMD_SINGLESTOP_SHIFT                          1                                   /**< Shift value for ADC_SINGLESTOP */
#define _ADC_CMD_SINGLESTOP_MASK                           0x2UL                               /**< Bit mask for ADC_SINGLESTOP */
#define _ADC_CMD_SINGLESTOP_DEFAULT                        0x00000000UL                        /**< Mode DEFAULT for ADC_CMD */
#define ADC_CMD_SINGLESTOP_DEFAULT                         (_ADC_CMD_SINGLESTOP_DEFAULT << 1)  /**< Shifted mode DEFAULT for ADC_CMD */
#define ADC_CMD_SCANSTART                                  (0x1UL << 2)                        /**< Scan Sequence Start */
#define _ADC_CMD_SCANSTART_SHIFT                           2                                   /**< Shift value for ADC_SCANSTART */
#define _ADC_CMD_SCANSTART_MASK                            0x4UL                               /**< Bit mask for ADC_SCANSTART */
#define _ADC_CMD_SCANSTART_DEFAULT                         0x00000000UL                        /**< Mode DEFAULT for ADC_CMD */
#define ADC_CMD_SCANSTART_DEFAULT                          (_ADC_CMD_SCANSTART_DEFAULT << 2)   /**< Shifted mode DEFAULT for ADC_CMD */
#define ADC_CMD_SCANSTOP                                   (0x1UL << 3)                        /**< Scan Sequence Stop */
#define _ADC_CMD_SCANSTOP_SHIFT                            3                                   /**< Shift value for ADC_SCANSTOP */
#define _ADC_CMD_SCANSTOP_MASK                             0x8UL                               /**< Bit mask for ADC_SCANSTOP */
#define _ADC_CMD_SCANSTOP_DEFAULT                          0x00000000UL                        /**< Mode DEFAULT for ADC_CMD */
#define ADC_CMD_SCANSTOP_DEFAULT                           (_ADC_CMD_SCANSTOP_DEFAULT << 3)    /**< Shifted mode DEFAULT for ADC_CMD */

/* Bit fields for ADC STATUS */
#define _ADC_STATUS_RESETVALUE                             0x00000000UL                             /**< Default value for ADC_STATUS */
#define _ADC_STATUS_MASK                                   0x00031F03UL                             /**< Mask for ADC_STATUS */
#define ADC_STATUS_SINGLEACT                               (0x1UL << 0)                             /**< Single Channel Conversion Active */
#define _ADC_STATUS_SINGLEACT_SHIFT                        0                                        /**< Shift value for ADC_SINGLEACT */
#define _ADC_STATUS_SINGLEACT_MASK                         0x1UL                                    /**< Bit mask for ADC_SINGLEACT */
#define _ADC_STATUS_SINGLEACT_DEFAULT                      0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEACT_DEFAULT                       (_ADC_STATUS_SINGLEACT_DEFAULT << 0)     /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANACT                                 (0x1UL << 1)                             /**< Scan Conversion Active */
#define _ADC_STATUS_SCANACT_SHIFT                          1                                        /**< Shift value for ADC_SCANACT */
#define _ADC_STATUS_SCANACT_MASK                           0x2UL                                    /**< Bit mask for ADC_SCANACT */
#define _ADC_STATUS_SCANACT_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANACT_DEFAULT                         (_ADC_STATUS_SCANACT_DEFAULT << 1)       /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEREFWARM                           (0x1UL << 8)                             /**< Single Channel Reference Warmed Up */
#define _ADC_STATUS_SINGLEREFWARM_SHIFT                    8                                        /**< Shift value for ADC_SINGLEREFWARM */
#define _ADC_STATUS_SINGLEREFWARM_MASK                     0x100UL                                  /**< Bit mask for ADC_SINGLEREFWARM */
#define _ADC_STATUS_SINGLEREFWARM_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEREFWARM_DEFAULT                   (_ADC_STATUS_SINGLEREFWARM_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANREFWARM                             (0x1UL << 9)                             /**< Scan Reference Warmed Up */
#define _ADC_STATUS_SCANREFWARM_SHIFT                      9                                        /**< Shift value for ADC_SCANREFWARM */
#define _ADC_STATUS_SCANREFWARM_MASK                       0x200UL                                  /**< Bit mask for ADC_SCANREFWARM */
#define _ADC_STATUS_SCANREFWARM_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANREFWARM_DEFAULT                     (_ADC_STATUS_SCANREFWARM_DEFAULT << 9)   /**< Shifted mode DEFAULT for ADC_STATUS */
#define _ADC_STATUS_PROGERR_SHIFT                          10                                       /**< Shift value for ADC_PROGERR */
#define _ADC_STATUS_PROGERR_MASK                           0xC00UL                                  /**< Bit mask for ADC_PROGERR */
#define _ADC_STATUS_PROGERR_DEFAULT                        0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define _ADC_STATUS_PROGERR_BUSCONF                        0x00000001UL                             /**< Mode BUSCONF for ADC_STATUS */
#define _ADC_STATUS_PROGERR_NEGSELCONF                     0x00000002UL                             /**< Mode NEGSELCONF for ADC_STATUS */
#define ADC_STATUS_PROGERR_DEFAULT                         (_ADC_STATUS_PROGERR_DEFAULT << 10)      /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_PROGERR_BUSCONF                         (_ADC_STATUS_PROGERR_BUSCONF << 10)      /**< Shifted mode BUSCONF for ADC_STATUS */
#define ADC_STATUS_PROGERR_NEGSELCONF                      (_ADC_STATUS_PROGERR_NEGSELCONF << 10)   /**< Shifted mode NEGSELCONF for ADC_STATUS */
#define ADC_STATUS_WARM                                    (0x1UL << 12)                            /**< ADC Warmed Up */
#define _ADC_STATUS_WARM_SHIFT                             12                                       /**< Shift value for ADC_WARM */
#define _ADC_STATUS_WARM_MASK                              0x1000UL                                 /**< Bit mask for ADC_WARM */
#define _ADC_STATUS_WARM_DEFAULT                           0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_WARM_DEFAULT                            (_ADC_STATUS_WARM_DEFAULT << 12)         /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEDV                                (0x1UL << 16)                            /**< Single Channel Data Valid */
#define _ADC_STATUS_SINGLEDV_SHIFT                         16                                       /**< Shift value for ADC_SINGLEDV */
#define _ADC_STATUS_SINGLEDV_MASK                          0x10000UL                                /**< Bit mask for ADC_SINGLEDV */
#define _ADC_STATUS_SINGLEDV_DEFAULT                       0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SINGLEDV_DEFAULT                        (_ADC_STATUS_SINGLEDV_DEFAULT << 16)     /**< Shifted mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANDV                                  (0x1UL << 17)                            /**< Scan Data Valid */
#define _ADC_STATUS_SCANDV_SHIFT                           17                                       /**< Shift value for ADC_SCANDV */
#define _ADC_STATUS_SCANDV_MASK                            0x20000UL                                /**< Bit mask for ADC_SCANDV */
#define _ADC_STATUS_SCANDV_DEFAULT                         0x00000000UL                             /**< Mode DEFAULT for ADC_STATUS */
#define ADC_STATUS_SCANDV_DEFAULT                          (_ADC_STATUS_SCANDV_DEFAULT << 17)       /**< Shifted mode DEFAULT for ADC_STATUS */

/* Bit fields for ADC SINGLECTRL */
#define _ADC_SINGLECTRL_RESETVALUE                         0x00FFFF00UL                               /**< Default value for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_MASK                               0xAFFFFFFFUL                               /**< Mask for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REP                                 (0x1UL << 0)                               /**< Single Channel Repetitive Mode */
#define _ADC_SINGLECTRL_REP_SHIFT                          0                                          /**< Shift value for ADC_REP */
#define _ADC_SINGLECTRL_REP_MASK                           0x1UL                                      /**< Bit mask for ADC_REP */
#define _ADC_SINGLECTRL_REP_DEFAULT                        0x00000000UL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REP_DEFAULT                         (_ADC_SINGLECTRL_REP_DEFAULT << 0)         /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_DIFF                                (0x1UL << 1)                               /**< Single Channel Differential Mode */
#define _ADC_SINGLECTRL_DIFF_SHIFT                         1                                          /**< Shift value for ADC_DIFF */
#define _ADC_SINGLECTRL_DIFF_MASK                          0x2UL                                      /**< Bit mask for ADC_DIFF */
#define _ADC_SINGLECTRL_DIFF_DEFAULT                       0x00000000UL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_DIFF_DEFAULT                        (_ADC_SINGLECTRL_DIFF_DEFAULT << 1)        /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_ADJ                                 (0x1UL << 2)                               /**< Single Channel Result Adjustment */
#define _ADC_SINGLECTRL_ADJ_SHIFT                          2                                          /**< Shift value for ADC_ADJ */
#define _ADC_SINGLECTRL_ADJ_MASK                           0x4UL                                      /**< Bit mask for ADC_ADJ */
#define _ADC_SINGLECTRL_ADJ_DEFAULT                        0x00000000UL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_ADJ_RIGHT                          0x00000000UL                               /**< Mode RIGHT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_ADJ_LEFT                           0x00000001UL                               /**< Mode LEFT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_ADJ_DEFAULT                         (_ADC_SINGLECTRL_ADJ_DEFAULT << 2)         /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_ADJ_RIGHT                           (_ADC_SINGLECTRL_ADJ_RIGHT << 2)           /**< Shifted mode RIGHT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_ADJ_LEFT                            (_ADC_SINGLECTRL_ADJ_LEFT << 2)            /**< Shifted mode LEFT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_SHIFT                          3                                          /**< Shift value for ADC_RES */
#define _ADC_SINGLECTRL_RES_MASK                           0x18UL                                     /**< Bit mask for ADC_RES */
#define _ADC_SINGLECTRL_RES_DEFAULT                        0x00000000UL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_12BIT                          0x00000000UL                               /**< Mode 12BIT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_8BIT                           0x00000001UL                               /**< Mode 8BIT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_6BIT                           0x00000002UL                               /**< Mode 6BIT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_RES_OVS                            0x00000003UL                               /**< Mode OVS for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_DEFAULT                         (_ADC_SINGLECTRL_RES_DEFAULT << 3)         /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_12BIT                           (_ADC_SINGLECTRL_RES_12BIT << 3)           /**< Shifted mode 12BIT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_8BIT                            (_ADC_SINGLECTRL_RES_8BIT << 3)            /**< Shifted mode 8BIT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_6BIT                            (_ADC_SINGLECTRL_RES_6BIT << 3)            /**< Shifted mode 6BIT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_RES_OVS                             (_ADC_SINGLECTRL_RES_OVS << 3)             /**< Shifted mode OVS for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_SHIFT                          5                                          /**< Shift value for ADC_REF */
#define _ADC_SINGLECTRL_REF_MASK                           0xE0UL                                     /**< Bit mask for ADC_REF */
#define _ADC_SINGLECTRL_REF_DEFAULT                        0x00000000UL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_1V25                           0x00000000UL                               /**< Mode 1V25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_2V5                            0x00000001UL                               /**< Mode 2V5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_VDD                            0x00000002UL                               /**< Mode VDD for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_5V                             0x00000003UL                               /**< Mode 5V for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_EXTSINGLE                      0x00000004UL                               /**< Mode EXTSINGLE for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_2XEXTDIFF                      0x00000005UL                               /**< Mode 2XEXTDIFF for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_2XVDD                          0x00000006UL                               /**< Mode 2XVDD for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_REF_CONF                           0x00000007UL                               /**< Mode CONF for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_DEFAULT                         (_ADC_SINGLECTRL_REF_DEFAULT << 5)         /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_1V25                            (_ADC_SINGLECTRL_REF_1V25 << 5)            /**< Shifted mode 1V25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_2V5                             (_ADC_SINGLECTRL_REF_2V5 << 5)             /**< Shifted mode 2V5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_VDD                             (_ADC_SINGLECTRL_REF_VDD << 5)             /**< Shifted mode VDD for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_5V                              (_ADC_SINGLECTRL_REF_5V << 5)              /**< Shifted mode 5V for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_EXTSINGLE                       (_ADC_SINGLECTRL_REF_EXTSINGLE << 5)       /**< Shifted mode EXTSINGLE for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_2XEXTDIFF                       (_ADC_SINGLECTRL_REF_2XEXTDIFF << 5)       /**< Shifted mode 2XEXTDIFF for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_2XVDD                           (_ADC_SINGLECTRL_REF_2XVDD << 5)           /**< Shifted mode 2XVDD for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_REF_CONF                            (_ADC_SINGLECTRL_REF_CONF << 5)            /**< Shifted mode CONF for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_SHIFT                       8                                          /**< Shift value for ADC_POSSEL */
#define _ADC_SINGLECTRL_POSSEL_MASK                        0xFF00UL                                   /**< Bit mask for ADC_POSSEL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH0                  0x00000000UL                               /**< Mode APORT0XCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH1                  0x00000001UL                               /**< Mode APORT0XCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH2                  0x00000002UL                               /**< Mode APORT0XCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH3                  0x00000003UL                               /**< Mode APORT0XCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH4                  0x00000004UL                               /**< Mode APORT0XCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH5                  0x00000005UL                               /**< Mode APORT0XCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH6                  0x00000006UL                               /**< Mode APORT0XCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH7                  0x00000007UL                               /**< Mode APORT0XCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH8                  0x00000008UL                               /**< Mode APORT0XCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH9                  0x00000009UL                               /**< Mode APORT0XCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH10                 0x0000000AUL                               /**< Mode APORT0XCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH11                 0x0000000BUL                               /**< Mode APORT0XCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH12                 0x0000000CUL                               /**< Mode APORT0XCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH13                 0x0000000DUL                               /**< Mode APORT0XCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH14                 0x0000000EUL                               /**< Mode APORT0XCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0XCH15                 0x0000000FUL                               /**< Mode APORT0XCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH0                  0x00000010UL                               /**< Mode APORT0YCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH1                  0x00000011UL                               /**< Mode APORT0YCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH2                  0x00000012UL                               /**< Mode APORT0YCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH3                  0x00000013UL                               /**< Mode APORT0YCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH4                  0x00000014UL                               /**< Mode APORT0YCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH5                  0x00000015UL                               /**< Mode APORT0YCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH6                  0x00000016UL                               /**< Mode APORT0YCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH7                  0x00000017UL                               /**< Mode APORT0YCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH8                  0x00000018UL                               /**< Mode APORT0YCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH9                  0x00000019UL                               /**< Mode APORT0YCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH10                 0x0000001AUL                               /**< Mode APORT0YCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH11                 0x0000001BUL                               /**< Mode APORT0YCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH12                 0x0000001CUL                               /**< Mode APORT0YCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH13                 0x0000001DUL                               /**< Mode APORT0YCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH14                 0x0000001EUL                               /**< Mode APORT0YCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT0YCH15                 0x0000001FUL                               /**< Mode APORT0YCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH0                  0x00000020UL                               /**< Mode APORT1XCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH1                  0x00000021UL                               /**< Mode APORT1YCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH2                  0x00000022UL                               /**< Mode APORT1XCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH3                  0x00000023UL                               /**< Mode APORT1YCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH4                  0x00000024UL                               /**< Mode APORT1XCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH5                  0x00000025UL                               /**< Mode APORT1YCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH6                  0x00000026UL                               /**< Mode APORT1XCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH7                  0x00000027UL                               /**< Mode APORT1YCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH8                  0x00000028UL                               /**< Mode APORT1XCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH9                  0x00000029UL                               /**< Mode APORT1YCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH10                 0x0000002AUL                               /**< Mode APORT1XCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH11                 0x0000002BUL                               /**< Mode APORT1YCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH12                 0x0000002CUL                               /**< Mode APORT1XCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH13                 0x0000002DUL                               /**< Mode APORT1YCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH14                 0x0000002EUL                               /**< Mode APORT1XCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH15                 0x0000002FUL                               /**< Mode APORT1YCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH16                 0x00000030UL                               /**< Mode APORT1XCH16 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH17                 0x00000031UL                               /**< Mode APORT1YCH17 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH18                 0x00000032UL                               /**< Mode APORT1XCH18 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH19                 0x00000033UL                               /**< Mode APORT1YCH19 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH20                 0x00000034UL                               /**< Mode APORT1XCH20 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH21                 0x00000035UL                               /**< Mode APORT1YCH21 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH22                 0x00000036UL                               /**< Mode APORT1XCH22 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH23                 0x00000037UL                               /**< Mode APORT1YCH23 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH24                 0x00000038UL                               /**< Mode APORT1XCH24 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH25                 0x00000039UL                               /**< Mode APORT1YCH25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH26                 0x0000003AUL                               /**< Mode APORT1XCH26 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH27                 0x0000003BUL                               /**< Mode APORT1YCH27 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH28                 0x0000003CUL                               /**< Mode APORT1XCH28 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH29                 0x0000003DUL                               /**< Mode APORT1YCH29 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1XCH30                 0x0000003EUL                               /**< Mode APORT1XCH30 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT1YCH31                 0x0000003FUL                               /**< Mode APORT1YCH31 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH0                  0x00000040UL                               /**< Mode APORT2YCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH1                  0x00000041UL                               /**< Mode APORT2XCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH2                  0x00000042UL                               /**< Mode APORT2YCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH3                  0x00000043UL                               /**< Mode APORT2XCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH4                  0x00000044UL                               /**< Mode APORT2YCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH5                  0x00000045UL                               /**< Mode APORT2XCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH6                  0x00000046UL                               /**< Mode APORT2YCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH7                  0x00000047UL                               /**< Mode APORT2XCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH8                  0x00000048UL                               /**< Mode APORT2YCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH9                  0x00000049UL                               /**< Mode APORT2XCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH10                 0x0000004AUL                               /**< Mode APORT2YCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH11                 0x0000004BUL                               /**< Mode APORT2XCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH12                 0x0000004CUL                               /**< Mode APORT2YCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH13                 0x0000004DUL                               /**< Mode APORT2XCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH14                 0x0000004EUL                               /**< Mode APORT2YCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH15                 0x0000004FUL                               /**< Mode APORT2XCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH16                 0x00000050UL                               /**< Mode APORT2YCH16 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH17                 0x00000051UL                               /**< Mode APORT2XCH17 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH18                 0x00000052UL                               /**< Mode APORT2YCH18 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH19                 0x00000053UL                               /**< Mode APORT2XCH19 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH20                 0x00000054UL                               /**< Mode APORT2YCH20 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH21                 0x00000055UL                               /**< Mode APORT2XCH21 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH22                 0x00000056UL                               /**< Mode APORT2YCH22 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH23                 0x00000057UL                               /**< Mode APORT2XCH23 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH24                 0x00000058UL                               /**< Mode APORT2YCH24 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH25                 0x00000059UL                               /**< Mode APORT2XCH25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH26                 0x0000005AUL                               /**< Mode APORT2YCH26 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH27                 0x0000005BUL                               /**< Mode APORT2XCH27 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH28                 0x0000005CUL                               /**< Mode APORT2YCH28 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH29                 0x0000005DUL                               /**< Mode APORT2XCH29 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2YCH30                 0x0000005EUL                               /**< Mode APORT2YCH30 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT2XCH31                 0x0000005FUL                               /**< Mode APORT2XCH31 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH0                  0x00000060UL                               /**< Mode APORT3XCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH1                  0x00000061UL                               /**< Mode APORT3YCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH2                  0x00000062UL                               /**< Mode APORT3XCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH3                  0x00000063UL                               /**< Mode APORT3YCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH4                  0x00000064UL                               /**< Mode APORT3XCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH5                  0x00000065UL                               /**< Mode APORT3YCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH6                  0x00000066UL                               /**< Mode APORT3XCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH7                  0x00000067UL                               /**< Mode APORT3YCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH8                  0x00000068UL                               /**< Mode APORT3XCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH9                  0x00000069UL                               /**< Mode APORT3YCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH10                 0x0000006AUL                               /**< Mode APORT3XCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH11                 0x0000006BUL                               /**< Mode APORT3YCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH12                 0x0000006CUL                               /**< Mode APORT3XCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH13                 0x0000006DUL                               /**< Mode APORT3YCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH14                 0x0000006EUL                               /**< Mode APORT3XCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH15                 0x0000006FUL                               /**< Mode APORT3YCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH16                 0x00000070UL                               /**< Mode APORT3XCH16 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH17                 0x00000071UL                               /**< Mode APORT3YCH17 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH18                 0x00000072UL                               /**< Mode APORT3XCH18 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH19                 0x00000073UL                               /**< Mode APORT3YCH19 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH20                 0x00000074UL                               /**< Mode APORT3XCH20 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH21                 0x00000075UL                               /**< Mode APORT3YCH21 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH22                 0x00000076UL                               /**< Mode APORT3XCH22 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH23                 0x00000077UL                               /**< Mode APORT3YCH23 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH24                 0x00000078UL                               /**< Mode APORT3XCH24 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH25                 0x00000079UL                               /**< Mode APORT3YCH25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH26                 0x0000007AUL                               /**< Mode APORT3XCH26 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH27                 0x0000007BUL                               /**< Mode APORT3YCH27 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH28                 0x0000007CUL                               /**< Mode APORT3XCH28 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH29                 0x0000007DUL                               /**< Mode APORT3YCH29 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3XCH30                 0x0000007EUL                               /**< Mode APORT3XCH30 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT3YCH31                 0x0000007FUL                               /**< Mode APORT3YCH31 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH0                  0x00000080UL                               /**< Mode APORT4YCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH1                  0x00000081UL                               /**< Mode APORT4XCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH2                  0x00000082UL                               /**< Mode APORT4YCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH3                  0x00000083UL                               /**< Mode APORT4XCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH4                  0x00000084UL                               /**< Mode APORT4YCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH5                  0x00000085UL                               /**< Mode APORT4XCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH6                  0x00000086UL                               /**< Mode APORT4YCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH7                  0x00000087UL                               /**< Mode APORT4XCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH8                  0x00000088UL                               /**< Mode APORT4YCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH9                  0x00000089UL                               /**< Mode APORT4XCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH10                 0x0000008AUL                               /**< Mode APORT4YCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH11                 0x0000008BUL                               /**< Mode APORT4XCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH12                 0x0000008CUL                               /**< Mode APORT4YCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH13                 0x0000008DUL                               /**< Mode APORT4XCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH14                 0x0000008EUL                               /**< Mode APORT4YCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH15                 0x0000008FUL                               /**< Mode APORT4XCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH16                 0x00000090UL                               /**< Mode APORT4YCH16 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH17                 0x00000091UL                               /**< Mode APORT4XCH17 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH18                 0x00000092UL                               /**< Mode APORT4YCH18 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH19                 0x00000093UL                               /**< Mode APORT4XCH19 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH20                 0x00000094UL                               /**< Mode APORT4YCH20 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH21                 0x00000095UL                               /**< Mode APORT4XCH21 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH22                 0x00000096UL                               /**< Mode APORT4YCH22 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH23                 0x00000097UL                               /**< Mode APORT4XCH23 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH24                 0x00000098UL                               /**< Mode APORT4YCH24 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH25                 0x00000099UL                               /**< Mode APORT4XCH25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH26                 0x0000009AUL                               /**< Mode APORT4YCH26 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH27                 0x0000009BUL                               /**< Mode APORT4XCH27 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH28                 0x0000009CUL                               /**< Mode APORT4YCH28 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH29                 0x0000009DUL                               /**< Mode APORT4XCH29 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4YCH30                 0x0000009EUL                               /**< Mode APORT4YCH30 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_APORT4XCH31                 0x0000009FUL                               /**< Mode APORT4XCH31 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_AVDD                        0x000000E0UL                               /**< Mode AVDD for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_BU                          0x000000E1UL                               /**< Mode BU for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_AREG                        0x000000E2UL                               /**< Mode AREG for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_VREGOUTPA                   0x000000E3UL                               /**< Mode VREGOUTPA for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_PDBU                        0x000000E4UL                               /**< Mode PDBU for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_IO0                         0x000000E5UL                               /**< Mode IO0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_IO1                         0x000000E6UL                               /**< Mode IO1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_VSP                         0x000000E7UL                               /**< Mode VSP for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_OPA2                        0x000000F2UL                               /**< Mode OPA2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_TEMP                        0x000000F3UL                               /**< Mode TEMP for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_DAC0OUT0                    0x000000F4UL                               /**< Mode DAC0OUT0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_TESTP                       0x000000F5UL                               /**< Mode TESTP for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_SP1                         0x000000F6UL                               /**< Mode SP1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_SP2                         0x000000F7UL                               /**< Mode SP2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_DAC0OUT1                    0x000000F8UL                               /**< Mode DAC0OUT1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_SUBLSB                      0x000000F9UL                               /**< Mode SUBLSB for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_OPA3                        0x000000FAUL                               /**< Mode OPA3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_DEFAULT                     0x000000FFUL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_POSSEL_VSS                         0x000000FFUL                               /**< Mode VSS for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH0                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH0 << 8)   /**< Shifted mode APORT0XCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH1                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH1 << 8)   /**< Shifted mode APORT0XCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH2                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH2 << 8)   /**< Shifted mode APORT0XCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH3                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH3 << 8)   /**< Shifted mode APORT0XCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH4                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH4 << 8)   /**< Shifted mode APORT0XCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH5                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH5 << 8)   /**< Shifted mode APORT0XCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH6                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH6 << 8)   /**< Shifted mode APORT0XCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH7                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH7 << 8)   /**< Shifted mode APORT0XCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH8                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH8 << 8)   /**< Shifted mode APORT0XCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH9                   (_ADC_SINGLECTRL_POSSEL_APORT0XCH9 << 8)   /**< Shifted mode APORT0XCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH10                  (_ADC_SINGLECTRL_POSSEL_APORT0XCH10 << 8)  /**< Shifted mode APORT0XCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH11                  (_ADC_SINGLECTRL_POSSEL_APORT0XCH11 << 8)  /**< Shifted mode APORT0XCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH12                  (_ADC_SINGLECTRL_POSSEL_APORT0XCH12 << 8)  /**< Shifted mode APORT0XCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH13                  (_ADC_SINGLECTRL_POSSEL_APORT0XCH13 << 8)  /**< Shifted mode APORT0XCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH14                  (_ADC_SINGLECTRL_POSSEL_APORT0XCH14 << 8)  /**< Shifted mode APORT0XCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0XCH15                  (_ADC_SINGLECTRL_POSSEL_APORT0XCH15 << 8)  /**< Shifted mode APORT0XCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH0                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH0 << 8)   /**< Shifted mode APORT0YCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH1                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH1 << 8)   /**< Shifted mode APORT0YCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH2                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH2 << 8)   /**< Shifted mode APORT0YCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH3                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH3 << 8)   /**< Shifted mode APORT0YCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH4                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH4 << 8)   /**< Shifted mode APORT0YCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH5                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH5 << 8)   /**< Shifted mode APORT0YCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH6                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH6 << 8)   /**< Shifted mode APORT0YCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH7                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH7 << 8)   /**< Shifted mode APORT0YCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH8                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH8 << 8)   /**< Shifted mode APORT0YCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH9                   (_ADC_SINGLECTRL_POSSEL_APORT0YCH9 << 8)   /**< Shifted mode APORT0YCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH10                  (_ADC_SINGLECTRL_POSSEL_APORT0YCH10 << 8)  /**< Shifted mode APORT0YCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH11                  (_ADC_SINGLECTRL_POSSEL_APORT0YCH11 << 8)  /**< Shifted mode APORT0YCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH12                  (_ADC_SINGLECTRL_POSSEL_APORT0YCH12 << 8)  /**< Shifted mode APORT0YCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH13                  (_ADC_SINGLECTRL_POSSEL_APORT0YCH13 << 8)  /**< Shifted mode APORT0YCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH14                  (_ADC_SINGLECTRL_POSSEL_APORT0YCH14 << 8)  /**< Shifted mode APORT0YCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT0YCH15                  (_ADC_SINGLECTRL_POSSEL_APORT0YCH15 << 8)  /**< Shifted mode APORT0YCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH0                   (_ADC_SINGLECTRL_POSSEL_APORT1XCH0 << 8)   /**< Shifted mode APORT1XCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH1                   (_ADC_SINGLECTRL_POSSEL_APORT1YCH1 << 8)   /**< Shifted mode APORT1YCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH2                   (_ADC_SINGLECTRL_POSSEL_APORT1XCH2 << 8)   /**< Shifted mode APORT1XCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH3                   (_ADC_SINGLECTRL_POSSEL_APORT1YCH3 << 8)   /**< Shifted mode APORT1YCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH4                   (_ADC_SINGLECTRL_POSSEL_APORT1XCH4 << 8)   /**< Shifted mode APORT1XCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH5                   (_ADC_SINGLECTRL_POSSEL_APORT1YCH5 << 8)   /**< Shifted mode APORT1YCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH6                   (_ADC_SINGLECTRL_POSSEL_APORT1XCH6 << 8)   /**< Shifted mode APORT1XCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH7                   (_ADC_SINGLECTRL_POSSEL_APORT1YCH7 << 8)   /**< Shifted mode APORT1YCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH8                   (_ADC_SINGLECTRL_POSSEL_APORT1XCH8 << 8)   /**< Shifted mode APORT1XCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH9                   (_ADC_SINGLECTRL_POSSEL_APORT1YCH9 << 8)   /**< Shifted mode APORT1YCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH10                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH10 << 8)  /**< Shifted mode APORT1XCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH11                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH11 << 8)  /**< Shifted mode APORT1YCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH12                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH12 << 8)  /**< Shifted mode APORT1XCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH13                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH13 << 8)  /**< Shifted mode APORT1YCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH14                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH14 << 8)  /**< Shifted mode APORT1XCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH15                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH15 << 8)  /**< Shifted mode APORT1YCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH16                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH16 << 8)  /**< Shifted mode APORT1XCH16 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH17                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH17 << 8)  /**< Shifted mode APORT1YCH17 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH18                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH18 << 8)  /**< Shifted mode APORT1XCH18 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH19                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH19 << 8)  /**< Shifted mode APORT1YCH19 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH20                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH20 << 8)  /**< Shifted mode APORT1XCH20 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH21                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH21 << 8)  /**< Shifted mode APORT1YCH21 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH22                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH22 << 8)  /**< Shifted mode APORT1XCH22 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH23                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH23 << 8)  /**< Shifted mode APORT1YCH23 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH24                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH24 << 8)  /**< Shifted mode APORT1XCH24 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH25                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH25 << 8)  /**< Shifted mode APORT1YCH25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH26                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH26 << 8)  /**< Shifted mode APORT1XCH26 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH27                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH27 << 8)  /**< Shifted mode APORT1YCH27 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH28                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH28 << 8)  /**< Shifted mode APORT1XCH28 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH29                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH29 << 8)  /**< Shifted mode APORT1YCH29 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1XCH30                  (_ADC_SINGLECTRL_POSSEL_APORT1XCH30 << 8)  /**< Shifted mode APORT1XCH30 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT1YCH31                  (_ADC_SINGLECTRL_POSSEL_APORT1YCH31 << 8)  /**< Shifted mode APORT1YCH31 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH0                   (_ADC_SINGLECTRL_POSSEL_APORT2YCH0 << 8)   /**< Shifted mode APORT2YCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH1                   (_ADC_SINGLECTRL_POSSEL_APORT2XCH1 << 8)   /**< Shifted mode APORT2XCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH2                   (_ADC_SINGLECTRL_POSSEL_APORT2YCH2 << 8)   /**< Shifted mode APORT2YCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH3                   (_ADC_SINGLECTRL_POSSEL_APORT2XCH3 << 8)   /**< Shifted mode APORT2XCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH4                   (_ADC_SINGLECTRL_POSSEL_APORT2YCH4 << 8)   /**< Shifted mode APORT2YCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH5                   (_ADC_SINGLECTRL_POSSEL_APORT2XCH5 << 8)   /**< Shifted mode APORT2XCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH6                   (_ADC_SINGLECTRL_POSSEL_APORT2YCH6 << 8)   /**< Shifted mode APORT2YCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH7                   (_ADC_SINGLECTRL_POSSEL_APORT2XCH7 << 8)   /**< Shifted mode APORT2XCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH8                   (_ADC_SINGLECTRL_POSSEL_APORT2YCH8 << 8)   /**< Shifted mode APORT2YCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH9                   (_ADC_SINGLECTRL_POSSEL_APORT2XCH9 << 8)   /**< Shifted mode APORT2XCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH10                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH10 << 8)  /**< Shifted mode APORT2YCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH11                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH11 << 8)  /**< Shifted mode APORT2XCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH12                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH12 << 8)  /**< Shifted mode APORT2YCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH13                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH13 << 8)  /**< Shifted mode APORT2XCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH14                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH14 << 8)  /**< Shifted mode APORT2YCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH15                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH15 << 8)  /**< Shifted mode APORT2XCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH16                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH16 << 8)  /**< Shifted mode APORT2YCH16 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH17                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH17 << 8)  /**< Shifted mode APORT2XCH17 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH18                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH18 << 8)  /**< Shifted mode APORT2YCH18 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH19                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH19 << 8)  /**< Shifted mode APORT2XCH19 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH20                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH20 << 8)  /**< Shifted mode APORT2YCH20 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH21                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH21 << 8)  /**< Shifted mode APORT2XCH21 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH22                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH22 << 8)  /**< Shifted mode APORT2YCH22 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH23                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH23 << 8)  /**< Shifted mode APORT2XCH23 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH24                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH24 << 8)  /**< Shifted mode APORT2YCH24 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH25                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH25 << 8)  /**< Shifted mode APORT2XCH25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH26                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH26 << 8)  /**< Shifted mode APORT2YCH26 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH27                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH27 << 8)  /**< Shifted mode APORT2XCH27 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH28                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH28 << 8)  /**< Shifted mode APORT2YCH28 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH29                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH29 << 8)  /**< Shifted mode APORT2XCH29 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2YCH30                  (_ADC_SINGLECTRL_POSSEL_APORT2YCH30 << 8)  /**< Shifted mode APORT2YCH30 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT2XCH31                  (_ADC_SINGLECTRL_POSSEL_APORT2XCH31 << 8)  /**< Shifted mode APORT2XCH31 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH0                   (_ADC_SINGLECTRL_POSSEL_APORT3XCH0 << 8)   /**< Shifted mode APORT3XCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH1                   (_ADC_SINGLECTRL_POSSEL_APORT3YCH1 << 8)   /**< Shifted mode APORT3YCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH2                   (_ADC_SINGLECTRL_POSSEL_APORT3XCH2 << 8)   /**< Shifted mode APORT3XCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH3                   (_ADC_SINGLECTRL_POSSEL_APORT3YCH3 << 8)   /**< Shifted mode APORT3YCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH4                   (_ADC_SINGLECTRL_POSSEL_APORT3XCH4 << 8)   /**< Shifted mode APORT3XCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH5                   (_ADC_SINGLECTRL_POSSEL_APORT3YCH5 << 8)   /**< Shifted mode APORT3YCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH6                   (_ADC_SINGLECTRL_POSSEL_APORT3XCH6 << 8)   /**< Shifted mode APORT3XCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH7                   (_ADC_SINGLECTRL_POSSEL_APORT3YCH7 << 8)   /**< Shifted mode APORT3YCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH8                   (_ADC_SINGLECTRL_POSSEL_APORT3XCH8 << 8)   /**< Shifted mode APORT3XCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH9                   (_ADC_SINGLECTRL_POSSEL_APORT3YCH9 << 8)   /**< Shifted mode APORT3YCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH10                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH10 << 8)  /**< Shifted mode APORT3XCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH11                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH11 << 8)  /**< Shifted mode APORT3YCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH12                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH12 << 8)  /**< Shifted mode APORT3XCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH13                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH13 << 8)  /**< Shifted mode APORT3YCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH14                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH14 << 8)  /**< Shifted mode APORT3XCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH15                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH15 << 8)  /**< Shifted mode APORT3YCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH16                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH16 << 8)  /**< Shifted mode APORT3XCH16 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH17                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH17 << 8)  /**< Shifted mode APORT3YCH17 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH18                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH18 << 8)  /**< Shifted mode APORT3XCH18 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH19                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH19 << 8)  /**< Shifted mode APORT3YCH19 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH20                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH20 << 8)  /**< Shifted mode APORT3XCH20 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH21                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH21 << 8)  /**< Shifted mode APORT3YCH21 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH22                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH22 << 8)  /**< Shifted mode APORT3XCH22 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH23                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH23 << 8)  /**< Shifted mode APORT3YCH23 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH24                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH24 << 8)  /**< Shifted mode APORT3XCH24 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH25                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH25 << 8)  /**< Shifted mode APORT3YCH25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH26                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH26 << 8)  /**< Shifted mode APORT3XCH26 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH27                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH27 << 8)  /**< Shifted mode APORT3YCH27 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH28                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH28 << 8)  /**< Shifted mode APORT3XCH28 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH29                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH29 << 8)  /**< Shifted mode APORT3YCH29 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3XCH30                  (_ADC_SINGLECTRL_POSSEL_APORT3XCH30 << 8)  /**< Shifted mode APORT3XCH30 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT3YCH31                  (_ADC_SINGLECTRL_POSSEL_APORT3YCH31 << 8)  /**< Shifted mode APORT3YCH31 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH0                   (_ADC_SINGLECTRL_POSSEL_APORT4YCH0 << 8)   /**< Shifted mode APORT4YCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH1                   (_ADC_SINGLECTRL_POSSEL_APORT4XCH1 << 8)   /**< Shifted mode APORT4XCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH2                   (_ADC_SINGLECTRL_POSSEL_APORT4YCH2 << 8)   /**< Shifted mode APORT4YCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH3                   (_ADC_SINGLECTRL_POSSEL_APORT4XCH3 << 8)   /**< Shifted mode APORT4XCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH4                   (_ADC_SINGLECTRL_POSSEL_APORT4YCH4 << 8)   /**< Shifted mode APORT4YCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH5                   (_ADC_SINGLECTRL_POSSEL_APORT4XCH5 << 8)   /**< Shifted mode APORT4XCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH6                   (_ADC_SINGLECTRL_POSSEL_APORT4YCH6 << 8)   /**< Shifted mode APORT4YCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH7                   (_ADC_SINGLECTRL_POSSEL_APORT4XCH7 << 8)   /**< Shifted mode APORT4XCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH8                   (_ADC_SINGLECTRL_POSSEL_APORT4YCH8 << 8)   /**< Shifted mode APORT4YCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH9                   (_ADC_SINGLECTRL_POSSEL_APORT4XCH9 << 8)   /**< Shifted mode APORT4XCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH10                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH10 << 8)  /**< Shifted mode APORT4YCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH11                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH11 << 8)  /**< Shifted mode APORT4XCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH12                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH12 << 8)  /**< Shifted mode APORT4YCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH13                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH13 << 8)  /**< Shifted mode APORT4XCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH14                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH14 << 8)  /**< Shifted mode APORT4YCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH15                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH15 << 8)  /**< Shifted mode APORT4XCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH16                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH16 << 8)  /**< Shifted mode APORT4YCH16 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH17                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH17 << 8)  /**< Shifted mode APORT4XCH17 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH18                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH18 << 8)  /**< Shifted mode APORT4YCH18 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH19                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH19 << 8)  /**< Shifted mode APORT4XCH19 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH20                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH20 << 8)  /**< Shifted mode APORT4YCH20 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH21                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH21 << 8)  /**< Shifted mode APORT4XCH21 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH22                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH22 << 8)  /**< Shifted mode APORT4YCH22 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH23                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH23 << 8)  /**< Shifted mode APORT4XCH23 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH24                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH24 << 8)  /**< Shifted mode APORT4YCH24 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH25                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH25 << 8)  /**< Shifted mode APORT4XCH25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH26                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH26 << 8)  /**< Shifted mode APORT4YCH26 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH27                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH27 << 8)  /**< Shifted mode APORT4XCH27 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH28                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH28 << 8)  /**< Shifted mode APORT4YCH28 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH29                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH29 << 8)  /**< Shifted mode APORT4XCH29 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4YCH30                  (_ADC_SINGLECTRL_POSSEL_APORT4YCH30 << 8)  /**< Shifted mode APORT4YCH30 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_APORT4XCH31                  (_ADC_SINGLECTRL_POSSEL_APORT4XCH31 << 8)  /**< Shifted mode APORT4XCH31 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_AVDD                         (_ADC_SINGLECTRL_POSSEL_AVDD << 8)         /**< Shifted mode AVDD for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_BU                           (_ADC_SINGLECTRL_POSSEL_BU << 8)           /**< Shifted mode BU for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_AREG                         (_ADC_SINGLECTRL_POSSEL_AREG << 8)         /**< Shifted mode AREG for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_VREGOUTPA                    (_ADC_SINGLECTRL_POSSEL_VREGOUTPA << 8)    /**< Shifted mode VREGOUTPA for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_PDBU                         (_ADC_SINGLECTRL_POSSEL_PDBU << 8)         /**< Shifted mode PDBU for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_IO0                          (_ADC_SINGLECTRL_POSSEL_IO0 << 8)          /**< Shifted mode IO0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_IO1                          (_ADC_SINGLECTRL_POSSEL_IO1 << 8)          /**< Shifted mode IO1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_VSP                          (_ADC_SINGLECTRL_POSSEL_VSP << 8)          /**< Shifted mode VSP for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_OPA2                         (_ADC_SINGLECTRL_POSSEL_OPA2 << 8)         /**< Shifted mode OPA2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_TEMP                         (_ADC_SINGLECTRL_POSSEL_TEMP << 8)         /**< Shifted mode TEMP for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_DAC0OUT0                     (_ADC_SINGLECTRL_POSSEL_DAC0OUT0 << 8)     /**< Shifted mode DAC0OUT0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_TESTP                        (_ADC_SINGLECTRL_POSSEL_TESTP << 8)        /**< Shifted mode TESTP for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_SP1                          (_ADC_SINGLECTRL_POSSEL_SP1 << 8)          /**< Shifted mode SP1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_SP2                          (_ADC_SINGLECTRL_POSSEL_SP2 << 8)          /**< Shifted mode SP2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_DAC0OUT1                     (_ADC_SINGLECTRL_POSSEL_DAC0OUT1 << 8)     /**< Shifted mode DAC0OUT1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_SUBLSB                       (_ADC_SINGLECTRL_POSSEL_SUBLSB << 8)       /**< Shifted mode SUBLSB for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_OPA3                         (_ADC_SINGLECTRL_POSSEL_OPA3 << 8)         /**< Shifted mode OPA3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_DEFAULT                      (_ADC_SINGLECTRL_POSSEL_DEFAULT << 8)      /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_POSSEL_VSS                          (_ADC_SINGLECTRL_POSSEL_VSS << 8)          /**< Shifted mode VSS for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_SHIFT                       16                                         /**< Shift value for ADC_NEGSEL */
#define _ADC_SINGLECTRL_NEGSEL_MASK                        0xFF0000UL                                 /**< Bit mask for ADC_NEGSEL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH0                  0x00000000UL                               /**< Mode APORT0XCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH1                  0x00000001UL                               /**< Mode APORT0XCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH2                  0x00000002UL                               /**< Mode APORT0XCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH3                  0x00000003UL                               /**< Mode APORT0XCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH4                  0x00000004UL                               /**< Mode APORT0XCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH5                  0x00000005UL                               /**< Mode APORT0XCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH6                  0x00000006UL                               /**< Mode APORT0XCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH7                  0x00000007UL                               /**< Mode APORT0XCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH8                  0x00000008UL                               /**< Mode APORT0XCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH9                  0x00000009UL                               /**< Mode APORT0XCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH10                 0x0000000AUL                               /**< Mode APORT0XCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH11                 0x0000000BUL                               /**< Mode APORT0XCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH12                 0x0000000CUL                               /**< Mode APORT0XCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH13                 0x0000000DUL                               /**< Mode APORT0XCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH14                 0x0000000EUL                               /**< Mode APORT0XCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0XCH15                 0x0000000FUL                               /**< Mode APORT0XCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH0                  0x00000010UL                               /**< Mode APORT0YCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH1                  0x00000011UL                               /**< Mode APORT0YCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH2                  0x00000012UL                               /**< Mode APORT0YCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH3                  0x00000013UL                               /**< Mode APORT0YCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH4                  0x00000014UL                               /**< Mode APORT0YCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH5                  0x00000015UL                               /**< Mode APORT0YCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH6                  0x00000016UL                               /**< Mode APORT0YCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH7                  0x00000017UL                               /**< Mode APORT0YCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH8                  0x00000018UL                               /**< Mode APORT0YCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH9                  0x00000019UL                               /**< Mode APORT0YCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH10                 0x0000001AUL                               /**< Mode APORT0YCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH11                 0x0000001BUL                               /**< Mode APORT0YCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH12                 0x0000001CUL                               /**< Mode APORT0YCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH13                 0x0000001DUL                               /**< Mode APORT0YCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH14                 0x0000001EUL                               /**< Mode APORT0YCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT0YCH15                 0x0000001FUL                               /**< Mode APORT0YCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH0                  0x00000020UL                               /**< Mode APORT1XCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH1                  0x00000021UL                               /**< Mode APORT1YCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH2                  0x00000022UL                               /**< Mode APORT1XCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH3                  0x00000023UL                               /**< Mode APORT1YCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH4                  0x00000024UL                               /**< Mode APORT1XCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH5                  0x00000025UL                               /**< Mode APORT1YCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH6                  0x00000026UL                               /**< Mode APORT1XCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH7                  0x00000027UL                               /**< Mode APORT1YCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH8                  0x00000028UL                               /**< Mode APORT1XCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH9                  0x00000029UL                               /**< Mode APORT1YCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH10                 0x0000002AUL                               /**< Mode APORT1XCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH11                 0x0000002BUL                               /**< Mode APORT1YCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH12                 0x0000002CUL                               /**< Mode APORT1XCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH13                 0x0000002DUL                               /**< Mode APORT1YCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH14                 0x0000002EUL                               /**< Mode APORT1XCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH15                 0x0000002FUL                               /**< Mode APORT1YCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH16                 0x00000030UL                               /**< Mode APORT1XCH16 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH17                 0x00000031UL                               /**< Mode APORT1YCH17 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH18                 0x00000032UL                               /**< Mode APORT1XCH18 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH19                 0x00000033UL                               /**< Mode APORT1YCH19 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH20                 0x00000034UL                               /**< Mode APORT1XCH20 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH21                 0x00000035UL                               /**< Mode APORT1YCH21 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH22                 0x00000036UL                               /**< Mode APORT1XCH22 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH23                 0x00000037UL                               /**< Mode APORT1YCH23 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH24                 0x00000038UL                               /**< Mode APORT1XCH24 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH25                 0x00000039UL                               /**< Mode APORT1YCH25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH26                 0x0000003AUL                               /**< Mode APORT1XCH26 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH27                 0x0000003BUL                               /**< Mode APORT1YCH27 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH28                 0x0000003CUL                               /**< Mode APORT1XCH28 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH29                 0x0000003DUL                               /**< Mode APORT1YCH29 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1XCH30                 0x0000003EUL                               /**< Mode APORT1XCH30 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT1YCH31                 0x0000003FUL                               /**< Mode APORT1YCH31 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH0                  0x00000040UL                               /**< Mode APORT2YCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH1                  0x00000041UL                               /**< Mode APORT2XCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH2                  0x00000042UL                               /**< Mode APORT2YCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH3                  0x00000043UL                               /**< Mode APORT2XCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH4                  0x00000044UL                               /**< Mode APORT2YCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH5                  0x00000045UL                               /**< Mode APORT2XCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH6                  0x00000046UL                               /**< Mode APORT2YCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH7                  0x00000047UL                               /**< Mode APORT2XCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH8                  0x00000048UL                               /**< Mode APORT2YCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH9                  0x00000049UL                               /**< Mode APORT2XCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH10                 0x0000004AUL                               /**< Mode APORT2YCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH11                 0x0000004BUL                               /**< Mode APORT2XCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH12                 0x0000004CUL                               /**< Mode APORT2YCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH13                 0x0000004DUL                               /**< Mode APORT2XCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH14                 0x0000004EUL                               /**< Mode APORT2YCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH15                 0x0000004FUL                               /**< Mode APORT2XCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH16                 0x00000050UL                               /**< Mode APORT2YCH16 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH17                 0x00000051UL                               /**< Mode APORT2XCH17 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH18                 0x00000052UL                               /**< Mode APORT2YCH18 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH19                 0x00000053UL                               /**< Mode APORT2XCH19 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH20                 0x00000054UL                               /**< Mode APORT2YCH20 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH21                 0x00000055UL                               /**< Mode APORT2XCH21 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH22                 0x00000056UL                               /**< Mode APORT2YCH22 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH23                 0x00000057UL                               /**< Mode APORT2XCH23 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH24                 0x00000058UL                               /**< Mode APORT2YCH24 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH25                 0x00000059UL                               /**< Mode APORT2XCH25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH26                 0x0000005AUL                               /**< Mode APORT2YCH26 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH27                 0x0000005BUL                               /**< Mode APORT2XCH27 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH28                 0x0000005CUL                               /**< Mode APORT2YCH28 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH29                 0x0000005DUL                               /**< Mode APORT2XCH29 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2YCH30                 0x0000005EUL                               /**< Mode APORT2YCH30 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT2XCH31                 0x0000005FUL                               /**< Mode APORT2XCH31 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH0                  0x00000060UL                               /**< Mode APORT3XCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH1                  0x00000061UL                               /**< Mode APORT3YCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH2                  0x00000062UL                               /**< Mode APORT3XCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH3                  0x00000063UL                               /**< Mode APORT3YCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH4                  0x00000064UL                               /**< Mode APORT3XCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH5                  0x00000065UL                               /**< Mode APORT3YCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH6                  0x00000066UL                               /**< Mode APORT3XCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH7                  0x00000067UL                               /**< Mode APORT3YCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH8                  0x00000068UL                               /**< Mode APORT3XCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH9                  0x00000069UL                               /**< Mode APORT3YCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH10                 0x0000006AUL                               /**< Mode APORT3XCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH11                 0x0000006BUL                               /**< Mode APORT3YCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH12                 0x0000006CUL                               /**< Mode APORT3XCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH13                 0x0000006DUL                               /**< Mode APORT3YCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH14                 0x0000006EUL                               /**< Mode APORT3XCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH15                 0x0000006FUL                               /**< Mode APORT3YCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH16                 0x00000070UL                               /**< Mode APORT3XCH16 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH17                 0x00000071UL                               /**< Mode APORT3YCH17 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH18                 0x00000072UL                               /**< Mode APORT3XCH18 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH19                 0x00000073UL                               /**< Mode APORT3YCH19 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH20                 0x00000074UL                               /**< Mode APORT3XCH20 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH21                 0x00000075UL                               /**< Mode APORT3YCH21 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH22                 0x00000076UL                               /**< Mode APORT3XCH22 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH23                 0x00000077UL                               /**< Mode APORT3YCH23 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH24                 0x00000078UL                               /**< Mode APORT3XCH24 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH25                 0x00000079UL                               /**< Mode APORT3YCH25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH26                 0x0000007AUL                               /**< Mode APORT3XCH26 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH27                 0x0000007BUL                               /**< Mode APORT3YCH27 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH28                 0x0000007CUL                               /**< Mode APORT3XCH28 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH29                 0x0000007DUL                               /**< Mode APORT3YCH29 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3XCH30                 0x0000007EUL                               /**< Mode APORT3XCH30 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT3YCH31                 0x0000007FUL                               /**< Mode APORT3YCH31 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH0                  0x00000080UL                               /**< Mode APORT4YCH0 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH1                  0x00000081UL                               /**< Mode APORT4XCH1 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH2                  0x00000082UL                               /**< Mode APORT4YCH2 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH3                  0x00000083UL                               /**< Mode APORT4XCH3 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH4                  0x00000084UL                               /**< Mode APORT4YCH4 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH5                  0x00000085UL                               /**< Mode APORT4XCH5 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH6                  0x00000086UL                               /**< Mode APORT4YCH6 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH7                  0x00000087UL                               /**< Mode APORT4XCH7 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH8                  0x00000088UL                               /**< Mode APORT4YCH8 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH9                  0x00000089UL                               /**< Mode APORT4XCH9 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH10                 0x0000008AUL                               /**< Mode APORT4YCH10 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH11                 0x0000008BUL                               /**< Mode APORT4XCH11 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH12                 0x0000008CUL                               /**< Mode APORT4YCH12 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH13                 0x0000008DUL                               /**< Mode APORT4XCH13 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH14                 0x0000008EUL                               /**< Mode APORT4YCH14 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH15                 0x0000008FUL                               /**< Mode APORT4XCH15 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH16                 0x00000090UL                               /**< Mode APORT4YCH16 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH17                 0x00000091UL                               /**< Mode APORT4XCH17 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH18                 0x00000092UL                               /**< Mode APORT4YCH18 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH19                 0x00000093UL                               /**< Mode APORT4XCH19 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH20                 0x00000094UL                               /**< Mode APORT4YCH20 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH21                 0x00000095UL                               /**< Mode APORT4XCH21 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH22                 0x00000096UL                               /**< Mode APORT4YCH22 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH23                 0x00000097UL                               /**< Mode APORT4XCH23 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH24                 0x00000098UL                               /**< Mode APORT4YCH24 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH25                 0x00000099UL                               /**< Mode APORT4XCH25 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH26                 0x0000009AUL                               /**< Mode APORT4YCH26 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH27                 0x0000009BUL                               /**< Mode APORT4XCH27 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH28                 0x0000009CUL                               /**< Mode APORT4YCH28 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH29                 0x0000009DUL                               /**< Mode APORT4XCH29 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4YCH30                 0x0000009EUL                               /**< Mode APORT4YCH30 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_APORT4XCH31                 0x0000009FUL                               /**< Mode APORT4XCH31 for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_TESTN                       0x000000F5UL                               /**< Mode TESTN for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_DEFAULT                     0x000000FFUL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_NEGSEL_VSS                         0x000000FFUL                               /**< Mode VSS for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH0                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH0 << 16)  /**< Shifted mode APORT0XCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH1                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH1 << 16)  /**< Shifted mode APORT0XCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH2                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH2 << 16)  /**< Shifted mode APORT0XCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH3                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH3 << 16)  /**< Shifted mode APORT0XCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH4                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH4 << 16)  /**< Shifted mode APORT0XCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH5                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH5 << 16)  /**< Shifted mode APORT0XCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH6                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH6 << 16)  /**< Shifted mode APORT0XCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH7                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH7 << 16)  /**< Shifted mode APORT0XCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH8                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH8 << 16)  /**< Shifted mode APORT0XCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH9                   (_ADC_SINGLECTRL_NEGSEL_APORT0XCH9 << 16)  /**< Shifted mode APORT0XCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH10                  (_ADC_SINGLECTRL_NEGSEL_APORT0XCH10 << 16) /**< Shifted mode APORT0XCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH11                  (_ADC_SINGLECTRL_NEGSEL_APORT0XCH11 << 16) /**< Shifted mode APORT0XCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH12                  (_ADC_SINGLECTRL_NEGSEL_APORT0XCH12 << 16) /**< Shifted mode APORT0XCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH13                  (_ADC_SINGLECTRL_NEGSEL_APORT0XCH13 << 16) /**< Shifted mode APORT0XCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH14                  (_ADC_SINGLECTRL_NEGSEL_APORT0XCH14 << 16) /**< Shifted mode APORT0XCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0XCH15                  (_ADC_SINGLECTRL_NEGSEL_APORT0XCH15 << 16) /**< Shifted mode APORT0XCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH0                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH0 << 16)  /**< Shifted mode APORT0YCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH1                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH1 << 16)  /**< Shifted mode APORT0YCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH2                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH2 << 16)  /**< Shifted mode APORT0YCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH3                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH3 << 16)  /**< Shifted mode APORT0YCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH4                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH4 << 16)  /**< Shifted mode APORT0YCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH5                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH5 << 16)  /**< Shifted mode APORT0YCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH6                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH6 << 16)  /**< Shifted mode APORT0YCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH7                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH7 << 16)  /**< Shifted mode APORT0YCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH8                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH8 << 16)  /**< Shifted mode APORT0YCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH9                   (_ADC_SINGLECTRL_NEGSEL_APORT0YCH9 << 16)  /**< Shifted mode APORT0YCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH10                  (_ADC_SINGLECTRL_NEGSEL_APORT0YCH10 << 16) /**< Shifted mode APORT0YCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH11                  (_ADC_SINGLECTRL_NEGSEL_APORT0YCH11 << 16) /**< Shifted mode APORT0YCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH12                  (_ADC_SINGLECTRL_NEGSEL_APORT0YCH12 << 16) /**< Shifted mode APORT0YCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH13                  (_ADC_SINGLECTRL_NEGSEL_APORT0YCH13 << 16) /**< Shifted mode APORT0YCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH14                  (_ADC_SINGLECTRL_NEGSEL_APORT0YCH14 << 16) /**< Shifted mode APORT0YCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT0YCH15                  (_ADC_SINGLECTRL_NEGSEL_APORT0YCH15 << 16) /**< Shifted mode APORT0YCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH0                   (_ADC_SINGLECTRL_NEGSEL_APORT1XCH0 << 16)  /**< Shifted mode APORT1XCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH1                   (_ADC_SINGLECTRL_NEGSEL_APORT1YCH1 << 16)  /**< Shifted mode APORT1YCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH2                   (_ADC_SINGLECTRL_NEGSEL_APORT1XCH2 << 16)  /**< Shifted mode APORT1XCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH3                   (_ADC_SINGLECTRL_NEGSEL_APORT1YCH3 << 16)  /**< Shifted mode APORT1YCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH4                   (_ADC_SINGLECTRL_NEGSEL_APORT1XCH4 << 16)  /**< Shifted mode APORT1XCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH5                   (_ADC_SINGLECTRL_NEGSEL_APORT1YCH5 << 16)  /**< Shifted mode APORT1YCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH6                   (_ADC_SINGLECTRL_NEGSEL_APORT1XCH6 << 16)  /**< Shifted mode APORT1XCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH7                   (_ADC_SINGLECTRL_NEGSEL_APORT1YCH7 << 16)  /**< Shifted mode APORT1YCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH8                   (_ADC_SINGLECTRL_NEGSEL_APORT1XCH8 << 16)  /**< Shifted mode APORT1XCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH9                   (_ADC_SINGLECTRL_NEGSEL_APORT1YCH9 << 16)  /**< Shifted mode APORT1YCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH10                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH10 << 16) /**< Shifted mode APORT1XCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH11                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH11 << 16) /**< Shifted mode APORT1YCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH12                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH12 << 16) /**< Shifted mode APORT1XCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH13                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH13 << 16) /**< Shifted mode APORT1YCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH14                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH14 << 16) /**< Shifted mode APORT1XCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH15                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH15 << 16) /**< Shifted mode APORT1YCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH16                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH16 << 16) /**< Shifted mode APORT1XCH16 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH17                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH17 << 16) /**< Shifted mode APORT1YCH17 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH18                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH18 << 16) /**< Shifted mode APORT1XCH18 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH19                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH19 << 16) /**< Shifted mode APORT1YCH19 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH20                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH20 << 16) /**< Shifted mode APORT1XCH20 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH21                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH21 << 16) /**< Shifted mode APORT1YCH21 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH22                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH22 << 16) /**< Shifted mode APORT1XCH22 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH23                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH23 << 16) /**< Shifted mode APORT1YCH23 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH24                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH24 << 16) /**< Shifted mode APORT1XCH24 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH25                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH25 << 16) /**< Shifted mode APORT1YCH25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH26                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH26 << 16) /**< Shifted mode APORT1XCH26 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH27                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH27 << 16) /**< Shifted mode APORT1YCH27 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH28                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH28 << 16) /**< Shifted mode APORT1XCH28 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH29                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH29 << 16) /**< Shifted mode APORT1YCH29 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1XCH30                  (_ADC_SINGLECTRL_NEGSEL_APORT1XCH30 << 16) /**< Shifted mode APORT1XCH30 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT1YCH31                  (_ADC_SINGLECTRL_NEGSEL_APORT1YCH31 << 16) /**< Shifted mode APORT1YCH31 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH0                   (_ADC_SINGLECTRL_NEGSEL_APORT2YCH0 << 16)  /**< Shifted mode APORT2YCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH1                   (_ADC_SINGLECTRL_NEGSEL_APORT2XCH1 << 16)  /**< Shifted mode APORT2XCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH2                   (_ADC_SINGLECTRL_NEGSEL_APORT2YCH2 << 16)  /**< Shifted mode APORT2YCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH3                   (_ADC_SINGLECTRL_NEGSEL_APORT2XCH3 << 16)  /**< Shifted mode APORT2XCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH4                   (_ADC_SINGLECTRL_NEGSEL_APORT2YCH4 << 16)  /**< Shifted mode APORT2YCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH5                   (_ADC_SINGLECTRL_NEGSEL_APORT2XCH5 << 16)  /**< Shifted mode APORT2XCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH6                   (_ADC_SINGLECTRL_NEGSEL_APORT2YCH6 << 16)  /**< Shifted mode APORT2YCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH7                   (_ADC_SINGLECTRL_NEGSEL_APORT2XCH7 << 16)  /**< Shifted mode APORT2XCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH8                   (_ADC_SINGLECTRL_NEGSEL_APORT2YCH8 << 16)  /**< Shifted mode APORT2YCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH9                   (_ADC_SINGLECTRL_NEGSEL_APORT2XCH9 << 16)  /**< Shifted mode APORT2XCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH10                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH10 << 16) /**< Shifted mode APORT2YCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH11                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH11 << 16) /**< Shifted mode APORT2XCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH12                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH12 << 16) /**< Shifted mode APORT2YCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH13                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH13 << 16) /**< Shifted mode APORT2XCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH14                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH14 << 16) /**< Shifted mode APORT2YCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH15                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH15 << 16) /**< Shifted mode APORT2XCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH16                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH16 << 16) /**< Shifted mode APORT2YCH16 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH17                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH17 << 16) /**< Shifted mode APORT2XCH17 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH18                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH18 << 16) /**< Shifted mode APORT2YCH18 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH19                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH19 << 16) /**< Shifted mode APORT2XCH19 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH20                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH20 << 16) /**< Shifted mode APORT2YCH20 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH21                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH21 << 16) /**< Shifted mode APORT2XCH21 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH22                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH22 << 16) /**< Shifted mode APORT2YCH22 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH23                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH23 << 16) /**< Shifted mode APORT2XCH23 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH24                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH24 << 16) /**< Shifted mode APORT2YCH24 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH25                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH25 << 16) /**< Shifted mode APORT2XCH25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH26                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH26 << 16) /**< Shifted mode APORT2YCH26 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH27                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH27 << 16) /**< Shifted mode APORT2XCH27 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH28                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH28 << 16) /**< Shifted mode APORT2YCH28 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH29                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH29 << 16) /**< Shifted mode APORT2XCH29 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2YCH30                  (_ADC_SINGLECTRL_NEGSEL_APORT2YCH30 << 16) /**< Shifted mode APORT2YCH30 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT2XCH31                  (_ADC_SINGLECTRL_NEGSEL_APORT2XCH31 << 16) /**< Shifted mode APORT2XCH31 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH0                   (_ADC_SINGLECTRL_NEGSEL_APORT3XCH0 << 16)  /**< Shifted mode APORT3XCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH1                   (_ADC_SINGLECTRL_NEGSEL_APORT3YCH1 << 16)  /**< Shifted mode APORT3YCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH2                   (_ADC_SINGLECTRL_NEGSEL_APORT3XCH2 << 16)  /**< Shifted mode APORT3XCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH3                   (_ADC_SINGLECTRL_NEGSEL_APORT3YCH3 << 16)  /**< Shifted mode APORT3YCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH4                   (_ADC_SINGLECTRL_NEGSEL_APORT3XCH4 << 16)  /**< Shifted mode APORT3XCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH5                   (_ADC_SINGLECTRL_NEGSEL_APORT3YCH5 << 16)  /**< Shifted mode APORT3YCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH6                   (_ADC_SINGLECTRL_NEGSEL_APORT3XCH6 << 16)  /**< Shifted mode APORT3XCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH7                   (_ADC_SINGLECTRL_NEGSEL_APORT3YCH7 << 16)  /**< Shifted mode APORT3YCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH8                   (_ADC_SINGLECTRL_NEGSEL_APORT3XCH8 << 16)  /**< Shifted mode APORT3XCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH9                   (_ADC_SINGLECTRL_NEGSEL_APORT3YCH9 << 16)  /**< Shifted mode APORT3YCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH10                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH10 << 16) /**< Shifted mode APORT3XCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH11                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH11 << 16) /**< Shifted mode APORT3YCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH12                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH12 << 16) /**< Shifted mode APORT3XCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH13                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH13 << 16) /**< Shifted mode APORT3YCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH14                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH14 << 16) /**< Shifted mode APORT3XCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH15                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH15 << 16) /**< Shifted mode APORT3YCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH16                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH16 << 16) /**< Shifted mode APORT3XCH16 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH17                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH17 << 16) /**< Shifted mode APORT3YCH17 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH18                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH18 << 16) /**< Shifted mode APORT3XCH18 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH19                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH19 << 16) /**< Shifted mode APORT3YCH19 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH20                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH20 << 16) /**< Shifted mode APORT3XCH20 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH21                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH21 << 16) /**< Shifted mode APORT3YCH21 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH22                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH22 << 16) /**< Shifted mode APORT3XCH22 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH23                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH23 << 16) /**< Shifted mode APORT3YCH23 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH24                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH24 << 16) /**< Shifted mode APORT3XCH24 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH25                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH25 << 16) /**< Shifted mode APORT3YCH25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH26                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH26 << 16) /**< Shifted mode APORT3XCH26 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH27                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH27 << 16) /**< Shifted mode APORT3YCH27 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH28                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH28 << 16) /**< Shifted mode APORT3XCH28 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH29                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH29 << 16) /**< Shifted mode APORT3YCH29 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3XCH30                  (_ADC_SINGLECTRL_NEGSEL_APORT3XCH30 << 16) /**< Shifted mode APORT3XCH30 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT3YCH31                  (_ADC_SINGLECTRL_NEGSEL_APORT3YCH31 << 16) /**< Shifted mode APORT3YCH31 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH0                   (_ADC_SINGLECTRL_NEGSEL_APORT4YCH0 << 16)  /**< Shifted mode APORT4YCH0 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH1                   (_ADC_SINGLECTRL_NEGSEL_APORT4XCH1 << 16)  /**< Shifted mode APORT4XCH1 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH2                   (_ADC_SINGLECTRL_NEGSEL_APORT4YCH2 << 16)  /**< Shifted mode APORT4YCH2 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH3                   (_ADC_SINGLECTRL_NEGSEL_APORT4XCH3 << 16)  /**< Shifted mode APORT4XCH3 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH4                   (_ADC_SINGLECTRL_NEGSEL_APORT4YCH4 << 16)  /**< Shifted mode APORT4YCH4 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH5                   (_ADC_SINGLECTRL_NEGSEL_APORT4XCH5 << 16)  /**< Shifted mode APORT4XCH5 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH6                   (_ADC_SINGLECTRL_NEGSEL_APORT4YCH6 << 16)  /**< Shifted mode APORT4YCH6 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH7                   (_ADC_SINGLECTRL_NEGSEL_APORT4XCH7 << 16)  /**< Shifted mode APORT4XCH7 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH8                   (_ADC_SINGLECTRL_NEGSEL_APORT4YCH8 << 16)  /**< Shifted mode APORT4YCH8 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH9                   (_ADC_SINGLECTRL_NEGSEL_APORT4XCH9 << 16)  /**< Shifted mode APORT4XCH9 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH10                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH10 << 16) /**< Shifted mode APORT4YCH10 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH11                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH11 << 16) /**< Shifted mode APORT4XCH11 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH12                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH12 << 16) /**< Shifted mode APORT4YCH12 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH13                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH13 << 16) /**< Shifted mode APORT4XCH13 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH14                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH14 << 16) /**< Shifted mode APORT4YCH14 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH15                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH15 << 16) /**< Shifted mode APORT4XCH15 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH16                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH16 << 16) /**< Shifted mode APORT4YCH16 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH17                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH17 << 16) /**< Shifted mode APORT4XCH17 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH18                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH18 << 16) /**< Shifted mode APORT4YCH18 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH19                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH19 << 16) /**< Shifted mode APORT4XCH19 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH20                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH20 << 16) /**< Shifted mode APORT4YCH20 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH21                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH21 << 16) /**< Shifted mode APORT4XCH21 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH22                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH22 << 16) /**< Shifted mode APORT4YCH22 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH23                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH23 << 16) /**< Shifted mode APORT4XCH23 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH24                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH24 << 16) /**< Shifted mode APORT4YCH24 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH25                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH25 << 16) /**< Shifted mode APORT4XCH25 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH26                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH26 << 16) /**< Shifted mode APORT4YCH26 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH27                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH27 << 16) /**< Shifted mode APORT4XCH27 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH28                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH28 << 16) /**< Shifted mode APORT4YCH28 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH29                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH29 << 16) /**< Shifted mode APORT4XCH29 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4YCH30                  (_ADC_SINGLECTRL_NEGSEL_APORT4YCH30 << 16) /**< Shifted mode APORT4YCH30 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_APORT4XCH31                  (_ADC_SINGLECTRL_NEGSEL_APORT4XCH31 << 16) /**< Shifted mode APORT4XCH31 for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_TESTN                        (_ADC_SINGLECTRL_NEGSEL_TESTN << 16)       /**< Shifted mode TESTN for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_DEFAULT                      (_ADC_SINGLECTRL_NEGSEL_DEFAULT << 16)     /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_NEGSEL_VSS                          (_ADC_SINGLECTRL_NEGSEL_VSS << 16)         /**< Shifted mode VSS for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_SHIFT                           24                                         /**< Shift value for ADC_AT */
#define _ADC_SINGLECTRL_AT_MASK                            0xF000000UL                                /**< Bit mask for ADC_AT */
#define _ADC_SINGLECTRL_AT_DEFAULT                         0x00000000UL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_1CYCLE                          0x00000000UL                               /**< Mode 1CYCLE for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_2CYCLES                         0x00000001UL                               /**< Mode 2CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_3CYCLES                         0x00000002UL                               /**< Mode 3CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_4CYCLES                         0x00000003UL                               /**< Mode 4CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_8CYCLES                         0x00000004UL                               /**< Mode 8CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_16CYCLES                        0x00000005UL                               /**< Mode 16CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_32CYCLES                        0x00000006UL                               /**< Mode 32CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_64CYCLES                        0x00000007UL                               /**< Mode 64CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_128CYCLES                       0x00000008UL                               /**< Mode 128CYCLES for ADC_SINGLECTRL */
#define _ADC_SINGLECTRL_AT_256CYCLES                       0x00000009UL                               /**< Mode 256CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_DEFAULT                          (_ADC_SINGLECTRL_AT_DEFAULT << 24)         /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_1CYCLE                           (_ADC_SINGLECTRL_AT_1CYCLE << 24)          /**< Shifted mode 1CYCLE for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_2CYCLES                          (_ADC_SINGLECTRL_AT_2CYCLES << 24)         /**< Shifted mode 2CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_3CYCLES                          (_ADC_SINGLECTRL_AT_3CYCLES << 24)         /**< Shifted mode 3CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_4CYCLES                          (_ADC_SINGLECTRL_AT_4CYCLES << 24)         /**< Shifted mode 4CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_8CYCLES                          (_ADC_SINGLECTRL_AT_8CYCLES << 24)         /**< Shifted mode 8CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_16CYCLES                         (_ADC_SINGLECTRL_AT_16CYCLES << 24)        /**< Shifted mode 16CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_32CYCLES                         (_ADC_SINGLECTRL_AT_32CYCLES << 24)        /**< Shifted mode 32CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_64CYCLES                         (_ADC_SINGLECTRL_AT_64CYCLES << 24)        /**< Shifted mode 64CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_128CYCLES                        (_ADC_SINGLECTRL_AT_128CYCLES << 24)       /**< Shifted mode 128CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_AT_256CYCLES                        (_ADC_SINGLECTRL_AT_256CYCLES << 24)       /**< Shifted mode 256CYCLES for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSEN                               (0x1UL << 29)                              /**< Single Channel PRS Trigger Enable */
#define _ADC_SINGLECTRL_PRSEN_SHIFT                        29                                         /**< Shift value for ADC_PRSEN */
#define _ADC_SINGLECTRL_PRSEN_MASK                         0x20000000UL                               /**< Bit mask for ADC_PRSEN */
#define _ADC_SINGLECTRL_PRSEN_DEFAULT                      0x00000000UL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_PRSEN_DEFAULT                       (_ADC_SINGLECTRL_PRSEN_DEFAULT << 29)      /**< Shifted mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_CMPEN                               (0x1UL << 31)                              /**< Compare Logic Enable for Single Channel */
#define _ADC_SINGLECTRL_CMPEN_SHIFT                        31                                         /**< Shift value for ADC_CMPEN */
#define _ADC_SINGLECTRL_CMPEN_MASK                         0x80000000UL                               /**< Bit mask for ADC_CMPEN */
#define _ADC_SINGLECTRL_CMPEN_DEFAULT                      0x00000000UL                               /**< Mode DEFAULT for ADC_SINGLECTRL */
#define ADC_SINGLECTRL_CMPEN_DEFAULT                       (_ADC_SINGLECTRL_CMPEN_DEFAULT << 31)      /**< Shifted mode DEFAULT for ADC_SINGLECTRL */

/* Bit fields for ADC SINGLECTRLX */
#define _ADC_SINGLECTRLX_RESETVALUE                        0x00000000UL                                      /**< Default value for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_MASK                              0x0F1F7FFFUL                                      /**< Mask for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_SHIFT                     0                                                 /**< Shift value for ADC_VREFSEL */
#define _ADC_SINGLECTRLX_VREFSEL_MASK                      0x7UL                                             /**< Bit mask for ADC_VREFSEL */
#define _ADC_SINGLECTRLX_VREFSEL_DEFAULT                   0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_VBGR                      0x00000000UL                                      /**< Mode VBGR for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_VDDXWATT                  0x00000001UL                                      /**< Mode VDDXWATT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_VREFPWATT                 0x00000002UL                                      /**< Mode VREFPWATT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_VREFP                     0x00000003UL                                      /**< Mode VREFP for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_VENTROPY                  0x00000004UL                                      /**< Mode VENTROPY for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_VREFPNWATT                0x00000005UL                                      /**< Mode VREFPNWATT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_VREFPN                    0x00000006UL                                      /**< Mode VREFPN for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFSEL_VBGRLOW                   0x00000007UL                                      /**< Mode VBGRLOW for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_DEFAULT                    (_ADC_SINGLECTRLX_VREFSEL_DEFAULT << 0)           /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_VBGR                       (_ADC_SINGLECTRLX_VREFSEL_VBGR << 0)              /**< Shifted mode VBGR for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_VDDXWATT                   (_ADC_SINGLECTRLX_VREFSEL_VDDXWATT << 0)          /**< Shifted mode VDDXWATT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_VREFPWATT                  (_ADC_SINGLECTRLX_VREFSEL_VREFPWATT << 0)         /**< Shifted mode VREFPWATT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_VREFP                      (_ADC_SINGLECTRLX_VREFSEL_VREFP << 0)             /**< Shifted mode VREFP for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_VENTROPY                   (_ADC_SINGLECTRLX_VREFSEL_VENTROPY << 0)          /**< Shifted mode VENTROPY for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_VREFPNWATT                 (_ADC_SINGLECTRLX_VREFSEL_VREFPNWATT << 0)        /**< Shifted mode VREFPNWATT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_VREFPN                     (_ADC_SINGLECTRLX_VREFSEL_VREFPN << 0)            /**< Shifted mode VREFPN for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFSEL_VBGRLOW                    (_ADC_SINGLECTRLX_VREFSEL_VBGRLOW << 0)           /**< Shifted mode VBGRLOW for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFATTFIX                         (0x1UL << 3)                                      /**< Enable fixed scaling on VREF */
#define _ADC_SINGLECTRLX_VREFATTFIX_SHIFT                  3                                                 /**< Shift value for ADC_VREFATTFIX */
#define _ADC_SINGLECTRLX_VREFATTFIX_MASK                   0x8UL                                             /**< Bit mask for ADC_VREFATTFIX */
#define _ADC_SINGLECTRLX_VREFATTFIX_DEFAULT                0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFATTFIX_DEFAULT                 (_ADC_SINGLECTRLX_VREFATTFIX_DEFAULT << 3)        /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VREFATT_SHIFT                     4                                                 /**< Shift value for ADC_VREFATT */
#define _ADC_SINGLECTRLX_VREFATT_MASK                      0xF0UL                                            /**< Bit mask for ADC_VREFATT */
#define _ADC_SINGLECTRLX_VREFATT_DEFAULT                   0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VREFATT_DEFAULT                    (_ADC_SINGLECTRLX_VREFATT_DEFAULT << 4)           /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_VINATT_SHIFT                      8                                                 /**< Shift value for ADC_VINATT */
#define _ADC_SINGLECTRLX_VINATT_MASK                       0xF00UL                                           /**< Bit mask for ADC_VINATT */
#define _ADC_SINGLECTRLX_VINATT_DEFAULT                    0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_VINATT_DEFAULT                     (_ADC_SINGLECTRLX_VINATT_DEFAULT << 8)            /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_DVL_SHIFT                         12                                                /**< Shift value for ADC_DVL */
#define _ADC_SINGLECTRLX_DVL_MASK                          0x3000UL                                          /**< Bit mask for ADC_DVL */
#define _ADC_SINGLECTRLX_DVL_DEFAULT                       0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_DVL_DEFAULT                        (_ADC_SINGLECTRLX_DVL_DEFAULT << 12)              /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_FIFOOFACT                          (0x1UL << 14)                                     /**< Single Channel FIFO Overflow Action */
#define _ADC_SINGLECTRLX_FIFOOFACT_SHIFT                   14                                                /**< Shift value for ADC_FIFOOFACT */
#define _ADC_SINGLECTRLX_FIFOOFACT_MASK                    0x4000UL                                          /**< Bit mask for ADC_FIFOOFACT */
#define _ADC_SINGLECTRLX_FIFOOFACT_DEFAULT                 0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_FIFOOFACT_DISCARD                 0x00000000UL                                      /**< Mode DISCARD for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_FIFOOFACT_OVERWRITE               0x00000001UL                                      /**< Mode OVERWRITE for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_FIFOOFACT_DEFAULT                  (_ADC_SINGLECTRLX_FIFOOFACT_DEFAULT << 14)        /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_FIFOOFACT_DISCARD                  (_ADC_SINGLECTRLX_FIFOOFACT_DISCARD << 14)        /**< Shifted mode DISCARD for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_FIFOOFACT_OVERWRITE                (_ADC_SINGLECTRLX_FIFOOFACT_OVERWRITE << 14)      /**< Shifted mode OVERWRITE for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSMODE                            (0x1UL << 16)                                     /**< Single Channel PRS Trigger Mode */
#define _ADC_SINGLECTRLX_PRSMODE_SHIFT                     16                                                /**< Shift value for ADC_PRSMODE */
#define _ADC_SINGLECTRLX_PRSMODE_MASK                      0x10000UL                                         /**< Bit mask for ADC_PRSMODE */
#define _ADC_SINGLECTRLX_PRSMODE_DEFAULT                   0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSMODE_PULSED                    0x00000000UL                                      /**< Mode PULSED for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSMODE_TIMED                     0x00000001UL                                      /**< Mode TIMED for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSMODE_DEFAULT                    (_ADC_SINGLECTRLX_PRSMODE_DEFAULT << 16)          /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSMODE_PULSED                     (_ADC_SINGLECTRLX_PRSMODE_PULSED << 16)           /**< Shifted mode PULSED for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSMODE_TIMED                      (_ADC_SINGLECTRLX_PRSMODE_TIMED << 16)            /**< Shifted mode TIMED for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_SHIFT                      17                                                /**< Shift value for ADC_PRSSEL */
#define _ADC_SINGLECTRLX_PRSSEL_MASK                       0x1E0000UL                                        /**< Bit mask for ADC_PRSSEL */
#define _ADC_SINGLECTRLX_PRSSEL_DEFAULT                    0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH0                     0x00000000UL                                      /**< Mode PRSCH0 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH1                     0x00000001UL                                      /**< Mode PRSCH1 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH2                     0x00000002UL                                      /**< Mode PRSCH2 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH3                     0x00000003UL                                      /**< Mode PRSCH3 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH4                     0x00000004UL                                      /**< Mode PRSCH4 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH5                     0x00000005UL                                      /**< Mode PRSCH5 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH6                     0x00000006UL                                      /**< Mode PRSCH6 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH7                     0x00000007UL                                      /**< Mode PRSCH7 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH8                     0x00000008UL                                      /**< Mode PRSCH8 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH9                     0x00000009UL                                      /**< Mode PRSCH9 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH10                    0x0000000AUL                                      /**< Mode PRSCH10 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_PRSSEL_PRSCH11                    0x0000000BUL                                      /**< Mode PRSCH11 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_DEFAULT                     (_ADC_SINGLECTRLX_PRSSEL_DEFAULT << 17)           /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH0                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH0 << 17)            /**< Shifted mode PRSCH0 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH1                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH1 << 17)            /**< Shifted mode PRSCH1 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH2                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH2 << 17)            /**< Shifted mode PRSCH2 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH3                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH3 << 17)            /**< Shifted mode PRSCH3 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH4                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH4 << 17)            /**< Shifted mode PRSCH4 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH5                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH5 << 17)            /**< Shifted mode PRSCH5 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH6                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH6 << 17)            /**< Shifted mode PRSCH6 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH7                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH7 << 17)            /**< Shifted mode PRSCH7 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH8                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH8 << 17)            /**< Shifted mode PRSCH8 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH9                      (_ADC_SINGLECTRLX_PRSSEL_PRSCH9 << 17)            /**< Shifted mode PRSCH9 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH10                     (_ADC_SINGLECTRLX_PRSSEL_PRSCH10 << 17)           /**< Shifted mode PRSCH10 for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_PRSSEL_PRSCH11                     (_ADC_SINGLECTRLX_PRSSEL_PRSCH11 << 17)           /**< Shifted mode PRSCH11 for ADC_SINGLECTRLX */
#define _ADC_SINGLECTRLX_CONVSTARTDELAY_SHIFT              24                                                /**< Shift value for ADC_CONVSTARTDELAY */
#define _ADC_SINGLECTRLX_CONVSTARTDELAY_MASK               0x7000000UL                                       /**< Bit mask for ADC_CONVSTARTDELAY */
#define _ADC_SINGLECTRLX_CONVSTARTDELAY_DEFAULT            0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_CONVSTARTDELAY_DEFAULT             (_ADC_SINGLECTRLX_CONVSTARTDELAY_DEFAULT << 24)   /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_CONVSTARTDELAYEN                   (0x1UL << 27)                                     /**< Enable delaying next conversion start */
#define _ADC_SINGLECTRLX_CONVSTARTDELAYEN_SHIFT            27                                                /**< Shift value for ADC_CONVSTARTDELAYEN */
#define _ADC_SINGLECTRLX_CONVSTARTDELAYEN_MASK             0x8000000UL                                       /**< Bit mask for ADC_CONVSTARTDELAYEN */
#define _ADC_SINGLECTRLX_CONVSTARTDELAYEN_DEFAULT          0x00000000UL                                      /**< Mode DEFAULT for ADC_SINGLECTRLX */
#define ADC_SINGLECTRLX_CONVSTARTDELAYEN_DEFAULT           (_ADC_SINGLECTRLX_CONVSTARTDELAYEN_DEFAULT << 27) /**< Shifted mode DEFAULT for ADC_SINGLECTRLX */

/* Bit fields for ADC SCANCTRL */
#define _ADC_SCANCTRL_RESETVALUE                           0x00000000UL                        /**< Default value for ADC_SCANCTRL */
#define _ADC_SCANCTRL_MASK                                 0xAF0000FFUL                        /**< Mask for ADC_SCANCTRL */
#define ADC_SCANCTRL_REP                                   (0x1UL << 0)                        /**< Scan Sequence Repetitive Mode */
#define _ADC_SCANCTRL_REP_SHIFT                            0                                   /**< Shift value for ADC_REP */
#define _ADC_SCANCTRL_REP_MASK                             0x1UL                               /**< Bit mask for ADC_REP */
#define _ADC_SCANCTRL_REP_DEFAULT                          0x00000000UL                        /**< Mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_REP_DEFAULT                           (_ADC_SCANCTRL_REP_DEFAULT << 0)    /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_DIFF                                  (0x1UL << 1)                        /**< Scan Sequence Differential Mode */
#define _ADC_SCANCTRL_DIFF_SHIFT                           1                                   /**< Shift value for ADC_DIFF */
#define _ADC_SCANCTRL_DIFF_MASK                            0x2UL                               /**< Bit mask for ADC_DIFF */
#define _ADC_SCANCTRL_DIFF_DEFAULT                         0x00000000UL                        /**< Mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_DIFF_DEFAULT                          (_ADC_SCANCTRL_DIFF_DEFAULT << 1)   /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_ADJ                                   (0x1UL << 2)                        /**< Scan Sequence Result Adjustment */
#define _ADC_SCANCTRL_ADJ_SHIFT                            2                                   /**< Shift value for ADC_ADJ */
#define _ADC_SCANCTRL_ADJ_MASK                             0x4UL                               /**< Bit mask for ADC_ADJ */
#define _ADC_SCANCTRL_ADJ_DEFAULT                          0x00000000UL                        /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_ADJ_RIGHT                            0x00000000UL                        /**< Mode RIGHT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_ADJ_LEFT                             0x00000001UL                        /**< Mode LEFT for ADC_SCANCTRL */
#define ADC_SCANCTRL_ADJ_DEFAULT                           (_ADC_SCANCTRL_ADJ_DEFAULT << 2)    /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_ADJ_RIGHT                             (_ADC_SCANCTRL_ADJ_RIGHT << 2)      /**< Shifted mode RIGHT for ADC_SCANCTRL */
#define ADC_SCANCTRL_ADJ_LEFT                              (_ADC_SCANCTRL_ADJ_LEFT << 2)       /**< Shifted mode LEFT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_SHIFT                            3                                   /**< Shift value for ADC_RES */
#define _ADC_SCANCTRL_RES_MASK                             0x18UL                              /**< Bit mask for ADC_RES */
#define _ADC_SCANCTRL_RES_DEFAULT                          0x00000000UL                        /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_12BIT                            0x00000000UL                        /**< Mode 12BIT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_8BIT                             0x00000001UL                        /**< Mode 8BIT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_6BIT                             0x00000002UL                        /**< Mode 6BIT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_RES_OVS                              0x00000003UL                        /**< Mode OVS for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_DEFAULT                           (_ADC_SCANCTRL_RES_DEFAULT << 3)    /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_12BIT                             (_ADC_SCANCTRL_RES_12BIT << 3)      /**< Shifted mode 12BIT for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_8BIT                              (_ADC_SCANCTRL_RES_8BIT << 3)       /**< Shifted mode 8BIT for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_6BIT                              (_ADC_SCANCTRL_RES_6BIT << 3)       /**< Shifted mode 6BIT for ADC_SCANCTRL */
#define ADC_SCANCTRL_RES_OVS                               (_ADC_SCANCTRL_RES_OVS << 3)        /**< Shifted mode OVS for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_SHIFT                            5                                   /**< Shift value for ADC_REF */
#define _ADC_SCANCTRL_REF_MASK                             0xE0UL                              /**< Bit mask for ADC_REF */
#define _ADC_SCANCTRL_REF_DEFAULT                          0x00000000UL                        /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_1V25                             0x00000000UL                        /**< Mode 1V25 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_2V5                              0x00000001UL                        /**< Mode 2V5 for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_VDD                              0x00000002UL                        /**< Mode VDD for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_5V                               0x00000003UL                        /**< Mode 5V for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_EXTSINGLE                        0x00000004UL                        /**< Mode EXTSINGLE for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_2XEXTDIFF                        0x00000005UL                        /**< Mode 2XEXTDIFF for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_2XVDD                            0x00000006UL                        /**< Mode 2XVDD for ADC_SCANCTRL */
#define _ADC_SCANCTRL_REF_CONF                             0x00000007UL                        /**< Mode CONF for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_DEFAULT                           (_ADC_SCANCTRL_REF_DEFAULT << 5)    /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_1V25                              (_ADC_SCANCTRL_REF_1V25 << 5)       /**< Shifted mode 1V25 for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_2V5                               (_ADC_SCANCTRL_REF_2V5 << 5)        /**< Shifted mode 2V5 for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_VDD                               (_ADC_SCANCTRL_REF_VDD << 5)        /**< Shifted mode VDD for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_5V                                (_ADC_SCANCTRL_REF_5V << 5)         /**< Shifted mode 5V for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_EXTSINGLE                         (_ADC_SCANCTRL_REF_EXTSINGLE << 5)  /**< Shifted mode EXTSINGLE for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_2XEXTDIFF                         (_ADC_SCANCTRL_REF_2XEXTDIFF << 5)  /**< Shifted mode 2XEXTDIFF for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_2XVDD                             (_ADC_SCANCTRL_REF_2XVDD << 5)      /**< Shifted mode 2XVDD for ADC_SCANCTRL */
#define ADC_SCANCTRL_REF_CONF                              (_ADC_SCANCTRL_REF_CONF << 5)       /**< Shifted mode CONF for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_SHIFT                             24                                  /**< Shift value for ADC_AT */
#define _ADC_SCANCTRL_AT_MASK                              0xF000000UL                         /**< Bit mask for ADC_AT */
#define _ADC_SCANCTRL_AT_DEFAULT                           0x00000000UL                        /**< Mode DEFAULT for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_1CYCLE                            0x00000000UL                        /**< Mode 1CYCLE for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_2CYCLES                           0x00000001UL                        /**< Mode 2CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_3CYCLES                           0x00000002UL                        /**< Mode 3CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_4CYCLES                           0x00000003UL                        /**< Mode 4CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_8CYCLES                           0x00000004UL                        /**< Mode 8CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_16CYCLES                          0x00000005UL                        /**< Mode 16CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_32CYCLES                          0x00000006UL                        /**< Mode 32CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_64CYCLES                          0x00000007UL                        /**< Mode 64CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_128CYCLES                         0x00000008UL                        /**< Mode 128CYCLES for ADC_SCANCTRL */
#define _ADC_SCANCTRL_AT_256CYCLES                         0x00000009UL                        /**< Mode 256CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_DEFAULT                            (_ADC_SCANCTRL_AT_DEFAULT << 24)    /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_1CYCLE                             (_ADC_SCANCTRL_AT_1CYCLE << 24)     /**< Shifted mode 1CYCLE for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_2CYCLES                            (_ADC_SCANCTRL_AT_2CYCLES << 24)    /**< Shifted mode 2CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_3CYCLES                            (_ADC_SCANCTRL_AT_3CYCLES << 24)    /**< Shifted mode 3CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_4CYCLES                            (_ADC_SCANCTRL_AT_4CYCLES << 24)    /**< Shifted mode 4CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_8CYCLES                            (_ADC_SCANCTRL_AT_8CYCLES << 24)    /**< Shifted mode 8CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_16CYCLES                           (_ADC_SCANCTRL_AT_16CYCLES << 24)   /**< Shifted mode 16CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_32CYCLES                           (_ADC_SCANCTRL_AT_32CYCLES << 24)   /**< Shifted mode 32CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_64CYCLES                           (_ADC_SCANCTRL_AT_64CYCLES << 24)   /**< Shifted mode 64CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_128CYCLES                          (_ADC_SCANCTRL_AT_128CYCLES << 24)  /**< Shifted mode 128CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_AT_256CYCLES                          (_ADC_SCANCTRL_AT_256CYCLES << 24)  /**< Shifted mode 256CYCLES for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSEN                                 (0x1UL << 29)                       /**< Scan Sequence PRS Trigger Enable */
#define _ADC_SCANCTRL_PRSEN_SHIFT                          29                                  /**< Shift value for ADC_PRSEN */
#define _ADC_SCANCTRL_PRSEN_MASK                           0x20000000UL                        /**< Bit mask for ADC_PRSEN */
#define _ADC_SCANCTRL_PRSEN_DEFAULT                        0x00000000UL                        /**< Mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_PRSEN_DEFAULT                         (_ADC_SCANCTRL_PRSEN_DEFAULT << 29) /**< Shifted mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_CMPEN                                 (0x1UL << 31)                       /**< Compare Logic Enable for Scan */
#define _ADC_SCANCTRL_CMPEN_SHIFT                          31                                  /**< Shift value for ADC_CMPEN */
#define _ADC_SCANCTRL_CMPEN_MASK                           0x80000000UL                        /**< Bit mask for ADC_CMPEN */
#define _ADC_SCANCTRL_CMPEN_DEFAULT                        0x00000000UL                        /**< Mode DEFAULT for ADC_SCANCTRL */
#define ADC_SCANCTRL_CMPEN_DEFAULT                         (_ADC_SCANCTRL_CMPEN_DEFAULT << 31) /**< Shifted mode DEFAULT for ADC_SCANCTRL */

/* Bit fields for ADC SCANCTRLX */
#define _ADC_SCANCTRLX_RESETVALUE                          0x00000000UL                                    /**< Default value for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_MASK                                0x0F1F7FFFUL                                    /**< Mask for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFSEL_SHIFT                       0                                               /**< Shift value for ADC_VREFSEL */
#define _ADC_SCANCTRLX_VREFSEL_MASK                        0x7UL                                           /**< Bit mask for ADC_VREFSEL */
#define _ADC_SCANCTRLX_VREFSEL_DEFAULT                     0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFSEL_VBGR                        0x00000000UL                                    /**< Mode VBGR for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFSEL_VDDXWATT                    0x00000001UL                                    /**< Mode VDDXWATT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFSEL_VREFPWATT                   0x00000002UL                                    /**< Mode VREFPWATT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFSEL_VREFP                       0x00000003UL                                    /**< Mode VREFP for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFSEL_VREFPNWATT                  0x00000005UL                                    /**< Mode VREFPNWATT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFSEL_VREFPN                      0x00000006UL                                    /**< Mode VREFPN for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFSEL_VBGRLOW                     0x00000007UL                                    /**< Mode VBGRLOW for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFSEL_DEFAULT                      (_ADC_SCANCTRLX_VREFSEL_DEFAULT << 0)           /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFSEL_VBGR                         (_ADC_SCANCTRLX_VREFSEL_VBGR << 0)              /**< Shifted mode VBGR for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFSEL_VDDXWATT                     (_ADC_SCANCTRLX_VREFSEL_VDDXWATT << 0)          /**< Shifted mode VDDXWATT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFSEL_VREFPWATT                    (_ADC_SCANCTRLX_VREFSEL_VREFPWATT << 0)         /**< Shifted mode VREFPWATT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFSEL_VREFP                        (_ADC_SCANCTRLX_VREFSEL_VREFP << 0)             /**< Shifted mode VREFP for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFSEL_VREFPNWATT                   (_ADC_SCANCTRLX_VREFSEL_VREFPNWATT << 0)        /**< Shifted mode VREFPNWATT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFSEL_VREFPN                       (_ADC_SCANCTRLX_VREFSEL_VREFPN << 0)            /**< Shifted mode VREFPN for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFSEL_VBGRLOW                      (_ADC_SCANCTRLX_VREFSEL_VBGRLOW << 0)           /**< Shifted mode VBGRLOW for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFATTFIX                           (0x1UL << 3)                                    /**< Enable fixed scaling on VREF */
#define _ADC_SCANCTRLX_VREFATTFIX_SHIFT                    3                                               /**< Shift value for ADC_VREFATTFIX */
#define _ADC_SCANCTRLX_VREFATTFIX_MASK                     0x8UL                                           /**< Bit mask for ADC_VREFATTFIX */
#define _ADC_SCANCTRLX_VREFATTFIX_DEFAULT                  0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFATTFIX_DEFAULT                   (_ADC_SCANCTRLX_VREFATTFIX_DEFAULT << 3)        /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VREFATT_SHIFT                       4                                               /**< Shift value for ADC_VREFATT */
#define _ADC_SCANCTRLX_VREFATT_MASK                        0xF0UL                                          /**< Bit mask for ADC_VREFATT */
#define _ADC_SCANCTRLX_VREFATT_DEFAULT                     0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VREFATT_DEFAULT                      (_ADC_SCANCTRLX_VREFATT_DEFAULT << 4)           /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_VINATT_SHIFT                        8                                               /**< Shift value for ADC_VINATT */
#define _ADC_SCANCTRLX_VINATT_MASK                         0xF00UL                                         /**< Bit mask for ADC_VINATT */
#define _ADC_SCANCTRLX_VINATT_DEFAULT                      0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_VINATT_DEFAULT                       (_ADC_SCANCTRLX_VINATT_DEFAULT << 8)            /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_DVL_SHIFT                           12                                              /**< Shift value for ADC_DVL */
#define _ADC_SCANCTRLX_DVL_MASK                            0x3000UL                                        /**< Bit mask for ADC_DVL */
#define _ADC_SCANCTRLX_DVL_DEFAULT                         0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_DVL_DEFAULT                          (_ADC_SCANCTRLX_DVL_DEFAULT << 12)              /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_FIFOOFACT                            (0x1UL << 14)                                   /**< Scan FIFO Overflow Action */
#define _ADC_SCANCTRLX_FIFOOFACT_SHIFT                     14                                              /**< Shift value for ADC_FIFOOFACT */
#define _ADC_SCANCTRLX_FIFOOFACT_MASK                      0x4000UL                                        /**< Bit mask for ADC_FIFOOFACT */
#define _ADC_SCANCTRLX_FIFOOFACT_DEFAULT                   0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_FIFOOFACT_DISCARD                   0x00000000UL                                    /**< Mode DISCARD for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_FIFOOFACT_OVERWRITE                 0x00000001UL                                    /**< Mode OVERWRITE for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_FIFOOFACT_DEFAULT                    (_ADC_SCANCTRLX_FIFOOFACT_DEFAULT << 14)        /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_FIFOOFACT_DISCARD                    (_ADC_SCANCTRLX_FIFOOFACT_DISCARD << 14)        /**< Shifted mode DISCARD for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_FIFOOFACT_OVERWRITE                  (_ADC_SCANCTRLX_FIFOOFACT_OVERWRITE << 14)      /**< Shifted mode OVERWRITE for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSMODE                              (0x1UL << 16)                                   /**< Scan PRS Trigger Mode */
#define _ADC_SCANCTRLX_PRSMODE_SHIFT                       16                                              /**< Shift value for ADC_PRSMODE */
#define _ADC_SCANCTRLX_PRSMODE_MASK                        0x10000UL                                       /**< Bit mask for ADC_PRSMODE */
#define _ADC_SCANCTRLX_PRSMODE_DEFAULT                     0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSMODE_PULSED                      0x00000000UL                                    /**< Mode PULSED for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSMODE_TIMED                       0x00000001UL                                    /**< Mode TIMED for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSMODE_DEFAULT                      (_ADC_SCANCTRLX_PRSMODE_DEFAULT << 16)          /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSMODE_PULSED                       (_ADC_SCANCTRLX_PRSMODE_PULSED << 16)           /**< Shifted mode PULSED for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSMODE_TIMED                        (_ADC_SCANCTRLX_PRSMODE_TIMED << 16)            /**< Shifted mode TIMED for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_SHIFT                        17                                              /**< Shift value for ADC_PRSSEL */
#define _ADC_SCANCTRLX_PRSSEL_MASK                         0x1E0000UL                                      /**< Bit mask for ADC_PRSSEL */
#define _ADC_SCANCTRLX_PRSSEL_DEFAULT                      0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH0                       0x00000000UL                                    /**< Mode PRSCH0 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH1                       0x00000001UL                                    /**< Mode PRSCH1 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH2                       0x00000002UL                                    /**< Mode PRSCH2 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH3                       0x00000003UL                                    /**< Mode PRSCH3 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH4                       0x00000004UL                                    /**< Mode PRSCH4 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH5                       0x00000005UL                                    /**< Mode PRSCH5 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH6                       0x00000006UL                                    /**< Mode PRSCH6 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH7                       0x00000007UL                                    /**< Mode PRSCH7 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH8                       0x00000008UL                                    /**< Mode PRSCH8 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH9                       0x00000009UL                                    /**< Mode PRSCH9 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH10                      0x0000000AUL                                    /**< Mode PRSCH10 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_PRSSEL_PRSCH11                      0x0000000BUL                                    /**< Mode PRSCH11 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_DEFAULT                       (_ADC_SCANCTRLX_PRSSEL_DEFAULT << 17)           /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH0                        (_ADC_SCANCTRLX_PRSSEL_PRSCH0 << 17)            /**< Shifted mode PRSCH0 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH1                        (_ADC_SCANCTRLX_PRSSEL_PRSCH1 << 17)            /**< Shifted mode PRSCH1 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH2                        (_ADC_SCANCTRLX_PRSSEL_PRSCH2 << 17)            /**< Shifted mode PRSCH2 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH3                        (_ADC_SCANCTRLX_PRSSEL_PRSCH3 << 17)            /**< Shifted mode PRSCH3 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH4                        (_ADC_SCANCTRLX_PRSSEL_PRSCH4 << 17)            /**< Shifted mode PRSCH4 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH5                        (_ADC_SCANCTRLX_PRSSEL_PRSCH5 << 17)            /**< Shifted mode PRSCH5 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH6                        (_ADC_SCANCTRLX_PRSSEL_PRSCH6 << 17)            /**< Shifted mode PRSCH6 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH7                        (_ADC_SCANCTRLX_PRSSEL_PRSCH7 << 17)            /**< Shifted mode PRSCH7 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH8                        (_ADC_SCANCTRLX_PRSSEL_PRSCH8 << 17)            /**< Shifted mode PRSCH8 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH9                        (_ADC_SCANCTRLX_PRSSEL_PRSCH9 << 17)            /**< Shifted mode PRSCH9 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH10                       (_ADC_SCANCTRLX_PRSSEL_PRSCH10 << 17)           /**< Shifted mode PRSCH10 for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_PRSSEL_PRSCH11                       (_ADC_SCANCTRLX_PRSSEL_PRSCH11 << 17)           /**< Shifted mode PRSCH11 for ADC_SCANCTRLX */
#define _ADC_SCANCTRLX_CONVSTARTDELAY_SHIFT                24                                              /**< Shift value for ADC_CONVSTARTDELAY */
#define _ADC_SCANCTRLX_CONVSTARTDELAY_MASK                 0x7000000UL                                     /**< Bit mask for ADC_CONVSTARTDELAY */
#define _ADC_SCANCTRLX_CONVSTARTDELAY_DEFAULT              0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_CONVSTARTDELAY_DEFAULT               (_ADC_SCANCTRLX_CONVSTARTDELAY_DEFAULT << 24)   /**< Shifted mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_CONVSTARTDELAYEN                     (0x1UL << 27)                                   /**< Enable delaying next conversion start */
#define _ADC_SCANCTRLX_CONVSTARTDELAYEN_SHIFT              27                                              /**< Shift value for ADC_CONVSTARTDELAYEN */
#define _ADC_SCANCTRLX_CONVSTARTDELAYEN_MASK               0x8000000UL                                     /**< Bit mask for ADC_CONVSTARTDELAYEN */
#define _ADC_SCANCTRLX_CONVSTARTDELAYEN_DEFAULT            0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANCTRLX */
#define ADC_SCANCTRLX_CONVSTARTDELAYEN_DEFAULT             (_ADC_SCANCTRLX_CONVSTARTDELAYEN_DEFAULT << 27) /**< Shifted mode DEFAULT for ADC_SCANCTRLX */

/* Bit fields for ADC SCANMASK */
#define _ADC_SCANMASK_RESETVALUE                           0x00000000UL                                          /**< Default value for ADC_SCANMASK */
#define _ADC_SCANMASK_MASK                                 0xFFFFFFFFUL                                          /**< Mask for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_SHIFT                    0                                                     /**< Shift value for ADC_SCANINPUTEN */
#define _ADC_SCANMASK_SCANINPUTEN_MASK                     0xFFFFFFFFUL                                          /**< Bit mask for ADC_SCANINPUTEN */
#define _ADC_SCANMASK_SCANINPUTEN_DEFAULT                  0x00000000UL                                          /**< Mode DEFAULT for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT0INPUT0NEGSEL       0x00000001UL                                          /**< Mode INPUT0INPUT0NEGSEL for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT0                   0x00000001UL                                          /**< Mode INPUT0 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT1                   0x00000002UL                                          /**< Mode INPUT1 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT1INPUT2             0x00000002UL                                          /**< Mode INPUT1INPUT2 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT2                   0x00000004UL                                          /**< Mode INPUT2 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT2INPUT2NEGSEL       0x00000004UL                                          /**< Mode INPUT2INPUT2NEGSEL for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT3                   0x00000008UL                                          /**< Mode INPUT3 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT3INPUT4             0x00000008UL                                          /**< Mode INPUT3INPUT4 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT4                   0x00000010UL                                          /**< Mode INPUT4 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT4INPUT4NEGSEL       0x00000010UL                                          /**< Mode INPUT4INPUT4NEGSEL for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT5INPUT6             0x00000020UL                                          /**< Mode INPUT5INPUT6 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT5                   0x00000020UL                                          /**< Mode INPUT5 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT6INPUT6NEGSEL       0x00000040UL                                          /**< Mode INPUT6INPUT6NEGSEL for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT6                   0x00000040UL                                          /**< Mode INPUT6 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT7                   0x00000080UL                                          /**< Mode INPUT7 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT7INPUT0             0x00000080UL                                          /**< Mode INPUT7INPUT0 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT8INPUT9             0x00000100UL                                          /**< Mode INPUT8INPUT9 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT8                   0x00000100UL                                          /**< Mode INPUT8 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT9                   0x00000200UL                                          /**< Mode INPUT9 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT9INPUT9NEGSEL       0x00000200UL                                          /**< Mode INPUT9INPUT9NEGSEL for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT10INPUT11           0x00000400UL                                          /**< Mode INPUT10INPUT11 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT10                  0x00000400UL                                          /**< Mode INPUT10 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT11INPUT11NEGSEL     0x00000800UL                                          /**< Mode INPUT11INPUT11NEGSEL for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT11                  0x00000800UL                                          /**< Mode INPUT11 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT12INPUT13           0x00001000UL                                          /**< Mode INPUT12INPUT13 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT12                  0x00001000UL                                          /**< Mode INPUT12 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT13INPUT13NEGSEL     0x00002000UL                                          /**< Mode INPUT13INPUT13NEGSEL for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT13                  0x00002000UL                                          /**< Mode INPUT13 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT14INPUT15           0x00004000UL                                          /**< Mode INPUT14INPUT15 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT14                  0x00004000UL                                          /**< Mode INPUT14 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT15INPUT15NEGSEL     0x00008000UL                                          /**< Mode INPUT15INPUT15NEGSEL for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT15                  0x00008000UL                                          /**< Mode INPUT15 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT16INPUT17           0x00010000UL                                          /**< Mode INPUT16INPUT17 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT16                  0x00010000UL                                          /**< Mode INPUT16 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT17INPUT18           0x00020000UL                                          /**< Mode INPUT17INPUT18 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT17                  0x00020000UL                                          /**< Mode INPUT17 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT18INPUT19           0x00040000UL                                          /**< Mode INPUT18INPUT19 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT18                  0x00040000UL                                          /**< Mode INPUT18 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT19                  0x00080000UL                                          /**< Mode INPUT19 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT19INPUT20           0x00080000UL                                          /**< Mode INPUT19INPUT20 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT20INPUT21           0x00100000UL                                          /**< Mode INPUT20INPUT21 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT20                  0x00100000UL                                          /**< Mode INPUT20 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT21                  0x00200000UL                                          /**< Mode INPUT21 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT21INPUT22           0x00200000UL                                          /**< Mode INPUT21INPUT22 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT22INPUT23           0x00400000UL                                          /**< Mode INPUT22INPUT23 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT22                  0x00400000UL                                          /**< Mode INPUT22 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT23INPUT16           0x00800000UL                                          /**< Mode INPUT23INPUT16 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT23                  0x00800000UL                                          /**< Mode INPUT23 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT24                  0x01000000UL                                          /**< Mode INPUT24 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT24INPUT25           0x01000000UL                                          /**< Mode INPUT24INPUT25 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT25INPUT26           0x02000000UL                                          /**< Mode INPUT25INPUT26 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT25                  0x02000000UL                                          /**< Mode INPUT25 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT26                  0x04000000UL                                          /**< Mode INPUT26 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT26INPUT27           0x04000000UL                                          /**< Mode INPUT26INPUT27 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT27INPUT28           0x08000000UL                                          /**< Mode INPUT27INPUT28 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT27                  0x08000000UL                                          /**< Mode INPUT27 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT28INPUT29           0x10000000UL                                          /**< Mode INPUT28INPUT29 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT28                  0x10000000UL                                          /**< Mode INPUT28 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT29                  0x20000000UL                                          /**< Mode INPUT29 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT29INPUT30           0x20000000UL                                          /**< Mode INPUT29INPUT30 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT30                  0x40000000UL                                          /**< Mode INPUT30 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT30INPUT31           0x40000000UL                                          /**< Mode INPUT30INPUT31 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT31INPUT24           0x80000000UL                                          /**< Mode INPUT31INPUT24 for ADC_SCANMASK */
#define _ADC_SCANMASK_SCANINPUTEN_INPUT31                  0x80000000UL                                          /**< Mode INPUT31 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_DEFAULT                   (_ADC_SCANMASK_SCANINPUTEN_DEFAULT << 0)              /**< Shifted mode DEFAULT for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT0INPUT0NEGSEL        (_ADC_SCANMASK_SCANINPUTEN_INPUT0INPUT0NEGSEL << 0)   /**< Shifted mode INPUT0INPUT0NEGSEL for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT0                    (_ADC_SCANMASK_SCANINPUTEN_INPUT0 << 0)               /**< Shifted mode INPUT0 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT1                    (_ADC_SCANMASK_SCANINPUTEN_INPUT1 << 0)               /**< Shifted mode INPUT1 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT1INPUT2              (_ADC_SCANMASK_SCANINPUTEN_INPUT1INPUT2 << 0)         /**< Shifted mode INPUT1INPUT2 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT2                    (_ADC_SCANMASK_SCANINPUTEN_INPUT2 << 0)               /**< Shifted mode INPUT2 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT2INPUT2NEGSEL        (_ADC_SCANMASK_SCANINPUTEN_INPUT2INPUT2NEGSEL << 0)   /**< Shifted mode INPUT2INPUT2NEGSEL for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT3                    (_ADC_SCANMASK_SCANINPUTEN_INPUT3 << 0)               /**< Shifted mode INPUT3 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT3INPUT4              (_ADC_SCANMASK_SCANINPUTEN_INPUT3INPUT4 << 0)         /**< Shifted mode INPUT3INPUT4 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT4                    (_ADC_SCANMASK_SCANINPUTEN_INPUT4 << 0)               /**< Shifted mode INPUT4 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT4INPUT4NEGSEL        (_ADC_SCANMASK_SCANINPUTEN_INPUT4INPUT4NEGSEL << 0)   /**< Shifted mode INPUT4INPUT4NEGSEL for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT5INPUT6              (_ADC_SCANMASK_SCANINPUTEN_INPUT5INPUT6 << 0)         /**< Shifted mode INPUT5INPUT6 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT5                    (_ADC_SCANMASK_SCANINPUTEN_INPUT5 << 0)               /**< Shifted mode INPUT5 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT6INPUT6NEGSEL        (_ADC_SCANMASK_SCANINPUTEN_INPUT6INPUT6NEGSEL << 0)   /**< Shifted mode INPUT6INPUT6NEGSEL for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT6                    (_ADC_SCANMASK_SCANINPUTEN_INPUT6 << 0)               /**< Shifted mode INPUT6 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT7                    (_ADC_SCANMASK_SCANINPUTEN_INPUT7 << 0)               /**< Shifted mode INPUT7 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT7INPUT0              (_ADC_SCANMASK_SCANINPUTEN_INPUT7INPUT0 << 0)         /**< Shifted mode INPUT7INPUT0 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT8INPUT9              (_ADC_SCANMASK_SCANINPUTEN_INPUT8INPUT9 << 0)         /**< Shifted mode INPUT8INPUT9 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT8                    (_ADC_SCANMASK_SCANINPUTEN_INPUT8 << 0)               /**< Shifted mode INPUT8 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT9                    (_ADC_SCANMASK_SCANINPUTEN_INPUT9 << 0)               /**< Shifted mode INPUT9 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT9INPUT9NEGSEL        (_ADC_SCANMASK_SCANINPUTEN_INPUT9INPUT9NEGSEL << 0)   /**< Shifted mode INPUT9INPUT9NEGSEL for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT10INPUT11            (_ADC_SCANMASK_SCANINPUTEN_INPUT10INPUT11 << 0)       /**< Shifted mode INPUT10INPUT11 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT10                   (_ADC_SCANMASK_SCANINPUTEN_INPUT10 << 0)              /**< Shifted mode INPUT10 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT11INPUT11NEGSEL      (_ADC_SCANMASK_SCANINPUTEN_INPUT11INPUT11NEGSEL << 0) /**< Shifted mode INPUT11INPUT11NEGSEL for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT11                   (_ADC_SCANMASK_SCANINPUTEN_INPUT11 << 0)              /**< Shifted mode INPUT11 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT12INPUT13            (_ADC_SCANMASK_SCANINPUTEN_INPUT12INPUT13 << 0)       /**< Shifted mode INPUT12INPUT13 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT12                   (_ADC_SCANMASK_SCANINPUTEN_INPUT12 << 0)              /**< Shifted mode INPUT12 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT13INPUT13NEGSEL      (_ADC_SCANMASK_SCANINPUTEN_INPUT13INPUT13NEGSEL << 0) /**< Shifted mode INPUT13INPUT13NEGSEL for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT13                   (_ADC_SCANMASK_SCANINPUTEN_INPUT13 << 0)              /**< Shifted mode INPUT13 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT14INPUT15            (_ADC_SCANMASK_SCANINPUTEN_INPUT14INPUT15 << 0)       /**< Shifted mode INPUT14INPUT15 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT14                   (_ADC_SCANMASK_SCANINPUTEN_INPUT14 << 0)              /**< Shifted mode INPUT14 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT15INPUT15NEGSEL      (_ADC_SCANMASK_SCANINPUTEN_INPUT15INPUT15NEGSEL << 0) /**< Shifted mode INPUT15INPUT15NEGSEL for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT15                   (_ADC_SCANMASK_SCANINPUTEN_INPUT15 << 0)              /**< Shifted mode INPUT15 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT16INPUT17            (_ADC_SCANMASK_SCANINPUTEN_INPUT16INPUT17 << 0)       /**< Shifted mode INPUT16INPUT17 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT16                   (_ADC_SCANMASK_SCANINPUTEN_INPUT16 << 0)              /**< Shifted mode INPUT16 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT17INPUT18            (_ADC_SCANMASK_SCANINPUTEN_INPUT17INPUT18 << 0)       /**< Shifted mode INPUT17INPUT18 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT17                   (_ADC_SCANMASK_SCANINPUTEN_INPUT17 << 0)              /**< Shifted mode INPUT17 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT18INPUT19            (_ADC_SCANMASK_SCANINPUTEN_INPUT18INPUT19 << 0)       /**< Shifted mode INPUT18INPUT19 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT18                   (_ADC_SCANMASK_SCANINPUTEN_INPUT18 << 0)              /**< Shifted mode INPUT18 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT19                   (_ADC_SCANMASK_SCANINPUTEN_INPUT19 << 0)              /**< Shifted mode INPUT19 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT19INPUT20            (_ADC_SCANMASK_SCANINPUTEN_INPUT19INPUT20 << 0)       /**< Shifted mode INPUT19INPUT20 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT20INPUT21            (_ADC_SCANMASK_SCANINPUTEN_INPUT20INPUT21 << 0)       /**< Shifted mode INPUT20INPUT21 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT20                   (_ADC_SCANMASK_SCANINPUTEN_INPUT20 << 0)              /**< Shifted mode INPUT20 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT21                   (_ADC_SCANMASK_SCANINPUTEN_INPUT21 << 0)              /**< Shifted mode INPUT21 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT21INPUT22            (_ADC_SCANMASK_SCANINPUTEN_INPUT21INPUT22 << 0)       /**< Shifted mode INPUT21INPUT22 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT22INPUT23            (_ADC_SCANMASK_SCANINPUTEN_INPUT22INPUT23 << 0)       /**< Shifted mode INPUT22INPUT23 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT22                   (_ADC_SCANMASK_SCANINPUTEN_INPUT22 << 0)              /**< Shifted mode INPUT22 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT23INPUT16            (_ADC_SCANMASK_SCANINPUTEN_INPUT23INPUT16 << 0)       /**< Shifted mode INPUT23INPUT16 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT23                   (_ADC_SCANMASK_SCANINPUTEN_INPUT23 << 0)              /**< Shifted mode INPUT23 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT24                   (_ADC_SCANMASK_SCANINPUTEN_INPUT24 << 0)              /**< Shifted mode INPUT24 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT24INPUT25            (_ADC_SCANMASK_SCANINPUTEN_INPUT24INPUT25 << 0)       /**< Shifted mode INPUT24INPUT25 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT25INPUT26            (_ADC_SCANMASK_SCANINPUTEN_INPUT25INPUT26 << 0)       /**< Shifted mode INPUT25INPUT26 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT25                   (_ADC_SCANMASK_SCANINPUTEN_INPUT25 << 0)              /**< Shifted mode INPUT25 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT26                   (_ADC_SCANMASK_SCANINPUTEN_INPUT26 << 0)              /**< Shifted mode INPUT26 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT26INPUT27            (_ADC_SCANMASK_SCANINPUTEN_INPUT26INPUT27 << 0)       /**< Shifted mode INPUT26INPUT27 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT27INPUT28            (_ADC_SCANMASK_SCANINPUTEN_INPUT27INPUT28 << 0)       /**< Shifted mode INPUT27INPUT28 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT27                   (_ADC_SCANMASK_SCANINPUTEN_INPUT27 << 0)              /**< Shifted mode INPUT27 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT28INPUT29            (_ADC_SCANMASK_SCANINPUTEN_INPUT28INPUT29 << 0)       /**< Shifted mode INPUT28INPUT29 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT28                   (_ADC_SCANMASK_SCANINPUTEN_INPUT28 << 0)              /**< Shifted mode INPUT28 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT29                   (_ADC_SCANMASK_SCANINPUTEN_INPUT29 << 0)              /**< Shifted mode INPUT29 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT29INPUT30            (_ADC_SCANMASK_SCANINPUTEN_INPUT29INPUT30 << 0)       /**< Shifted mode INPUT29INPUT30 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT30                   (_ADC_SCANMASK_SCANINPUTEN_INPUT30 << 0)              /**< Shifted mode INPUT30 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT30INPUT31            (_ADC_SCANMASK_SCANINPUTEN_INPUT30INPUT31 << 0)       /**< Shifted mode INPUT30INPUT31 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT31INPUT24            (_ADC_SCANMASK_SCANINPUTEN_INPUT31INPUT24 << 0)       /**< Shifted mode INPUT31INPUT24 for ADC_SCANMASK */
#define ADC_SCANMASK_SCANINPUTEN_INPUT31                   (_ADC_SCANMASK_SCANINPUTEN_INPUT31 << 0)              /**< Shifted mode INPUT31 for ADC_SCANMASK */

/* Bit fields for ADC SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_RESETVALUE                       0x00000000UL                                            /**< Default value for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_MASK                             0x1F1F1F1FUL                                            /**< Mask for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_SHIFT               0                                                       /**< Shift value for ADC_INPUT0TO7SEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_MASK                0x1FUL                                                  /**< Bit mask for ADC_INPUT0TO7SEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_DEFAULT             0x00000000UL                                            /**< Mode DEFAULT for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT0CH0TO7        0x00000000UL                                            /**< Mode APORT0CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT0CH8TO15       0x00000001UL                                            /**< Mode APORT0CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH0TO7        0x00000004UL                                            /**< Mode APORT1CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH8TO15       0x00000005UL                                            /**< Mode APORT1CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH16TO23      0x00000006UL                                            /**< Mode APORT1CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH24TO31      0x00000007UL                                            /**< Mode APORT1CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH0TO7        0x00000008UL                                            /**< Mode APORT2CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH8TO15       0x00000009UL                                            /**< Mode APORT2CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH16TO23      0x0000000AUL                                            /**< Mode APORT2CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH24TO31      0x0000000BUL                                            /**< Mode APORT2CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH0TO7        0x0000000CUL                                            /**< Mode APORT3CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH8TO15       0x0000000DUL                                            /**< Mode APORT3CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH16TO23      0x0000000EUL                                            /**< Mode APORT3CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH24TO31      0x0000000FUL                                            /**< Mode APORT3CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH0TO7        0x00000010UL                                            /**< Mode APORT4CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH8TO15       0x00000011UL                                            /**< Mode APORT4CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH16TO23      0x00000012UL                                            /**< Mode APORT4CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH24TO31      0x00000013UL                                            /**< Mode APORT4CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_DEFAULT              (_ADC_SCANINPUTSEL_INPUT0TO7SEL_DEFAULT << 0)           /**< Shifted mode DEFAULT for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT0CH0TO7         (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT0CH0TO7 << 0)      /**< Shifted mode APORT0CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT0CH8TO15        (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT0CH8TO15 << 0)     /**< Shifted mode APORT0CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH0TO7         (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH0TO7 << 0)      /**< Shifted mode APORT1CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH8TO15        (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH8TO15 << 0)     /**< Shifted mode APORT1CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH16TO23       (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH16TO23 << 0)    /**< Shifted mode APORT1CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH24TO31       (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT1CH24TO31 << 0)    /**< Shifted mode APORT1CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH0TO7         (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH0TO7 << 0)      /**< Shifted mode APORT2CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH8TO15        (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH8TO15 << 0)     /**< Shifted mode APORT2CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH16TO23       (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH16TO23 << 0)    /**< Shifted mode APORT2CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH24TO31       (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT2CH24TO31 << 0)    /**< Shifted mode APORT2CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH0TO7         (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH0TO7 << 0)      /**< Shifted mode APORT3CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH8TO15        (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH8TO15 << 0)     /**< Shifted mode APORT3CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH16TO23       (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH16TO23 << 0)    /**< Shifted mode APORT3CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH24TO31       (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT3CH24TO31 << 0)    /**< Shifted mode APORT3CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH0TO7         (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH0TO7 << 0)      /**< Shifted mode APORT4CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH8TO15        (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH8TO15 << 0)     /**< Shifted mode APORT4CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH16TO23       (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH16TO23 << 0)    /**< Shifted mode APORT4CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH24TO31       (_ADC_SCANINPUTSEL_INPUT0TO7SEL_APORT4CH24TO31 << 0)    /**< Shifted mode APORT4CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_SHIFT              8                                                       /**< Shift value for ADC_INPUT8TO15SEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_MASK               0x1F00UL                                                /**< Bit mask for ADC_INPUT8TO15SEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_DEFAULT            0x00000000UL                                            /**< Mode DEFAULT for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT0CH0TO7       0x00000000UL                                            /**< Mode APORT0CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT0CH8TO15      0x00000001UL                                            /**< Mode APORT0CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH0TO7       0x00000004UL                                            /**< Mode APORT1CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH8TO15      0x00000005UL                                            /**< Mode APORT1CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH16TO23     0x00000006UL                                            /**< Mode APORT1CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH24TO31     0x00000007UL                                            /**< Mode APORT1CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH0TO7       0x00000008UL                                            /**< Mode APORT2CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH8TO15      0x00000009UL                                            /**< Mode APORT2CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH16TO23     0x0000000AUL                                            /**< Mode APORT2CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH24TO31     0x0000000BUL                                            /**< Mode APORT2CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH0TO7       0x0000000CUL                                            /**< Mode APORT3CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH8TO15      0x0000000DUL                                            /**< Mode APORT3CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH16TO23     0x0000000EUL                                            /**< Mode APORT3CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH24TO31     0x0000000FUL                                            /**< Mode APORT3CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH0TO7       0x00000010UL                                            /**< Mode APORT4CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH8TO15      0x00000011UL                                            /**< Mode APORT4CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH16TO23     0x00000012UL                                            /**< Mode APORT4CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH24TO31     0x00000013UL                                            /**< Mode APORT4CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_DEFAULT             (_ADC_SCANINPUTSEL_INPUT8TO15SEL_DEFAULT << 8)          /**< Shifted mode DEFAULT for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT0CH0TO7        (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT0CH0TO7 << 8)     /**< Shifted mode APORT0CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT0CH8TO15       (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT0CH8TO15 << 8)    /**< Shifted mode APORT0CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH0TO7        (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH0TO7 << 8)     /**< Shifted mode APORT1CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH8TO15       (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH8TO15 << 8)    /**< Shifted mode APORT1CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH16TO23      (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH16TO23 << 8)   /**< Shifted mode APORT1CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH24TO31      (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT1CH24TO31 << 8)   /**< Shifted mode APORT1CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH0TO7        (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH0TO7 << 8)     /**< Shifted mode APORT2CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH8TO15       (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH8TO15 << 8)    /**< Shifted mode APORT2CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH16TO23      (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH16TO23 << 8)   /**< Shifted mode APORT2CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH24TO31      (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT2CH24TO31 << 8)   /**< Shifted mode APORT2CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH0TO7        (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH0TO7 << 8)     /**< Shifted mode APORT3CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH8TO15       (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH8TO15 << 8)    /**< Shifted mode APORT3CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH16TO23      (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH16TO23 << 8)   /**< Shifted mode APORT3CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH24TO31      (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT3CH24TO31 << 8)   /**< Shifted mode APORT3CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH0TO7        (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH0TO7 << 8)     /**< Shifted mode APORT4CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH8TO15       (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH8TO15 << 8)    /**< Shifted mode APORT4CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH16TO23      (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH16TO23 << 8)   /**< Shifted mode APORT4CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH24TO31      (_ADC_SCANINPUTSEL_INPUT8TO15SEL_APORT4CH24TO31 << 8)   /**< Shifted mode APORT4CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_SHIFT             16                                                      /**< Shift value for ADC_INPUT16TO23SEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_MASK              0x1F0000UL                                              /**< Bit mask for ADC_INPUT16TO23SEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_DEFAULT           0x00000000UL                                            /**< Mode DEFAULT for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT0CH0TO7      0x00000000UL                                            /**< Mode APORT0CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT0CH8TO15     0x00000001UL                                            /**< Mode APORT0CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH0TO7      0x00000004UL                                            /**< Mode APORT1CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH8TO15     0x00000005UL                                            /**< Mode APORT1CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH16TO23    0x00000006UL                                            /**< Mode APORT1CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH24TO31    0x00000007UL                                            /**< Mode APORT1CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH0TO7      0x00000008UL                                            /**< Mode APORT2CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH8TO15     0x00000009UL                                            /**< Mode APORT2CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH16TO23    0x0000000AUL                                            /**< Mode APORT2CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH24TO31    0x0000000BUL                                            /**< Mode APORT2CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH0TO7      0x0000000CUL                                            /**< Mode APORT3CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH8TO15     0x0000000DUL                                            /**< Mode APORT3CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH16TO23    0x0000000EUL                                            /**< Mode APORT3CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH24TO31    0x0000000FUL                                            /**< Mode APORT3CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH0TO7      0x00000010UL                                            /**< Mode APORT4CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH8TO15     0x00000011UL                                            /**< Mode APORT4CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH16TO23    0x00000012UL                                            /**< Mode APORT4CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH24TO31    0x00000013UL                                            /**< Mode APORT4CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_DEFAULT            (_ADC_SCANINPUTSEL_INPUT16TO23SEL_DEFAULT << 16)        /**< Shifted mode DEFAULT for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT0CH0TO7       (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT0CH0TO7 << 16)   /**< Shifted mode APORT0CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT0CH8TO15      (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT0CH8TO15 << 16)  /**< Shifted mode APORT0CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH0TO7       (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH0TO7 << 16)   /**< Shifted mode APORT1CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH8TO15      (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH8TO15 << 16)  /**< Shifted mode APORT1CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH16TO23     (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH16TO23 << 16) /**< Shifted mode APORT1CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH24TO31     (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT1CH24TO31 << 16) /**< Shifted mode APORT1CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH0TO7       (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH0TO7 << 16)   /**< Shifted mode APORT2CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH8TO15      (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH8TO15 << 16)  /**< Shifted mode APORT2CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH16TO23     (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH16TO23 << 16) /**< Shifted mode APORT2CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH24TO31     (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT2CH24TO31 << 16) /**< Shifted mode APORT2CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH0TO7       (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH0TO7 << 16)   /**< Shifted mode APORT3CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH8TO15      (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH8TO15 << 16)  /**< Shifted mode APORT3CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH16TO23     (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH16TO23 << 16) /**< Shifted mode APORT3CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH24TO31     (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT3CH24TO31 << 16) /**< Shifted mode APORT3CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH0TO7       (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH0TO7 << 16)   /**< Shifted mode APORT4CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH8TO15      (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH8TO15 << 16)  /**< Shifted mode APORT4CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH16TO23     (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH16TO23 << 16) /**< Shifted mode APORT4CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH24TO31     (_ADC_SCANINPUTSEL_INPUT16TO23SEL_APORT4CH24TO31 << 16) /**< Shifted mode APORT4CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_SHIFT             24                                                      /**< Shift value for ADC_INPUT24TO31SEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_MASK              0x1F000000UL                                            /**< Bit mask for ADC_INPUT24TO31SEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_DEFAULT           0x00000000UL                                            /**< Mode DEFAULT for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT0CH0TO7      0x00000000UL                                            /**< Mode APORT0CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT0CH8TO15     0x00000001UL                                            /**< Mode APORT0CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH0TO7      0x00000004UL                                            /**< Mode APORT1CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH8TO15     0x00000005UL                                            /**< Mode APORT1CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH16TO23    0x00000006UL                                            /**< Mode APORT1CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH24TO31    0x00000007UL                                            /**< Mode APORT1CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH0TO7      0x00000008UL                                            /**< Mode APORT2CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH8TO15     0x00000009UL                                            /**< Mode APORT2CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH16TO23    0x0000000AUL                                            /**< Mode APORT2CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH24TO31    0x0000000BUL                                            /**< Mode APORT2CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH0TO7      0x0000000CUL                                            /**< Mode APORT3CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH8TO15     0x0000000DUL                                            /**< Mode APORT3CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH16TO23    0x0000000EUL                                            /**< Mode APORT3CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH24TO31    0x0000000FUL                                            /**< Mode APORT3CH24TO31 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH0TO7      0x00000010UL                                            /**< Mode APORT4CH0TO7 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH8TO15     0x00000011UL                                            /**< Mode APORT4CH8TO15 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH16TO23    0x00000012UL                                            /**< Mode APORT4CH16TO23 for ADC_SCANINPUTSEL */
#define _ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH24TO31    0x00000013UL                                            /**< Mode APORT4CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_DEFAULT            (_ADC_SCANINPUTSEL_INPUT24TO31SEL_DEFAULT << 24)        /**< Shifted mode DEFAULT for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT0CH0TO7       (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT0CH0TO7 << 24)   /**< Shifted mode APORT0CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT0CH8TO15      (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT0CH8TO15 << 24)  /**< Shifted mode APORT0CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH0TO7       (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH0TO7 << 24)   /**< Shifted mode APORT1CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH8TO15      (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH8TO15 << 24)  /**< Shifted mode APORT1CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH16TO23     (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH16TO23 << 24) /**< Shifted mode APORT1CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH24TO31     (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT1CH24TO31 << 24) /**< Shifted mode APORT1CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH0TO7       (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH0TO7 << 24)   /**< Shifted mode APORT2CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH8TO15      (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH8TO15 << 24)  /**< Shifted mode APORT2CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH16TO23     (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH16TO23 << 24) /**< Shifted mode APORT2CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH24TO31     (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT2CH24TO31 << 24) /**< Shifted mode APORT2CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH0TO7       (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH0TO7 << 24)   /**< Shifted mode APORT3CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH8TO15      (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH8TO15 << 24)  /**< Shifted mode APORT3CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH16TO23     (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH16TO23 << 24) /**< Shifted mode APORT3CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH24TO31     (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT3CH24TO31 << 24) /**< Shifted mode APORT3CH24TO31 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH0TO7       (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH0TO7 << 24)   /**< Shifted mode APORT4CH0TO7 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH8TO15      (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH8TO15 << 24)  /**< Shifted mode APORT4CH8TO15 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH16TO23     (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH16TO23 << 24) /**< Shifted mode APORT4CH16TO23 for ADC_SCANINPUTSEL */
#define ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH24TO31     (_ADC_SCANINPUTSEL_INPUT24TO31SEL_APORT4CH24TO31 << 24) /**< Shifted mode APORT4CH24TO31 for ADC_SCANINPUTSEL */

/* Bit fields for ADC SCANNEGSEL */
#define _ADC_SCANNEGSEL_RESETVALUE                         0x000039E4UL                                  /**< Default value for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_MASK                               0x0000FFFFUL                                  /**< Mask for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT0NEGSEL_SHIFT                 0                                             /**< Shift value for ADC_INPUT0NEGSEL */
#define _ADC_SCANNEGSEL_INPUT0NEGSEL_MASK                  0x3UL                                         /**< Bit mask for ADC_INPUT0NEGSEL */
#define _ADC_SCANNEGSEL_INPUT0NEGSEL_DEFAULT               0x00000000UL                                  /**< Mode DEFAULT for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT1                0x00000000UL                                  /**< Mode INPUT1 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT3                0x00000001UL                                  /**< Mode INPUT3 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT5                0x00000002UL                                  /**< Mode INPUT5 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT7                0x00000003UL                                  /**< Mode INPUT7 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT0NEGSEL_DEFAULT                (_ADC_SCANNEGSEL_INPUT0NEGSEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT1                 (_ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT1 << 0)    /**< Shifted mode INPUT1 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT3                 (_ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT3 << 0)    /**< Shifted mode INPUT3 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT5                 (_ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT5 << 0)    /**< Shifted mode INPUT5 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT7                 (_ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT7 << 0)    /**< Shifted mode INPUT7 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT2NEGSEL_SHIFT                 2                                             /**< Shift value for ADC_INPUT2NEGSEL */
#define _ADC_SCANNEGSEL_INPUT2NEGSEL_MASK                  0xCUL                                         /**< Bit mask for ADC_INPUT2NEGSEL */
#define _ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT1                0x00000000UL                                  /**< Mode INPUT1 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT2NEGSEL_DEFAULT               0x00000001UL                                  /**< Mode DEFAULT for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT3                0x00000001UL                                  /**< Mode INPUT3 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT5                0x00000002UL                                  /**< Mode INPUT5 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT7                0x00000003UL                                  /**< Mode INPUT7 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT1                 (_ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT1 << 2)    /**< Shifted mode INPUT1 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT2NEGSEL_DEFAULT                (_ADC_SCANNEGSEL_INPUT2NEGSEL_DEFAULT << 2)   /**< Shifted mode DEFAULT for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT3                 (_ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT3 << 2)    /**< Shifted mode INPUT3 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT5                 (_ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT5 << 2)    /**< Shifted mode INPUT5 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT7                 (_ADC_SCANNEGSEL_INPUT2NEGSEL_INPUT7 << 2)    /**< Shifted mode INPUT7 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT4NEGSEL_SHIFT                 4                                             /**< Shift value for ADC_INPUT4NEGSEL */
#define _ADC_SCANNEGSEL_INPUT4NEGSEL_MASK                  0x30UL                                        /**< Bit mask for ADC_INPUT4NEGSEL */
#define _ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT1                0x00000000UL                                  /**< Mode INPUT1 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT3                0x00000001UL                                  /**< Mode INPUT3 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT4NEGSEL_DEFAULT               0x00000002UL                                  /**< Mode DEFAULT for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT5                0x00000002UL                                  /**< Mode INPUT5 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT7                0x00000003UL                                  /**< Mode INPUT7 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT1                 (_ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT1 << 4)    /**< Shifted mode INPUT1 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT3                 (_ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT3 << 4)    /**< Shifted mode INPUT3 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT4NEGSEL_DEFAULT                (_ADC_SCANNEGSEL_INPUT4NEGSEL_DEFAULT << 4)   /**< Shifted mode DEFAULT for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT5                 (_ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT5 << 4)    /**< Shifted mode INPUT5 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT7                 (_ADC_SCANNEGSEL_INPUT4NEGSEL_INPUT7 << 4)    /**< Shifted mode INPUT7 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT6NEGSEL_SHIFT                 6                                             /**< Shift value for ADC_INPUT6NEGSEL */
#define _ADC_SCANNEGSEL_INPUT6NEGSEL_MASK                  0xC0UL                                        /**< Bit mask for ADC_INPUT6NEGSEL */
#define _ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT1                0x00000000UL                                  /**< Mode INPUT1 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT3                0x00000001UL                                  /**< Mode INPUT3 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT5                0x00000002UL                                  /**< Mode INPUT5 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT6NEGSEL_DEFAULT               0x00000003UL                                  /**< Mode DEFAULT for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT7                0x00000003UL                                  /**< Mode INPUT7 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT1                 (_ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT1 << 6)    /**< Shifted mode INPUT1 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT3                 (_ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT3 << 6)    /**< Shifted mode INPUT3 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT5                 (_ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT5 << 6)    /**< Shifted mode INPUT5 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT6NEGSEL_DEFAULT                (_ADC_SCANNEGSEL_INPUT6NEGSEL_DEFAULT << 6)   /**< Shifted mode DEFAULT for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT7                 (_ADC_SCANNEGSEL_INPUT6NEGSEL_INPUT7 << 6)    /**< Shifted mode INPUT7 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT9NEGSEL_SHIFT                 8                                             /**< Shift value for ADC_INPUT9NEGSEL */
#define _ADC_SCANNEGSEL_INPUT9NEGSEL_MASK                  0x300UL                                       /**< Bit mask for ADC_INPUT9NEGSEL */
#define _ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT8                0x00000000UL                                  /**< Mode INPUT8 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT9NEGSEL_DEFAULT               0x00000001UL                                  /**< Mode DEFAULT for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT10               0x00000001UL                                  /**< Mode INPUT10 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT12               0x00000002UL                                  /**< Mode INPUT12 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT14               0x00000003UL                                  /**< Mode INPUT14 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT8                 (_ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT8 << 8)    /**< Shifted mode INPUT8 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT9NEGSEL_DEFAULT                (_ADC_SCANNEGSEL_INPUT9NEGSEL_DEFAULT << 8)   /**< Shifted mode DEFAULT for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT10                (_ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT10 << 8)   /**< Shifted mode INPUT10 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT12                (_ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT12 << 8)   /**< Shifted mode INPUT12 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT14                (_ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT14 << 8)   /**< Shifted mode INPUT14 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT11NEGSEL_SHIFT                10                                            /**< Shift value for ADC_INPUT11NEGSEL */
#define _ADC_SCANNEGSEL_INPUT11NEGSEL_MASK                 0xC00UL                                       /**< Bit mask for ADC_INPUT11NEGSEL */
#define _ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT8               0x00000000UL                                  /**< Mode INPUT8 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT10              0x00000001UL                                  /**< Mode INPUT10 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT11NEGSEL_DEFAULT              0x00000002UL                                  /**< Mode DEFAULT for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT12              0x00000002UL                                  /**< Mode INPUT12 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT14              0x00000003UL                                  /**< Mode INPUT14 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT8                (_ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT8 << 10)  /**< Shifted mode INPUT8 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT10               (_ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT10 << 10) /**< Shifted mode INPUT10 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT11NEGSEL_DEFAULT               (_ADC_SCANNEGSEL_INPUT11NEGSEL_DEFAULT << 10) /**< Shifted mode DEFAULT for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT12               (_ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT12 << 10) /**< Shifted mode INPUT12 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT14               (_ADC_SCANNEGSEL_INPUT11NEGSEL_INPUT14 << 10) /**< Shifted mode INPUT14 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT13NEGSEL_SHIFT                12                                            /**< Shift value for ADC_INPUT13NEGSEL */
#define _ADC_SCANNEGSEL_INPUT13NEGSEL_MASK                 0x3000UL                                      /**< Bit mask for ADC_INPUT13NEGSEL */
#define _ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT8               0x00000000UL                                  /**< Mode INPUT8 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT10              0x00000001UL                                  /**< Mode INPUT10 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT12              0x00000002UL                                  /**< Mode INPUT12 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT13NEGSEL_DEFAULT              0x00000003UL                                  /**< Mode DEFAULT for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT14              0x00000003UL                                  /**< Mode INPUT14 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT8                (_ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT8 << 12)  /**< Shifted mode INPUT8 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT10               (_ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT10 << 12) /**< Shifted mode INPUT10 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT12               (_ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT12 << 12) /**< Shifted mode INPUT12 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT13NEGSEL_DEFAULT               (_ADC_SCANNEGSEL_INPUT13NEGSEL_DEFAULT << 12) /**< Shifted mode DEFAULT for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT14               (_ADC_SCANNEGSEL_INPUT13NEGSEL_INPUT14 << 12) /**< Shifted mode INPUT14 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT15NEGSEL_SHIFT                14                                            /**< Shift value for ADC_INPUT15NEGSEL */
#define _ADC_SCANNEGSEL_INPUT15NEGSEL_MASK                 0xC000UL                                      /**< Bit mask for ADC_INPUT15NEGSEL */
#define _ADC_SCANNEGSEL_INPUT15NEGSEL_DEFAULT              0x00000000UL                                  /**< Mode DEFAULT for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT8               0x00000000UL                                  /**< Mode INPUT8 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT10              0x00000001UL                                  /**< Mode INPUT10 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT12              0x00000002UL                                  /**< Mode INPUT12 for ADC_SCANNEGSEL */
#define _ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT14              0x00000003UL                                  /**< Mode INPUT14 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT15NEGSEL_DEFAULT               (_ADC_SCANNEGSEL_INPUT15NEGSEL_DEFAULT << 14) /**< Shifted mode DEFAULT for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT8                (_ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT8 << 14)  /**< Shifted mode INPUT8 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT10               (_ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT10 << 14) /**< Shifted mode INPUT10 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT12               (_ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT12 << 14) /**< Shifted mode INPUT12 for ADC_SCANNEGSEL */
#define ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT14               (_ADC_SCANNEGSEL_INPUT15NEGSEL_INPUT14 << 14) /**< Shifted mode INPUT14 for ADC_SCANNEGSEL */

/* Bit fields for ADC CMPTHR */
#define _ADC_CMPTHR_RESETVALUE                             0x00000000UL                     /**< Default value for ADC_CMPTHR */
#define _ADC_CMPTHR_MASK                                   0xFFFFFFFFUL                     /**< Mask for ADC_CMPTHR */
#define _ADC_CMPTHR_ADLT_SHIFT                             0                                /**< Shift value for ADC_ADLT */
#define _ADC_CMPTHR_ADLT_MASK                              0xFFFFUL                         /**< Bit mask for ADC_ADLT */
#define _ADC_CMPTHR_ADLT_DEFAULT                           0x00000000UL                     /**< Mode DEFAULT for ADC_CMPTHR */
#define ADC_CMPTHR_ADLT_DEFAULT                            (_ADC_CMPTHR_ADLT_DEFAULT << 0)  /**< Shifted mode DEFAULT for ADC_CMPTHR */
#define _ADC_CMPTHR_ADGT_SHIFT                             16                               /**< Shift value for ADC_ADGT */
#define _ADC_CMPTHR_ADGT_MASK                              0xFFFF0000UL                     /**< Bit mask for ADC_ADGT */
#define _ADC_CMPTHR_ADGT_DEFAULT                           0x00000000UL                     /**< Mode DEFAULT for ADC_CMPTHR */
#define ADC_CMPTHR_ADGT_DEFAULT                            (_ADC_CMPTHR_ADGT_DEFAULT << 16) /**< Shifted mode DEFAULT for ADC_CMPTHR */

/* Bit fields for ADC BIASPROG */
#define _ADC_BIASPROG_RESETVALUE                           0x00000000UL                             /**< Default value for ADC_BIASPROG */
#define _ADC_BIASPROG_MASK                                 0x0001100FUL                             /**< Mask for ADC_BIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_SHIFT                    0                                        /**< Shift value for ADC_ADCBIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_MASK                     0xFUL                                    /**< Bit mask for ADC_ADCBIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for ADC_BIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_NORMAL                   0x00000000UL                             /**< Mode NORMAL for ADC_BIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_SCALE2                   0x00000004UL                             /**< Mode SCALE2 for ADC_BIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_SCALE4                   0x00000008UL                             /**< Mode SCALE4 for ADC_BIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_SCALE8                   0x0000000CUL                             /**< Mode SCALE8 for ADC_BIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_SCALE16                  0x0000000EUL                             /**< Mode SCALE16 for ADC_BIASPROG */
#define _ADC_BIASPROG_ADCBIASPROG_SCALE32                  0x0000000FUL                             /**< Mode SCALE32 for ADC_BIASPROG */
#define ADC_BIASPROG_ADCBIASPROG_DEFAULT                   (_ADC_BIASPROG_ADCBIASPROG_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_BIASPROG */
#define ADC_BIASPROG_ADCBIASPROG_NORMAL                    (_ADC_BIASPROG_ADCBIASPROG_NORMAL << 0)  /**< Shifted mode NORMAL for ADC_BIASPROG */
#define ADC_BIASPROG_ADCBIASPROG_SCALE2                    (_ADC_BIASPROG_ADCBIASPROG_SCALE2 << 0)  /**< Shifted mode SCALE2 for ADC_BIASPROG */
#define ADC_BIASPROG_ADCBIASPROG_SCALE4                    (_ADC_BIASPROG_ADCBIASPROG_SCALE4 << 0)  /**< Shifted mode SCALE4 for ADC_BIASPROG */
#define ADC_BIASPROG_ADCBIASPROG_SCALE8                    (_ADC_BIASPROG_ADCBIASPROG_SCALE8 << 0)  /**< Shifted mode SCALE8 for ADC_BIASPROG */
#define ADC_BIASPROG_ADCBIASPROG_SCALE16                   (_ADC_BIASPROG_ADCBIASPROG_SCALE16 << 0) /**< Shifted mode SCALE16 for ADC_BIASPROG */
#define ADC_BIASPROG_ADCBIASPROG_SCALE32                   (_ADC_BIASPROG_ADCBIASPROG_SCALE32 << 0) /**< Shifted mode SCALE32 for ADC_BIASPROG */
#define ADC_BIASPROG_VFAULTCLR                             (0x1UL << 12)                            /**< Clear VREFOF flag */
#define _ADC_BIASPROG_VFAULTCLR_SHIFT                      12                                       /**< Shift value for ADC_VFAULTCLR */
#define _ADC_BIASPROG_VFAULTCLR_MASK                       0x1000UL                                 /**< Bit mask for ADC_VFAULTCLR */
#define _ADC_BIASPROG_VFAULTCLR_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for ADC_BIASPROG */
#define ADC_BIASPROG_VFAULTCLR_DEFAULT                     (_ADC_BIASPROG_VFAULTCLR_DEFAULT << 12)  /**< Shifted mode DEFAULT for ADC_BIASPROG */
#define ADC_BIASPROG_GPBIASACC                             (0x1UL << 16)                            /**< Accuracy setting for the system bias during ADC operation */
#define _ADC_BIASPROG_GPBIASACC_SHIFT                      16                                       /**< Shift value for ADC_GPBIASACC */
#define _ADC_BIASPROG_GPBIASACC_MASK                       0x10000UL                                /**< Bit mask for ADC_GPBIASACC */
#define _ADC_BIASPROG_GPBIASACC_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for ADC_BIASPROG */
#define _ADC_BIASPROG_GPBIASACC_HIGHACC                    0x00000000UL                             /**< Mode HIGHACC for ADC_BIASPROG */
#define _ADC_BIASPROG_GPBIASACC_LOWACC                     0x00000001UL                             /**< Mode LOWACC for ADC_BIASPROG */
#define ADC_BIASPROG_GPBIASACC_DEFAULT                     (_ADC_BIASPROG_GPBIASACC_DEFAULT << 16)  /**< Shifted mode DEFAULT for ADC_BIASPROG */
#define ADC_BIASPROG_GPBIASACC_HIGHACC                     (_ADC_BIASPROG_GPBIASACC_HIGHACC << 16)  /**< Shifted mode HIGHACC for ADC_BIASPROG */
#define ADC_BIASPROG_GPBIASACC_LOWACC                      (_ADC_BIASPROG_GPBIASACC_LOWACC << 16)   /**< Shifted mode LOWACC for ADC_BIASPROG */

/* Bit fields for ADC CAL */
#define _ADC_CAL_RESETVALUE                                0x40784078UL                            /**< Default value for ADC_CAL */
#define _ADC_CAL_MASK                                      0xFFFFFFFFUL                            /**< Mask for ADC_CAL */
#define _ADC_CAL_SINGLEOFFSET_SHIFT                        0                                       /**< Shift value for ADC_SINGLEOFFSET */
#define _ADC_CAL_SINGLEOFFSET_MASK                         0xFUL                                   /**< Bit mask for ADC_SINGLEOFFSET */
#define _ADC_CAL_SINGLEOFFSET_DEFAULT                      0x00000008UL                            /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SINGLEOFFSET_DEFAULT                       (_ADC_CAL_SINGLEOFFSET_DEFAULT << 0)    /**< Shifted mode DEFAULT for ADC_CAL */
#define _ADC_CAL_SINGLEOFFSETINV_SHIFT                     4                                       /**< Shift value for ADC_SINGLEOFFSETINV */
#define _ADC_CAL_SINGLEOFFSETINV_MASK                      0xF0UL                                  /**< Bit mask for ADC_SINGLEOFFSETINV */
#define _ADC_CAL_SINGLEOFFSETINV_DEFAULT                   0x00000007UL                            /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SINGLEOFFSETINV_DEFAULT                    (_ADC_CAL_SINGLEOFFSETINV_DEFAULT << 4) /**< Shifted mode DEFAULT for ADC_CAL */
#define _ADC_CAL_SINGLEGAIN_SHIFT                          8                                       /**< Shift value for ADC_SINGLEGAIN */
#define _ADC_CAL_SINGLEGAIN_MASK                           0x7F00UL                                /**< Bit mask for ADC_SINGLEGAIN */
#define _ADC_CAL_SINGLEGAIN_DEFAULT                        0x00000040UL                            /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SINGLEGAIN_DEFAULT                         (_ADC_CAL_SINGLEGAIN_DEFAULT << 8)      /**< Shifted mode DEFAULT for ADC_CAL */
#define ADC_CAL_OFFSETINVMODE                              (0x1UL << 15)                           /**< Negative single-ended offset calibration is enabled */
#define _ADC_CAL_OFFSETINVMODE_SHIFT                       15                                      /**< Shift value for ADC_OFFSETINVMODE */
#define _ADC_CAL_OFFSETINVMODE_MASK                        0x8000UL                                /**< Bit mask for ADC_OFFSETINVMODE */
#define _ADC_CAL_OFFSETINVMODE_DEFAULT                     0x00000000UL                            /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_OFFSETINVMODE_DEFAULT                      (_ADC_CAL_OFFSETINVMODE_DEFAULT << 15)  /**< Shifted mode DEFAULT for ADC_CAL */
#define _ADC_CAL_SCANOFFSET_SHIFT                          16                                      /**< Shift value for ADC_SCANOFFSET */
#define _ADC_CAL_SCANOFFSET_MASK                           0xF0000UL                               /**< Bit mask for ADC_SCANOFFSET */
#define _ADC_CAL_SCANOFFSET_DEFAULT                        0x00000008UL                            /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SCANOFFSET_DEFAULT                         (_ADC_CAL_SCANOFFSET_DEFAULT << 16)     /**< Shifted mode DEFAULT for ADC_CAL */
#define _ADC_CAL_SCANOFFSETINV_SHIFT                       20                                      /**< Shift value for ADC_SCANOFFSETINV */
#define _ADC_CAL_SCANOFFSETINV_MASK                        0xF00000UL                              /**< Bit mask for ADC_SCANOFFSETINV */
#define _ADC_CAL_SCANOFFSETINV_DEFAULT                     0x00000007UL                            /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SCANOFFSETINV_DEFAULT                      (_ADC_CAL_SCANOFFSETINV_DEFAULT << 20)  /**< Shifted mode DEFAULT for ADC_CAL */
#define _ADC_CAL_SCANGAIN_SHIFT                            24                                      /**< Shift value for ADC_SCANGAIN */
#define _ADC_CAL_SCANGAIN_MASK                             0x7F000000UL                            /**< Bit mask for ADC_SCANGAIN */
#define _ADC_CAL_SCANGAIN_DEFAULT                          0x00000040UL                            /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_SCANGAIN_DEFAULT                           (_ADC_CAL_SCANGAIN_DEFAULT << 24)       /**< Shifted mode DEFAULT for ADC_CAL */
#define ADC_CAL_CALEN                                      (0x1UL << 31)                           /**< Calibration mode is enabled */
#define _ADC_CAL_CALEN_SHIFT                               31                                      /**< Shift value for ADC_CALEN */
#define _ADC_CAL_CALEN_MASK                                0x80000000UL                            /**< Bit mask for ADC_CALEN */
#define _ADC_CAL_CALEN_DEFAULT                             0x00000000UL                            /**< Mode DEFAULT for ADC_CAL */
#define ADC_CAL_CALEN_DEFAULT                              (_ADC_CAL_CALEN_DEFAULT << 31)          /**< Shifted mode DEFAULT for ADC_CAL */

/* Bit fields for ADC IF */
#define _ADC_IF_RESETVALUE                                 0x00000000UL                      /**< Default value for ADC_IF */
#define _ADC_IF_MASK                                       0x03030F03UL                      /**< Mask for ADC_IF */
#define ADC_IF_SINGLE                                      (0x1UL << 0)                      /**< Single Conversion Complete Interrupt Flag */
#define _ADC_IF_SINGLE_SHIFT                               0                                 /**< Shift value for ADC_SINGLE */
#define _ADC_IF_SINGLE_MASK                                0x1UL                             /**< Bit mask for ADC_SINGLE */
#define _ADC_IF_SINGLE_DEFAULT                             0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLE_DEFAULT                              (_ADC_IF_SINGLE_DEFAULT << 0)     /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SCAN                                        (0x1UL << 1)                      /**< Scan Conversion Complete Interrupt Flag */
#define _ADC_IF_SCAN_SHIFT                                 1                                 /**< Shift value for ADC_SCAN */
#define _ADC_IF_SCAN_MASK                                  0x2UL                             /**< Bit mask for ADC_SCAN */
#define _ADC_IF_SCAN_DEFAULT                               0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SCAN_DEFAULT                                (_ADC_IF_SCAN_DEFAULT << 1)       /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLEOF                                    (0x1UL << 8)                      /**< Single FIFO Overflow Interrupt Flag */
#define _ADC_IF_SINGLEOF_SHIFT                             8                                 /**< Shift value for ADC_SINGLEOF */
#define _ADC_IF_SINGLEOF_MASK                              0x100UL                           /**< Bit mask for ADC_SINGLEOF */
#define _ADC_IF_SINGLEOF_DEFAULT                           0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLEOF_DEFAULT                            (_ADC_IF_SINGLEOF_DEFAULT << 8)   /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SCANOF                                      (0x1UL << 9)                      /**< Scan FIFO Overflow Interrupt Flag */
#define _ADC_IF_SCANOF_SHIFT                               9                                 /**< Shift value for ADC_SCANOF */
#define _ADC_IF_SCANOF_MASK                                0x200UL                           /**< Bit mask for ADC_SCANOF */
#define _ADC_IF_SCANOF_DEFAULT                             0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SCANOF_DEFAULT                              (_ADC_IF_SCANOF_DEFAULT << 9)     /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLEUF                                    (0x1UL << 10)                     /**< Single FIFO Underflow Interrupt Flag */
#define _ADC_IF_SINGLEUF_SHIFT                             10                                /**< Shift value for ADC_SINGLEUF */
#define _ADC_IF_SINGLEUF_MASK                              0x400UL                           /**< Bit mask for ADC_SINGLEUF */
#define _ADC_IF_SINGLEUF_DEFAULT                           0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLEUF_DEFAULT                            (_ADC_IF_SINGLEUF_DEFAULT << 10)  /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SCANUF                                      (0x1UL << 11)                     /**< Scan FIFO Underflow Interrupt Flag */
#define _ADC_IF_SCANUF_SHIFT                               11                                /**< Shift value for ADC_SCANUF */
#define _ADC_IF_SCANUF_MASK                                0x800UL                           /**< Bit mask for ADC_SCANUF */
#define _ADC_IF_SCANUF_DEFAULT                             0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SCANUF_DEFAULT                              (_ADC_IF_SCANUF_DEFAULT << 11)    /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLECMP                                   (0x1UL << 16)                     /**< Single Result Compare Match Interrupt Flag */
#define _ADC_IF_SINGLECMP_SHIFT                            16                                /**< Shift value for ADC_SINGLECMP */
#define _ADC_IF_SINGLECMP_MASK                             0x10000UL                         /**< Bit mask for ADC_SINGLECMP */
#define _ADC_IF_SINGLECMP_DEFAULT                          0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SINGLECMP_DEFAULT                           (_ADC_IF_SINGLECMP_DEFAULT << 16) /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_SCANCMP                                     (0x1UL << 17)                     /**< Scan Result Compare Match Interrupt Flag */
#define _ADC_IF_SCANCMP_SHIFT                              17                                /**< Shift value for ADC_SCANCMP */
#define _ADC_IF_SCANCMP_MASK                               0x20000UL                         /**< Bit mask for ADC_SCANCMP */
#define _ADC_IF_SCANCMP_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_SCANCMP_DEFAULT                             (_ADC_IF_SCANCMP_DEFAULT << 17)   /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_VREFOV                                      (0x1UL << 24)                     /**< VREF Over Voltage Interrupt Flag */
#define _ADC_IF_VREFOV_SHIFT                               24                                /**< Shift value for ADC_VREFOV */
#define _ADC_IF_VREFOV_MASK                                0x1000000UL                       /**< Bit mask for ADC_VREFOV */
#define _ADC_IF_VREFOV_DEFAULT                             0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_VREFOV_DEFAULT                              (_ADC_IF_VREFOV_DEFAULT << 24)    /**< Shifted mode DEFAULT for ADC_IF */
#define ADC_IF_PROGERR                                     (0x1UL << 25)                     /**< Programming Error Interrupt Flag */
#define _ADC_IF_PROGERR_SHIFT                              25                                /**< Shift value for ADC_PROGERR */
#define _ADC_IF_PROGERR_MASK                               0x2000000UL                       /**< Bit mask for ADC_PROGERR */
#define _ADC_IF_PROGERR_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for ADC_IF */
#define ADC_IF_PROGERR_DEFAULT                             (_ADC_IF_PROGERR_DEFAULT << 25)   /**< Shifted mode DEFAULT for ADC_IF */

/* Bit fields for ADC IFS */
#define _ADC_IFS_RESETVALUE                                0x00000000UL                       /**< Default value for ADC_IFS */
#define _ADC_IFS_MASK                                      0x03030F00UL                       /**< Mask for ADC_IFS */
#define ADC_IFS_SINGLEOF                                   (0x1UL << 8)                       /**< Set SINGLEOF Interrupt Flag */
#define _ADC_IFS_SINGLEOF_SHIFT                            8                                  /**< Shift value for ADC_SINGLEOF */
#define _ADC_IFS_SINGLEOF_MASK                             0x100UL                            /**< Bit mask for ADC_SINGLEOF */
#define _ADC_IFS_SINGLEOF_DEFAULT                          0x00000000UL                       /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SINGLEOF_DEFAULT                           (_ADC_IFS_SINGLEOF_DEFAULT << 8)   /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCANOF                                     (0x1UL << 9)                       /**< Set SCANOF Interrupt Flag */
#define _ADC_IFS_SCANOF_SHIFT                              9                                  /**< Shift value for ADC_SCANOF */
#define _ADC_IFS_SCANOF_MASK                               0x200UL                            /**< Bit mask for ADC_SCANOF */
#define _ADC_IFS_SCANOF_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCANOF_DEFAULT                             (_ADC_IFS_SCANOF_DEFAULT << 9)     /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_SINGLEUF                                   (0x1UL << 10)                      /**< Set SINGLEUF Interrupt Flag */
#define _ADC_IFS_SINGLEUF_SHIFT                            10                                 /**< Shift value for ADC_SINGLEUF */
#define _ADC_IFS_SINGLEUF_MASK                             0x400UL                            /**< Bit mask for ADC_SINGLEUF */
#define _ADC_IFS_SINGLEUF_DEFAULT                          0x00000000UL                       /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SINGLEUF_DEFAULT                           (_ADC_IFS_SINGLEUF_DEFAULT << 10)  /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCANUF                                     (0x1UL << 11)                      /**< Set SCANUF Interrupt Flag */
#define _ADC_IFS_SCANUF_SHIFT                              11                                 /**< Shift value for ADC_SCANUF */
#define _ADC_IFS_SCANUF_MASK                               0x800UL                            /**< Bit mask for ADC_SCANUF */
#define _ADC_IFS_SCANUF_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCANUF_DEFAULT                             (_ADC_IFS_SCANUF_DEFAULT << 11)    /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_SINGLECMP                                  (0x1UL << 16)                      /**< Set SINGLECMP Interrupt Flag */
#define _ADC_IFS_SINGLECMP_SHIFT                           16                                 /**< Shift value for ADC_SINGLECMP */
#define _ADC_IFS_SINGLECMP_MASK                            0x10000UL                          /**< Bit mask for ADC_SINGLECMP */
#define _ADC_IFS_SINGLECMP_DEFAULT                         0x00000000UL                       /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SINGLECMP_DEFAULT                          (_ADC_IFS_SINGLECMP_DEFAULT << 16) /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCANCMP                                    (0x1UL << 17)                      /**< Set SCANCMP Interrupt Flag */
#define _ADC_IFS_SCANCMP_SHIFT                             17                                 /**< Shift value for ADC_SCANCMP */
#define _ADC_IFS_SCANCMP_MASK                              0x20000UL                          /**< Bit mask for ADC_SCANCMP */
#define _ADC_IFS_SCANCMP_DEFAULT                           0x00000000UL                       /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_SCANCMP_DEFAULT                            (_ADC_IFS_SCANCMP_DEFAULT << 17)   /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_VREFOV                                     (0x1UL << 24)                      /**< Set VREFOV Interrupt Flag */
#define _ADC_IFS_VREFOV_SHIFT                              24                                 /**< Shift value for ADC_VREFOV */
#define _ADC_IFS_VREFOV_MASK                               0x1000000UL                        /**< Bit mask for ADC_VREFOV */
#define _ADC_IFS_VREFOV_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_VREFOV_DEFAULT                             (_ADC_IFS_VREFOV_DEFAULT << 24)    /**< Shifted mode DEFAULT for ADC_IFS */
#define ADC_IFS_PROGERR                                    (0x1UL << 25)                      /**< Set PROGERR Interrupt Flag */
#define _ADC_IFS_PROGERR_SHIFT                             25                                 /**< Shift value for ADC_PROGERR */
#define _ADC_IFS_PROGERR_MASK                              0x2000000UL                        /**< Bit mask for ADC_PROGERR */
#define _ADC_IFS_PROGERR_DEFAULT                           0x00000000UL                       /**< Mode DEFAULT for ADC_IFS */
#define ADC_IFS_PROGERR_DEFAULT                            (_ADC_IFS_PROGERR_DEFAULT << 25)   /**< Shifted mode DEFAULT for ADC_IFS */

/* Bit fields for ADC IFC */
#define _ADC_IFC_RESETVALUE                                0x00000000UL                       /**< Default value for ADC_IFC */
#define _ADC_IFC_MASK                                      0x03030F00UL                       /**< Mask for ADC_IFC */
#define ADC_IFC_SINGLEOF                                   (0x1UL << 8)                       /**< Clear SINGLEOF Interrupt Flag */
#define _ADC_IFC_SINGLEOF_SHIFT                            8                                  /**< Shift value for ADC_SINGLEOF */
#define _ADC_IFC_SINGLEOF_MASK                             0x100UL                            /**< Bit mask for ADC_SINGLEOF */
#define _ADC_IFC_SINGLEOF_DEFAULT                          0x00000000UL                       /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SINGLEOF_DEFAULT                           (_ADC_IFC_SINGLEOF_DEFAULT << 8)   /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCANOF                                     (0x1UL << 9)                       /**< Clear SCANOF Interrupt Flag */
#define _ADC_IFC_SCANOF_SHIFT                              9                                  /**< Shift value for ADC_SCANOF */
#define _ADC_IFC_SCANOF_MASK                               0x200UL                            /**< Bit mask for ADC_SCANOF */
#define _ADC_IFC_SCANOF_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCANOF_DEFAULT                             (_ADC_IFC_SCANOF_DEFAULT << 9)     /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_SINGLEUF                                   (0x1UL << 10)                      /**< Clear SINGLEUF Interrupt Flag */
#define _ADC_IFC_SINGLEUF_SHIFT                            10                                 /**< Shift value for ADC_SINGLEUF */
#define _ADC_IFC_SINGLEUF_MASK                             0x400UL                            /**< Bit mask for ADC_SINGLEUF */
#define _ADC_IFC_SINGLEUF_DEFAULT                          0x00000000UL                       /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SINGLEUF_DEFAULT                           (_ADC_IFC_SINGLEUF_DEFAULT << 10)  /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCANUF                                     (0x1UL << 11)                      /**< Clear SCANUF Interrupt Flag */
#define _ADC_IFC_SCANUF_SHIFT                              11                                 /**< Shift value for ADC_SCANUF */
#define _ADC_IFC_SCANUF_MASK                               0x800UL                            /**< Bit mask for ADC_SCANUF */
#define _ADC_IFC_SCANUF_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCANUF_DEFAULT                             (_ADC_IFC_SCANUF_DEFAULT << 11)    /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_SINGLECMP                                  (0x1UL << 16)                      /**< Clear SINGLECMP Interrupt Flag */
#define _ADC_IFC_SINGLECMP_SHIFT                           16                                 /**< Shift value for ADC_SINGLECMP */
#define _ADC_IFC_SINGLECMP_MASK                            0x10000UL                          /**< Bit mask for ADC_SINGLECMP */
#define _ADC_IFC_SINGLECMP_DEFAULT                         0x00000000UL                       /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SINGLECMP_DEFAULT                          (_ADC_IFC_SINGLECMP_DEFAULT << 16) /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCANCMP                                    (0x1UL << 17)                      /**< Clear SCANCMP Interrupt Flag */
#define _ADC_IFC_SCANCMP_SHIFT                             17                                 /**< Shift value for ADC_SCANCMP */
#define _ADC_IFC_SCANCMP_MASK                              0x20000UL                          /**< Bit mask for ADC_SCANCMP */
#define _ADC_IFC_SCANCMP_DEFAULT                           0x00000000UL                       /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_SCANCMP_DEFAULT                            (_ADC_IFC_SCANCMP_DEFAULT << 17)   /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_VREFOV                                     (0x1UL << 24)                      /**< Clear VREFOV Interrupt Flag */
#define _ADC_IFC_VREFOV_SHIFT                              24                                 /**< Shift value for ADC_VREFOV */
#define _ADC_IFC_VREFOV_MASK                               0x1000000UL                        /**< Bit mask for ADC_VREFOV */
#define _ADC_IFC_VREFOV_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_VREFOV_DEFAULT                             (_ADC_IFC_VREFOV_DEFAULT << 24)    /**< Shifted mode DEFAULT for ADC_IFC */
#define ADC_IFC_PROGERR                                    (0x1UL << 25)                      /**< Clear PROGERR Interrupt Flag */
#define _ADC_IFC_PROGERR_SHIFT                             25                                 /**< Shift value for ADC_PROGERR */
#define _ADC_IFC_PROGERR_MASK                              0x2000000UL                        /**< Bit mask for ADC_PROGERR */
#define _ADC_IFC_PROGERR_DEFAULT                           0x00000000UL                       /**< Mode DEFAULT for ADC_IFC */
#define ADC_IFC_PROGERR_DEFAULT                            (_ADC_IFC_PROGERR_DEFAULT << 25)   /**< Shifted mode DEFAULT for ADC_IFC */

/* Bit fields for ADC IEN */
#define _ADC_IEN_RESETVALUE                                0x00000000UL                       /**< Default value for ADC_IEN */
#define _ADC_IEN_MASK                                      0x03030F03UL                       /**< Mask for ADC_IEN */
#define ADC_IEN_SINGLE                                     (0x1UL << 0)                       /**< SINGLE Interrupt Enable */
#define _ADC_IEN_SINGLE_SHIFT                              0                                  /**< Shift value for ADC_SINGLE */
#define _ADC_IEN_SINGLE_MASK                               0x1UL                              /**< Bit mask for ADC_SINGLE */
#define _ADC_IEN_SINGLE_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLE_DEFAULT                             (_ADC_IEN_SINGLE_DEFAULT << 0)     /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCAN                                       (0x1UL << 1)                       /**< SCAN Interrupt Enable */
#define _ADC_IEN_SCAN_SHIFT                                1                                  /**< Shift value for ADC_SCAN */
#define _ADC_IEN_SCAN_MASK                                 0x2UL                              /**< Bit mask for ADC_SCAN */
#define _ADC_IEN_SCAN_DEFAULT                              0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCAN_DEFAULT                               (_ADC_IEN_SCAN_DEFAULT << 1)       /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLEOF                                   (0x1UL << 8)                       /**< SINGLEOF Interrupt Enable */
#define _ADC_IEN_SINGLEOF_SHIFT                            8                                  /**< Shift value for ADC_SINGLEOF */
#define _ADC_IEN_SINGLEOF_MASK                             0x100UL                            /**< Bit mask for ADC_SINGLEOF */
#define _ADC_IEN_SINGLEOF_DEFAULT                          0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLEOF_DEFAULT                           (_ADC_IEN_SINGLEOF_DEFAULT << 8)   /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCANOF                                     (0x1UL << 9)                       /**< SCANOF Interrupt Enable */
#define _ADC_IEN_SCANOF_SHIFT                              9                                  /**< Shift value for ADC_SCANOF */
#define _ADC_IEN_SCANOF_MASK                               0x200UL                            /**< Bit mask for ADC_SCANOF */
#define _ADC_IEN_SCANOF_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCANOF_DEFAULT                             (_ADC_IEN_SCANOF_DEFAULT << 9)     /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLEUF                                   (0x1UL << 10)                      /**< SINGLEUF Interrupt Enable */
#define _ADC_IEN_SINGLEUF_SHIFT                            10                                 /**< Shift value for ADC_SINGLEUF */
#define _ADC_IEN_SINGLEUF_MASK                             0x400UL                            /**< Bit mask for ADC_SINGLEUF */
#define _ADC_IEN_SINGLEUF_DEFAULT                          0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLEUF_DEFAULT                           (_ADC_IEN_SINGLEUF_DEFAULT << 10)  /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCANUF                                     (0x1UL << 11)                      /**< SCANUF Interrupt Enable */
#define _ADC_IEN_SCANUF_SHIFT                              11                                 /**< Shift value for ADC_SCANUF */
#define _ADC_IEN_SCANUF_MASK                               0x800UL                            /**< Bit mask for ADC_SCANUF */
#define _ADC_IEN_SCANUF_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCANUF_DEFAULT                             (_ADC_IEN_SCANUF_DEFAULT << 11)    /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLECMP                                  (0x1UL << 16)                      /**< SINGLECMP Interrupt Enable */
#define _ADC_IEN_SINGLECMP_SHIFT                           16                                 /**< Shift value for ADC_SINGLECMP */
#define _ADC_IEN_SINGLECMP_MASK                            0x10000UL                          /**< Bit mask for ADC_SINGLECMP */
#define _ADC_IEN_SINGLECMP_DEFAULT                         0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SINGLECMP_DEFAULT                          (_ADC_IEN_SINGLECMP_DEFAULT << 16) /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCANCMP                                    (0x1UL << 17)                      /**< SCANCMP Interrupt Enable */
#define _ADC_IEN_SCANCMP_SHIFT                             17                                 /**< Shift value for ADC_SCANCMP */
#define _ADC_IEN_SCANCMP_MASK                              0x20000UL                          /**< Bit mask for ADC_SCANCMP */
#define _ADC_IEN_SCANCMP_DEFAULT                           0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_SCANCMP_DEFAULT                            (_ADC_IEN_SCANCMP_DEFAULT << 17)   /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_VREFOV                                     (0x1UL << 24)                      /**< VREFOV Interrupt Enable */
#define _ADC_IEN_VREFOV_SHIFT                              24                                 /**< Shift value for ADC_VREFOV */
#define _ADC_IEN_VREFOV_MASK                               0x1000000UL                        /**< Bit mask for ADC_VREFOV */
#define _ADC_IEN_VREFOV_DEFAULT                            0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_VREFOV_DEFAULT                             (_ADC_IEN_VREFOV_DEFAULT << 24)    /**< Shifted mode DEFAULT for ADC_IEN */
#define ADC_IEN_PROGERR                                    (0x1UL << 25)                      /**< PROGERR Interrupt Enable */
#define _ADC_IEN_PROGERR_SHIFT                             25                                 /**< Shift value for ADC_PROGERR */
#define _ADC_IEN_PROGERR_MASK                              0x2000000UL                        /**< Bit mask for ADC_PROGERR */
#define _ADC_IEN_PROGERR_DEFAULT                           0x00000000UL                       /**< Mode DEFAULT for ADC_IEN */
#define ADC_IEN_PROGERR_DEFAULT                            (_ADC_IEN_PROGERR_DEFAULT << 25)   /**< Shifted mode DEFAULT for ADC_IEN */

/* Bit fields for ADC SINGLEDATA */
#define _ADC_SINGLEDATA_RESETVALUE                         0x00000000UL                        /**< Default value for ADC_SINGLEDATA */
#define _ADC_SINGLEDATA_MASK                               0xFFFFFFFFUL                        /**< Mask for ADC_SINGLEDATA */
#define _ADC_SINGLEDATA_DATA_SHIFT                         0                                   /**< Shift value for ADC_DATA */
#define _ADC_SINGLEDATA_DATA_MASK                          0xFFFFFFFFUL                        /**< Bit mask for ADC_DATA */
#define _ADC_SINGLEDATA_DATA_DEFAULT                       0x00000000UL                        /**< Mode DEFAULT for ADC_SINGLEDATA */
#define ADC_SINGLEDATA_DATA_DEFAULT                        (_ADC_SINGLEDATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SINGLEDATA */

/* Bit fields for ADC SCANDATA */
#define _ADC_SCANDATA_RESETVALUE                           0x00000000UL                      /**< Default value for ADC_SCANDATA */
#define _ADC_SCANDATA_MASK                                 0xFFFFFFFFUL                      /**< Mask for ADC_SCANDATA */
#define _ADC_SCANDATA_DATA_SHIFT                           0                                 /**< Shift value for ADC_DATA */
#define _ADC_SCANDATA_DATA_MASK                            0xFFFFFFFFUL                      /**< Bit mask for ADC_DATA */
#define _ADC_SCANDATA_DATA_DEFAULT                         0x00000000UL                      /**< Mode DEFAULT for ADC_SCANDATA */
#define ADC_SCANDATA_DATA_DEFAULT                          (_ADC_SCANDATA_DATA_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SCANDATA */

/* Bit fields for ADC SINGLEDATAP */
#define _ADC_SINGLEDATAP_RESETVALUE                        0x00000000UL                          /**< Default value for ADC_SINGLEDATAP */
#define _ADC_SINGLEDATAP_MASK                              0xFFFFFFFFUL                          /**< Mask for ADC_SINGLEDATAP */
#define _ADC_SINGLEDATAP_DATAP_SHIFT                       0                                     /**< Shift value for ADC_DATAP */
#define _ADC_SINGLEDATAP_DATAP_MASK                        0xFFFFFFFFUL                          /**< Bit mask for ADC_DATAP */
#define _ADC_SINGLEDATAP_DATAP_DEFAULT                     0x00000000UL                          /**< Mode DEFAULT for ADC_SINGLEDATAP */
#define ADC_SINGLEDATAP_DATAP_DEFAULT                      (_ADC_SINGLEDATAP_DATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SINGLEDATAP */

/* Bit fields for ADC SCANDATAP */
#define _ADC_SCANDATAP_RESETVALUE                          0x00000000UL                        /**< Default value for ADC_SCANDATAP */
#define _ADC_SCANDATAP_MASK                                0xFFFFFFFFUL                        /**< Mask for ADC_SCANDATAP */
#define _ADC_SCANDATAP_DATAP_SHIFT                         0                                   /**< Shift value for ADC_DATAP */
#define _ADC_SCANDATAP_DATAP_MASK                          0xFFFFFFFFUL                        /**< Bit mask for ADC_DATAP */
#define _ADC_SCANDATAP_DATAP_DEFAULT                       0x00000000UL                        /**< Mode DEFAULT for ADC_SCANDATAP */
#define ADC_SCANDATAP_DATAP_DEFAULT                        (_ADC_SCANDATAP_DATAP_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SCANDATAP */

/* Bit fields for ADC SCANDATAX */
#define _ADC_SCANDATAX_RESETVALUE                          0x00000000UL                               /**< Default value for ADC_SCANDATAX */
#define _ADC_SCANDATAX_MASK                                0x001FFFFFUL                               /**< Mask for ADC_SCANDATAX */
#define _ADC_SCANDATAX_DATA_SHIFT                          0                                          /**< Shift value for ADC_DATA */
#define _ADC_SCANDATAX_DATA_MASK                           0xFFFFUL                                   /**< Bit mask for ADC_DATA */
#define _ADC_SCANDATAX_DATA_DEFAULT                        0x00000000UL                               /**< Mode DEFAULT for ADC_SCANDATAX */
#define ADC_SCANDATAX_DATA_DEFAULT                         (_ADC_SCANDATAX_DATA_DEFAULT << 0)         /**< Shifted mode DEFAULT for ADC_SCANDATAX */
#define _ADC_SCANDATAX_SCANINPUTID_SHIFT                   16                                         /**< Shift value for ADC_SCANINPUTID */
#define _ADC_SCANDATAX_SCANINPUTID_MASK                    0x1F0000UL                                 /**< Bit mask for ADC_SCANINPUTID */
#define _ADC_SCANDATAX_SCANINPUTID_DEFAULT                 0x00000000UL                               /**< Mode DEFAULT for ADC_SCANDATAX */
#define ADC_SCANDATAX_SCANINPUTID_DEFAULT                  (_ADC_SCANDATAX_SCANINPUTID_DEFAULT << 16) /**< Shifted mode DEFAULT for ADC_SCANDATAX */

/* Bit fields for ADC SCANDATAXP */
#define _ADC_SCANDATAXP_RESETVALUE                         0x00000000UL                                    /**< Default value for ADC_SCANDATAXP */
#define _ADC_SCANDATAXP_MASK                               0x001FFFFFUL                                    /**< Mask for ADC_SCANDATAXP */
#define _ADC_SCANDATAXP_DATAP_SHIFT                        0                                               /**< Shift value for ADC_DATAP */
#define _ADC_SCANDATAXP_DATAP_MASK                         0xFFFFUL                                        /**< Bit mask for ADC_DATAP */
#define _ADC_SCANDATAXP_DATAP_DEFAULT                      0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANDATAXP */
#define ADC_SCANDATAXP_DATAP_DEFAULT                       (_ADC_SCANDATAXP_DATAP_DEFAULT << 0)            /**< Shifted mode DEFAULT for ADC_SCANDATAXP */
#define _ADC_SCANDATAXP_SCANINPUTIDPEEK_SHIFT              16                                              /**< Shift value for ADC_SCANINPUTIDPEEK */
#define _ADC_SCANDATAXP_SCANINPUTIDPEEK_MASK               0x1F0000UL                                      /**< Bit mask for ADC_SCANINPUTIDPEEK */
#define _ADC_SCANDATAXP_SCANINPUTIDPEEK_DEFAULT            0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANDATAXP */
#define ADC_SCANDATAXP_SCANINPUTIDPEEK_DEFAULT             (_ADC_SCANDATAXP_SCANINPUTIDPEEK_DEFAULT << 16) /**< Shifted mode DEFAULT for ADC_SCANDATAXP */

/* Bit fields for ADC APORTREQ */
#define _ADC_APORTREQ_RESETVALUE                           0x00000000UL                            /**< Default value for ADC_APORTREQ */
#define _ADC_APORTREQ_MASK                                 0x000003FFUL                            /**< Mask for ADC_APORTREQ */
#define ADC_APORTREQ_APORT0XREQ                            (0x1UL << 0)                            /**< 1 if the bus connected to APORT0X is requested */
#define _ADC_APORTREQ_APORT0XREQ_SHIFT                     0                                       /**< Shift value for ADC_APORT0XREQ */
#define _ADC_APORTREQ_APORT0XREQ_MASK                      0x1UL                                   /**< Bit mask for ADC_APORT0XREQ */
#define _ADC_APORTREQ_APORT0XREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT0XREQ_DEFAULT                    (_ADC_APORTREQ_APORT0XREQ_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT0YREQ                            (0x1UL << 1)                            /**< 1 if the bus connected to APORT0Y is requested */
#define _ADC_APORTREQ_APORT0YREQ_SHIFT                     1                                       /**< Shift value for ADC_APORT0YREQ */
#define _ADC_APORTREQ_APORT0YREQ_MASK                      0x2UL                                   /**< Bit mask for ADC_APORT0YREQ */
#define _ADC_APORTREQ_APORT0YREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT0YREQ_DEFAULT                    (_ADC_APORTREQ_APORT0YREQ_DEFAULT << 1) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT1XREQ                            (0x1UL << 2)                            /**< 1 if the bus connected to APORT1X is requested */
#define _ADC_APORTREQ_APORT1XREQ_SHIFT                     2                                       /**< Shift value for ADC_APORT1XREQ */
#define _ADC_APORTREQ_APORT1XREQ_MASK                      0x4UL                                   /**< Bit mask for ADC_APORT1XREQ */
#define _ADC_APORTREQ_APORT1XREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT1XREQ_DEFAULT                    (_ADC_APORTREQ_APORT1XREQ_DEFAULT << 2) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT1YREQ                            (0x1UL << 3)                            /**< 1 if the bus connected to APORT1Y is requested */
#define _ADC_APORTREQ_APORT1YREQ_SHIFT                     3                                       /**< Shift value for ADC_APORT1YREQ */
#define _ADC_APORTREQ_APORT1YREQ_MASK                      0x8UL                                   /**< Bit mask for ADC_APORT1YREQ */
#define _ADC_APORTREQ_APORT1YREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT1YREQ_DEFAULT                    (_ADC_APORTREQ_APORT1YREQ_DEFAULT << 3) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT2XREQ                            (0x1UL << 4)                            /**< 1 if the bus connected to APORT2X is requested */
#define _ADC_APORTREQ_APORT2XREQ_SHIFT                     4                                       /**< Shift value for ADC_APORT2XREQ */
#define _ADC_APORTREQ_APORT2XREQ_MASK                      0x10UL                                  /**< Bit mask for ADC_APORT2XREQ */
#define _ADC_APORTREQ_APORT2XREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT2XREQ_DEFAULT                    (_ADC_APORTREQ_APORT2XREQ_DEFAULT << 4) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT2YREQ                            (0x1UL << 5)                            /**< 1 if the bus connected to APORT2Y is requested */
#define _ADC_APORTREQ_APORT2YREQ_SHIFT                     5                                       /**< Shift value for ADC_APORT2YREQ */
#define _ADC_APORTREQ_APORT2YREQ_MASK                      0x20UL                                  /**< Bit mask for ADC_APORT2YREQ */
#define _ADC_APORTREQ_APORT2YREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT2YREQ_DEFAULT                    (_ADC_APORTREQ_APORT2YREQ_DEFAULT << 5) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT3XREQ                            (0x1UL << 6)                            /**< 1 if the bus connected to APORT3X is requested */
#define _ADC_APORTREQ_APORT3XREQ_SHIFT                     6                                       /**< Shift value for ADC_APORT3XREQ */
#define _ADC_APORTREQ_APORT3XREQ_MASK                      0x40UL                                  /**< Bit mask for ADC_APORT3XREQ */
#define _ADC_APORTREQ_APORT3XREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT3XREQ_DEFAULT                    (_ADC_APORTREQ_APORT3XREQ_DEFAULT << 6) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT3YREQ                            (0x1UL << 7)                            /**< 1 if the bus connected to APORT3Y is requested */
#define _ADC_APORTREQ_APORT3YREQ_SHIFT                     7                                       /**< Shift value for ADC_APORT3YREQ */
#define _ADC_APORTREQ_APORT3YREQ_MASK                      0x80UL                                  /**< Bit mask for ADC_APORT3YREQ */
#define _ADC_APORTREQ_APORT3YREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT3YREQ_DEFAULT                    (_ADC_APORTREQ_APORT3YREQ_DEFAULT << 7) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT4XREQ                            (0x1UL << 8)                            /**< 1 if the bus connected to APORT4X is requested */
#define _ADC_APORTREQ_APORT4XREQ_SHIFT                     8                                       /**< Shift value for ADC_APORT4XREQ */
#define _ADC_APORTREQ_APORT4XREQ_MASK                      0x100UL                                 /**< Bit mask for ADC_APORT4XREQ */
#define _ADC_APORTREQ_APORT4XREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT4XREQ_DEFAULT                    (_ADC_APORTREQ_APORT4XREQ_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT4YREQ                            (0x1UL << 9)                            /**< 1 if the bus connected to APORT4Y is requested */
#define _ADC_APORTREQ_APORT4YREQ_SHIFT                     9                                       /**< Shift value for ADC_APORT4YREQ */
#define _ADC_APORTREQ_APORT4YREQ_MASK                      0x200UL                                 /**< Bit mask for ADC_APORT4YREQ */
#define _ADC_APORTREQ_APORT4YREQ_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ADC_APORTREQ */
#define ADC_APORTREQ_APORT4YREQ_DEFAULT                    (_ADC_APORTREQ_APORT4YREQ_DEFAULT << 9) /**< Shifted mode DEFAULT for ADC_APORTREQ */

/* Bit fields for ADC APORTCONFLICT */
#define _ADC_APORTCONFLICT_RESETVALUE                      0x00000000UL                                      /**< Default value for ADC_APORTCONFLICT */
#define _ADC_APORTCONFLICT_MASK                            0x000003FFUL                                      /**< Mask for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT0XCONFLICT                  (0x1UL << 0)                                      /**< 1 if the bus connected to APORT0X is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT0XCONFLICT_SHIFT           0                                                 /**< Shift value for ADC_APORT0XCONFLICT */
#define _ADC_APORTCONFLICT_APORT0XCONFLICT_MASK            0x1UL                                             /**< Bit mask for ADC_APORT0XCONFLICT */
#define _ADC_APORTCONFLICT_APORT0XCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT0XCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT0XCONFLICT_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT0YCONFLICT                  (0x1UL << 1)                                      /**< 1 if the bus connected to APORT0Y is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT0YCONFLICT_SHIFT           1                                                 /**< Shift value for ADC_APORT0YCONFLICT */
#define _ADC_APORTCONFLICT_APORT0YCONFLICT_MASK            0x2UL                                             /**< Bit mask for ADC_APORT0YCONFLICT */
#define _ADC_APORTCONFLICT_APORT0YCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT0YCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT0YCONFLICT_DEFAULT << 1) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT1XCONFLICT                  (0x1UL << 2)                                      /**< 1 if the bus connected to APORT1X is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT1XCONFLICT_SHIFT           2                                                 /**< Shift value for ADC_APORT1XCONFLICT */
#define _ADC_APORTCONFLICT_APORT1XCONFLICT_MASK            0x4UL                                             /**< Bit mask for ADC_APORT1XCONFLICT */
#define _ADC_APORTCONFLICT_APORT1XCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT1XCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT1XCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT1YCONFLICT                  (0x1UL << 3)                                      /**< 1 if the bus connected to APORT1Y is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT1YCONFLICT_SHIFT           3                                                 /**< Shift value for ADC_APORT1YCONFLICT */
#define _ADC_APORTCONFLICT_APORT1YCONFLICT_MASK            0x8UL                                             /**< Bit mask for ADC_APORT1YCONFLICT */
#define _ADC_APORTCONFLICT_APORT1YCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT1YCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT1YCONFLICT_DEFAULT << 3) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT2XCONFLICT                  (0x1UL << 4)                                      /**< 1 if the bus connected to APORT2X is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT2XCONFLICT_SHIFT           4                                                 /**< Shift value for ADC_APORT2XCONFLICT */
#define _ADC_APORTCONFLICT_APORT2XCONFLICT_MASK            0x10UL                                            /**< Bit mask for ADC_APORT2XCONFLICT */
#define _ADC_APORTCONFLICT_APORT2XCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT2XCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT2XCONFLICT_DEFAULT << 4) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT2YCONFLICT                  (0x1UL << 5)                                      /**< 1 if the bus connected to APORT2Y is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT2YCONFLICT_SHIFT           5                                                 /**< Shift value for ADC_APORT2YCONFLICT */
#define _ADC_APORTCONFLICT_APORT2YCONFLICT_MASK            0x20UL                                            /**< Bit mask for ADC_APORT2YCONFLICT */
#define _ADC_APORTCONFLICT_APORT2YCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT2YCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT2YCONFLICT_DEFAULT << 5) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT3XCONFLICT                  (0x1UL << 6)                                      /**< 1 if the bus connected to APORT3X is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT3XCONFLICT_SHIFT           6                                                 /**< Shift value for ADC_APORT3XCONFLICT */
#define _ADC_APORTCONFLICT_APORT3XCONFLICT_MASK            0x40UL                                            /**< Bit mask for ADC_APORT3XCONFLICT */
#define _ADC_APORTCONFLICT_APORT3XCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT3XCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT3XCONFLICT_DEFAULT << 6) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT3YCONFLICT                  (0x1UL << 7)                                      /**< 1 if the bus connected to APORT3Y is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT3YCONFLICT_SHIFT           7                                                 /**< Shift value for ADC_APORT3YCONFLICT */
#define _ADC_APORTCONFLICT_APORT3YCONFLICT_MASK            0x80UL                                            /**< Bit mask for ADC_APORT3YCONFLICT */
#define _ADC_APORTCONFLICT_APORT3YCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT3YCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT3YCONFLICT_DEFAULT << 7) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT4XCONFLICT                  (0x1UL << 8)                                      /**< 1 if the bus connected to APORT4X is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT4XCONFLICT_SHIFT           8                                                 /**< Shift value for ADC_APORT4XCONFLICT */
#define _ADC_APORTCONFLICT_APORT4XCONFLICT_MASK            0x100UL                                           /**< Bit mask for ADC_APORT4XCONFLICT */
#define _ADC_APORTCONFLICT_APORT4XCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT4XCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT4XCONFLICT_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT4YCONFLICT                  (0x1UL << 9)                                      /**< 1 if the bus connected to APORT4Y is in conflict with another peripheral */
#define _ADC_APORTCONFLICT_APORT4YCONFLICT_SHIFT           9                                                 /**< Shift value for ADC_APORT4YCONFLICT */
#define _ADC_APORTCONFLICT_APORT4YCONFLICT_MASK            0x200UL                                           /**< Bit mask for ADC_APORT4YCONFLICT */
#define _ADC_APORTCONFLICT_APORT4YCONFLICT_DEFAULT         0x00000000UL                                      /**< Mode DEFAULT for ADC_APORTCONFLICT */
#define ADC_APORTCONFLICT_APORT4YCONFLICT_DEFAULT          (_ADC_APORTCONFLICT_APORT4YCONFLICT_DEFAULT << 9) /**< Shifted mode DEFAULT for ADC_APORTCONFLICT */

/* Bit fields for ADC SINGLEFIFOCOUNT */
#define _ADC_SINGLEFIFOCOUNT_RESETVALUE                    0x00000000UL                                 /**< Default value for ADC_SINGLEFIFOCOUNT */
#define _ADC_SINGLEFIFOCOUNT_MASK                          0x00000007UL                                 /**< Mask for ADC_SINGLEFIFOCOUNT */
#define _ADC_SINGLEFIFOCOUNT_SINGLEDC_SHIFT                0                                            /**< Shift value for ADC_SINGLEDC */
#define _ADC_SINGLEFIFOCOUNT_SINGLEDC_MASK                 0x7UL                                        /**< Bit mask for ADC_SINGLEDC */
#define _ADC_SINGLEFIFOCOUNT_SINGLEDC_DEFAULT              0x00000000UL                                 /**< Mode DEFAULT for ADC_SINGLEFIFOCOUNT */
#define ADC_SINGLEFIFOCOUNT_SINGLEDC_DEFAULT               (_ADC_SINGLEFIFOCOUNT_SINGLEDC_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SINGLEFIFOCOUNT */

/* Bit fields for ADC SCANFIFOCOUNT */
#define _ADC_SCANFIFOCOUNT_RESETVALUE                      0x00000000UL                             /**< Default value for ADC_SCANFIFOCOUNT */
#define _ADC_SCANFIFOCOUNT_MASK                            0x00000007UL                             /**< Mask for ADC_SCANFIFOCOUNT */
#define _ADC_SCANFIFOCOUNT_SCANDC_SHIFT                    0                                        /**< Shift value for ADC_SCANDC */
#define _ADC_SCANFIFOCOUNT_SCANDC_MASK                     0x7UL                                    /**< Bit mask for ADC_SCANDC */
#define _ADC_SCANFIFOCOUNT_SCANDC_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for ADC_SCANFIFOCOUNT */
#define ADC_SCANFIFOCOUNT_SCANDC_DEFAULT                   (_ADC_SCANFIFOCOUNT_SCANDC_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SCANFIFOCOUNT */

/* Bit fields for ADC SINGLEFIFOCLEAR */
#define _ADC_SINGLEFIFOCLEAR_RESETVALUE                    0x00000000UL                                        /**< Default value for ADC_SINGLEFIFOCLEAR */
#define _ADC_SINGLEFIFOCLEAR_MASK                          0x00000001UL                                        /**< Mask for ADC_SINGLEFIFOCLEAR */
#define ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR                (0x1UL << 0)                                        /**< Clear Single FIFO content */
#define _ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR_SHIFT         0                                                   /**< Shift value for ADC_SINGLEFIFOCLEAR */
#define _ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR_MASK          0x1UL                                               /**< Bit mask for ADC_SINGLEFIFOCLEAR */
#define _ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_SINGLEFIFOCLEAR */
#define ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR_DEFAULT        (_ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SINGLEFIFOCLEAR */

/* Bit fields for ADC SCANFIFOCLEAR */
#define _ADC_SCANFIFOCLEAR_RESETVALUE                      0x00000000UL                                    /**< Default value for ADC_SCANFIFOCLEAR */
#define _ADC_SCANFIFOCLEAR_MASK                            0x00000001UL                                    /**< Mask for ADC_SCANFIFOCLEAR */
#define ADC_SCANFIFOCLEAR_SCANFIFOCLEAR                    (0x1UL << 0)                                    /**< Clear Scan FIFO content */
#define _ADC_SCANFIFOCLEAR_SCANFIFOCLEAR_SHIFT             0                                               /**< Shift value for ADC_SCANFIFOCLEAR */
#define _ADC_SCANFIFOCLEAR_SCANFIFOCLEAR_MASK              0x1UL                                           /**< Bit mask for ADC_SCANFIFOCLEAR */
#define _ADC_SCANFIFOCLEAR_SCANFIFOCLEAR_DEFAULT           0x00000000UL                                    /**< Mode DEFAULT for ADC_SCANFIFOCLEAR */
#define ADC_SCANFIFOCLEAR_SCANFIFOCLEAR_DEFAULT            (_ADC_SCANFIFOCLEAR_SCANFIFOCLEAR_DEFAULT << 0) /**< Shifted mode DEFAULT for ADC_SCANFIFOCLEAR */

/* Bit fields for ADC APORTMASTERDIS */
#define _ADC_APORTMASTERDIS_RESETVALUE                     0x00000000UL                                        /**< Default value for ADC_APORTMASTERDIS */
#define _ADC_APORTMASTERDIS_MASK                           0x000003FCUL                                        /**< Mask for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT1XMASTERDIS                (0x1UL << 2)                                        /**< APORT1X Master Disable */
#define _ADC_APORTMASTERDIS_APORT1XMASTERDIS_SHIFT         2                                                   /**< Shift value for ADC_APORT1XMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT1XMASTERDIS_MASK          0x4UL                                               /**< Bit mask for ADC_APORT1XMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT1XMASTERDIS_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT1XMASTERDIS_DEFAULT        (_ADC_APORTMASTERDIS_APORT1XMASTERDIS_DEFAULT << 2) /**< Shifted mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT1YMASTERDIS                (0x1UL << 3)                                        /**< APORT1Y Master Disable */
#define _ADC_APORTMASTERDIS_APORT1YMASTERDIS_SHIFT         3                                                   /**< Shift value for ADC_APORT1YMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT1YMASTERDIS_MASK          0x8UL                                               /**< Bit mask for ADC_APORT1YMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT1YMASTERDIS_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT1YMASTERDIS_DEFAULT        (_ADC_APORTMASTERDIS_APORT1YMASTERDIS_DEFAULT << 3) /**< Shifted mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT2XMASTERDIS                (0x1UL << 4)                                        /**< APORT2X Master Disable */
#define _ADC_APORTMASTERDIS_APORT2XMASTERDIS_SHIFT         4                                                   /**< Shift value for ADC_APORT2XMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT2XMASTERDIS_MASK          0x10UL                                              /**< Bit mask for ADC_APORT2XMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT2XMASTERDIS_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT2XMASTERDIS_DEFAULT        (_ADC_APORTMASTERDIS_APORT2XMASTERDIS_DEFAULT << 4) /**< Shifted mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT2YMASTERDIS                (0x1UL << 5)                                        /**< APORT2Y Master Disable */
#define _ADC_APORTMASTERDIS_APORT2YMASTERDIS_SHIFT         5                                                   /**< Shift value for ADC_APORT2YMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT2YMASTERDIS_MASK          0x20UL                                              /**< Bit mask for ADC_APORT2YMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT2YMASTERDIS_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT2YMASTERDIS_DEFAULT        (_ADC_APORTMASTERDIS_APORT2YMASTERDIS_DEFAULT << 5) /**< Shifted mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT3XMASTERDIS                (0x1UL << 6)                                        /**< APORT3X Master Disable */
#define _ADC_APORTMASTERDIS_APORT3XMASTERDIS_SHIFT         6                                                   /**< Shift value for ADC_APORT3XMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT3XMASTERDIS_MASK          0x40UL                                              /**< Bit mask for ADC_APORT3XMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT3XMASTERDIS_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT3XMASTERDIS_DEFAULT        (_ADC_APORTMASTERDIS_APORT3XMASTERDIS_DEFAULT << 6) /**< Shifted mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT3YMASTERDIS                (0x1UL << 7)                                        /**< APORT3Y Master Disable */
#define _ADC_APORTMASTERDIS_APORT3YMASTERDIS_SHIFT         7                                                   /**< Shift value for ADC_APORT3YMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT3YMASTERDIS_MASK          0x80UL                                              /**< Bit mask for ADC_APORT3YMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT3YMASTERDIS_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT3YMASTERDIS_DEFAULT        (_ADC_APORTMASTERDIS_APORT3YMASTERDIS_DEFAULT << 7) /**< Shifted mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT4XMASTERDIS                (0x1UL << 8)                                        /**< APORT4X Master Disable */
#define _ADC_APORTMASTERDIS_APORT4XMASTERDIS_SHIFT         8                                                   /**< Shift value for ADC_APORT4XMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT4XMASTERDIS_MASK          0x100UL                                             /**< Bit mask for ADC_APORT4XMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT4XMASTERDIS_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT4XMASTERDIS_DEFAULT        (_ADC_APORTMASTERDIS_APORT4XMASTERDIS_DEFAULT << 8) /**< Shifted mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT4YMASTERDIS                (0x1UL << 9)                                        /**< APORT4Y Master Disable */
#define _ADC_APORTMASTERDIS_APORT4YMASTERDIS_SHIFT         9                                                   /**< Shift value for ADC_APORT4YMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT4YMASTERDIS_MASK          0x200UL                                             /**< Bit mask for ADC_APORT4YMASTERDIS */
#define _ADC_APORTMASTERDIS_APORT4YMASTERDIS_DEFAULT       0x00000000UL                                        /**< Mode DEFAULT for ADC_APORTMASTERDIS */
#define ADC_APORTMASTERDIS_APORT4YMASTERDIS_DEFAULT        (_ADC_APORTMASTERDIS_APORT4YMASTERDIS_DEFAULT << 9) /**< Shifted mode DEFAULT for ADC_APORTMASTERDIS */

/** @} End of group EFR32FG1P_ADC */
/** @} End of group Parts */

