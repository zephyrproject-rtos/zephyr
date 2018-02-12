/**************************************************************************//**
 * @file efm32hg_dma.h
 * @brief EFM32HG_DMA register and bit field definitions
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
 * @defgroup EFM32HG_DMA
 * @{
 * @brief EFM32HG_DMA Register Declaration
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

  uint32_t       RESERVED4[60];  /**< Reserved registers */
  DMA_CH_TypeDef CH[6];          /**< Channel registers */
} DMA_TypeDef;                   /** @} */

/**************************************************************************//**
 * @defgroup EFM32HG_DMA_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for DMA STATUS */
#define _DMA_STATUS_RESETVALUE                          0x10050000UL                          /**< Default value for DMA_STATUS */
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
#define _DMA_STATUS_CHNUM_DEFAULT                       0x00000005UL                          /**< Mode DEFAULT for DMA_STATUS */
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
#define _DMA_ALTCTRLBASE_RESETVALUE                     0x00000080UL                                /**< Default value for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_MASK                           0xFFFFFFFFUL                                /**< Mask for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_SHIFT              0                                           /**< Shift value for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_MASK               0xFFFFFFFFUL                                /**< Bit mask for DMA_ALTCTRLBASE */
#define _DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT            0x00000080UL                                /**< Mode DEFAULT for DMA_ALTCTRLBASE */
#define DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT             (_DMA_ALTCTRLBASE_ALTCTRLBASE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_ALTCTRLBASE */

/* Bit fields for DMA CHWAITSTATUS */
#define _DMA_CHWAITSTATUS_RESETVALUE                    0x0000003FUL                                   /**< Default value for DMA_CHWAITSTATUS */
#define _DMA_CHWAITSTATUS_MASK                          0x0000003FUL                                   /**< Mask for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH0WAITSTATUS                  (0x1UL << 0)                                   /**< Channel 0 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_SHIFT           0                                              /**< Shift value for DMA_CH0WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_MASK            0x1UL                                          /**< Bit mask for DMA_CH0WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT         0x00000001UL                                   /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH0WAITSTATUS_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH1WAITSTATUS                  (0x1UL << 1)                                   /**< Channel 1 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_SHIFT           1                                              /**< Shift value for DMA_CH1WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_MASK            0x2UL                                          /**< Bit mask for DMA_CH1WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT         0x00000001UL                                   /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH1WAITSTATUS_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH2WAITSTATUS                  (0x1UL << 2)                                   /**< Channel 2 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_SHIFT           2                                              /**< Shift value for DMA_CH2WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_MASK            0x4UL                                          /**< Bit mask for DMA_CH2WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT         0x00000001UL                                   /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH2WAITSTATUS_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH3WAITSTATUS                  (0x1UL << 3)                                   /**< Channel 3 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_SHIFT           3                                              /**< Shift value for DMA_CH3WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_MASK            0x8UL                                          /**< Bit mask for DMA_CH3WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT         0x00000001UL                                   /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH3WAITSTATUS_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH4WAITSTATUS                  (0x1UL << 4)                                   /**< Channel 4 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_SHIFT           4                                              /**< Shift value for DMA_CH4WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_MASK            0x10UL                                         /**< Bit mask for DMA_CH4WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT         0x00000001UL                                   /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH4WAITSTATUS_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH5WAITSTATUS                  (0x1UL << 5)                                   /**< Channel 5 Wait on Request Status */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_SHIFT           5                                              /**< Shift value for DMA_CH5WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_MASK            0x20UL                                         /**< Bit mask for DMA_CH5WAITSTATUS */
#define _DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT         0x00000001UL                                   /**< Mode DEFAULT for DMA_CHWAITSTATUS */
#define DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT          (_DMA_CHWAITSTATUS_CH5WAITSTATUS_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHWAITSTATUS */

/* Bit fields for DMA CHSWREQ */
#define _DMA_CHSWREQ_RESETVALUE                         0x00000000UL                         /**< Default value for DMA_CHSWREQ */
#define _DMA_CHSWREQ_MASK                               0x0000003FUL                         /**< Mask for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH0SWREQ                            (0x1UL << 0)                         /**< Channel 0 Software Request */
#define _DMA_CHSWREQ_CH0SWREQ_SHIFT                     0                                    /**< Shift value for DMA_CH0SWREQ */
#define _DMA_CHSWREQ_CH0SWREQ_MASK                      0x1UL                                /**< Bit mask for DMA_CH0SWREQ */
#define _DMA_CHSWREQ_CH0SWREQ_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH0SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH0SWREQ_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH1SWREQ                            (0x1UL << 1)                         /**< Channel 1 Software Request */
#define _DMA_CHSWREQ_CH1SWREQ_SHIFT                     1                                    /**< Shift value for DMA_CH1SWREQ */
#define _DMA_CHSWREQ_CH1SWREQ_MASK                      0x2UL                                /**< Bit mask for DMA_CH1SWREQ */
#define _DMA_CHSWREQ_CH1SWREQ_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH1SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH1SWREQ_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH2SWREQ                            (0x1UL << 2)                         /**< Channel 2 Software Request */
#define _DMA_CHSWREQ_CH2SWREQ_SHIFT                     2                                    /**< Shift value for DMA_CH2SWREQ */
#define _DMA_CHSWREQ_CH2SWREQ_MASK                      0x4UL                                /**< Bit mask for DMA_CH2SWREQ */
#define _DMA_CHSWREQ_CH2SWREQ_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH2SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH2SWREQ_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH3SWREQ                            (0x1UL << 3)                         /**< Channel 3 Software Request */
#define _DMA_CHSWREQ_CH3SWREQ_SHIFT                     3                                    /**< Shift value for DMA_CH3SWREQ */
#define _DMA_CHSWREQ_CH3SWREQ_MASK                      0x8UL                                /**< Bit mask for DMA_CH3SWREQ */
#define _DMA_CHSWREQ_CH3SWREQ_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH3SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH3SWREQ_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH4SWREQ                            (0x1UL << 4)                         /**< Channel 4 Software Request */
#define _DMA_CHSWREQ_CH4SWREQ_SHIFT                     4                                    /**< Shift value for DMA_CH4SWREQ */
#define _DMA_CHSWREQ_CH4SWREQ_MASK                      0x10UL                               /**< Bit mask for DMA_CH4SWREQ */
#define _DMA_CHSWREQ_CH4SWREQ_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH4SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH4SWREQ_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH5SWREQ                            (0x1UL << 5)                         /**< Channel 5 Software Request */
#define _DMA_CHSWREQ_CH5SWREQ_SHIFT                     5                                    /**< Shift value for DMA_CH5SWREQ */
#define _DMA_CHSWREQ_CH5SWREQ_MASK                      0x20UL                               /**< Bit mask for DMA_CH5SWREQ */
#define _DMA_CHSWREQ_CH5SWREQ_DEFAULT                   0x00000000UL                         /**< Mode DEFAULT for DMA_CHSWREQ */
#define DMA_CHSWREQ_CH5SWREQ_DEFAULT                    (_DMA_CHSWREQ_CH5SWREQ_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHSWREQ */

/* Bit fields for DMA CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_RESETVALUE                     0x00000000UL                                        /**< Default value for DMA_CHUSEBURSTS */
#define _DMA_CHUSEBURSTS_MASK                           0x0000003FUL                                        /**< Mask for DMA_CHUSEBURSTS */
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

