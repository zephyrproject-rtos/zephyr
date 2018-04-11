/**************************************************************************//**
 * @file efm32pg12b_msc.h
 * @brief EFM32PG12B_MSC register and bit field definitions
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
 * @defgroup EFM32PG12B_MSC
 * @{
 * @brief EFM32PG12B_MSC Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;           /**< Memory System Control Register  */
  __IOM uint32_t READCTRL;       /**< Read Control Register  */
  __IOM uint32_t WRITECTRL;      /**< Write Control Register  */
  __IOM uint32_t WRITECMD;       /**< Write Command Register  */
  __IOM uint32_t ADDRB;          /**< Page Erase/Write Address Buffer  */
  uint32_t       RESERVED0[1];   /**< Reserved for future use **/
  __IOM uint32_t WDATA;          /**< Write Data Register  */
  __IM uint32_t  STATUS;         /**< Status Register  */

  uint32_t       RESERVED1[4];   /**< Reserved for future use **/
  __IM uint32_t  IF;             /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;            /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;            /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;            /**< Interrupt Enable Register  */
  __IOM uint32_t LOCK;           /**< Configuration Lock Register  */
  __IOM uint32_t CACHECMD;       /**< Flash Cache Command Register  */
  __IM uint32_t  CACHEHITS;      /**< Cache Hits Performance Counter  */
  __IM uint32_t  CACHEMISSES;    /**< Cache Misses Performance Counter  */

  uint32_t       RESERVED2[1];   /**< Reserved for future use **/
  __IOM uint32_t MASSLOCK;       /**< Mass Erase Lock Register  */

  uint32_t       RESERVED3[1];   /**< Reserved for future use **/
  __IOM uint32_t STARTUP;        /**< Startup Control  */

  uint32_t       RESERVED4[4];   /**< Reserved for future use **/
  __IOM uint32_t BANKSWITCHLOCK; /**< Bank Switching Lock Register  */
  __IOM uint32_t CMD;            /**< Command Register  */

  uint32_t       RESERVED5[6];   /**< Reserved for future use **/
  __IOM uint32_t BOOTLOADERCTRL; /**< Bootloader read and write enable, write once register  */
  __IOM uint32_t AAPUNLOCKCMD;   /**< Software Unlock AAP Command Register  */
  __IOM uint32_t CACHECONFIG0;   /**< Cache Configuration Register 0  */

  uint32_t       RESERVED6[25];  /**< Reserved for future use **/
  __IOM uint32_t RAMCTRL;        /**< RAM Control enable Register  */
} MSC_TypeDef;                   /** @} */

/**************************************************************************//**
 * @defgroup EFM32PG12B_MSC_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for MSC CTRL */
#define _MSC_CTRL_RESETVALUE                              0x00000001UL                            /**< Default value for MSC_CTRL */
#define _MSC_CTRL_MASK                                    0x0000001FUL                            /**< Mask for MSC_CTRL */
#define MSC_CTRL_ADDRFAULTEN                              (0x1UL << 0)                            /**< Invalid Address Bus Fault Response Enable */
#define _MSC_CTRL_ADDRFAULTEN_SHIFT                       0                                       /**< Shift value for MSC_ADDRFAULTEN */
#define _MSC_CTRL_ADDRFAULTEN_MASK                        0x1UL                                   /**< Bit mask for MSC_ADDRFAULTEN */
#define _MSC_CTRL_ADDRFAULTEN_DEFAULT                     0x00000001UL                            /**< Mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_ADDRFAULTEN_DEFAULT                      (_MSC_CTRL_ADDRFAULTEN_DEFAULT << 0)    /**< Shifted mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_CLKDISFAULTEN                            (0x1UL << 1)                            /**< Clock-disabled Bus Fault Response Enable */
#define _MSC_CTRL_CLKDISFAULTEN_SHIFT                     1                                       /**< Shift value for MSC_CLKDISFAULTEN */
#define _MSC_CTRL_CLKDISFAULTEN_MASK                      0x2UL                                   /**< Bit mask for MSC_CLKDISFAULTEN */
#define _MSC_CTRL_CLKDISFAULTEN_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_CLKDISFAULTEN_DEFAULT                    (_MSC_CTRL_CLKDISFAULTEN_DEFAULT << 1)  /**< Shifted mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_PWRUPONDEMAND                            (0x1UL << 2)                            /**< Power Up On Demand During Wake Up */
#define _MSC_CTRL_PWRUPONDEMAND_SHIFT                     2                                       /**< Shift value for MSC_PWRUPONDEMAND */
#define _MSC_CTRL_PWRUPONDEMAND_MASK                      0x4UL                                   /**< Bit mask for MSC_PWRUPONDEMAND */
#define _MSC_CTRL_PWRUPONDEMAND_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_PWRUPONDEMAND_DEFAULT                    (_MSC_CTRL_PWRUPONDEMAND_DEFAULT << 2)  /**< Shifted mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_IFCREADCLEAR                             (0x1UL << 3)                            /**< IFC Read Clears IF */
#define _MSC_CTRL_IFCREADCLEAR_SHIFT                      3                                       /**< Shift value for MSC_IFCREADCLEAR */
#define _MSC_CTRL_IFCREADCLEAR_MASK                       0x8UL                                   /**< Bit mask for MSC_IFCREADCLEAR */
#define _MSC_CTRL_IFCREADCLEAR_DEFAULT                    0x00000000UL                            /**< Mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_IFCREADCLEAR_DEFAULT                     (_MSC_CTRL_IFCREADCLEAR_DEFAULT << 3)   /**< Shifted mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_TIMEOUTFAULTEN                           (0x1UL << 4)                            /**< Timeout Bus Fault Response Enable */
#define _MSC_CTRL_TIMEOUTFAULTEN_SHIFT                    4                                       /**< Shift value for MSC_TIMEOUTFAULTEN */
#define _MSC_CTRL_TIMEOUTFAULTEN_MASK                     0x10UL                                  /**< Bit mask for MSC_TIMEOUTFAULTEN */
#define _MSC_CTRL_TIMEOUTFAULTEN_DEFAULT                  0x00000000UL                            /**< Mode DEFAULT for MSC_CTRL */
#define MSC_CTRL_TIMEOUTFAULTEN_DEFAULT                   (_MSC_CTRL_TIMEOUTFAULTEN_DEFAULT << 4) /**< Shifted mode DEFAULT for MSC_CTRL */

/* Bit fields for MSC READCTRL */
#define _MSC_READCTRL_RESETVALUE                          0x01000100UL                          /**< Default value for MSC_READCTRL */
#define _MSC_READCTRL_MASK                                0x13000338UL                          /**< Mask for MSC_READCTRL */
#define MSC_READCTRL_IFCDIS                               (0x1UL << 3)                          /**< Internal Flash Cache Disable */
#define _MSC_READCTRL_IFCDIS_SHIFT                        3                                     /**< Shift value for MSC_IFCDIS */
#define _MSC_READCTRL_IFCDIS_MASK                         0x8UL                                 /**< Bit mask for MSC_IFCDIS */
#define _MSC_READCTRL_IFCDIS_DEFAULT                      0x00000000UL                          /**< Mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_IFCDIS_DEFAULT                       (_MSC_READCTRL_IFCDIS_DEFAULT << 3)   /**< Shifted mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_AIDIS                                (0x1UL << 4)                          /**< Automatic Invalidate Disable */
#define _MSC_READCTRL_AIDIS_SHIFT                         4                                     /**< Shift value for MSC_AIDIS */
#define _MSC_READCTRL_AIDIS_MASK                          0x10UL                                /**< Bit mask for MSC_AIDIS */
#define _MSC_READCTRL_AIDIS_DEFAULT                       0x00000000UL                          /**< Mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_AIDIS_DEFAULT                        (_MSC_READCTRL_AIDIS_DEFAULT << 4)    /**< Shifted mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_ICCDIS                               (0x1UL << 5)                          /**< Interrupt Context Cache Disable */
#define _MSC_READCTRL_ICCDIS_SHIFT                        5                                     /**< Shift value for MSC_ICCDIS */
#define _MSC_READCTRL_ICCDIS_MASK                         0x20UL                                /**< Bit mask for MSC_ICCDIS */
#define _MSC_READCTRL_ICCDIS_DEFAULT                      0x00000000UL                          /**< Mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_ICCDIS_DEFAULT                       (_MSC_READCTRL_ICCDIS_DEFAULT << 5)   /**< Shifted mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_PREFETCH                             (0x1UL << 8)                          /**< Prefetch Mode */
#define _MSC_READCTRL_PREFETCH_SHIFT                      8                                     /**< Shift value for MSC_PREFETCH */
#define _MSC_READCTRL_PREFETCH_MASK                       0x100UL                               /**< Bit mask for MSC_PREFETCH */
#define _MSC_READCTRL_PREFETCH_DEFAULT                    0x00000001UL                          /**< Mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_PREFETCH_DEFAULT                     (_MSC_READCTRL_PREFETCH_DEFAULT << 8) /**< Shifted mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_USEHPROT                             (0x1UL << 9)                          /**< AHB_HPROT Mode */
#define _MSC_READCTRL_USEHPROT_SHIFT                      9                                     /**< Shift value for MSC_USEHPROT */
#define _MSC_READCTRL_USEHPROT_MASK                       0x200UL                               /**< Bit mask for MSC_USEHPROT */
#define _MSC_READCTRL_USEHPROT_DEFAULT                    0x00000000UL                          /**< Mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_USEHPROT_DEFAULT                     (_MSC_READCTRL_USEHPROT_DEFAULT << 9) /**< Shifted mode DEFAULT for MSC_READCTRL */
#define _MSC_READCTRL_MODE_SHIFT                          24                                    /**< Shift value for MSC_MODE */
#define _MSC_READCTRL_MODE_MASK                           0x3000000UL                           /**< Bit mask for MSC_MODE */
#define _MSC_READCTRL_MODE_WS0                            0x00000000UL                          /**< Mode WS0 for MSC_READCTRL */
#define _MSC_READCTRL_MODE_DEFAULT                        0x00000001UL                          /**< Mode DEFAULT for MSC_READCTRL */
#define _MSC_READCTRL_MODE_WS1                            0x00000001UL                          /**< Mode WS1 for MSC_READCTRL */
#define _MSC_READCTRL_MODE_WS2                            0x00000002UL                          /**< Mode WS2 for MSC_READCTRL */
#define _MSC_READCTRL_MODE_WS3                            0x00000003UL                          /**< Mode WS3 for MSC_READCTRL */
#define MSC_READCTRL_MODE_WS0                             (_MSC_READCTRL_MODE_WS0 << 24)        /**< Shifted mode WS0 for MSC_READCTRL */
#define MSC_READCTRL_MODE_DEFAULT                         (_MSC_READCTRL_MODE_DEFAULT << 24)    /**< Shifted mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_MODE_WS1                             (_MSC_READCTRL_MODE_WS1 << 24)        /**< Shifted mode WS1 for MSC_READCTRL */
#define MSC_READCTRL_MODE_WS2                             (_MSC_READCTRL_MODE_WS2 << 24)        /**< Shifted mode WS2 for MSC_READCTRL */
#define MSC_READCTRL_MODE_WS3                             (_MSC_READCTRL_MODE_WS3 << 24)        /**< Shifted mode WS3 for MSC_READCTRL */
#define MSC_READCTRL_SCBTP                                (0x1UL << 28)                         /**< Suppress Conditional Branch Target Perfetch */
#define _MSC_READCTRL_SCBTP_SHIFT                         28                                    /**< Shift value for MSC_SCBTP */
#define _MSC_READCTRL_SCBTP_MASK                          0x10000000UL                          /**< Bit mask for MSC_SCBTP */
#define _MSC_READCTRL_SCBTP_DEFAULT                       0x00000000UL                          /**< Mode DEFAULT for MSC_READCTRL */
#define MSC_READCTRL_SCBTP_DEFAULT                        (_MSC_READCTRL_SCBTP_DEFAULT << 28)   /**< Shifted mode DEFAULT for MSC_READCTRL */

