/**************************************************************************//**
 * @file efr32fg1p_letimer.h
 * @brief EFR32FG1P_LETIMER register and bit field definitions
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
 * @defgroup EFR32FG1P_LETIMER
 * @{
 * @brief EFR32FG1P_LETIMER Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;         /**< Control Register  */
  __IOM uint32_t CMD;          /**< Command Register  */
  __IM uint32_t  STATUS;       /**< Status Register  */
  __IOM uint32_t CNT;          /**< Counter Value Register  */
  __IOM uint32_t COMP0;        /**< Compare Value Register 0  */
  __IOM uint32_t COMP1;        /**< Compare Value Register 1  */
  __IOM uint32_t REP0;         /**< Repeat Counter Register 0  */
  __IOM uint32_t REP1;         /**< Repeat Counter Register 1  */
  __IM uint32_t  IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;          /**< Interrupt Enable Register  */

  uint32_t       RESERVED0[1]; /**< Reserved for future use **/
  __IM uint32_t  SYNCBUSY;     /**< Synchronization Busy Register  */

  uint32_t       RESERVED1[2]; /**< Reserved for future use **/
  __IOM uint32_t ROUTEPEN;     /**< I/O Routing Pin Enable Register  */
  __IOM uint32_t ROUTELOC0;    /**< I/O Routing Location Register  */

  uint32_t       RESERVED2[2]; /**< Reserved for future use **/
  __IOM uint32_t PRSSEL;       /**< PRS Input Select Register  */
} LETIMER_TypeDef;             /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_LETIMER_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for LETIMER CTRL */
#define _LETIMER_CTRL_RESETVALUE                0x00000000UL                           /**< Default value for LETIMER_CTRL */
#define _LETIMER_CTRL_MASK                      0x000013FFUL                           /**< Mask for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_SHIFT             0                                      /**< Shift value for LETIMER_REPMODE */
#define _LETIMER_CTRL_REPMODE_MASK              0x3UL                                  /**< Bit mask for LETIMER_REPMODE */
#define _LETIMER_CTRL_REPMODE_DEFAULT           0x00000000UL                           /**< Mode DEFAULT for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_FREE              0x00000000UL                           /**< Mode FREE for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_ONESHOT           0x00000001UL                           /**< Mode ONESHOT for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_BUFFERED          0x00000002UL                           /**< Mode BUFFERED for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_DOUBLE            0x00000003UL                           /**< Mode DOUBLE for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_DEFAULT            (_LETIMER_CTRL_REPMODE_DEFAULT << 0)   /**< Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_FREE               (_LETIMER_CTRL_REPMODE_FREE << 0)      /**< Shifted mode FREE for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_ONESHOT            (_LETIMER_CTRL_REPMODE_ONESHOT << 0)   /**< Shifted mode ONESHOT for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_BUFFERED           (_LETIMER_CTRL_REPMODE_BUFFERED << 0)  /**< Shifted mode BUFFERED for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_DOUBLE             (_LETIMER_CTRL_REPMODE_DOUBLE << 0)    /**< Shifted mode DOUBLE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_SHIFT               2                                      /**< Shift value for LETIMER_UFOA0 */
#define _LETIMER_CTRL_UFOA0_MASK                0xCUL                                  /**< Bit mask for LETIMER_UFOA0 */
#define _LETIMER_CTRL_UFOA0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_NONE                0x00000000UL                           /**< Mode NONE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_TOGGLE              0x00000001UL                           /**< Mode TOGGLE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_PULSE               0x00000002UL                           /**< Mode PULSE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_PWM                 0x00000003UL                           /**< Mode PWM for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_DEFAULT              (_LETIMER_CTRL_UFOA0_DEFAULT << 2)     /**< Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_NONE                 (_LETIMER_CTRL_UFOA0_NONE << 2)        /**< Shifted mode NONE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_TOGGLE               (_LETIMER_CTRL_UFOA0_TOGGLE << 2)      /**< Shifted mode TOGGLE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_PULSE                (_LETIMER_CTRL_UFOA0_PULSE << 2)       /**< Shifted mode PULSE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_PWM                  (_LETIMER_CTRL_UFOA0_PWM << 2)         /**< Shifted mode PWM for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_SHIFT               4                                      /**< Shift value for LETIMER_UFOA1 */
#define _LETIMER_CTRL_UFOA1_MASK                0x30UL                                 /**< Bit mask for LETIMER_UFOA1 */
#define _LETIMER_CTRL_UFOA1_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_NONE                0x00000000UL                           /**< Mode NONE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_TOGGLE              0x00000001UL                           /**< Mode TOGGLE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_PULSE               0x00000002UL                           /**< Mode PULSE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_PWM                 0x00000003UL                           /**< Mode PWM for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_DEFAULT              (_LETIMER_CTRL_UFOA1_DEFAULT << 4)     /**< Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_NONE                 (_LETIMER_CTRL_UFOA1_NONE << 4)        /**< Shifted mode NONE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_TOGGLE               (_LETIMER_CTRL_UFOA1_TOGGLE << 4)      /**< Shifted mode TOGGLE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_PULSE                (_LETIMER_CTRL_UFOA1_PULSE << 4)       /**< Shifted mode PULSE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_PWM                  (_LETIMER_CTRL_UFOA1_PWM << 4)         /**< Shifted mode PWM for LETIMER_CTRL */
#define LETIMER_CTRL_OPOL0                      (0x1UL << 6)                           /**< Output 0 Polarity */
#define _LETIMER_CTRL_OPOL0_SHIFT               6                                      /**< Shift value for LETIMER_OPOL0 */
#define _LETIMER_CTRL_OPOL0_MASK                0x40UL                                 /**< Bit mask for LETIMER_OPOL0 */
#define _LETIMER_CTRL_OPOL0_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_OPOL0_DEFAULT              (_LETIMER_CTRL_OPOL0_DEFAULT << 6)     /**< Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_OPOL1                      (0x1UL << 7)                           /**< Output 1 Polarity */
#define _LETIMER_CTRL_OPOL1_SHIFT               7                                      /**< Shift value for LETIMER_OPOL1 */
#define _LETIMER_CTRL_OPOL1_MASK                0x80UL                                 /**< Bit mask for LETIMER_OPOL1 */
#define _LETIMER_CTRL_OPOL1_DEFAULT             0x00000000UL                           /**< Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_OPOL1_DEFAULT              (_LETIMER_CTRL_OPOL1_DEFAULT << 7)     /**< Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_BUFTOP                     (0x1UL << 8)                           /**< Buffered Top */
#define _LETIMER_CTRL_BUFTOP_SHIFT              8                                      /**< Shift value for LETIMER_BUFTOP */
#define _LETIMER_CTRL_BUFTOP_MASK               0x100UL                                /**< Bit mask for LETIMER_BUFTOP */
#define _LETIMER_CTRL_BUFTOP_DEFAULT            0x00000000UL                           /**< Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_BUFTOP_DEFAULT             (_LETIMER_CTRL_BUFTOP_DEFAULT << 8)    /**< Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_COMP0TOP                   (0x1UL << 9)                           /**< Compare Value 0 Is Top Value */
#define _LETIMER_CTRL_COMP0TOP_SHIFT            9                                      /**< Shift value for LETIMER_COMP0TOP */
#define _LETIMER_CTRL_COMP0TOP_MASK             0x200UL                                /**< Bit mask for LETIMER_COMP0TOP */
#define _LETIMER_CTRL_COMP0TOP_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_COMP0TOP_DEFAULT           (_LETIMER_CTRL_COMP0TOP_DEFAULT << 9)  /**< Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_DEBUGRUN                   (0x1UL << 12)                          /**< Debug Mode Run Enable */
#define _LETIMER_CTRL_DEBUGRUN_SHIFT            12                                     /**< Shift value for LETIMER_DEBUGRUN */
#define _LETIMER_CTRL_DEBUGRUN_MASK             0x1000UL                               /**< Bit mask for LETIMER_DEBUGRUN */
#define _LETIMER_CTRL_DEBUGRUN_DEFAULT          0x00000000UL                           /**< Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_DEBUGRUN_DEFAULT           (_LETIMER_CTRL_DEBUGRUN_DEFAULT << 12) /**< Shifted mode DEFAULT for LETIMER_CTRL */

