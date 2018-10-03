/**************************************************************************//**
 * @file efr32fg1p_cryotimer.h
 * @brief EFR32FG1P_CRYOTIMER register and bit field definitions
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
 * @defgroup EFR32FG1P_CRYOTIMER
 * @{
 * @brief EFR32FG1P_CRYOTIMER Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;      /**< Control Register  */
  __IOM uint32_t PERIODSEL; /**< Interrupt Duration  */
  __IM uint32_t  CNT;       /**< Counter Value  */
  __IOM uint32_t EM4WUEN;   /**< Wake Up Enable  */
  __IM uint32_t  IF;        /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;       /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;       /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;       /**< Interrupt Enable Register  */
} CRYOTIMER_TypeDef;        /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_CRYOTIMER_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for CRYOTIMER CTRL */
#define _CRYOTIMER_CTRL_RESETVALUE                0x00000000UL                            /**< Default value for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_MASK                      0x000000EFUL                            /**< Mask for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_EN                         (0x1UL << 0)                            /**< Enable CRYOTIMER */
#define _CRYOTIMER_CTRL_EN_SHIFT                  0                                       /**< Shift value for CRYOTIMER_EN */
#define _CRYOTIMER_CTRL_EN_MASK                   0x1UL                                   /**< Bit mask for CRYOTIMER_EN */
#define _CRYOTIMER_CTRL_EN_DEFAULT                0x00000000UL                            /**< Mode DEFAULT for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_EN_DEFAULT                 (_CRYOTIMER_CTRL_EN_DEFAULT << 0)       /**< Shifted mode DEFAULT for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_DEBUGRUN                   (0x1UL << 1)                            /**< Debug Mode Run Enable */
#define _CRYOTIMER_CTRL_DEBUGRUN_SHIFT            1                                       /**< Shift value for CRYOTIMER_DEBUGRUN */
#define _CRYOTIMER_CTRL_DEBUGRUN_MASK             0x2UL                                   /**< Bit mask for CRYOTIMER_DEBUGRUN */
#define _CRYOTIMER_CTRL_DEBUGRUN_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_DEBUGRUN_DEFAULT           (_CRYOTIMER_CTRL_DEBUGRUN_DEFAULT << 1) /**< Shifted mode DEFAULT for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_OSCSEL_SHIFT              2                                       /**< Shift value for CRYOTIMER_OSCSEL */
#define _CRYOTIMER_CTRL_OSCSEL_MASK               0xCUL                                   /**< Bit mask for CRYOTIMER_OSCSEL */
#define _CRYOTIMER_CTRL_OSCSEL_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_OSCSEL_LFRCO              0x00000000UL                            /**< Mode LFRCO for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_OSCSEL_LFXO               0x00000001UL                            /**< Mode LFXO for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_OSCSEL_ULFRCO             0x00000002UL                            /**< Mode ULFRCO for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_OSCSEL_DEFAULT             (_CRYOTIMER_CTRL_OSCSEL_DEFAULT << 2)   /**< Shifted mode DEFAULT for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_OSCSEL_LFRCO               (_CRYOTIMER_CTRL_OSCSEL_LFRCO << 2)     /**< Shifted mode LFRCO for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_OSCSEL_LFXO                (_CRYOTIMER_CTRL_OSCSEL_LFXO << 2)      /**< Shifted mode LFXO for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_OSCSEL_ULFRCO              (_CRYOTIMER_CTRL_OSCSEL_ULFRCO << 2)    /**< Shifted mode ULFRCO for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_SHIFT               5                                       /**< Shift value for CRYOTIMER_PRESC */
#define _CRYOTIMER_CTRL_PRESC_MASK                0xE0UL                                  /**< Bit mask for CRYOTIMER_PRESC */
#define _CRYOTIMER_CTRL_PRESC_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_DIV1                0x00000000UL                            /**< Mode DIV1 for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_DIV2                0x00000001UL                            /**< Mode DIV2 for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_DIV4                0x00000002UL                            /**< Mode DIV4 for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_DIV8                0x00000003UL                            /**< Mode DIV8 for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_DIV16               0x00000004UL                            /**< Mode DIV16 for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_DIV32               0x00000005UL                            /**< Mode DIV32 for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_DIV64               0x00000006UL                            /**< Mode DIV64 for CRYOTIMER_CTRL */
#define _CRYOTIMER_CTRL_PRESC_DIV128              0x00000007UL                            /**< Mode DIV128 for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DEFAULT              (_CRYOTIMER_CTRL_PRESC_DEFAULT << 5)    /**< Shifted mode DEFAULT for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DIV1                 (_CRYOTIMER_CTRL_PRESC_DIV1 << 5)       /**< Shifted mode DIV1 for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DIV2                 (_CRYOTIMER_CTRL_PRESC_DIV2 << 5)       /**< Shifted mode DIV2 for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DIV4                 (_CRYOTIMER_CTRL_PRESC_DIV4 << 5)       /**< Shifted mode DIV4 for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DIV8                 (_CRYOTIMER_CTRL_PRESC_DIV8 << 5)       /**< Shifted mode DIV8 for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DIV16                (_CRYOTIMER_CTRL_PRESC_DIV16 << 5)      /**< Shifted mode DIV16 for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DIV32                (_CRYOTIMER_CTRL_PRESC_DIV32 << 5)      /**< Shifted mode DIV32 for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DIV64                (_CRYOTIMER_CTRL_PRESC_DIV64 << 5)      /**< Shifted mode DIV64 for CRYOTIMER_CTRL */
#define CRYOTIMER_CTRL_PRESC_DIV128               (_CRYOTIMER_CTRL_PRESC_DIV128 << 5)     /**< Shifted mode DIV128 for CRYOTIMER_CTRL */

