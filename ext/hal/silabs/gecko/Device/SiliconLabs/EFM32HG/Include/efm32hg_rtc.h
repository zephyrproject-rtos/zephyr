/**************************************************************************//**
 * @file efm32hg_rtc.h
 * @brief EFM32HG_RTC register and bit field definitions
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
 * @defgroup EFM32HG_RTC
 * @{
 * @brief EFM32HG_RTC Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;     /**< Control Register  */
  __IOM uint32_t CNT;      /**< Counter Value Register  */
  __IOM uint32_t COMP0;    /**< Compare Value Register 0  */
  __IOM uint32_t COMP1;    /**< Compare Value Register 1  */
  __IM uint32_t  IF;       /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;      /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;      /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;      /**< Interrupt Enable Register  */

  __IOM uint32_t FREEZE;   /**< Freeze Register  */
  __IM uint32_t  SYNCBUSY; /**< Synchronization Busy Register  */
} RTC_TypeDef;             /** @} */

/**************************************************************************//**
 * @defgroup EFM32HG_RTC_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for RTC CTRL */
#define _RTC_CTRL_RESETVALUE             0x00000000UL                      /**< Default value for RTC_CTRL */
#define _RTC_CTRL_MASK                   0x00000007UL                      /**< Mask for RTC_CTRL */
#define RTC_CTRL_EN                      (0x1UL << 0)                      /**< RTC Enable */
#define _RTC_CTRL_EN_SHIFT               0                                 /**< Shift value for RTC_EN */
#define _RTC_CTRL_EN_MASK                0x1UL                             /**< Bit mask for RTC_EN */
#define _RTC_CTRL_EN_DEFAULT             0x00000000UL                      /**< Mode DEFAULT for RTC_CTRL */
#define RTC_CTRL_EN_DEFAULT              (_RTC_CTRL_EN_DEFAULT << 0)       /**< Shifted mode DEFAULT for RTC_CTRL */
#define RTC_CTRL_DEBUGRUN                (0x1UL << 1)                      /**< Debug Mode Run Enable */
#define _RTC_CTRL_DEBUGRUN_SHIFT         1                                 /**< Shift value for RTC_DEBUGRUN */
#define _RTC_CTRL_DEBUGRUN_MASK          0x2UL                             /**< Bit mask for RTC_DEBUGRUN */
#define _RTC_CTRL_DEBUGRUN_DEFAULT       0x00000000UL                      /**< Mode DEFAULT for RTC_CTRL */
#define RTC_CTRL_DEBUGRUN_DEFAULT        (_RTC_CTRL_DEBUGRUN_DEFAULT << 1) /**< Shifted mode DEFAULT for RTC_CTRL */
#define RTC_CTRL_COMP0TOP                (0x1UL << 2)                      /**< Compare Channel 0 is Top Value */
#define _RTC_CTRL_COMP0TOP_SHIFT         2                                 /**< Shift value for RTC_COMP0TOP */
#define _RTC_CTRL_COMP0TOP_MASK          0x4UL                             /**< Bit mask for RTC_COMP0TOP */
#define _RTC_CTRL_COMP0TOP_DEFAULT       0x00000000UL                      /**< Mode DEFAULT for RTC_CTRL */
#define _RTC_CTRL_COMP0TOP_DISABLE       0x00000000UL                      /**< Mode DISABLE for RTC_CTRL */
#define _RTC_CTRL_COMP0TOP_ENABLE        0x00000001UL                      /**< Mode ENABLE for RTC_CTRL */
#define RTC_CTRL_COMP0TOP_DEFAULT        (_RTC_CTRL_COMP0TOP_DEFAULT << 2) /**< Shifted mode DEFAULT for RTC_CTRL */
#define RTC_CTRL_COMP0TOP_DISABLE        (_RTC_CTRL_COMP0TOP_DISABLE << 2) /**< Shifted mode DISABLE for RTC_CTRL */
#define RTC_CTRL_COMP0TOP_ENABLE         (_RTC_CTRL_COMP0TOP_ENABLE << 2)  /**< Shifted mode ENABLE for RTC_CTRL */