/* Bit fields for LETIMER CMD */
#define _LETIMER_CMD_RESETVALUE                 0x00000000UL                      /**< Default value for LETIMER_CMD */
#define _LETIMER_CMD_MASK                       0x0000001FUL                      /**< Mask for LETIMER_CMD */
#define LETIMER_CMD_START                       (0x1UL << 0)                      /**< Start LETIMER */
#define _LETIMER_CMD_START_SHIFT                0                                 /**< Shift value for LETIMER_START */
#define _LETIMER_CMD_START_MASK                 0x1UL                             /**< Bit mask for LETIMER_START */
#define _LETIMER_CMD_START_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_START_DEFAULT               (_LETIMER_CMD_START_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_STOP                        (0x1UL << 1)                      /**< Stop LETIMER */
#define _LETIMER_CMD_STOP_SHIFT                 1                                 /**< Shift value for LETIMER_STOP */
#define _LETIMER_CMD_STOP_MASK                  0x2UL                             /**< Bit mask for LETIMER_STOP */
#define _LETIMER_CMD_STOP_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_STOP_DEFAULT                (_LETIMER_CMD_STOP_DEFAULT << 1)  /**< Shifted mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CLEAR                       (0x1UL << 2)                      /**< Clear LETIMER */
#define _LETIMER_CMD_CLEAR_SHIFT                2                                 /**< Shift value for LETIMER_CLEAR */
#define _LETIMER_CMD_CLEAR_MASK                 0x4UL                             /**< Bit mask for LETIMER_CLEAR */
#define _LETIMER_CMD_CLEAR_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CLEAR_DEFAULT               (_LETIMER_CMD_CLEAR_DEFAULT << 2) /**< Shifted mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CTO0                        (0x1UL << 3)                      /**< Clear Toggle Output 0 */
#define _LETIMER_CMD_CTO0_SHIFT                 3                                 /**< Shift value for LETIMER_CTO0 */
#define _LETIMER_CMD_CTO0_MASK                  0x8UL                             /**< Bit mask for LETIMER_CTO0 */
#define _LETIMER_CMD_CTO0_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CTO0_DEFAULT                (_LETIMER_CMD_CTO0_DEFAULT << 3)  /**< Shifted mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CTO1                        (0x1UL << 4)                      /**< Clear Toggle Output 1 */
#define _LETIMER_CMD_CTO1_SHIFT                 4                                 /**< Shift value for LETIMER_CTO1 */
#define _LETIMER_CMD_CTO1_MASK                  0x10UL                            /**< Bit mask for LETIMER_CTO1 */
#define _LETIMER_CMD_CTO1_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CTO1_DEFAULT                (_LETIMER_CMD_CTO1_DEFAULT << 4)  /**< Shifted mode DEFAULT for LETIMER_CMD */

/* Bit fields for LETIMER STATUS */
#define _LETIMER_STATUS_RESETVALUE              0x00000000UL                           /**< Default value for LETIMER_STATUS */
#define _LETIMER_STATUS_MASK                    0x00000001UL                           /**< Mask for LETIMER_STATUS */
#define LETIMER_STATUS_RUNNING                  (0x1UL << 0)                           /**< LETIMER Running */
#define _LETIMER_STATUS_RUNNING_SHIFT           0                                      /**< Shift value for LETIMER_RUNNING */
#define _LETIMER_STATUS_RUNNING_MASK            0x1UL                                  /**< Bit mask for LETIMER_RUNNING */
#define _LETIMER_STATUS_RUNNING_DEFAULT         0x00000000UL                           /**< Mode DEFAULT for LETIMER_STATUS */
#define LETIMER_STATUS_RUNNING_DEFAULT          (_LETIMER_STATUS_RUNNING_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_STATUS */

/* Bit fields for LETIMER CNT */
#define _LETIMER_CNT_RESETVALUE                 0x00000000UL                    /**< Default value for LETIMER_CNT */
#define _LETIMER_CNT_MASK                       0x0000FFFFUL                    /**< Mask for LETIMER_CNT */
#define _LETIMER_CNT_CNT_SHIFT                  0                               /**< Shift value for LETIMER_CNT */
#define _LETIMER_CNT_CNT_MASK                   0xFFFFUL                        /**< Bit mask for LETIMER_CNT */
#define _LETIMER_CNT_CNT_DEFAULT                0x00000000UL                    /**< Mode DEFAULT for LETIMER_CNT */
#define LETIMER_CNT_CNT_DEFAULT                 (_LETIMER_CNT_CNT_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_CNT */

/* Bit fields for LETIMER COMP0 */
#define _LETIMER_COMP0_RESETVALUE               0x00000000UL                        /**< Default value for LETIMER_COMP0 */
#define _LETIMER_COMP0_MASK                     0x0000FFFFUL                        /**< Mask for LETIMER_COMP0 */
#define _LETIMER_COMP0_COMP0_SHIFT              0                                   /**< Shift value for LETIMER_COMP0 */
#define _LETIMER_COMP0_COMP0_MASK               0xFFFFUL                            /**< Bit mask for LETIMER_COMP0 */
#define _LETIMER_COMP0_COMP0_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for LETIMER_COMP0 */
#define LETIMER_COMP0_COMP0_DEFAULT             (_LETIMER_COMP0_COMP0_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_COMP0 */

