/**************************************************************************//**
 * @file efr32fg1p_ldma.h
 * @brief EFR32FG1P_LDMA register and bit field definitions
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
 * @defgroup EFR32FG1P_LDMA
 * @{
 * @brief EFR32FG1P_LDMA Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t  CTRL;         /**< DMA Control Register  */
  __IM uint32_t   STATUS;       /**< DMA Status Register  */
  __IOM uint32_t  SYNC;         /**< DMA Synchronization Trigger Register (Single-Cycle RMW)  */
  uint32_t        RESERVED0[5]; /**< Reserved for future use **/
  __IOM uint32_t  CHEN;         /**< DMA Channel Enable Register (Single-Cycle RMW)  */
  __IM uint32_t   CHBUSY;       /**< DMA Channel Busy Register  */
  __IOM uint32_t  CHDONE;       /**< DMA Channel Linking Done Register (Single-Cycle RMW)  */
  __IOM uint32_t  DBGHALT;      /**< DMA Channel Debug Halt Register  */
  __IOM uint32_t  SWREQ;        /**< DMA Channel Software Transfer Request Register  */
  __IOM uint32_t  REQDIS;       /**< DMA Channel Request Disable Register  */
  __IM uint32_t   REQPEND;      /**< DMA Channel Requests Pending Register  */
  __IOM uint32_t  LINKLOAD;     /**< DMA Channel Link Load Register  */
  __IOM uint32_t  REQCLEAR;     /**< DMA Channel Request Clear Register  */
  uint32_t        RESERVED1[7]; /**< Reserved for future use **/
  __IM uint32_t   IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t  IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t  IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t  IEN;          /**< Interrupt Enable register  */

  uint32_t        RESERVED2[4]; /**< Reserved registers */
  LDMA_CH_TypeDef CH[8];        /**< DMA Channel Registers */
} LDMA_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_LDMA_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for LDMA CTRL */
#define _LDMA_CTRL_RESETVALUE                        0x07000000UL                           /**< Default value for LDMA_CTRL */
#define _LDMA_CTRL_MASK                              0x0700FFFFUL                           /**< Mask for LDMA_CTRL */
#define _LDMA_CTRL_SYNCPRSSETEN_SHIFT                0                                      /**< Shift value for LDMA_SYNCPRSSETEN */
#define _LDMA_CTRL_SYNCPRSSETEN_MASK                 0xFFUL                                 /**< Bit mask for LDMA_SYNCPRSSETEN */
#define _LDMA_CTRL_SYNCPRSSETEN_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for LDMA_CTRL */
#define LDMA_CTRL_SYNCPRSSETEN_DEFAULT               (_LDMA_CTRL_SYNCPRSSETEN_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_CTRL */
#define _LDMA_CTRL_SYNCPRSCLREN_SHIFT                8                                      /**< Shift value for LDMA_SYNCPRSCLREN */
#define _LDMA_CTRL_SYNCPRSCLREN_MASK                 0xFF00UL                               /**< Bit mask for LDMA_SYNCPRSCLREN */
#define _LDMA_CTRL_SYNCPRSCLREN_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for LDMA_CTRL */
#define LDMA_CTRL_SYNCPRSCLREN_DEFAULT               (_LDMA_CTRL_SYNCPRSCLREN_DEFAULT << 8) /**< Shifted mode DEFAULT for LDMA_CTRL */
#define _LDMA_CTRL_NUMFIXED_SHIFT                    24                                     /**< Shift value for LDMA_NUMFIXED */
#define _LDMA_CTRL_NUMFIXED_MASK                     0x7000000UL                            /**< Bit mask for LDMA_NUMFIXED */
#define _LDMA_CTRL_NUMFIXED_DEFAULT                  0x00000007UL                           /**< Mode DEFAULT for LDMA_CTRL */
#define LDMA_CTRL_NUMFIXED_DEFAULT                   (_LDMA_CTRL_NUMFIXED_DEFAULT << 24)    /**< Shifted mode DEFAULT for LDMA_CTRL */

/* Bit fields for LDMA STATUS */
#define _LDMA_STATUS_RESETVALUE                      0x08100000UL                           /**< Default value for LDMA_STATUS */
#define _LDMA_STATUS_MASK                            0x1F1F073BUL                           /**< Mask for LDMA_STATUS */
#define LDMA_STATUS_ANYBUSY                          (0x1UL << 0)                           /**< Any DMA Channel Busy */
#define _LDMA_STATUS_ANYBUSY_SHIFT                   0                                      /**< Shift value for LDMA_ANYBUSY */
#define _LDMA_STATUS_ANYBUSY_MASK                    0x1UL                                  /**< Bit mask for LDMA_ANYBUSY */
#define _LDMA_STATUS_ANYBUSY_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LDMA_STATUS */
#define LDMA_STATUS_ANYBUSY_DEFAULT                  (_LDMA_STATUS_ANYBUSY_DEFAULT << 0)    /**< Shifted mode DEFAULT for LDMA_STATUS */
#define LDMA_STATUS_ANYREQ                           (0x1UL << 1)                           /**< Any DMA Channel Request Pending */
#define _LDMA_STATUS_ANYREQ_SHIFT                    1                                      /**< Shift value for LDMA_ANYREQ */
#define _LDMA_STATUS_ANYREQ_MASK                     0x2UL                                  /**< Bit mask for LDMA_ANYREQ */
#define _LDMA_STATUS_ANYREQ_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for LDMA_STATUS */
#define LDMA_STATUS_ANYREQ_DEFAULT                   (_LDMA_STATUS_ANYREQ_DEFAULT << 1)     /**< Shifted mode DEFAULT for LDMA_STATUS */
#define _LDMA_STATUS_CHGRANT_SHIFT                   3                                      /**< Shift value for LDMA_CHGRANT */
#define _LDMA_STATUS_CHGRANT_MASK                    0x38UL                                 /**< Bit mask for LDMA_CHGRANT */
#define _LDMA_STATUS_CHGRANT_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LDMA_STATUS */
#define LDMA_STATUS_CHGRANT_DEFAULT                  (_LDMA_STATUS_CHGRANT_DEFAULT << 3)    /**< Shifted mode DEFAULT for LDMA_STATUS */
#define _LDMA_STATUS_CHERROR_SHIFT                   8                                      /**< Shift value for LDMA_CHERROR */
#define _LDMA_STATUS_CHERROR_MASK                    0x700UL                                /**< Bit mask for LDMA_CHERROR */
#define _LDMA_STATUS_CHERROR_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for LDMA_STATUS */
#define LDMA_STATUS_CHERROR_DEFAULT                  (_LDMA_STATUS_CHERROR_DEFAULT << 8)    /**< Shifted mode DEFAULT for LDMA_STATUS */
#define _LDMA_STATUS_FIFOLEVEL_SHIFT                 16                                     /**< Shift value for LDMA_FIFOLEVEL */
#define _LDMA_STATUS_FIFOLEVEL_MASK                  0x1F0000UL                             /**< Bit mask for LDMA_FIFOLEVEL */
#define _LDMA_STATUS_FIFOLEVEL_DEFAULT               0x00000010UL                           /**< Mode DEFAULT for LDMA_STATUS */
#define LDMA_STATUS_FIFOLEVEL_DEFAULT                (_LDMA_STATUS_FIFOLEVEL_DEFAULT << 16) /**< Shifted mode DEFAULT for LDMA_STATUS */
#define _LDMA_STATUS_CHNUM_SHIFT                     24                                     /**< Shift value for LDMA_CHNUM */
#define _LDMA_STATUS_CHNUM_MASK                      0x1F000000UL                           /**< Bit mask for LDMA_CHNUM */
#define _LDMA_STATUS_CHNUM_DEFAULT                   0x00000008UL                           /**< Mode DEFAULT for LDMA_STATUS */
#define LDMA_STATUS_CHNUM_DEFAULT                    (_LDMA_STATUS_CHNUM_DEFAULT << 24)     /**< Shifted mode DEFAULT for LDMA_STATUS */