/* Bit fields for RTC CNT */
#define _RTC_CNT_RESETVALUE              0x00000000UL                /**< Default value for RTC_CNT */
#define _RTC_CNT_MASK                    0x00FFFFFFUL                /**< Mask for RTC_CNT */
#define _RTC_CNT_CNT_SHIFT               0                           /**< Shift value for RTC_CNT */
#define _RTC_CNT_CNT_MASK                0xFFFFFFUL                  /**< Bit mask for RTC_CNT */
#define _RTC_CNT_CNT_DEFAULT             0x00000000UL                /**< Mode DEFAULT for RTC_CNT */
#define RTC_CNT_CNT_DEFAULT              (_RTC_CNT_CNT_DEFAULT << 0) /**< Shifted mode DEFAULT for RTC_CNT */

/* Bit fields for RTC COMP0 */
#define _RTC_COMP0_RESETVALUE            0x00000000UL                    /**< Default value for RTC_COMP0 */
#define _RTC_COMP0_MASK                  0x00FFFFFFUL                    /**< Mask for RTC_COMP0 */
#define _RTC_COMP0_COMP0_SHIFT           0                               /**< Shift value for RTC_COMP0 */
#define _RTC_COMP0_COMP0_MASK            0xFFFFFFUL                      /**< Bit mask for RTC_COMP0 */
#define _RTC_COMP0_COMP0_DEFAULT         0x00000000UL                    /**< Mode DEFAULT for RTC_COMP0 */
#define RTC_COMP0_COMP0_DEFAULT          (_RTC_COMP0_COMP0_DEFAULT << 0) /**< Shifted mode DEFAULT for RTC_COMP0 */

/* Bit fields for RTC COMP1 */
#define _RTC_COMP1_RESETVALUE            0x00000000UL                    /**< Default value for RTC_COMP1 */
#define _RTC_COMP1_MASK                  0x00FFFFFFUL                    /**< Mask for RTC_COMP1 */
#define _RTC_COMP1_COMP1_SHIFT           0                               /**< Shift value for RTC_COMP1 */
#define _RTC_COMP1_COMP1_MASK            0xFFFFFFUL                      /**< Bit mask for RTC_COMP1 */
#define _RTC_COMP1_COMP1_DEFAULT         0x00000000UL                    /**< Mode DEFAULT for RTC_COMP1 */
#define RTC_COMP1_COMP1_DEFAULT          (_RTC_COMP1_COMP1_DEFAULT << 0) /**< Shifted mode DEFAULT for RTC_COMP1 */

/* Bit fields for RTC IF */
#define _RTC_IF_RESETVALUE               0x00000000UL                 /**< Default value for RTC_IF */
#define _RTC_IF_MASK                     0x00000007UL                 /**< Mask for RTC_IF */
#define RTC_IF_OF                        (0x1UL << 0)                 /**< Overflow Interrupt Flag */
#define _RTC_IF_OF_SHIFT                 0                            /**< Shift value for RTC_OF */
#define _RTC_IF_OF_MASK                  0x1UL                        /**< Bit mask for RTC_OF */
#define _RTC_IF_OF_DEFAULT               0x00000000UL                 /**< Mode DEFAULT for RTC_IF */
#define RTC_IF_OF_DEFAULT                (_RTC_IF_OF_DEFAULT << 0)    /**< Shifted mode DEFAULT for RTC_IF */
#define RTC_IF_COMP0                     (0x1UL << 1)                 /**< Compare Match 0 Interrupt Flag */
#define _RTC_IF_COMP0_SHIFT              1                            /**< Shift value for RTC_COMP0 */
#define _RTC_IF_COMP0_MASK               0x2UL                        /**< Bit mask for RTC_COMP0 */
#define _RTC_IF_COMP0_DEFAULT            0x00000000UL                 /**< Mode DEFAULT for RTC_IF */
#define RTC_IF_COMP0_DEFAULT             (_RTC_IF_COMP0_DEFAULT << 1) /**< Shifted mode DEFAULT for RTC_IF */
#define RTC_IF_COMP1                     (0x1UL << 2)                 /**< Compare Match 1 Interrupt Flag */
#define _RTC_IF_COMP1_SHIFT              2                            /**< Shift value for RTC_COMP1 */
#define _RTC_IF_COMP1_MASK               0x4UL                        /**< Bit mask for RTC_COMP1 */
#define _RTC_IF_COMP1_DEFAULT            0x00000000UL                 /**< Mode DEFAULT for RTC_IF */
#define RTC_IF_COMP1_DEFAULT             (_RTC_IF_COMP1_DEFAULT << 2) /**< Shifted mode DEFAULT for RTC_IF */