/* Bit fields for LETIMER COMP1 */
#define _LETIMER_COMP1_RESETVALUE               0x00000000UL                        /**< Default value for LETIMER_COMP1 */
#define _LETIMER_COMP1_MASK                     0x0000FFFFUL                        /**< Mask for LETIMER_COMP1 */
#define _LETIMER_COMP1_COMP1_SHIFT              0                                   /**< Shift value for LETIMER_COMP1 */
#define _LETIMER_COMP1_COMP1_MASK               0xFFFFUL                            /**< Bit mask for LETIMER_COMP1 */
#define _LETIMER_COMP1_COMP1_DEFAULT            0x00000000UL                        /**< Mode DEFAULT for LETIMER_COMP1 */
#define LETIMER_COMP1_COMP1_DEFAULT             (_LETIMER_COMP1_COMP1_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_COMP1 */

/* Bit fields for LETIMER REP0 */
#define _LETIMER_REP0_RESETVALUE                0x00000000UL                      /**< Default value for LETIMER_REP0 */
#define _LETIMER_REP0_MASK                      0x000000FFUL                      /**< Mask for LETIMER_REP0 */
#define _LETIMER_REP0_REP0_SHIFT                0                                 /**< Shift value for LETIMER_REP0 */
#define _LETIMER_REP0_REP0_MASK                 0xFFUL                            /**< Bit mask for LETIMER_REP0 */
#define _LETIMER_REP0_REP0_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_REP0 */
#define LETIMER_REP0_REP0_DEFAULT               (_LETIMER_REP0_REP0_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_REP0 */

/* Bit fields for LETIMER REP1 */
#define _LETIMER_REP1_RESETVALUE                0x00000000UL                      /**< Default value for LETIMER_REP1 */
#define _LETIMER_REP1_MASK                      0x000000FFUL                      /**< Mask for LETIMER_REP1 */
#define _LETIMER_REP1_REP1_SHIFT                0                                 /**< Shift value for LETIMER_REP1 */
#define _LETIMER_REP1_REP1_MASK                 0xFFUL                            /**< Bit mask for LETIMER_REP1 */
#define _LETIMER_REP1_REP1_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_REP1 */
#define LETIMER_REP1_REP1_DEFAULT               (_LETIMER_REP1_REP1_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_REP1 */

/* Bit fields for LETIMER IF */
#define _LETIMER_IF_RESETVALUE                  0x00000000UL                     /**< Default value for LETIMER_IF */
#define _LETIMER_IF_MASK                        0x0000001FUL                     /**< Mask for LETIMER_IF */
#define LETIMER_IF_COMP0                        (0x1UL << 0)                     /**< Compare Match 0 Interrupt Flag */
#define _LETIMER_IF_COMP0_SHIFT                 0                                /**< Shift value for LETIMER_COMP0 */
#define _LETIMER_IF_COMP0_MASK                  0x1UL                            /**< Bit mask for LETIMER_COMP0 */
#define _LETIMER_IF_COMP0_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_COMP0_DEFAULT                (_LETIMER_IF_COMP0_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_COMP1                        (0x1UL << 1)                     /**< Compare Match 1 Interrupt Flag */
#define _LETIMER_IF_COMP1_SHIFT                 1                                /**< Shift value for LETIMER_COMP1 */
#define _LETIMER_IF_COMP1_MASK                  0x2UL                            /**< Bit mask for LETIMER_COMP1 */
#define _LETIMER_IF_COMP1_DEFAULT               0x00000000UL                     /**< Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_COMP1_DEFAULT                (_LETIMER_IF_COMP1_DEFAULT << 1) /**< Shifted mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_UF                           (0x1UL << 2)                     /**< Underflow Interrupt Flag */
#define _LETIMER_IF_UF_SHIFT                    2                                /**< Shift value for LETIMER_UF */
#define _LETIMER_IF_UF_MASK                     0x4UL                            /**< Bit mask for LETIMER_UF */
#define _LETIMER_IF_UF_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_UF_DEFAULT                   (_LETIMER_IF_UF_DEFAULT << 2)    /**< Shifted mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_REP0                         (0x1UL << 3)                     /**< Repeat Counter 0 Interrupt Flag */
#define _LETIMER_IF_REP0_SHIFT                  3                                /**< Shift value for LETIMER_REP0 */
#define _LETIMER_IF_REP0_MASK                   0x8UL                            /**< Bit mask for LETIMER_REP0 */
#define _LETIMER_IF_REP0_DEFAULT                0x00000000UL                     /**< Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_REP0_DEFAULT                 (_LETIMER_IF_REP0_DEFAULT << 3)  /**< Shifted mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_REP1                         (0x1UL << 4)                     /**< Repeat Counter 1 Interrupt Flag */
#define _LETIMER_IF_REP1_SHIFT                  4                                /**< Shift value for LETIMER_REP1 */
#define _LETIMER_IF_REP1_MASK                   0x10UL                           /**< Bit mask for LETIMER_REP1 */
#define _LETIMER_IF_REP1_DEFAULT                0x00000000UL                     /**< Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_REP1_DEFAULT                 (_LETIMER_IF_REP1_DEFAULT << 4)  /**< Shifted mode DEFAULT for LETIMER_IF */

/* Bit fields for LETIMER IFS */
#define _LETIMER_IFS_RESETVALUE                 0x00000000UL                      /**< Default value for LETIMER_IFS */
#define _LETIMER_IFS_MASK                       0x0000001FUL                      /**< Mask for LETIMER_IFS */
#define LETIMER_IFS_COMP0                       (0x1UL << 0)                      /**< Set COMP0 Interrupt Flag */
#define _LETIMER_IFS_COMP0_SHIFT                0                                 /**< Shift value for LETIMER_COMP0 */
#define _LETIMER_IFS_COMP0_MASK                 0x1UL                             /**< Bit mask for LETIMER_COMP0 */
#define _LETIMER_IFS_COMP0_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_COMP0_DEFAULT               (_LETIMER_IFS_COMP0_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_COMP1                       (0x1UL << 1)                      /**< Set COMP1 Interrupt Flag */
#define _LETIMER_IFS_COMP1_SHIFT                1                                 /**< Shift value for LETIMER_COMP1 */
#define _LETIMER_IFS_COMP1_MASK                 0x2UL                             /**< Bit mask for LETIMER_COMP1 */
#define _LETIMER_IFS_COMP1_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_COMP1_DEFAULT               (_LETIMER_IFS_COMP1_DEFAULT << 1) /**< Shifted mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_UF                          (0x1UL << 2)                      /**< Set UF Interrupt Flag */
#define _LETIMER_IFS_UF_SHIFT                   2                                 /**< Shift value for LETIMER_UF */
#define _LETIMER_IFS_UF_MASK                    0x4UL                             /**< Bit mask for LETIMER_UF */
#define _LETIMER_IFS_UF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_UF_DEFAULT                  (_LETIMER_IFS_UF_DEFAULT << 2)    /**< Shifted mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_REP0                        (0x1UL << 3)                      /**< Set REP0 Interrupt Flag */
#define _LETIMER_IFS_REP0_SHIFT                 3                                 /**< Shift value for LETIMER_REP0 */
#define _LETIMER_IFS_REP0_MASK                  0x8UL                             /**< Bit mask for LETIMER_REP0 */
#define _LETIMER_IFS_REP0_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_REP0_DEFAULT                (_LETIMER_IFS_REP0_DEFAULT << 3)  /**< Shifted mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_REP1                        (0x1UL << 4)                      /**< Set REP1 Interrupt Flag */
#define _LETIMER_IFS_REP1_SHIFT                 4                                 /**< Shift value for LETIMER_REP1 */
#define _LETIMER_IFS_REP1_MASK                  0x10UL                            /**< Bit mask for LETIMER_REP1 */
#define _LETIMER_IFS_REP1_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_REP1_DEFAULT                (_LETIMER_IFS_REP1_DEFAULT << 4)  /**< Shifted mode DEFAULT for LETIMER_IFS */