/* Bit fields for DMA CHUSEBURSTC */
#define _DMA_CHUSEBURSTC_RESETVALUE                     0x00000000UL                                 /**< Default value for DMA_CHUSEBURSTC */
#define _DMA_CHUSEBURSTC_MASK                           0x0000003FUL                                 /**< Mask for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH0USEBURSTC                    (0x1UL << 0)                                 /**< Channel 0 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_SHIFT             0                                            /**< Shift value for DMA_CH0USEBURSTC */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_MASK              0x1UL                                        /**< Bit mask for DMA_CH0USEBURSTC */
#define _DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH0USEBURSTC_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH1USEBURSTC                    (0x1UL << 1)                                 /**< Channel 1 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_SHIFT             1                                            /**< Shift value for DMA_CH1USEBURSTC */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_MASK              0x2UL                                        /**< Bit mask for DMA_CH1USEBURSTC */
#define _DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH1USEBURSTC_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH2USEBURSTC                    (0x1UL << 2)                                 /**< Channel 2 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_SHIFT             2                                            /**< Shift value for DMA_CH2USEBURSTC */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_MASK              0x4UL                                        /**< Bit mask for DMA_CH2USEBURSTC */
#define _DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH2USEBURSTC_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH3USEBURSTC                    (0x1UL << 3)                                 /**< Channel 3 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_SHIFT             3                                            /**< Shift value for DMA_CH3USEBURSTC */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_MASK              0x8UL                                        /**< Bit mask for DMA_CH3USEBURSTC */
#define _DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH3USEBURSTC_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH4USEBURSTC                    (0x1UL << 4)                                 /**< Channel 4 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_SHIFT             4                                            /**< Shift value for DMA_CH4USEBURSTC */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_MASK              0x10UL                                       /**< Bit mask for DMA_CH4USEBURSTC */
#define _DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH4USEBURSTC_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH5USEBURSTC                    (0x1UL << 5)                                 /**< Channel 5 Useburst Clear */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_SHIFT             5                                            /**< Shift value for DMA_CH5USEBURSTC */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_MASK              0x20UL                                       /**< Bit mask for DMA_CH5USEBURSTC */
#define _DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHUSEBURSTC */
#define DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT            (_DMA_CHUSEBURSTC_CH5USEBURSTC_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHUSEBURSTC */

/* Bit fields for DMA CHREQMASKS */
#define _DMA_CHREQMASKS_RESETVALUE                      0x00000000UL                               /**< Default value for DMA_CHREQMASKS */
#define _DMA_CHREQMASKS_MASK                            0x0000003FUL                               /**< Mask for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH0REQMASKS                      (0x1UL << 0)                               /**< Channel 0 Request Mask Set */
#define _DMA_CHREQMASKS_CH0REQMASKS_SHIFT               0                                          /**< Shift value for DMA_CH0REQMASKS */
#define _DMA_CHREQMASKS_CH0REQMASKS_MASK                0x1UL                                      /**< Bit mask for DMA_CH0REQMASKS */
#define _DMA_CHREQMASKS_CH0REQMASKS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH0REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH0REQMASKS_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH1REQMASKS                      (0x1UL << 1)                               /**< Channel 1 Request Mask Set */
#define _DMA_CHREQMASKS_CH1REQMASKS_SHIFT               1                                          /**< Shift value for DMA_CH1REQMASKS */
#define _DMA_CHREQMASKS_CH1REQMASKS_MASK                0x2UL                                      /**< Bit mask for DMA_CH1REQMASKS */
#define _DMA_CHREQMASKS_CH1REQMASKS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH1REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH1REQMASKS_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH2REQMASKS                      (0x1UL << 2)                               /**< Channel 2 Request Mask Set */
#define _DMA_CHREQMASKS_CH2REQMASKS_SHIFT               2                                          /**< Shift value for DMA_CH2REQMASKS */
#define _DMA_CHREQMASKS_CH2REQMASKS_MASK                0x4UL                                      /**< Bit mask for DMA_CH2REQMASKS */
#define _DMA_CHREQMASKS_CH2REQMASKS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH2REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH2REQMASKS_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH3REQMASKS                      (0x1UL << 3)                               /**< Channel 3 Request Mask Set */
#define _DMA_CHREQMASKS_CH3REQMASKS_SHIFT               3                                          /**< Shift value for DMA_CH3REQMASKS */
#define _DMA_CHREQMASKS_CH3REQMASKS_MASK                0x8UL                                      /**< Bit mask for DMA_CH3REQMASKS */
#define _DMA_CHREQMASKS_CH3REQMASKS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH3REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH3REQMASKS_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH4REQMASKS                      (0x1UL << 4)                               /**< Channel 4 Request Mask Set */
#define _DMA_CHREQMASKS_CH4REQMASKS_SHIFT               4                                          /**< Shift value for DMA_CH4REQMASKS */
#define _DMA_CHREQMASKS_CH4REQMASKS_MASK                0x10UL                                     /**< Bit mask for DMA_CH4REQMASKS */
#define _DMA_CHREQMASKS_CH4REQMASKS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH4REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH4REQMASKS_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH5REQMASKS                      (0x1UL << 5)                               /**< Channel 5 Request Mask Set */
#define _DMA_CHREQMASKS_CH5REQMASKS_SHIFT               5                                          /**< Shift value for DMA_CH5REQMASKS */
#define _DMA_CHREQMASKS_CH5REQMASKS_MASK                0x20UL                                     /**< Bit mask for DMA_CH5REQMASKS */
#define _DMA_CHREQMASKS_CH5REQMASKS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKS */
#define DMA_CHREQMASKS_CH5REQMASKS_DEFAULT              (_DMA_CHREQMASKS_CH5REQMASKS_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHREQMASKS */

/* Bit fields for DMA CHREQMASKC */
#define _DMA_CHREQMASKC_RESETVALUE                      0x00000000UL                               /**< Default value for DMA_CHREQMASKC */
#define _DMA_CHREQMASKC_MASK                            0x0000003FUL                               /**< Mask for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH0REQMASKC                      (0x1UL << 0)                               /**< Channel 0 Request Mask Clear */
#define _DMA_CHREQMASKC_CH0REQMASKC_SHIFT               0                                          /**< Shift value for DMA_CH0REQMASKC */
#define _DMA_CHREQMASKC_CH0REQMASKC_MASK                0x1UL                                      /**< Bit mask for DMA_CH0REQMASKC */
#define _DMA_CHREQMASKC_CH0REQMASKC_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH0REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH0REQMASKC_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH1REQMASKC                      (0x1UL << 1)                               /**< Channel 1 Request Mask Clear */
#define _DMA_CHREQMASKC_CH1REQMASKC_SHIFT               1                                          /**< Shift value for DMA_CH1REQMASKC */
#define _DMA_CHREQMASKC_CH1REQMASKC_MASK                0x2UL                                      /**< Bit mask for DMA_CH1REQMASKC */
#define _DMA_CHREQMASKC_CH1REQMASKC_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH1REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH1REQMASKC_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH2REQMASKC                      (0x1UL << 2)                               /**< Channel 2 Request Mask Clear */
#define _DMA_CHREQMASKC_CH2REQMASKC_SHIFT               2                                          /**< Shift value for DMA_CH2REQMASKC */
#define _DMA_CHREQMASKC_CH2REQMASKC_MASK                0x4UL                                      /**< Bit mask for DMA_CH2REQMASKC */
#define _DMA_CHREQMASKC_CH2REQMASKC_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH2REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH2REQMASKC_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH3REQMASKC                      (0x1UL << 3)                               /**< Channel 3 Request Mask Clear */
#define _DMA_CHREQMASKC_CH3REQMASKC_SHIFT               3                                          /**< Shift value for DMA_CH3REQMASKC */
#define _DMA_CHREQMASKC_CH3REQMASKC_MASK                0x8UL                                      /**< Bit mask for DMA_CH3REQMASKC */
#define _DMA_CHREQMASKC_CH3REQMASKC_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH3REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH3REQMASKC_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH4REQMASKC                      (0x1UL << 4)                               /**< Channel 4 Request Mask Clear */
#define _DMA_CHREQMASKC_CH4REQMASKC_SHIFT               4                                          /**< Shift value for DMA_CH4REQMASKC */
#define _DMA_CHREQMASKC_CH4REQMASKC_MASK                0x10UL                                     /**< Bit mask for DMA_CH4REQMASKC */
#define _DMA_CHREQMASKC_CH4REQMASKC_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH4REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH4REQMASKC_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH5REQMASKC                      (0x1UL << 5)                               /**< Channel 5 Request Mask Clear */
#define _DMA_CHREQMASKC_CH5REQMASKC_SHIFT               5                                          /**< Shift value for DMA_CH5REQMASKC */
#define _DMA_CHREQMASKC_CH5REQMASKC_MASK                0x20UL                                     /**< Bit mask for DMA_CH5REQMASKC */
#define _DMA_CHREQMASKC_CH5REQMASKC_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for DMA_CHREQMASKC */
#define DMA_CHREQMASKC_CH5REQMASKC_DEFAULT              (_DMA_CHREQMASKC_CH5REQMASKC_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHREQMASKC */

/* Bit fields for DMA CHENS */
#define _DMA_CHENS_RESETVALUE                           0x00000000UL                     /**< Default value for DMA_CHENS */
#define _DMA_CHENS_MASK                                 0x0000003FUL                     /**< Mask for DMA_CHENS */
#define DMA_CHENS_CH0ENS                                (0x1UL << 0)                     /**< Channel 0 Enable Set */
#define _DMA_CHENS_CH0ENS_SHIFT                         0                                /**< Shift value for DMA_CH0ENS */
#define _DMA_CHENS_CH0ENS_MASK                          0x1UL                            /**< Bit mask for DMA_CH0ENS */
#define _DMA_CHENS_CH0ENS_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH0ENS_DEFAULT                        (_DMA_CHENS_CH0ENS_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH1ENS                                (0x1UL << 1)                     /**< Channel 1 Enable Set */
#define _DMA_CHENS_CH1ENS_SHIFT                         1                                /**< Shift value for DMA_CH1ENS */
#define _DMA_CHENS_CH1ENS_MASK                          0x2UL                            /**< Bit mask for DMA_CH1ENS */
#define _DMA_CHENS_CH1ENS_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH1ENS_DEFAULT                        (_DMA_CHENS_CH1ENS_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH2ENS                                (0x1UL << 2)                     /**< Channel 2 Enable Set */
#define _DMA_CHENS_CH2ENS_SHIFT                         2                                /**< Shift value for DMA_CH2ENS */
#define _DMA_CHENS_CH2ENS_MASK                          0x4UL                            /**< Bit mask for DMA_CH2ENS */
#define _DMA_CHENS_CH2ENS_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH2ENS_DEFAULT                        (_DMA_CHENS_CH2ENS_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH3ENS                                (0x1UL << 3)                     /**< Channel 3 Enable Set */
#define _DMA_CHENS_CH3ENS_SHIFT                         3                                /**< Shift value for DMA_CH3ENS */
#define _DMA_CHENS_CH3ENS_MASK                          0x8UL                            /**< Bit mask for DMA_CH3ENS */
#define _DMA_CHENS_CH3ENS_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH3ENS_DEFAULT                        (_DMA_CHENS_CH3ENS_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH4ENS                                (0x1UL << 4)                     /**< Channel 4 Enable Set */
#define _DMA_CHENS_CH4ENS_SHIFT                         4                                /**< Shift value for DMA_CH4ENS */
#define _DMA_CHENS_CH4ENS_MASK                          0x10UL                           /**< Bit mask for DMA_CH4ENS */
#define _DMA_CHENS_CH4ENS_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH4ENS_DEFAULT                        (_DMA_CHENS_CH4ENS_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH5ENS                                (0x1UL << 5)                     /**< Channel 5 Enable Set */
#define _DMA_CHENS_CH5ENS_SHIFT                         5                                /**< Shift value for DMA_CH5ENS */
#define _DMA_CHENS_CH5ENS_MASK                          0x20UL                           /**< Bit mask for DMA_CH5ENS */
#define _DMA_CHENS_CH5ENS_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENS */
#define DMA_CHENS_CH5ENS_DEFAULT                        (_DMA_CHENS_CH5ENS_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHENS */

/* Bit fields for DMA CHENC */
#define _DMA_CHENC_RESETVALUE                           0x00000000UL                     /**< Default value for DMA_CHENC */
#define _DMA_CHENC_MASK                                 0x0000003FUL                     /**< Mask for DMA_CHENC */
#define DMA_CHENC_CH0ENC                                (0x1UL << 0)                     /**< Channel 0 Enable Clear */
#define _DMA_CHENC_CH0ENC_SHIFT                         0                                /**< Shift value for DMA_CH0ENC */
#define _DMA_CHENC_CH0ENC_MASK                          0x1UL                            /**< Bit mask for DMA_CH0ENC */
#define _DMA_CHENC_CH0ENC_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH0ENC_DEFAULT                        (_DMA_CHENC_CH0ENC_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH1ENC                                (0x1UL << 1)                     /**< Channel 1 Enable Clear */
#define _DMA_CHENC_CH1ENC_SHIFT                         1                                /**< Shift value for DMA_CH1ENC */
#define _DMA_CHENC_CH1ENC_MASK                          0x2UL                            /**< Bit mask for DMA_CH1ENC */
#define _DMA_CHENC_CH1ENC_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH1ENC_DEFAULT                        (_DMA_CHENC_CH1ENC_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH2ENC                                (0x1UL << 2)                     /**< Channel 2 Enable Clear */
#define _DMA_CHENC_CH2ENC_SHIFT                         2                                /**< Shift value for DMA_CH2ENC */
#define _DMA_CHENC_CH2ENC_MASK                          0x4UL                            /**< Bit mask for DMA_CH2ENC */
#define _DMA_CHENC_CH2ENC_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH2ENC_DEFAULT                        (_DMA_CHENC_CH2ENC_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH3ENC                                (0x1UL << 3)                     /**< Channel 3 Enable Clear */
#define _DMA_CHENC_CH3ENC_SHIFT                         3                                /**< Shift value for DMA_CH3ENC */
#define _DMA_CHENC_CH3ENC_MASK                          0x8UL                            /**< Bit mask for DMA_CH3ENC */
#define _DMA_CHENC_CH3ENC_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH3ENC_DEFAULT                        (_DMA_CHENC_CH3ENC_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH4ENC                                (0x1UL << 4)                     /**< Channel 4 Enable Clear */
#define _DMA_CHENC_CH4ENC_SHIFT                         4                                /**< Shift value for DMA_CH4ENC */
#define _DMA_CHENC_CH4ENC_MASK                          0x10UL                           /**< Bit mask for DMA_CH4ENC */
#define _DMA_CHENC_CH4ENC_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH4ENC_DEFAULT                        (_DMA_CHENC_CH4ENC_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH5ENC                                (0x1UL << 5)                     /**< Channel 5 Enable Clear */
#define _DMA_CHENC_CH5ENC_SHIFT                         5                                /**< Shift value for DMA_CH5ENC */
#define _DMA_CHENC_CH5ENC_MASK                          0x20UL                           /**< Bit mask for DMA_CH5ENC */
#define _DMA_CHENC_CH5ENC_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for DMA_CHENC */
#define DMA_CHENC_CH5ENC_DEFAULT                        (_DMA_CHENC_CH5ENC_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHENC */

/* Bit fields for DMA CHALTS */
#define _DMA_CHALTS_RESETVALUE                          0x00000000UL                       /**< Default value for DMA_CHALTS */
#define _DMA_CHALTS_MASK                                0x0000003FUL                       /**< Mask for DMA_CHALTS */
#define DMA_CHALTS_CH0ALTS                              (0x1UL << 0)                       /**< Channel 0 Alternate Structure Set */
#define _DMA_CHALTS_CH0ALTS_SHIFT                       0                                  /**< Shift value for DMA_CH0ALTS */
#define _DMA_CHALTS_CH0ALTS_MASK                        0x1UL                              /**< Bit mask for DMA_CH0ALTS */
#define _DMA_CHALTS_CH0ALTS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH0ALTS_DEFAULT                      (_DMA_CHALTS_CH0ALTS_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH1ALTS                              (0x1UL << 1)                       /**< Channel 1 Alternate Structure Set */
#define _DMA_CHALTS_CH1ALTS_SHIFT                       1                                  /**< Shift value for DMA_CH1ALTS */
#define _DMA_CHALTS_CH1ALTS_MASK                        0x2UL                              /**< Bit mask for DMA_CH1ALTS */
#define _DMA_CHALTS_CH1ALTS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH1ALTS_DEFAULT                      (_DMA_CHALTS_CH1ALTS_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH2ALTS                              (0x1UL << 2)                       /**< Channel 2 Alternate Structure Set */
#define _DMA_CHALTS_CH2ALTS_SHIFT                       2                                  /**< Shift value for DMA_CH2ALTS */
#define _DMA_CHALTS_CH2ALTS_MASK                        0x4UL                              /**< Bit mask for DMA_CH2ALTS */
#define _DMA_CHALTS_CH2ALTS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH2ALTS_DEFAULT                      (_DMA_CHALTS_CH2ALTS_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH3ALTS                              (0x1UL << 3)                       /**< Channel 3 Alternate Structure Set */
#define _DMA_CHALTS_CH3ALTS_SHIFT                       3                                  /**< Shift value for DMA_CH3ALTS */
#define _DMA_CHALTS_CH3ALTS_MASK                        0x8UL                              /**< Bit mask for DMA_CH3ALTS */
#define _DMA_CHALTS_CH3ALTS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH3ALTS_DEFAULT                      (_DMA_CHALTS_CH3ALTS_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH4ALTS                              (0x1UL << 4)                       /**< Channel 4 Alternate Structure Set */
#define _DMA_CHALTS_CH4ALTS_SHIFT                       4                                  /**< Shift value for DMA_CH4ALTS */
#define _DMA_CHALTS_CH4ALTS_MASK                        0x10UL                             /**< Bit mask for DMA_CH4ALTS */
#define _DMA_CHALTS_CH4ALTS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH4ALTS_DEFAULT                      (_DMA_CHALTS_CH4ALTS_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH5ALTS                              (0x1UL << 5)                       /**< Channel 5 Alternate Structure Set */
#define _DMA_CHALTS_CH5ALTS_SHIFT                       5                                  /**< Shift value for DMA_CH5ALTS */
#define _DMA_CHALTS_CH5ALTS_MASK                        0x20UL                             /**< Bit mask for DMA_CH5ALTS */
#define _DMA_CHALTS_CH5ALTS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTS */
#define DMA_CHALTS_CH5ALTS_DEFAULT                      (_DMA_CHALTS_CH5ALTS_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHALTS */

/* Bit fields for DMA CHALTC */
#define _DMA_CHALTC_RESETVALUE                          0x00000000UL                       /**< Default value for DMA_CHALTC */
#define _DMA_CHALTC_MASK                                0x0000003FUL                       /**< Mask for DMA_CHALTC */
#define DMA_CHALTC_CH0ALTC                              (0x1UL << 0)                       /**< Channel 0 Alternate Clear */
#define _DMA_CHALTC_CH0ALTC_SHIFT                       0                                  /**< Shift value for DMA_CH0ALTC */
#define _DMA_CHALTC_CH0ALTC_MASK                        0x1UL                              /**< Bit mask for DMA_CH0ALTC */
#define _DMA_CHALTC_CH0ALTC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH0ALTC_DEFAULT                      (_DMA_CHALTC_CH0ALTC_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH1ALTC                              (0x1UL << 1)                       /**< Channel 1 Alternate Clear */
#define _DMA_CHALTC_CH1ALTC_SHIFT                       1                                  /**< Shift value for DMA_CH1ALTC */
#define _DMA_CHALTC_CH1ALTC_MASK                        0x2UL                              /**< Bit mask for DMA_CH1ALTC */
#define _DMA_CHALTC_CH1ALTC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH1ALTC_DEFAULT                      (_DMA_CHALTC_CH1ALTC_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH2ALTC                              (0x1UL << 2)                       /**< Channel 2 Alternate Clear */
#define _DMA_CHALTC_CH2ALTC_SHIFT                       2                                  /**< Shift value for DMA_CH2ALTC */
#define _DMA_CHALTC_CH2ALTC_MASK                        0x4UL                              /**< Bit mask for DMA_CH2ALTC */
#define _DMA_CHALTC_CH2ALTC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH2ALTC_DEFAULT                      (_DMA_CHALTC_CH2ALTC_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH3ALTC                              (0x1UL << 3)                       /**< Channel 3 Alternate Clear */
#define _DMA_CHALTC_CH3ALTC_SHIFT                       3                                  /**< Shift value for DMA_CH3ALTC */
#define _DMA_CHALTC_CH3ALTC_MASK                        0x8UL                              /**< Bit mask for DMA_CH3ALTC */
#define _DMA_CHALTC_CH3ALTC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH3ALTC_DEFAULT                      (_DMA_CHALTC_CH3ALTC_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH4ALTC                              (0x1UL << 4)                       /**< Channel 4 Alternate Clear */
#define _DMA_CHALTC_CH4ALTC_SHIFT                       4                                  /**< Shift value for DMA_CH4ALTC */
#define _DMA_CHALTC_CH4ALTC_MASK                        0x10UL                             /**< Bit mask for DMA_CH4ALTC */
#define _DMA_CHALTC_CH4ALTC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH4ALTC_DEFAULT                      (_DMA_CHALTC_CH4ALTC_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH5ALTC                              (0x1UL << 5)                       /**< Channel 5 Alternate Clear */
#define _DMA_CHALTC_CH5ALTC_SHIFT                       5                                  /**< Shift value for DMA_CH5ALTC */
#define _DMA_CHALTC_CH5ALTC_MASK                        0x20UL                             /**< Bit mask for DMA_CH5ALTC */
#define _DMA_CHALTC_CH5ALTC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHALTC */
#define DMA_CHALTC_CH5ALTC_DEFAULT                      (_DMA_CHALTC_CH5ALTC_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHALTC */

/* Bit fields for DMA CHPRIS */
#define _DMA_CHPRIS_RESETVALUE                          0x00000000UL                       /**< Default value for DMA_CHPRIS */
#define _DMA_CHPRIS_MASK                                0x0000003FUL                       /**< Mask for DMA_CHPRIS */
#define DMA_CHPRIS_CH0PRIS                              (0x1UL << 0)                       /**< Channel 0 High Priority Set */
#define _DMA_CHPRIS_CH0PRIS_SHIFT                       0                                  /**< Shift value for DMA_CH0PRIS */
#define _DMA_CHPRIS_CH0PRIS_MASK                        0x1UL                              /**< Bit mask for DMA_CH0PRIS */
#define _DMA_CHPRIS_CH0PRIS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH0PRIS_DEFAULT                      (_DMA_CHPRIS_CH0PRIS_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH1PRIS                              (0x1UL << 1)                       /**< Channel 1 High Priority Set */
#define _DMA_CHPRIS_CH1PRIS_SHIFT                       1                                  /**< Shift value for DMA_CH1PRIS */
#define _DMA_CHPRIS_CH1PRIS_MASK                        0x2UL                              /**< Bit mask for DMA_CH1PRIS */
#define _DMA_CHPRIS_CH1PRIS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH1PRIS_DEFAULT                      (_DMA_CHPRIS_CH1PRIS_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH2PRIS                              (0x1UL << 2)                       /**< Channel 2 High Priority Set */
#define _DMA_CHPRIS_CH2PRIS_SHIFT                       2                                  /**< Shift value for DMA_CH2PRIS */
#define _DMA_CHPRIS_CH2PRIS_MASK                        0x4UL                              /**< Bit mask for DMA_CH2PRIS */
#define _DMA_CHPRIS_CH2PRIS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH2PRIS_DEFAULT                      (_DMA_CHPRIS_CH2PRIS_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH3PRIS                              (0x1UL << 3)                       /**< Channel 3 High Priority Set */
#define _DMA_CHPRIS_CH3PRIS_SHIFT                       3                                  /**< Shift value for DMA_CH3PRIS */
#define _DMA_CHPRIS_CH3PRIS_MASK                        0x8UL                              /**< Bit mask for DMA_CH3PRIS */
#define _DMA_CHPRIS_CH3PRIS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH3PRIS_DEFAULT                      (_DMA_CHPRIS_CH3PRIS_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH4PRIS                              (0x1UL << 4)                       /**< Channel 4 High Priority Set */
#define _DMA_CHPRIS_CH4PRIS_SHIFT                       4                                  /**< Shift value for DMA_CH4PRIS */
#define _DMA_CHPRIS_CH4PRIS_MASK                        0x10UL                             /**< Bit mask for DMA_CH4PRIS */
#define _DMA_CHPRIS_CH4PRIS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH4PRIS_DEFAULT                      (_DMA_CHPRIS_CH4PRIS_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH5PRIS                              (0x1UL << 5)                       /**< Channel 5 High Priority Set */
#define _DMA_CHPRIS_CH5PRIS_SHIFT                       5                                  /**< Shift value for DMA_CH5PRIS */
#define _DMA_CHPRIS_CH5PRIS_MASK                        0x20UL                             /**< Bit mask for DMA_CH5PRIS */
#define _DMA_CHPRIS_CH5PRIS_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIS */
#define DMA_CHPRIS_CH5PRIS_DEFAULT                      (_DMA_CHPRIS_CH5PRIS_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHPRIS */

/* Bit fields for DMA CHPRIC */
#define _DMA_CHPRIC_RESETVALUE                          0x00000000UL                       /**< Default value for DMA_CHPRIC */
#define _DMA_CHPRIC_MASK                                0x0000003FUL                       /**< Mask for DMA_CHPRIC */
#define DMA_CHPRIC_CH0PRIC                              (0x1UL << 0)                       /**< Channel 0 High Priority Clear */
#define _DMA_CHPRIC_CH0PRIC_SHIFT                       0                                  /**< Shift value for DMA_CH0PRIC */
#define _DMA_CHPRIC_CH0PRIC_MASK                        0x1UL                              /**< Bit mask for DMA_CH0PRIC */
#define _DMA_CHPRIC_CH0PRIC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH0PRIC_DEFAULT                      (_DMA_CHPRIC_CH0PRIC_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH1PRIC                              (0x1UL << 1)                       /**< Channel 1 High Priority Clear */
#define _DMA_CHPRIC_CH1PRIC_SHIFT                       1                                  /**< Shift value for DMA_CH1PRIC */
#define _DMA_CHPRIC_CH1PRIC_MASK                        0x2UL                              /**< Bit mask for DMA_CH1PRIC */
#define _DMA_CHPRIC_CH1PRIC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH1PRIC_DEFAULT                      (_DMA_CHPRIC_CH1PRIC_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH2PRIC                              (0x1UL << 2)                       /**< Channel 2 High Priority Clear */
#define _DMA_CHPRIC_CH2PRIC_SHIFT                       2                                  /**< Shift value for DMA_CH2PRIC */
#define _DMA_CHPRIC_CH2PRIC_MASK                        0x4UL                              /**< Bit mask for DMA_CH2PRIC */
#define _DMA_CHPRIC_CH2PRIC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH2PRIC_DEFAULT                      (_DMA_CHPRIC_CH2PRIC_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH3PRIC                              (0x1UL << 3)                       /**< Channel 3 High Priority Clear */
#define _DMA_CHPRIC_CH3PRIC_SHIFT                       3                                  /**< Shift value for DMA_CH3PRIC */
#define _DMA_CHPRIC_CH3PRIC_MASK                        0x8UL                              /**< Bit mask for DMA_CH3PRIC */
#define _DMA_CHPRIC_CH3PRIC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH3PRIC_DEFAULT                      (_DMA_CHPRIC_CH3PRIC_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH4PRIC                              (0x1UL << 4)                       /**< Channel 4 High Priority Clear */
#define _DMA_CHPRIC_CH4PRIC_SHIFT                       4                                  /**< Shift value for DMA_CH4PRIC */
#define _DMA_CHPRIC_CH4PRIC_MASK                        0x10UL                             /**< Bit mask for DMA_CH4PRIC */
#define _DMA_CHPRIC_CH4PRIC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH4PRIC_DEFAULT                      (_DMA_CHPRIC_CH4PRIC_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH5PRIC                              (0x1UL << 5)                       /**< Channel 5 High Priority Clear */
#define _DMA_CHPRIC_CH5PRIC_SHIFT                       5                                  /**< Shift value for DMA_CH5PRIC */
#define _DMA_CHPRIC_CH5PRIC_MASK                        0x20UL                             /**< Bit mask for DMA_CH5PRIC */
#define _DMA_CHPRIC_CH5PRIC_DEFAULT                     0x00000000UL                       /**< Mode DEFAULT for DMA_CHPRIC */
#define DMA_CHPRIC_CH5PRIC_DEFAULT                      (_DMA_CHPRIC_CH5PRIC_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHPRIC */

/* Bit fields for DMA ERRORC */
#define _DMA_ERRORC_RESETVALUE                          0x00000000UL                      /**< Default value for DMA_ERRORC */
#define _DMA_ERRORC_MASK                                0x00000001UL                      /**< Mask for DMA_ERRORC */
#define DMA_ERRORC_ERRORC                               (0x1UL << 0)                      /**< Bus Error Clear */
#define _DMA_ERRORC_ERRORC_SHIFT                        0                                 /**< Shift value for DMA_ERRORC */
#define _DMA_ERRORC_ERRORC_MASK                         0x1UL                             /**< Bit mask for DMA_ERRORC */
#define _DMA_ERRORC_ERRORC_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for DMA_ERRORC */
#define DMA_ERRORC_ERRORC_DEFAULT                       (_DMA_ERRORC_ERRORC_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_ERRORC */

/* Bit fields for DMA CHREQSTATUS */
#define _DMA_CHREQSTATUS_RESETVALUE                     0x00000000UL                                 /**< Default value for DMA_CHREQSTATUS */
#define _DMA_CHREQSTATUS_MASK                           0x0000003FUL                                 /**< Mask for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH0REQSTATUS                    (0x1UL << 0)                                 /**< Channel 0 Request Status */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_SHIFT             0                                            /**< Shift value for DMA_CH0REQSTATUS */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_MASK              0x1UL                                        /**< Bit mask for DMA_CH0REQSTATUS */
#define _DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH0REQSTATUS_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH1REQSTATUS                    (0x1UL << 1)                                 /**< Channel 1 Request Status */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_SHIFT             1                                            /**< Shift value for DMA_CH1REQSTATUS */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_MASK              0x2UL                                        /**< Bit mask for DMA_CH1REQSTATUS */
#define _DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH1REQSTATUS_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH2REQSTATUS                    (0x1UL << 2)                                 /**< Channel 2 Request Status */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_SHIFT             2                                            /**< Shift value for DMA_CH2REQSTATUS */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_MASK              0x4UL                                        /**< Bit mask for DMA_CH2REQSTATUS */
#define _DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH2REQSTATUS_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH3REQSTATUS                    (0x1UL << 3)                                 /**< Channel 3 Request Status */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_SHIFT             3                                            /**< Shift value for DMA_CH3REQSTATUS */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_MASK              0x8UL                                        /**< Bit mask for DMA_CH3REQSTATUS */
#define _DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH3REQSTATUS_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH4REQSTATUS                    (0x1UL << 4)                                 /**< Channel 4 Request Status */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_SHIFT             4                                            /**< Shift value for DMA_CH4REQSTATUS */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_MASK              0x10UL                                       /**< Bit mask for DMA_CH4REQSTATUS */
#define _DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH4REQSTATUS_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH5REQSTATUS                    (0x1UL << 5)                                 /**< Channel 5 Request Status */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_SHIFT             5                                            /**< Shift value for DMA_CH5REQSTATUS */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_MASK              0x20UL                                       /**< Bit mask for DMA_CH5REQSTATUS */
#define _DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for DMA_CHREQSTATUS */
#define DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT            (_DMA_CHREQSTATUS_CH5REQSTATUS_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHREQSTATUS */

/* Bit fields for DMA CHSREQSTATUS */
#define _DMA_CHSREQSTATUS_RESETVALUE                    0x00000000UL                                   /**< Default value for DMA_CHSREQSTATUS */
#define _DMA_CHSREQSTATUS_MASK                          0x0000003FUL                                   /**< Mask for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH0SREQSTATUS                  (0x1UL << 0)                                   /**< Channel 0 Single Request Status */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_SHIFT           0                                              /**< Shift value for DMA_CH0SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_MASK            0x1UL                                          /**< Bit mask for DMA_CH0SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT         0x00000000UL                                   /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH0SREQSTATUS_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH1SREQSTATUS                  (0x1UL << 1)                                   /**< Channel 1 Single Request Status */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_SHIFT           1                                              /**< Shift value for DMA_CH1SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_MASK            0x2UL                                          /**< Bit mask for DMA_CH1SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT         0x00000000UL                                   /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH1SREQSTATUS_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH2SREQSTATUS                  (0x1UL << 2)                                   /**< Channel 2 Single Request Status */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_SHIFT           2                                              /**< Shift value for DMA_CH2SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_MASK            0x4UL                                          /**< Bit mask for DMA_CH2SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT         0x00000000UL                                   /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH2SREQSTATUS_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH3SREQSTATUS                  (0x1UL << 3)                                   /**< Channel 3 Single Request Status */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_SHIFT           3                                              /**< Shift value for DMA_CH3SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_MASK            0x8UL                                          /**< Bit mask for DMA_CH3SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT         0x00000000UL                                   /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH3SREQSTATUS_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH4SREQSTATUS                  (0x1UL << 4)                                   /**< Channel 4 Single Request Status */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_SHIFT           4                                              /**< Shift value for DMA_CH4SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_MASK            0x10UL                                         /**< Bit mask for DMA_CH4SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT         0x00000000UL                                   /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH4SREQSTATUS_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH5SREQSTATUS                  (0x1UL << 5)                                   /**< Channel 5 Single Request Status */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_SHIFT           5                                              /**< Shift value for DMA_CH5SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_MASK            0x20UL                                         /**< Bit mask for DMA_CH5SREQSTATUS */
#define _DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT         0x00000000UL                                   /**< Mode DEFAULT for DMA_CHSREQSTATUS */
#define DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT          (_DMA_CHSREQSTATUS_CH5SREQSTATUS_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_CHSREQSTATUS */