/* Bit fields for MSC WRITECTRL */
#define _MSC_WRITECTRL_RESETVALUE                         0x00000000UL                                /**< Default value for MSC_WRITECTRL */
#define _MSC_WRITECTRL_MASK                               0x00000023UL                                /**< Mask for MSC_WRITECTRL */
#define MSC_WRITECTRL_WREN                                (0x1UL << 0)                                /**< Enable Write/Erase Controller  */
#define _MSC_WRITECTRL_WREN_SHIFT                         0                                           /**< Shift value for MSC_WREN */
#define _MSC_WRITECTRL_WREN_MASK                          0x1UL                                       /**< Bit mask for MSC_WREN */
#define _MSC_WRITECTRL_WREN_DEFAULT                       0x00000000UL                                /**< Mode DEFAULT for MSC_WRITECTRL */
#define MSC_WRITECTRL_WREN_DEFAULT                        (_MSC_WRITECTRL_WREN_DEFAULT << 0)          /**< Shifted mode DEFAULT for MSC_WRITECTRL */
#define MSC_WRITECTRL_IRQERASEABORT                       (0x1UL << 1)                                /**< Abort Page Erase on Interrupt */
#define _MSC_WRITECTRL_IRQERASEABORT_SHIFT                1                                           /**< Shift value for MSC_IRQERASEABORT */
#define _MSC_WRITECTRL_IRQERASEABORT_MASK                 0x2UL                                       /**< Bit mask for MSC_IRQERASEABORT */
#define _MSC_WRITECTRL_IRQERASEABORT_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for MSC_WRITECTRL */
#define MSC_WRITECTRL_IRQERASEABORT_DEFAULT               (_MSC_WRITECTRL_IRQERASEABORT_DEFAULT << 1) /**< Shifted mode DEFAULT for MSC_WRITECTRL */
#define MSC_WRITECTRL_RWWEN                               (0x1UL << 5)                                /**< Read-While-Write Enable */
#define _MSC_WRITECTRL_RWWEN_SHIFT                        5                                           /**< Shift value for MSC_RWWEN */
#define _MSC_WRITECTRL_RWWEN_MASK                         0x20UL                                      /**< Bit mask for MSC_RWWEN */
#define _MSC_WRITECTRL_RWWEN_DEFAULT                      0x00000000UL                                /**< Mode DEFAULT for MSC_WRITECTRL */
#define MSC_WRITECTRL_RWWEN_DEFAULT                       (_MSC_WRITECTRL_RWWEN_DEFAULT << 5)         /**< Shifted mode DEFAULT for MSC_WRITECTRL */

/* Bit fields for MSC WRITECMD */
#define _MSC_WRITECMD_RESETVALUE                          0x00000000UL                             /**< Default value for MSC_WRITECMD */
#define _MSC_WRITECMD_MASK                                0x0000133FUL                             /**< Mask for MSC_WRITECMD */
#define MSC_WRITECMD_LADDRIM                              (0x1UL << 0)                             /**< Load MSC_ADDRB into ADDR */
#define _MSC_WRITECMD_LADDRIM_SHIFT                       0                                        /**< Shift value for MSC_LADDRIM */
#define _MSC_WRITECMD_LADDRIM_MASK                        0x1UL                                    /**< Bit mask for MSC_LADDRIM */
#define _MSC_WRITECMD_LADDRIM_DEFAULT                     0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_LADDRIM_DEFAULT                      (_MSC_WRITECMD_LADDRIM_DEFAULT << 0)     /**< Shifted mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_ERASEPAGE                            (0x1UL << 1)                             /**< Erase Page */
#define _MSC_WRITECMD_ERASEPAGE_SHIFT                     1                                        /**< Shift value for MSC_ERASEPAGE */
#define _MSC_WRITECMD_ERASEPAGE_MASK                      0x2UL                                    /**< Bit mask for MSC_ERASEPAGE */
#define _MSC_WRITECMD_ERASEPAGE_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_ERASEPAGE_DEFAULT                    (_MSC_WRITECMD_ERASEPAGE_DEFAULT << 1)   /**< Shifted mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_WRITEEND                             (0x1UL << 2)                             /**< End Write Mode */
#define _MSC_WRITECMD_WRITEEND_SHIFT                      2                                        /**< Shift value for MSC_WRITEEND */
#define _MSC_WRITECMD_WRITEEND_MASK                       0x4UL                                    /**< Bit mask for MSC_WRITEEND */
#define _MSC_WRITECMD_WRITEEND_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_WRITEEND_DEFAULT                     (_MSC_WRITECMD_WRITEEND_DEFAULT << 2)    /**< Shifted mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_WRITEONCE                            (0x1UL << 3)                             /**< Word Write-Once Trigger */
#define _MSC_WRITECMD_WRITEONCE_SHIFT                     3                                        /**< Shift value for MSC_WRITEONCE */
#define _MSC_WRITECMD_WRITEONCE_MASK                      0x8UL                                    /**< Bit mask for MSC_WRITEONCE */
#define _MSC_WRITECMD_WRITEONCE_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_WRITEONCE_DEFAULT                    (_MSC_WRITECMD_WRITEONCE_DEFAULT << 3)   /**< Shifted mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_WRITETRIG                            (0x1UL << 4)                             /**< Word Write Sequence Trigger */
#define _MSC_WRITECMD_WRITETRIG_SHIFT                     4                                        /**< Shift value for MSC_WRITETRIG */
#define _MSC_WRITECMD_WRITETRIG_MASK                      0x10UL                                   /**< Bit mask for MSC_WRITETRIG */
#define _MSC_WRITECMD_WRITETRIG_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_WRITETRIG_DEFAULT                    (_MSC_WRITECMD_WRITETRIG_DEFAULT << 4)   /**< Shifted mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_ERASEABORT                           (0x1UL << 5)                             /**< Abort erase sequence */
#define _MSC_WRITECMD_ERASEABORT_SHIFT                    5                                        /**< Shift value for MSC_ERASEABORT */
#define _MSC_WRITECMD_ERASEABORT_MASK                     0x20UL                                   /**< Bit mask for MSC_ERASEABORT */
#define _MSC_WRITECMD_ERASEABORT_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_ERASEABORT_DEFAULT                   (_MSC_WRITECMD_ERASEABORT_DEFAULT << 5)  /**< Shifted mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_ERASEMAIN0                           (0x1UL << 8)                             /**< Mass erase region 0 */
#define _MSC_WRITECMD_ERASEMAIN0_SHIFT                    8                                        /**< Shift value for MSC_ERASEMAIN0 */
#define _MSC_WRITECMD_ERASEMAIN0_MASK                     0x100UL                                  /**< Bit mask for MSC_ERASEMAIN0 */
#define _MSC_WRITECMD_ERASEMAIN0_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_ERASEMAIN0_DEFAULT                   (_MSC_WRITECMD_ERASEMAIN0_DEFAULT << 8)  /**< Shifted mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_ERASEMAIN1                           (0x1UL << 9)                             /**< Mass erase region 1 */
#define _MSC_WRITECMD_ERASEMAIN1_SHIFT                    9                                        /**< Shift value for MSC_ERASEMAIN1 */
#define _MSC_WRITECMD_ERASEMAIN1_MASK                     0x200UL                                  /**< Bit mask for MSC_ERASEMAIN1 */
#define _MSC_WRITECMD_ERASEMAIN1_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_ERASEMAIN1_DEFAULT                   (_MSC_WRITECMD_ERASEMAIN1_DEFAULT << 9)  /**< Shifted mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_CLEARWDATA                           (0x1UL << 12)                            /**< Clear WDATA state */
#define _MSC_WRITECMD_CLEARWDATA_SHIFT                    12                                       /**< Shift value for MSC_CLEARWDATA */
#define _MSC_WRITECMD_CLEARWDATA_MASK                     0x1000UL                                 /**< Bit mask for MSC_CLEARWDATA */
#define _MSC_WRITECMD_CLEARWDATA_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for MSC_WRITECMD */
#define MSC_WRITECMD_CLEARWDATA_DEFAULT                   (_MSC_WRITECMD_CLEARWDATA_DEFAULT << 12) /**< Shifted mode DEFAULT for MSC_WRITECMD */