/* Bit fields for LETIMER IFC */
#define _LETIMER_IFC_RESETVALUE                 0x00000000UL                      /**< Default value for LETIMER_IFC */
#define _LETIMER_IFC_MASK                       0x0000001FUL                      /**< Mask for LETIMER_IFC */
#define LETIMER_IFC_COMP0                       (0x1UL << 0)                      /**< Clear COMP0 Interrupt Flag */
#define _LETIMER_IFC_COMP0_SHIFT                0                                 /**< Shift value for LETIMER_COMP0 */
#define _LETIMER_IFC_COMP0_MASK                 0x1UL                             /**< Bit mask for LETIMER_COMP0 */
#define _LETIMER_IFC_COMP0_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_COMP0_DEFAULT               (_LETIMER_IFC_COMP0_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_COMP1                       (0x1UL << 1)                      /**< Clear COMP1 Interrupt Flag */
#define _LETIMER_IFC_COMP1_SHIFT                1                                 /**< Shift value for LETIMER_COMP1 */
#define _LETIMER_IFC_COMP1_MASK                 0x2UL                             /**< Bit mask for LETIMER_COMP1 */
#define _LETIMER_IFC_COMP1_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_COMP1_DEFAULT               (_LETIMER_IFC_COMP1_DEFAULT << 1) /**< Shifted mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_UF                          (0x1UL << 2)                      /**< Clear UF Interrupt Flag */
#define _LETIMER_IFC_UF_SHIFT                   2                                 /**< Shift value for LETIMER_UF */
#define _LETIMER_IFC_UF_MASK                    0x4UL                             /**< Bit mask for LETIMER_UF */
#define _LETIMER_IFC_UF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_UF_DEFAULT                  (_LETIMER_IFC_UF_DEFAULT << 2)    /**< Shifted mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_REP0                        (0x1UL << 3)                      /**< Clear REP0 Interrupt Flag */
#define _LETIMER_IFC_REP0_SHIFT                 3                                 /**< Shift value for LETIMER_REP0 */
#define _LETIMER_IFC_REP0_MASK                  0x8UL                             /**< Bit mask for LETIMER_REP0 */
#define _LETIMER_IFC_REP0_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_REP0_DEFAULT                (_LETIMER_IFC_REP0_DEFAULT << 3)  /**< Shifted mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_REP1                        (0x1UL << 4)                      /**< Clear REP1 Interrupt Flag */
#define _LETIMER_IFC_REP1_SHIFT                 4                                 /**< Shift value for LETIMER_REP1 */
#define _LETIMER_IFC_REP1_MASK                  0x10UL                            /**< Bit mask for LETIMER_REP1 */
#define _LETIMER_IFC_REP1_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_REP1_DEFAULT                (_LETIMER_IFC_REP1_DEFAULT << 4)  /**< Shifted mode DEFAULT for LETIMER_IFC */

/* Bit fields for LETIMER IEN */
#define _LETIMER_IEN_RESETVALUE                 0x00000000UL                      /**< Default value for LETIMER_IEN */
#define _LETIMER_IEN_MASK                       0x0000001FUL                      /**< Mask for LETIMER_IEN */
#define LETIMER_IEN_COMP0                       (0x1UL << 0)                      /**< COMP0 Interrupt Enable */
#define _LETIMER_IEN_COMP0_SHIFT                0                                 /**< Shift value for LETIMER_COMP0 */
#define _LETIMER_IEN_COMP0_MASK                 0x1UL                             /**< Bit mask for LETIMER_COMP0 */
#define _LETIMER_IEN_COMP0_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_COMP0_DEFAULT               (_LETIMER_IEN_COMP0_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_COMP1                       (0x1UL << 1)                      /**< COMP1 Interrupt Enable */
#define _LETIMER_IEN_COMP1_SHIFT                1                                 /**< Shift value for LETIMER_COMP1 */
#define _LETIMER_IEN_COMP1_MASK                 0x2UL                             /**< Bit mask for LETIMER_COMP1 */
#define _LETIMER_IEN_COMP1_DEFAULT              0x00000000UL                      /**< Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_COMP1_DEFAULT               (_LETIMER_IEN_COMP1_DEFAULT << 1) /**< Shifted mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_UF                          (0x1UL << 2)                      /**< UF Interrupt Enable */
#define _LETIMER_IEN_UF_SHIFT                   2                                 /**< Shift value for LETIMER_UF */
#define _LETIMER_IEN_UF_MASK                    0x4UL                             /**< Bit mask for LETIMER_UF */
#define _LETIMER_IEN_UF_DEFAULT                 0x00000000UL                      /**< Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_UF_DEFAULT                  (_LETIMER_IEN_UF_DEFAULT << 2)    /**< Shifted mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_REP0                        (0x1UL << 3)                      /**< REP0 Interrupt Enable */
#define _LETIMER_IEN_REP0_SHIFT                 3                                 /**< Shift value for LETIMER_REP0 */
#define _LETIMER_IEN_REP0_MASK                  0x8UL                             /**< Bit mask for LETIMER_REP0 */
#define _LETIMER_IEN_REP0_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_REP0_DEFAULT                (_LETIMER_IEN_REP0_DEFAULT << 3)  /**< Shifted mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_REP1                        (0x1UL << 4)                      /**< REP1 Interrupt Enable */
#define _LETIMER_IEN_REP1_SHIFT                 4                                 /**< Shift value for LETIMER_REP1 */
#define _LETIMER_IEN_REP1_MASK                  0x10UL                            /**< Bit mask for LETIMER_REP1 */
#define _LETIMER_IEN_REP1_DEFAULT               0x00000000UL                      /**< Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_REP1_DEFAULT                (_LETIMER_IEN_REP1_DEFAULT << 4)  /**< Shifted mode DEFAULT for LETIMER_IEN */

/* Bit fields for LETIMER SYNCBUSY */
#define _LETIMER_SYNCBUSY_RESETVALUE            0x00000000UL                         /**< Default value for LETIMER_SYNCBUSY */
#define _LETIMER_SYNCBUSY_MASK                  0x00000002UL                         /**< Mask for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_CMD                    (0x1UL << 1)                         /**< CMD Register Busy */
#define _LETIMER_SYNCBUSY_CMD_SHIFT             1                                    /**< Shift value for LETIMER_CMD */
#define _LETIMER_SYNCBUSY_CMD_MASK              0x2UL                                /**< Bit mask for LETIMER_CMD */
#define _LETIMER_SYNCBUSY_CMD_DEFAULT           0x00000000UL                         /**< Mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_CMD_DEFAULT            (_LETIMER_SYNCBUSY_CMD_DEFAULT << 1) /**< Shifted mode DEFAULT for LETIMER_SYNCBUSY */