/* Bit fields for LDMA SYNC */
#define _LDMA_SYNC_RESETVALUE                        0x00000000UL                       /**< Default value for LDMA_SYNC */
#define _LDMA_SYNC_MASK                              0x000000FFUL                       /**< Mask for LDMA_SYNC */
#define _LDMA_SYNC_SYNCTRIG_SHIFT                    0                                  /**< Shift value for LDMA_SYNCTRIG */
#define _LDMA_SYNC_SYNCTRIG_MASK                     0xFFUL                             /**< Bit mask for LDMA_SYNCTRIG */
#define _LDMA_SYNC_SYNCTRIG_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for LDMA_SYNC */
#define LDMA_SYNC_SYNCTRIG_DEFAULT                   (_LDMA_SYNC_SYNCTRIG_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_SYNC */

/* Bit fields for LDMA CHEN */
#define _LDMA_CHEN_RESETVALUE                        0x00000000UL                   /**< Default value for LDMA_CHEN */
#define _LDMA_CHEN_MASK                              0x000000FFUL                   /**< Mask for LDMA_CHEN */
#define _LDMA_CHEN_CHEN_SHIFT                        0                              /**< Shift value for LDMA_CHEN */
#define _LDMA_CHEN_CHEN_MASK                         0xFFUL                         /**< Bit mask for LDMA_CHEN */
#define _LDMA_CHEN_CHEN_DEFAULT                      0x00000000UL                   /**< Mode DEFAULT for LDMA_CHEN */
#define LDMA_CHEN_CHEN_DEFAULT                       (_LDMA_CHEN_CHEN_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_CHEN */

/* Bit fields for LDMA CHBUSY */
#define _LDMA_CHBUSY_RESETVALUE                      0x00000000UL                     /**< Default value for LDMA_CHBUSY */
#define _LDMA_CHBUSY_MASK                            0x000000FFUL                     /**< Mask for LDMA_CHBUSY */
#define _LDMA_CHBUSY_BUSY_SHIFT                      0                                /**< Shift value for LDMA_BUSY */
#define _LDMA_CHBUSY_BUSY_MASK                       0xFFUL                           /**< Bit mask for LDMA_BUSY */
#define _LDMA_CHBUSY_BUSY_DEFAULT                    0x00000000UL                     /**< Mode DEFAULT for LDMA_CHBUSY */
#define LDMA_CHBUSY_BUSY_DEFAULT                     (_LDMA_CHBUSY_BUSY_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_CHBUSY */

/* Bit fields for LDMA CHDONE */
#define _LDMA_CHDONE_RESETVALUE                      0x00000000UL                       /**< Default value for LDMA_CHDONE */
#define _LDMA_CHDONE_MASK                            0x000000FFUL                       /**< Mask for LDMA_CHDONE */
#define _LDMA_CHDONE_CHDONE_SHIFT                    0                                  /**< Shift value for LDMA_CHDONE */
#define _LDMA_CHDONE_CHDONE_MASK                     0xFFUL                             /**< Bit mask for LDMA_CHDONE */
#define _LDMA_CHDONE_CHDONE_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for LDMA_CHDONE */
#define LDMA_CHDONE_CHDONE_DEFAULT                   (_LDMA_CHDONE_CHDONE_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_CHDONE */

/* Bit fields for LDMA DBGHALT */
#define _LDMA_DBGHALT_RESETVALUE                     0x00000000UL                         /**< Default value for LDMA_DBGHALT */
#define _LDMA_DBGHALT_MASK                           0x000000FFUL                         /**< Mask for LDMA_DBGHALT */
#define _LDMA_DBGHALT_DBGHALT_SHIFT                  0                                    /**< Shift value for LDMA_DBGHALT */
#define _LDMA_DBGHALT_DBGHALT_MASK                   0xFFUL                               /**< Bit mask for LDMA_DBGHALT */
#define _LDMA_DBGHALT_DBGHALT_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for LDMA_DBGHALT */
#define LDMA_DBGHALT_DBGHALT_DEFAULT                 (_LDMA_DBGHALT_DBGHALT_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_DBGHALT */

/* Bit fields for LDMA SWREQ */
#define _LDMA_SWREQ_RESETVALUE                       0x00000000UL                     /**< Default value for LDMA_SWREQ */
#define _LDMA_SWREQ_MASK                             0x000000FFUL                     /**< Mask for LDMA_SWREQ */
#define _LDMA_SWREQ_SWREQ_SHIFT                      0                                /**< Shift value for LDMA_SWREQ */
#define _LDMA_SWREQ_SWREQ_MASK                       0xFFUL                           /**< Bit mask for LDMA_SWREQ */
#define _LDMA_SWREQ_SWREQ_DEFAULT                    0x00000000UL                     /**< Mode DEFAULT for LDMA_SWREQ */
#define LDMA_SWREQ_SWREQ_DEFAULT                     (_LDMA_SWREQ_SWREQ_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_SWREQ */

/* Bit fields for LDMA REQDIS */
#define _LDMA_REQDIS_RESETVALUE                      0x00000000UL                       /**< Default value for LDMA_REQDIS */
#define _LDMA_REQDIS_MASK                            0x000000FFUL                       /**< Mask for LDMA_REQDIS */
#define _LDMA_REQDIS_REQDIS_SHIFT                    0                                  /**< Shift value for LDMA_REQDIS */
#define _LDMA_REQDIS_REQDIS_MASK                     0xFFUL                             /**< Bit mask for LDMA_REQDIS */
#define _LDMA_REQDIS_REQDIS_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for LDMA_REQDIS */
#define LDMA_REQDIS_REQDIS_DEFAULT                   (_LDMA_REQDIS_REQDIS_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_REQDIS */

/* Bit fields for LDMA REQPEND */
#define _LDMA_REQPEND_RESETVALUE                     0x00000000UL                         /**< Default value for LDMA_REQPEND */
#define _LDMA_REQPEND_MASK                           0x000000FFUL                         /**< Mask for LDMA_REQPEND */
#define _LDMA_REQPEND_REQPEND_SHIFT                  0                                    /**< Shift value for LDMA_REQPEND */
#define _LDMA_REQPEND_REQPEND_MASK                   0xFFUL                               /**< Bit mask for LDMA_REQPEND */
#define _LDMA_REQPEND_REQPEND_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for LDMA_REQPEND */
#define LDMA_REQPEND_REQPEND_DEFAULT                 (_LDMA_REQPEND_REQPEND_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_REQPEND */

/* Bit fields for LDMA LINKLOAD */
#define _LDMA_LINKLOAD_RESETVALUE                    0x00000000UL                           /**< Default value for LDMA_LINKLOAD */
#define _LDMA_LINKLOAD_MASK                          0x000000FFUL                           /**< Mask for LDMA_LINKLOAD */
#define _LDMA_LINKLOAD_LINKLOAD_SHIFT                0                                      /**< Shift value for LDMA_LINKLOAD */
#define _LDMA_LINKLOAD_LINKLOAD_MASK                 0xFFUL                                 /**< Bit mask for LDMA_LINKLOAD */
#define _LDMA_LINKLOAD_LINKLOAD_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for LDMA_LINKLOAD */
#define LDMA_LINKLOAD_LINKLOAD_DEFAULT               (_LDMA_LINKLOAD_LINKLOAD_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_LINKLOAD */