/* Bit fields for MSC ADDRB */
#define _MSC_ADDRB_RESETVALUE                             0x00000000UL                    /**< Default value for MSC_ADDRB */
#define _MSC_ADDRB_MASK                                   0xFFFFFFFFUL                    /**< Mask for MSC_ADDRB */
#define _MSC_ADDRB_ADDRB_SHIFT                            0                               /**< Shift value for MSC_ADDRB */
#define _MSC_ADDRB_ADDRB_MASK                             0xFFFFFFFFUL                    /**< Bit mask for MSC_ADDRB */
#define _MSC_ADDRB_ADDRB_DEFAULT                          0x00000000UL                    /**< Mode DEFAULT for MSC_ADDRB */
#define MSC_ADDRB_ADDRB_DEFAULT                           (_MSC_ADDRB_ADDRB_DEFAULT << 0) /**< Shifted mode DEFAULT for MSC_ADDRB */

/* Bit fields for MSC WDATA */
#define _MSC_WDATA_RESETVALUE                             0x00000000UL                    /**< Default value for MSC_WDATA */
#define _MSC_WDATA_MASK                                   0xFFFFFFFFUL                    /**< Mask for MSC_WDATA */
#define _MSC_WDATA_WDATA_SHIFT                            0                               /**< Shift value for MSC_WDATA */
#define _MSC_WDATA_WDATA_MASK                             0xFFFFFFFFUL                    /**< Bit mask for MSC_WDATA */
#define _MSC_WDATA_WDATA_DEFAULT                          0x00000000UL                    /**< Mode DEFAULT for MSC_WDATA */
#define MSC_WDATA_WDATA_DEFAULT                           (_MSC_WDATA_WDATA_DEFAULT << 0) /**< Shifted mode DEFAULT for MSC_WDATA */

/* Bit fields for MSC STATUS */
#define _MSC_STATUS_RESETVALUE                            0x00000008UL                                   /**< Default value for MSC_STATUS */
#define _MSC_STATUS_MASK                                  0xFF0000FFUL                                   /**< Mask for MSC_STATUS */
#define MSC_STATUS_BUSY                                   (0x1UL << 0)                                   /**< Erase/Write Busy */
#define _MSC_STATUS_BUSY_SHIFT                            0                                              /**< Shift value for MSC_BUSY */
#define _MSC_STATUS_BUSY_MASK                             0x1UL                                          /**< Bit mask for MSC_BUSY */
#define _MSC_STATUS_BUSY_DEFAULT                          0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_BUSY_DEFAULT                           (_MSC_STATUS_BUSY_DEFAULT << 0)                /**< Shifted mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_LOCKED                                 (0x1UL << 1)                                   /**< Access Locked */
#define _MSC_STATUS_LOCKED_SHIFT                          1                                              /**< Shift value for MSC_LOCKED */
#define _MSC_STATUS_LOCKED_MASK                           0x2UL                                          /**< Bit mask for MSC_LOCKED */
#define _MSC_STATUS_LOCKED_DEFAULT                        0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_LOCKED_DEFAULT                         (_MSC_STATUS_LOCKED_DEFAULT << 1)              /**< Shifted mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_INVADDR                                (0x1UL << 2)                                   /**< Invalid Write Address or Erase Page */
#define _MSC_STATUS_INVADDR_SHIFT                         2                                              /**< Shift value for MSC_INVADDR */
#define _MSC_STATUS_INVADDR_MASK                          0x4UL                                          /**< Bit mask for MSC_INVADDR */
#define _MSC_STATUS_INVADDR_DEFAULT                       0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_INVADDR_DEFAULT                        (_MSC_STATUS_INVADDR_DEFAULT << 2)             /**< Shifted mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_WDATAREADY                             (0x1UL << 3)                                   /**< WDATA Write Ready */
#define _MSC_STATUS_WDATAREADY_SHIFT                      3                                              /**< Shift value for MSC_WDATAREADY */
#define _MSC_STATUS_WDATAREADY_MASK                       0x8UL                                          /**< Bit mask for MSC_WDATAREADY */
#define _MSC_STATUS_WDATAREADY_DEFAULT                    0x00000001UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_WDATAREADY_DEFAULT                     (_MSC_STATUS_WDATAREADY_DEFAULT << 3)          /**< Shifted mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_WORDTIMEOUT                            (0x1UL << 4)                                   /**< Flash Write Word Timeout */
#define _MSC_STATUS_WORDTIMEOUT_SHIFT                     4                                              /**< Shift value for MSC_WORDTIMEOUT */
#define _MSC_STATUS_WORDTIMEOUT_MASK                      0x10UL                                         /**< Bit mask for MSC_WORDTIMEOUT */
#define _MSC_STATUS_WORDTIMEOUT_DEFAULT                   0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_WORDTIMEOUT_DEFAULT                    (_MSC_STATUS_WORDTIMEOUT_DEFAULT << 4)         /**< Shifted mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_ERASEABORTED                           (0x1UL << 5)                                   /**< The Current Flash Erase Operation Aborted */
#define _MSC_STATUS_ERASEABORTED_SHIFT                    5                                              /**< Shift value for MSC_ERASEABORTED */
#define _MSC_STATUS_ERASEABORTED_MASK                     0x20UL                                         /**< Bit mask for MSC_ERASEABORTED */
#define _MSC_STATUS_ERASEABORTED_DEFAULT                  0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_ERASEABORTED_DEFAULT                   (_MSC_STATUS_ERASEABORTED_DEFAULT << 5)        /**< Shifted mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_PCRUNNING                              (0x1UL << 6)                                   /**< Performance Counters Running */
#define _MSC_STATUS_PCRUNNING_SHIFT                       6                                              /**< Shift value for MSC_PCRUNNING */
#define _MSC_STATUS_PCRUNNING_MASK                        0x40UL                                         /**< Bit mask for MSC_PCRUNNING */
#define _MSC_STATUS_PCRUNNING_DEFAULT                     0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_PCRUNNING_DEFAULT                      (_MSC_STATUS_PCRUNNING_DEFAULT << 6)           /**< Shifted mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_BANKSWITCHED                           (0x1UL << 7)                                   /**< BANK SWITCHING STATUS */
#define _MSC_STATUS_BANKSWITCHED_SHIFT                    7                                              /**< Shift value for MSC_BANKSWITCHED */
#define _MSC_STATUS_BANKSWITCHED_MASK                     0x80UL                                         /**< Bit mask for MSC_BANKSWITCHED */
#define _MSC_STATUS_BANKSWITCHED_DEFAULT                  0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_BANKSWITCHED_DEFAULT                   (_MSC_STATUS_BANKSWITCHED_DEFAULT << 7)        /**< Shifted mode DEFAULT for MSC_STATUS */
#define _MSC_STATUS_WDATAVALID_SHIFT                      24                                             /**< Shift value for MSC_WDATAVALID */
#define _MSC_STATUS_WDATAVALID_MASK                       0xF000000UL                                    /**< Bit mask for MSC_WDATAVALID */
#define _MSC_STATUS_WDATAVALID_DEFAULT                    0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_WDATAVALID_DEFAULT                     (_MSC_STATUS_WDATAVALID_DEFAULT << 24)         /**< Shifted mode DEFAULT for MSC_STATUS */
#define _MSC_STATUS_PWRUPCKBDFAILCOUNT_SHIFT              28                                             /**< Shift value for MSC_PWRUPCKBDFAILCOUNT */
#define _MSC_STATUS_PWRUPCKBDFAILCOUNT_MASK               0xF0000000UL                                   /**< Bit mask for MSC_PWRUPCKBDFAILCOUNT */
#define _MSC_STATUS_PWRUPCKBDFAILCOUNT_DEFAULT            0x00000000UL                                   /**< Mode DEFAULT for MSC_STATUS */
#define MSC_STATUS_PWRUPCKBDFAILCOUNT_DEFAULT             (_MSC_STATUS_PWRUPCKBDFAILCOUNT_DEFAULT << 28) /**< Shifted mode DEFAULT for MSC_STATUS */