/* Bit fields for LETIMER ROUTEPEN */
#define _LETIMER_ROUTEPEN_RESETVALUE            0x00000000UL                             /**< Default value for LETIMER_ROUTEPEN */
#define _LETIMER_ROUTEPEN_MASK                  0x00000003UL                             /**< Mask for LETIMER_ROUTEPEN */
#define LETIMER_ROUTEPEN_OUT0PEN                (0x1UL << 0)                             /**< Output 0 Pin Enable */
#define _LETIMER_ROUTEPEN_OUT0PEN_SHIFT         0                                        /**< Shift value for LETIMER_OUT0PEN */
#define _LETIMER_ROUTEPEN_OUT0PEN_MASK          0x1UL                                    /**< Bit mask for LETIMER_OUT0PEN */
#define _LETIMER_ROUTEPEN_OUT0PEN_DEFAULT       0x00000000UL                             /**< Mode DEFAULT for LETIMER_ROUTEPEN */
#define LETIMER_ROUTEPEN_OUT0PEN_DEFAULT        (_LETIMER_ROUTEPEN_OUT0PEN_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_ROUTEPEN */
#define LETIMER_ROUTEPEN_OUT1PEN                (0x1UL << 1)                             /**< Output 1 Pin Enable */
#define _LETIMER_ROUTEPEN_OUT1PEN_SHIFT         1                                        /**< Shift value for LETIMER_OUT1PEN */
#define _LETIMER_ROUTEPEN_OUT1PEN_MASK          0x2UL                                    /**< Bit mask for LETIMER_OUT1PEN */
#define _LETIMER_ROUTEPEN_OUT1PEN_DEFAULT       0x00000000UL                             /**< Mode DEFAULT for LETIMER_ROUTEPEN */
#define LETIMER_ROUTEPEN_OUT1PEN_DEFAULT        (_LETIMER_ROUTEPEN_OUT1PEN_DEFAULT << 1) /**< Shifted mode DEFAULT for LETIMER_ROUTEPEN */