/* Bit fields for LDMA REQCLEAR */
#define _LDMA_REQCLEAR_RESETVALUE                    0x00000000UL                           /**< Default value for LDMA_REQCLEAR */
#define _LDMA_REQCLEAR_MASK                          0x000000FFUL                           /**< Mask for LDMA_REQCLEAR */
#define _LDMA_REQCLEAR_REQCLEAR_SHIFT                0                                      /**< Shift value for LDMA_REQCLEAR */
#define _LDMA_REQCLEAR_REQCLEAR_MASK                 0xFFUL                                 /**< Bit mask for LDMA_REQCLEAR */
#define _LDMA_REQCLEAR_REQCLEAR_DEFAULT              0x00000000UL                           /**< Mode DEFAULT for LDMA_REQCLEAR */
#define LDMA_REQCLEAR_REQCLEAR_DEFAULT               (_LDMA_REQCLEAR_REQCLEAR_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_REQCLEAR */

/* Bit fields for LDMA IF */
#define _LDMA_IF_RESETVALUE                          0x00000000UL                   /**< Default value for LDMA_IF */
#define _LDMA_IF_MASK                                0x800000FFUL                   /**< Mask for LDMA_IF */
#define _LDMA_IF_DONE_SHIFT                          0                              /**< Shift value for LDMA_DONE */
#define _LDMA_IF_DONE_MASK                           0xFFUL                         /**< Bit mask for LDMA_DONE */
#define _LDMA_IF_DONE_DEFAULT                        0x00000000UL                   /**< Mode DEFAULT for LDMA_IF */
#define LDMA_IF_DONE_DEFAULT                         (_LDMA_IF_DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LDMA_IF */
#define LDMA_IF_ERROR                                (0x1UL << 31)                  /**< Transfer Error Interrupt Flag */
#define _LDMA_IF_ERROR_SHIFT                         31                             /**< Shift value for LDMA_ERROR */
#define _LDMA_IF_ERROR_MASK                          0x80000000UL                   /**< Bit mask for LDMA_ERROR */
#define _LDMA_IF_ERROR_DEFAULT                       0x00000000UL                   /**< Mode DEFAULT for LDMA_IF */
#define LDMA_IF_ERROR_DEFAULT                        (_LDMA_IF_ERROR_DEFAULT << 31) /**< Shifted mode DEFAULT for LDMA_IF */

/* Bit fields for LDMA IFS */
#define _LDMA_IFS_RESETVALUE                         0x00000000UL                    /**< Default value for LDMA_IFS */
#define _LDMA_IFS_MASK                               0x800000FFUL                    /**< Mask for LDMA_IFS */
#define _LDMA_IFS_DONE_SHIFT                         0                               /**< Shift value for LDMA_DONE */
#define _LDMA_IFS_DONE_MASK                          0xFFUL                          /**< Bit mask for LDMA_DONE */
#define _LDMA_IFS_DONE_DEFAULT                       0x00000000UL                    /**< Mode DEFAULT for LDMA_IFS */
#define LDMA_IFS_DONE_DEFAULT                        (_LDMA_IFS_DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LDMA_IFS */
#define LDMA_IFS_ERROR                               (0x1UL << 31)                   /**< Set ERROR Interrupt Flag */
#define _LDMA_IFS_ERROR_SHIFT                        31                              /**< Shift value for LDMA_ERROR */
#define _LDMA_IFS_ERROR_MASK                         0x80000000UL                    /**< Bit mask for LDMA_ERROR */
#define _LDMA_IFS_ERROR_DEFAULT                      0x00000000UL                    /**< Mode DEFAULT for LDMA_IFS */
#define LDMA_IFS_ERROR_DEFAULT                       (_LDMA_IFS_ERROR_DEFAULT << 31) /**< Shifted mode DEFAULT for LDMA_IFS */

/* Bit fields for LDMA IFC */
#define _LDMA_IFC_RESETVALUE                         0x00000000UL                    /**< Default value for LDMA_IFC */
#define _LDMA_IFC_MASK                               0x800000FFUL                    /**< Mask for LDMA_IFC */
#define _LDMA_IFC_DONE_SHIFT                         0                               /**< Shift value for LDMA_DONE */
#define _LDMA_IFC_DONE_MASK                          0xFFUL                          /**< Bit mask for LDMA_DONE */
#define _LDMA_IFC_DONE_DEFAULT                       0x00000000UL                    /**< Mode DEFAULT for LDMA_IFC */
#define LDMA_IFC_DONE_DEFAULT                        (_LDMA_IFC_DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LDMA_IFC */
#define LDMA_IFC_ERROR                               (0x1UL << 31)                   /**< Clear ERROR Interrupt Flag */
#define _LDMA_IFC_ERROR_SHIFT                        31                              /**< Shift value for LDMA_ERROR */
#define _LDMA_IFC_ERROR_MASK                         0x80000000UL                    /**< Bit mask for LDMA_ERROR */
#define _LDMA_IFC_ERROR_DEFAULT                      0x00000000UL                    /**< Mode DEFAULT for LDMA_IFC */
#define LDMA_IFC_ERROR_DEFAULT                       (_LDMA_IFC_ERROR_DEFAULT << 31) /**< Shifted mode DEFAULT for LDMA_IFC */

/* Bit fields for LDMA IEN */
#define _LDMA_IEN_RESETVALUE                         0x00000000UL                    /**< Default value for LDMA_IEN */
#define _LDMA_IEN_MASK                               0x800000FFUL                    /**< Mask for LDMA_IEN */
#define _LDMA_IEN_DONE_SHIFT                         0                               /**< Shift value for LDMA_DONE */
#define _LDMA_IEN_DONE_MASK                          0xFFUL                          /**< Bit mask for LDMA_DONE */
#define _LDMA_IEN_DONE_DEFAULT                       0x00000000UL                    /**< Mode DEFAULT for LDMA_IEN */
#define LDMA_IEN_DONE_DEFAULT                        (_LDMA_IEN_DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LDMA_IEN */
#define LDMA_IEN_ERROR                               (0x1UL << 31)                   /**< ERROR Interrupt Enable */
#define _LDMA_IEN_ERROR_SHIFT                        31                              /**< Shift value for LDMA_ERROR */
#define _LDMA_IEN_ERROR_MASK                         0x80000000UL                    /**< Bit mask for LDMA_ERROR */
#define _LDMA_IEN_ERROR_DEFAULT                      0x00000000UL                    /**< Mode DEFAULT for LDMA_IEN */
#define LDMA_IEN_ERROR_DEFAULT                       (_LDMA_IEN_ERROR_DEFAULT << 31) /**< Shifted mode DEFAULT for LDMA_IEN */