/* Bit fields for CRYOTIMER PERIODSEL */
#define _CRYOTIMER_PERIODSEL_RESETVALUE           0x00000020UL                                  /**< Default value for CRYOTIMER_PERIODSEL */
#define _CRYOTIMER_PERIODSEL_MASK                 0x0000003FUL                                  /**< Mask for CRYOTIMER_PERIODSEL */
#define _CRYOTIMER_PERIODSEL_PERIODSEL_SHIFT      0                                             /**< Shift value for CRYOTIMER_PERIODSEL */
#define _CRYOTIMER_PERIODSEL_PERIODSEL_MASK       0x3FUL                                        /**< Bit mask for CRYOTIMER_PERIODSEL */
#define _CRYOTIMER_PERIODSEL_PERIODSEL_DEFAULT    0x00000020UL                                  /**< Mode DEFAULT for CRYOTIMER_PERIODSEL */
#define CRYOTIMER_PERIODSEL_PERIODSEL_DEFAULT     (_CRYOTIMER_PERIODSEL_PERIODSEL_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYOTIMER_PERIODSEL */

/* Bit fields for CRYOTIMER CNT */
#define _CRYOTIMER_CNT_RESETVALUE                 0x00000000UL                      /**< Default value for CRYOTIMER_CNT */
#define _CRYOTIMER_CNT_MASK                       0xFFFFFFFFUL                      /**< Mask for CRYOTIMER_CNT */
#define _CRYOTIMER_CNT_CNT_SHIFT                  0                                 /**< Shift value for CRYOTIMER_CNT */
#define _CRYOTIMER_CNT_CNT_MASK                   0xFFFFFFFFUL                      /**< Bit mask for CRYOTIMER_CNT */
#define _CRYOTIMER_CNT_CNT_DEFAULT                0x00000000UL                      /**< Mode DEFAULT for CRYOTIMER_CNT */
#define CRYOTIMER_CNT_CNT_DEFAULT                 (_CRYOTIMER_CNT_CNT_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYOTIMER_CNT */

/* Bit fields for CRYOTIMER EM4WUEN */
#define _CRYOTIMER_EM4WUEN_RESETVALUE             0x00000000UL                            /**< Default value for CRYOTIMER_EM4WUEN */
#define _CRYOTIMER_EM4WUEN_MASK                   0x00000001UL                            /**< Mask for CRYOTIMER_EM4WUEN */
#define CRYOTIMER_EM4WUEN_EM4WU                   (0x1UL << 0)                            /**< EM4 Wake-up enable */
#define _CRYOTIMER_EM4WUEN_EM4WU_SHIFT            0                                       /**< Shift value for CRYOTIMER_EM4WU */
#define _CRYOTIMER_EM4WUEN_EM4WU_MASK             0x1UL                                   /**< Bit mask for CRYOTIMER_EM4WU */
#define _CRYOTIMER_EM4WUEN_EM4WU_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for CRYOTIMER_EM4WUEN */
#define CRYOTIMER_EM4WUEN_EM4WU_DEFAULT           (_CRYOTIMER_EM4WUEN_EM4WU_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYOTIMER_EM4WUEN */