/* Bit fields for LETIMER ROUTELOC0 */
#define _LETIMER_ROUTELOC0_RESETVALUE           0x00000000UL                              /**< Default value for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_MASK                 0x00001F1FUL                              /**< Mask for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_SHIFT        0                                         /**< Shift value for LETIMER_OUT0LOC */
#define _LETIMER_ROUTELOC0_OUT0LOC_MASK         0x1FUL                                    /**< Bit mask for LETIMER_OUT0LOC */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC0         0x00000000UL                              /**< Mode LOC0 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_DEFAULT      0x00000000UL                              /**< Mode DEFAULT for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC1         0x00000001UL                              /**< Mode LOC1 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC2         0x00000002UL                              /**< Mode LOC2 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC3         0x00000003UL                              /**< Mode LOC3 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC4         0x00000004UL                              /**< Mode LOC4 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC5         0x00000005UL                              /**< Mode LOC5 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC6         0x00000006UL                              /**< Mode LOC6 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC7         0x00000007UL                              /**< Mode LOC7 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC8         0x00000008UL                              /**< Mode LOC8 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC9         0x00000009UL                              /**< Mode LOC9 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC10        0x0000000AUL                              /**< Mode LOC10 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC11        0x0000000BUL                              /**< Mode LOC11 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC12        0x0000000CUL                              /**< Mode LOC12 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC13        0x0000000DUL                              /**< Mode LOC13 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC14        0x0000000EUL                              /**< Mode LOC14 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC15        0x0000000FUL                              /**< Mode LOC15 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC16        0x00000010UL                              /**< Mode LOC16 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC17        0x00000011UL                              /**< Mode LOC17 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC18        0x00000012UL                              /**< Mode LOC18 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC19        0x00000013UL                              /**< Mode LOC19 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC20        0x00000014UL                              /**< Mode LOC20 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC21        0x00000015UL                              /**< Mode LOC21 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC22        0x00000016UL                              /**< Mode LOC22 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC23        0x00000017UL                              /**< Mode LOC23 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC24        0x00000018UL                              /**< Mode LOC24 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC25        0x00000019UL                              /**< Mode LOC25 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC26        0x0000001AUL                              /**< Mode LOC26 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC27        0x0000001BUL                              /**< Mode LOC27 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC28        0x0000001CUL                              /**< Mode LOC28 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC29        0x0000001DUL                              /**< Mode LOC29 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC30        0x0000001EUL                              /**< Mode LOC30 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT0LOC_LOC31        0x0000001FUL                              /**< Mode LOC31 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC0          (_LETIMER_ROUTELOC0_OUT0LOC_LOC0 << 0)    /**< Shifted mode LOC0 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_DEFAULT       (_LETIMER_ROUTELOC0_OUT0LOC_DEFAULT << 0) /**< Shifted mode DEFAULT for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC1          (_LETIMER_ROUTELOC0_OUT0LOC_LOC1 << 0)    /**< Shifted mode LOC1 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC2          (_LETIMER_ROUTELOC0_OUT0LOC_LOC2 << 0)    /**< Shifted mode LOC2 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC3          (_LETIMER_ROUTELOC0_OUT0LOC_LOC3 << 0)    /**< Shifted mode LOC3 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC4          (_LETIMER_ROUTELOC0_OUT0LOC_LOC4 << 0)    /**< Shifted mode LOC4 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC5          (_LETIMER_ROUTELOC0_OUT0LOC_LOC5 << 0)    /**< Shifted mode LOC5 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC6          (_LETIMER_ROUTELOC0_OUT0LOC_LOC6 << 0)    /**< Shifted mode LOC6 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC7          (_LETIMER_ROUTELOC0_OUT0LOC_LOC7 << 0)    /**< Shifted mode LOC7 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC8          (_LETIMER_ROUTELOC0_OUT0LOC_LOC8 << 0)    /**< Shifted mode LOC8 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC9          (_LETIMER_ROUTELOC0_OUT0LOC_LOC9 << 0)    /**< Shifted mode LOC9 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC10         (_LETIMER_ROUTELOC0_OUT0LOC_LOC10 << 0)   /**< Shifted mode LOC10 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC11         (_LETIMER_ROUTELOC0_OUT0LOC_LOC11 << 0)   /**< Shifted mode LOC11 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC12         (_LETIMER_ROUTELOC0_OUT0LOC_LOC12 << 0)   /**< Shifted mode LOC12 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC13         (_LETIMER_ROUTELOC0_OUT0LOC_LOC13 << 0)   /**< Shifted mode LOC13 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC14         (_LETIMER_ROUTELOC0_OUT0LOC_LOC14 << 0)   /**< Shifted mode LOC14 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC15         (_LETIMER_ROUTELOC0_OUT0LOC_LOC15 << 0)   /**< Shifted mode LOC15 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC16         (_LETIMER_ROUTELOC0_OUT0LOC_LOC16 << 0)   /**< Shifted mode LOC16 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC17         (_LETIMER_ROUTELOC0_OUT0LOC_LOC17 << 0)   /**< Shifted mode LOC17 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC18         (_LETIMER_ROUTELOC0_OUT0LOC_LOC18 << 0)   /**< Shifted mode LOC18 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC19         (_LETIMER_ROUTELOC0_OUT0LOC_LOC19 << 0)   /**< Shifted mode LOC19 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC20         (_LETIMER_ROUTELOC0_OUT0LOC_LOC20 << 0)   /**< Shifted mode LOC20 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC21         (_LETIMER_ROUTELOC0_OUT0LOC_LOC21 << 0)   /**< Shifted mode LOC21 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC22         (_LETIMER_ROUTELOC0_OUT0LOC_LOC22 << 0)   /**< Shifted mode LOC22 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC23         (_LETIMER_ROUTELOC0_OUT0LOC_LOC23 << 0)   /**< Shifted mode LOC23 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC24         (_LETIMER_ROUTELOC0_OUT0LOC_LOC24 << 0)   /**< Shifted mode LOC24 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC25         (_LETIMER_ROUTELOC0_OUT0LOC_LOC25 << 0)   /**< Shifted mode LOC25 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC26         (_LETIMER_ROUTELOC0_OUT0LOC_LOC26 << 0)   /**< Shifted mode LOC26 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC27         (_LETIMER_ROUTELOC0_OUT0LOC_LOC27 << 0)   /**< Shifted mode LOC27 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC28         (_LETIMER_ROUTELOC0_OUT0LOC_LOC28 << 0)   /**< Shifted mode LOC28 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC29         (_LETIMER_ROUTELOC0_OUT0LOC_LOC29 << 0)   /**< Shifted mode LOC29 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC30         (_LETIMER_ROUTELOC0_OUT0LOC_LOC30 << 0)   /**< Shifted mode LOC30 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT0LOC_LOC31         (_LETIMER_ROUTELOC0_OUT0LOC_LOC31 << 0)   /**< Shifted mode LOC31 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_SHIFT        8                                         /**< Shift value for LETIMER_OUT1LOC */
#define _LETIMER_ROUTELOC0_OUT1LOC_MASK         0x1F00UL                                  /**< Bit mask for LETIMER_OUT1LOC */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC0         0x00000000UL                              /**< Mode LOC0 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_DEFAULT      0x00000000UL                              /**< Mode DEFAULT for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC1         0x00000001UL                              /**< Mode LOC1 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC2         0x00000002UL                              /**< Mode LOC2 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC3         0x00000003UL                              /**< Mode LOC3 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC4         0x00000004UL                              /**< Mode LOC4 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC5         0x00000005UL                              /**< Mode LOC5 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC6         0x00000006UL                              /**< Mode LOC6 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC7         0x00000007UL                              /**< Mode LOC7 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC8         0x00000008UL                              /**< Mode LOC8 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC9         0x00000009UL                              /**< Mode LOC9 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC10        0x0000000AUL                              /**< Mode LOC10 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC11        0x0000000BUL                              /**< Mode LOC11 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC12        0x0000000CUL                              /**< Mode LOC12 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC13        0x0000000DUL                              /**< Mode LOC13 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC14        0x0000000EUL                              /**< Mode LOC14 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC15        0x0000000FUL                              /**< Mode LOC15 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC16        0x00000010UL                              /**< Mode LOC16 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC17        0x00000011UL                              /**< Mode LOC17 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC18        0x00000012UL                              /**< Mode LOC18 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC19        0x00000013UL                              /**< Mode LOC19 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC20        0x00000014UL                              /**< Mode LOC20 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC21        0x00000015UL                              /**< Mode LOC21 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC22        0x00000016UL                              /**< Mode LOC22 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC23        0x00000017UL                              /**< Mode LOC23 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC24        0x00000018UL                              /**< Mode LOC24 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC25        0x00000019UL                              /**< Mode LOC25 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC26        0x0000001AUL                              /**< Mode LOC26 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC27        0x0000001BUL                              /**< Mode LOC27 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC28        0x0000001CUL                              /**< Mode LOC28 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC29        0x0000001DUL                              /**< Mode LOC29 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC30        0x0000001EUL                              /**< Mode LOC30 for LETIMER_ROUTELOC0 */
#define _LETIMER_ROUTELOC0_OUT1LOC_LOC31        0x0000001FUL                              /**< Mode LOC31 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC0          (_LETIMER_ROUTELOC0_OUT1LOC_LOC0 << 8)    /**< Shifted mode LOC0 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_DEFAULT       (_LETIMER_ROUTELOC0_OUT1LOC_DEFAULT << 8) /**< Shifted mode DEFAULT for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC1          (_LETIMER_ROUTELOC0_OUT1LOC_LOC1 << 8)    /**< Shifted mode LOC1 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC2          (_LETIMER_ROUTELOC0_OUT1LOC_LOC2 << 8)    /**< Shifted mode LOC2 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC3          (_LETIMER_ROUTELOC0_OUT1LOC_LOC3 << 8)    /**< Shifted mode LOC3 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC4          (_LETIMER_ROUTELOC0_OUT1LOC_LOC4 << 8)    /**< Shifted mode LOC4 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC5          (_LETIMER_ROUTELOC0_OUT1LOC_LOC5 << 8)    /**< Shifted mode LOC5 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC6          (_LETIMER_ROUTELOC0_OUT1LOC_LOC6 << 8)    /**< Shifted mode LOC6 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC7          (_LETIMER_ROUTELOC0_OUT1LOC_LOC7 << 8)    /**< Shifted mode LOC7 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC8          (_LETIMER_ROUTELOC0_OUT1LOC_LOC8 << 8)    /**< Shifted mode LOC8 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC9          (_LETIMER_ROUTELOC0_OUT1LOC_LOC9 << 8)    /**< Shifted mode LOC9 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC10         (_LETIMER_ROUTELOC0_OUT1LOC_LOC10 << 8)   /**< Shifted mode LOC10 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC11         (_LETIMER_ROUTELOC0_OUT1LOC_LOC11 << 8)   /**< Shifted mode LOC11 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC12         (_LETIMER_ROUTELOC0_OUT1LOC_LOC12 << 8)   /**< Shifted mode LOC12 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC13         (_LETIMER_ROUTELOC0_OUT1LOC_LOC13 << 8)   /**< Shifted mode LOC13 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC14         (_LETIMER_ROUTELOC0_OUT1LOC_LOC14 << 8)   /**< Shifted mode LOC14 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC15         (_LETIMER_ROUTELOC0_OUT1LOC_LOC15 << 8)   /**< Shifted mode LOC15 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC16         (_LETIMER_ROUTELOC0_OUT1LOC_LOC16 << 8)   /**< Shifted mode LOC16 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC17         (_LETIMER_ROUTELOC0_OUT1LOC_LOC17 << 8)   /**< Shifted mode LOC17 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC18         (_LETIMER_ROUTELOC0_OUT1LOC_LOC18 << 8)   /**< Shifted mode LOC18 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC19         (_LETIMER_ROUTELOC0_OUT1LOC_LOC19 << 8)   /**< Shifted mode LOC19 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC20         (_LETIMER_ROUTELOC0_OUT1LOC_LOC20 << 8)   /**< Shifted mode LOC20 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC21         (_LETIMER_ROUTELOC0_OUT1LOC_LOC21 << 8)   /**< Shifted mode LOC21 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC22         (_LETIMER_ROUTELOC0_OUT1LOC_LOC22 << 8)   /**< Shifted mode LOC22 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC23         (_LETIMER_ROUTELOC0_OUT1LOC_LOC23 << 8)   /**< Shifted mode LOC23 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC24         (_LETIMER_ROUTELOC0_OUT1LOC_LOC24 << 8)   /**< Shifted mode LOC24 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC25         (_LETIMER_ROUTELOC0_OUT1LOC_LOC25 << 8)   /**< Shifted mode LOC25 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC26         (_LETIMER_ROUTELOC0_OUT1LOC_LOC26 << 8)   /**< Shifted mode LOC26 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC27         (_LETIMER_ROUTELOC0_OUT1LOC_LOC27 << 8)   /**< Shifted mode LOC27 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC28         (_LETIMER_ROUTELOC0_OUT1LOC_LOC28 << 8)   /**< Shifted mode LOC28 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC29         (_LETIMER_ROUTELOC0_OUT1LOC_LOC29 << 8)   /**< Shifted mode LOC29 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC30         (_LETIMER_ROUTELOC0_OUT1LOC_LOC30 << 8)   /**< Shifted mode LOC30 for LETIMER_ROUTELOC0 */
#define LETIMER_ROUTELOC0_OUT1LOC_LOC31         (_LETIMER_ROUTELOC0_OUT1LOC_LOC31 << 8)   /**< Shifted mode LOC31 for LETIMER_ROUTELOC0 */