/* Bit fields for LDMA CH_REQSEL */
#define _LDMA_CH_REQSEL_RESETVALUE                   0x00000000UL                                     /**< Default value for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_MASK                         0x003F000FUL                                     /**< Mask for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_SHIFT                 0                                                /**< Shift value for LDMA_SIGSEL */
#define _LDMA_CH_REQSEL_SIGSEL_MASK                  0xFUL                                            /**< Bit mask for LDMA_SIGSEL */
#define _LDMA_CH_REQSEL_SIGSEL_PRSREQ0               0x00000000UL                                     /**< Mode PRSREQ0 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_ADC0SINGLE            0x00000000UL                                     /**< Mode ADC0SINGLE for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_USART0RXDATAV         0x00000000UL                                     /**< Mode USART0RXDATAV for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_USART1RXDATAV         0x00000000UL                                     /**< Mode USART1RXDATAV for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_LEUART0RXDATAV        0x00000000UL                                     /**< Mode LEUART0RXDATAV for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_I2C0RXDATAV           0x00000000UL                                     /**< Mode I2C0RXDATAV for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER0UFOF            0x00000000UL                                     /**< Mode TIMER0UFOF for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER1UFOF            0x00000000UL                                     /**< Mode TIMER1UFOF for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_MSCWDATA              0x00000000UL                                     /**< Mode MSCWDATA for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0WR         0x00000000UL                                     /**< Mode CRYPTODATA0WR for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_PRSREQ1               0x00000001UL                                     /**< Mode PRSREQ1 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_ADC0SCAN              0x00000001UL                                     /**< Mode ADC0SCAN for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_USART0TXBL            0x00000001UL                                     /**< Mode USART0TXBL for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_USART1TXBL            0x00000001UL                                     /**< Mode USART1TXBL for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_LEUART0TXBL           0x00000001UL                                     /**< Mode LEUART0TXBL for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_I2C0TXBL              0x00000001UL                                     /**< Mode I2C0TXBL for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER0CC0             0x00000001UL                                     /**< Mode TIMER0CC0 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER1CC0             0x00000001UL                                     /**< Mode TIMER1CC0 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0XWR        0x00000001UL                                     /**< Mode CRYPTODATA0XWR for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_USART0TXEMPTY         0x00000002UL                                     /**< Mode USART0TXEMPTY for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_USART1TXEMPTY         0x00000002UL                                     /**< Mode USART1TXEMPTY for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_LEUART0TXEMPTY        0x00000002UL                                     /**< Mode LEUART0TXEMPTY for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER0CC1             0x00000002UL                                     /**< Mode TIMER0CC1 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER1CC1             0x00000002UL                                     /**< Mode TIMER1CC1 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0RD         0x00000002UL                                     /**< Mode CRYPTODATA0RD for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT    0x00000003UL                                     /**< Mode USART1RXDATAVRIGHT for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER0CC2             0x00000003UL                                     /**< Mode TIMER0CC2 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER1CC2             0x00000003UL                                     /**< Mode TIMER1CC2 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1WR         0x00000003UL                                     /**< Mode CRYPTODATA1WR for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_USART1TXBLRIGHT       0x00000004UL                                     /**< Mode USART1TXBLRIGHT for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_TIMER1CC3             0x00000004UL                                     /**< Mode TIMER1CC3 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1RD         0x00000004UL                                     /**< Mode CRYPTODATA1RD for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_PRSREQ0                (_LDMA_CH_REQSEL_SIGSEL_PRSREQ0 << 0)            /**< Shifted mode PRSREQ0 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_ADC0SINGLE             (_LDMA_CH_REQSEL_SIGSEL_ADC0SINGLE << 0)         /**< Shifted mode ADC0SINGLE for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_USART0RXDATAV          (_LDMA_CH_REQSEL_SIGSEL_USART0RXDATAV << 0)      /**< Shifted mode USART0RXDATAV for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_USART1RXDATAV          (_LDMA_CH_REQSEL_SIGSEL_USART1RXDATAV << 0)      /**< Shifted mode USART1RXDATAV for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_LEUART0RXDATAV         (_LDMA_CH_REQSEL_SIGSEL_LEUART0RXDATAV << 0)     /**< Shifted mode LEUART0RXDATAV for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_I2C0RXDATAV            (_LDMA_CH_REQSEL_SIGSEL_I2C0RXDATAV << 0)        /**< Shifted mode I2C0RXDATAV for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER0UFOF             (_LDMA_CH_REQSEL_SIGSEL_TIMER0UFOF << 0)         /**< Shifted mode TIMER0UFOF for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER1UFOF             (_LDMA_CH_REQSEL_SIGSEL_TIMER1UFOF << 0)         /**< Shifted mode TIMER1UFOF for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_MSCWDATA               (_LDMA_CH_REQSEL_SIGSEL_MSCWDATA << 0)           /**< Shifted mode MSCWDATA for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0WR          (_LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0WR << 0)      /**< Shifted mode CRYPTODATA0WR for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_PRSREQ1                (_LDMA_CH_REQSEL_SIGSEL_PRSREQ1 << 0)            /**< Shifted mode PRSREQ1 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_ADC0SCAN               (_LDMA_CH_REQSEL_SIGSEL_ADC0SCAN << 0)           /**< Shifted mode ADC0SCAN for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_USART0TXBL             (_LDMA_CH_REQSEL_SIGSEL_USART0TXBL << 0)         /**< Shifted mode USART0TXBL for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_USART1TXBL             (_LDMA_CH_REQSEL_SIGSEL_USART1TXBL << 0)         /**< Shifted mode USART1TXBL for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_LEUART0TXBL            (_LDMA_CH_REQSEL_SIGSEL_LEUART0TXBL << 0)        /**< Shifted mode LEUART0TXBL for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_I2C0TXBL               (_LDMA_CH_REQSEL_SIGSEL_I2C0TXBL << 0)           /**< Shifted mode I2C0TXBL for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER0CC0              (_LDMA_CH_REQSEL_SIGSEL_TIMER0CC0 << 0)          /**< Shifted mode TIMER0CC0 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER1CC0              (_LDMA_CH_REQSEL_SIGSEL_TIMER1CC0 << 0)          /**< Shifted mode TIMER1CC0 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0XWR         (_LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0XWR << 0)     /**< Shifted mode CRYPTODATA0XWR for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_USART0TXEMPTY          (_LDMA_CH_REQSEL_SIGSEL_USART0TXEMPTY << 0)      /**< Shifted mode USART0TXEMPTY for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_USART1TXEMPTY          (_LDMA_CH_REQSEL_SIGSEL_USART1TXEMPTY << 0)      /**< Shifted mode USART1TXEMPTY for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_LEUART0TXEMPTY         (_LDMA_CH_REQSEL_SIGSEL_LEUART0TXEMPTY << 0)     /**< Shifted mode LEUART0TXEMPTY for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER0CC1              (_LDMA_CH_REQSEL_SIGSEL_TIMER0CC1 << 0)          /**< Shifted mode TIMER0CC1 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER1CC1              (_LDMA_CH_REQSEL_SIGSEL_TIMER1CC1 << 0)          /**< Shifted mode TIMER1CC1 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0RD          (_LDMA_CH_REQSEL_SIGSEL_CRYPTODATA0RD << 0)      /**< Shifted mode CRYPTODATA0RD for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT     (_LDMA_CH_REQSEL_SIGSEL_USART1RXDATAVRIGHT << 0) /**< Shifted mode USART1RXDATAVRIGHT for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER0CC2              (_LDMA_CH_REQSEL_SIGSEL_TIMER0CC2 << 0)          /**< Shifted mode TIMER0CC2 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER1CC2              (_LDMA_CH_REQSEL_SIGSEL_TIMER1CC2 << 0)          /**< Shifted mode TIMER1CC2 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1WR          (_LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1WR << 0)      /**< Shifted mode CRYPTODATA1WR for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_USART1TXBLRIGHT        (_LDMA_CH_REQSEL_SIGSEL_USART1TXBLRIGHT << 0)    /**< Shifted mode USART1TXBLRIGHT for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_TIMER1CC3              (_LDMA_CH_REQSEL_SIGSEL_TIMER1CC3 << 0)          /**< Shifted mode TIMER1CC3 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1RD          (_LDMA_CH_REQSEL_SIGSEL_CRYPTODATA1RD << 0)      /**< Shifted mode CRYPTODATA1RD for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_SHIFT              16                                               /**< Shift value for LDMA_SOURCESEL */
#define _LDMA_CH_REQSEL_SOURCESEL_MASK               0x3F0000UL                                       /**< Bit mask for LDMA_SOURCESEL */
#define _LDMA_CH_REQSEL_SOURCESEL_NONE               0x00000000UL                                     /**< Mode NONE for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_PRS                0x00000001UL                                     /**< Mode PRS for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_ADC0               0x00000008UL                                     /**< Mode ADC0 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_USART0             0x0000000CUL                                     /**< Mode USART0 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_USART1             0x0000000DUL                                     /**< Mode USART1 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_LEUART0            0x00000010UL                                     /**< Mode LEUART0 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_I2C0               0x00000014UL                                     /**< Mode I2C0 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_TIMER0             0x00000018UL                                     /**< Mode TIMER0 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_TIMER1             0x00000019UL                                     /**< Mode TIMER1 for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_MSC                0x00000030UL                                     /**< Mode MSC for LDMA_CH_REQSEL */
#define _LDMA_CH_REQSEL_SOURCESEL_CRYPTO             0x00000031UL                                     /**< Mode CRYPTO for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_NONE                (_LDMA_CH_REQSEL_SOURCESEL_NONE << 16)           /**< Shifted mode NONE for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_PRS                 (_LDMA_CH_REQSEL_SOURCESEL_PRS << 16)            /**< Shifted mode PRS for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_ADC0                (_LDMA_CH_REQSEL_SOURCESEL_ADC0 << 16)           /**< Shifted mode ADC0 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_USART0              (_LDMA_CH_REQSEL_SOURCESEL_USART0 << 16)         /**< Shifted mode USART0 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_USART1              (_LDMA_CH_REQSEL_SOURCESEL_USART1 << 16)         /**< Shifted mode USART1 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_LEUART0             (_LDMA_CH_REQSEL_SOURCESEL_LEUART0 << 16)        /**< Shifted mode LEUART0 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_I2C0                (_LDMA_CH_REQSEL_SOURCESEL_I2C0 << 16)           /**< Shifted mode I2C0 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_TIMER0              (_LDMA_CH_REQSEL_SOURCESEL_TIMER0 << 16)         /**< Shifted mode TIMER0 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_TIMER1              (_LDMA_CH_REQSEL_SOURCESEL_TIMER1 << 16)         /**< Shifted mode TIMER1 for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_MSC                 (_LDMA_CH_REQSEL_SOURCESEL_MSC << 16)            /**< Shifted mode MSC for LDMA_CH_REQSEL */
#define LDMA_CH_REQSEL_SOURCESEL_CRYPTO              (_LDMA_CH_REQSEL_SOURCESEL_CRYPTO << 16)         /**< Shifted mode CRYPTO for LDMA_CH_REQSEL */