/* Bit fields for CRYOTIMER IF */
#define _CRYOTIMER_IF_RESETVALUE                  0x00000000UL                        /**< Default value for CRYOTIMER_IF */
#define _CRYOTIMER_IF_MASK                        0x00000001UL                        /**< Mask for CRYOTIMER_IF */
#define CRYOTIMER_IF_PERIOD                       (0x1UL << 0)                        /**< Wakeup event/Interrupt */
#define _CRYOTIMER_IF_PERIOD_SHIFT                0                                   /**< Shift value for CRYOTIMER_PERIOD */
#define _CRYOTIMER_IF_PERIOD_MASK                 0x1UL                               /**< Bit mask for CRYOTIMER_PERIOD */
#define _CRYOTIMER_IF_PERIOD_DEFAULT              0x00000000UL                        /**< Mode DEFAULT for CRYOTIMER_IF */
#define CRYOTIMER_IF_PERIOD_DEFAULT               (_CRYOTIMER_IF_PERIOD_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYOTIMER_IF */

/* Bit fields for CRYOTIMER IFS */
#define _CRYOTIMER_IFS_RESETVALUE                 0x00000000UL                         /**< Default value for CRYOTIMER_IFS */
#define _CRYOTIMER_IFS_MASK                       0x00000001UL                         /**< Mask for CRYOTIMER_IFS */
#define CRYOTIMER_IFS_PERIOD                      (0x1UL << 0)                         /**< Set PERIOD Interrupt Flag */
#define _CRYOTIMER_IFS_PERIOD_SHIFT               0                                    /**< Shift value for CRYOTIMER_PERIOD */
#define _CRYOTIMER_IFS_PERIOD_MASK                0x1UL                                /**< Bit mask for CRYOTIMER_PERIOD */
#define _CRYOTIMER_IFS_PERIOD_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for CRYOTIMER_IFS */
#define CRYOTIMER_IFS_PERIOD_DEFAULT              (_CRYOTIMER_IFS_PERIOD_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYOTIMER_IFS */

/* Bit fields for CRYOTIMER IFC */
#define _CRYOTIMER_IFC_RESETVALUE                 0x00000000UL                         /**< Default value for CRYOTIMER_IFC */
#define _CRYOTIMER_IFC_MASK                       0x00000001UL                         /**< Mask for CRYOTIMER_IFC */
#define CRYOTIMER_IFC_PERIOD                      (0x1UL << 0)                         /**< Clear PERIOD Interrupt Flag */
#define _CRYOTIMER_IFC_PERIOD_SHIFT               0                                    /**< Shift value for CRYOTIMER_PERIOD */
#define _CRYOTIMER_IFC_PERIOD_MASK                0x1UL                                /**< Bit mask for CRYOTIMER_PERIOD */
#define _CRYOTIMER_IFC_PERIOD_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for CRYOTIMER_IFC */
#define CRYOTIMER_IFC_PERIOD_DEFAULT              (_CRYOTIMER_IFC_PERIOD_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYOTIMER_IFC */

/* Bit fields for CRYOTIMER IEN */
#define _CRYOTIMER_IEN_RESETVALUE                 0x00000000UL                         /**< Default value for CRYOTIMER_IEN */
#define _CRYOTIMER_IEN_MASK                       0x00000001UL                         /**< Mask for CRYOTIMER_IEN */
#define CRYOTIMER_IEN_PERIOD                      (0x1UL << 0)                         /**< PERIOD Interrupt Enable */
#define _CRYOTIMER_IEN_PERIOD_SHIFT               0                                    /**< Shift value for CRYOTIMER_PERIOD */
#define _CRYOTIMER_IEN_PERIOD_MASK                0x1UL                                /**< Bit mask for CRYOTIMER_PERIOD */
#define _CRYOTIMER_IEN_PERIOD_DEFAULT             0x00000000UL                         /**< Mode DEFAULT for CRYOTIMER_IEN */
#define CRYOTIMER_IEN_PERIOD_DEFAULT              (_CRYOTIMER_IEN_PERIOD_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYOTIMER_IEN */

/** @} End of group EFR32FG1P_CRYOTIMER */
/** @} End of group Parts */