/* Bit fields for MSC IF */
#define _MSC_IF_RESETVALUE                                0x00000000UL                    /**< Default value for MSC_IF */
#define _MSC_IF_MASK                                      0x0000017FUL                    /**< Mask for MSC_IF */
#define MSC_IF_ERASE                                      (0x1UL << 0)                    /**< Erase Done Interrupt Read Flag */
#define _MSC_IF_ERASE_SHIFT                               0                               /**< Shift value for MSC_ERASE */
#define _MSC_IF_ERASE_MASK                                0x1UL                           /**< Bit mask for MSC_ERASE */
#define _MSC_IF_ERASE_DEFAULT                             0x00000000UL                    /**< Mode DEFAULT for MSC_IF */
#define MSC_IF_ERASE_DEFAULT                              (_MSC_IF_ERASE_DEFAULT << 0)    /**< Shifted mode DEFAULT for MSC_IF */
#define MSC_IF_WRITE                                      (0x1UL << 1)                    /**< Write Done Interrupt Read Flag */
#define _MSC_IF_WRITE_SHIFT                               1                               /**< Shift value for MSC_WRITE */
#define _MSC_IF_WRITE_MASK                                0x2UL                           /**< Bit mask for MSC_WRITE */
#define _MSC_IF_WRITE_DEFAULT                             0x00000000UL                    /**< Mode DEFAULT for MSC_IF */
#define MSC_IF_WRITE_DEFAULT                              (_MSC_IF_WRITE_DEFAULT << 1)    /**< Shifted mode DEFAULT for MSC_IF */
#define MSC_IF_CHOF                                       (0x1UL << 2)                    /**< Cache Hits Overflow Interrupt Flag */
#define _MSC_IF_CHOF_SHIFT                                2                               /**< Shift value for MSC_CHOF */
#define _MSC_IF_CHOF_MASK                                 0x4UL                           /**< Bit mask for MSC_CHOF */
#define _MSC_IF_CHOF_DEFAULT                              0x00000000UL                    /**< Mode DEFAULT for MSC_IF */
#define MSC_IF_CHOF_DEFAULT                               (_MSC_IF_CHOF_DEFAULT << 2)     /**< Shifted mode DEFAULT for MSC_IF */
#define MSC_IF_CMOF                                       (0x1UL << 3)                    /**< Cache Misses Overflow Interrupt Flag */
#define _MSC_IF_CMOF_SHIFT                                3                               /**< Shift value for MSC_CMOF */
#define _MSC_IF_CMOF_MASK                                 0x8UL                           /**< Bit mask for MSC_CMOF */
#define _MSC_IF_CMOF_DEFAULT                              0x00000000UL                    /**< Mode DEFAULT for MSC_IF */
#define MSC_IF_CMOF_DEFAULT                               (_MSC_IF_CMOF_DEFAULT << 3)     /**< Shifted mode DEFAULT for MSC_IF */
#define MSC_IF_PWRUPF                                     (0x1UL << 4)                    /**< Flash Power Up Sequence Complete Flag */
#define _MSC_IF_PWRUPF_SHIFT                              4                               /**< Shift value for MSC_PWRUPF */
#define _MSC_IF_PWRUPF_MASK                               0x10UL                          /**< Bit mask for MSC_PWRUPF */
#define _MSC_IF_PWRUPF_DEFAULT                            0x00000000UL                    /**< Mode DEFAULT for MSC_IF */
#define MSC_IF_PWRUPF_DEFAULT                             (_MSC_IF_PWRUPF_DEFAULT << 4)   /**< Shifted mode DEFAULT for MSC_IF */
#define MSC_IF_ICACHERR                                   (0x1UL << 5)                    /**< iCache RAM Parity Error Flag */
#define _MSC_IF_ICACHERR_SHIFT                            5                               /**< Shift value for MSC_ICACHERR */
#define _MSC_IF_ICACHERR_MASK                             0x20UL                          /**< Bit mask for MSC_ICACHERR */
#define _MSC_IF_ICACHERR_DEFAULT                          0x00000000UL                    /**< Mode DEFAULT for MSC_IF */
#define MSC_IF_ICACHERR_DEFAULT                           (_MSC_IF_ICACHERR_DEFAULT << 5) /**< Shifted mode DEFAULT for MSC_IF */
#define MSC_IF_WDATAOV                                    (0x1UL << 6)                    /**< Flash controller write buffer overflow */
#define _MSC_IF_WDATAOV_SHIFT                             6                               /**< Shift value for MSC_WDATAOV */
#define _MSC_IF_WDATAOV_MASK                              0x40UL                          /**< Bit mask for MSC_WDATAOV */
#define _MSC_IF_WDATAOV_DEFAULT                           0x00000000UL                    /**< Mode DEFAULT for MSC_IF */
#define MSC_IF_WDATAOV_DEFAULT                            (_MSC_IF_WDATAOV_DEFAULT << 6)  /**< Shifted mode DEFAULT for MSC_IF */
#define MSC_IF_LVEWRITE                                   (0x1UL << 8)                    /**< Flash LVE Write Error Flag */
#define _MSC_IF_LVEWRITE_SHIFT                            8                               /**< Shift value for MSC_LVEWRITE */
#define _MSC_IF_LVEWRITE_MASK                             0x100UL                         /**< Bit mask for MSC_LVEWRITE */
#define _MSC_IF_LVEWRITE_DEFAULT                          0x00000000UL                    /**< Mode DEFAULT for MSC_IF */
#define MSC_IF_LVEWRITE_DEFAULT                           (_MSC_IF_LVEWRITE_DEFAULT << 8) /**< Shifted mode DEFAULT for MSC_IF */

/* Bit fields for MSC IFS */
#define _MSC_IFS_RESETVALUE                               0x00000000UL                     /**< Default value for MSC_IFS */
#define _MSC_IFS_MASK                                     0x0000017FUL                     /**< Mask for MSC_IFS */
#define MSC_IFS_ERASE                                     (0x1UL << 0)                     /**< Set ERASE Interrupt Flag */
#define _MSC_IFS_ERASE_SHIFT                              0                                /**< Shift value for MSC_ERASE */
#define _MSC_IFS_ERASE_MASK                               0x1UL                            /**< Bit mask for MSC_ERASE */
#define _MSC_IFS_ERASE_DEFAULT                            0x00000000UL                     /**< Mode DEFAULT for MSC_IFS */
#define MSC_IFS_ERASE_DEFAULT                             (_MSC_IFS_ERASE_DEFAULT << 0)    /**< Shifted mode DEFAULT for MSC_IFS */
#define MSC_IFS_WRITE                                     (0x1UL << 1)                     /**< Set WRITE Interrupt Flag */
#define _MSC_IFS_WRITE_SHIFT                              1                                /**< Shift value for MSC_WRITE */
#define _MSC_IFS_WRITE_MASK                               0x2UL                            /**< Bit mask for MSC_WRITE */
#define _MSC_IFS_WRITE_DEFAULT                            0x00000000UL                     /**< Mode DEFAULT for MSC_IFS */
#define MSC_IFS_WRITE_DEFAULT                             (_MSC_IFS_WRITE_DEFAULT << 1)    /**< Shifted mode DEFAULT for MSC_IFS */
#define MSC_IFS_CHOF                                      (0x1UL << 2)                     /**< Set CHOF Interrupt Flag */
#define _MSC_IFS_CHOF_SHIFT                               2                                /**< Shift value for MSC_CHOF */
#define _MSC_IFS_CHOF_MASK                                0x4UL                            /**< Bit mask for MSC_CHOF */
#define _MSC_IFS_CHOF_DEFAULT                             0x00000000UL                     /**< Mode DEFAULT for MSC_IFS */
#define MSC_IFS_CHOF_DEFAULT                              (_MSC_IFS_CHOF_DEFAULT << 2)     /**< Shifted mode DEFAULT for MSC_IFS */
#define MSC_IFS_CMOF                                      (0x1UL << 3)                     /**< Set CMOF Interrupt Flag */
#define _MSC_IFS_CMOF_SHIFT                               3                                /**< Shift value for MSC_CMOF */
#define _MSC_IFS_CMOF_MASK                                0x8UL                            /**< Bit mask for MSC_CMOF */
#define _MSC_IFS_CMOF_DEFAULT                             0x00000000UL                     /**< Mode DEFAULT for MSC_IFS */
#define MSC_IFS_CMOF_DEFAULT                              (_MSC_IFS_CMOF_DEFAULT << 3)     /**< Shifted mode DEFAULT for MSC_IFS */
#define MSC_IFS_PWRUPF                                    (0x1UL << 4)                     /**< Set PWRUPF Interrupt Flag */
#define _MSC_IFS_PWRUPF_SHIFT                             4                                /**< Shift value for MSC_PWRUPF */
#define _MSC_IFS_PWRUPF_MASK                              0x10UL                           /**< Bit mask for MSC_PWRUPF */
#define _MSC_IFS_PWRUPF_DEFAULT                           0x00000000UL                     /**< Mode DEFAULT for MSC_IFS */
#define MSC_IFS_PWRUPF_DEFAULT                            (_MSC_IFS_PWRUPF_DEFAULT << 4)   /**< Shifted mode DEFAULT for MSC_IFS */
#define MSC_IFS_ICACHERR                                  (0x1UL << 5)                     /**< Set ICACHERR Interrupt Flag */
#define _MSC_IFS_ICACHERR_SHIFT                           5                                /**< Shift value for MSC_ICACHERR */
#define _MSC_IFS_ICACHERR_MASK                            0x20UL                           /**< Bit mask for MSC_ICACHERR */
#define _MSC_IFS_ICACHERR_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for MSC_IFS */
#define MSC_IFS_ICACHERR_DEFAULT                          (_MSC_IFS_ICACHERR_DEFAULT << 5) /**< Shifted mode DEFAULT for MSC_IFS */
#define MSC_IFS_WDATAOV                                   (0x1UL << 6)                     /**< Set WDATAOV Interrupt Flag */
#define _MSC_IFS_WDATAOV_SHIFT                            6                                /**< Shift value for MSC_WDATAOV */
#define _MSC_IFS_WDATAOV_MASK                             0x40UL                           /**< Bit mask for MSC_WDATAOV */
#define _MSC_IFS_WDATAOV_DEFAULT                          0x00000000UL                     /**< Mode DEFAULT for MSC_IFS */
#define MSC_IFS_WDATAOV_DEFAULT                           (_MSC_IFS_WDATAOV_DEFAULT << 6)  /**< Shifted mode DEFAULT for MSC_IFS */
#define MSC_IFS_LVEWRITE                                  (0x1UL << 8)                     /**< Set LVEWRITE Interrupt Flag */
#define _MSC_IFS_LVEWRITE_SHIFT                           8                                /**< Shift value for MSC_LVEWRITE */
#define _MSC_IFS_LVEWRITE_MASK                            0x100UL                          /**< Bit mask for MSC_LVEWRITE */
#define _MSC_IFS_LVEWRITE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for MSC_IFS */
#define MSC_IFS_LVEWRITE_DEFAULT                          (_MSC_IFS_LVEWRITE_DEFAULT << 8) /**< Shifted mode DEFAULT for MSC_IFS */