/* Bit fields for LDMA CH_CFG */
#define _LDMA_CH_CFG_RESETVALUE                      0x00000000UL                             /**< Default value for LDMA_CH_CFG */
#define _LDMA_CH_CFG_MASK                            0x00330000UL                             /**< Mask for LDMA_CH_CFG */
#define _LDMA_CH_CFG_ARBSLOTS_SHIFT                  16                                       /**< Shift value for LDMA_ARBSLOTS */
#define _LDMA_CH_CFG_ARBSLOTS_MASK                   0x30000UL                                /**< Bit mask for LDMA_ARBSLOTS */
#define _LDMA_CH_CFG_ARBSLOTS_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for LDMA_CH_CFG */
#define _LDMA_CH_CFG_ARBSLOTS_ONE                    0x00000000UL                             /**< Mode ONE for LDMA_CH_CFG */
#define _LDMA_CH_CFG_ARBSLOTS_TWO                    0x00000001UL                             /**< Mode TWO for LDMA_CH_CFG */
#define _LDMA_CH_CFG_ARBSLOTS_FOUR                   0x00000002UL                             /**< Mode FOUR for LDMA_CH_CFG */
#define _LDMA_CH_CFG_ARBSLOTS_EIGHT                  0x00000003UL                             /**< Mode EIGHT for LDMA_CH_CFG */
#define LDMA_CH_CFG_ARBSLOTS_DEFAULT                 (_LDMA_CH_CFG_ARBSLOTS_DEFAULT << 16)    /**< Shifted mode DEFAULT for LDMA_CH_CFG */
#define LDMA_CH_CFG_ARBSLOTS_ONE                     (_LDMA_CH_CFG_ARBSLOTS_ONE << 16)        /**< Shifted mode ONE for LDMA_CH_CFG */
#define LDMA_CH_CFG_ARBSLOTS_TWO                     (_LDMA_CH_CFG_ARBSLOTS_TWO << 16)        /**< Shifted mode TWO for LDMA_CH_CFG */
#define LDMA_CH_CFG_ARBSLOTS_FOUR                    (_LDMA_CH_CFG_ARBSLOTS_FOUR << 16)       /**< Shifted mode FOUR for LDMA_CH_CFG */
#define LDMA_CH_CFG_ARBSLOTS_EIGHT                   (_LDMA_CH_CFG_ARBSLOTS_EIGHT << 16)      /**< Shifted mode EIGHT for LDMA_CH_CFG */
#define LDMA_CH_CFG_SRCINCSIGN                       (0x1UL << 20)                            /**< Source Address Increment Sign */
#define _LDMA_CH_CFG_SRCINCSIGN_SHIFT                20                                       /**< Shift value for LDMA_SRCINCSIGN */
#define _LDMA_CH_CFG_SRCINCSIGN_MASK                 0x100000UL                               /**< Bit mask for LDMA_SRCINCSIGN */
#define _LDMA_CH_CFG_SRCINCSIGN_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for LDMA_CH_CFG */
#define _LDMA_CH_CFG_SRCINCSIGN_POSITIVE             0x00000000UL                             /**< Mode POSITIVE for LDMA_CH_CFG */
#define _LDMA_CH_CFG_SRCINCSIGN_NEGATIVE             0x00000001UL                             /**< Mode NEGATIVE for LDMA_CH_CFG */
#define LDMA_CH_CFG_SRCINCSIGN_DEFAULT               (_LDMA_CH_CFG_SRCINCSIGN_DEFAULT << 20)  /**< Shifted mode DEFAULT for LDMA_CH_CFG */
#define LDMA_CH_CFG_SRCINCSIGN_POSITIVE              (_LDMA_CH_CFG_SRCINCSIGN_POSITIVE << 20) /**< Shifted mode POSITIVE for LDMA_CH_CFG */
#define LDMA_CH_CFG_SRCINCSIGN_NEGATIVE              (_LDMA_CH_CFG_SRCINCSIGN_NEGATIVE << 20) /**< Shifted mode NEGATIVE for LDMA_CH_CFG */
#define LDMA_CH_CFG_DSTINCSIGN                       (0x1UL << 21)                            /**< Destination Address Increment Sign */
#define _LDMA_CH_CFG_DSTINCSIGN_SHIFT                21                                       /**< Shift value for LDMA_DSTINCSIGN */
#define _LDMA_CH_CFG_DSTINCSIGN_MASK                 0x200000UL                               /**< Bit mask for LDMA_DSTINCSIGN */
#define _LDMA_CH_CFG_DSTINCSIGN_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for LDMA_CH_CFG */
#define _LDMA_CH_CFG_DSTINCSIGN_POSITIVE             0x00000000UL                             /**< Mode POSITIVE for LDMA_CH_CFG */
#define _LDMA_CH_CFG_DSTINCSIGN_NEGATIVE             0x00000001UL                             /**< Mode NEGATIVE for LDMA_CH_CFG */
#define LDMA_CH_CFG_DSTINCSIGN_DEFAULT               (_LDMA_CH_CFG_DSTINCSIGN_DEFAULT << 21)  /**< Shifted mode DEFAULT for LDMA_CH_CFG */
#define LDMA_CH_CFG_DSTINCSIGN_POSITIVE              (_LDMA_CH_CFG_DSTINCSIGN_POSITIVE << 21) /**< Shifted mode POSITIVE for LDMA_CH_CFG */
#define LDMA_CH_CFG_DSTINCSIGN_NEGATIVE              (_LDMA_CH_CFG_DSTINCSIGN_NEGATIVE << 21) /**< Shifted mode NEGATIVE for LDMA_CH_CFG */