/* Bit fields for RTC IFS */
#define _RTC_IFS_RESETVALUE              0x00000000UL                  /**< Default value for RTC_IFS */
#define _RTC_IFS_MASK                    0x00000007UL                  /**< Mask for RTC_IFS */
#define RTC_IFS_OF                       (0x1UL << 0)                  /**< Set Overflow Interrupt Flag */
#define _RTC_IFS_OF_SHIFT                0                             /**< Shift value for RTC_OF */
#define _RTC_IFS_OF_MASK                 0x1UL                         /**< Bit mask for RTC_OF */
#define _RTC_IFS_OF_DEFAULT              0x00000000UL                  /**< Mode DEFAULT for RTC_IFS */
#define RTC_IFS_OF_DEFAULT               (_RTC_IFS_OF_DEFAULT << 0)    /**< Shifted mode DEFAULT for RTC_IFS */
#define RTC_IFS_COMP0                    (0x1UL << 1)                  /**< Set Compare match 0 Interrupt Flag */
#define _RTC_IFS_COMP0_SHIFT             1                             /**< Shift value for RTC_COMP0 */
#define _RTC_IFS_COMP0_MASK              0x2UL                         /**< Bit mask for RTC_COMP0 */
#define _RTC_IFS_COMP0_DEFAULT           0x00000000UL                  /**< Mode DEFAULT for RTC_IFS */
#define RTC_IFS_COMP0_DEFAULT            (_RTC_IFS_COMP0_DEFAULT << 1) /**< Shifted mode DEFAULT for RTC_IFS */
#define RTC_IFS_COMP1                    (0x1UL << 2)                  /**< Set Compare match 1 Interrupt Flag */
#define _RTC_IFS_COMP1_SHIFT             2                             /**< Shift value for RTC_COMP1 */
#define _RTC_IFS_COMP1_MASK              0x4UL                         /**< Bit mask for RTC_COMP1 */
#define _RTC_IFS_COMP1_DEFAULT           0x00000000UL                  /**< Mode DEFAULT for RTC_IFS */
#define RTC_IFS_COMP1_DEFAULT            (_RTC_IFS_COMP1_DEFAULT << 2) /**< Shifted mode DEFAULT for RTC_IFS */