/* Bit fields for MSC IFC */
#define _MSC_IFC_RESETVALUE                               0x00000000UL                     /**< Default value for MSC_IFC */
#define _MSC_IFC_MASK                                     0x0000017FUL                     /**< Mask for MSC_IFC */
#define MSC_IFC_ERASE                                     (0x1UL << 0)                     /**< Clear ERASE Interrupt Flag */
#define _MSC_IFC_ERASE_SHIFT                              0                                /**< Shift value for MSC_ERASE */
#define _MSC_IFC_ERASE_MASK                               0x1UL                            /**< Bit mask for MSC_ERASE */
#define _MSC_IFC_ERASE_DEFAULT                            0x00000000UL                     /**< Mode DEFAULT for MSC_IFC */
#define MSC_IFC_ERASE_DEFAULT                             (_MSC_IFC_ERASE_DEFAULT << 0)    /**< Shifted mode DEFAULT for MSC_IFC */
#define MSC_IFC_WRITE                                     (0x1UL << 1)                     /**< Clear WRITE Interrupt Flag */
#define _MSC_IFC_WRITE_SHIFT                              1                                /**< Shift value for MSC_WRITE */
#define _MSC_IFC_WRITE_MASK                               0x2UL                            /**< Bit mask for MSC_WRITE */
#define _MSC_IFC_WRITE_DEFAULT                            0x00000000UL                     /**< Mode DEFAULT for MSC_IFC */
#define MSC_IFC_WRITE_DEFAULT                             (_MSC_IFC_WRITE_DEFAULT << 1)    /**< Shifted mode DEFAULT for MSC_IFC */
#define MSC_IFC_CHOF                                      (0x1UL << 2)                     /**< Clear CHOF Interrupt Flag */
#define _MSC_IFC_CHOF_SHIFT                               2                                /**< Shift value for MSC_CHOF */
#define _MSC_IFC_CHOF_MASK                                0x4UL                            /**< Bit mask for MSC_CHOF */
#define _MSC_IFC_CHOF_DEFAULT                             0x00000000UL                     /**< Mode DEFAULT for MSC_IFC */
#define MSC_IFC_CHOF_DEFAULT                              (_MSC_IFC_CHOF_DEFAULT << 2)     /**< Shifted mode DEFAULT for MSC_IFC */
#define MSC_IFC_CMOF                                      (0x1UL << 3)                     /**< Clear CMOF Interrupt Flag */
#define _MSC_IFC_CMOF_SHIFT                               3                                /**< Shift value for MSC_CMOF */
#define _MSC_IFC_CMOF_MASK                                0x8UL                            /**< Bit mask for MSC_CMOF */
#define _MSC_IFC_CMOF_DEFAULT                             0x00000000UL                     /**< Mode DEFAULT for MSC_IFC */
#define MSC_IFC_CMOF_DEFAULT                              (_MSC_IFC_CMOF_DEFAULT << 3)     /**< Shifted mode DEFAULT for MSC_IFC */
#define MSC_IFC_PWRUPF                                    (0x1UL << 4)                     /**< Clear PWRUPF Interrupt Flag */
#define _MSC_IFC_PWRUPF_SHIFT                             4                                /**< Shift value for MSC_PWRUPF */
#define _MSC_IFC_PWRUPF_MASK                              0x10UL                           /**< Bit mask for MSC_PWRUPF */
#define _MSC_IFC_PWRUPF_DEFAULT                           0x00000000UL                     /**< Mode DEFAULT for MSC_IFC */
#define MSC_IFC_PWRUPF_DEFAULT                            (_MSC_IFC_PWRUPF_DEFAULT << 4)   /**< Shifted mode DEFAULT for MSC_IFC */
#define MSC_IFC_ICACHERR                                  (0x1UL << 5)                     /**< Clear ICACHERR Interrupt Flag */
#define _MSC_IFC_ICACHERR_SHIFT                           5                                /**< Shift value for MSC_ICACHERR */
#define _MSC_IFC_ICACHERR_MASK                            0x20UL                           /**< Bit mask for MSC_ICACHERR */
#define _MSC_IFC_ICACHERR_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for MSC_IFC */
#define MSC_IFC_ICACHERR_DEFAULT                          (_MSC_IFC_ICACHERR_DEFAULT << 5) /**< Shifted mode DEFAULT for MSC_IFC */
#define MSC_IFC_WDATAOV                                   (0x1UL << 6)                     /**< Clear WDATAOV Interrupt Flag */
#define _MSC_IFC_WDATAOV_SHIFT                            6                                /**< Shift value for MSC_WDATAOV */
#define _MSC_IFC_WDATAOV_MASK                             0x40UL                           /**< Bit mask for MSC_WDATAOV */
#define _MSC_IFC_WDATAOV_DEFAULT                          0x00000000UL                     /**< Mode DEFAULT for MSC_IFC */
#define MSC_IFC_WDATAOV_DEFAULT                           (_MSC_IFC_WDATAOV_DEFAULT << 6)  /**< Shifted mode DEFAULT for MSC_IFC */
#define MSC_IFC_LVEWRITE                                  (0x1UL << 8)                     /**< Clear LVEWRITE Interrupt Flag */
#define _MSC_IFC_LVEWRITE_SHIFT                           8                                /**< Shift value for MSC_LVEWRITE */
#define _MSC_IFC_LVEWRITE_MASK                            0x100UL                          /**< Bit mask for MSC_LVEWRITE */
#define _MSC_IFC_LVEWRITE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for MSC_IFC */
#define MSC_IFC_LVEWRITE_DEFAULT                          (_MSC_IFC_LVEWRITE_DEFAULT << 8) /**< Shifted mode DEFAULT for MSC_IFC */