/* Bit fields for LETIMER PRSSEL */
#define _LETIMER_PRSSEL_RESETVALUE              0x00000000UL                                 /**< Default value for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_MASK                    0x0CCCF3CFUL                                 /**< Mask for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_SHIFT       0                                            /**< Shift value for LETIMER_PRSSTARTSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_MASK        0xFUL                                        /**< Bit mask for LETIMER_PRSSTARTSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_DEFAULT     0x00000000UL                                 /**< Mode DEFAULT for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH0      0x00000000UL                                 /**< Mode PRSCH0 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH1      0x00000001UL                                 /**< Mode PRSCH1 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH2      0x00000002UL                                 /**< Mode PRSCH2 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH3      0x00000003UL                                 /**< Mode PRSCH3 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH4      0x00000004UL                                 /**< Mode PRSCH4 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH5      0x00000005UL                                 /**< Mode PRSCH5 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH6      0x00000006UL                                 /**< Mode PRSCH6 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH7      0x00000007UL                                 /**< Mode PRSCH7 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH8      0x00000008UL                                 /**< Mode PRSCH8 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH9      0x00000009UL                                 /**< Mode PRSCH9 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH10     0x0000000AUL                                 /**< Mode PRSCH10 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTSEL_PRSCH11     0x0000000BUL                                 /**< Mode PRSCH11 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_DEFAULT      (_LETIMER_PRSSEL_PRSSTARTSEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH0       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH0 << 0)    /**< Shifted mode PRSCH0 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH1       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH1 << 0)    /**< Shifted mode PRSCH1 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH2       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH2 << 0)    /**< Shifted mode PRSCH2 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH3       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH3 << 0)    /**< Shifted mode PRSCH3 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH4       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH4 << 0)    /**< Shifted mode PRSCH4 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH5       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH5 << 0)    /**< Shifted mode PRSCH5 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH6       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH6 << 0)    /**< Shifted mode PRSCH6 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH7       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH7 << 0)    /**< Shifted mode PRSCH7 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH8       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH8 << 0)    /**< Shifted mode PRSCH8 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH9       (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH9 << 0)    /**< Shifted mode PRSCH9 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH10      (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH10 << 0)   /**< Shifted mode PRSCH10 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTSEL_PRSCH11      (_LETIMER_PRSSEL_PRSSTARTSEL_PRSCH11 << 0)   /**< Shifted mode PRSCH11 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_SHIFT        6                                            /**< Shift value for LETIMER_PRSSTOPSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_MASK         0x3C0UL                                      /**< Bit mask for LETIMER_PRSSTOPSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_DEFAULT      0x00000000UL                                 /**< Mode DEFAULT for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH0       0x00000000UL                                 /**< Mode PRSCH0 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH1       0x00000001UL                                 /**< Mode PRSCH1 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH2       0x00000002UL                                 /**< Mode PRSCH2 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH3       0x00000003UL                                 /**< Mode PRSCH3 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH4       0x00000004UL                                 /**< Mode PRSCH4 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH5       0x00000005UL                                 /**< Mode PRSCH5 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH6       0x00000006UL                                 /**< Mode PRSCH6 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH7       0x00000007UL                                 /**< Mode PRSCH7 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH8       0x00000008UL                                 /**< Mode PRSCH8 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH9       0x00000009UL                                 /**< Mode PRSCH9 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH10      0x0000000AUL                                 /**< Mode PRSCH10 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPSEL_PRSCH11      0x0000000BUL                                 /**< Mode PRSCH11 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_DEFAULT       (_LETIMER_PRSSEL_PRSSTOPSEL_DEFAULT << 6)    /**< Shifted mode DEFAULT for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH0        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH0 << 6)     /**< Shifted mode PRSCH0 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH1        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH1 << 6)     /**< Shifted mode PRSCH1 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH2        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH2 << 6)     /**< Shifted mode PRSCH2 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH3        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH3 << 6)     /**< Shifted mode PRSCH3 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH4        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH4 << 6)     /**< Shifted mode PRSCH4 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH5        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH5 << 6)     /**< Shifted mode PRSCH5 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH6        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH6 << 6)     /**< Shifted mode PRSCH6 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH7        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH7 << 6)     /**< Shifted mode PRSCH7 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH8        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH8 << 6)     /**< Shifted mode PRSCH8 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH9        (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH9 << 6)     /**< Shifted mode PRSCH9 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH10       (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH10 << 6)    /**< Shifted mode PRSCH10 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPSEL_PRSCH11       (_LETIMER_PRSSEL_PRSSTOPSEL_PRSCH11 << 6)    /**< Shifted mode PRSCH11 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_SHIFT       12                                           /**< Shift value for LETIMER_PRSCLEARSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_MASK        0xF000UL                                     /**< Bit mask for LETIMER_PRSCLEARSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_DEFAULT     0x00000000UL                                 /**< Mode DEFAULT for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH0      0x00000000UL                                 /**< Mode PRSCH0 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH1      0x00000001UL                                 /**< Mode PRSCH1 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH2      0x00000002UL                                 /**< Mode PRSCH2 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH3      0x00000003UL                                 /**< Mode PRSCH3 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH4      0x00000004UL                                 /**< Mode PRSCH4 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH5      0x00000005UL                                 /**< Mode PRSCH5 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH6      0x00000006UL                                 /**< Mode PRSCH6 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH7      0x00000007UL                                 /**< Mode PRSCH7 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH8      0x00000008UL                                 /**< Mode PRSCH8 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH9      0x00000009UL                                 /**< Mode PRSCH9 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH10     0x0000000AUL                                 /**< Mode PRSCH10 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARSEL_PRSCH11     0x0000000BUL                                 /**< Mode PRSCH11 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_DEFAULT      (_LETIMER_PRSSEL_PRSCLEARSEL_DEFAULT << 12)  /**< Shifted mode DEFAULT for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH0       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH0 << 12)   /**< Shifted mode PRSCH0 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH1       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH1 << 12)   /**< Shifted mode PRSCH1 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH2       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH2 << 12)   /**< Shifted mode PRSCH2 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH3       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH3 << 12)   /**< Shifted mode PRSCH3 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH4       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH4 << 12)   /**< Shifted mode PRSCH4 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH5       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH5 << 12)   /**< Shifted mode PRSCH5 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH6       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH6 << 12)   /**< Shifted mode PRSCH6 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH7       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH7 << 12)   /**< Shifted mode PRSCH7 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH8       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH8 << 12)   /**< Shifted mode PRSCH8 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH9       (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH9 << 12)   /**< Shifted mode PRSCH9 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH10      (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH10 << 12)  /**< Shifted mode PRSCH10 for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARSEL_PRSCH11      (_LETIMER_PRSSEL_PRSCLEARSEL_PRSCH11 << 12)  /**< Shifted mode PRSCH11 for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTMODE_SHIFT      18                                           /**< Shift value for LETIMER_PRSSTARTMODE */
#define _LETIMER_PRSSEL_PRSSTARTMODE_MASK       0xC0000UL                                    /**< Bit mask for LETIMER_PRSSTARTMODE */
#define _LETIMER_PRSSEL_PRSSTARTMODE_DEFAULT    0x00000000UL                                 /**< Mode DEFAULT for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTMODE_NONE       0x00000000UL                                 /**< Mode NONE for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTMODE_RISING     0x00000001UL                                 /**< Mode RISING for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTMODE_FALLING    0x00000002UL                                 /**< Mode FALLING for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTARTMODE_BOTH       0x00000003UL                                 /**< Mode BOTH for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTMODE_DEFAULT     (_LETIMER_PRSSEL_PRSSTARTMODE_DEFAULT << 18) /**< Shifted mode DEFAULT for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTMODE_NONE        (_LETIMER_PRSSEL_PRSSTARTMODE_NONE << 18)    /**< Shifted mode NONE for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTMODE_RISING      (_LETIMER_PRSSEL_PRSSTARTMODE_RISING << 18)  /**< Shifted mode RISING for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTMODE_FALLING     (_LETIMER_PRSSEL_PRSSTARTMODE_FALLING << 18) /**< Shifted mode FALLING for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTARTMODE_BOTH        (_LETIMER_PRSSEL_PRSSTARTMODE_BOTH << 18)    /**< Shifted mode BOTH for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPMODE_SHIFT       22                                           /**< Shift value for LETIMER_PRSSTOPMODE */
#define _LETIMER_PRSSEL_PRSSTOPMODE_MASK        0xC00000UL                                   /**< Bit mask for LETIMER_PRSSTOPMODE */
#define _LETIMER_PRSSEL_PRSSTOPMODE_DEFAULT     0x00000000UL                                 /**< Mode DEFAULT for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPMODE_NONE        0x00000000UL                                 /**< Mode NONE for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPMODE_RISING      0x00000001UL                                 /**< Mode RISING for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPMODE_FALLING     0x00000002UL                                 /**< Mode FALLING for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSSTOPMODE_BOTH        0x00000003UL                                 /**< Mode BOTH for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPMODE_DEFAULT      (_LETIMER_PRSSEL_PRSSTOPMODE_DEFAULT << 22)  /**< Shifted mode DEFAULT for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPMODE_NONE         (_LETIMER_PRSSEL_PRSSTOPMODE_NONE << 22)     /**< Shifted mode NONE for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPMODE_RISING       (_LETIMER_PRSSEL_PRSSTOPMODE_RISING << 22)   /**< Shifted mode RISING for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPMODE_FALLING      (_LETIMER_PRSSEL_PRSSTOPMODE_FALLING << 22)  /**< Shifted mode FALLING for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSSTOPMODE_BOTH         (_LETIMER_PRSSEL_PRSSTOPMODE_BOTH << 22)     /**< Shifted mode BOTH for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARMODE_SHIFT      26                                           /**< Shift value for LETIMER_PRSCLEARMODE */
#define _LETIMER_PRSSEL_PRSCLEARMODE_MASK       0xC000000UL                                  /**< Bit mask for LETIMER_PRSCLEARMODE */
#define _LETIMER_PRSSEL_PRSCLEARMODE_DEFAULT    0x00000000UL                                 /**< Mode DEFAULT for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARMODE_NONE       0x00000000UL                                 /**< Mode NONE for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARMODE_RISING     0x00000001UL                                 /**< Mode RISING for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARMODE_FALLING    0x00000002UL                                 /**< Mode FALLING for LETIMER_PRSSEL */
#define _LETIMER_PRSSEL_PRSCLEARMODE_BOTH       0x00000003UL                                 /**< Mode BOTH for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARMODE_DEFAULT     (_LETIMER_PRSSEL_PRSCLEARMODE_DEFAULT << 26) /**< Shifted mode DEFAULT for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARMODE_NONE        (_LETIMER_PRSSEL_PRSCLEARMODE_NONE << 26)    /**< Shifted mode NONE for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARMODE_RISING      (_LETIMER_PRSSEL_PRSCLEARMODE_RISING << 26)  /**< Shifted mode RISING for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARMODE_FALLING     (_LETIMER_PRSSEL_PRSCLEARMODE_FALLING << 26) /**< Shifted mode FALLING for LETIMER_PRSSEL */
#define LETIMER_PRSSEL_PRSCLEARMODE_BOTH        (_LETIMER_PRSSEL_PRSCLEARMODE_BOTH << 26)    /**< Shifted mode BOTH for LETIMER_PRSSEL */

/** @} End of group EFR32FG1P_LETIMER */
/** @} End of group Parts */