/* Bit fields for LDMA CH_LOOP */
#define _LDMA_CH_LOOP_RESETVALUE                     0x00000000UL                         /**< Default value for LDMA_CH_LOOP */
#define _LDMA_CH_LOOP_MASK                           0x000000FFUL                         /**< Mask for LDMA_CH_LOOP */
#define _LDMA_CH_LOOP_LOOPCNT_SHIFT                  0                                    /**< Shift value for LDMA_LOOPCNT */
#define _LDMA_CH_LOOP_LOOPCNT_MASK                   0xFFUL                               /**< Bit mask for LDMA_LOOPCNT */
#define _LDMA_CH_LOOP_LOOPCNT_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for LDMA_CH_LOOP */
#define LDMA_CH_LOOP_LOOPCNT_DEFAULT                 (_LDMA_CH_LOOP_LOOPCNT_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_CH_LOOP */

/* Bit fields for LDMA CH_CTRL */
#define _LDMA_CH_CTRL_RESETVALUE                     0x00000000UL                                /**< Default value for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_MASK                           0xFFFFFFFBUL                                /**< Mask for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_STRUCTTYPE_SHIFT               0                                           /**< Shift value for LDMA_STRUCTTYPE */
#define _LDMA_CH_CTRL_STRUCTTYPE_MASK                0x3UL                                       /**< Bit mask for LDMA_STRUCTTYPE */
#define _LDMA_CH_CTRL_STRUCTTYPE_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_STRUCTTYPE_TRANSFER            0x00000000UL                                /**< Mode TRANSFER for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_STRUCTTYPE_SYNCHRONIZE         0x00000001UL                                /**< Mode SYNCHRONIZE for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_STRUCTTYPE_WRITE               0x00000002UL                                /**< Mode WRITE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_STRUCTTYPE_DEFAULT              (_LDMA_CH_CTRL_STRUCTTYPE_DEFAULT << 0)     /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_STRUCTTYPE_TRANSFER             (_LDMA_CH_CTRL_STRUCTTYPE_TRANSFER << 0)    /**< Shifted mode TRANSFER for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_STRUCTTYPE_SYNCHRONIZE          (_LDMA_CH_CTRL_STRUCTTYPE_SYNCHRONIZE << 0) /**< Shifted mode SYNCHRONIZE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_STRUCTTYPE_WRITE                (_LDMA_CH_CTRL_STRUCTTYPE_WRITE << 0)       /**< Shifted mode WRITE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_STRUCTREQ                       (0x1UL << 3)                                /**< Structure DMA Transfer Request */
#define _LDMA_CH_CTRL_STRUCTREQ_SHIFT                3                                           /**< Shift value for LDMA_STRUCTREQ */
#define _LDMA_CH_CTRL_STRUCTREQ_MASK                 0x8UL                                       /**< Bit mask for LDMA_STRUCTREQ */
#define _LDMA_CH_CTRL_STRUCTREQ_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_STRUCTREQ_DEFAULT               (_LDMA_CH_CTRL_STRUCTREQ_DEFAULT << 3)      /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_XFERCNT_SHIFT                  4                                           /**< Shift value for LDMA_XFERCNT */
#define _LDMA_CH_CTRL_XFERCNT_MASK                   0x7FF0UL                                    /**< Bit mask for LDMA_XFERCNT */
#define _LDMA_CH_CTRL_XFERCNT_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_XFERCNT_DEFAULT                 (_LDMA_CH_CTRL_XFERCNT_DEFAULT << 4)        /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BYTESWAP                        (0x1UL << 15)                               /**< Endian Byte Swap */
#define _LDMA_CH_CTRL_BYTESWAP_SHIFT                 15                                          /**< Shift value for LDMA_BYTESWAP */
#define _LDMA_CH_CTRL_BYTESWAP_MASK                  0x8000UL                                    /**< Bit mask for LDMA_BYTESWAP */
#define _LDMA_CH_CTRL_BYTESWAP_DEFAULT               0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BYTESWAP_DEFAULT                (_LDMA_CH_CTRL_BYTESWAP_DEFAULT << 15)      /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_SHIFT                16                                          /**< Shift value for LDMA_BLOCKSIZE */
#define _LDMA_CH_CTRL_BLOCKSIZE_MASK                 0xF0000UL                                   /**< Bit mask for LDMA_BLOCKSIZE */
#define _LDMA_CH_CTRL_BLOCKSIZE_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT1                0x00000000UL                                /**< Mode UNIT1 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT2                0x00000001UL                                /**< Mode UNIT2 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT3                0x00000002UL                                /**< Mode UNIT3 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT4                0x00000003UL                                /**< Mode UNIT4 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT6                0x00000004UL                                /**< Mode UNIT6 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT8                0x00000005UL                                /**< Mode UNIT8 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT16               0x00000007UL                                /**< Mode UNIT16 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT32               0x00000009UL                                /**< Mode UNIT32 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT64               0x0000000AUL                                /**< Mode UNIT64 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT128              0x0000000BUL                                /**< Mode UNIT128 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT256              0x0000000CUL                                /**< Mode UNIT256 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT512              0x0000000DUL                                /**< Mode UNIT512 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_UNIT1024             0x0000000EUL                                /**< Mode UNIT1024 for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_BLOCKSIZE_ALL                  0x0000000FUL                                /**< Mode ALL for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_DEFAULT               (_LDMA_CH_CTRL_BLOCKSIZE_DEFAULT << 16)     /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT1                 (_LDMA_CH_CTRL_BLOCKSIZE_UNIT1 << 16)       /**< Shifted mode UNIT1 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT2                 (_LDMA_CH_CTRL_BLOCKSIZE_UNIT2 << 16)       /**< Shifted mode UNIT2 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT3                 (_LDMA_CH_CTRL_BLOCKSIZE_UNIT3 << 16)       /**< Shifted mode UNIT3 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT4                 (_LDMA_CH_CTRL_BLOCKSIZE_UNIT4 << 16)       /**< Shifted mode UNIT4 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT6                 (_LDMA_CH_CTRL_BLOCKSIZE_UNIT6 << 16)       /**< Shifted mode UNIT6 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT8                 (_LDMA_CH_CTRL_BLOCKSIZE_UNIT8 << 16)       /**< Shifted mode UNIT8 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT16                (_LDMA_CH_CTRL_BLOCKSIZE_UNIT16 << 16)      /**< Shifted mode UNIT16 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT32                (_LDMA_CH_CTRL_BLOCKSIZE_UNIT32 << 16)      /**< Shifted mode UNIT32 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT64                (_LDMA_CH_CTRL_BLOCKSIZE_UNIT64 << 16)      /**< Shifted mode UNIT64 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT128               (_LDMA_CH_CTRL_BLOCKSIZE_UNIT128 << 16)     /**< Shifted mode UNIT128 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT256               (_LDMA_CH_CTRL_BLOCKSIZE_UNIT256 << 16)     /**< Shifted mode UNIT256 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT512               (_LDMA_CH_CTRL_BLOCKSIZE_UNIT512 << 16)     /**< Shifted mode UNIT512 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_UNIT1024              (_LDMA_CH_CTRL_BLOCKSIZE_UNIT1024 << 16)    /**< Shifted mode UNIT1024 for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_BLOCKSIZE_ALL                   (_LDMA_CH_CTRL_BLOCKSIZE_ALL << 16)         /**< Shifted mode ALL for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DONEIFSEN                       (0x1UL << 20)                               /**< DMA Operation Done Interrupt Flag Set Enable */
#define _LDMA_CH_CTRL_DONEIFSEN_SHIFT                20                                          /**< Shift value for LDMA_DONEIFSEN */
#define _LDMA_CH_CTRL_DONEIFSEN_MASK                 0x100000UL                                  /**< Bit mask for LDMA_DONEIFSEN */
#define _LDMA_CH_CTRL_DONEIFSEN_DEFAULT              0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DONEIFSEN_DEFAULT               (_LDMA_CH_CTRL_DONEIFSEN_DEFAULT << 20)     /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_REQMODE                         (0x1UL << 21)                               /**< DMA Request Transfer Mode Select */
#define _LDMA_CH_CTRL_REQMODE_SHIFT                  21                                          /**< Shift value for LDMA_REQMODE */
#define _LDMA_CH_CTRL_REQMODE_MASK                   0x200000UL                                  /**< Bit mask for LDMA_REQMODE */
#define _LDMA_CH_CTRL_REQMODE_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_REQMODE_BLOCK                  0x00000000UL                                /**< Mode BLOCK for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_REQMODE_ALL                    0x00000001UL                                /**< Mode ALL for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_REQMODE_DEFAULT                 (_LDMA_CH_CTRL_REQMODE_DEFAULT << 21)       /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_REQMODE_BLOCK                   (_LDMA_CH_CTRL_REQMODE_BLOCK << 21)         /**< Shifted mode BLOCK for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_REQMODE_ALL                     (_LDMA_CH_CTRL_REQMODE_ALL << 21)           /**< Shifted mode ALL for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DECLOOPCNT                      (0x1UL << 22)                               /**< Decrement Loop Count */
#define _LDMA_CH_CTRL_DECLOOPCNT_SHIFT               22                                          /**< Shift value for LDMA_DECLOOPCNT */
#define _LDMA_CH_CTRL_DECLOOPCNT_MASK                0x400000UL                                  /**< Bit mask for LDMA_DECLOOPCNT */
#define _LDMA_CH_CTRL_DECLOOPCNT_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DECLOOPCNT_DEFAULT              (_LDMA_CH_CTRL_DECLOOPCNT_DEFAULT << 22)    /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_IGNORESREQ                      (0x1UL << 23)                               /**< Ignore Sreq */
#define _LDMA_CH_CTRL_IGNORESREQ_SHIFT               23                                          /**< Shift value for LDMA_IGNORESREQ */
#define _LDMA_CH_CTRL_IGNORESREQ_MASK                0x800000UL                                  /**< Bit mask for LDMA_IGNORESREQ */
#define _LDMA_CH_CTRL_IGNORESREQ_DEFAULT             0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_IGNORESREQ_DEFAULT              (_LDMA_CH_CTRL_IGNORESREQ_DEFAULT << 23)    /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SRCINC_SHIFT                   24                                          /**< Shift value for LDMA_SRCINC */
#define _LDMA_CH_CTRL_SRCINC_MASK                    0x3000000UL                                 /**< Bit mask for LDMA_SRCINC */
#define _LDMA_CH_CTRL_SRCINC_DEFAULT                 0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SRCINC_ONE                     0x00000000UL                                /**< Mode ONE for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SRCINC_TWO                     0x00000001UL                                /**< Mode TWO for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SRCINC_FOUR                    0x00000002UL                                /**< Mode FOUR for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SRCINC_NONE                    0x00000003UL                                /**< Mode NONE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCINC_DEFAULT                  (_LDMA_CH_CTRL_SRCINC_DEFAULT << 24)        /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCINC_ONE                      (_LDMA_CH_CTRL_SRCINC_ONE << 24)            /**< Shifted mode ONE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCINC_TWO                      (_LDMA_CH_CTRL_SRCINC_TWO << 24)            /**< Shifted mode TWO for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCINC_FOUR                     (_LDMA_CH_CTRL_SRCINC_FOUR << 24)           /**< Shifted mode FOUR for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCINC_NONE                     (_LDMA_CH_CTRL_SRCINC_NONE << 24)           /**< Shifted mode NONE for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SIZE_SHIFT                     26                                          /**< Shift value for LDMA_SIZE */
#define _LDMA_CH_CTRL_SIZE_MASK                      0xC000000UL                                 /**< Bit mask for LDMA_SIZE */
#define _LDMA_CH_CTRL_SIZE_DEFAULT                   0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SIZE_BYTE                      0x00000000UL                                /**< Mode BYTE for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SIZE_HALFWORD                  0x00000001UL                                /**< Mode HALFWORD for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SIZE_WORD                      0x00000002UL                                /**< Mode WORD for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SIZE_DEFAULT                    (_LDMA_CH_CTRL_SIZE_DEFAULT << 26)          /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SIZE_BYTE                       (_LDMA_CH_CTRL_SIZE_BYTE << 26)             /**< Shifted mode BYTE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SIZE_HALFWORD                   (_LDMA_CH_CTRL_SIZE_HALFWORD << 26)         /**< Shifted mode HALFWORD for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SIZE_WORD                       (_LDMA_CH_CTRL_SIZE_WORD << 26)             /**< Shifted mode WORD for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_DSTINC_SHIFT                   28                                          /**< Shift value for LDMA_DSTINC */
#define _LDMA_CH_CTRL_DSTINC_MASK                    0x30000000UL                                /**< Bit mask for LDMA_DSTINC */
#define _LDMA_CH_CTRL_DSTINC_DEFAULT                 0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_DSTINC_ONE                     0x00000000UL                                /**< Mode ONE for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_DSTINC_TWO                     0x00000001UL                                /**< Mode TWO for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_DSTINC_FOUR                    0x00000002UL                                /**< Mode FOUR for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_DSTINC_NONE                    0x00000003UL                                /**< Mode NONE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTINC_DEFAULT                  (_LDMA_CH_CTRL_DSTINC_DEFAULT << 28)        /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTINC_ONE                      (_LDMA_CH_CTRL_DSTINC_ONE << 28)            /**< Shifted mode ONE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTINC_TWO                      (_LDMA_CH_CTRL_DSTINC_TWO << 28)            /**< Shifted mode TWO for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTINC_FOUR                     (_LDMA_CH_CTRL_DSTINC_FOUR << 28)           /**< Shifted mode FOUR for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTINC_NONE                     (_LDMA_CH_CTRL_DSTINC_NONE << 28)           /**< Shifted mode NONE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCMODE                         (0x1UL << 30)                               /**< Source Addressing Mode */
#define _LDMA_CH_CTRL_SRCMODE_SHIFT                  30                                          /**< Shift value for LDMA_SRCMODE */
#define _LDMA_CH_CTRL_SRCMODE_MASK                   0x40000000UL                                /**< Bit mask for LDMA_SRCMODE */
#define _LDMA_CH_CTRL_SRCMODE_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SRCMODE_ABSOLUTE               0x00000000UL                                /**< Mode ABSOLUTE for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_SRCMODE_RELATIVE               0x00000001UL                                /**< Mode RELATIVE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCMODE_DEFAULT                 (_LDMA_CH_CTRL_SRCMODE_DEFAULT << 30)       /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCMODE_ABSOLUTE                (_LDMA_CH_CTRL_SRCMODE_ABSOLUTE << 30)      /**< Shifted mode ABSOLUTE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_SRCMODE_RELATIVE                (_LDMA_CH_CTRL_SRCMODE_RELATIVE << 30)      /**< Shifted mode RELATIVE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTMODE                         (0x1UL << 31)                               /**< Destination Addressing Mode */
#define _LDMA_CH_CTRL_DSTMODE_SHIFT                  31                                          /**< Shift value for LDMA_DSTMODE */
#define _LDMA_CH_CTRL_DSTMODE_MASK                   0x80000000UL                                /**< Bit mask for LDMA_DSTMODE */
#define _LDMA_CH_CTRL_DSTMODE_DEFAULT                0x00000000UL                                /**< Mode DEFAULT for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_DSTMODE_ABSOLUTE               0x00000000UL                                /**< Mode ABSOLUTE for LDMA_CH_CTRL */
#define _LDMA_CH_CTRL_DSTMODE_RELATIVE               0x00000001UL                                /**< Mode RELATIVE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTMODE_DEFAULT                 (_LDMA_CH_CTRL_DSTMODE_DEFAULT << 31)       /**< Shifted mode DEFAULT for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTMODE_ABSOLUTE                (_LDMA_CH_CTRL_DSTMODE_ABSOLUTE << 31)      /**< Shifted mode ABSOLUTE for LDMA_CH_CTRL */
#define LDMA_CH_CTRL_DSTMODE_RELATIVE                (_LDMA_CH_CTRL_DSTMODE_RELATIVE << 31)      /**< Shifted mode RELATIVE for LDMA_CH_CTRL */