/* Bit fields for DMA IF */
#define _DMA_IF_RESETVALUE                              0x00000000UL                   /**< Default value for DMA_IF */
#define _DMA_IF_MASK                                    0x8000003FUL                   /**< Mask for DMA_IF */
#define DMA_IF_CH0DONE                                  (0x1UL << 0)                   /**< DMA Channel 0 Complete Interrupt Flag */
#define _DMA_IF_CH0DONE_SHIFT                           0                              /**< Shift value for DMA_CH0DONE */
#define _DMA_IF_CH0DONE_MASK                            0x1UL                          /**< Bit mask for DMA_CH0DONE */
#define _DMA_IF_CH0DONE_DEFAULT                         0x00000000UL                   /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH0DONE_DEFAULT                          (_DMA_IF_CH0DONE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH1DONE                                  (0x1UL << 1)                   /**< DMA Channel 1 Complete Interrupt Flag */
#define _DMA_IF_CH1DONE_SHIFT                           1                              /**< Shift value for DMA_CH1DONE */
#define _DMA_IF_CH1DONE_MASK                            0x2UL                          /**< Bit mask for DMA_CH1DONE */
#define _DMA_IF_CH1DONE_DEFAULT                         0x00000000UL                   /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH1DONE_DEFAULT                          (_DMA_IF_CH1DONE_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH2DONE                                  (0x1UL << 2)                   /**< DMA Channel 2 Complete Interrupt Flag */
#define _DMA_IF_CH2DONE_SHIFT                           2                              /**< Shift value for DMA_CH2DONE */
#define _DMA_IF_CH2DONE_MASK                            0x4UL                          /**< Bit mask for DMA_CH2DONE */
#define _DMA_IF_CH2DONE_DEFAULT                         0x00000000UL                   /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH2DONE_DEFAULT                          (_DMA_IF_CH2DONE_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH3DONE                                  (0x1UL << 3)                   /**< DMA Channel 3 Complete Interrupt Flag */
#define _DMA_IF_CH3DONE_SHIFT                           3                              /**< Shift value for DMA_CH3DONE */
#define _DMA_IF_CH3DONE_MASK                            0x8UL                          /**< Bit mask for DMA_CH3DONE */
#define _DMA_IF_CH3DONE_DEFAULT                         0x00000000UL                   /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH3DONE_DEFAULT                          (_DMA_IF_CH3DONE_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH4DONE                                  (0x1UL << 4)                   /**< DMA Channel 4 Complete Interrupt Flag */
#define _DMA_IF_CH4DONE_SHIFT                           4                              /**< Shift value for DMA_CH4DONE */
#define _DMA_IF_CH4DONE_MASK                            0x10UL                         /**< Bit mask for DMA_CH4DONE */
#define _DMA_IF_CH4DONE_DEFAULT                         0x00000000UL                   /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH4DONE_DEFAULT                          (_DMA_IF_CH4DONE_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_CH5DONE                                  (0x1UL << 5)                   /**< DMA Channel 5 Complete Interrupt Flag */
#define _DMA_IF_CH5DONE_SHIFT                           5                              /**< Shift value for DMA_CH5DONE */
#define _DMA_IF_CH5DONE_MASK                            0x20UL                         /**< Bit mask for DMA_CH5DONE */
#define _DMA_IF_CH5DONE_DEFAULT                         0x00000000UL                   /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_CH5DONE_DEFAULT                          (_DMA_IF_CH5DONE_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_IF */
#define DMA_IF_ERR                                      (0x1UL << 31)                  /**< DMA Error Interrupt Flag */
#define _DMA_IF_ERR_SHIFT                               31                             /**< Shift value for DMA_ERR */
#define _DMA_IF_ERR_MASK                                0x80000000UL                   /**< Bit mask for DMA_ERR */
#define _DMA_IF_ERR_DEFAULT                             0x00000000UL                   /**< Mode DEFAULT for DMA_IF */
#define DMA_IF_ERR_DEFAULT                              (_DMA_IF_ERR_DEFAULT << 31)    /**< Shifted mode DEFAULT for DMA_IF */

/* Bit fields for DMA IFS */
#define _DMA_IFS_RESETVALUE                             0x00000000UL                    /**< Default value for DMA_IFS */
#define _DMA_IFS_MASK                                   0x8000003FUL                    /**< Mask for DMA_IFS */
#define DMA_IFS_CH0DONE                                 (0x1UL << 0)                    /**< DMA Channel 0 Complete Interrupt Flag Set */
#define _DMA_IFS_CH0DONE_SHIFT                          0                               /**< Shift value for DMA_CH0DONE */
#define _DMA_IFS_CH0DONE_MASK                           0x1UL                           /**< Bit mask for DMA_CH0DONE */
#define _DMA_IFS_CH0DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH0DONE_DEFAULT                         (_DMA_IFS_CH0DONE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH1DONE                                 (0x1UL << 1)                    /**< DMA Channel 1 Complete Interrupt Flag Set */
#define _DMA_IFS_CH1DONE_SHIFT                          1                               /**< Shift value for DMA_CH1DONE */
#define _DMA_IFS_CH1DONE_MASK                           0x2UL                           /**< Bit mask for DMA_CH1DONE */
#define _DMA_IFS_CH1DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH1DONE_DEFAULT                         (_DMA_IFS_CH1DONE_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH2DONE                                 (0x1UL << 2)                    /**< DMA Channel 2 Complete Interrupt Flag Set */
#define _DMA_IFS_CH2DONE_SHIFT                          2                               /**< Shift value for DMA_CH2DONE */
#define _DMA_IFS_CH2DONE_MASK                           0x4UL                           /**< Bit mask for DMA_CH2DONE */
#define _DMA_IFS_CH2DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH2DONE_DEFAULT                         (_DMA_IFS_CH2DONE_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH3DONE                                 (0x1UL << 3)                    /**< DMA Channel 3 Complete Interrupt Flag Set */
#define _DMA_IFS_CH3DONE_SHIFT                          3                               /**< Shift value for DMA_CH3DONE */
#define _DMA_IFS_CH3DONE_MASK                           0x8UL                           /**< Bit mask for DMA_CH3DONE */
#define _DMA_IFS_CH3DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH3DONE_DEFAULT                         (_DMA_IFS_CH3DONE_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH4DONE                                 (0x1UL << 4)                    /**< DMA Channel 4 Complete Interrupt Flag Set */
#define _DMA_IFS_CH4DONE_SHIFT                          4                               /**< Shift value for DMA_CH4DONE */
#define _DMA_IFS_CH4DONE_MASK                           0x10UL                          /**< Bit mask for DMA_CH4DONE */
#define _DMA_IFS_CH4DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH4DONE_DEFAULT                         (_DMA_IFS_CH4DONE_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH5DONE                                 (0x1UL << 5)                    /**< DMA Channel 5 Complete Interrupt Flag Set */
#define _DMA_IFS_CH5DONE_SHIFT                          5                               /**< Shift value for DMA_CH5DONE */
#define _DMA_IFS_CH5DONE_MASK                           0x20UL                          /**< Bit mask for DMA_CH5DONE */
#define _DMA_IFS_CH5DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_CH5DONE_DEFAULT                         (_DMA_IFS_CH5DONE_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_IFS */
#define DMA_IFS_ERR                                     (0x1UL << 31)                   /**< DMA Error Interrupt Flag Set */
#define _DMA_IFS_ERR_SHIFT                              31                              /**< Shift value for DMA_ERR */
#define _DMA_IFS_ERR_MASK                               0x80000000UL                    /**< Bit mask for DMA_ERR */
#define _DMA_IFS_ERR_DEFAULT                            0x00000000UL                    /**< Mode DEFAULT for DMA_IFS */
#define DMA_IFS_ERR_DEFAULT                             (_DMA_IFS_ERR_DEFAULT << 31)    /**< Shifted mode DEFAULT for DMA_IFS */

