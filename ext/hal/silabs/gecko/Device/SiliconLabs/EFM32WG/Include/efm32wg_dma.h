/**************************************************************************//**
 * @file efm32wg_dma.h
 * @brief EFM32WG_DMA register and bit field definitions
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
 * @defgroup EFM32WG_DMA
 * @{
 * @brief EFM32WG_DMA Register Declaration
 *****************************************************************************/
typedef struct
{
  __IM uint32_t  STATUS;         /**< DMA Status Registers  */
  __OM uint32_t  CONFIG;         /**< DMA Configuration Register  */
  __IOM uint32_t CTRLBASE;       /**< Channel Control Data Base Pointer Register  */
  __IM uint32_t  ALTCTRLBASE;    /**< Channel Alternate Control Data Base Pointer Register  */
  __IM uint32_t  CHWAITSTATUS;   /**< Channel Wait on Request Status Register  */
  __OM uint32_t  CHSWREQ;        /**< Channel Software Request Register  */
  __IOM uint32_t CHUSEBURSTS;    /**< Channel Useburst Set Register  */
  __OM uint32_t  CHUSEBURSTC;    /**< Channel Useburst Clear Register  */
  __IOM uint32_t CHREQMASKS;     /**< Channel Request Mask Set Register  */
  __OM uint32_t  CHREQMASKC;     /**< Channel Request Mask Clear Register  */
  __IOM uint32_t CHENS;          /**< Channel Enable Set Register  */
  __OM uint32_t  CHENC;          /**< Channel Enable Clear Register  */
  __IOM uint32_t CHALTS;         /**< Channel Alternate Set Register  */
  __OM uint32_t  CHALTC;         /**< Channel Alternate Clear Register  */
  __IOM uint32_t CHPRIS;         /**< Channel Priority Set Register  */
  __OM uint32_t  CHPRIC;         /**< Channel Priority Clear Register  */
  uint32_t       RESERVED0[3];   /**< Reserved for future use **/
  __IOM uint32_t ERRORC;         /**< Bus Error Clear Register  */

  uint32_t       RESERVED1[880]; /**< Reserved for future use **/
  __IM uint32_t  CHREQSTATUS;    /**< Channel Request Status  */
  uint32_t       RESERVED2[1];   /**< Reserved for future use **/
  __IM uint32_t  CHSREQSTATUS;   /**< Channel Single Request Status  */

  uint32_t       RESERVED3[121]; /**< Reserved for future use **/
  __IM uint32_t  IF;             /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;            /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;            /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;            /**< Interrupt Enable register  */
  __IOM uint32_t CTRL;           /**< DMA Control Register  */
  __IOM uint32_t RDS;            /**< DMA Retain Descriptor State  */

  uint32_t       RESERVED4[2];   /**< Reserved for future use **/
  __IOM uint32_t LOOP0;          /**< Channel 0 Loop Register  */
  __IOM uint32_t LOOP1;          /**< Channel 1 Loop Register  */
  uint32_t       RESERVED5[14];  /**< Reserved for future use **/
  __IOM uint32_t RECT0;          /**< Channel 0 Rectangle Register  */

  uint32_t       RESERVED6[39];  /**< Reserved registers */
  DMA_CH_TypeDef CH[12];         /**< Channel registers */
} DMA_TypeDef;                   /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_DMA_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for DMA STATUS */
#define _DMA_STATUS_RESETVALUE                          0x100B0000UL                          /**< Default value for DMA_STATUS */
#define _DMA_STATUS_MASK                                0x001F00F1UL                          /**< Mask for DMA_STATUS */
#define DMA_STATUS_EN                                   (0x1UL << 0)                          /**< DMA Enable Status */
#define _DMA_STATUS_EN_SHIFT                            0                                     /**< Shift value for DMA_EN */
#define _DMA_STATUS_EN_MASK                             0x1UL                                 /**< Bit mask for DMA_EN */
#define _DMA_STATUS_EN_DEFAULT                          0x00000000UL                          /**< Mode DEFAULT for DMA_STATUS */
#define DMA_STATUS_EN_DEFAULT                           (_DMA_STATUS_EN_DEFAULT << 0)         /**< Shifted mode DEFAULT for DMA_STATUS */
#define _DMA_STATUS_STATE_SHIFT                         4                                     /**< Shift value for DMA_STATE */
#define _DMA_STATUS_STATE_MASK                          0xF0UL                                /**< Bit mask for DMA_STATE */
#define _DMA_STATUS_STATE_DEFAULT                       0x00000000UL                          /**< Mode DEFAULT for DMA_STATUS */
#define _DMA_STATUS_STATE_IDLE                          0x00000000UL                          /**< Mode IDLE for DMA_STATUS */
#define _DMA_STATUS_STATE_RDCHCTRLDATA                  0x00000001UL                          /**< Mode RDCHCTRLDATA for DMA_STATUS */
#define _DMA_STATUS_STATE_RDSRCENDPTR                   0x00000002UL                          /**< Mode RDSRCENDPTR for DMA_STATUS */
#define _DMA_STATUS_STATE_RDDSTENDPTR                   0x00000003UL                          /**< Mode RDDSTENDPTR for DMA_STATUS */
#define _DMA_STATUS_STATE_RDSRCDATA                     0x00000004UL                          /**< Mode RDSRCDATA for DMA_STATUS */
#define _DMA_STATUS_STATE_WRDSTDATA                     0x00000005UL                          /**< Mode WRDSTDATA for DMA_STATUS */
#define _DMA_STATUS_STATE_WAITREQCLR                    0x00000006UL                          /**< Mode WAITREQCLR for DMA_STATUS */
#define _DMA_STATUS_STATE_WRCHCTRLDATA                  0x00000007UL                          /**< Mode WRCHCTRLDATA for DMA_STATUS */
#define _DMA_STATUS_STATE_STALLED                       0x00000008UL                          /**< Mode STALLED for DMA_STATUS */
#define _DMA_STATUS_STATE_DONE                          0x00000009UL                          /**< Mode DONE for DMA_STATUS */
#define _DMA_STATUS_STATE_PERSCATTRANS                  0x0000000AUL                          /**< Mode PERSCATTRANS for DMA_STATUS */
#define DMA_STATUS_STATE_DEFAULT                        (_DMA_STATUS_STATE_DEFAULT << 4)      /**< Shifted mode DEFAULT for DMA_STATUS */
#define DMA_STATUS_STATE_IDLE                           (_DMA_STATUS_STATE_IDLE << 4)         /**< Shifted mode IDLE for DMA_STATUS */
#define DMA_STATUS_STATE_RDCHCTRLDATA                   (_DMA_STATUS_STATE_RDCHCTRLDATA << 4) /**< Shifted mode RDCHCTRLDATA for DMA_STATUS */
#define DMA_STATUS_STATE_RDSRCENDPTR                    (_DMA_STATUS_STATE_RDSRCENDPTR << 4)  /**< Shifted mode RDSRCENDPTR for DMA_STATUS */
#define DMA_STATUS_STATE_RDDSTENDPTR                    (_DMA_STATUS_STATE_RDDSTENDPTR << 4)  /**< Shifted mode RDDSTENDPTR for DMA_STATUS */
#define DMA_STATUS_STATE_RDSRCDATA                      (_DMA_STATUS_STATE_RDSRCDATA << 4)    /**< Shifted mode RDSRCDATA for DMA_STATUS */
#define DMA_STATUS_STATE_WRDSTDATA                      (_DMA_STATUS_STATE_WRDSTDATA << 4)    /**< Shifted mode WRDSTDATA for DMA_STATUS */
#define DMA_STATUS_STATE_WAITREQCLR                     (_DMA_STATUS_STATE_WAITREQCLR << 4)   /**< Shifted mode WAITREQCLR for DMA_STATUS */
#define DMA_STATUS_STATE_WRCHCTRLDATA                   (_DMA_STATUS_STATE_WRCHCTRLDATA << 4) /**< Shifted mode WRCHCTRLDATA for DMA_STATUS */
#define DMA_STATUS_STATE_STALLED                        (_DMA_STATUS_STATE_STALLED << 4)      /**< Shifted mode STALLED for DMA_STATUS */
#define DMA_STATUS_STATE_DONE                           (_DMA_STATUS_STATE_DONE << 4)         /**< Shifted mode DONE for DMA_STATUS */
#define DMA_STATUS_STATE_PERSCATTRANS                   (_DMA_STATUS_STATE_PERSCATTRANS << 4) /**< Shifted mode PERSCATTRANS for DMA_STATUS */
#define _DMA_STATUS_CHNUM_SHIFT                         16                                    /**< Shift value for DMA_CHNUM */
#define _DMA_STATUS_CHNUM_MASK                          0x1F0000UL                            /**< Bit mask for DMA_CHNUM */
#define _DMA_STATUS_CHNUM_DEFAULT                       0x0000000BUL                          /**< Mode DEFAULT for DMA_STATUS */
#define DMA_STATUS_CHNUM_DEFAULT                        (_DMA_STATUS_CHNUM_DEFAULT << 16)     /**< Shifted mode DEFAULT for DMA_STATUS */

/* Bit fields for DMA CONFIG */
#define _DMA_CONFIG_RESETVALUE                          0x00000000UL                      /**< Default value for DMA_CONFIG */
#define _DMA_CONFIG_MASK                                0x00000021UL                      /**< Mask for DMA_CONFIG */
#define DMA_CONFIG_EN                                   (0x1UL << 0)                      /**< Enable DMA */
#define _DMA_CONFIG_EN_SHIFT                            0                                 /**< Shift value for DMA_EN */
#define _DMA_CONFIG_EN_MASK                             0x1UL                             /**< Bit mask for DMA_EN */
#define _DMA_CONFIG_EN_DEFAULT                          0x00000000UL                      /**< Mode DEFAULT for DMA_CONFIG */
#define DMA_CONFIG_EN_DEFAULT                           (_DMA_CONFIG_EN_DEFAULT << 0)     /**< Shifted mode DEFAULT for DMA_CONFIG */
#define DMA_CONFIG_CHPROT                               (0x1UL << 5)                      /**< Channel Protection Control */
#define _DMA_CONFIG_CHPROT_SHIFT                        5                                 /**< Shift value for DMA_CHPROT */
#define _DMA_CONFIG_CHPROT_MASK                         0x20UL                            /**< Bit mask for DMA_CHPROT */
#define _DMA_CONFIG_CHPROT_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for DMA_CONFIG */
#define DMA_CONFIG_CHPROT_DEFAULT                       (_DMA_CONFIG_CHPROT_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CONFIG */

/* Bit fields for DMA CTRLBASE */
#define _DMA_CTRLBASE_RESETVALUE                        0x00000000UL                          /**< Default value for DMA_CTRLBASE */
#define _DMA_CTRLBASE_MASK                              0xFFFFFFFFUL                          /**< Mask for DMA_CTRLBASE */
#define _DMA_CTRLBASE_CTRLBASE_SHIFT                    0                                     /**< Shift value for DMA_CTRLBASE */
#define _DMA_CTRLBASE_CTRLBASE_MASK                     0xFFFFFFFFUL                          /**< Bit mask for DMA_CTRLBASE */
#define _DMA_CTRLBASE_CTRLBASE_DEFAULT                  0x00000000UL                          /**< Mode DEFAULT for DMA_CTRLBASE */
#define DMA_CTRLBASE_CTRLBASE_DEFAULT                   (_DMA_CTRLBASE_CTRLBASE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CTRLBASE */

/* Bit fields for DMA ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_RESETVALUE                     0x00000100UL                                /**< Default value for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_MASK                           0xFFFFFFFFUL                                /**< Mask for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_SHIFT              0                                           /**< Shift value for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_MASK               0xFFFFFFFFUL                                /**< Bit mask for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT            0x00000100UL                                /**< Mode DEFAULT for DMA_ALTCTRLBASE */
#define DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT             (_DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_ALTCTRLBASE */

/* Bit fields for DMA CHWAITSTATUS */
#define _DMA_CHWAITSTATUS_RESETVALUE                    0x00000FFFUL                                     /**< Default value for DMA_CHWAITSTATUS */
#define _DMA_CHWAITSTATUS_MASK                          0x00000FFFUL                                     /**< Mask for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH0WAITSTATUS                  (0x1UL << 0)                                     /**< Channel 0 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_SHIFT           0                                                /**< Shift value for DMA_CH0WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_MASK            0x1UL                                            /**< Bit mask for DMA_CH0WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH1WAITSTATUS                  (0x1UL << 1)                                     /**< Channel 1 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_SHIFT           1                                                /**< Shift value for DMA_CH1WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_MASK            0x2UL                                            /**< Bit mask for DMA_CH1WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH2WAITSTATUS                  (0x1UL << 2)                                     /**< Channel 2 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_SHIFT           2                                                /**< Shift value for DMA_CH2WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_MASK            0x4UL                                            /**< Bit mask for DMA_CH2WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH3WAITSTATUS                  (0x1UL << 3)                                     /**< Channel 3 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_SHIFT           3                                                /**< Shift value for DMA_CH3WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_MASK            0x8UL                                            /**< Bit mask for DMA_CH3WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH4WAITSTATUS                  (0x1UL << 4)                                     /**< Channel 4 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_SHIFT           4                                                /**< Shift value for DMA_CH4WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_MASK            0x10UL                                           /**< Bit mask for DMA_CH4WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH5WAITSTATUS                  (0x1UL << 5)                                     /**< Channel 5 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_SHIFT           5                                                /**< Shift value for DMA_CH5WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_MASK            0x20UL                                           /**< Bit mask for DMA_CH5WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH6WAITSTATUS                  (0x1UL << 6)                                     /**< Channel 6 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH6WAITSTATUS_SHIFT           6                                                /**< Shift value for DMA_CH6WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH6WAITSTATUS_MASK            0x40UL                                           /**< Bit mask for DMA_CH6WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH6WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH6WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH6WAITSTATUS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH7WAITSTATUS                  (0x1UL << 7)                                     /**< Channel 7 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH7WAITSTATUS_SHIFT           7                                                /**< Shift value for DMA_CH7WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH7WAITSTATUS_MASK            0x80UL                                           /**< Bit mask for DMA_CH7WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH7WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH7WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH7WAITSTATUS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH8WAITSTATUS                  (0x1UL << 8)                                     /**< Channel 8 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH8WAITSTATUS_SHIFT           8                                                /**< Shift value for DMA_CH8WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH8WAITSTATUS_MASK            0x100UL                                          /**< Bit mask for DMA_CH8WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH8WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH8WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH8WAITSTATUS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH9WAITSTATUS                  (0x1UL << 9)                                     /**< Channel 9 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH9WAITSTATUS_SHIFT           9                                                /**< Shift value for DMA_CH9WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH9WAITSTATUS_MASK            0x200UL                                          /**< Bit mask for DMA_CH9WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH9WAITSTATUS_DEFAULT         0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH9WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH9WAITSTATUS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH10WAITSTATUS                 (0x1UL << 10)                                    /**< Channel 10 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH10WAITSTATUS_SHIFT          10                                               /**< Shift value for DMA_CH10WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH10WAITSTATUS_MASK           0x400UL                                          /**< Bit mask for DMA_CH10WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH10WAITSTATUS_DEFAULT        0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH10WAITSTATUS_DEFAULT         (_DMA_CHWAITSTATUS_CH10WAITSTATUS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH11WAITSTATUS                 (0x1UL << 11)                                    /**< Channel 11 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH11WAITSTATUS_SHIFT          11                                               /**< Shift value for DMA_CH11WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH11WAITSTATUS_MASK           0x800UL                                          /**< Bit mask for DMA_CH11WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH11WAITSTATUS_DEFAULT        0x00000001UL                                     /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH11WAITSTATUS_DEFAULT         (_DMA_CHWAITSTATUS_CH11WAITSTATUS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */

/* Bit fields for DMA CHSWREQ */
#define _DMA_CHSWREQ_RESETVALUE                         0x00000000UL                           /**< Default value for DMA_CHSWREQ */
#define _DMA_CHSWREQ_MASK                               0x00000FFFUL                           /**< Mask for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH0SWREQ                            (0x1UL << 0)                           /**< Channel 0 Software Request */
#define _DMA_CHSWREQ_CH0SWREQ_SHIFT                     0                                      /**< Shift value for DMA_CH0SWREQ */
#define _DMA_CHSWREQ_CH0SWREQ_MASK                      0x1UL                                  /**< Bit mask for DMA_CH0SWREQ */
#define _DMA_CHSWREQ_CH0SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH0SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH0SWREQ_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH1SWREQ                            (0x1UL << 1)                           /**< Channel 1 Software Request */
#define _DMA_CHSWREQ_CH1SWREQ_SHIFT                     1                                      /**< Shift value for DMA_CH1SWREQ */
#define _DMA_CHSWREQ_CH1SWREQ_MASK                      0x2UL                                  /**< Bit mask for DMA_CH1SWREQ */
#define _DMA_CHSWREQ_CH1SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH1SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH1SWREQ_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH2SWREQ                            (0x1UL << 2)                           /**< Channel 2 Software Request */
#define _DMA_CHSWREQ_CH2SWREQ_SHIFT                     2                                      /**< Shift value for DMA_CH2SWREQ */
#define _DMA_CHSWREQ_CH2SWREQ_MASK                      0x4UL                                  /**< Bit mask for DMA_CH2SWREQ */
#define _DMA_CHSWREQ_CH2SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH2SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH2SWREQ_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH3SWREQ                            (0x1UL << 3)                           /**< Channel 3 Software Request */
#define _DMA_CHSWREQ_CH3SWREQ_SHIFT                     3                                      /**< Shift value for DMA_CH3SWREQ */
#define _DMA_CHSWREQ_CH3SWREQ_MASK                      0x8UL                                  /**< Bit mask for DMA_CH3SWREQ */
#define _DMA_CHSWREQ_CH3SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH3SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH3SWREQ_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH4SWREQ                            (0x1UL << 4)                           /**< Channel 4 Software Request */
#define _DMA_CHSWREQ_CH4SWREQ_SHIFT                     4                                      /**< Shift value for DMA_CH4SWREQ */
#define _DMA_CHSWREQ_CH4SWREQ_MASK                      0x10UL                                 /**< Bit mask for DMA_CH4SWREQ */
#define _DMA_CHSWREQ_CH4SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH4SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH4SWREQ_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH5SWREQ                            (0x1UL << 5)                           /**< Channel 5 Software Request */
#define _DMA_CHSWREQ_CH5SWREQ_SHIFT                     5                                      /**< Shift value for DMA_CH5SWREQ */
#define _DMA_CHSWREQ_CH5SWREQ_MASK                      0x20UL                                 /**< Bit mask for DMA_CH5SWREQ */
#define _DMA_CHSWREQ_CH5SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH5SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH5SWREQ_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH6SWREQ                            (0x1UL << 6)                           /**< Channel 6 Software Request */
#define _DMA_CHSWREQ_CH6SWREQ_SHIFT                     6                                      /**< Shift value for DMA_CH6SWREQ */
#define _DMA_CHSWREQ_CH6SWREQ_MASK                      0x40UL                                 /**< Bit mask for DMA_CH6SWREQ */
#define _DMA_CHSWREQ_CH6SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH6SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH6SWREQ_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH7SWREQ                            (0x1UL << 7)                           /**< Channel 7 Software Request */
#define _DMA_CHSWREQ_CH7SWREQ_SHIFT                     7                                      /**< Shift value for DMA_CH7SWREQ */
#define _DMA_CHSWREQ_CH7SWREQ_MASK                      0x80UL                                 /**< Bit mask for DMA_CH7SWREQ */
#define _DMA_CHSWREQ_CH7SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH7SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH7SWREQ_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH8SWREQ                            (0x1UL << 8)                           /**< Channel 8 Software Request */
#define _DMA_CHSWREQ_CH8SWREQ_SHIFT                     8                                      /**< Shift value for DMA_CH8SWREQ */
#define _DMA_CHSWREQ_CH8SWREQ_MASK                      0x100UL                                /**< Bit mask for DMA_CH8SWREQ */
#define _DMA_CHSWREQ_CH8SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH8SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH8SWREQ_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH9SWREQ                            (0x1UL << 9)                           /**< Channel 9 Software Request */
#define _DMA_CHSWREQ_CH9SWREQ_SHIFT                     9                                      /**< Shift value for DMA_CH9SWREQ */
#define _DMA_CHSWREQ_CH9SWREQ_MASK                      0x200UL                                /**< Bit mask for DMA_CH9SWREQ */
#define _DMA_CHSWREQ_CH9SWREQ_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH9SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH9SWREQ_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH10SWREQ                           (0x1UL << 10)                          /**< Channel 10 Software Request */
#define _DMA_CHSWREQ_CH10SWREQ_SHIFT                    10                                     /**< Shift value for DMA_CH10SWREQ */
#define _DMA_CHSWREQ_CH10SWREQ_MASK                     0x400UL                                /**< Bit mask for DMA_CH10SWREQ */
#define _DMA_CHSWREQ_CH10SWREQ_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH10SWREQ_DEFAULT                   (_DMA_CHSWREQ_CH10SWREQ_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH11SWREQ                           (0x1UL << 11)                          /**< Channel 11 Software Request */
#define _DMA_CHSWREQ_CH11SWREQ_SHIFT                    11                                     /**< Shift value for DMA_CH11SWREQ */
#define _DMA_CHSWREQ_CH11SWREQ_MASK                     0x800UL                                /**< Bit mask for DMA_CH11SWREQ */
#define _DMA_CHSWREQ_CH11SWREQ_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH11SWREQ_DEFAULT                   (_DMA_CHSWREQ_CH11SWREQ_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHSWREQ */

/* Bit fields for DMA CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_RESETVALUE                     0x00000000UL                                        /**< Default value for DMA_CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_MASK                           0x00000FFFUL                                        /**< Mask for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH0USEBURSTS                    (0x1UL << 0)                                        /**< Channel 0 Useburst Set */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_SHIFT             0                                                   /**< Shift value for DMA_CH0USEBURSTS */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_MASK              0x1UL                                               /**< Bit mask for DMA_CH0USEBURSTS */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_SINGLEANDBURST    0x00000000UL                                        /**< Mode SINGLEANDBURST for DMA_CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_CH0USEBURSTS_BURSTONLY         0x00000001UL                                        /**< Mode BURSTONLY for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH0USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH0USEBURSTS_DEFAULT << 0)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH0USEBURSTS_SINGLEANDBURST     (_DMA_CHUSEBURSTS_CH0USEBURSTS_SINGLEANDBURST << 0) /**< Shifted mode SINGLEANDBURST for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH0USEBURSTS_BURSTONLY          (_DMA_CHUSEBURSTS_CH0USEBURSTS_BURSTONLY << 0)      /**< Shifted mode BURSTONLY for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH1USEBURSTS                    (0x1UL << 1)                                        /**< Channel 1 Useburst Set */
#define _DMA_CHUSEBURSTS_CH1USEBURSTS_SHIFT             1                                                   /**< Shift value for DMA_CH1USEBURSTS */
#define _DMA_CHUSEBURSTS_CH1USEBURSTS_MASK              0x2UL                                               /**< Bit mask for DMA_CH1USEBURSTS */
#define _DMA_CHUSEBURSTS_CH1USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH1USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH1USEBURSTS_DEFAULT << 1)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH2USEBURSTS                    (0x1UL << 2)                                        /**< Channel 2 Useburst Set */
#define _DMA_CHUSEBURSTS_CH2USEBURSTS_SHIFT             2                                                   /**< Shift value for DMA_CH2USEBURSTS */
#define _DMA_CHUSEBURSTS_CH2USEBURSTS_MASK              0x4UL                                               /**< Bit mask for DMA_CH2USEBURSTS */
#define _DMA_CHUSEBURSTS_CH2USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH2USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH2USEBURSTS_DEFAULT << 2)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH3USEBURSTS                    (0x1UL << 3)                                        /**< Channel 3 Useburst Set */
#define _DMA_CHUSEBURSTS_CH3USEBURSTS_SHIFT             3                                                   /**< Shift value for DMA_CH3USEBURSTS */
#define _DMA_CHUSEBURSTS_CH3USEBURSTS_MASK              0x8UL                                               /**< Bit mask for DMA_CH3USEBURSTS */
#define _DMA_CHUSEBURSTS_CH3USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH3USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH3USEBURSTS_DEFAULT << 3)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH4USEBURSTS                    (0x1UL << 4)                                        /**< Channel 4 Useburst Set */
#define _DMA_CHUSEBURSTS_CH4USEBURSTS_SHIFT             4                                                   /**< Shift value for DMA_CH4USEBURSTS */
#define _DMA_CHUSEBURSTS_CH4USEBURSTS_MASK              0x10UL                                              /**< Bit mask for DMA_CH4USEBURSTS */
#define _DMA_CHUSEBURSTS_CH4USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH4USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH4USEBURSTS_DEFAULT << 4)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH5USEBURSTS                    (0x1UL << 5)                                        /**< Channel 5 Useburst Set */
#define _DMA_CHUSEBURSTS_CH5USEBURSTS_SHIFT             5                                                   /**< Shift value for DMA_CH5USEBURSTS */
#define _DMA_CHUSEBURSTS_CH5USEBURSTS_MASK              0x20UL                                              /**< Bit mask for DMA_CH5USEBURSTS */
#define _DMA_CHUSEBURSTS_CH5USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH5USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH5USEBURSTS_DEFAULT << 5)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH6USEBURSTS                    (0x1UL << 6)                                        /**< Channel 6 Useburst Set */
#define _DMA_CHUSEBURSTS_CH6USEBURSTS_SHIFT             6                                                   /**< Shift value for DMA_CH6USEBURSTS */
#define _DMA_CHUSEBURSTS_CH6USEBURSTS_MASK              0x40UL                                              /**< Bit mask for DMA_CH6USEBURSTS */
#define _DMA_CHUSEBURSTS_CH6USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH6USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH6USEBURSTS_DEFAULT << 6)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH7USEBURSTS                    (0x1UL << 7)                                        /**< Channel 7 Useburst Set */
#define _DMA_CHUSEBURSTS_CH7USEBURSTS_SHIFT             7                                                   /**< Shift value for DMA_CH7USEBURSTS */
#define _DMA_CHUSEBURSTS_CH7USEBURSTS_MASK              0x80UL                                              /**< Bit mask for DMA_CH7USEBURSTS */
#define _DMA_CHUSEBURSTS_CH7USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH7USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH7USEBURSTS_DEFAULT << 7)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH8USEBURSTS                    (0x1UL << 8)                                        /**< Channel 8 Useburst Set */
#define _DMA_CHUSEBURSTS_CH8USEBURSTS_SHIFT             8                                                   /**< Shift value for DMA_CH8USEBURSTS */
#define _DMA_CHUSEBURSTS_CH8USEBURSTS_MASK              0x100UL                                             /**< Bit mask for DMA_CH8USEBURSTS */
#define _DMA_CHUSEBURSTS_CH8USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH8USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH8USEBURSTS_DEFAULT << 8)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH9USEBURSTS                    (0x1UL << 9)                                        /**< Channel 9 Useburst Set */
#define _DMA_CHUSEBURSTS_CH9USEBURSTS_SHIFT             9                                                   /**< Shift value for DMA_CH9USEBURSTS */
#define _DMA_CHUSEBURSTS_CH9USEBURSTS_MASK              0x200UL                                             /**< Bit mask for DMA_CH9USEBURSTS */
#define _DMA_CHUSEBURSTS_CH9USEBURSTS_DEFAULT           0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH9USEBURSTS_DEFAULT            (_DMA_CHUSEBURSTS_CH9USEBURSTS_DEFAULT << 9)        /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH10USEBURSTS                   (0x1UL << 10)                                       /**< Channel 10 Useburst Set */
#define _DMA_CHUSEBURSTS_CH10USEBURSTS_SHIFT            10                                                  /**< Shift value for DMA_CH10USEBURSTS */
#define _DMA_CHUSEBURSTS_CH10USEBURSTS_MASK             0x400UL                                             /**< Bit mask for DMA_CH10USEBURSTS */
#define _DMA_CHUSEBURSTS_CH10USEBURSTS_DEFAULT          0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH10USEBURSTS_DEFAULT           (_DMA_CHUSEBURSTS_CH10USEBURSTS_DEFAULT << 10)      /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH11USEBURSTS                   (0x1UL << 11)                                       /**< Channel 11 Useburst Set */
#define _DMA_CHUSEBURSTS_CH11USEBURSTS_SHIFT            11                                                  /**< Shift value for DMA_CH11USEBURSTS */
#define _DMA_CHUSEBURSTS_CH11USEBURSTS_MASK             0x800UL                                             /**< Bit mask for DMA_CH11USEBURSTS */
#define _DMA_CHUSEBURSTS_CH11USEBURSTS_DEFAULT          0x00000000UL                                        /**< Mode DEFAULT for DMA_CHUSEBURSTS */
#define DMA_CHUSEBURSTS_CH11USEBURSTS_DEFAULT           (_DMA_CHUSEBURSTS_CH11USEBURSTS_DEFAULT << 11)      /**< Shifted mode DEFAULT for DMA_CHUSEBURSTS */

/* Bit fields for DMA CHUSEBURSTC */
#define _DMA_CHUSEBURSTC_RESETVALUE                     0x00000000UL                                   /**< Default value for DMA_CHUSEBURSTC */
#define _DMA_CHUSEBURSTC_MASK                           0x00000FFFUL                                   /**< Mask for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH0USEBURSTC                    (0x1UL << 0)                                   /**< Channel 0 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_SHIFT             0                                              /**< Shift value for DMA_CH0USEBURSTC */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_MASK              0x1UL                                          /**< Bit mask for DMA_CH0USEBURSTC */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH1USEBURSTC                    (0x1UL << 1)                                   /**< Channel 1 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_SHIFT             1                                              /**< Shift value for DMA_CH1USEBURSTC */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_MASK              0x2UL                                          /**< Bit mask for DMA_CH1USEBURSTC */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH2USEBURSTC                    (0x1UL << 2)                                   /**< Channel 2 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_SHIFT             2                                              /**< Shift value for DMA_CH2USEBURSTC */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_MASK              0x4UL                                          /**< Bit mask for DMA_CH2USEBURSTC */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH3USEBURSTC                    (0x1UL << 3)                                   /**< Channel 3 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_SHIFT             3                                              /**< Shift value for DMA_CH3USEBURSTC */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_MASK              0x8UL                                          /**< Bit mask for DMA_CH3USEBURSTC */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH4USEBURSTC                    (0x1UL << 4)                                   /**< Channel 4 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_SHIFT             4                                              /**< Shift value for DMA_CH4USEBURSTC */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_MASK              0x10UL                                         /**< Bit mask for DMA_CH4USEBURSTC */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH5USEBURSTC                    (0x1UL << 5)                                   /**< Channel 5 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_SHIFT             5                                              /**< Shift value for DMA_CH5USEBURSTC */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_MASK              0x20UL                                         /**< Bit mask for DMA_CH5USEBURSTC */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH6USEBURSTC                    (0x1UL << 6)                                   /**< Channel 6 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH6USEBURSTC_SHIFT             6                                              /**< Shift value for DMA_CH6USEBURSTC */
#define _DMA_CHUSEBURSTC_CH6USEBURSTC_MASK              0x40UL                                         /**< Bit mask for DMA_CH6USEBURSTC */
#define _DMA_CHUSEBURSTC_CH6USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH6USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH6USEBURSTC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH7USEBURSTC                    (0x1UL << 7)                                   /**< Channel 7 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH7USEBURSTC_SHIFT             7                                              /**< Shift value for DMA_CH7USEBURSTC */
#define _DMA_CHUSEBURSTC_CH7USEBURSTC_MASK              0x80UL                                         /**< Bit mask for DMA_CH7USEBURSTC */
#define _DMA_CHUSEBURSTC_CH7USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH7USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH7USEBURSTC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH08USEBURSTC                   (0x1UL << 8)                                   /**< Channel 8 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH08USEBURSTC_SHIFT            8                                              /**< Shift value for DMA_CH08USEBURSTC */
#define _DMA_CHUSEBURSTC_CH08USEBURSTC_MASK             0x100UL                                        /**< Bit mask for DMA_CH08USEBURSTC */
#define _DMA_CHUSEBURSTC_CH08USEBURSTC_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH08USEBURSTC_DEFAULT           (_DMA_CHUSEBURSTC_CH08USEBURSTC_DEFAULT << 8)  /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH9USEBURSTC                    (0x1UL << 9)                                   /**< Channel 9 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH9USEBURSTC_SHIFT             9                                              /**< Shift value for DMA_CH9USEBURSTC */
#define _DMA_CHUSEBURSTC_CH9USEBURSTC_MASK              0x200UL                                        /**< Bit mask for DMA_CH9USEBURSTC */
#define _DMA_CHUSEBURSTC_CH9USEBURSTC_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH9USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH9USEBURSTC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH10USEBURSTC                   (0x1UL << 10)                                  /**< Channel 10 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH10USEBURSTC_SHIFT            10                                             /**< Shift value for DMA_CH10USEBURSTC */
#define _DMA_CHUSEBURSTC_CH10USEBURSTC_MASK             0x400UL                                        /**< Bit mask for DMA_CH10USEBURSTC */
#define _DMA_CHUSEBURSTC_CH10USEBURSTC_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH10USEBURSTC_DEFAULT           (_DMA_CHUSEBURSTC_CH10USEBURSTC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH11USEBURSTC                   (0x1UL << 11)                                  /**< Channel 11 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH11USEBURSTC_SHIFT            11                                             /**< Shift value for DMA_CH11USEBURSTC */
#define _DMA_CHUSEBURSTC_CH11USEBURSTC_MASK             0x800UL                                        /**< Bit mask for DMA_CH11USEBURSTC */
#define _DMA_CHUSEBURSTC_CH11USEBURSTC_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH11USEBURSTC_DEFAULT           (_DMA_CHUSEBURSTC_CH11USEBURSTC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */

/* Bit fields for DMA CHREQMASKS */
#define _DMA_CHREQMASKS_RESETVALUE                      0x00000000UL                                 /**< Default value for DMA_CHREQMASKS */
#define _DMA_CHREQMASKS_MASK                            0x00000FFFUL                                 /**< Mask for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH0REQMASKS                      (0x1UL << 0)                                 /**< Channel 0 Request Mask Set */
#define _DMA_CHREQMASKS_CH0REQMASKS_SHIFT               0                                            /**< Shift value for DMA_CH0REQMASKS */
#define _DMA_CHREQMASKS_CH0REQMASKS_MASK                0x1UL                                        /**< Bit mask for DMA_CH0REQMASKS */
#define _DMA_CHREQMASKS_CH0REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH0REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH0REQMASKS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH1REQMASKS                      (0x1UL << 1)                                 /**< Channel 1 Request Mask Set */
#define _DMA_CHREQMASKS_CH1REQMASKS_SHIFT               1                                            /**< Shift value for DMA_CH1REQMASKS */
#define _DMA_CHREQMASKS_CH1REQMASKS_MASK                0x2UL                                        /**< Bit mask for DMA_CH1REQMASKS */
#define _DMA_CHREQMASKS_CH1REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH1REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH1REQMASKS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH2REQMASKS                      (0x1UL << 2)                                 /**< Channel 2 Request Mask Set */
#define _DMA_CHREQMASKS_CH2REQMASKS_SHIFT               2                                            /**< Shift value for DMA_CH2REQMASKS */
#define _DMA_CHREQMASKS_CH2REQMASKS_MASK                0x4UL                                        /**< Bit mask for DMA_CH2REQMASKS */
#define _DMA_CHREQMASKS_CH2REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH2REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH2REQMASKS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH3REQMASKS                      (0x1UL << 3)                                 /**< Channel 3 Request Mask Set */
#define _DMA_CHREQMASKS_CH3REQMASKS_SHIFT               3                                            /**< Shift value for DMA_CH3REQMASKS */
#define _DMA_CHREQMASKS_CH3REQMASKS_MASK                0x8UL                                        /**< Bit mask for DMA_CH3REQMASKS */
#define _DMA_CHREQMASKS_CH3REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH3REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH3REQMASKS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH4REQMASKS                      (0x1UL << 4)                                 /**< Channel 4 Request Mask Set */
#define _DMA_CHREQMASKS_CH4REQMASKS_SHIFT               4                                            /**< Shift value for DMA_CH4REQMASKS */
#define _DMA_CHREQMASKS_CH4REQMASKS_MASK                0x10UL                                       /**< Bit mask for DMA_CH4REQMASKS */
#define _DMA_CHREQMASKS_CH4REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH4REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH4REQMASKS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH5REQMASKS                      (0x1UL << 5)                                 /**< Channel 5 Request Mask Set */
#define _DMA_CHREQMASKS_CH5REQMASKS_SHIFT               5                                            /**< Shift value for DMA_CH5REQMASKS */
#define _DMA_CHREQMASKS_CH5REQMASKS_MASK                0x20UL                                       /**< Bit mask for DMA_CH5REQMASKS */
#define _DMA_CHREQMASKS_CH5REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH5REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH5REQMASKS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH6REQMASKS                      (0x1UL << 6)                                 /**< Channel 6 Request Mask Set */
#define _DMA_CHREQMASKS_CH6REQMASKS_SHIFT               6                                            /**< Shift value for DMA_CH6REQMASKS */
#define _DMA_CHREQMASKS_CH6REQMASKS_MASK                0x40UL                                       /**< Bit mask for DMA_CH6REQMASKS */
#define _DMA_CHREQMASKS_CH6REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH6REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH6REQMASKS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH7REQMASKS                      (0x1UL << 7)                                 /**< Channel 7 Request Mask Set */
#define _DMA_CHREQMASKS_CH7REQMASKS_SHIFT               7                                            /**< Shift value for DMA_CH7REQMASKS */
#define _DMA_CHREQMASKS_CH7REQMASKS_MASK                0x80UL                                       /**< Bit mask for DMA_CH7REQMASKS */
#define _DMA_CHREQMASKS_CH7REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH7REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH7REQMASKS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH8REQMASKS                      (0x1UL << 8)                                 /**< Channel 8 Request Mask Set */
#define _DMA_CHREQMASKS_CH8REQMASKS_SHIFT               8                                            /**< Shift value for DMA_CH8REQMASKS */
#define _DMA_CHREQMASKS_CH8REQMASKS_MASK                0x100UL                                      /**< Bit mask for DMA_CH8REQMASKS */
#define _DMA_CHREQMASKS_CH8REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH8REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH8REQMASKS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH9REQMASKS                      (0x1UL << 9)                                 /**< Channel 9 Request Mask Set */
#define _DMA_CHREQMASKS_CH9REQMASKS_SHIFT               9                                            /**< Shift value for DMA_CH9REQMASKS */
#define _DMA_CHREQMASKS_CH9REQMASKS_MASK                0x200UL                                      /**< Bit mask for DMA_CH9REQMASKS */
#define _DMA_CHREQMASKS_CH9REQMASKS_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH9REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH9REQMASKS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH10REQMASKS                     (0x1UL << 10)                                /**< Channel 10 Request Mask Set */
#define _DMA_CHREQMASKS_CH10REQMASKS_SHIFT              10                                           /**< Shift value for DMA_CH10REQMASKS */
#define _DMA_CHREQMASKS_CH10REQMASKS_MASK               0x400UL                                      /**< Bit mask for DMA_CH10REQMASKS */
#define _DMA_CHREQMASKS_CH10REQMASKS_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH10REQMASKS_DEFAULT             (_DMA_CHREQMASKS_CH10REQMASKS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH11REQMASKS                     (0x1UL << 11)                                /**< Channel 11 Request Mask Set */
#define _DMA_CHREQMASKS_CH11REQMASKS_SHIFT              11                                           /**< Shift value for DMA_CH11REQMASKS */
#define _DMA_CHREQMASKS_CH11REQMASKS_MASK               0x800UL                                      /**< Bit mask for DMA_CH11REQMASKS */
#define _DMA_CHREQMASKS_CH11REQMASKS_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH11REQMASKS_DEFAULT             (_DMA_CHREQMASKS_CH11REQMASKS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */

/* Bit fields for DMA CHREQMASKC */
#define _DMA_CHREQMASKC_RESETVALUE                      0x00000000UL                                 /**< Default value for DMA_CHREQMASKC */
#define _DMA_CHREQMASKC_MASK                            0x00000FFFUL                                 /**< Mask for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH0REQMASKC                      (0x1UL << 0)                                 /**< Channel 0 Request Mask Clear */
#define _DMA_CHREQMASKC_CH0REQMASKC_SHIFT               0                                            /**< Shift value for DMA_CH0REQMASKC */
#define _DMA_CHREQMASKC_CH0REQMASKC_MASK                0x1UL                                        /**< Bit mask for DMA_CH0REQMASKC */
#define _DMA_CHREQMASKC_CH0REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH0REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH0REQMASKC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH1REQMASKC                      (0x1UL << 1)                                 /**< Channel 1 Request Mask Clear */
#define _DMA_CHREQMASKC_CH1REQMASKC_SHIFT               1                                            /**< Shift value for DMA_CH1REQMASKC */
#define _DMA_CHREQMASKC_CH1REQMASKC_MASK                0x2UL                                        /**< Bit mask for DMA_CH1REQMASKC */
#define _DMA_CHREQMASKC_CH1REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH1REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH1REQMASKC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH2REQMASKC                      (0x1UL << 2)                                 /**< Channel 2 Request Mask Clear */
#define _DMA_CHREQMASKC_CH2REQMASKC_SHIFT               2                                            /**< Shift value for DMA_CH2REQMASKC */
#define _DMA_CHREQMASKC_CH2REQMASKC_MASK                0x4UL                                        /**< Bit mask for DMA_CH2REQMASKC */
#define _DMA_CHREQMASKC_CH2REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH2REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH2REQMASKC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH3REQMASKC                      (0x1UL << 3)                                 /**< Channel 3 Request Mask Clear */
#define _DMA_CHREQMASKC_CH3REQMASKC_SHIFT               3                                            /**< Shift value for DMA_CH3REQMASKC */
#define _DMA_CHREQMASKC_CH3REQMASKC_MASK                0x8UL                                        /**< Bit mask for DMA_CH3REQMASKC */
#define _DMA_CHREQMASKC_CH3REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH3REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH3REQMASKC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH4REQMASKC                      (0x1UL << 4)                                 /**< Channel 4 Request Mask Clear */
#define _DMA_CHREQMASKC_CH4REQMASKC_SHIFT               4                                            /**< Shift value for DMA_CH4REQMASKC */
#define _DMA_CHREQMASKC_CH4REQMASKC_MASK                0x10UL                                       /**< Bit mask for DMA_CH4REQMASKC */
#define _DMA_CHREQMASKC_CH4REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH4REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH4REQMASKC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH5REQMASKC                      (0x1UL << 5)                                 /**< Channel 5 Request Mask Clear */
#define _DMA_CHREQMASKC_CH5REQMASKC_SHIFT               5                                            /**< Shift value for DMA_CH5REQMASKC */
#define _DMA_CHREQMASKC_CH5REQMASKC_MASK                0x20UL                                       /**< Bit mask for DMA_CH5REQMASKC */
#define _DMA_CHREQMASKC_CH5REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH5REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH5REQMASKC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH6REQMASKC                      (0x1UL << 6)                                 /**< Channel 6 Request Mask Clear */
#define _DMA_CHREQMASKC_CH6REQMASKC_SHIFT               6                                            /**< Shift value for DMA_CH6REQMASKC */
#define _DMA_CHREQMASKC_CH6REQMASKC_MASK                0x40UL                                       /**< Bit mask for DMA_CH6REQMASKC */
#define _DMA_CHREQMASKC_CH6REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH6REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH6REQMASKC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH7REQMASKC                      (0x1UL << 7)                                 /**< Channel 7 Request Mask Clear */
#define _DMA_CHREQMASKC_CH7REQMASKC_SHIFT               7                                            /**< Shift value for DMA_CH7REQMASKC */
#define _DMA_CHREQMASKC_CH7REQMASKC_MASK                0x80UL                                       /**< Bit mask for DMA_CH7REQMASKC */
#define _DMA_CHREQMASKC_CH7REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH7REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH7REQMASKC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH8REQMASKC                      (0x1UL << 8)                                 /**< Channel 8 Request Mask Clear */
#define _DMA_CHREQMASKC_CH8REQMASKC_SHIFT               8                                            /**< Shift value for DMA_CH8REQMASKC */
#define _DMA_CHREQMASKC_CH8REQMASKC_MASK                0x100UL                                      /**< Bit mask for DMA_CH8REQMASKC */
#define _DMA_CHREQMASKC_CH8REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH8REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH8REQMASKC_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH9REQMASKC                      (0x1UL << 9)                                 /**< Channel 9 Request Mask Clear */
#define _DMA_CHREQMASKC_CH9REQMASKC_SHIFT               9                                            /**< Shift value for DMA_CH9REQMASKC */
#define _DMA_CHREQMASKC_CH9REQMASKC_MASK                0x200UL                                      /**< Bit mask for DMA_CH9REQMASKC */
#define _DMA_CHREQMASKC_CH9REQMASKC_DEFAULT             0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH9REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH9REQMASKC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH10REQMASKC                     (0x1UL << 10)                                /**< Channel 10 Request Mask Clear */
#define _DMA_CHREQMASKC_CH10REQMASKC_SHIFT              10                                           /**< Shift value for DMA_CH10REQMASKC */
#define _DMA_CHREQMASKC_CH10REQMASKC_MASK               0x400UL                                      /**< Bit mask for DMA_CH10REQMASKC */
#define _DMA_CHREQMASKC_CH10REQMASKC_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH10REQMASKC_DEFAULT             (_DMA_CHREQMASKC_CH10REQMASKC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH11REQMASKC                     (0x1UL << 11)                                /**< Channel 11 Request Mask Clear */
#define _DMA_CHREQMASKC_CH11REQMASKC_SHIFT              11                                           /**< Shift value for DMA_CH11REQMASKC */
#define _DMA_CHREQMASKC_CH11REQMASKC_MASK               0x800UL                                      /**< Bit mask for DMA_CH11REQMASKC */
#define _DMA_CHREQMASKC_CH11REQMASKC_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH11REQMASKC_DEFAULT             (_DMA_CHREQMASKC_CH11REQMASKC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */

/* Bit fields for DMA CHENS */
#define _DMA_CHENS_RESETVALUE                           0x00000000UL                       /**< Default value for DMA_CHENS */
#define _DMA_CHENS_MASK                                 0x00000FFFUL                       /**< Mask for DMA_CHENS */
#define DMA_CHENS_CH0ENS                                (0x1UL << 0)                       /**< Channel 0 Enable Set */
#define _DMA_CHENS_CH0ENS_SHIFT                         0                                  /**< Shift value for DMA_CH0ENS */
#define _DMA_CHENS_CH0ENS_MASK                          0x1UL                              /**< Bit mask for DMA_CH0ENS */
#define _DMA_CHENS_CH0ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH0ENS_DEFAULT                        (_DMA_CHENS_CH0ENS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH1ENS                                (0x1UL << 1)                       /**< Channel 1 Enable Set */
#define _DMA_CHENS_CH1ENS_SHIFT                         1                                  /**< Shift value for DMA_CH1ENS */
#define _DMA_CHENS_CH1ENS_MASK                          0x2UL                              /**< Bit mask for DMA_CH1ENS */
#define _DMA_CHENS_CH1ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH1ENS_DEFAULT                        (_DMA_CHENS_CH1ENS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH2ENS                                (0x1UL << 2)                       /**< Channel 2 Enable Set */
#define _DMA_CHENS_CH2ENS_SHIFT                         2                                  /**< Shift value for DMA_CH2ENS */
#define _DMA_CHENS_CH2ENS_MASK                          0x4UL                              /**< Bit mask for DMA_CH2ENS */
#define _DMA_CHENS_CH2ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH2ENS_DEFAULT                        (_DMA_CHENS_CH2ENS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH3ENS                                (0x1UL << 3)                       /**< Channel 3 Enable Set */
#define _DMA_CHENS_CH3ENS_SHIFT                         3                                  /**< Shift value for DMA_CH3ENS */
#define _DMA_CHENS_CH3ENS_MASK                          0x8UL                              /**< Bit mask for DMA_CH3ENS */
#define _DMA_CHENS_CH3ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH3ENS_DEFAULT                        (_DMA_CHENS_CH3ENS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH4ENS                                (0x1UL << 4)                       /**< Channel 4 Enable Set */
#define _DMA_CHENS_CH4ENS_SHIFT                         4                                  /**< Shift value for DMA_CH4ENS */
#define _DMA_CHENS_CH4ENS_MASK                          0x10UL                             /**< Bit mask for DMA_CH4ENS */
#define _DMA_CHENS_CH4ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH4ENS_DEFAULT                        (_DMA_CHENS_CH4ENS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH5ENS                                (0x1UL << 5)                       /**< Channel 5 Enable Set */
#define _DMA_CHENS_CH5ENS_SHIFT                         5                                  /**< Shift value for DMA_CH5ENS */
#define _DMA_CHENS_CH5ENS_MASK                          0x20UL                             /**< Bit mask for DMA_CH5ENS */
#define _DMA_CHENS_CH5ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH5ENS_DEFAULT                        (_DMA_CHENS_CH5ENS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH6ENS                                (0x1UL << 6)                       /**< Channel 6 Enable Set */
#define _DMA_CHENS_CH6ENS_SHIFT                         6                                  /**< Shift value for DMA_CH6ENS */
#define _DMA_CHENS_CH6ENS_MASK                          0x40UL                             /**< Bit mask for DMA_CH6ENS */
#define _DMA_CHENS_CH6ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH6ENS_DEFAULT                        (_DMA_CHENS_CH6ENS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH7ENS                                (0x1UL << 7)                       /**< Channel 7 Enable Set */
#define _DMA_CHENS_CH7ENS_SHIFT                         7                                  /**< Shift value for DMA_CH7ENS */
#define _DMA_CHENS_CH7ENS_MASK                          0x80UL                             /**< Bit mask for DMA_CH7ENS */
#define _DMA_CHENS_CH7ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH7ENS_DEFAULT                        (_DMA_CHENS_CH7ENS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH8ENS                                (0x1UL << 8)                       /**< Channel 8 Enable Set */
#define _DMA_CHENS_CH8ENS_SHIFT                         8                                  /**< Shift value for DMA_CH8ENS */
#define _DMA_CHENS_CH8ENS_MASK                          0x100UL                            /**< Bit mask for DMA_CH8ENS */
#define _DMA_CHENS_CH8ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH8ENS_DEFAULT                        (_DMA_CHENS_CH8ENS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH9ENS                                (0x1UL << 9)                       /**< Channel 9 Enable Set */
#define _DMA_CHENS_CH9ENS_SHIFT                         9                                  /**< Shift value for DMA_CH9ENS */
#define _DMA_CHENS_CH9ENS_MASK                          0x200UL                            /**< Bit mask for DMA_CH9ENS */
#define _DMA_CHENS_CH9ENS_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH9ENS_DEFAULT                        (_DMA_CHENS_CH9ENS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH10ENS                               (0x1UL << 10)                      /**< Channel 10 Enable Set */
#define _DMA_CHENS_CH10ENS_SHIFT                        10                                 /**< Shift value for DMA_CH10ENS */
#define _DMA_CHENS_CH10ENS_MASK                         0x400UL                            /**< Bit mask for DMA_CH10ENS */
#define _DMA_CHENS_CH10ENS_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH10ENS_DEFAULT                       (_DMA_CHENS_CH10ENS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH11ENS                               (0x1UL << 11)                      /**< Channel 11 Enable Set */
#define _DMA_CHENS_CH11ENS_SHIFT                        11                                 /**< Shift value for DMA_CH11ENS */
#define _DMA_CHENS_CH11ENS_MASK                         0x800UL                            /**< Bit mask for DMA_CH11ENS */
#define _DMA_CHENS_CH11ENS_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH11ENS_DEFAULT                       (_DMA_CHENS_CH11ENS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHENS */

/* Bit fields for DMA CHENC */
#define _DMA_CHENC_RESETVALUE                           0x00000000UL                       /**< Default value for DMA_CHENC */
#define _DMA_CHENC_MASK                                 0x00000FFFUL                       /**< Mask for DMA_CHENC */
#define DMA_CHENC_CH0ENC                                (0x1UL << 0)                       /**< Channel 0 Enable Clear */
#define _DMA_CHENC_CH0ENC_SHIFT                         0                                  /**< Shift value for DMA_CH0ENC */
#define _DMA_CHENC_CH0ENC_MASK                          0x1UL                              /**< Bit mask for DMA_CH0ENC */
#define _DMA_CHENC_CH0ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH0ENC_DEFAULT                        (_DMA_CHENC_CH0ENC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH1ENC                                (0x1UL << 1)                       /**< Channel 1 Enable Clear */
#define _DMA_CHENC_CH1ENC_SHIFT                         1                                  /**< Shift value for DMA_CH1ENC */
#define _DMA_CHENC_CH1ENC_MASK                          0x2UL                              /**< Bit mask for DMA_CH1ENC */
#define _DMA_CHENC_CH1ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH1ENC_DEFAULT                        (_DMA_CHENC_CH1ENC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH2ENC                                (0x1UL << 2)                       /**< Channel 2 Enable Clear */
#define _DMA_CHENC_CH2ENC_SHIFT                         2                                  /**< Shift value for DMA_CH2ENC */
#define _DMA_CHENC_CH2ENC_MASK                          0x4UL                              /**< Bit mask for DMA_CH2ENC */
#define _DMA_CHENC_CH2ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH2ENC_DEFAULT                        (_DMA_CHENC_CH2ENC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH3ENC                                (0x1UL << 3)                       /**< Channel 3 Enable Clear */
#define _DMA_CHENC_CH3ENC_SHIFT                         3                                  /**< Shift value for DMA_CH3ENC */
#define _DMA_CHENC_CH3ENC_MASK                          0x8UL                              /**< Bit mask for DMA_CH3ENC */
#define _DMA_CHENC_CH3ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH3ENC_DEFAULT                        (_DMA_CHENC_CH3ENC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH4ENC                                (0x1UL << 4)                       /**< Channel 4 Enable Clear */
#define _DMA_CHENC_CH4ENC_SHIFT                         4                                  /**< Shift value for DMA_CH4ENC */
#define _DMA_CHENC_CH4ENC_MASK                          0x10UL                             /**< Bit mask for DMA_CH4ENC */
#define _DMA_CHENC_CH4ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH4ENC_DEFAULT                        (_DMA_CHENC_CH4ENC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH5ENC                                (0x1UL << 5)                       /**< Channel 5 Enable Clear */
#define _DMA_CHENC_CH5ENC_SHIFT                         5                                  /**< Shift value for DMA_CH5ENC */
#define _DMA_CHENC_CH5ENC_MASK                          0x20UL                             /**< Bit mask for DMA_CH5ENC */
#define _DMA_CHENC_CH5ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH5ENC_DEFAULT                        (_DMA_CHENC_CH5ENC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH6ENC                                (0x1UL << 6)                       /**< Channel 6 Enable Clear */
#define _DMA_CHENC_CH6ENC_SHIFT                         6                                  /**< Shift value for DMA_CH6ENC */
#define _DMA_CHENC_CH6ENC_MASK                          0x40UL                             /**< Bit mask for DMA_CH6ENC */
#define _DMA_CHENC_CH6ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH6ENC_DEFAULT                        (_DMA_CHENC_CH6ENC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH7ENC                                (0x1UL << 7)                       /**< Channel 7 Enable Clear */
#define _DMA_CHENC_CH7ENC_SHIFT                         7                                  /**< Shift value for DMA_CH7ENC */
#define _DMA_CHENC_CH7ENC_MASK                          0x80UL                             /**< Bit mask for DMA_CH7ENC */
#define _DMA_CHENC_CH7ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH7ENC_DEFAULT                        (_DMA_CHENC_CH7ENC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH8ENC                                (0x1UL << 8)                       /**< Channel 8 Enable Clear */
#define _DMA_CHENC_CH8ENC_SHIFT                         8                                  /**< Shift value for DMA_CH8ENC */
#define _DMA_CHENC_CH8ENC_MASK                          0x100UL                            /**< Bit mask for DMA_CH8ENC */
#define _DMA_CHENC_CH8ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH8ENC_DEFAULT                        (_DMA_CHENC_CH8ENC_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH9ENC                                (0x1UL << 9)                       /**< Channel 9 Enable Clear */
#define _DMA_CHENC_CH9ENC_SHIFT                         9                                  /**< Shift value for DMA_CH9ENC */
#define _DMA_CHENC_CH9ENC_MASK                          0x200UL                            /**< Bit mask for DMA_CH9ENC */
#define _DMA_CHENC_CH9ENC_DEFAULT                       0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH9ENC_DEFAULT                        (_DMA_CHENC_CH9ENC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH10ENC                               (0x1UL << 10)                      /**< Channel 10 Enable Clear */
#define _DMA_CHENC_CH10ENC_SHIFT                        10                                 /**< Shift value for DMA_CH10ENC */
#define _DMA_CHENC_CH10ENC_MASK                         0x400UL                            /**< Bit mask for DMA_CH10ENC */
#define _DMA_CHENC_CH10ENC_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH10ENC_DEFAULT                       (_DMA_CHENC_CH10ENC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH11ENC                               (0x1UL << 11)                      /**< Channel 11 Enable Clear */
#define _DMA_CHENC_CH11ENC_SHIFT                        11                                 /**< Shift value for DMA_CH11ENC */
#define _DMA_CHENC_CH11ENC_MASK                         0x800UL                            /**< Bit mask for DMA_CH11ENC */
#define _DMA_CHENC_CH11ENC_DEFAULT                      0x00000000UL                       /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH11ENC_DEFAULT                       (_DMA_CHENC_CH11ENC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHENC */

/* Bit fields for DMA CHALTS */
#define _DMA_CHALTS_RESETVALUE                          0x00000000UL                         /**< Default value for DMA_CHALTS */
#define _DMA_CHALTS_MASK                                0x00000FFFUL                         /**< Mask for DMA_CHALTS */
#define DMA_CHALTS_CH0ALTS                              (0x1UL << 0)                         /**< Channel 0 Alternate Structure Set */
#define _DMA_CHALTS_CH0ALTS_SHIFT                       0                                    /**< Shift value for DMA_CH0ALTS */
#define _DMA_CHALTS_CH0ALTS_MASK                        0x1UL                                /**< Bit mask for DMA_CH0ALTS */
#define _DMA_CHALTS_CH0ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH0ALTS_DEFAULT                      (_DMA_CHALTS_CH0ALTS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH1ALTS                              (0x1UL << 1)                         /**< Channel 1 Alternate Structure Set */
#define _DMA_CHALTS_CH1ALTS_SHIFT                       1                                    /**< Shift value for DMA_CH1ALTS */
#define _DMA_CHALTS_CH1ALTS_MASK                        0x2UL                                /**< Bit mask for DMA_CH1ALTS */
#define _DMA_CHALTS_CH1ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH1ALTS_DEFAULT                      (_DMA_CHALTS_CH1ALTS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH2ALTS                              (0x1UL << 2)                         /**< Channel 2 Alternate Structure Set */
#define _DMA_CHALTS_CH2ALTS_SHIFT                       2                                    /**< Shift value for DMA_CH2ALTS */
#define _DMA_CHALTS_CH2ALTS_MASK                        0x4UL                                /**< Bit mask for DMA_CH2ALTS */
#define _DMA_CHALTS_CH2ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH2ALTS_DEFAULT                      (_DMA_CHALTS_CH2ALTS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH3ALTS                              (0x1UL << 3)                         /**< Channel 3 Alternate Structure Set */
#define _DMA_CHALTS_CH3ALTS_SHIFT                       3                                    /**< Shift value for DMA_CH3ALTS */
#define _DMA_CHALTS_CH3ALTS_MASK                        0x8UL                                /**< Bit mask for DMA_CH3ALTS */
#define _DMA_CHALTS_CH3ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH3ALTS_DEFAULT                      (_DMA_CHALTS_CH3ALTS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH4ALTS                              (0x1UL << 4)                         /**< Channel 4 Alternate Structure Set */
#define _DMA_CHALTS_CH4ALTS_SHIFT                       4                                    /**< Shift value for DMA_CH4ALTS */
#define _DMA_CHALTS_CH4ALTS_MASK                        0x10UL                               /**< Bit mask for DMA_CH4ALTS */
#define _DMA_CHALTS_CH4ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH4ALTS_DEFAULT                      (_DMA_CHALTS_CH4ALTS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH5ALTS                              (0x1UL << 5)                         /**< Channel 5 Alternate Structure Set */
#define _DMA_CHALTS_CH5ALTS_SHIFT                       5                                    /**< Shift value for DMA_CH5ALTS */
#define _DMA_CHALTS_CH5ALTS_MASK                        0x20UL                               /**< Bit mask for DMA_CH5ALTS */
#define _DMA_CHALTS_CH5ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH5ALTS_DEFAULT                      (_DMA_CHALTS_CH5ALTS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH6ALTS                              (0x1UL << 6)                         /**< Channel 6 Alternate Structure Set */
#define _DMA_CHALTS_CH6ALTS_SHIFT                       6                                    /**< Shift value for DMA_CH6ALTS */
#define _DMA_CHALTS_CH6ALTS_MASK                        0x40UL                               /**< Bit mask for DMA_CH6ALTS */
#define _DMA_CHALTS_CH6ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH6ALTS_DEFAULT                      (_DMA_CHALTS_CH6ALTS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH7ALTS                              (0x1UL << 7)                         /**< Channel 7 Alternate Structure Set */
#define _DMA_CHALTS_CH7ALTS_SHIFT                       7                                    /**< Shift value for DMA_CH7ALTS */
#define _DMA_CHALTS_CH7ALTS_MASK                        0x80UL                               /**< Bit mask for DMA_CH7ALTS */
#define _DMA_CHALTS_CH7ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH7ALTS_DEFAULT                      (_DMA_CHALTS_CH7ALTS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH8ALTS                              (0x1UL << 8)                         /**< Channel 8 Alternate Structure Set */
#define _DMA_CHALTS_CH8ALTS_SHIFT                       8                                    /**< Shift value for DMA_CH8ALTS */
#define _DMA_CHALTS_CH8ALTS_MASK                        0x100UL                              /**< Bit mask for DMA_CH8ALTS */
#define _DMA_CHALTS_CH8ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH8ALTS_DEFAULT                      (_DMA_CHALTS_CH8ALTS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH9ALTS                              (0x1UL << 9)                         /**< Channel 9 Alternate Structure Set */
#define _DMA_CHALTS_CH9ALTS_SHIFT                       9                                    /**< Shift value for DMA_CH9ALTS */
#define _DMA_CHALTS_CH9ALTS_MASK                        0x200UL                              /**< Bit mask for DMA_CH9ALTS */
#define _DMA_CHALTS_CH9ALTS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH9ALTS_DEFAULT                      (_DMA_CHALTS_CH9ALTS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH10ALTS                             (0x1UL << 10)                        /**< Channel 10 Alternate Structure Set */
#define _DMA_CHALTS_CH10ALTS_SHIFT                      10                                   /**< Shift value for DMA_CH10ALTS */
#define _DMA_CHALTS_CH10ALTS_MASK                       0x400UL                              /**< Bit mask for DMA_CH10ALTS */
#define _DMA_CHALTS_CH10ALTS_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH10ALTS_DEFAULT                     (_DMA_CHALTS_CH10ALTS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH11ALTS                             (0x1UL << 11)                        /**< Channel 11 Alternate Structure Set */
#define _DMA_CHALTS_CH11ALTS_SHIFT                      11                                   /**< Shift value for DMA_CH11ALTS */
#define _DMA_CHALTS_CH11ALTS_MASK                       0x800UL                              /**< Bit mask for DMA_CH11ALTS */
#define _DMA_CHALTS_CH11ALTS_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH11ALTS_DEFAULT                     (_DMA_CHALTS_CH11ALTS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHALTS */

/* Bit fields for DMA CHALTC */
#define _DMA_CHALTC_RESETVALUE                          0x00000000UL                         /**< Default value for DMA_CHALTC */
#define _DMA_CHALTC_MASK                                0x00000FFFUL                         /**< Mask for DMA_CHALTC */
#define DMA_CHALTC_CH0ALTC                              (0x1UL << 0)                         /**< Channel 0 Alternate Clear */
#define _DMA_CHALTC_CH0ALTC_SHIFT                       0                                    /**< Shift value for DMA_CH0ALTC */
#define _DMA_CHALTC_CH0ALTC_MASK                        0x1UL                                /**< Bit mask for DMA_CH0ALTC */
#define _DMA_CHALTC_CH0ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH0ALTC_DEFAULT                      (_DMA_CHALTC_CH0ALTC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH1ALTC                              (0x1UL << 1)                         /**< Channel 1 Alternate Clear */
#define _DMA_CHALTC_CH1ALTC_SHIFT                       1                                    /**< Shift value for DMA_CH1ALTC */
#define _DMA_CHALTC_CH1ALTC_MASK                        0x2UL                                /**< Bit mask for DMA_CH1ALTC */
#define _DMA_CHALTC_CH1ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH1ALTC_DEFAULT                      (_DMA_CHALTC_CH1ALTC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH2ALTC                              (0x1UL << 2)                         /**< Channel 2 Alternate Clear */
#define _DMA_CHALTC_CH2ALTC_SHIFT                       2                                    /**< Shift value for DMA_CH2ALTC */
#define _DMA_CHALTC_CH2ALTC_MASK                        0x4UL                                /**< Bit mask for DMA_CH2ALTC */
#define _DMA_CHALTC_CH2ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH2ALTC_DEFAULT                      (_DMA_CHALTC_CH2ALTC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH3ALTC                              (0x1UL << 3)                         /**< Channel 3 Alternate Clear */
#define _DMA_CHALTC_CH3ALTC_SHIFT                       3                                    /**< Shift value for DMA_CH3ALTC */
#define _DMA_CHALTC_CH3ALTC_MASK                        0x8UL                                /**< Bit mask for DMA_CH3ALTC */
#define _DMA_CHALTC_CH3ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH3ALTC_DEFAULT                      (_DMA_CHALTC_CH3ALTC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH4ALTC                              (0x1UL << 4)                         /**< Channel 4 Alternate Clear */
#define _DMA_CHALTC_CH4ALTC_SHIFT                       4                                    /**< Shift value for DMA_CH4ALTC */
#define _DMA_CHALTC_CH4ALTC_MASK                        0x10UL                               /**< Bit mask for DMA_CH4ALTC */
#define _DMA_CHALTC_CH4ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH4ALTC_DEFAULT                      (_DMA_CHALTC_CH4ALTC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH5ALTC                              (0x1UL << 5)                         /**< Channel 5 Alternate Clear */
#define _DMA_CHALTC_CH5ALTC_SHIFT                       5                                    /**< Shift value for DMA_CH5ALTC */
#define _DMA_CHALTC_CH5ALTC_MASK                        0x20UL                               /**< Bit mask for DMA_CH5ALTC */
#define _DMA_CHALTC_CH5ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH5ALTC_DEFAULT                      (_DMA_CHALTC_CH5ALTC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH6ALTC                              (0x1UL << 6)                         /**< Channel 6 Alternate Clear */
#define _DMA_CHALTC_CH6ALTC_SHIFT                       6                                    /**< Shift value for DMA_CH6ALTC */
#define _DMA_CHALTC_CH6ALTC_MASK                        0x40UL                               /**< Bit mask for DMA_CH6ALTC */
#define _DMA_CHALTC_CH6ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH6ALTC_DEFAULT                      (_DMA_CHALTC_CH6ALTC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH7ALTC                              (0x1UL << 7)                         /**< Channel 7 Alternate Clear */
#define _DMA_CHALTC_CH7ALTC_SHIFT                       7                                    /**< Shift value for DMA_CH7ALTC */
#define _DMA_CHALTC_CH7ALTC_MASK                        0x80UL                               /**< Bit mask for DMA_CH7ALTC */
#define _DMA_CHALTC_CH7ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH7ALTC_DEFAULT                      (_DMA_CHALTC_CH7ALTC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH8ALTC                              (0x1UL << 8)                         /**< Channel 8 Alternate Clear */
#define _DMA_CHALTC_CH8ALTC_SHIFT                       8                                    /**< Shift value for DMA_CH8ALTC */
#define _DMA_CHALTC_CH8ALTC_MASK                        0x100UL                              /**< Bit mask for DMA_CH8ALTC */
#define _DMA_CHALTC_CH8ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH8ALTC_DEFAULT                      (_DMA_CHALTC_CH8ALTC_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH9ALTC                              (0x1UL << 9)                         /**< Channel 9 Alternate Clear */
#define _DMA_CHALTC_CH9ALTC_SHIFT                       9                                    /**< Shift value for DMA_CH9ALTC */
#define _DMA_CHALTC_CH9ALTC_MASK                        0x200UL                              /**< Bit mask for DMA_CH9ALTC */
#define _DMA_CHALTC_CH9ALTC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH9ALTC_DEFAULT                      (_DMA_CHALTC_CH9ALTC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH10ALTC                             (0x1UL << 10)                        /**< Channel 10 Alternate Clear */
#define _DMA_CHALTC_CH10ALTC_SHIFT                      10                                   /**< Shift value for DMA_CH10ALTC */
#define _DMA_CHALTC_CH10ALTC_MASK                       0x400UL                              /**< Bit mask for DMA_CH10ALTC */
#define _DMA_CHALTC_CH10ALTC_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH10ALTC_DEFAULT                     (_DMA_CHALTC_CH10ALTC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH11ALTC                             (0x1UL << 11)                        /**< Channel 11 Alternate Clear */
#define _DMA_CHALTC_CH11ALTC_SHIFT                      11                                   /**< Shift value for DMA_CH11ALTC */
#define _DMA_CHALTC_CH11ALTC_MASK                       0x800UL                              /**< Bit mask for DMA_CH11ALTC */
#define _DMA_CHALTC_CH11ALTC_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH11ALTC_DEFAULT                     (_DMA_CHALTC_CH11ALTC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHALTC */

/* Bit fields for DMA CHPRIS */
#define _DMA_CHPRIS_RESETVALUE                          0x00000000UL                         /**< Default value for DMA_CHPRIS */
#define _DMA_CHPRIS_MASK                                0x00000FFFUL                         /**< Mask for DMA_CHPRIS */
#define DMA_CHPRIS_CH0PRIS                              (0x1UL << 0)                         /**< Channel 0 High Priority Set */
#define _DMA_CHPRIS_CH0PRIS_SHIFT                       0                                    /**< Shift value for DMA_CH0PRIS */
#define _DMA_CHPRIS_CH0PRIS_MASK                        0x1UL                                /**< Bit mask for DMA_CH0PRIS */
#define _DMA_CHPRIS_CH0PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH0PRIS_DEFAULT                      (_DMA_CHPRIS_CH0PRIS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH1PRIS                              (0x1UL << 1)                         /**< Channel 1 High Priority Set */
#define _DMA_CHPRIS_CH1PRIS_SHIFT                       1                                    /**< Shift value for DMA_CH1PRIS */
#define _DMA_CHPRIS_CH1PRIS_MASK                        0x2UL                                /**< Bit mask for DMA_CH1PRIS */
#define _DMA_CHPRIS_CH1PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH1PRIS_DEFAULT                      (_DMA_CHPRIS_CH1PRIS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH2PRIS                              (0x1UL << 2)                         /**< Channel 2 High Priority Set */
#define _DMA_CHPRIS_CH2PRIS_SHIFT                       2                                    /**< Shift value for DMA_CH2PRIS */
#define _DMA_CHPRIS_CH2PRIS_MASK                        0x4UL                                /**< Bit mask for DMA_CH2PRIS */
#define _DMA_CHPRIS_CH2PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH2PRIS_DEFAULT                      (_DMA_CHPRIS_CH2PRIS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH3PRIS                              (0x1UL << 3)                         /**< Channel 3 High Priority Set */
#define _DMA_CHPRIS_CH3PRIS_SHIFT                       3                                    /**< Shift value for DMA_CH3PRIS */
#define _DMA_CHPRIS_CH3PRIS_MASK                        0x8UL                                /**< Bit mask for DMA_CH3PRIS */
#define _DMA_CHPRIS_CH3PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH3PRIS_DEFAULT                      (_DMA_CHPRIS_CH3PRIS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH4PRIS                              (0x1UL << 4)                         /**< Channel 4 High Priority Set */
#define _DMA_CHPRIS_CH4PRIS_SHIFT                       4                                    /**< Shift value for DMA_CH4PRIS */
#define _DMA_CHPRIS_CH4PRIS_MASK                        0x10UL                               /**< Bit mask for DMA_CH4PRIS */
#define _DMA_CHPRIS_CH4PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH4PRIS_DEFAULT                      (_DMA_CHPRIS_CH4PRIS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH5PRIS                              (0x1UL << 5)                         /**< Channel 5 High Priority Set */
#define _DMA_CHPRIS_CH5PRIS_SHIFT                       5                                    /**< Shift value for DMA_CH5PRIS */
#define _DMA_CHPRIS_CH5PRIS_MASK                        0x20UL                               /**< Bit mask for DMA_CH5PRIS */
#define _DMA_CHPRIS_CH5PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH5PRIS_DEFAULT                      (_DMA_CHPRIS_CH5PRIS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH6PRIS                              (0x1UL << 6)                         /**< Channel 6 High Priority Set */
#define _DMA_CHPRIS_CH6PRIS_SHIFT                       6                                    /**< Shift value for DMA_CH6PRIS */
#define _DMA_CHPRIS_CH6PRIS_MASK                        0x40UL                               /**< Bit mask for DMA_CH6PRIS */
#define _DMA_CHPRIS_CH6PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH6PRIS_DEFAULT                      (_DMA_CHPRIS_CH6PRIS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH7PRIS                              (0x1UL << 7)                         /**< Channel 7 High Priority Set */
#define _DMA_CHPRIS_CH7PRIS_SHIFT                       7                                    /**< Shift value for DMA_CH7PRIS */
#define _DMA_CHPRIS_CH7PRIS_MASK                        0x80UL                               /**< Bit mask for DMA_CH7PRIS */
#define _DMA_CHPRIS_CH7PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH7PRIS_DEFAULT                      (_DMA_CHPRIS_CH7PRIS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH8PRIS                              (0x1UL << 8)                         /**< Channel 8 High Priority Set */
#define _DMA_CHPRIS_CH8PRIS_SHIFT                       8                                    /**< Shift value for DMA_CH8PRIS */
#define _DMA_CHPRIS_CH8PRIS_MASK                        0x100UL                              /**< Bit mask for DMA_CH8PRIS */
#define _DMA_CHPRIS_CH8PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH8PRIS_DEFAULT                      (_DMA_CHPRIS_CH8PRIS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH9PRIS                              (0x1UL << 9)                         /**< Channel 9 High Priority Set */
#define _DMA_CHPRIS_CH9PRIS_SHIFT                       9                                    /**< Shift value for DMA_CH9PRIS */
#define _DMA_CHPRIS_CH9PRIS_MASK                        0x200UL                              /**< Bit mask for DMA_CH9PRIS */
#define _DMA_CHPRIS_CH9PRIS_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH9PRIS_DEFAULT                      (_DMA_CHPRIS_CH9PRIS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH10PRIS                             (0x1UL << 10)                        /**< Channel 10 High Priority Set */
#define _DMA_CHPRIS_CH10PRIS_SHIFT                      10                                   /**< Shift value for DMA_CH10PRIS */
#define _DMA_CHPRIS_CH10PRIS_MASK                       0x400UL                              /**< Bit mask for DMA_CH10PRIS */
#define _DMA_CHPRIS_CH10PRIS_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH10PRIS_DEFAULT                     (_DMA_CHPRIS_CH10PRIS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH11PRIS                             (0x1UL << 11)                        /**< Channel 11 High Priority Set */
#define _DMA_CHPRIS_CH11PRIS_SHIFT                      11                                   /**< Shift value for DMA_CH11PRIS */
#define _DMA_CHPRIS_CH11PRIS_MASK                       0x800UL                              /**< Bit mask for DMA_CH11PRIS */
#define _DMA_CHPRIS_CH11PRIS_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH11PRIS_DEFAULT                     (_DMA_CHPRIS_CH11PRIS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHPRIS */

/* Bit fields for DMA CHPRIC */
#define _DMA_CHPRIC_RESETVALUE                          0x00000000UL                         /**< Default value for DMA_CHPRIC */
#define _DMA_CHPRIC_MASK                                0x00000FFFUL                         /**< Mask for DMA_CHPRIC */
#define DMA_CHPRIC_CH0PRIC                              (0x1UL << 0)                         /**< Channel 0 High Priority Clear */
#define _DMA_CHPRIC_CH0PRIC_SHIFT                       0                                    /**< Shift value for DMA_CH0PRIC */
#define _DMA_CHPRIC_CH0PRIC_MASK                        0x1UL                                /**< Bit mask for DMA_CH0PRIC */
#define _DMA_CHPRIC_CH0PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH0PRIC_DEFAULT                      (_DMA_CHPRIC_CH0PRIC_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH1PRIC                              (0x1UL << 1)                         /**< Channel 1 High Priority Clear */
#define _DMA_CHPRIC_CH1PRIC_SHIFT                       1                                    /**< Shift value for DMA_CH1PRIC */
#define _DMA_CHPRIC_CH1PRIC_MASK                        0x2UL                                /**< Bit mask for DMA_CH1PRIC */
#define _DMA_CHPRIC_CH1PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH1PRIC_DEFAULT                      (_DMA_CHPRIC_CH1PRIC_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH2PRIC                              (0x1UL << 2)                         /**< Channel 2 High Priority Clear */
#define _DMA_CHPRIC_CH2PRIC_SHIFT                       2                                    /**< Shift value for DMA_CH2PRIC */
#define _DMA_CHPRIC_CH2PRIC_MASK                        0x4UL                                /**< Bit mask for DMA_CH2PRIC */
#define _DMA_CHPRIC_CH2PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH2PRIC_DEFAULT                      (_DMA_CHPRIC_CH2PRIC_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH3PRIC                              (0x1UL << 3)                         /**< Channel 3 High Priority Clear */
#define _DMA_CHPRIC_CH3PRIC_SHIFT                       3                                    /**< Shift value for DMA_CH3PRIC */
#define _DMA_CHPRIC_CH3PRIC_MASK                        0x8UL                                /**< Bit mask for DMA_CH3PRIC */
#define _DMA_CHPRIC_CH3PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH3PRIC_DEFAULT                      (_DMA_CHPRIC_CH3PRIC_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH4PRIC                              (0x1UL << 4)                         /**< Channel 4 High Priority Clear */
#define _DMA_CHPRIC_CH4PRIC_SHIFT                       4                                    /**< Shift value for DMA_CH4PRIC */
#define _DMA_CHPRIC_CH4PRIC_MASK                        0x10UL                               /**< Bit mask for DMA_CH4PRIC */
#define _DMA_CHPRIC_CH4PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH4PRIC_DEFAULT                      (_DMA_CHPRIC_CH4PRIC_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH5PRIC                              (0x1UL << 5)                         /**< Channel 5 High Priority Clear */
#define _DMA_CHPRIC_CH5PRIC_SHIFT                       5                                    /**< Shift value for DMA_CH5PRIC */
#define _DMA_CHPRIC_CH5PRIC_MASK                        0x20UL                               /**< Bit mask for DMA_CH5PRIC */
#define _DMA_CHPRIC_CH5PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH5PRIC_DEFAULT                      (_DMA_CHPRIC_CH5PRIC_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH6PRIC                              (0x1UL << 6)                         /**< Channel 6 High Priority Clear */
#define _DMA_CHPRIC_CH6PRIC_SHIFT                       6                                    /**< Shift value for DMA_CH6PRIC */
#define _DMA_CHPRIC_CH6PRIC_MASK                        0x40UL                               /**< Bit mask for DMA_CH6PRIC */
#define _DMA_CHPRIC_CH6PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH6PRIC_DEFAULT                      (_DMA_CHPRIC_CH6PRIC_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH7PRIC                              (0x1UL << 7)                         /**< Channel 7 High Priority Clear */
#define _DMA_CHPRIC_CH7PRIC_SHIFT                       7                                    /**< Shift value for DMA_CH7PRIC */
#define _DMA_CHPRIC_CH7PRIC_MASK                        0x80UL                               /**< Bit mask for DMA_CH7PRIC */
#define _DMA_CHPRIC_CH7PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH7PRIC_DEFAULT                      (_DMA_CHPRIC_CH7PRIC_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH8PRIC                              (0x1UL << 8)                         /**< Channel 8 High Priority Clear */
#define _DMA_CHPRIC_CH8PRIC_SHIFT                       8                                    /**< Shift value for DMA_CH8PRIC */
#define _DMA_CHPRIC_CH8PRIC_MASK                        0x100UL                              /**< Bit mask for DMA_CH8PRIC */
#define _DMA_CHPRIC_CH8PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH8PRIC_DEFAULT                      (_DMA_CHPRIC_CH8PRIC_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH9PRIC                              (0x1UL << 9)                         /**< Channel 9 High Priority Clear */
#define _DMA_CHPRIC_CH9PRIC_SHIFT                       9                                    /**< Shift value for DMA_CH9PRIC */
#define _DMA_CHPRIC_CH9PRIC_MASK                        0x200UL                              /**< Bit mask for DMA_CH9PRIC */
#define _DMA_CHPRIC_CH9PRIC_DEFAULT                     0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH9PRIC_DEFAULT                      (_DMA_CHPRIC_CH9PRIC_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH10PRIC                             (0x1UL << 10)                        /**< Channel 10 High Priority Clear */
#define _DMA_CHPRIC_CH10PRIC_SHIFT                      10                                   /**< Shift value for DMA_CH10PRIC */
#define _DMA_CHPRIC_CH10PRIC_MASK                       0x400UL                              /**< Bit mask for DMA_CH10PRIC */
#define _DMA_CHPRIC_CH10PRIC_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH10PRIC_DEFAULT                     (_DMA_CHPRIC_CH10PRIC_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH11PRIC                             (0x1UL << 11)                        /**< Channel 11 High Priority Clear */
#define _DMA_CHPRIC_CH11PRIC_SHIFT                      11                                   /**< Shift value for DMA_CH11PRIC */
#define _DMA_CHPRIC_CH11PRIC_MASK                       0x800UL                              /**< Bit mask for DMA_CH11PRIC */
#define _DMA_CHPRIC_CH11PRIC_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH11PRIC_DEFAULT                     (_DMA_CHPRIC_CH11PRIC_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHPRIC */

/* Bit fields for DMA ERRORC */
#define _DMA_ERRORC_RESETVALUE                          0x00000000UL                      /**< Default value for DMA_ERRORC */
#define _DMA_ERRORC_MASK                                0x00000001UL                      /**< Mask for DMA_ERRORC */
#define DMA_ERRORC_ERRORC                               (0x1UL << 0)                      /**< Bus Error Clear */
#define _DMA_ERRORC_ERRORC_SHIFT                        0                                 /**< Shift value for DMA_ERRORC */
#define _DMA_ERRORC_ERRORC_MASK                         0x1UL                             /**< Bit mask for DMA_ERRORC */
#define _DMA_ERRORC_ERRORC_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for DMA_ERRORC */
#define DMA_ERRORC_ERRORC_DEFAULT                       (_DMA_ERRORC_ERRORC_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_ERRORC */

/* Bit fields for DMA CHREQSTATUS */
#define _DMA_CHREQSTATUS_RESETVALUE                     0x00000000UL                                   /**< Default value for DMA_CHREQSTATUS */
#define _DMA_CHREQSTATUS_MASK                           0x00000FFFUL                                   /**< Mask for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH0REQSTATUS                    (0x1UL << 0)                                   /**< Channel 0 Request Status */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_SHIFT             0                                              /**< Shift value for DMA_CH0REQSTATUS */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_MASK              0x1UL                                          /**< Bit mask for DMA_CH0REQSTATUS */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH1REQSTATUS                    (0x1UL << 1)                                   /**< Channel 1 Request Status */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_SHIFT             1                                              /**< Shift value for DMA_CH1REQSTATUS */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_MASK              0x2UL                                          /**< Bit mask for DMA_CH1REQSTATUS */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH2REQSTATUS                    (0x1UL << 2)                                   /**< Channel 2 Request Status */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_SHIFT             2                                              /**< Shift value for DMA_CH2REQSTATUS */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_MASK              0x4UL                                          /**< Bit mask for DMA_CH2REQSTATUS */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH3REQSTATUS                    (0x1UL << 3)                                   /**< Channel 3 Request Status */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_SHIFT             3                                              /**< Shift value for DMA_CH3REQSTATUS */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_MASK              0x8UL                                          /**< Bit mask for DMA_CH3REQSTATUS */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH4REQSTATUS                    (0x1UL << 4)                                   /**< Channel 4 Request Status */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_SHIFT             4                                              /**< Shift value for DMA_CH4REQSTATUS */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_MASK              0x10UL                                         /**< Bit mask for DMA_CH4REQSTATUS */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH5REQSTATUS                    (0x1UL << 5)                                   /**< Channel 5 Request Status */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_SHIFT             5                                              /**< Shift value for DMA_CH5REQSTATUS */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_MASK              0x20UL                                         /**< Bit mask for DMA_CH5REQSTATUS */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH6REQSTATUS                    (0x1UL << 6)                                   /**< Channel 6 Request Status */
#define _DMA_CHREQSTATUS_CH6REQSTATUS_SHIFT             6                                              /**< Shift value for DMA_CH6REQSTATUS */
#define _DMA_CHREQSTATUS_CH6REQSTATUS_MASK              0x40UL                                         /**< Bit mask for DMA_CH6REQSTATUS */
#define _DMA_CHREQSTATUS_CH6REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH6REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH6REQSTATUS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH7REQSTATUS                    (0x1UL << 7)                                   /**< Channel 7 Request Status */
#define _DMA_CHREQSTATUS_CH7REQSTATUS_SHIFT             7                                              /**< Shift value for DMA_CH7REQSTATUS */
#define _DMA_CHREQSTATUS_CH7REQSTATUS_MASK              0x80UL                                         /**< Bit mask for DMA_CH7REQSTATUS */
#define _DMA_CHREQSTATUS_CH7REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH7REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH7REQSTATUS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH8REQSTATUS                    (0x1UL << 8)                                   /**< Channel 8 Request Status */
#define _DMA_CHREQSTATUS_CH8REQSTATUS_SHIFT             8                                              /**< Shift value for DMA_CH8REQSTATUS */
#define _DMA_CHREQSTATUS_CH8REQSTATUS_MASK              0x100UL                                        /**< Bit mask for DMA_CH8REQSTATUS */
#define _DMA_CHREQSTATUS_CH8REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH8REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH8REQSTATUS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH9REQSTATUS                    (0x1UL << 9)                                   /**< Channel 9 Request Status */
#define _DMA_CHREQSTATUS_CH9REQSTATUS_SHIFT             9                                              /**< Shift value for DMA_CH9REQSTATUS */
#define _DMA_CHREQSTATUS_CH9REQSTATUS_MASK              0x200UL                                        /**< Bit mask for DMA_CH9REQSTATUS */
#define _DMA_CHREQSTATUS_CH9REQSTATUS_DEFAULT           0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH9REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH9REQSTATUS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH10REQSTATUS                   (0x1UL << 10)                                  /**< Channel 10 Request Status */
#define _DMA_CHREQSTATUS_CH10REQSTATUS_SHIFT            10                                             /**< Shift value for DMA_CH10REQSTATUS */
#define _DMA_CHREQSTATUS_CH10REQSTATUS_MASK             0x400UL                                        /**< Bit mask for DMA_CH10REQSTATUS */
#define _DMA_CHREQSTATUS_CH10REQSTATUS_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH10REQSTATUS_DEFAULT           (_DMA_CHREQSTATUS_CH10REQSTATUS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH11REQSTATUS                   (0x1UL << 11)                                  /**< Channel 11 Request Status */
#define _DMA_CHREQSTATUS_CH11REQSTATUS_SHIFT            11                                             /**< Shift value for DMA_CH11REQSTATUS */
#define _DMA_CHREQSTATUS_CH11REQSTATUS_MASK             0x800UL                                        /**< Bit mask for DMA_CH11REQSTATUS */
#define _DMA_CHREQSTATUS_CH11REQSTATUS_DEFAULT          0x00000000UL                                   /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH11REQSTATUS_DEFAULT           (_DMA_CHREQSTATUS_CH11REQSTATUS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */

/* Bit fields for DMA CHSREQSTATUS */
#define _DMA_CHSREQSTATUS_RESETVALUE                    0x00000000UL                                     /**< Default value for DMA_CHSREQSTATUS */
#define _DMA_CHSREQSTATUS_MASK                          0x00000FFFUL                                     /**< Mask for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH0SREQSTATUS                  (0x1UL << 0)                                     /**< Channel 0 Single Request Status */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_SHIFT           0                                                /**< Shift value for DMA_CH0SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_MASK            0x1UL                                            /**< Bit mask for DMA_CH0SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH1SREQSTATUS                  (0x1UL << 1)                                     /**< Channel 1 Single Request Status */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_SHIFT           1                                                /**< Shift value for DMA_CH1SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_MASK            0x2UL                                            /**< Bit mask for DMA_CH1SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH2SREQSTATUS                  (0x1UL << 2)                                     /**< Channel 2 Single Request Status */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_SHIFT           2                                                /**< Shift value for DMA_CH2SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_MASK            0x4UL                                            /**< Bit mask for DMA_CH2SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH3SREQSTATUS                  (0x1UL << 3)                                     /**< Channel 3 Single Request Status */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_SHIFT           3                                                /**< Shift value for DMA_CH3SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_MASK            0x8UL                                            /**< Bit mask for DMA_CH3SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH4SREQSTATUS                  (0x1UL << 4)                                     /**< Channel 4 Single Request Status */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_SHIFT           4                                                /**< Shift value for DMA_CH4SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_MASK            0x10UL                                           /**< Bit mask for DMA_CH4SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH5SREQSTATUS                  (0x1UL << 5)                                     /**< Channel 5 Single Request Status */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_SHIFT           5                                                /**< Shift value for DMA_CH5SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_MASK            0x20UL                                           /**< Bit mask for DMA_CH5SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH6SREQSTATUS                  (0x1UL << 6)                                     /**< Channel 6 Single Request Status */
#define _DMA_CHSREQSTATUS_CH6SREQSTATUS_SHIFT           6                                                /**< Shift value for DMA_CH6SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH6SREQSTATUS_MASK            0x40UL                                           /**< Bit mask for DMA_CH6SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH6SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH6SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH6SREQSTATUS_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH7SREQSTATUS                  (0x1UL << 7)                                     /**< Channel 7 Single Request Status */
#define _DMA_CHSREQSTATUS_CH7SREQSTATUS_SHIFT           7                                                /**< Shift value for DMA_CH7SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH7SREQSTATUS_MASK            0x80UL                                           /**< Bit mask for DMA_CH7SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH7SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH7SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH7SREQSTATUS_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH8SREQSTATUS                  (0x1UL << 8)                                     /**< Channel 8 Single Request Status */
#define _DMA_CHSREQSTATUS_CH8SREQSTATUS_SHIFT           8                                                /**< Shift value for DMA_CH8SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH8SREQSTATUS_MASK            0x100UL                                          /**< Bit mask for DMA_CH8SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH8SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH8SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH8SREQSTATUS_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH9SREQSTATUS                  (0x1UL << 9)                                     /**< Channel 9 Single Request Status */
#define _DMA_CHSREQSTATUS_CH9SREQSTATUS_SHIFT           9                                                /**< Shift value for DMA_CH9SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH9SREQSTATUS_MASK            0x200UL                                          /**< Bit mask for DMA_CH9SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH9SREQSTATUS_DEFAULT         0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH9SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH9SREQSTATUS_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH10SREQSTATUS                 (0x1UL << 10)                                    /**< Channel 10 Single Request Status */
#define _DMA_CHSREQSTATUS_CH10SREQSTATUS_SHIFT          10                                               /**< Shift value for DMA_CH10SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH10SREQSTATUS_MASK           0x400UL                                          /**< Bit mask for DMA_CH10SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH10SREQSTATUS_DEFAULT        0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH10SREQSTATUS_DEFAULT         (_DMA_CHSREQSTATUS_CH10SREQSTATUS_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH11SREQSTATUS                 (0x1UL << 11)                                    /**< Channel 11 Single Request Status */
#define _DMA_CHSREQSTATUS_CH11SREQSTATUS_SHIFT          11                                               /**< Shift value for DMA_CH11SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH11SREQSTATUS_MASK           0x800UL                                          /**< Bit mask for DMA_CH11SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH11SREQSTATUS_DEFAULT        0x00000000UL                                     /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH11SREQSTATUS_DEFAULT         (_DMA_CHSREQSTATUS_CH11SREQSTATUS_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */

/* Bit fields for DMA IF */
#define _DMA_IF_RESETVALUE                              0x00000000UL                     /**< Default value for DMA_IF */
#define _DMA_IF_MASK                                    0x80000FFFUL                     /**< Mask for DMA_IF */
#define DMA_IF_CH0DONE                                  (0x1UL << 0)                     /**< DMA Channel 0 Complete Interrupt Flag */
#define _DMA_IF_CH0DONE_SHIFT                           0                                /**< Shift value for DMA_CH0DONE */
#define _DMA_IF_CH0DONE_MASK                            0x1UL                            /**< Bit mask for DMA_CH0DONE */
#define _DMA_IF_CH0DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH0DONE_DEFAULT                          (_DMA_IF_CH0DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH1DONE                                  (0x1UL << 1)                     /**< DMA Channel 1 Complete Interrupt Flag */
#define _DMA_IF_CH1DONE_SHIFT                           1                                /**< Shift value for DMA_CH1DONE */
#define _DMA_IF_CH1DONE_MASK                            0x2UL                            /**< Bit mask for DMA_CH1DONE */
#define _DMA_IF_CH1DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH1DONE_DEFAULT                          (_DMA_IF_CH1DONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH2DONE                                  (0x1UL << 2)                     /**< DMA Channel 2 Complete Interrupt Flag */
#define _DMA_IF_CH2DONE_SHIFT                           2                                /**< Shift value for DMA_CH2DONE */
#define _DMA_IF_CH2DONE_MASK                            0x4UL                            /**< Bit mask for DMA_CH2DONE */
#define _DMA_IF_CH2DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH2DONE_DEFAULT                          (_DMA_IF_CH2DONE_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH3DONE                                  (0x1UL << 3)                     /**< DMA Channel 3 Complete Interrupt Flag */
#define _DMA_IF_CH3DONE_SHIFT                           3                                /**< Shift value for DMA_CH3DONE */
#define _DMA_IF_CH3DONE_MASK                            0x8UL                            /**< Bit mask for DMA_CH3DONE */
#define _DMA_IF_CH3DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH3DONE_DEFAULT                          (_DMA_IF_CH3DONE_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH4DONE                                  (0x1UL << 4)                     /**< DMA Channel 4 Complete Interrupt Flag */
#define _DMA_IF_CH4DONE_SHIFT                           4                                /**< Shift value for DMA_CH4DONE */
#define _DMA_IF_CH4DONE_MASK                            0x10UL                           /**< Bit mask for DMA_CH4DONE */
#define _DMA_IF_CH4DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH4DONE_DEFAULT                          (_DMA_IF_CH4DONE_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH5DONE                                  (0x1UL << 5)                     /**< DMA Channel 5 Complete Interrupt Flag */
#define _DMA_IF_CH5DONE_SHIFT                           5                                /**< Shift value for DMA_CH5DONE */
#define _DMA_IF_CH5DONE_MASK                            0x20UL                           /**< Bit mask for DMA_CH5DONE */
#define _DMA_IF_CH5DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH5DONE_DEFAULT                          (_DMA_IF_CH5DONE_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH6DONE                                  (0x1UL << 6)                     /**< DMA Channel 6 Complete Interrupt Flag */
#define _DMA_IF_CH6DONE_SHIFT                           6                                /**< Shift value for DMA_CH6DONE */
#define _DMA_IF_CH6DONE_MASK                            0x40UL                           /**< Bit mask for DMA_CH6DONE */
#define _DMA_IF_CH6DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH6DONE_DEFAULT                          (_DMA_IF_CH6DONE_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH7DONE                                  (0x1UL << 7)                     /**< DMA Channel 7 Complete Interrupt Flag */
#define _DMA_IF_CH7DONE_SHIFT                           7                                /**< Shift value for DMA_CH7DONE */
#define _DMA_IF_CH7DONE_MASK                            0x80UL                           /**< Bit mask for DMA_CH7DONE */
#define _DMA_IF_CH7DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH7DONE_DEFAULT                          (_DMA_IF_CH7DONE_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH8DONE                                  (0x1UL << 8)                     /**< DMA Channel 8 Complete Interrupt Flag */
#define _DMA_IF_CH8DONE_SHIFT                           8                                /**< Shift value for DMA_CH8DONE */
#define _DMA_IF_CH8DONE_MASK                            0x100UL                          /**< Bit mask for DMA_CH8DONE */
#define _DMA_IF_CH8DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH8DONE_DEFAULT                          (_DMA_IF_CH8DONE_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH9DONE                                  (0x1UL << 9)                     /**< DMA Channel 9 Complete Interrupt Flag */
#define _DMA_IF_CH9DONE_SHIFT                           9                                /**< Shift value for DMA_CH9DONE */
#define _DMA_IF_CH9DONE_MASK                            0x200UL                          /**< Bit mask for DMA_CH9DONE */
#define _DMA_IF_CH9DONE_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH9DONE_DEFAULT                          (_DMA_IF_CH9DONE_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH10DONE                                 (0x1UL << 10)                    /**< DMA Channel 10 Complete Interrupt Flag */
#define _DMA_IF_CH10DONE_SHIFT                          10                               /**< Shift value for DMA_CH10DONE */
#define _DMA_IF_CH10DONE_MASK                           0x400UL                          /**< Bit mask for DMA_CH10DONE */
#define _DMA_IF_CH10DONE_DEFAULT                        0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH10DONE_DEFAULT                         (_DMA_IF_CH10DONE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH11DONE                                 (0x1UL << 11)                    /**< DMA Channel 11 Complete Interrupt Flag */
#define _DMA_IF_CH11DONE_SHIFT                          11                               /**< Shift value for DMA_CH11DONE */
#define _DMA_IF_CH11DONE_MASK                           0x800UL                          /**< Bit mask for DMA_CH11DONE */
#define _DMA_IF_CH11DONE_DEFAULT                        0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH11DONE_DEFAULT                         (_DMA_IF_CH11DONE_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_ERR                                      (0x1UL << 31)                    /**< DMA Error Interrupt Flag */
#define _DMA_IF_ERR_SHIFT                               31                               /**< Shift value for DMA_ERR */
#define _DMA_IF_ERR_MASK                                0x80000000UL                     /**< Bit mask for DMA_ERR */
#define _DMA_IF_ERR_DEFAULT                             0x00000000UL                     /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_ERR_DEFAULT                              (_DMA_IF_ERR_DEFAULT << 31)      /**< Shifted mode DEFAULT for DMA_IF */

/* Bit fields for DMA IFS */
#define _DMA_IFS_RESETVALUE                             0x00000000UL                      /**< Default value for DMA_IFS */
#define _DMA_IFS_MASK                                   0x80000FFFUL                      /**< Mask for DMA_IFS */
#define DMA_IFS_CH0DONE                                 (0x1UL << 0)                      /**< DMA Channel 0 Complete Interrupt Flag Set */
#define _DMA_IFS_CH0DONE_SHIFT                          0                                 /**< Shift value for DMA_CH0DONE */
#define _DMA_IFS_CH0DONE_MASK                           0x1UL                             /**< Bit mask for DMA_CH0DONE */
#define _DMA_IFS_CH0DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH0DONE_DEFAULT                         (_DMA_IFS_CH0DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH1DONE                                 (0x1UL << 1)                      /**< DMA Channel 1 Complete Interrupt Flag Set */
#define _DMA_IFS_CH1DONE_SHIFT                          1                                 /**< Shift value for DMA_CH1DONE */
#define _DMA_IFS_CH1DONE_MASK                           0x2UL                             /**< Bit mask for DMA_CH1DONE */
#define _DMA_IFS_CH1DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH1DONE_DEFAULT                         (_DMA_IFS_CH1DONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH2DONE                                 (0x1UL << 2)                      /**< DMA Channel 2 Complete Interrupt Flag Set */
#define _DMA_IFS_CH2DONE_SHIFT                          2                                 /**< Shift value for DMA_CH2DONE */
#define _DMA_IFS_CH2DONE_MASK                           0x4UL                             /**< Bit mask for DMA_CH2DONE */
#define _DMA_IFS_CH2DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH2DONE_DEFAULT                         (_DMA_IFS_CH2DONE_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH3DONE                                 (0x1UL << 3)                      /**< DMA Channel 3 Complete Interrupt Flag Set */
#define _DMA_IFS_CH3DONE_SHIFT                          3                                 /**< Shift value for DMA_CH3DONE */
#define _DMA_IFS_CH3DONE_MASK                           0x8UL                             /**< Bit mask for DMA_CH3DONE */
#define _DMA_IFS_CH3DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH3DONE_DEFAULT                         (_DMA_IFS_CH3DONE_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH4DONE                                 (0x1UL << 4)                      /**< DMA Channel 4 Complete Interrupt Flag Set */
#define _DMA_IFS_CH4DONE_SHIFT                          4                                 /**< Shift value for DMA_CH4DONE */
#define _DMA_IFS_CH4DONE_MASK                           0x10UL                            /**< Bit mask for DMA_CH4DONE */
#define _DMA_IFS_CH4DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH4DONE_DEFAULT                         (_DMA_IFS_CH4DONE_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH5DONE                                 (0x1UL << 5)                      /**< DMA Channel 5 Complete Interrupt Flag Set */
#define _DMA_IFS_CH5DONE_SHIFT                          5                                 /**< Shift value for DMA_CH5DONE */
#define _DMA_IFS_CH5DONE_MASK                           0x20UL                            /**< Bit mask for DMA_CH5DONE */
#define _DMA_IFS_CH5DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH5DONE_DEFAULT                         (_DMA_IFS_CH5DONE_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH6DONE                                 (0x1UL << 6)                      /**< DMA Channel 6 Complete Interrupt Flag Set */
#define _DMA_IFS_CH6DONE_SHIFT                          6                                 /**< Shift value for DMA_CH6DONE */
#define _DMA_IFS_CH6DONE_MASK                           0x40UL                            /**< Bit mask for DMA_CH6DONE */
#define _DMA_IFS_CH6DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH6DONE_DEFAULT                         (_DMA_IFS_CH6DONE_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH7DONE                                 (0x1UL << 7)                      /**< DMA Channel 7 Complete Interrupt Flag Set */
#define _DMA_IFS_CH7DONE_SHIFT                          7                                 /**< Shift value for DMA_CH7DONE */
#define _DMA_IFS_CH7DONE_MASK                           0x80UL                            /**< Bit mask for DMA_CH7DONE */
#define _DMA_IFS_CH7DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH7DONE_DEFAULT                         (_DMA_IFS_CH7DONE_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH8DONE                                 (0x1UL << 8)                      /**< DMA Channel 8 Complete Interrupt Flag Set */
#define _DMA_IFS_CH8DONE_SHIFT                          8                                 /**< Shift value for DMA_CH8DONE */
#define _DMA_IFS_CH8DONE_MASK                           0x100UL                           /**< Bit mask for DMA_CH8DONE */
#define _DMA_IFS_CH8DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH8DONE_DEFAULT                         (_DMA_IFS_CH8DONE_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH9DONE                                 (0x1UL << 9)                      /**< DMA Channel 9 Complete Interrupt Flag Set */
#define _DMA_IFS_CH9DONE_SHIFT                          9                                 /**< Shift value for DMA_CH9DONE */
#define _DMA_IFS_CH9DONE_MASK                           0x200UL                           /**< Bit mask for DMA_CH9DONE */
#define _DMA_IFS_CH9DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH9DONE_DEFAULT                         (_DMA_IFS_CH9DONE_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH10DONE                                (0x1UL << 10)                     /**< DMA Channel 10 Complete Interrupt Flag Set */
#define _DMA_IFS_CH10DONE_SHIFT                         10                                /**< Shift value for DMA_CH10DONE */
#define _DMA_IFS_CH10DONE_MASK                          0x400UL                           /**< Bit mask for DMA_CH10DONE */
#define _DMA_IFS_CH10DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH10DONE_DEFAULT                        (_DMA_IFS_CH10DONE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH11DONE                                (0x1UL << 11)                     /**< DMA Channel 11 Complete Interrupt Flag Set */
#define _DMA_IFS_CH11DONE_SHIFT                         11                                /**< Shift value for DMA_CH11DONE */
#define _DMA_IFS_CH11DONE_MASK                          0x800UL                           /**< Bit mask for DMA_CH11DONE */
#define _DMA_IFS_CH11DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH11DONE_DEFAULT                        (_DMA_IFS_CH11DONE_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_ERR                                     (0x1UL << 31)                     /**< DMA Error Interrupt Flag Set */
#define _DMA_IFS_ERR_SHIFT                              31                                /**< Shift value for DMA_ERR */
#define _DMA_IFS_ERR_MASK                               0x80000000UL                      /**< Bit mask for DMA_ERR */
#define _DMA_IFS_ERR_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_ERR_DEFAULT                             (_DMA_IFS_ERR_DEFAULT << 31)      /**< Shifted mode DEFAULT for DMA_IFS */

/* Bit fields for DMA IFC */
#define _DMA_IFC_RESETVALUE                             0x00000000UL                      /**< Default value for DMA_IFC */
#define _DMA_IFC_MASK                                   0x80000FFFUL                      /**< Mask for DMA_IFC */
#define DMA_IFC_CH0DONE                                 (0x1UL << 0)                      /**< DMA Channel 0 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH0DONE_SHIFT                          0                                 /**< Shift value for DMA_CH0DONE */
#define _DMA_IFC_CH0DONE_MASK                           0x1UL                             /**< Bit mask for DMA_CH0DONE */
#define _DMA_IFC_CH0DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH0DONE_DEFAULT                         (_DMA_IFC_CH0DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH1DONE                                 (0x1UL << 1)                      /**< DMA Channel 1 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH1DONE_SHIFT                          1                                 /**< Shift value for DMA_CH1DONE */
#define _DMA_IFC_CH1DONE_MASK                           0x2UL                             /**< Bit mask for DMA_CH1DONE */
#define _DMA_IFC_CH1DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH1DONE_DEFAULT                         (_DMA_IFC_CH1DONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH2DONE                                 (0x1UL << 2)                      /**< DMA Channel 2 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH2DONE_SHIFT                          2                                 /**< Shift value for DMA_CH2DONE */
#define _DMA_IFC_CH2DONE_MASK                           0x4UL                             /**< Bit mask for DMA_CH2DONE */
#define _DMA_IFC_CH2DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH2DONE_DEFAULT                         (_DMA_IFC_CH2DONE_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH3DONE                                 (0x1UL << 3)                      /**< DMA Channel 3 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH3DONE_SHIFT                          3                                 /**< Shift value for DMA_CH3DONE */
#define _DMA_IFC_CH3DONE_MASK                           0x8UL                             /**< Bit mask for DMA_CH3DONE */
#define _DMA_IFC_CH3DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH3DONE_DEFAULT                         (_DMA_IFC_CH3DONE_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH4DONE                                 (0x1UL << 4)                      /**< DMA Channel 4 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH4DONE_SHIFT                          4                                 /**< Shift value for DMA_CH4DONE */
#define _DMA_IFC_CH4DONE_MASK                           0x10UL                            /**< Bit mask for DMA_CH4DONE */
#define _DMA_IFC_CH4DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH4DONE_DEFAULT                         (_DMA_IFC_CH4DONE_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH5DONE                                 (0x1UL << 5)                      /**< DMA Channel 5 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH5DONE_SHIFT                          5                                 /**< Shift value for DMA_CH5DONE */
#define _DMA_IFC_CH5DONE_MASK                           0x20UL                            /**< Bit mask for DMA_CH5DONE */
#define _DMA_IFC_CH5DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH5DONE_DEFAULT                         (_DMA_IFC_CH5DONE_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH6DONE                                 (0x1UL << 6)                      /**< DMA Channel 6 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH6DONE_SHIFT                          6                                 /**< Shift value for DMA_CH6DONE */
#define _DMA_IFC_CH6DONE_MASK                           0x40UL                            /**< Bit mask for DMA_CH6DONE */
#define _DMA_IFC_CH6DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH6DONE_DEFAULT                         (_DMA_IFC_CH6DONE_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH7DONE                                 (0x1UL << 7)                      /**< DMA Channel 7 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH7DONE_SHIFT                          7                                 /**< Shift value for DMA_CH7DONE */
#define _DMA_IFC_CH7DONE_MASK                           0x80UL                            /**< Bit mask for DMA_CH7DONE */
#define _DMA_IFC_CH7DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH7DONE_DEFAULT                         (_DMA_IFC_CH7DONE_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH8DONE                                 (0x1UL << 8)                      /**< DMA Channel 8 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH8DONE_SHIFT                          8                                 /**< Shift value for DMA_CH8DONE */
#define _DMA_IFC_CH8DONE_MASK                           0x100UL                           /**< Bit mask for DMA_CH8DONE */
#define _DMA_IFC_CH8DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH8DONE_DEFAULT                         (_DMA_IFC_CH8DONE_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH9DONE                                 (0x1UL << 9)                      /**< DMA Channel 9 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH9DONE_SHIFT                          9                                 /**< Shift value for DMA_CH9DONE */
#define _DMA_IFC_CH9DONE_MASK                           0x200UL                           /**< Bit mask for DMA_CH9DONE */
#define _DMA_IFC_CH9DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH9DONE_DEFAULT                         (_DMA_IFC_CH9DONE_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH10DONE                                (0x1UL << 10)                     /**< DMA Channel 10 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH10DONE_SHIFT                         10                                /**< Shift value for DMA_CH10DONE */
#define _DMA_IFC_CH10DONE_MASK                          0x400UL                           /**< Bit mask for DMA_CH10DONE */
#define _DMA_IFC_CH10DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH10DONE_DEFAULT                        (_DMA_IFC_CH10DONE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH11DONE                                (0x1UL << 11)                     /**< DMA Channel 11 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH11DONE_SHIFT                         11                                /**< Shift value for DMA_CH11DONE */
#define _DMA_IFC_CH11DONE_MASK                          0x800UL                           /**< Bit mask for DMA_CH11DONE */
#define _DMA_IFC_CH11DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH11DONE_DEFAULT                        (_DMA_IFC_CH11DONE_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_ERR                                     (0x1UL << 31)                     /**< DMA Error Interrupt Flag Clear */
#define _DMA_IFC_ERR_SHIFT                              31                                /**< Shift value for DMA_ERR */
#define _DMA_IFC_ERR_MASK                               0x80000000UL                      /**< Bit mask for DMA_ERR */
#define _DMA_IFC_ERR_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_ERR_DEFAULT                             (_DMA_IFC_ERR_DEFAULT << 31)      /**< Shifted mode DEFAULT for DMA_IFC */