/* Bit fields for LDMA CH_SRC */
#define _LDMA_CH_SRC_RESETVALUE                      0x00000000UL                        /**< Default value for LDMA_CH_SRC */
#define _LDMA_CH_SRC_MASK                            0xFFFFFFFFUL                        /**< Mask for LDMA_CH_SRC */
#define _LDMA_CH_SRC_SRCADDR_SHIFT                   0                                   /**< Shift value for LDMA_SRCADDR */
#define _LDMA_CH_SRC_SRCADDR_MASK                    0xFFFFFFFFUL                        /**< Bit mask for LDMA_SRCADDR */
#define _LDMA_CH_SRC_SRCADDR_DEFAULT                 0x00000000UL                        /**< Mode DEFAULT for LDMA_CH_SRC */
#define LDMA_CH_SRC_SRCADDR_DEFAULT                  (_LDMA_CH_SRC_SRCADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_CH_SRC */

/* Bit fields for LDMA CH_DST */
#define _LDMA_CH_DST_RESETVALUE                      0x00000000UL                        /**< Default value for LDMA_CH_DST */
#define _LDMA_CH_DST_MASK                            0xFFFFFFFFUL                        /**< Mask for LDMA_CH_DST */
#define _LDMA_CH_DST_DSTADDR_SHIFT                   0                                   /**< Shift value for LDMA_DSTADDR */
#define _LDMA_CH_DST_DSTADDR_MASK                    0xFFFFFFFFUL                        /**< Bit mask for LDMA_DSTADDR */
#define _LDMA_CH_DST_DSTADDR_DEFAULT                 0x00000000UL                        /**< Mode DEFAULT for LDMA_CH_DST */
#define LDMA_CH_DST_DSTADDR_DEFAULT                  (_LDMA_CH_DST_DSTADDR_DEFAULT << 0) /**< Shifted mode DEFAULT for LDMA_CH_DST */