/* Bit fields for MSC IEN */
#define _MSC_IEN_RESETVALUE                               0x00000000UL                     /**< Default value for MSC_IEN */
#define _MSC_IEN_MASK                                     0x0000017FUL                     /**< Mask for MSC_IEN */
#define MSC_IEN_ERASE                                     (0x1UL << 0)                     /**< ERASE Interrupt Enable */
#define _MSC_IEN_ERASE_SHIFT                              0                                /**< Shift value for MSC_ERASE */
#define _MSC_IEN_ERASE_MASK                               0x1UL                            /**< Bit mask for MSC_ERASE */
#define _MSC_IEN_ERASE_DEFAULT                            0x00000000UL                     /**< Mode DEFAULT for MSC_IEN */
#define MSC_IEN_ERASE_DEFAULT                             (_MSC_IEN_ERASE_DEFAULT << 0)    /**< Shifted mode DEFAULT for MSC_IEN */
#define MSC_IEN_WRITE                                     (0x1UL << 1)                     /**< WRITE Interrupt Enable */
#define _MSC_IEN_WRITE_SHIFT                              1                                /**< Shift value for MSC_WRITE */
#define _MSC_IEN_WRITE_MASK                               0x2UL                            /**< Bit mask for MSC_WRITE */
#define _MSC_IEN_WRITE_DEFAULT                            0x00000000UL                     /**< Mode DEFAULT for MSC_IEN */
#define MSC_IEN_WRITE_DEFAULT                             (_MSC_IEN_WRITE_DEFAULT << 1)    /**< Shifted mode DEFAULT for MSC_IEN */
#define MSC_IEN_CHOF                                      (0x1UL << 2)                     /**< CHOF Interrupt Enable */
#define _MSC_IEN_CHOF_SHIFT                               2                                /**< Shift value for MSC_CHOF */
#define _MSC_IEN_CHOF_MASK                                0x4UL                            /**< Bit mask for MSC_CHOF */
#define _MSC_IEN_CHOF_DEFAULT                             0x00000000UL                     /**< Mode DEFAULT for MSC_IEN */
#define MSC_IEN_CHOF_DEFAULT                              (_MSC_IEN_CHOF_DEFAULT << 2)     /**< Shifted mode DEFAULT for MSC_IEN */
#define MSC_IEN_CMOF                                      (0x1UL << 3)                     /**< CMOF Interrupt Enable */
#define _MSC_IEN_CMOF_SHIFT                               3                                /**< Shift value for MSC_CMOF */
#define _MSC_IEN_CMOF_MASK                                0x8UL                            /**< Bit mask for MSC_CMOF */
#define _MSC_IEN_CMOF_DEFAULT                             0x00000000UL                     /**< Mode DEFAULT for MSC_IEN */
#define MSC_IEN_CMOF_DEFAULT                              (_MSC_IEN_CMOF_DEFAULT << 3)     /**< Shifted mode DEFAULT for MSC_IEN */
#define MSC_IEN_PWRUPF                                    (0x1UL << 4)                     /**< PWRUPF Interrupt Enable */
#define _MSC_IEN_PWRUPF_SHIFT                             4                                /**< Shift value for MSC_PWRUPF */
#define _MSC_IEN_PWRUPF_MASK                              0x10UL                           /**< Bit mask for MSC_PWRUPF */
#define _MSC_IEN_PWRUPF_DEFAULT                           0x00000000UL                     /**< Mode DEFAULT for MSC_IEN */
#define MSC_IEN_PWRUPF_DEFAULT                            (_MSC_IEN_PWRUPF_DEFAULT << 4)   /**< Shifted mode DEFAULT for MSC_IEN */
#define MSC_IEN_ICACHERR                                  (0x1UL << 5)                     /**< ICACHERR Interrupt Enable */
#define _MSC_IEN_ICACHERR_SHIFT                           5                                /**< Shift value for MSC_ICACHERR */
#define _MSC_IEN_ICACHERR_MASK                            0x20UL                           /**< Bit mask for MSC_ICACHERR */
#define _MSC_IEN_ICACHERR_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for MSC_IEN */
#define MSC_IEN_ICACHERR_DEFAULT                          (_MSC_IEN_ICACHERR_DEFAULT << 5) /**< Shifted mode DEFAULT for MSC_IEN */
#define MSC_IEN_WDATAOV                                   (0x1UL << 6)                     /**< WDATAOV Interrupt Enable */
#define _MSC_IEN_WDATAOV_SHIFT                            6                                /**< Shift value for MSC_WDATAOV */
#define _MSC_IEN_WDATAOV_MASK                             0x40UL                           /**< Bit mask for MSC_WDATAOV */
#define _MSC_IEN_WDATAOV_DEFAULT                          0x00000000UL                     /**< Mode DEFAULT for MSC_IEN */
#define MSC_IEN_WDATAOV_DEFAULT                           (_MSC_IEN_WDATAOV_DEFAULT << 6)  /**< Shifted mode DEFAULT for MSC_IEN */
#define MSC_IEN_LVEWRITE                                  (0x1UL << 8)                     /**< LVEWRITE Interrupt Enable */
#define _MSC_IEN_LVEWRITE_SHIFT                           8                                /**< Shift value for MSC_LVEWRITE */
#define _MSC_IEN_LVEWRITE_MASK                            0x100UL                          /**< Bit mask for MSC_LVEWRITE */
#define _MSC_IEN_LVEWRITE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for MSC_IEN */
#define MSC_IEN_LVEWRITE_DEFAULT                          (_MSC_IEN_LVEWRITE_DEFAULT << 8) /**< Shifted mode DEFAULT for MSC_IEN */

/* Bit fields for MSC LOCK */
#define _MSC_LOCK_RESETVALUE                              0x00000000UL                      /**< Default value for MSC_LOCK */
#define _MSC_LOCK_MASK                                    0x0000FFFFUL                      /**< Mask for MSC_LOCK */
#define _MSC_LOCK_LOCKKEY_SHIFT                           0                                 /**< Shift value for MSC_LOCKKEY */
#define _MSC_LOCK_LOCKKEY_MASK                            0xFFFFUL                          /**< Bit mask for MSC_LOCKKEY */
#define _MSC_LOCK_LOCKKEY_DEFAULT                         0x00000000UL                      /**< Mode DEFAULT for MSC_LOCK */
#define _MSC_LOCK_LOCKKEY_LOCK                            0x00000000UL                      /**< Mode LOCK for MSC_LOCK */
#define _MSC_LOCK_LOCKKEY_UNLOCKED                        0x00000000UL                      /**< Mode UNLOCKED for MSC_LOCK */
#define _MSC_LOCK_LOCKKEY_LOCKED                          0x00000001UL                      /**< Mode LOCKED for MSC_LOCK */
#define _MSC_LOCK_LOCKKEY_UNLOCK                          0x00001B71UL                      /**< Mode UNLOCK for MSC_LOCK */
#define MSC_LOCK_LOCKKEY_DEFAULT                          (_MSC_LOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for MSC_LOCK */
#define MSC_LOCK_LOCKKEY_LOCK                             (_MSC_LOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for MSC_LOCK */
#define MSC_LOCK_LOCKKEY_UNLOCKED                         (_MSC_LOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for MSC_LOCK */
#define MSC_LOCK_LOCKKEY_LOCKED                           (_MSC_LOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for MSC_LOCK */
#define MSC_LOCK_LOCKKEY_UNLOCK                           (_MSC_LOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for MSC_LOCK */

/* Bit fields for MSC CACHECMD */
#define _MSC_CACHECMD_RESETVALUE                          0x00000000UL                          /**< Default value for MSC_CACHECMD */
#define _MSC_CACHECMD_MASK                                0x00000007UL                          /**< Mask for MSC_CACHECMD */
#define MSC_CACHECMD_INVCACHE                             (0x1UL << 0)                          /**< Invalidate Instruction Cache */
#define _MSC_CACHECMD_INVCACHE_SHIFT                      0                                     /**< Shift value for MSC_INVCACHE */
#define _MSC_CACHECMD_INVCACHE_MASK                       0x1UL                                 /**< Bit mask for MSC_INVCACHE */
#define _MSC_CACHECMD_INVCACHE_DEFAULT                    0x00000000UL                          /**< Mode DEFAULT for MSC_CACHECMD */
#define MSC_CACHECMD_INVCACHE_DEFAULT                     (_MSC_CACHECMD_INVCACHE_DEFAULT << 0) /**< Shifted mode DEFAULT for MSC_CACHECMD */
#define MSC_CACHECMD_STARTPC                              (0x1UL << 1)                          /**< Start Performance Counters */
#define _MSC_CACHECMD_STARTPC_SHIFT                       1                                     /**< Shift value for MSC_STARTPC */
#define _MSC_CACHECMD_STARTPC_MASK                        0x2UL                                 /**< Bit mask for MSC_STARTPC */
#define _MSC_CACHECMD_STARTPC_DEFAULT                     0x00000000UL                          /**< Mode DEFAULT for MSC_CACHECMD */
#define MSC_CACHECMD_STARTPC_DEFAULT                      (_MSC_CACHECMD_STARTPC_DEFAULT << 1)  /**< Shifted mode DEFAULT for MSC_CACHECMD */
#define MSC_CACHECMD_STOPPC                               (0x1UL << 2)                          /**< Stop Performance Counters */
#define _MSC_CACHECMD_STOPPC_SHIFT                        2                                     /**< Shift value for MSC_STOPPC */
#define _MSC_CACHECMD_STOPPC_MASK                         0x4UL                                 /**< Bit mask for MSC_STOPPC */
#define _MSC_CACHECMD_STOPPC_DEFAULT                      0x00000000UL                          /**< Mode DEFAULT for MSC_CACHECMD */
#define MSC_CACHECMD_STOPPC_DEFAULT                       (_MSC_CACHECMD_STOPPC_DEFAULT << 2)   /**< Shifted mode DEFAULT for MSC_CACHECMD */

/* Bit fields for MSC CACHEHITS */
#define _MSC_CACHEHITS_RESETVALUE                         0x00000000UL                            /**< Default value for MSC_CACHEHITS */
#define _MSC_CACHEHITS_MASK                               0x000FFFFFUL                            /**< Mask for MSC_CACHEHITS */
#define _MSC_CACHEHITS_CACHEHITS_SHIFT                    0                                       /**< Shift value for MSC_CACHEHITS */
#define _MSC_CACHEHITS_CACHEHITS_MASK                     0xFFFFFUL                               /**< Bit mask for MSC_CACHEHITS */
#define _MSC_CACHEHITS_CACHEHITS_DEFAULT                  0x00000000UL                            /**< Mode DEFAULT for MSC_CACHEHITS */
#define MSC_CACHEHITS_CACHEHITS_DEFAULT                   (_MSC_CACHEHITS_CACHEHITS_DEFAULT << 0) /**< Shifted mode DEFAULT for MSC_CACHEHITS */

/* Bit fields for MSC CACHEMISSES */
#define _MSC_CACHEMISSES_RESETVALUE                       0x00000000UL                                /**< Default value for MSC_CACHEMISSES */
#define _MSC_CACHEMISSES_MASK                             0x000FFFFFUL                                /**< Mask for MSC_CACHEMISSES */
#define _MSC_CACHEMISSES_CACHEMISSES_SHIFT                0                                           /**< Shift value for MSC_CACHEMISSES */
#define _MSC_CACHEMISSES_CACHEMISSES_MASK                 0xFFFFFUL                                   /**< Bit mask for MSC_CACHEMISSES */
#define _MSC_CACHEMISSES_CACHEMISSES_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for MSC_CACHEMISSES */
#define MSC_CACHEMISSES_CACHEMISSES_DEFAULT               (_MSC_CACHEMISSES_CACHEMISSES_DEFAULT << 0) /**< Shifted mode DEFAULT for MSC_CACHEMISSES */