/* Bit fields for DMA IEN */
#define _DMA_IEN_RESETVALUE                             0x00000000UL                      /**< Default value for DMA_IEN */
#define _DMA_IEN_MASK                                   0x80000FFFUL                      /**< Mask for DMA_IEN */
#define DMA_IEN_CH0DONE                                 (0x1UL << 0)                      /**< DMA Channel 0 Complete Interrupt Enable */
#define _DMA_IEN_CH0DONE_SHIFT                          0                                 /**< Shift value for DMA_CH0DONE */
#define _DMA_IEN_CH0DONE_MASK                           0x1UL                             /**< Bit mask for DMA_CH0DONE */
#define _DMA_IEN_CH0DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH0DONE_DEFAULT                         (_DMA_IEN_CH0DONE_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH1DONE                                 (0x1UL << 1)                      /**< DMA Channel 1 Complete Interrupt Enable */
#define _DMA_IEN_CH1DONE_SHIFT                          1                                 /**< Shift value for DMA_CH1DONE */
#define _DMA_IEN_CH1DONE_MASK                           0x2UL                             /**< Bit mask for DMA_CH1DONE */
#define _DMA_IEN_CH1DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH1DONE_DEFAULT                         (_DMA_IEN_CH1DONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH2DONE                                 (0x1UL << 2)                      /**< DMA Channel 2 Complete Interrupt Enable */
#define _DMA_IEN_CH2DONE_SHIFT                          2                                 /**< Shift value for DMA_CH2DONE */
#define _DMA_IEN_CH2DONE_MASK                           0x4UL                             /**< Bit mask for DMA_CH2DONE */
#define _DMA_IEN_CH2DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH2DONE_DEFAULT                         (_DMA_IEN_CH2DONE_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH3DONE                                 (0x1UL << 3)                      /**< DMA Channel 3 Complete Interrupt Enable */
#define _DMA_IEN_CH3DONE_SHIFT                          3                                 /**< Shift value for DMA_CH3DONE */
#define _DMA_IEN_CH3DONE_MASK                           0x8UL                             /**< Bit mask for DMA_CH3DONE */
#define _DMA_IEN_CH3DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH3DONE_DEFAULT                         (_DMA_IEN_CH3DONE_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH4DONE                                 (0x1UL << 4)                      /**< DMA Channel 4 Complete Interrupt Enable */
#define _DMA_IEN_CH4DONE_SHIFT                          4                                 /**< Shift value for DMA_CH4DONE */
#define _DMA_IEN_CH4DONE_MASK                           0x10UL                            /**< Bit mask for DMA_CH4DONE */
#define _DMA_IEN_CH4DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH4DONE_DEFAULT                         (_DMA_IEN_CH4DONE_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH5DONE                                 (0x1UL << 5)                      /**< DMA Channel 5 Complete Interrupt Enable */
#define _DMA_IEN_CH5DONE_SHIFT                          5                                 /**< Shift value for DMA_CH5DONE */
#define _DMA_IEN_CH5DONE_MASK                           0x20UL                            /**< Bit mask for DMA_CH5DONE */
#define _DMA_IEN_CH5DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH5DONE_DEFAULT                         (_DMA_IEN_CH5DONE_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH6DONE                                 (0x1UL << 6)                      /**< DMA Channel 6 Complete Interrupt Enable */
#define _DMA_IEN_CH6DONE_SHIFT                          6                                 /**< Shift value for DMA_CH6DONE */
#define _DMA_IEN_CH6DONE_MASK                           0x40UL                            /**< Bit mask for DMA_CH6DONE */
#define _DMA_IEN_CH6DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH6DONE_DEFAULT                         (_DMA_IEN_CH6DONE_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH7DONE                                 (0x1UL << 7)                      /**< DMA Channel 7 Complete Interrupt Enable */
#define _DMA_IEN_CH7DONE_SHIFT                          7                                 /**< Shift value for DMA_CH7DONE */
#define _DMA_IEN_CH7DONE_MASK                           0x80UL                            /**< Bit mask for DMA_CH7DONE */
#define _DMA_IEN_CH7DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH7DONE_DEFAULT                         (_DMA_IEN_CH7DONE_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH8DONE                                 (0x1UL << 8)                      /**< DMA Channel 8 Complete Interrupt Enable */
#define _DMA_IEN_CH8DONE_SHIFT                          8                                 /**< Shift value for DMA_CH8DONE */
#define _DMA_IEN_CH8DONE_MASK                           0x100UL                           /**< Bit mask for DMA_CH8DONE */
#define _DMA_IEN_CH8DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH8DONE_DEFAULT                         (_DMA_IEN_CH8DONE_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH9DONE                                 (0x1UL << 9)                      /**< DMA Channel 9 Complete Interrupt Enable */
#define _DMA_IEN_CH9DONE_SHIFT                          9                                 /**< Shift value for DMA_CH9DONE */
#define _DMA_IEN_CH9DONE_MASK                           0x200UL                           /**< Bit mask for DMA_CH9DONE */
#define _DMA_IEN_CH9DONE_DEFAULT                        0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH9DONE_DEFAULT                         (_DMA_IEN_CH9DONE_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH10DONE                                (0x1UL << 10)                     /**< DMA Channel 10 Complete Interrupt Enable */
#define _DMA_IEN_CH10DONE_SHIFT                         10                                /**< Shift value for DMA_CH10DONE */
#define _DMA_IEN_CH10DONE_MASK                          0x400UL                           /**< Bit mask for DMA_CH10DONE */
#define _DMA_IEN_CH10DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH10DONE_DEFAULT                        (_DMA_IEN_CH10DONE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH11DONE                                (0x1UL << 11)                     /**< DMA Channel 11 Complete Interrupt Enable */
#define _DMA_IEN_CH11DONE_SHIFT                         11                                /**< Shift value for DMA_CH11DONE */
#define _DMA_IEN_CH11DONE_MASK                          0x800UL                           /**< Bit mask for DMA_CH11DONE */
#define _DMA_IEN_CH11DONE_DEFAULT                       0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH11DONE_DEFAULT                        (_DMA_IEN_CH11DONE_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_ERR                                     (0x1UL << 31)                     /**< DMA Error Interrupt Flag Enable */
#define _DMA_IEN_ERR_SHIFT                              31                                /**< Shift value for DMA_ERR */
#define _DMA_IEN_ERR_MASK                               0x80000000UL                      /**< Bit mask for DMA_ERR */
#define _DMA_IEN_ERR_DEFAULT                            0x00000000UL                      /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_ERR_DEFAULT                             (_DMA_IEN_ERR_DEFAULT << 31)      /**< Shifted mode DEFAULT for DMA_IEN */

/* Bit fields for DMA CTRL */
#define _DMA_CTRL_RESETVALUE                            0x00000000UL                      /**< Default value for DMA_CTRL */
#define _DMA_CTRL_MASK                                  0x00000003UL                      /**< Mask for DMA_CTRL */
#define DMA_CTRL_DESCRECT                               (0x1UL << 0)                      /**< Descriptor Specifies Rectangle */
#define _DMA_CTRL_DESCRECT_SHIFT                        0                                 /**< Shift value for DMA_DESCRECT */
#define _DMA_CTRL_DESCRECT_MASK                         0x1UL                             /**< Bit mask for DMA_DESCRECT */
#define _DMA_CTRL_DESCRECT_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for DMA_CTRL */
#define DMA_CTRL_DESCRECT_DEFAULT                       (_DMA_CTRL_DESCRECT_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CTRL */
#define DMA_CTRL_PRDU                                   (0x1UL << 1)                      /**< Prevent Rect Descriptor Update */
#define _DMA_CTRL_PRDU_SHIFT                            1                                 /**< Shift value for DMA_PRDU */
#define _DMA_CTRL_PRDU_MASK                             0x2UL                             /**< Bit mask for DMA_PRDU */
#define _DMA_CTRL_PRDU_DEFAULT                          0x00000000UL                      /**< Mode DEFAULT for DMA_CTRL */
#define DMA_CTRL_PRDU_DEFAULT                           (_DMA_CTRL_PRDU_DEFAULT << 1)     /**< Shifted mode DEFAULT for DMA_CTRL */

/* Bit fields for DMA RDS */
#define _DMA_RDS_RESETVALUE                             0x00000000UL                     /**< Default value for DMA_RDS */
#define _DMA_RDS_MASK                                   0x00000FFFUL                     /**< Mask for DMA_RDS */
#define DMA_RDS_RDSCH0                                  (0x1UL << 0)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH0_SHIFT                           0                                /**< Shift value for DMA_RDSCH0 */
#define _DMA_RDS_RDSCH0_MASK                            0x1UL                            /**< Bit mask for DMA_RDSCH0 */
#define _DMA_RDS_RDSCH0_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH0_DEFAULT                          (_DMA_RDS_RDSCH0_DEFAULT << 0)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH1                                  (0x1UL << 1)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH1_SHIFT                           1                                /**< Shift value for DMA_RDSCH1 */
#define _DMA_RDS_RDSCH1_MASK                            0x2UL                            /**< Bit mask for DMA_RDSCH1 */
#define _DMA_RDS_RDSCH1_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH1_DEFAULT                          (_DMA_RDS_RDSCH1_DEFAULT << 1)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH2                                  (0x1UL << 2)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH2_SHIFT                           2                                /**< Shift value for DMA_RDSCH2 */
#define _DMA_RDS_RDSCH2_MASK                            0x4UL                            /**< Bit mask for DMA_RDSCH2 */
#define _DMA_RDS_RDSCH2_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH2_DEFAULT                          (_DMA_RDS_RDSCH2_DEFAULT << 2)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH3                                  (0x1UL << 3)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH3_SHIFT                           3                                /**< Shift value for DMA_RDSCH3 */
#define _DMA_RDS_RDSCH3_MASK                            0x8UL                            /**< Bit mask for DMA_RDSCH3 */
#define _DMA_RDS_RDSCH3_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH3_DEFAULT                          (_DMA_RDS_RDSCH3_DEFAULT << 3)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH4                                  (0x1UL << 4)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH4_SHIFT                           4                                /**< Shift value for DMA_RDSCH4 */
#define _DMA_RDS_RDSCH4_MASK                            0x10UL                           /**< Bit mask for DMA_RDSCH4 */
#define _DMA_RDS_RDSCH4_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH4_DEFAULT                          (_DMA_RDS_RDSCH4_DEFAULT << 4)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH5                                  (0x1UL << 5)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH5_SHIFT                           5                                /**< Shift value for DMA_RDSCH5 */
#define _DMA_RDS_RDSCH5_MASK                            0x20UL                           /**< Bit mask for DMA_RDSCH5 */
#define _DMA_RDS_RDSCH5_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH5_DEFAULT                          (_DMA_RDS_RDSCH5_DEFAULT << 5)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH6                                  (0x1UL << 6)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH6_SHIFT                           6                                /**< Shift value for DMA_RDSCH6 */
#define _DMA_RDS_RDSCH6_MASK                            0x40UL                           /**< Bit mask for DMA_RDSCH6 */
#define _DMA_RDS_RDSCH6_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH6_DEFAULT                          (_DMA_RDS_RDSCH6_DEFAULT << 6)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH7                                  (0x1UL << 7)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH7_SHIFT                           7                                /**< Shift value for DMA_RDSCH7 */
#define _DMA_RDS_RDSCH7_MASK                            0x80UL                           /**< Bit mask for DMA_RDSCH7 */
#define _DMA_RDS_RDSCH7_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH7_DEFAULT                          (_DMA_RDS_RDSCH7_DEFAULT << 7)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH8                                  (0x1UL << 8)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH8_SHIFT                           8                                /**< Shift value for DMA_RDSCH8 */
#define _DMA_RDS_RDSCH8_MASK                            0x100UL                          /**< Bit mask for DMA_RDSCH8 */
#define _DMA_RDS_RDSCH8_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH8_DEFAULT                          (_DMA_RDS_RDSCH8_DEFAULT << 8)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH9                                  (0x1UL << 9)                     /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH9_SHIFT                           9                                /**< Shift value for DMA_RDSCH9 */
#define _DMA_RDS_RDSCH9_MASK                            0x200UL                          /**< Bit mask for DMA_RDSCH9 */
#define _DMA_RDS_RDSCH9_DEFAULT                         0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH9_DEFAULT                          (_DMA_RDS_RDSCH9_DEFAULT << 9)   /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH10                                 (0x1UL << 10)                    /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH10_SHIFT                          10                               /**< Shift value for DMA_RDSCH10 */
#define _DMA_RDS_RDSCH10_MASK                           0x400UL                          /**< Bit mask for DMA_RDSCH10 */
#define _DMA_RDS_RDSCH10_DEFAULT                        0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH10_DEFAULT                         (_DMA_RDS_RDSCH10_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH11                                 (0x1UL << 11)                    /**< Retain Descriptor State */
#define _DMA_RDS_RDSCH11_SHIFT                          11                               /**< Shift value for DMA_RDSCH11 */
#define _DMA_RDS_RDSCH11_MASK                           0x800UL                          /**< Bit mask for DMA_RDSCH11 */
#define _DMA_RDS_RDSCH11_DEFAULT                        0x00000000UL                     /**< Mode DEFAULT for DMA_RDS */
#define DMA_RDS_RDSCH11_DEFAULT                         (_DMA_RDS_RDSCH11_DEFAULT << 11) /**< Shifted mode DEFAULT for DMA_RDS */

/* Bit fields for DMA LOOP0 */
#define _DMA_LOOP0_RESETVALUE                           0x00000000UL                    /**< Default value for DMA_LOOP0 */
#define _DMA_LOOP0_MASK                                 0x000103FFUL                    /**< Mask for DMA_LOOP0 */
#define _DMA_LOOP0_WIDTH_SHIFT                          0                               /**< Shift value for DMA_WIDTH */
#define _DMA_LOOP0_WIDTH_MASK                           0x3FFUL                         /**< Bit mask for DMA_WIDTH */
#define _DMA_LOOP0_WIDTH_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_LOOP0 */
#define DMA_LOOP0_WIDTH_DEFAULT                         (_DMA_LOOP0_WIDTH_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_LOOP0 */
#define DMA_LOOP0_EN                                    (0x1UL << 16)                   /**< DMA Channel 0 Loop Enable */
#define _DMA_LOOP0_EN_SHIFT                             16                              /**< Shift value for DMA_EN */
#define _DMA_LOOP0_EN_MASK                              0x10000UL                       /**< Bit mask for DMA_EN */
#define _DMA_LOOP0_EN_DEFAULT                           0x00000000UL                    /**< Mode DEFAULT for DMA_LOOP0 */
#define DMA_LOOP0_EN_DEFAULT                            (_DMA_LOOP0_EN_DEFAULT << 16)   /**< Shifted mode DEFAULT for DMA_LOOP0 */

/* Bit fields for DMA LOOP1 */
#define _DMA_LOOP1_RESETVALUE                           0x00000000UL                    /**< Default value for DMA_LOOP1 */
#define _DMA_LOOP1_MASK                                 0x000103FFUL                    /**< Mask for DMA_LOOP1 */
#define _DMA_LOOP1_WIDTH_SHIFT                          0                               /**< Shift value for DMA_WIDTH */
#define _DMA_LOOP1_WIDTH_MASK                           0x3FFUL                         /**< Bit mask for DMA_WIDTH */
#define _DMA_LOOP1_WIDTH_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_LOOP1 */
#define DMA_LOOP1_WIDTH_DEFAULT                         (_DMA_LOOP1_WIDTH_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_LOOP1 */
#define DMA_LOOP1_EN                                    (0x1UL << 16)                   /**< DMA Channel 1 Loop Enable */
#define _DMA_LOOP1_EN_SHIFT                             16                              /**< Shift value for DMA_EN */
#define _DMA_LOOP1_EN_MASK                              0x10000UL                       /**< Bit mask for DMA_EN */
#define _DMA_LOOP1_EN_DEFAULT                           0x00000000UL                    /**< Mode DEFAULT for DMA_LOOP1 */
#define DMA_LOOP1_EN_DEFAULT                            (_DMA_LOOP1_EN_DEFAULT << 16)   /**< Shifted mode DEFAULT for DMA_LOOP1 */