/* Bit fields for LDMA CH_LINK */
#define _LDMA_CH_LINK_RESETVALUE                     0x00000000UL                           /**< Default value for LDMA_CH_LINK */
#define _LDMA_CH_LINK_MASK                           0xFFFFFFFFUL                           /**< Mask for LDMA_CH_LINK */
#define LDMA_CH_LINK_LINKMODE                        (0x1UL << 0)                           /**< Link Structure Addressing Mode */
#define _LDMA_CH_LINK_LINKMODE_SHIFT                 0                                      /**< Shift value for LDMA_LINKMODE */
#define _LDMA_CH_LINK_LINKMODE_MASK                  0x1UL                                  /**< Bit mask for LDMA_LINKMODE */
#define _LDMA_CH_LINK_LINKMODE_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for LDMA_CH_LINK */
#define _LDMA_CH_LINK_LINKMODE_ABSOLUTE              0x00000000UL                           /**< Mode ABSOLUTE for LDMA_CH_LINK */
#define _LDMA_CH_LINK_LINKMODE_RELATIVE              0x00000001UL                           /**< Mode RELATIVE for LDMA_CH_LINK */
#define LDMA_CH_LINK_LINKMODE_DEFAULT                (_LDMA_CH_LINK_LINKMODE_DEFAULT << 0)  /**< Shifted mode DEFAULT for LDMA_CH_LINK */
#define LDMA_CH_LINK_LINKMODE_ABSOLUTE               (_LDMA_CH_LINK_LINKMODE_ABSOLUTE << 0) /**< Shifted mode ABSOLUTE for LDMA_CH_LINK */
#define LDMA_CH_LINK_LINKMODE_RELATIVE               (_LDMA_CH_LINK_LINKMODE_RELATIVE << 0) /**< Shifted mode RELATIVE for LDMA_CH_LINK */
#define LDMA_CH_LINK_LINK                            (0x1UL << 1)                           /**< Link Next Structure */
#define _LDMA_CH_LINK_LINK_SHIFT                     1                                      /**< Shift value for LDMA_LINK */
#define _LDMA_CH_LINK_LINK_MASK                      0x2UL                                  /**< Bit mask for LDMA_LINK */
#define _LDMA_CH_LINK_LINK_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for LDMA_CH_LINK */
#define LDMA_CH_LINK_LINK_DEFAULT                    (_LDMA_CH_LINK_LINK_DEFAULT << 1)      /**< Shifted mode DEFAULT for LDMA_CH_LINK */
#define _LDMA_CH_LINK_LINKADDR_SHIFT                 2                                      /**< Shift value for LDMA_LINKADDR */
#define _LDMA_CH_LINK_LINKADDR_MASK                  0xFFFFFFFCUL                           /**< Bit mask for LDMA_LINKADDR */
#define _LDMA_CH_LINK_LINKADDR_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for LDMA_CH_LINK */
#define LDMA_CH_LINK_LINKADDR_DEFAULT                (_LDMA_CH_LINK_LINKADDR_DEFAULT << 2)  /**< Shifted mode DEFAULT for LDMA_CH_LINK */

/** @} End of group EFR32FG1P_LDMA */
/** @} End of group Parts */