/* Bit fields for MSC MASSLOCK */
#define _MSC_MASSLOCK_RESETVALUE                          0x00000001UL                          /**< Default value for MSC_MASSLOCK */
#define _MSC_MASSLOCK_MASK                                0x0000FFFFUL                          /**< Mask for MSC_MASSLOCK */
#define _MSC_MASSLOCK_LOCKKEY_SHIFT                       0                                     /**< Shift value for MSC_LOCKKEY */
#define _MSC_MASSLOCK_LOCKKEY_MASK                        0xFFFFUL                              /**< Bit mask for MSC_LOCKKEY */
#define _MSC_MASSLOCK_LOCKKEY_LOCK                        0x00000000UL                          /**< Mode LOCK for MSC_MASSLOCK */
#define _MSC_MASSLOCK_LOCKKEY_UNLOCKED                    0x00000000UL                          /**< Mode UNLOCKED for MSC_MASSLOCK */
#define _MSC_MASSLOCK_LOCKKEY_DEFAULT                     0x00000001UL                          /**< Mode DEFAULT for MSC_MASSLOCK */
#define _MSC_MASSLOCK_LOCKKEY_LOCKED                      0x00000001UL                          /**< Mode LOCKED for MSC_MASSLOCK */
#define _MSC_MASSLOCK_LOCKKEY_UNLOCK                      0x0000631AUL                          /**< Mode UNLOCK for MSC_MASSLOCK */
#define MSC_MASSLOCK_LOCKKEY_LOCK                         (_MSC_MASSLOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for MSC_MASSLOCK */
#define MSC_MASSLOCK_LOCKKEY_UNLOCKED                     (_MSC_MASSLOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for MSC_MASSLOCK */
#define MSC_MASSLOCK_LOCKKEY_DEFAULT                      (_MSC_MASSLOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for MSC_MASSLOCK */
#define MSC_MASSLOCK_LOCKKEY_LOCKED                       (_MSC_MASSLOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for MSC_MASSLOCK */
#define MSC_MASSLOCK_LOCKKEY_UNLOCK                       (_MSC_MASSLOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for MSC_MASSLOCK */

/* Bit fields for MSC STARTUP */
#define _MSC_STARTUP_RESETVALUE                           0x1300104DUL                         /**< Default value for MSC_STARTUP */
#define _MSC_STARTUP_MASK                                 0x773FF3FFUL                         /**< Mask for MSC_STARTUP */
#define _MSC_STARTUP_STDLY0_SHIFT                         0                                    /**< Shift value for MSC_STDLY0 */
#define _MSC_STARTUP_STDLY0_MASK                          0x3FFUL                              /**< Bit mask for MSC_STDLY0 */
#define _MSC_STARTUP_STDLY0_DEFAULT                       0x0000004DUL                         /**< Mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_STDLY0_DEFAULT                        (_MSC_STARTUP_STDLY0_DEFAULT << 0)   /**< Shifted mode DEFAULT for MSC_STARTUP */
#define _MSC_STARTUP_STDLY1_SHIFT                         12                                   /**< Shift value for MSC_STDLY1 */
#define _MSC_STARTUP_STDLY1_MASK                          0x3FF000UL                           /**< Bit mask for MSC_STDLY1 */
#define _MSC_STARTUP_STDLY1_DEFAULT                       0x00000001UL                         /**< Mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_STDLY1_DEFAULT                        (_MSC_STARTUP_STDLY1_DEFAULT << 12)  /**< Shifted mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_ASTWAIT                               (0x1UL << 24)                        /**< Active Startup Wait */
#define _MSC_STARTUP_ASTWAIT_SHIFT                        24                                   /**< Shift value for MSC_ASTWAIT */
#define _MSC_STARTUP_ASTWAIT_MASK                         0x1000000UL                          /**< Bit mask for MSC_ASTWAIT */
#define _MSC_STARTUP_ASTWAIT_DEFAULT                      0x00000001UL                         /**< Mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_ASTWAIT_DEFAULT                       (_MSC_STARTUP_ASTWAIT_DEFAULT << 24) /**< Shifted mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_STWSEN                                (0x1UL << 25)                        /**< Startup Waitstates Enable */
#define _MSC_STARTUP_STWSEN_SHIFT                         25                                   /**< Shift value for MSC_STWSEN */
#define _MSC_STARTUP_STWSEN_MASK                          0x2000000UL                          /**< Bit mask for MSC_STWSEN */
#define _MSC_STARTUP_STWSEN_DEFAULT                       0x00000001UL                         /**< Mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_STWSEN_DEFAULT                        (_MSC_STARTUP_STWSEN_DEFAULT << 25)  /**< Shifted mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_STWSAEN                               (0x1UL << 26)                        /**< Startup Waitstates Always Enable */
#define _MSC_STARTUP_STWSAEN_SHIFT                        26                                   /**< Shift value for MSC_STWSAEN */
#define _MSC_STARTUP_STWSAEN_MASK                         0x4000000UL                          /**< Bit mask for MSC_STWSAEN */
#define _MSC_STARTUP_STWSAEN_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_STWSAEN_DEFAULT                       (_MSC_STARTUP_STWSAEN_DEFAULT << 26) /**< Shifted mode DEFAULT for MSC_STARTUP */
#define _MSC_STARTUP_STWS_SHIFT                           28                                   /**< Shift value for MSC_STWS */
#define _MSC_STARTUP_STWS_MASK                            0x70000000UL                         /**< Bit mask for MSC_STWS */
#define _MSC_STARTUP_STWS_DEFAULT                         0x00000001UL                         /**< Mode DEFAULT for MSC_STARTUP */
#define MSC_STARTUP_STWS_DEFAULT                          (_MSC_STARTUP_STWS_DEFAULT << 28)    /**< Shifted mode DEFAULT for MSC_STARTUP */

/* Bit fields for MSC BANKSWITCHLOCK */
#define _MSC_BANKSWITCHLOCK_RESETVALUE                    0x00000001UL                                          /**< Default value for MSC_BANKSWITCHLOCK */
#define _MSC_BANKSWITCHLOCK_MASK                          0x0000FFFFUL                                          /**< Mask for MSC_BANKSWITCHLOCK */
#define _MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_SHIFT       0                                                     /**< Shift value for MSC_BANKSWITCHLOCKKEY */
#define _MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_MASK        0xFFFFUL                                              /**< Bit mask for MSC_BANKSWITCHLOCKKEY */
#define _MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_LOCK        0x00000000UL                                          /**< Mode LOCK for MSC_BANKSWITCHLOCK */
#define _MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_UNLOCKED    0x00000000UL                                          /**< Mode UNLOCKED for MSC_BANKSWITCHLOCK */
#define _MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_DEFAULT     0x00000001UL                                          /**< Mode DEFAULT for MSC_BANKSWITCHLOCK */
#define _MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_LOCKED      0x00000001UL                                          /**< Mode LOCKED for MSC_BANKSWITCHLOCK */
#define _MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_UNLOCK      0x00007C2BUL                                          /**< Mode UNLOCK for MSC_BANKSWITCHLOCK */
#define MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_LOCK         (_MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for MSC_BANKSWITCHLOCK */
#define MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_UNLOCKED     (_MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for MSC_BANKSWITCHLOCK */
#define MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_DEFAULT      (_MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for MSC_BANKSWITCHLOCK */
#define MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_LOCKED       (_MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for MSC_BANKSWITCHLOCK */
#define MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_UNLOCK       (_MSC_BANKSWITCHLOCK_BANKSWITCHLOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for MSC_BANKSWITCHLOCK */

/* Bit fields for MSC CMD */
#define _MSC_CMD_RESETVALUE                               0x00000000UL                          /**< Default value for MSC_CMD */
#define _MSC_CMD_MASK                                     0x00000003UL                          /**< Mask for MSC_CMD */
#define MSC_CMD_PWRUP                                     (0x1UL << 0)                          /**< Flash Power Up Command */
#define _MSC_CMD_PWRUP_SHIFT                              0                                     /**< Shift value for MSC_PWRUP */
#define _MSC_CMD_PWRUP_MASK                               0x1UL                                 /**< Bit mask for MSC_PWRUP */
#define _MSC_CMD_PWRUP_DEFAULT                            0x00000000UL                          /**< Mode DEFAULT for MSC_CMD */
#define MSC_CMD_PWRUP_DEFAULT                             (_MSC_CMD_PWRUP_DEFAULT << 0)         /**< Shifted mode DEFAULT for MSC_CMD */
#define MSC_CMD_SWITCHINGBANK                             (0x1UL << 1)                          /**< BANK SWITCHING COMMAND */
#define _MSC_CMD_SWITCHINGBANK_SHIFT                      1                                     /**< Shift value for MSC_SWITCHINGBANK */
#define _MSC_CMD_SWITCHINGBANK_MASK                       0x2UL                                 /**< Bit mask for MSC_SWITCHINGBANK */
#define _MSC_CMD_SWITCHINGBANK_DEFAULT                    0x00000000UL                          /**< Mode DEFAULT for MSC_CMD */
#define MSC_CMD_SWITCHINGBANK_DEFAULT                     (_MSC_CMD_SWITCHINGBANK_DEFAULT << 1) /**< Shifted mode DEFAULT for MSC_CMD */