/* Bit fields for DMA RECT0 */
#define _DMA_RECT0_RESETVALUE                           0x00000000UL                         /**< Default value for DMA_RECT0 */
#define _DMA_RECT0_MASK                                 0xFFFFFFFFUL                         /**< Mask for DMA_RECT0 */
#define _DMA_RECT0_HEIGHT_SHIFT                         0                                    /**< Shift value for DMA_HEIGHT */
#define _DMA_RECT0_HEIGHT_MASK                          0x3FFUL                              /**< Bit mask for DMA_HEIGHT */
#define _DMA_RECT0_HEIGHT_DEFAULT                       0x00000000UL                         /**< Mode DEFAULT for DMA_RECT0 */
#define DMA_RECT0_HEIGHT_DEFAULT                        (_DMA_RECT0_HEIGHT_DEFAULT << 0)     /**< Shifted mode DEFAULT for DMA_RECT0 */
#define _DMA_RECT0_SRCSTRIDE_SHIFT                      10                                   /**< Shift value for DMA_SRCSTRIDE */
#define _DMA_RECT0_SRCSTRIDE_MASK                       0x1FFC00UL                           /**< Bit mask for DMA_SRCSTRIDE */
#define _DMA_RECT0_SRCSTRIDE_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_RECT0 */
#define DMA_RECT0_SRCSTRIDE_DEFAULT                     (_DMA_RECT0_SRCSTRIDE_DEFAULT << 10) /**< Shifted mode DEFAULT for DMA_RECT0 */
#define _DMA_RECT0_DSTSTRIDE_SHIFT                      21                                   /**< Shift value for DMA_DSTSTRIDE */
#define _DMA_RECT0_DSTSTRIDE_MASK                       0xFFE00000UL                         /**< Bit mask for DMA_DSTSTRIDE */
#define _DMA_RECT0_DSTSTRIDE_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for DMA_RECT0 */
#define DMA_RECT0_DSTSTRIDE_DEFAULT                     (_DMA_RECT0_DSTSTRIDE_DEFAULT << 21) /**< Shifted mode DEFAULT for DMA_RECT0 */

/* Bit fields for DMA CH_CTRL */
#define _DMA_CH_CTRL_RESETVALUE                         0x00000000UL                                  /**< Default value for DMA_CH_CTRL */
#define _DMA_CH_CTRL_MASK                               0x003F000FUL                                  /**< Mask for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_SHIFT                       0                                             /**< Shift value for DMA_SIGSEL */
#define _DMA_CH_CTRL_SIGSEL_MASK                        0xFUL                                         /**< Bit mask for DMA_SIGSEL */
#define _DMA_CH_CTRL_SIGSEL_ADC0SINGLE                  0x00000000UL                                  /**< Mode ADC0SINGLE for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_DAC0CH0                     0x00000000UL                                  /**< Mode DAC0CH0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0RXDATAV               0x00000000UL                                  /**< Mode USART0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1RXDATAV               0x00000000UL                                  /**< Mode USART1RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2RXDATAV               0x00000000UL                                  /**< Mode USART2RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV              0x00000000UL                                  /**< Mode LEUART0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART1RXDATAV              0x00000000UL                                  /**< Mode LEUART1RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C0RXDATAV                 0x00000000UL                                  /**< Mode I2C0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C1RXDATAV                 0x00000000UL                                  /**< Mode I2C1RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0UFOF                  0x00000000UL                                  /**< Mode TIMER0UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1UFOF                  0x00000000UL                                  /**< Mode TIMER1UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2UFOF                  0x00000000UL                                  /**< Mode TIMER2UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER3UFOF                  0x00000000UL                                  /**< Mode TIMER3UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_UART0RXDATAV                0x00000000UL                                  /**< Mode UART0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_UART1RXDATAV                0x00000000UL                                  /**< Mode UART1RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_MSCWDATA                    0x00000000UL                                  /**< Mode MSCWDATA for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESDATAWR                   0x00000000UL                                  /**< Mode AESDATAWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LESENSEBUFDATAV             0x00000000UL                                  /**< Mode LESENSEBUFDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_EBIPXL0EMPTY                0x00000000UL                                  /**< Mode EBIPXL0EMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_ADC0SCAN                    0x00000001UL                                  /**< Mode ADC0SCAN for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_DAC0CH1                     0x00000001UL                                  /**< Mode DAC0CH1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0TXBL                  0x00000001UL                                  /**< Mode USART0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXBL                  0x00000001UL                                  /**< Mode USART1TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2TXBL                  0x00000001UL                                  /**< Mode USART2TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0TXBL                 0x00000001UL                                  /**< Mode LEUART0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART1TXBL                 0x00000001UL                                  /**< Mode LEUART1TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C0TXBL                    0x00000001UL                                  /**< Mode I2C0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C1TXBL                    0x00000001UL                                  /**< Mode I2C1TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC0                   0x00000001UL                                  /**< Mode TIMER0CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC0                   0x00000001UL                                  /**< Mode TIMER1CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC0                   0x00000001UL                                  /**< Mode TIMER2CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER3CC0                   0x00000001UL                                  /**< Mode TIMER3CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_UART0TXBL                   0x00000001UL                                  /**< Mode UART0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_UART1TXBL                   0x00000001UL                                  /**< Mode UART1TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESXORDATAWR                0x00000001UL                                  /**< Mode AESXORDATAWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_EBIPXL1EMPTY                0x00000001UL                                  /**< Mode EBIPXL1EMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0TXEMPTY               0x00000002UL                                  /**< Mode USART0TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXEMPTY               0x00000002UL                                  /**< Mode USART1TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2TXEMPTY               0x00000002UL                                  /**< Mode USART2TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY              0x00000002UL                                  /**< Mode LEUART0TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART1TXEMPTY              0x00000002UL                                  /**< Mode LEUART1TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC1                   0x00000002UL                                  /**< Mode TIMER0CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC1                   0x00000002UL                                  /**< Mode TIMER1CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC1                   0x00000002UL                                  /**< Mode TIMER2CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER3CC1                   0x00000002UL                                  /**< Mode TIMER3CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_UART0TXEMPTY                0x00000002UL                                  /**< Mode UART0TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_UART1TXEMPTY                0x00000002UL                                  /**< Mode UART1TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESDATARD                   0x00000002UL                                  /**< Mode AESDATARD for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_EBIPXLFULL                  0x00000002UL                                  /**< Mode EBIPXLFULL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT          0x00000003UL                                  /**< Mode USART1RXDATAVRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2RXDATAVRIGHT          0x00000003UL                                  /**< Mode USART2RXDATAVRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC2                   0x00000003UL                                  /**< Mode TIMER0CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC2                   0x00000003UL                                  /**< Mode TIMER1CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC2                   0x00000003UL                                  /**< Mode TIMER2CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER3CC2                   0x00000003UL                                  /**< Mode TIMER3CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESKEYWR                    0x00000003UL                                  /**< Mode AESKEYWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_EBIDDEMPTY                  0x00000003UL                                  /**< Mode EBIDDEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT             0x00000004UL                                  /**< Mode USART1TXBLRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART2TXBLRIGHT             0x00000004UL                                  /**< Mode USART2TXBLRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_ADC0SINGLE                   (_DMA_CH_CTRL_SIGSEL_ADC0SINGLE << 0)         /**< Shifted mode ADC0SINGLE for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_DAC0CH0                      (_DMA_CH_CTRL_SIGSEL_DAC0CH0 << 0)            /**< Shifted mode DAC0CH0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0RXDATAV                (_DMA_CH_CTRL_SIGSEL_USART0RXDATAV << 0)      /**< Shifted mode USART0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1RXDATAV                (_DMA_CH_CTRL_SIGSEL_USART1RXDATAV << 0)      /**< Shifted mode USART1RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2RXDATAV                (_DMA_CH_CTRL_SIGSEL_USART2RXDATAV << 0)      /**< Shifted mode USART2RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV               (_DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV << 0)     /**< Shifted mode LEUART0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART1RXDATAV               (_DMA_CH_CTRL_SIGSEL_LEUART1RXDATAV << 0)     /**< Shifted mode LEUART1RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C0RXDATAV                  (_DMA_CH_CTRL_SIGSEL_I2C0RXDATAV << 0)        /**< Shifted mode I2C0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C1RXDATAV                  (_DMA_CH_CTRL_SIGSEL_I2C1RXDATAV << 0)        /**< Shifted mode I2C1RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER0UFOF << 0)         /**< Shifted mode TIMER0UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER1UFOF << 0)         /**< Shifted mode TIMER1UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER2UFOF << 0)         /**< Shifted mode TIMER2UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER3UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER3UFOF << 0)         /**< Shifted mode TIMER3UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_UART0RXDATAV                 (_DMA_CH_CTRL_SIGSEL_UART0RXDATAV << 0)       /**< Shifted mode UART0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_UART1RXDATAV                 (_DMA_CH_CTRL_SIGSEL_UART1RXDATAV << 0)       /**< Shifted mode UART1RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_MSCWDATA                     (_DMA_CH_CTRL_SIGSEL_MSCWDATA << 0)           /**< Shifted mode MSCWDATA for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESDATAWR                    (_DMA_CH_CTRL_SIGSEL_AESDATAWR << 0)          /**< Shifted mode AESDATAWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LESENSEBUFDATAV              (_DMA_CH_CTRL_SIGSEL_LESENSEBUFDATAV << 0)    /**< Shifted mode LESENSEBUFDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_EBIPXL0EMPTY                 (_DMA_CH_CTRL_SIGSEL_EBIPXL0EMPTY << 0)       /**< Shifted mode EBIPXL0EMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_ADC0SCAN                     (_DMA_CH_CTRL_SIGSEL_ADC0SCAN << 0)           /**< Shifted mode ADC0SCAN for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_DAC0CH1                      (_DMA_CH_CTRL_SIGSEL_DAC0CH1 << 0)            /**< Shifted mode DAC0CH1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0TXBL                   (_DMA_CH_CTRL_SIGSEL_USART0TXBL << 0)         /**< Shifted mode USART0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXBL                   (_DMA_CH_CTRL_SIGSEL_USART1TXBL << 0)         /**< Shifted mode USART1TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2TXBL                   (_DMA_CH_CTRL_SIGSEL_USART2TXBL << 0)         /**< Shifted mode USART2TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0TXBL                  (_DMA_CH_CTRL_SIGSEL_LEUART0TXBL << 0)        /**< Shifted mode LEUART0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART1TXBL                  (_DMA_CH_CTRL_SIGSEL_LEUART1TXBL << 0)        /**< Shifted mode LEUART1TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C0TXBL                     (_DMA_CH_CTRL_SIGSEL_I2C0TXBL << 0)           /**< Shifted mode I2C0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C1TXBL                     (_DMA_CH_CTRL_SIGSEL_I2C1TXBL << 0)           /**< Shifted mode I2C1TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC0 << 0)          /**< Shifted mode TIMER0CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC0 << 0)          /**< Shifted mode TIMER1CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC0 << 0)          /**< Shifted mode TIMER2CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER3CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER3CC0 << 0)          /**< Shifted mode TIMER3CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_UART0TXBL                    (_DMA_CH_CTRL_SIGSEL_UART0TXBL << 0)          /**< Shifted mode UART0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_UART1TXBL                    (_DMA_CH_CTRL_SIGSEL_UART1TXBL << 0)          /**< Shifted mode UART1TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESXORDATAWR                 (_DMA_CH_CTRL_SIGSEL_AESXORDATAWR << 0)       /**< Shifted mode AESXORDATAWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_EBIPXL1EMPTY                 (_DMA_CH_CTRL_SIGSEL_EBIPXL1EMPTY << 0)       /**< Shifted mode EBIPXL1EMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0TXEMPTY                (_DMA_CH_CTRL_SIGSEL_USART0TXEMPTY << 0)      /**< Shifted mode USART0TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXEMPTY                (_DMA_CH_CTRL_SIGSEL_USART1TXEMPTY << 0)      /**< Shifted mode USART1TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2TXEMPTY                (_DMA_CH_CTRL_SIGSEL_USART2TXEMPTY << 0)      /**< Shifted mode USART2TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY               (_DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY << 0)     /**< Shifted mode LEUART0TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART1TXEMPTY               (_DMA_CH_CTRL_SIGSEL_LEUART1TXEMPTY << 0)     /**< Shifted mode LEUART1TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC1 << 0)          /**< Shifted mode TIMER0CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC1 << 0)          /**< Shifted mode TIMER1CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC1 << 0)          /**< Shifted mode TIMER2CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER3CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER3CC1 << 0)          /**< Shifted mode TIMER3CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_UART0TXEMPTY                 (_DMA_CH_CTRL_SIGSEL_UART0TXEMPTY << 0)       /**< Shifted mode UART0TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_UART1TXEMPTY                 (_DMA_CH_CTRL_SIGSEL_UART1TXEMPTY << 0)       /**< Shifted mode UART1TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESDATARD                    (_DMA_CH_CTRL_SIGSEL_AESDATARD << 0)          /**< Shifted mode AESDATARD for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_EBIPXLFULL                   (_DMA_CH_CTRL_SIGSEL_EBIPXLFULL << 0)         /**< Shifted mode EBIPXLFULL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT           (_DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT << 0) /**< Shifted mode USART1RXDATAVRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2RXDATAVRIGHT           (_DMA_CH_CTRL_SIGSEL_USART2RXDATAVRIGHT << 0) /**< Shifted mode USART2RXDATAVRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC2 << 0)          /**< Shifted mode TIMER0CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC2 << 0)          /**< Shifted mode TIMER1CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC2 << 0)          /**< Shifted mode TIMER2CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER3CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER3CC2 << 0)          /**< Shifted mode TIMER3CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESKEYWR                     (_DMA_CH_CTRL_SIGSEL_AESKEYWR << 0)           /**< Shifted mode AESKEYWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_EBIDDEMPTY                   (_DMA_CH_CTRL_SIGSEL_EBIDDEMPTY << 0)         /**< Shifted mode EBIDDEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT              (_DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT << 0)    /**< Shifted mode USART1TXBLRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART2TXBLRIGHT              (_DMA_CH_CTRL_SIGSEL_USART2TXBLRIGHT << 0)    /**< Shifted mode USART2TXBLRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_SHIFT                    16                                            /**< Shift value for DMA_SOURCESEL */
#define _DMA_CH_CTRL_SOURCESEL_MASK                     0x3F0000UL                                    /**< Bit mask for DMA_SOURCESEL */
#define _DMA_CH_CTRL_SOURCESEL_NONE                     0x00000000UL                                  /**< Mode NONE for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_ADC0                     0x00000008UL                                  /**< Mode ADC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_DAC0                     0x0000000AUL                                  /**< Mode DAC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_USART0                   0x0000000CUL                                  /**< Mode USART0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_USART1                   0x0000000DUL                                  /**< Mode USART1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_USART2                   0x0000000EUL                                  /**< Mode USART2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_LEUART0                  0x00000010UL                                  /**< Mode LEUART0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_LEUART1                  0x00000011UL                                  /**< Mode LEUART1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_I2C0                     0x00000014UL                                  /**< Mode I2C0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_I2C1                     0x00000015UL                                  /**< Mode I2C1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER0                   0x00000018UL                                  /**< Mode TIMER0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER1                   0x00000019UL                                  /**< Mode TIMER1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER2                   0x0000001AUL                                  /**< Mode TIMER2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER3                   0x0000001BUL                                  /**< Mode TIMER3 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_UART0                    0x0000002CUL                                  /**< Mode UART0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_UART1                    0x0000002DUL                                  /**< Mode UART1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_MSC                      0x00000030UL                                  /**< Mode MSC for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_AES                      0x00000031UL                                  /**< Mode AES for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_LESENSE                  0x00000032UL                                  /**< Mode LESENSE for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_EBI                      0x00000033UL                                  /**< Mode EBI for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_NONE                      (_DMA_CH_CTRL_SOURCESEL_NONE << 16)           /**< Shifted mode NONE for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_ADC0                      (_DMA_CH_CTRL_SOURCESEL_ADC0 << 16)           /**< Shifted mode ADC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_DAC0                      (_DMA_CH_CTRL_SOURCESEL_DAC0 << 16)           /**< Shifted mode DAC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_USART0                    (_DMA_CH_CTRL_SOURCESEL_USART0 << 16)         /**< Shifted mode USART0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_USART1                    (_DMA_CH_CTRL_SOURCESEL_USART1 << 16)         /**< Shifted mode USART1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_USART2                    (_DMA_CH_CTRL_SOURCESEL_USART2 << 16)         /**< Shifted mode USART2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_LEUART0                   (_DMA_CH_CTRL_SOURCESEL_LEUART0 << 16)        /**< Shifted mode LEUART0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_LEUART1                   (_DMA_CH_CTRL_SOURCESEL_LEUART1 << 16)        /**< Shifted mode LEUART1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_I2C0                      (_DMA_CH_CTRL_SOURCESEL_I2C0 << 16)           /**< Shifted mode I2C0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_I2C1                      (_DMA_CH_CTRL_SOURCESEL_I2C1 << 16)           /**< Shifted mode I2C1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER0                    (_DMA_CH_CTRL_SOURCESEL_TIMER0 << 16)         /**< Shifted mode TIMER0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER1                    (_DMA_CH_CTRL_SOURCESEL_TIMER1 << 16)         /**< Shifted mode TIMER1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER2                    (_DMA_CH_CTRL_SOURCESEL_TIMER2 << 16)         /**< Shifted mode TIMER2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER3                    (_DMA_CH_CTRL_SOURCESEL_TIMER3 << 16)         /**< Shifted mode TIMER3 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_UART0                     (_DMA_CH_CTRL_SOURCESEL_UART0 << 16)          /**< Shifted mode UART0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_UART1                     (_DMA_CH_CTRL_SOURCESEL_UART1 << 16)          /**< Shifted mode UART1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_MSC                       (_DMA_CH_CTRL_SOURCESEL_MSC << 16)            /**< Shifted mode MSC for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_AES                       (_DMA_CH_CTRL_SOURCESEL_AES << 16)            /**< Shifted mode AES for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_LESENSE                   (_DMA_CH_CTRL_SOURCESEL_LESENSE << 16)        /**< Shifted mode LESENSE for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_EBI                       (_DMA_CH_CTRL_SOURCESEL_EBI << 16)            /**< Shifted mode EBI for DMA_CH_CTRL */

/** @} End of group EFM32WG_DMA */
/** @} End of group Parts */