/* Bit fields for DMA IFC */
#define _DMA_IFC_RESETVALUE                             0x00000000UL                    /**< Default value for DMA_IFC */
#define _DMA_IFC_MASK                                   0x8000003FUL                    /**< Mask for DMA_IFC */
#define DMA_IFC_CH0DONE                                 (0x1UL << 0)                    /**< DMA Channel 0 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH0DONE_SHIFT                          0                               /**< Shift value for DMA_CH0DONE */
#define _DMA_IFC_CH0DONE_MASK                           0x1UL                           /**< Bit mask for DMA_CH0DONE */
#define _DMA_IFC_CH0DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH0DONE_DEFAULT                         (_DMA_IFC_CH0DONE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH1DONE                                 (0x1UL << 1)                    /**< DMA Channel 1 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH1DONE_SHIFT                          1                               /**< Shift value for DMA_CH1DONE */
#define _DMA_IFC_CH1DONE_MASK                           0x2UL                           /**< Bit mask for DMA_CH1DONE */
#define _DMA_IFC_CH1DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH1DONE_DEFAULT                         (_DMA_IFC_CH1DONE_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH2DONE                                 (0x1UL << 2)                    /**< DMA Channel 2 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH2DONE_SHIFT                          2                               /**< Shift value for DMA_CH2DONE */
#define _DMA_IFC_CH2DONE_MASK                           0x4UL                           /**< Bit mask for DMA_CH2DONE */
#define _DMA_IFC_CH2DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH2DONE_DEFAULT                         (_DMA_IFC_CH2DONE_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH3DONE                                 (0x1UL << 3)                    /**< DMA Channel 3 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH3DONE_SHIFT                          3                               /**< Shift value for DMA_CH3DONE */
#define _DMA_IFC_CH3DONE_MASK                           0x8UL                           /**< Bit mask for DMA_CH3DONE */
#define _DMA_IFC_CH3DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH3DONE_DEFAULT                         (_DMA_IFC_CH3DONE_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH4DONE                                 (0x1UL << 4)                    /**< DMA Channel 4 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH4DONE_SHIFT                          4                               /**< Shift value for DMA_CH4DONE */
#define _DMA_IFC_CH4DONE_MASK                           0x10UL                          /**< Bit mask for DMA_CH4DONE */
#define _DMA_IFC_CH4DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH4DONE_DEFAULT                         (_DMA_IFC_CH4DONE_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH5DONE                                 (0x1UL << 5)                    /**< DMA Channel 5 Complete Interrupt Flag Clear */
#define _DMA_IFC_CH5DONE_SHIFT                          5                               /**< Shift value for DMA_CH5DONE */
#define _DMA_IFC_CH5DONE_MASK                           0x20UL                          /**< Bit mask for DMA_CH5DONE */
#define _DMA_IFC_CH5DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_CH5DONE_DEFAULT                         (_DMA_IFC_CH5DONE_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_IFC */
#define DMA_IFC_ERR                                     (0x1UL << 31)                   /**< DMA Error Interrupt Flag Clear */
#define _DMA_IFC_ERR_SHIFT                              31                              /**< Shift value for DMA_ERR */
#define _DMA_IFC_ERR_MASK                               0x80000000UL                    /**< Bit mask for DMA_ERR */
#define _DMA_IFC_ERR_DEFAULT                            0x00000000UL                    /**< Mode DEFAULT for DMA_IFC */
#define DMA_IFC_ERR_DEFAULT                             (_DMA_IFC_ERR_DEFAULT << 31)    /**< Shifted mode DEFAULT for DMA_IFC */

/* Bit fields for DMA IEN */
#define _DMA_IEN_RESETVALUE                             0x00000000UL                    /**< Default value for DMA_IEN */
#define _DMA_IEN_MASK                                   0x8000003FUL                    /**< Mask for DMA_IEN */
#define DMA_IEN_CH0DONE                                 (0x1UL << 0)                    /**< DMA Channel 0 Complete Interrupt Enable */
#define _DMA_IEN_CH0DONE_SHIFT                          0                               /**< Shift value for DMA_CH0DONE */
#define _DMA_IEN_CH0DONE_MASK                           0x1UL                           /**< Bit mask for DMA_CH0DONE */
#define _DMA_IEN_CH0DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH0DONE_DEFAULT                         (_DMA_IEN_CH0DONE_DEFAULT << 0) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH1DONE                                 (0x1UL << 1)                    /**< DMA Channel 1 Complete Interrupt Enable */
#define _DMA_IEN_CH1DONE_SHIFT                          1                               /**< Shift value for DMA_CH1DONE */
#define _DMA_IEN_CH1DONE_MASK                           0x2UL                           /**< Bit mask for DMA_CH1DONE */
#define _DMA_IEN_CH1DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH1DONE_DEFAULT                         (_DMA_IEN_CH1DONE_DEFAULT << 1) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH2DONE                                 (0x1UL << 2)                    /**< DMA Channel 2 Complete Interrupt Enable */
#define _DMA_IEN_CH2DONE_SHIFT                          2                               /**< Shift value for DMA_CH2DONE */
#define _DMA_IEN_CH2DONE_MASK                           0x4UL                           /**< Bit mask for DMA_CH2DONE */
#define _DMA_IEN_CH2DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH2DONE_DEFAULT                         (_DMA_IEN_CH2DONE_DEFAULT << 2) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH3DONE                                 (0x1UL << 3)                    /**< DMA Channel 3 Complete Interrupt Enable */
#define _DMA_IEN_CH3DONE_SHIFT                          3                               /**< Shift value for DMA_CH3DONE */
#define _DMA_IEN_CH3DONE_MASK                           0x8UL                           /**< Bit mask for DMA_CH3DONE */
#define _DMA_IEN_CH3DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH3DONE_DEFAULT                         (_DMA_IEN_CH3DONE_DEFAULT << 3) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH4DONE                                 (0x1UL << 4)                    /**< DMA Channel 4 Complete Interrupt Enable */
#define _DMA_IEN_CH4DONE_SHIFT                          4                               /**< Shift value for DMA_CH4DONE */
#define _DMA_IEN_CH4DONE_MASK                           0x10UL                          /**< Bit mask for DMA_CH4DONE */
#define _DMA_IEN_CH4DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH4DONE_DEFAULT                         (_DMA_IEN_CH4DONE_DEFAULT << 4) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH5DONE                                 (0x1UL << 5)                    /**< DMA Channel 5 Complete Interrupt Enable */
#define _DMA_IEN_CH5DONE_SHIFT                          5                               /**< Shift value for DMA_CH5DONE */
#define _DMA_IEN_CH5DONE_MASK                           0x20UL                          /**< Bit mask for DMA_CH5DONE */
#define _DMA_IEN_CH5DONE_DEFAULT                        0x00000000UL                    /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_CH5DONE_DEFAULT                         (_DMA_IEN_CH5DONE_DEFAULT << 5) /**< Shifted mode DEFAULT for DMA_IEN */
#define DMA_IEN_ERR                                     (0x1UL << 31)                   /**< DMA Error Interrupt Flag Enable */
#define _DMA_IEN_ERR_SHIFT                              31                              /**< Shift value for DMA_ERR */
#define _DMA_IEN_ERR_MASK                               0x80000000UL                    /**< Bit mask for DMA_ERR */
#define _DMA_IEN_ERR_DEFAULT                            0x00000000UL                    /**< Mode DEFAULT for DMA_IEN */
#define DMA_IEN_ERR_DEFAULT                             (_DMA_IEN_ERR_DEFAULT << 31)    /**< Shifted mode DEFAULT for DMA_IEN */