/* Bit fields for RTC IFC */
#define _RTC_IFC_RESETVALUE              0x00000000UL                  /**< Default value for RTC_IFC */
#define _RTC_IFC_MASK                    0x00000007UL                  /**< Mask for RTC_IFC */
#define RTC_IFC_OF                       (0x1UL << 0)                  /**< Clear Overflow Interrupt Flag */
#define _RTC_IFC_OF_SHIFT                0                             /**< Shift value for RTC_OF */
#define _RTC_IFC_OF_MASK                 0x1UL                         /**< Bit mask for RTC_OF */
#define _RTC_IFC_OF_DEFAULT              0x00000000UL                  /**< Mode DEFAULT for RTC_IFC */
#define RTC_IFC_OF_DEFAULT               (_RTC_IFC_OF_DEFAULT << 0)    /**< Shifted mode DEFAULT for RTC_IFC */
#define RTC_IFC_COMP0                    (0x1UL << 1)                  /**< Clear Compare match 0 Interrupt Flag */
#define _RTC_IFC_COMP0_SHIFT             1                             /**< Shift value for RTC_COMP0 */
#define _RTC_IFC_COMP0_MASK              0x2UL                         /**< Bit mask for RTC_COMP0 */
#define _RTC_IFC_COMP0_DEFAULT           0x00000000UL                  /**< Mode DEFAULT for RTC_IFC */
#define RTC_IFC_COMP0_DEFAULT            (_RTC_IFC_COMP0_DEFAULT << 1) /**< Shifted mode DEFAULT for RTC_IFC */
#define RTC_IFC_COMP1                    (0x1UL << 2)                  /**< Clear Compare match 1 Interrupt Flag */
#define _RTC_IFC_COMP1_SHIFT             2                             /**< Shift value for RTC_COMP1 */
#define _RTC_IFC_COMP1_MASK              0x4UL                         /**< Bit mask for RTC_COMP1 */
#define _RTC_IFC_COMP1_DEFAULT           0x00000000UL                  /**< Mode DEFAULT for RTC_IFC */
#define RTC_IFC_COMP1_DEFAULT            (_RTC_IFC_COMP1_DEFAULT << 2) /**< Shifted mode DEFAULT for RTC_IFC */

/* Bit fields for RTC IEN */
#define _RTC_IEN_RESETVALUE              0x00000000UL                  /**< Default value for RTC_IEN */
#define _RTC_IEN_MASK                    0x00000007UL                  /**< Mask for RTC_IEN */
#define RTC_IEN_OF                       (0x1UL << 0)                  /**< Overflow Interrupt Enable */
#define _RTC_IEN_OF_SHIFT                0                             /**< Shift value for RTC_OF */
#define _RTC_IEN_OF_MASK                 0x1UL                         /**< Bit mask for RTC_OF */
#define _RTC_IEN_OF_DEFAULT              0x00000000UL                  /**< Mode DEFAULT for RTC_IEN */
#define RTC_IEN_OF_DEFAULT               (_RTC_IEN_OF_DEFAULT << 0)    /**< Shifted mode DEFAULT for RTC_IEN */
#define RTC_IEN_COMP0                    (0x1UL << 1)                  /**< Compare Match 0 Interrupt Enable */
#define _RTC_IEN_COMP0_SHIFT             1                             /**< Shift value for RTC_COMP0 */
#define _RTC_IEN_COMP0_MASK              0x2UL                         /**< Bit mask for RTC_COMP0 */
#define _RTC_IEN_COMP0_DEFAULT           0x00000000UL                  /**< Mode DEFAULT for RTC_IEN */
#define RTC_IEN_COMP0_DEFAULT            (_RTC_IEN_COMP0_DEFAULT << 1) /**< Shifted mode DEFAULT for RTC_IEN */
#define RTC_IEN_COMP1                    (0x1UL << 2)                  /**< Compare Match 1 Interrupt Enable */
#define _RTC_IEN_COMP1_SHIFT             2                             /**< Shift value for RTC_COMP1 */
#define _RTC_IEN_COMP1_MASK              0x4UL                         /**< Bit mask for RTC_COMP1 */
#define _RTC_IEN_COMP1_DEFAULT           0x00000000UL                  /**< Mode DEFAULT for RTC_IEN */
#define RTC_IEN_COMP1_DEFAULT            (_RTC_IEN_COMP1_DEFAULT << 2) /**< Shifted mode DEFAULT for RTC_IEN */