/* Bit fields for MSC BOOTLOADERCTRL */
#define _MSC_BOOTLOADERCTRL_RESETVALUE                    0x00000000UL                              /**< Default value for MSC_BOOTLOADERCTRL */
#define _MSC_BOOTLOADERCTRL_MASK                          0x00000003UL                              /**< Mask for MSC_BOOTLOADERCTRL */
#define MSC_BOOTLOADERCTRL_BLRDIS                         (0x1UL << 0)                              /**< Flash Bootloader Read Enable */
#define _MSC_BOOTLOADERCTRL_BLRDIS_SHIFT                  0                                         /**< Shift value for MSC_BLRDIS */
#define _MSC_BOOTLOADERCTRL_BLRDIS_MASK                   0x1UL                                     /**< Bit mask for MSC_BLRDIS */
#define _MSC_BOOTLOADERCTRL_BLRDIS_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for MSC_BOOTLOADERCTRL */
#define MSC_BOOTLOADERCTRL_BLRDIS_DEFAULT                 (_MSC_BOOTLOADERCTRL_BLRDIS_DEFAULT << 0) /**< Shifted mode DEFAULT for MSC_BOOTLOADERCTRL */
#define MSC_BOOTLOADERCTRL_BLWDIS                         (0x1UL << 1)                              /**< Flash Bootloader Write/Erase Eanble */
#define _MSC_BOOTLOADERCTRL_BLWDIS_SHIFT                  1                                         /**< Shift value for MSC_BLWDIS */
#define _MSC_BOOTLOADERCTRL_BLWDIS_MASK                   0x2UL                                     /**< Bit mask for MSC_BLWDIS */
#define _MSC_BOOTLOADERCTRL_BLWDIS_DEFAULT                0x00000000UL                              /**< Mode DEFAULT for MSC_BOOTLOADERCTRL */
#define MSC_BOOTLOADERCTRL_BLWDIS_DEFAULT                 (_MSC_BOOTLOADERCTRL_BLWDIS_DEFAULT << 1) /**< Shifted mode DEFAULT for MSC_BOOTLOADERCTRL */

/* Bit fields for MSC AAPUNLOCKCMD */
#define _MSC_AAPUNLOCKCMD_RESETVALUE                      0x00000000UL                               /**< Default value for MSC_AAPUNLOCKCMD */
#define _MSC_AAPUNLOCKCMD_MASK                            0x00000001UL                               /**< Mask for MSC_AAPUNLOCKCMD */
#define MSC_AAPUNLOCKCMD_UNLOCKAAP                        (0x1UL << 0)                               /**< Software unlock AAP command */
#define _MSC_AAPUNLOCKCMD_UNLOCKAAP_SHIFT                 0                                          /**< Shift value for MSC_UNLOCKAAP */
#define _MSC_AAPUNLOCKCMD_UNLOCKAAP_MASK                  0x1UL                                      /**< Bit mask for MSC_UNLOCKAAP */
#define _MSC_AAPUNLOCKCMD_UNLOCKAAP_DEFAULT               0x00000000UL                               /**< Mode DEFAULT for MSC_AAPUNLOCKCMD */
#define MSC_AAPUNLOCKCMD_UNLOCKAAP_DEFAULT                (_MSC_AAPUNLOCKCMD_UNLOCKAAP_DEFAULT << 0) /**< Shifted mode DEFAULT for MSC_AAPUNLOCKCMD */

/* Bit fields for MSC CACHECONFIG0 */
#define _MSC_CACHECONFIG0_RESETVALUE                      0x00000003UL                                      /**< Default value for MSC_CACHECONFIG0 */
#define _MSC_CACHECONFIG0_MASK                            0x00000003UL                                      /**< Mask for MSC_CACHECONFIG0 */
#define _MSC_CACHECONFIG0_CACHELPLEVEL_SHIFT              0                                                 /**< Shift value for MSC_CACHELPLEVEL */
#define _MSC_CACHECONFIG0_CACHELPLEVEL_MASK               0x3UL                                             /**< Bit mask for MSC_CACHELPLEVEL */
#define _MSC_CACHECONFIG0_CACHELPLEVEL_BASE               0x00000000UL                                      /**< Mode BASE for MSC_CACHECONFIG0 */
#define _MSC_CACHECONFIG0_CACHELPLEVEL_ADVANCED           0x00000001UL                                      /**< Mode ADVANCED for MSC_CACHECONFIG0 */
#define _MSC_CACHECONFIG0_CACHELPLEVEL_DEFAULT            0x00000003UL                                      /**< Mode DEFAULT for MSC_CACHECONFIG0 */
#define _MSC_CACHECONFIG0_CACHELPLEVEL_MINACTIVITY        0x00000003UL                                      /**< Mode MINACTIVITY for MSC_CACHECONFIG0 */
#define MSC_CACHECONFIG0_CACHELPLEVEL_BASE                (_MSC_CACHECONFIG0_CACHELPLEVEL_BASE << 0)        /**< Shifted mode BASE for MSC_CACHECONFIG0 */
#define MSC_CACHECONFIG0_CACHELPLEVEL_ADVANCED            (_MSC_CACHECONFIG0_CACHELPLEVEL_ADVANCED << 0)    /**< Shifted mode ADVANCED for MSC_CACHECONFIG0 */
#define MSC_CACHECONFIG0_CACHELPLEVEL_DEFAULT             (_MSC_CACHECONFIG0_CACHELPLEVEL_DEFAULT << 0)     /**< Shifted mode DEFAULT for MSC_CACHECONFIG0 */
#define MSC_CACHECONFIG0_CACHELPLEVEL_MINACTIVITY         (_MSC_CACHECONFIG0_CACHELPLEVEL_MINACTIVITY << 0) /**< Shifted mode MINACTIVITY for MSC_CACHECONFIG0 */

/* Bit fields for MSC RAMCTRL */
#define _MSC_RAMCTRL_RESETVALUE                           0x00000000UL                               /**< Default value for MSC_RAMCTRL */
#define _MSC_RAMCTRL_MASK                                 0x00090101UL                               /**< Mask for MSC_RAMCTRL */
#define MSC_RAMCTRL_RAMCACHEEN                            (0x1UL << 0)                               /**< RAM CACHE Enable */
#define _MSC_RAMCTRL_RAMCACHEEN_SHIFT                     0                                          /**< Shift value for MSC_RAMCACHEEN */
#define _MSC_RAMCTRL_RAMCACHEEN_MASK                      0x1UL                                      /**< Bit mask for MSC_RAMCACHEEN */
#define _MSC_RAMCTRL_RAMCACHEEN_DEFAULT                   0x00000000UL                               /**< Mode DEFAULT for MSC_RAMCTRL */
#define MSC_RAMCTRL_RAMCACHEEN_DEFAULT                    (_MSC_RAMCTRL_RAMCACHEEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for MSC_RAMCTRL */
#define MSC_RAMCTRL_RAM1CACHEEN                           (0x1UL << 8)                               /**< RAM1 CACHE Enable */
#define _MSC_RAMCTRL_RAM1CACHEEN_SHIFT                    8                                          /**< Shift value for MSC_RAM1CACHEEN */
#define _MSC_RAMCTRL_RAM1CACHEEN_MASK                     0x100UL                                    /**< Bit mask for MSC_RAM1CACHEEN */
#define _MSC_RAMCTRL_RAM1CACHEEN_DEFAULT                  0x00000000UL                               /**< Mode DEFAULT for MSC_RAMCTRL */
#define MSC_RAMCTRL_RAM1CACHEEN_DEFAULT                   (_MSC_RAMCTRL_RAM1CACHEEN_DEFAULT << 8)    /**< Shifted mode DEFAULT for MSC_RAMCTRL */
#define MSC_RAMCTRL_RAM2CACHEEN                           (0x1UL << 16)                              /**< RAM2 CACHE Enable */
#define _MSC_RAMCTRL_RAM2CACHEEN_SHIFT                    16                                         /**< Shift value for MSC_RAM2CACHEEN */
#define _MSC_RAMCTRL_RAM2CACHEEN_MASK                     0x10000UL                                  /**< Bit mask for MSC_RAM2CACHEEN */
#define _MSC_RAMCTRL_RAM2CACHEEN_DEFAULT                  0x00000000UL                               /**< Mode DEFAULT for MSC_RAMCTRL */
#define MSC_RAMCTRL_RAM2CACHEEN_DEFAULT                   (_MSC_RAMCTRL_RAM2CACHEEN_DEFAULT << 16)   /**< Shifted mode DEFAULT for MSC_RAMCTRL */
#define MSC_RAMCTRL_RAMSEQCACHEEN                         (0x1UL << 19)                              /**< RAMSEQ CACHE Enable */
#define _MSC_RAMCTRL_RAMSEQCACHEEN_SHIFT                  19                                         /**< Shift value for MSC_RAMSEQCACHEEN */
#define _MSC_RAMCTRL_RAMSEQCACHEEN_MASK                   0x80000UL                                  /**< Bit mask for MSC_RAMSEQCACHEEN */
#define _MSC_RAMCTRL_RAMSEQCACHEEN_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for MSC_RAMCTRL */
#define MSC_RAMCTRL_RAMSEQCACHEEN_DEFAULT                 (_MSC_RAMCTRL_RAMSEQCACHEEN_DEFAULT << 19) /**< Shifted mode DEFAULT for MSC_RAMCTRL */

/** @} End of group EFM32PG12B_MSC */
/** @} End of group Parts */