/* Bit fields for DMA CH_CTRL */
#define _DMA_CH_CTRL_RESETVALUE                         0x00000000UL                                  /**< Default value for DMA_CH_CTRL */
#define _DMA_CH_CTRL_MASK                               0x003F000FUL                                  /**< Mask for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_SHIFT                       0                                             /**< Shift value for DMA_SIGSEL */
#define _DMA_CH_CTRL_SIGSEL_MASK                        0xFUL                                         /**< Bit mask for DMA_SIGSEL */
#define _DMA_CH_CTRL_SIGSEL_ADC0SINGLE                  0x00000000UL                                  /**< Mode ADC0SINGLE for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0RXDATAV               0x00000000UL                                  /**< Mode USART0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1RXDATAV               0x00000000UL                                  /**< Mode USART1RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV              0x00000000UL                                  /**< Mode LEUART0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C0RXDATAV                 0x00000000UL                                  /**< Mode I2C0RXDATAV for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0UFOF                  0x00000000UL                                  /**< Mode TIMER0UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1UFOF                  0x00000000UL                                  /**< Mode TIMER1UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2UFOF                  0x00000000UL                                  /**< Mode TIMER2UFOF for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_MSCWDATA                    0x00000000UL                                  /**< Mode MSCWDATA for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESDATAWR                   0x00000000UL                                  /**< Mode AESDATAWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_ADC0SCAN                    0x00000001UL                                  /**< Mode ADC0SCAN for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0TXBL                  0x00000001UL                                  /**< Mode USART0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXBL                  0x00000001UL                                  /**< Mode USART1TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0TXBL                 0x00000001UL                                  /**< Mode LEUART0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_I2C0TXBL                    0x00000001UL                                  /**< Mode I2C0TXBL for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC0                   0x00000001UL                                  /**< Mode TIMER0CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC0                   0x00000001UL                                  /**< Mode TIMER1CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC0                   0x00000001UL                                  /**< Mode TIMER2CC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESXORDATAWR                0x00000001UL                                  /**< Mode AESXORDATAWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART0TXEMPTY               0x00000002UL                                  /**< Mode USART0TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXEMPTY               0x00000002UL                                  /**< Mode USART1TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY              0x00000002UL                                  /**< Mode LEUART0TXEMPTY for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC1                   0x00000002UL                                  /**< Mode TIMER0CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC1                   0x00000002UL                                  /**< Mode TIMER1CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC1                   0x00000002UL                                  /**< Mode TIMER2CC1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESDATARD                   0x00000002UL                                  /**< Mode AESDATARD for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT          0x00000003UL                                  /**< Mode USART1RXDATAVRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER0CC2                   0x00000003UL                                  /**< Mode TIMER0CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER1CC2                   0x00000003UL                                  /**< Mode TIMER1CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_TIMER2CC2                   0x00000003UL                                  /**< Mode TIMER2CC2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_AESKEYWR                    0x00000003UL                                  /**< Mode AESKEYWR for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT             0x00000004UL                                  /**< Mode USART1TXBLRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_ADC0SINGLE                   (_DMA_CH_CTRL_SIGSEL_ADC0SINGLE << 0)         /**< Shifted mode ADC0SINGLE for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0RXDATAV                (_DMA_CH_CTRL_SIGSEL_USART0RXDATAV << 0)      /**< Shifted mode USART0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1RXDATAV                (_DMA_CH_CTRL_SIGSEL_USART1RXDATAV << 0)      /**< Shifted mode USART1RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV               (_DMA_CH_CTRL_SIGSEL_LEUART0RXDATAV << 0)     /**< Shifted mode LEUART0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C0RXDATAV                  (_DMA_CH_CTRL_SIGSEL_I2C0RXDATAV << 0)        /**< Shifted mode I2C0RXDATAV for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER0UFOF << 0)         /**< Shifted mode TIMER0UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER1UFOF << 0)         /**< Shifted mode TIMER1UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2UFOF                   (_DMA_CH_CTRL_SIGSEL_TIMER2UFOF << 0)         /**< Shifted mode TIMER2UFOF for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_MSCWDATA                     (_DMA_CH_CTRL_SIGSEL_MSCWDATA << 0)           /**< Shifted mode MSCWDATA for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESDATAWR                    (_DMA_CH_CTRL_SIGSEL_AESDATAWR << 0)          /**< Shifted mode AESDATAWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_ADC0SCAN                     (_DMA_CH_CTRL_SIGSEL_ADC0SCAN << 0)           /**< Shifted mode ADC0SCAN for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0TXBL                   (_DMA_CH_CTRL_SIGSEL_USART0TXBL << 0)         /**< Shifted mode USART0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXBL                   (_DMA_CH_CTRL_SIGSEL_USART1TXBL << 0)         /**< Shifted mode USART1TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0TXBL                  (_DMA_CH_CTRL_SIGSEL_LEUART0TXBL << 0)        /**< Shifted mode LEUART0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_I2C0TXBL                     (_DMA_CH_CTRL_SIGSEL_I2C0TXBL << 0)           /**< Shifted mode I2C0TXBL for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC0 << 0)          /**< Shifted mode TIMER0CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC0 << 0)          /**< Shifted mode TIMER1CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC0                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC0 << 0)          /**< Shifted mode TIMER2CC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESXORDATAWR                 (_DMA_CH_CTRL_SIGSEL_AESXORDATAWR << 0)       /**< Shifted mode AESXORDATAWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART0TXEMPTY                (_DMA_CH_CTRL_SIGSEL_USART0TXEMPTY << 0)      /**< Shifted mode USART0TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXEMPTY                (_DMA_CH_CTRL_SIGSEL_USART1TXEMPTY << 0)      /**< Shifted mode USART1TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY               (_DMA_CH_CTRL_SIGSEL_LEUART0TXEMPTY << 0)     /**< Shifted mode LEUART0TXEMPTY for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC1 << 0)          /**< Shifted mode TIMER0CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC1 << 0)          /**< Shifted mode TIMER1CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC1                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC1 << 0)          /**< Shifted mode TIMER2CC1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESDATARD                    (_DMA_CH_CTRL_SIGSEL_AESDATARD << 0)          /**< Shifted mode AESDATARD for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT           (_DMA_CH_CTRL_SIGSEL_USART1RXDATAVRIGHT << 0) /**< Shifted mode USART1RXDATAVRIGHT for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER0CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER0CC2 << 0)          /**< Shifted mode TIMER0CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER1CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER1CC2 << 0)          /**< Shifted mode TIMER1CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_TIMER2CC2                    (_DMA_CH_CTRL_SIGSEL_TIMER2CC2 << 0)          /**< Shifted mode TIMER2CC2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_AESKEYWR                     (_DMA_CH_CTRL_SIGSEL_AESKEYWR << 0)           /**< Shifted mode AESKEYWR for DMA_CH_CTRL */
#define DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT              (_DMA_CH_CTRL_SIGSEL_USART1TXBLRIGHT << 0)    /**< Shifted mode USART1TXBLRIGHT for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_SHIFT                    16                                            /**< Shift value for DMA_SOURCESEL */
#define _DMA_CH_CTRL_SOURCESEL_MASK                     0x3F0000UL                                    /**< Bit mask for DMA_SOURCESEL */
#define _DMA_CH_CTRL_SOURCESEL_NONE                     0x00000000UL                                  /**< Mode NONE for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_ADC0                     0x00000008UL                                  /**< Mode ADC0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_USART0                   0x0000000CUL                                  /**< Mode USART0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_USART1                   0x0000000DUL                                  /**< Mode USART1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_LEUART0                  0x00000010UL                                  /**< Mode LEUART0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_I2C0                     0x00000014UL                                  /**< Mode I2C0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER0                   0x00000018UL                                  /**< Mode TIMER0 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER1                   0x00000019UL                                  /**< Mode TIMER1 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_TIMER2                   0x0000001AUL                                  /**< Mode TIMER2 for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_MSC                      0x00000030UL                                  /**< Mode MSC for DMA_CH_CTRL */
#define _DMA_CH_CTRL_SOURCESEL_AES                      0x00000031UL                                  /**< Mode AES for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_NONE                      (_DMA_CH_CTRL_SOURCESEL_NONE << 16)           /**< Shifted mode NONE for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_ADC0                      (_DMA_CH_CTRL_SOURCESEL_ADC0 << 16)           /**< Shifted mode ADC0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_USART0                    (_DMA_CH_CTRL_SOURCESEL_USART0 << 16)         /**< Shifted mode USART0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_USART1                    (_DMA_CH_CTRL_SOURCESEL_USART1 << 16)         /**< Shifted mode USART1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_LEUART0                   (_DMA_CH_CTRL_SOURCESEL_LEUART0 << 16)        /**< Shifted mode LEUART0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_I2C0                      (_DMA_CH_CTRL_SOURCESEL_I2C0 << 16)           /**< Shifted mode I2C0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER0                    (_DMA_CH_CTRL_SOURCESEL_TIMER0 << 16)         /**< Shifted mode TIMER0 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER1                    (_DMA_CH_CTRL_SOURCESEL_TIMER1 << 16)         /**< Shifted mode TIMER1 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_TIMER2                    (_DMA_CH_CTRL_SOURCESEL_TIMER2 << 16)         /**< Shifted mode TIMER2 for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_MSC                       (_DMA_CH_CTRL_SOURCESEL_MSC << 16)            /**< Shifted mode MSC for DMA_CH_CTRL */
#define DMA_CH_CTRL_SOURCESEL_AES                       (_DMA_CH_CTRL_SOURCESEL_AES << 16)            /**< Shifted mode AES for DMA_CH_CTRL */

/** @} End of group EFM32HG_DMA */
/** @} End of group Parts */