/* Bit fields for RTC FREEZE */
#define _RTC_FREEZE_RESETVALUE           0x00000000UL                         /**< Default value for RTC_FREEZE */
#define _RTC_FREEZE_MASK                 0x00000001UL                         /**< Mask for RTC_FREEZE */
#define RTC_FREEZE_REGFREEZE             (0x1UL << 0)                         /**< Register Update Freeze */
#define _RTC_FREEZE_REGFREEZE_SHIFT      0                                    /**< Shift value for RTC_REGFREEZE */
#define _RTC_FREEZE_REGFREEZE_MASK       0x1UL                                /**< Bit mask for RTC_REGFREEZE */
#define _RTC_FREEZE_REGFREEZE_DEFAULT    0x00000000UL                         /**< Mode DEFAULT for RTC_FREEZE */
#define _RTC_FREEZE_REGFREEZE_UPDATE     0x00000000UL                         /**< Mode UPDATE for RTC_FREEZE */
#define _RTC_FREEZE_REGFREEZE_FREEZE     0x00000001UL                         /**< Mode FREEZE for RTC_FREEZE */
#define RTC_FREEZE_REGFREEZE_DEFAULT     (_RTC_FREEZE_REGFREEZE_DEFAULT << 0) /**< Shifted mode DEFAULT for RTC_FREEZE */
#define RTC_FREEZE_REGFREEZE_UPDATE      (_RTC_FREEZE_REGFREEZE_UPDATE << 0)  /**< Shifted mode UPDATE for RTC_FREEZE */
#define RTC_FREEZE_REGFREEZE_FREEZE      (_RTC_FREEZE_REGFREEZE_FREEZE << 0)  /**< Shifted mode FREEZE for RTC_FREEZE */

/* Bit fields for RTC SYNCBUSY */
#define _RTC_SYNCBUSY_RESETVALUE         0x00000000UL                       /**< Default value for RTC_SYNCBUSY */
#define _RTC_SYNCBUSY_MASK               0x00000007UL                       /**< Mask for RTC_SYNCBUSY */
#define RTC_SYNCBUSY_CTRL                (0x1UL << 0)                       /**< CTRL Register Busy */
#define _RTC_SYNCBUSY_CTRL_SHIFT         0                                  /**< Shift value for RTC_CTRL */
#define _RTC_SYNCBUSY_CTRL_MASK          0x1UL                              /**< Bit mask for RTC_CTRL */
#define _RTC_SYNCBUSY_CTRL_DEFAULT       0x00000000UL                       /**< Mode DEFAULT for RTC_SYNCBUSY */
#define RTC_SYNCBUSY_CTRL_DEFAULT        (_RTC_SYNCBUSY_CTRL_DEFAULT << 0)  /**< Shifted mode DEFAULT for RTC_SYNCBUSY */
#define RTC_SYNCBUSY_COMP0               (0x1UL << 1)                       /**< COMP0 Register Busy */
#define _RTC_SYNCBUSY_COMP0_SHIFT        1                                  /**< Shift value for RTC_COMP0 */
#define _RTC_SYNCBUSY_COMP0_MASK         0x2UL                              /**< Bit mask for RTC_COMP0 */
#define _RTC_SYNCBUSY_COMP0_DEFAULT      0x00000000UL                       /**< Mode DEFAULT for RTC_SYNCBUSY */
#define RTC_SYNCBUSY_COMP0_DEFAULT       (_RTC_SYNCBUSY_COMP0_DEFAULT << 1) /**< Shifted mode DEFAULT for RTC_SYNCBUSY */
#define RTC_SYNCBUSY_COMP1               (0x1UL << 2)                       /**< COMP1 Register Busy */
#define _RTC_SYNCBUSY_COMP1_SHIFT        2                                  /**< Shift value for RTC_COMP1 */
#define _RTC_SYNCBUSY_COMP1_MASK         0x4UL                              /**< Bit mask for RTC_COMP1 */
#define _RTC_SYNCBUSY_COMP1_DEFAULT      0x00000000UL                       /**< Mode DEFAULT for RTC_SYNCBUSY */
#define RTC_SYNCBUSY_COMP1_DEFAULT       (_RTC_SYNCBUSY_COMP1_DEFAULT << 2) /**< Shifted mode DEFAULT for RTC_SYNCBUSY */

/** @} End of group EFM32HG_RTC */
/** @} End of group Parts */

