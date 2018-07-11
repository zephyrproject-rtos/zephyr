/**************************************************************************//**
 * @file efr32fg1p_acmp.h
 * @brief EFR32FG1P_ACMP register and bit field definitions
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
 * @defgroup EFR32FG1P_ACMP
 * @{
 * @brief EFR32FG1P_ACMP Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;          /**< Control Register  */
  __IOM uint32_t INPUTSEL;      /**< Input Selection Register  */
  __IM uint32_t  STATUS;        /**< Status Register  */
  __IM uint32_t  IF;            /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;           /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;           /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;           /**< Interrupt Enable Register  */
  uint32_t       RESERVED0[1];  /**< Reserved for future use **/
  __IM uint32_t  APORTREQ;      /**< APORT Request Status Register  */
  __IM uint32_t  APORTCONFLICT; /**< APORT Conflict Status Register  */
  __IOM uint32_t HYSTERESIS0;   /**< Hysteresis 0 Register  */
  __IOM uint32_t HYSTERESIS1;   /**< Hysteresis 1 Register  */

  uint32_t       RESERVED1[4];  /**< Reserved for future use **/
  __IOM uint32_t ROUTEPEN;      /**< I/O Routing Pine Enable Register  */
  __IOM uint32_t ROUTELOC0;     /**< I/O Routing Location Register  */
} ACMP_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_ACMP_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for ACMP CTRL */
#define _ACMP_CTRL_RESETVALUE                          0x07000000UL                               /**< Default value for ACMP_CTRL */
#define _ACMP_CTRL_MASK                                0xBF3CF70DUL                               /**< Mask for ACMP_CTRL */
#define ACMP_CTRL_EN                                   (0x1UL << 0)                               /**< Analog Comparator Enable */
#define _ACMP_CTRL_EN_SHIFT                            0                                          /**< Shift value for ACMP_EN */
#define _ACMP_CTRL_EN_MASK                             0x1UL                                      /**< Bit mask for ACMP_EN */
#define _ACMP_CTRL_EN_DEFAULT                          0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_EN_DEFAULT                           (_ACMP_CTRL_EN_DEFAULT << 0)               /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_INACTVAL                             (0x1UL << 2)                               /**< Inactive Value */
#define _ACMP_CTRL_INACTVAL_SHIFT                      2                                          /**< Shift value for ACMP_INACTVAL */
#define _ACMP_CTRL_INACTVAL_MASK                       0x4UL                                      /**< Bit mask for ACMP_INACTVAL */
#define _ACMP_CTRL_INACTVAL_DEFAULT                    0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_INACTVAL_LOW                        0x00000000UL                               /**< Mode LOW for ACMP_CTRL */
#define _ACMP_CTRL_INACTVAL_HIGH                       0x00000001UL                               /**< Mode HIGH for ACMP_CTRL */
#define ACMP_CTRL_INACTVAL_DEFAULT                     (_ACMP_CTRL_INACTVAL_DEFAULT << 2)         /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_INACTVAL_LOW                         (_ACMP_CTRL_INACTVAL_LOW << 2)             /**< Shifted mode LOW for ACMP_CTRL */
#define ACMP_CTRL_INACTVAL_HIGH                        (_ACMP_CTRL_INACTVAL_HIGH << 2)            /**< Shifted mode HIGH for ACMP_CTRL */
#define ACMP_CTRL_GPIOINV                              (0x1UL << 3)                               /**< Comparator GPIO Output Invert */
#define _ACMP_CTRL_GPIOINV_SHIFT                       3                                          /**< Shift value for ACMP_GPIOINV */
#define _ACMP_CTRL_GPIOINV_MASK                        0x8UL                                      /**< Bit mask for ACMP_GPIOINV */
#define _ACMP_CTRL_GPIOINV_DEFAULT                     0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_GPIOINV_NOTINV                      0x00000000UL                               /**< Mode NOTINV for ACMP_CTRL */
#define _ACMP_CTRL_GPIOINV_INV                         0x00000001UL                               /**< Mode INV for ACMP_CTRL */
#define ACMP_CTRL_GPIOINV_DEFAULT                      (_ACMP_CTRL_GPIOINV_DEFAULT << 3)          /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_GPIOINV_NOTINV                       (_ACMP_CTRL_GPIOINV_NOTINV << 3)           /**< Shifted mode NOTINV for ACMP_CTRL */
#define ACMP_CTRL_GPIOINV_INV                          (_ACMP_CTRL_GPIOINV_INV << 3)              /**< Shifted mode INV for ACMP_CTRL */
#define ACMP_CTRL_APORTXMASTERDIS                      (0x1UL << 8)                               /**< APORT Bus X Master Disable */
#define _ACMP_CTRL_APORTXMASTERDIS_SHIFT               8                                          /**< Shift value for ACMP_APORTXMASTERDIS */
#define _ACMP_CTRL_APORTXMASTERDIS_MASK                0x100UL                                    /**< Bit mask for ACMP_APORTXMASTERDIS */
#define _ACMP_CTRL_APORTXMASTERDIS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_APORTXMASTERDIS_DEFAULT              (_ACMP_CTRL_APORTXMASTERDIS_DEFAULT << 8)  /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_APORTYMASTERDIS                      (0x1UL << 9)                               /**< APORT Bus Y Master Disable */
#define _ACMP_CTRL_APORTYMASTERDIS_SHIFT               9                                          /**< Shift value for ACMP_APORTYMASTERDIS */
#define _ACMP_CTRL_APORTYMASTERDIS_MASK                0x200UL                                    /**< Bit mask for ACMP_APORTYMASTERDIS */
#define _ACMP_CTRL_APORTYMASTERDIS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_APORTYMASTERDIS_DEFAULT              (_ACMP_CTRL_APORTYMASTERDIS_DEFAULT << 9)  /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_APORTVMASTERDIS                      (0x1UL << 10)                              /**< APORT Bus Master Disable for Bus selected by VASEL */
#define _ACMP_CTRL_APORTVMASTERDIS_SHIFT               10                                         /**< Shift value for ACMP_APORTVMASTERDIS */
#define _ACMP_CTRL_APORTVMASTERDIS_MASK                0x400UL                                    /**< Bit mask for ACMP_APORTVMASTERDIS */
#define _ACMP_CTRL_APORTVMASTERDIS_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_APORTVMASTERDIS_DEFAULT              (_ACMP_CTRL_APORTVMASTERDIS_DEFAULT << 10) /**< Shifted mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_PWRSEL_SHIFT                        12                                         /**< Shift value for ACMP_PWRSEL */
#define _ACMP_CTRL_PWRSEL_MASK                         0x7000UL                                   /**< Bit mask for ACMP_PWRSEL */
#define _ACMP_CTRL_PWRSEL_DEFAULT                      0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_PWRSEL_AVDD                         0x00000000UL                               /**< Mode AVDD for ACMP_CTRL */
#define _ACMP_CTRL_PWRSEL_VREGVDD                      0x00000001UL                               /**< Mode VREGVDD for ACMP_CTRL */
#define _ACMP_CTRL_PWRSEL_IOVDD0                       0x00000002UL                               /**< Mode IOVDD0 for ACMP_CTRL */
#define _ACMP_CTRL_PWRSEL_IOVDD1                       0x00000004UL                               /**< Mode IOVDD1 for ACMP_CTRL */
#define ACMP_CTRL_PWRSEL_DEFAULT                       (_ACMP_CTRL_PWRSEL_DEFAULT << 12)          /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_PWRSEL_AVDD                          (_ACMP_CTRL_PWRSEL_AVDD << 12)             /**< Shifted mode AVDD for ACMP_CTRL */
#define ACMP_CTRL_PWRSEL_VREGVDD                       (_ACMP_CTRL_PWRSEL_VREGVDD << 12)          /**< Shifted mode VREGVDD for ACMP_CTRL */
#define ACMP_CTRL_PWRSEL_IOVDD0                        (_ACMP_CTRL_PWRSEL_IOVDD0 << 12)           /**< Shifted mode IOVDD0 for ACMP_CTRL */
#define ACMP_CTRL_PWRSEL_IOVDD1                        (_ACMP_CTRL_PWRSEL_IOVDD1 << 12)           /**< Shifted mode IOVDD1 for ACMP_CTRL */
#define ACMP_CTRL_ACCURACY                             (0x1UL << 15)                              /**< ACMP accuracy mode */
#define _ACMP_CTRL_ACCURACY_SHIFT                      15                                         /**< Shift value for ACMP_ACCURACY */
#define _ACMP_CTRL_ACCURACY_MASK                       0x8000UL                                   /**< Bit mask for ACMP_ACCURACY */
#define _ACMP_CTRL_ACCURACY_DEFAULT                    0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_ACCURACY_LOW                        0x00000000UL                               /**< Mode LOW for ACMP_CTRL */
#define _ACMP_CTRL_ACCURACY_HIGH                       0x00000001UL                               /**< Mode HIGH for ACMP_CTRL */
#define ACMP_CTRL_ACCURACY_DEFAULT                     (_ACMP_CTRL_ACCURACY_DEFAULT << 15)        /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_ACCURACY_LOW                         (_ACMP_CTRL_ACCURACY_LOW << 15)            /**< Shifted mode LOW for ACMP_CTRL */
#define ACMP_CTRL_ACCURACY_HIGH                        (_ACMP_CTRL_ACCURACY_HIGH << 15)           /**< Shifted mode HIGH for ACMP_CTRL */
#define _ACMP_CTRL_INPUTRANGE_SHIFT                    18                                         /**< Shift value for ACMP_INPUTRANGE */
#define _ACMP_CTRL_INPUTRANGE_MASK                     0xC0000UL                                  /**< Bit mask for ACMP_INPUTRANGE */
#define _ACMP_CTRL_INPUTRANGE_DEFAULT                  0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_INPUTRANGE_FULL                     0x00000000UL                               /**< Mode FULL for ACMP_CTRL */
#define _ACMP_CTRL_INPUTRANGE_GTVDDDIV2                0x00000001UL                               /**< Mode GTVDDDIV2 for ACMP_CTRL */
#define _ACMP_CTRL_INPUTRANGE_LTVDDDIV2                0x00000002UL                               /**< Mode LTVDDDIV2 for ACMP_CTRL */
#define ACMP_CTRL_INPUTRANGE_DEFAULT                   (_ACMP_CTRL_INPUTRANGE_DEFAULT << 18)      /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_INPUTRANGE_FULL                      (_ACMP_CTRL_INPUTRANGE_FULL << 18)         /**< Shifted mode FULL for ACMP_CTRL */
#define ACMP_CTRL_INPUTRANGE_GTVDDDIV2                 (_ACMP_CTRL_INPUTRANGE_GTVDDDIV2 << 18)    /**< Shifted mode GTVDDDIV2 for ACMP_CTRL */
#define ACMP_CTRL_INPUTRANGE_LTVDDDIV2                 (_ACMP_CTRL_INPUTRANGE_LTVDDDIV2 << 18)    /**< Shifted mode LTVDDDIV2 for ACMP_CTRL */
#define ACMP_CTRL_IRISE                                (0x1UL << 20)                              /**< Rising Edge Interrupt Sense */
#define _ACMP_CTRL_IRISE_SHIFT                         20                                         /**< Shift value for ACMP_IRISE */
#define _ACMP_CTRL_IRISE_MASK                          0x100000UL                                 /**< Bit mask for ACMP_IRISE */
#define _ACMP_CTRL_IRISE_DEFAULT                       0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_IRISE_DISABLED                      0x00000000UL                               /**< Mode DISABLED for ACMP_CTRL */
#define _ACMP_CTRL_IRISE_ENABLED                       0x00000001UL                               /**< Mode ENABLED for ACMP_CTRL */
#define ACMP_CTRL_IRISE_DEFAULT                        (_ACMP_CTRL_IRISE_DEFAULT << 20)           /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_IRISE_DISABLED                       (_ACMP_CTRL_IRISE_DISABLED << 20)          /**< Shifted mode DISABLED for ACMP_CTRL */
#define ACMP_CTRL_IRISE_ENABLED                        (_ACMP_CTRL_IRISE_ENABLED << 20)           /**< Shifted mode ENABLED for ACMP_CTRL */
#define ACMP_CTRL_IFALL                                (0x1UL << 21)                              /**< Falling Edge Interrupt Sense */
#define _ACMP_CTRL_IFALL_SHIFT                         21                                         /**< Shift value for ACMP_IFALL */
#define _ACMP_CTRL_IFALL_MASK                          0x200000UL                                 /**< Bit mask for ACMP_IFALL */
#define _ACMP_CTRL_IFALL_DEFAULT                       0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define _ACMP_CTRL_IFALL_DISABLED                      0x00000000UL                               /**< Mode DISABLED for ACMP_CTRL */
#define _ACMP_CTRL_IFALL_ENABLED                       0x00000001UL                               /**< Mode ENABLED for ACMP_CTRL */
#define ACMP_CTRL_IFALL_DEFAULT                        (_ACMP_CTRL_IFALL_DEFAULT << 21)           /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_IFALL_DISABLED                       (_ACMP_CTRL_IFALL_DISABLED << 21)          /**< Shifted mode DISABLED for ACMP_CTRL */
#define ACMP_CTRL_IFALL_ENABLED                        (_ACMP_CTRL_IFALL_ENABLED << 21)           /**< Shifted mode ENABLED for ACMP_CTRL */
#define _ACMP_CTRL_BIASPROG_SHIFT                      24                                         /**< Shift value for ACMP_BIASPROG */
#define _ACMP_CTRL_BIASPROG_MASK                       0x3F000000UL                               /**< Bit mask for ACMP_BIASPROG */
#define _ACMP_CTRL_BIASPROG_DEFAULT                    0x00000007UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_BIASPROG_DEFAULT                     (_ACMP_CTRL_BIASPROG_DEFAULT << 24)        /**< Shifted mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_FULLBIAS                             (0x1UL << 31)                              /**< Full Bias Current */
#define _ACMP_CTRL_FULLBIAS_SHIFT                      31                                         /**< Shift value for ACMP_FULLBIAS */
#define _ACMP_CTRL_FULLBIAS_MASK                       0x80000000UL                               /**< Bit mask for ACMP_FULLBIAS */
#define _ACMP_CTRL_FULLBIAS_DEFAULT                    0x00000000UL                               /**< Mode DEFAULT for ACMP_CTRL */
#define ACMP_CTRL_FULLBIAS_DEFAULT                     (_ACMP_CTRL_FULLBIAS_DEFAULT << 31)        /**< Shifted mode DEFAULT for ACMP_CTRL */

/* Bit fields for ACMP INPUTSEL */
#define _ACMP_INPUTSEL_RESETVALUE                      0x00000000UL                             /**< Default value for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_MASK                            0x757FFFFFUL                             /**< Mask for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_SHIFT                    0                                        /**< Shift value for ACMP_POSSEL */
#define _ACMP_INPUTSEL_POSSEL_MASK                     0xFFUL                                   /**< Bit mask for ACMP_POSSEL */
#define _ACMP_INPUTSEL_POSSEL_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH0               0x00000000UL                             /**< Mode APORT0XCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH1               0x00000001UL                             /**< Mode APORT0XCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH2               0x00000002UL                             /**< Mode APORT0XCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH3               0x00000003UL                             /**< Mode APORT0XCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH4               0x00000004UL                             /**< Mode APORT0XCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH5               0x00000005UL                             /**< Mode APORT0XCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH6               0x00000006UL                             /**< Mode APORT0XCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH7               0x00000007UL                             /**< Mode APORT0XCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH8               0x00000008UL                             /**< Mode APORT0XCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH9               0x00000009UL                             /**< Mode APORT0XCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH10              0x0000000AUL                             /**< Mode APORT0XCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH11              0x0000000BUL                             /**< Mode APORT0XCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH12              0x0000000CUL                             /**< Mode APORT0XCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH13              0x0000000DUL                             /**< Mode APORT0XCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH14              0x0000000EUL                             /**< Mode APORT0XCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0XCH15              0x0000000FUL                             /**< Mode APORT0XCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH0               0x00000010UL                             /**< Mode APORT0YCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH1               0x00000011UL                             /**< Mode APORT0YCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH2               0x00000012UL                             /**< Mode APORT0YCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH3               0x00000013UL                             /**< Mode APORT0YCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH4               0x00000014UL                             /**< Mode APORT0YCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH5               0x00000015UL                             /**< Mode APORT0YCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH6               0x00000016UL                             /**< Mode APORT0YCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH7               0x00000017UL                             /**< Mode APORT0YCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH8               0x00000018UL                             /**< Mode APORT0YCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH9               0x00000019UL                             /**< Mode APORT0YCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH10              0x0000001AUL                             /**< Mode APORT0YCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH11              0x0000001BUL                             /**< Mode APORT0YCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH12              0x0000001CUL                             /**< Mode APORT0YCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH13              0x0000001DUL                             /**< Mode APORT0YCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH14              0x0000001EUL                             /**< Mode APORT0YCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT0YCH15              0x0000001FUL                             /**< Mode APORT0YCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH0               0x00000020UL                             /**< Mode APORT1XCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH1               0x00000021UL                             /**< Mode APORT1YCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH2               0x00000022UL                             /**< Mode APORT1XCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH3               0x00000023UL                             /**< Mode APORT1YCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH4               0x00000024UL                             /**< Mode APORT1XCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH5               0x00000025UL                             /**< Mode APORT1YCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH6               0x00000026UL                             /**< Mode APORT1XCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH7               0x00000027UL                             /**< Mode APORT1YCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH8               0x00000028UL                             /**< Mode APORT1XCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH9               0x00000029UL                             /**< Mode APORT1YCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH10              0x0000002AUL                             /**< Mode APORT1XCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH11              0x0000002BUL                             /**< Mode APORT1YCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH12              0x0000002CUL                             /**< Mode APORT1XCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH13              0x0000002DUL                             /**< Mode APORT1YCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH14              0x0000002EUL                             /**< Mode APORT1XCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH15              0x0000002FUL                             /**< Mode APORT1YCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH16              0x00000030UL                             /**< Mode APORT1XCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH17              0x00000031UL                             /**< Mode APORT1YCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH18              0x00000032UL                             /**< Mode APORT1XCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH19              0x00000033UL                             /**< Mode APORT1YCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH20              0x00000034UL                             /**< Mode APORT1XCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH21              0x00000035UL                             /**< Mode APORT1YCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH22              0x00000036UL                             /**< Mode APORT1XCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH23              0x00000037UL                             /**< Mode APORT1YCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH24              0x00000038UL                             /**< Mode APORT1XCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH25              0x00000039UL                             /**< Mode APORT1YCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH26              0x0000003AUL                             /**< Mode APORT1XCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH27              0x0000003BUL                             /**< Mode APORT1YCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH28              0x0000003CUL                             /**< Mode APORT1XCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH29              0x0000003DUL                             /**< Mode APORT1YCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1XCH30              0x0000003EUL                             /**< Mode APORT1XCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT1YCH31              0x0000003FUL                             /**< Mode APORT1YCH31 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH0               0x00000040UL                             /**< Mode APORT2YCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH1               0x00000041UL                             /**< Mode APORT2XCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH2               0x00000042UL                             /**< Mode APORT2YCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH3               0x00000043UL                             /**< Mode APORT2XCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH4               0x00000044UL                             /**< Mode APORT2YCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH5               0x00000045UL                             /**< Mode APORT2XCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH6               0x00000046UL                             /**< Mode APORT2YCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH7               0x00000047UL                             /**< Mode APORT2XCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH8               0x00000048UL                             /**< Mode APORT2YCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH9               0x00000049UL                             /**< Mode APORT2XCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH10              0x0000004AUL                             /**< Mode APORT2YCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH11              0x0000004BUL                             /**< Mode APORT2XCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH12              0x0000004CUL                             /**< Mode APORT2YCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH13              0x0000004DUL                             /**< Mode APORT2XCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH14              0x0000004EUL                             /**< Mode APORT2YCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH15              0x0000004FUL                             /**< Mode APORT2XCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH16              0x00000050UL                             /**< Mode APORT2YCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH17              0x00000051UL                             /**< Mode APORT2XCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH18              0x00000052UL                             /**< Mode APORT2YCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH19              0x00000053UL                             /**< Mode APORT2XCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH20              0x00000054UL                             /**< Mode APORT2YCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH21              0x00000055UL                             /**< Mode APORT2XCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH22              0x00000056UL                             /**< Mode APORT2YCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH23              0x00000057UL                             /**< Mode APORT2XCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH24              0x00000058UL                             /**< Mode APORT2YCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH25              0x00000059UL                             /**< Mode APORT2XCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH26              0x0000005AUL                             /**< Mode APORT2YCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH27              0x0000005BUL                             /**< Mode APORT2XCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH28              0x0000005CUL                             /**< Mode APORT2YCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH29              0x0000005DUL                             /**< Mode APORT2XCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2YCH30              0x0000005EUL                             /**< Mode APORT2YCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT2XCH31              0x0000005FUL                             /**< Mode APORT2XCH31 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH0               0x00000060UL                             /**< Mode APORT3XCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH1               0x00000061UL                             /**< Mode APORT3YCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH2               0x00000062UL                             /**< Mode APORT3XCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH3               0x00000063UL                             /**< Mode APORT3YCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH4               0x00000064UL                             /**< Mode APORT3XCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH5               0x00000065UL                             /**< Mode APORT3YCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH6               0x00000066UL                             /**< Mode APORT3XCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH7               0x00000067UL                             /**< Mode APORT3YCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH8               0x00000068UL                             /**< Mode APORT3XCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH9               0x00000069UL                             /**< Mode APORT3YCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH10              0x0000006AUL                             /**< Mode APORT3XCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH11              0x0000006BUL                             /**< Mode APORT3YCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH12              0x0000006CUL                             /**< Mode APORT3XCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH13              0x0000006DUL                             /**< Mode APORT3YCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH14              0x0000006EUL                             /**< Mode APORT3XCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH15              0x0000006FUL                             /**< Mode APORT3YCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH16              0x00000070UL                             /**< Mode APORT3XCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH17              0x00000071UL                             /**< Mode APORT3YCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH18              0x00000072UL                             /**< Mode APORT3XCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH19              0x00000073UL                             /**< Mode APORT3YCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH20              0x00000074UL                             /**< Mode APORT3XCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH21              0x00000075UL                             /**< Mode APORT3YCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH22              0x00000076UL                             /**< Mode APORT3XCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH23              0x00000077UL                             /**< Mode APORT3YCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH24              0x00000078UL                             /**< Mode APORT3XCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH25              0x00000079UL                             /**< Mode APORT3YCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH26              0x0000007AUL                             /**< Mode APORT3XCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH27              0x0000007BUL                             /**< Mode APORT3YCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH28              0x0000007CUL                             /**< Mode APORT3XCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH29              0x0000007DUL                             /**< Mode APORT3YCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3XCH30              0x0000007EUL                             /**< Mode APORT3XCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT3YCH31              0x0000007FUL                             /**< Mode APORT3YCH31 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH0               0x00000080UL                             /**< Mode APORT4YCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH1               0x00000081UL                             /**< Mode APORT4XCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH2               0x00000082UL                             /**< Mode APORT4YCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH3               0x00000083UL                             /**< Mode APORT4XCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH4               0x00000084UL                             /**< Mode APORT4YCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH5               0x00000085UL                             /**< Mode APORT4XCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH6               0x00000086UL                             /**< Mode APORT4YCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH7               0x00000087UL                             /**< Mode APORT4XCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH8               0x00000088UL                             /**< Mode APORT4YCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH9               0x00000089UL                             /**< Mode APORT4XCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH10              0x0000008AUL                             /**< Mode APORT4YCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH11              0x0000008BUL                             /**< Mode APORT4XCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH12              0x0000008CUL                             /**< Mode APORT4YCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH13              0x0000008DUL                             /**< Mode APORT4XCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH16              0x00000090UL                             /**< Mode APORT4YCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH17              0x00000091UL                             /**< Mode APORT4XCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH18              0x00000092UL                             /**< Mode APORT4YCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH19              0x00000093UL                             /**< Mode APORT4XCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH20              0x00000094UL                             /**< Mode APORT4YCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH21              0x00000095UL                             /**< Mode APORT4XCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH22              0x00000096UL                             /**< Mode APORT4YCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH23              0x00000097UL                             /**< Mode APORT4XCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH24              0x00000098UL                             /**< Mode APORT4YCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH25              0x00000099UL                             /**< Mode APORT4XCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH26              0x0000009AUL                             /**< Mode APORT4YCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH27              0x0000009BUL                             /**< Mode APORT4XCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH28              0x0000009CUL                             /**< Mode APORT4YCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH29              0x0000009DUL                             /**< Mode APORT4XCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH30              0x0000009EUL                             /**< Mode APORT4YCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4YCH14              0x0000009EUL                             /**< Mode APORT4YCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH15              0x0000009FUL                             /**< Mode APORT4XCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_APORT4XCH31              0x0000009FUL                             /**< Mode APORT4XCH31 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_VLP                      0x000000FBUL                             /**< Mode VLP for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_VBDIV                    0x000000FCUL                             /**< Mode VBDIV for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_VADIV                    0x000000FDUL                             /**< Mode VADIV for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_VDD                      0x000000FEUL                             /**< Mode VDD for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_POSSEL_VSS                      0x000000FFUL                             /**< Mode VSS for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_DEFAULT                   (_ACMP_INPUTSEL_POSSEL_DEFAULT << 0)     /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH0                (_ACMP_INPUTSEL_POSSEL_APORT0XCH0 << 0)  /**< Shifted mode APORT0XCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH1                (_ACMP_INPUTSEL_POSSEL_APORT0XCH1 << 0)  /**< Shifted mode APORT0XCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH2                (_ACMP_INPUTSEL_POSSEL_APORT0XCH2 << 0)  /**< Shifted mode APORT0XCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH3                (_ACMP_INPUTSEL_POSSEL_APORT0XCH3 << 0)  /**< Shifted mode APORT0XCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH4                (_ACMP_INPUTSEL_POSSEL_APORT0XCH4 << 0)  /**< Shifted mode APORT0XCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH5                (_ACMP_INPUTSEL_POSSEL_APORT0XCH5 << 0)  /**< Shifted mode APORT0XCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH6                (_ACMP_INPUTSEL_POSSEL_APORT0XCH6 << 0)  /**< Shifted mode APORT0XCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH7                (_ACMP_INPUTSEL_POSSEL_APORT0XCH7 << 0)  /**< Shifted mode APORT0XCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH8                (_ACMP_INPUTSEL_POSSEL_APORT0XCH8 << 0)  /**< Shifted mode APORT0XCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH9                (_ACMP_INPUTSEL_POSSEL_APORT0XCH9 << 0)  /**< Shifted mode APORT0XCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH10               (_ACMP_INPUTSEL_POSSEL_APORT0XCH10 << 0) /**< Shifted mode APORT0XCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH11               (_ACMP_INPUTSEL_POSSEL_APORT0XCH11 << 0) /**< Shifted mode APORT0XCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH12               (_ACMP_INPUTSEL_POSSEL_APORT0XCH12 << 0) /**< Shifted mode APORT0XCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH13               (_ACMP_INPUTSEL_POSSEL_APORT0XCH13 << 0) /**< Shifted mode APORT0XCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH14               (_ACMP_INPUTSEL_POSSEL_APORT0XCH14 << 0) /**< Shifted mode APORT0XCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0XCH15               (_ACMP_INPUTSEL_POSSEL_APORT0XCH15 << 0) /**< Shifted mode APORT0XCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH0                (_ACMP_INPUTSEL_POSSEL_APORT0YCH0 << 0)  /**< Shifted mode APORT0YCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH1                (_ACMP_INPUTSEL_POSSEL_APORT0YCH1 << 0)  /**< Shifted mode APORT0YCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH2                (_ACMP_INPUTSEL_POSSEL_APORT0YCH2 << 0)  /**< Shifted mode APORT0YCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH3                (_ACMP_INPUTSEL_POSSEL_APORT0YCH3 << 0)  /**< Shifted mode APORT0YCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH4                (_ACMP_INPUTSEL_POSSEL_APORT0YCH4 << 0)  /**< Shifted mode APORT0YCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH5                (_ACMP_INPUTSEL_POSSEL_APORT0YCH5 << 0)  /**< Shifted mode APORT0YCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH6                (_ACMP_INPUTSEL_POSSEL_APORT0YCH6 << 0)  /**< Shifted mode APORT0YCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH7                (_ACMP_INPUTSEL_POSSEL_APORT0YCH7 << 0)  /**< Shifted mode APORT0YCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH8                (_ACMP_INPUTSEL_POSSEL_APORT0YCH8 << 0)  /**< Shifted mode APORT0YCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH9                (_ACMP_INPUTSEL_POSSEL_APORT0YCH9 << 0)  /**< Shifted mode APORT0YCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH10               (_ACMP_INPUTSEL_POSSEL_APORT0YCH10 << 0) /**< Shifted mode APORT0YCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH11               (_ACMP_INPUTSEL_POSSEL_APORT0YCH11 << 0) /**< Shifted mode APORT0YCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH12               (_ACMP_INPUTSEL_POSSEL_APORT0YCH12 << 0) /**< Shifted mode APORT0YCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH13               (_ACMP_INPUTSEL_POSSEL_APORT0YCH13 << 0) /**< Shifted mode APORT0YCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH14               (_ACMP_INPUTSEL_POSSEL_APORT0YCH14 << 0) /**< Shifted mode APORT0YCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT0YCH15               (_ACMP_INPUTSEL_POSSEL_APORT0YCH15 << 0) /**< Shifted mode APORT0YCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH0                (_ACMP_INPUTSEL_POSSEL_APORT1XCH0 << 0)  /**< Shifted mode APORT1XCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH1                (_ACMP_INPUTSEL_POSSEL_APORT1YCH1 << 0)  /**< Shifted mode APORT1YCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH2                (_ACMP_INPUTSEL_POSSEL_APORT1XCH2 << 0)  /**< Shifted mode APORT1XCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH3                (_ACMP_INPUTSEL_POSSEL_APORT1YCH3 << 0)  /**< Shifted mode APORT1YCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH4                (_ACMP_INPUTSEL_POSSEL_APORT1XCH4 << 0)  /**< Shifted mode APORT1XCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH5                (_ACMP_INPUTSEL_POSSEL_APORT1YCH5 << 0)  /**< Shifted mode APORT1YCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH6                (_ACMP_INPUTSEL_POSSEL_APORT1XCH6 << 0)  /**< Shifted mode APORT1XCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH7                (_ACMP_INPUTSEL_POSSEL_APORT1YCH7 << 0)  /**< Shifted mode APORT1YCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH8                (_ACMP_INPUTSEL_POSSEL_APORT1XCH8 << 0)  /**< Shifted mode APORT1XCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH9                (_ACMP_INPUTSEL_POSSEL_APORT1YCH9 << 0)  /**< Shifted mode APORT1YCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH10               (_ACMP_INPUTSEL_POSSEL_APORT1XCH10 << 0) /**< Shifted mode APORT1XCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH11               (_ACMP_INPUTSEL_POSSEL_APORT1YCH11 << 0) /**< Shifted mode APORT1YCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH12               (_ACMP_INPUTSEL_POSSEL_APORT1XCH12 << 0) /**< Shifted mode APORT1XCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH13               (_ACMP_INPUTSEL_POSSEL_APORT1YCH13 << 0) /**< Shifted mode APORT1YCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH14               (_ACMP_INPUTSEL_POSSEL_APORT1XCH14 << 0) /**< Shifted mode APORT1XCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH15               (_ACMP_INPUTSEL_POSSEL_APORT1YCH15 << 0) /**< Shifted mode APORT1YCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH16               (_ACMP_INPUTSEL_POSSEL_APORT1XCH16 << 0) /**< Shifted mode APORT1XCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH17               (_ACMP_INPUTSEL_POSSEL_APORT1YCH17 << 0) /**< Shifted mode APORT1YCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH18               (_ACMP_INPUTSEL_POSSEL_APORT1XCH18 << 0) /**< Shifted mode APORT1XCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH19               (_ACMP_INPUTSEL_POSSEL_APORT1YCH19 << 0) /**< Shifted mode APORT1YCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH20               (_ACMP_INPUTSEL_POSSEL_APORT1XCH20 << 0) /**< Shifted mode APORT1XCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH21               (_ACMP_INPUTSEL_POSSEL_APORT1YCH21 << 0) /**< Shifted mode APORT1YCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH22               (_ACMP_INPUTSEL_POSSEL_APORT1XCH22 << 0) /**< Shifted mode APORT1XCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH23               (_ACMP_INPUTSEL_POSSEL_APORT1YCH23 << 0) /**< Shifted mode APORT1YCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH24               (_ACMP_INPUTSEL_POSSEL_APORT1XCH24 << 0) /**< Shifted mode APORT1XCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH25               (_ACMP_INPUTSEL_POSSEL_APORT1YCH25 << 0) /**< Shifted mode APORT1YCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH26               (_ACMP_INPUTSEL_POSSEL_APORT1XCH26 << 0) /**< Shifted mode APORT1XCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH27               (_ACMP_INPUTSEL_POSSEL_APORT1YCH27 << 0) /**< Shifted mode APORT1YCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH28               (_ACMP_INPUTSEL_POSSEL_APORT1XCH28 << 0) /**< Shifted mode APORT1XCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH29               (_ACMP_INPUTSEL_POSSEL_APORT1YCH29 << 0) /**< Shifted mode APORT1YCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1XCH30               (_ACMP_INPUTSEL_POSSEL_APORT1XCH30 << 0) /**< Shifted mode APORT1XCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT1YCH31               (_ACMP_INPUTSEL_POSSEL_APORT1YCH31 << 0) /**< Shifted mode APORT1YCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH0                (_ACMP_INPUTSEL_POSSEL_APORT2YCH0 << 0)  /**< Shifted mode APORT2YCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH1                (_ACMP_INPUTSEL_POSSEL_APORT2XCH1 << 0)  /**< Shifted mode APORT2XCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH2                (_ACMP_INPUTSEL_POSSEL_APORT2YCH2 << 0)  /**< Shifted mode APORT2YCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH3                (_ACMP_INPUTSEL_POSSEL_APORT2XCH3 << 0)  /**< Shifted mode APORT2XCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH4                (_ACMP_INPUTSEL_POSSEL_APORT2YCH4 << 0)  /**< Shifted mode APORT2YCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH5                (_ACMP_INPUTSEL_POSSEL_APORT2XCH5 << 0)  /**< Shifted mode APORT2XCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH6                (_ACMP_INPUTSEL_POSSEL_APORT2YCH6 << 0)  /**< Shifted mode APORT2YCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH7                (_ACMP_INPUTSEL_POSSEL_APORT2XCH7 << 0)  /**< Shifted mode APORT2XCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH8                (_ACMP_INPUTSEL_POSSEL_APORT2YCH8 << 0)  /**< Shifted mode APORT2YCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH9                (_ACMP_INPUTSEL_POSSEL_APORT2XCH9 << 0)  /**< Shifted mode APORT2XCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH10               (_ACMP_INPUTSEL_POSSEL_APORT2YCH10 << 0) /**< Shifted mode APORT2YCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH11               (_ACMP_INPUTSEL_POSSEL_APORT2XCH11 << 0) /**< Shifted mode APORT2XCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH12               (_ACMP_INPUTSEL_POSSEL_APORT2YCH12 << 0) /**< Shifted mode APORT2YCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH13               (_ACMP_INPUTSEL_POSSEL_APORT2XCH13 << 0) /**< Shifted mode APORT2XCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH14               (_ACMP_INPUTSEL_POSSEL_APORT2YCH14 << 0) /**< Shifted mode APORT2YCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH15               (_ACMP_INPUTSEL_POSSEL_APORT2XCH15 << 0) /**< Shifted mode APORT2XCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH16               (_ACMP_INPUTSEL_POSSEL_APORT2YCH16 << 0) /**< Shifted mode APORT2YCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH17               (_ACMP_INPUTSEL_POSSEL_APORT2XCH17 << 0) /**< Shifted mode APORT2XCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH18               (_ACMP_INPUTSEL_POSSEL_APORT2YCH18 << 0) /**< Shifted mode APORT2YCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH19               (_ACMP_INPUTSEL_POSSEL_APORT2XCH19 << 0) /**< Shifted mode APORT2XCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH20               (_ACMP_INPUTSEL_POSSEL_APORT2YCH20 << 0) /**< Shifted mode APORT2YCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH21               (_ACMP_INPUTSEL_POSSEL_APORT2XCH21 << 0) /**< Shifted mode APORT2XCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH22               (_ACMP_INPUTSEL_POSSEL_APORT2YCH22 << 0) /**< Shifted mode APORT2YCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH23               (_ACMP_INPUTSEL_POSSEL_APORT2XCH23 << 0) /**< Shifted mode APORT2XCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH24               (_ACMP_INPUTSEL_POSSEL_APORT2YCH24 << 0) /**< Shifted mode APORT2YCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH25               (_ACMP_INPUTSEL_POSSEL_APORT2XCH25 << 0) /**< Shifted mode APORT2XCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH26               (_ACMP_INPUTSEL_POSSEL_APORT2YCH26 << 0) /**< Shifted mode APORT2YCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH27               (_ACMP_INPUTSEL_POSSEL_APORT2XCH27 << 0) /**< Shifted mode APORT2XCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH28               (_ACMP_INPUTSEL_POSSEL_APORT2YCH28 << 0) /**< Shifted mode APORT2YCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH29               (_ACMP_INPUTSEL_POSSEL_APORT2XCH29 << 0) /**< Shifted mode APORT2XCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2YCH30               (_ACMP_INPUTSEL_POSSEL_APORT2YCH30 << 0) /**< Shifted mode APORT2YCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT2XCH31               (_ACMP_INPUTSEL_POSSEL_APORT2XCH31 << 0) /**< Shifted mode APORT2XCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH0                (_ACMP_INPUTSEL_POSSEL_APORT3XCH0 << 0)  /**< Shifted mode APORT3XCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH1                (_ACMP_INPUTSEL_POSSEL_APORT3YCH1 << 0)  /**< Shifted mode APORT3YCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH2                (_ACMP_INPUTSEL_POSSEL_APORT3XCH2 << 0)  /**< Shifted mode APORT3XCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH3                (_ACMP_INPUTSEL_POSSEL_APORT3YCH3 << 0)  /**< Shifted mode APORT3YCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH4                (_ACMP_INPUTSEL_POSSEL_APORT3XCH4 << 0)  /**< Shifted mode APORT3XCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH5                (_ACMP_INPUTSEL_POSSEL_APORT3YCH5 << 0)  /**< Shifted mode APORT3YCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH6                (_ACMP_INPUTSEL_POSSEL_APORT3XCH6 << 0)  /**< Shifted mode APORT3XCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH7                (_ACMP_INPUTSEL_POSSEL_APORT3YCH7 << 0)  /**< Shifted mode APORT3YCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH8                (_ACMP_INPUTSEL_POSSEL_APORT3XCH8 << 0)  /**< Shifted mode APORT3XCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH9                (_ACMP_INPUTSEL_POSSEL_APORT3YCH9 << 0)  /**< Shifted mode APORT3YCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH10               (_ACMP_INPUTSEL_POSSEL_APORT3XCH10 << 0) /**< Shifted mode APORT3XCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH11               (_ACMP_INPUTSEL_POSSEL_APORT3YCH11 << 0) /**< Shifted mode APORT3YCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH12               (_ACMP_INPUTSEL_POSSEL_APORT3XCH12 << 0) /**< Shifted mode APORT3XCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH13               (_ACMP_INPUTSEL_POSSEL_APORT3YCH13 << 0) /**< Shifted mode APORT3YCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH14               (_ACMP_INPUTSEL_POSSEL_APORT3XCH14 << 0) /**< Shifted mode APORT3XCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH15               (_ACMP_INPUTSEL_POSSEL_APORT3YCH15 << 0) /**< Shifted mode APORT3YCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH16               (_ACMP_INPUTSEL_POSSEL_APORT3XCH16 << 0) /**< Shifted mode APORT3XCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH17               (_ACMP_INPUTSEL_POSSEL_APORT3YCH17 << 0) /**< Shifted mode APORT3YCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH18               (_ACMP_INPUTSEL_POSSEL_APORT3XCH18 << 0) /**< Shifted mode APORT3XCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH19               (_ACMP_INPUTSEL_POSSEL_APORT3YCH19 << 0) /**< Shifted mode APORT3YCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH20               (_ACMP_INPUTSEL_POSSEL_APORT3XCH20 << 0) /**< Shifted mode APORT3XCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH21               (_ACMP_INPUTSEL_POSSEL_APORT3YCH21 << 0) /**< Shifted mode APORT3YCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH22               (_ACMP_INPUTSEL_POSSEL_APORT3XCH22 << 0) /**< Shifted mode APORT3XCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH23               (_ACMP_INPUTSEL_POSSEL_APORT3YCH23 << 0) /**< Shifted mode APORT3YCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH24               (_ACMP_INPUTSEL_POSSEL_APORT3XCH24 << 0) /**< Shifted mode APORT3XCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH25               (_ACMP_INPUTSEL_POSSEL_APORT3YCH25 << 0) /**< Shifted mode APORT3YCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH26               (_ACMP_INPUTSEL_POSSEL_APORT3XCH26 << 0) /**< Shifted mode APORT3XCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH27               (_ACMP_INPUTSEL_POSSEL_APORT3YCH27 << 0) /**< Shifted mode APORT3YCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH28               (_ACMP_INPUTSEL_POSSEL_APORT3XCH28 << 0) /**< Shifted mode APORT3XCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH29               (_ACMP_INPUTSEL_POSSEL_APORT3YCH29 << 0) /**< Shifted mode APORT3YCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3XCH30               (_ACMP_INPUTSEL_POSSEL_APORT3XCH30 << 0) /**< Shifted mode APORT3XCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT3YCH31               (_ACMP_INPUTSEL_POSSEL_APORT3YCH31 << 0) /**< Shifted mode APORT3YCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH0                (_ACMP_INPUTSEL_POSSEL_APORT4YCH0 << 0)  /**< Shifted mode APORT4YCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH1                (_ACMP_INPUTSEL_POSSEL_APORT4XCH1 << 0)  /**< Shifted mode APORT4XCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH2                (_ACMP_INPUTSEL_POSSEL_APORT4YCH2 << 0)  /**< Shifted mode APORT4YCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH3                (_ACMP_INPUTSEL_POSSEL_APORT4XCH3 << 0)  /**< Shifted mode APORT4XCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH4                (_ACMP_INPUTSEL_POSSEL_APORT4YCH4 << 0)  /**< Shifted mode APORT4YCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH5                (_ACMP_INPUTSEL_POSSEL_APORT4XCH5 << 0)  /**< Shifted mode APORT4XCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH6                (_ACMP_INPUTSEL_POSSEL_APORT4YCH6 << 0)  /**< Shifted mode APORT4YCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH7                (_ACMP_INPUTSEL_POSSEL_APORT4XCH7 << 0)  /**< Shifted mode APORT4XCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH8                (_ACMP_INPUTSEL_POSSEL_APORT4YCH8 << 0)  /**< Shifted mode APORT4YCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH9                (_ACMP_INPUTSEL_POSSEL_APORT4XCH9 << 0)  /**< Shifted mode APORT4XCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH10               (_ACMP_INPUTSEL_POSSEL_APORT4YCH10 << 0) /**< Shifted mode APORT4YCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH11               (_ACMP_INPUTSEL_POSSEL_APORT4XCH11 << 0) /**< Shifted mode APORT4XCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH12               (_ACMP_INPUTSEL_POSSEL_APORT4YCH12 << 0) /**< Shifted mode APORT4YCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH13               (_ACMP_INPUTSEL_POSSEL_APORT4XCH13 << 0) /**< Shifted mode APORT4XCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH16               (_ACMP_INPUTSEL_POSSEL_APORT4YCH16 << 0) /**< Shifted mode APORT4YCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH17               (_ACMP_INPUTSEL_POSSEL_APORT4XCH17 << 0) /**< Shifted mode APORT4XCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH18               (_ACMP_INPUTSEL_POSSEL_APORT4YCH18 << 0) /**< Shifted mode APORT4YCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH19               (_ACMP_INPUTSEL_POSSEL_APORT4XCH19 << 0) /**< Shifted mode APORT4XCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH20               (_ACMP_INPUTSEL_POSSEL_APORT4YCH20 << 0) /**< Shifted mode APORT4YCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH21               (_ACMP_INPUTSEL_POSSEL_APORT4XCH21 << 0) /**< Shifted mode APORT4XCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH22               (_ACMP_INPUTSEL_POSSEL_APORT4YCH22 << 0) /**< Shifted mode APORT4YCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH23               (_ACMP_INPUTSEL_POSSEL_APORT4XCH23 << 0) /**< Shifted mode APORT4XCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH24               (_ACMP_INPUTSEL_POSSEL_APORT4YCH24 << 0) /**< Shifted mode APORT4YCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH25               (_ACMP_INPUTSEL_POSSEL_APORT4XCH25 << 0) /**< Shifted mode APORT4XCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH26               (_ACMP_INPUTSEL_POSSEL_APORT4YCH26 << 0) /**< Shifted mode APORT4YCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH27               (_ACMP_INPUTSEL_POSSEL_APORT4XCH27 << 0) /**< Shifted mode APORT4XCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH28               (_ACMP_INPUTSEL_POSSEL_APORT4YCH28 << 0) /**< Shifted mode APORT4YCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH29               (_ACMP_INPUTSEL_POSSEL_APORT4XCH29 << 0) /**< Shifted mode APORT4XCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH30               (_ACMP_INPUTSEL_POSSEL_APORT4YCH30 << 0) /**< Shifted mode APORT4YCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4YCH14               (_ACMP_INPUTSEL_POSSEL_APORT4YCH14 << 0) /**< Shifted mode APORT4YCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH15               (_ACMP_INPUTSEL_POSSEL_APORT4XCH15 << 0) /**< Shifted mode APORT4XCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_APORT4XCH31               (_ACMP_INPUTSEL_POSSEL_APORT4XCH31 << 0) /**< Shifted mode APORT4XCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_VLP                       (_ACMP_INPUTSEL_POSSEL_VLP << 0)         /**< Shifted mode VLP for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_VBDIV                     (_ACMP_INPUTSEL_POSSEL_VBDIV << 0)       /**< Shifted mode VBDIV for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_VADIV                     (_ACMP_INPUTSEL_POSSEL_VADIV << 0)       /**< Shifted mode VADIV for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_VDD                       (_ACMP_INPUTSEL_POSSEL_VDD << 0)         /**< Shifted mode VDD for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_POSSEL_VSS                       (_ACMP_INPUTSEL_POSSEL_VSS << 0)         /**< Shifted mode VSS for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_SHIFT                    8                                        /**< Shift value for ACMP_NEGSEL */
#define _ACMP_INPUTSEL_NEGSEL_MASK                     0xFF00UL                                 /**< Bit mask for ACMP_NEGSEL */
#define _ACMP_INPUTSEL_NEGSEL_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH0               0x00000000UL                             /**< Mode APORT0XCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH1               0x00000001UL                             /**< Mode APORT0XCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH2               0x00000002UL                             /**< Mode APORT0XCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH3               0x00000003UL                             /**< Mode APORT0XCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH4               0x00000004UL                             /**< Mode APORT0XCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH5               0x00000005UL                             /**< Mode APORT0XCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH6               0x00000006UL                             /**< Mode APORT0XCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH7               0x00000007UL                             /**< Mode APORT0XCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH8               0x00000008UL                             /**< Mode APORT0XCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH9               0x00000009UL                             /**< Mode APORT0XCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH10              0x0000000AUL                             /**< Mode APORT0XCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH11              0x0000000BUL                             /**< Mode APORT0XCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH12              0x0000000CUL                             /**< Mode APORT0XCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH13              0x0000000DUL                             /**< Mode APORT0XCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH14              0x0000000EUL                             /**< Mode APORT0XCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0XCH15              0x0000000FUL                             /**< Mode APORT0XCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH0               0x00000010UL                             /**< Mode APORT0YCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH1               0x00000011UL                             /**< Mode APORT0YCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH2               0x00000012UL                             /**< Mode APORT0YCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH3               0x00000013UL                             /**< Mode APORT0YCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH4               0x00000014UL                             /**< Mode APORT0YCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH5               0x00000015UL                             /**< Mode APORT0YCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH6               0x00000016UL                             /**< Mode APORT0YCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH7               0x00000017UL                             /**< Mode APORT0YCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH8               0x00000018UL                             /**< Mode APORT0YCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH9               0x00000019UL                             /**< Mode APORT0YCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH10              0x0000001AUL                             /**< Mode APORT0YCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH11              0x0000001BUL                             /**< Mode APORT0YCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH12              0x0000001CUL                             /**< Mode APORT0YCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH13              0x0000001DUL                             /**< Mode APORT0YCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH14              0x0000001EUL                             /**< Mode APORT0YCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT0YCH15              0x0000001FUL                             /**< Mode APORT0YCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH0               0x00000020UL                             /**< Mode APORT1XCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH1               0x00000021UL                             /**< Mode APORT1YCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH2               0x00000022UL                             /**< Mode APORT1XCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH3               0x00000023UL                             /**< Mode APORT1YCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH4               0x00000024UL                             /**< Mode APORT1XCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH5               0x00000025UL                             /**< Mode APORT1YCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH6               0x00000026UL                             /**< Mode APORT1XCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH7               0x00000027UL                             /**< Mode APORT1YCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH8               0x00000028UL                             /**< Mode APORT1XCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH9               0x00000029UL                             /**< Mode APORT1YCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH10              0x0000002AUL                             /**< Mode APORT1XCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH11              0x0000002BUL                             /**< Mode APORT1YCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH12              0x0000002CUL                             /**< Mode APORT1XCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH13              0x0000002DUL                             /**< Mode APORT1YCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH14              0x0000002EUL                             /**< Mode APORT1XCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH15              0x0000002FUL                             /**< Mode APORT1YCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH16              0x00000030UL                             /**< Mode APORT1XCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH17              0x00000031UL                             /**< Mode APORT1YCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH18              0x00000032UL                             /**< Mode APORT1XCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH19              0x00000033UL                             /**< Mode APORT1YCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH20              0x00000034UL                             /**< Mode APORT1XCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH21              0x00000035UL                             /**< Mode APORT1YCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH22              0x00000036UL                             /**< Mode APORT1XCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH23              0x00000037UL                             /**< Mode APORT1YCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH24              0x00000038UL                             /**< Mode APORT1XCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH25              0x00000039UL                             /**< Mode APORT1YCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH26              0x0000003AUL                             /**< Mode APORT1XCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH27              0x0000003BUL                             /**< Mode APORT1YCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH28              0x0000003CUL                             /**< Mode APORT1XCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH29              0x0000003DUL                             /**< Mode APORT1YCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1XCH30              0x0000003EUL                             /**< Mode APORT1XCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT1YCH31              0x0000003FUL                             /**< Mode APORT1YCH31 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH0               0x00000040UL                             /**< Mode APORT2YCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH1               0x00000041UL                             /**< Mode APORT2XCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH2               0x00000042UL                             /**< Mode APORT2YCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH3               0x00000043UL                             /**< Mode APORT2XCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH4               0x00000044UL                             /**< Mode APORT2YCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH5               0x00000045UL                             /**< Mode APORT2XCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH6               0x00000046UL                             /**< Mode APORT2YCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH7               0x00000047UL                             /**< Mode APORT2XCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH8               0x00000048UL                             /**< Mode APORT2YCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH9               0x00000049UL                             /**< Mode APORT2XCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH10              0x0000004AUL                             /**< Mode APORT2YCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH11              0x0000004BUL                             /**< Mode APORT2XCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH12              0x0000004CUL                             /**< Mode APORT2YCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH13              0x0000004DUL                             /**< Mode APORT2XCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH14              0x0000004EUL                             /**< Mode APORT2YCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH15              0x0000004FUL                             /**< Mode APORT2XCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH16              0x00000050UL                             /**< Mode APORT2YCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH17              0x00000051UL                             /**< Mode APORT2XCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH18              0x00000052UL                             /**< Mode APORT2YCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH19              0x00000053UL                             /**< Mode APORT2XCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH20              0x00000054UL                             /**< Mode APORT2YCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH21              0x00000055UL                             /**< Mode APORT2XCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH22              0x00000056UL                             /**< Mode APORT2YCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH23              0x00000057UL                             /**< Mode APORT2XCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH24              0x00000058UL                             /**< Mode APORT2YCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH25              0x00000059UL                             /**< Mode APORT2XCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH26              0x0000005AUL                             /**< Mode APORT2YCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH27              0x0000005BUL                             /**< Mode APORT2XCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH28              0x0000005CUL                             /**< Mode APORT2YCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH29              0x0000005DUL                             /**< Mode APORT2XCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2YCH30              0x0000005EUL                             /**< Mode APORT2YCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT2XCH31              0x0000005FUL                             /**< Mode APORT2XCH31 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH0               0x00000060UL                             /**< Mode APORT3XCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH1               0x00000061UL                             /**< Mode APORT3YCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH2               0x00000062UL                             /**< Mode APORT3XCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH3               0x00000063UL                             /**< Mode APORT3YCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH4               0x00000064UL                             /**< Mode APORT3XCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH5               0x00000065UL                             /**< Mode APORT3YCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH6               0x00000066UL                             /**< Mode APORT3XCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH7               0x00000067UL                             /**< Mode APORT3YCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH8               0x00000068UL                             /**< Mode APORT3XCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH9               0x00000069UL                             /**< Mode APORT3YCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH10              0x0000006AUL                             /**< Mode APORT3XCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH11              0x0000006BUL                             /**< Mode APORT3YCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH12              0x0000006CUL                             /**< Mode APORT3XCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH13              0x0000006DUL                             /**< Mode APORT3YCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH14              0x0000006EUL                             /**< Mode APORT3XCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH15              0x0000006FUL                             /**< Mode APORT3YCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH16              0x00000070UL                             /**< Mode APORT3XCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH17              0x00000071UL                             /**< Mode APORT3YCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH18              0x00000072UL                             /**< Mode APORT3XCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH19              0x00000073UL                             /**< Mode APORT3YCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH20              0x00000074UL                             /**< Mode APORT3XCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH21              0x00000075UL                             /**< Mode APORT3YCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH22              0x00000076UL                             /**< Mode APORT3XCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH23              0x00000077UL                             /**< Mode APORT3YCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH24              0x00000078UL                             /**< Mode APORT3XCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH25              0x00000079UL                             /**< Mode APORT3YCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH26              0x0000007AUL                             /**< Mode APORT3XCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH27              0x0000007BUL                             /**< Mode APORT3YCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH28              0x0000007CUL                             /**< Mode APORT3XCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH29              0x0000007DUL                             /**< Mode APORT3YCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3XCH30              0x0000007EUL                             /**< Mode APORT3XCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT3YCH31              0x0000007FUL                             /**< Mode APORT3YCH31 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH0               0x00000080UL                             /**< Mode APORT4YCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH1               0x00000081UL                             /**< Mode APORT4XCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH2               0x00000082UL                             /**< Mode APORT4YCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH3               0x00000083UL                             /**< Mode APORT4XCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH4               0x00000084UL                             /**< Mode APORT4YCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH5               0x00000085UL                             /**< Mode APORT4XCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH6               0x00000086UL                             /**< Mode APORT4YCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH7               0x00000087UL                             /**< Mode APORT4XCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH8               0x00000088UL                             /**< Mode APORT4YCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH9               0x00000089UL                             /**< Mode APORT4XCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH10              0x0000008AUL                             /**< Mode APORT4YCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH11              0x0000008BUL                             /**< Mode APORT4XCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH12              0x0000008CUL                             /**< Mode APORT4YCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH13              0x0000008DUL                             /**< Mode APORT4XCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH16              0x00000090UL                             /**< Mode APORT4YCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH17              0x00000091UL                             /**< Mode APORT4XCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH18              0x00000092UL                             /**< Mode APORT4YCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH19              0x00000093UL                             /**< Mode APORT4XCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH20              0x00000094UL                             /**< Mode APORT4YCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH21              0x00000095UL                             /**< Mode APORT4XCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH22              0x00000096UL                             /**< Mode APORT4YCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH23              0x00000097UL                             /**< Mode APORT4XCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH24              0x00000098UL                             /**< Mode APORT4YCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH25              0x00000099UL                             /**< Mode APORT4XCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH26              0x0000009AUL                             /**< Mode APORT4YCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH27              0x0000009BUL                             /**< Mode APORT4XCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH28              0x0000009CUL                             /**< Mode APORT4YCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH29              0x0000009DUL                             /**< Mode APORT4XCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH30              0x0000009EUL                             /**< Mode APORT4YCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4YCH14              0x0000009EUL                             /**< Mode APORT4YCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH15              0x0000009FUL                             /**< Mode APORT4XCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_APORT4XCH31              0x0000009FUL                             /**< Mode APORT4XCH31 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_VLP                      0x000000FBUL                             /**< Mode VLP for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_VBDIV                    0x000000FCUL                             /**< Mode VBDIV for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_VADIV                    0x000000FDUL                             /**< Mode VADIV for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_VDD                      0x000000FEUL                             /**< Mode VDD for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_NEGSEL_VSS                      0x000000FFUL                             /**< Mode VSS for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_DEFAULT                   (_ACMP_INPUTSEL_NEGSEL_DEFAULT << 8)     /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH0                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH0 << 8)  /**< Shifted mode APORT0XCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH1                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH1 << 8)  /**< Shifted mode APORT0XCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH2                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH2 << 8)  /**< Shifted mode APORT0XCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH3                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH3 << 8)  /**< Shifted mode APORT0XCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH4                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH4 << 8)  /**< Shifted mode APORT0XCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH5                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH5 << 8)  /**< Shifted mode APORT0XCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH6                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH6 << 8)  /**< Shifted mode APORT0XCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH7                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH7 << 8)  /**< Shifted mode APORT0XCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH8                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH8 << 8)  /**< Shifted mode APORT0XCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH9                (_ACMP_INPUTSEL_NEGSEL_APORT0XCH9 << 8)  /**< Shifted mode APORT0XCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH10               (_ACMP_INPUTSEL_NEGSEL_APORT0XCH10 << 8) /**< Shifted mode APORT0XCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH11               (_ACMP_INPUTSEL_NEGSEL_APORT0XCH11 << 8) /**< Shifted mode APORT0XCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH12               (_ACMP_INPUTSEL_NEGSEL_APORT0XCH12 << 8) /**< Shifted mode APORT0XCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH13               (_ACMP_INPUTSEL_NEGSEL_APORT0XCH13 << 8) /**< Shifted mode APORT0XCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH14               (_ACMP_INPUTSEL_NEGSEL_APORT0XCH14 << 8) /**< Shifted mode APORT0XCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0XCH15               (_ACMP_INPUTSEL_NEGSEL_APORT0XCH15 << 8) /**< Shifted mode APORT0XCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH0                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH0 << 8)  /**< Shifted mode APORT0YCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH1                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH1 << 8)  /**< Shifted mode APORT0YCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH2                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH2 << 8)  /**< Shifted mode APORT0YCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH3                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH3 << 8)  /**< Shifted mode APORT0YCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH4                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH4 << 8)  /**< Shifted mode APORT0YCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH5                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH5 << 8)  /**< Shifted mode APORT0YCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH6                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH6 << 8)  /**< Shifted mode APORT0YCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH7                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH7 << 8)  /**< Shifted mode APORT0YCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH8                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH8 << 8)  /**< Shifted mode APORT0YCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH9                (_ACMP_INPUTSEL_NEGSEL_APORT0YCH9 << 8)  /**< Shifted mode APORT0YCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH10               (_ACMP_INPUTSEL_NEGSEL_APORT0YCH10 << 8) /**< Shifted mode APORT0YCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH11               (_ACMP_INPUTSEL_NEGSEL_APORT0YCH11 << 8) /**< Shifted mode APORT0YCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH12               (_ACMP_INPUTSEL_NEGSEL_APORT0YCH12 << 8) /**< Shifted mode APORT0YCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH13               (_ACMP_INPUTSEL_NEGSEL_APORT0YCH13 << 8) /**< Shifted mode APORT0YCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH14               (_ACMP_INPUTSEL_NEGSEL_APORT0YCH14 << 8) /**< Shifted mode APORT0YCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT0YCH15               (_ACMP_INPUTSEL_NEGSEL_APORT0YCH15 << 8) /**< Shifted mode APORT0YCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH0                (_ACMP_INPUTSEL_NEGSEL_APORT1XCH0 << 8)  /**< Shifted mode APORT1XCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH1                (_ACMP_INPUTSEL_NEGSEL_APORT1YCH1 << 8)  /**< Shifted mode APORT1YCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH2                (_ACMP_INPUTSEL_NEGSEL_APORT1XCH2 << 8)  /**< Shifted mode APORT1XCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH3                (_ACMP_INPUTSEL_NEGSEL_APORT1YCH3 << 8)  /**< Shifted mode APORT1YCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH4                (_ACMP_INPUTSEL_NEGSEL_APORT1XCH4 << 8)  /**< Shifted mode APORT1XCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH5                (_ACMP_INPUTSEL_NEGSEL_APORT1YCH5 << 8)  /**< Shifted mode APORT1YCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH6                (_ACMP_INPUTSEL_NEGSEL_APORT1XCH6 << 8)  /**< Shifted mode APORT1XCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH7                (_ACMP_INPUTSEL_NEGSEL_APORT1YCH7 << 8)  /**< Shifted mode APORT1YCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH8                (_ACMP_INPUTSEL_NEGSEL_APORT1XCH8 << 8)  /**< Shifted mode APORT1XCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH9                (_ACMP_INPUTSEL_NEGSEL_APORT1YCH9 << 8)  /**< Shifted mode APORT1YCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH10               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH10 << 8) /**< Shifted mode APORT1XCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH11               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH11 << 8) /**< Shifted mode APORT1YCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH12               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH12 << 8) /**< Shifted mode APORT1XCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH13               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH13 << 8) /**< Shifted mode APORT1YCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH14               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH14 << 8) /**< Shifted mode APORT1XCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH15               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH15 << 8) /**< Shifted mode APORT1YCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH16               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH16 << 8) /**< Shifted mode APORT1XCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH17               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH17 << 8) /**< Shifted mode APORT1YCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH18               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH18 << 8) /**< Shifted mode APORT1XCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH19               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH19 << 8) /**< Shifted mode APORT1YCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH20               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH20 << 8) /**< Shifted mode APORT1XCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH21               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH21 << 8) /**< Shifted mode APORT1YCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH22               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH22 << 8) /**< Shifted mode APORT1XCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH23               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH23 << 8) /**< Shifted mode APORT1YCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH24               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH24 << 8) /**< Shifted mode APORT1XCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH25               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH25 << 8) /**< Shifted mode APORT1YCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH26               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH26 << 8) /**< Shifted mode APORT1XCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH27               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH27 << 8) /**< Shifted mode APORT1YCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH28               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH28 << 8) /**< Shifted mode APORT1XCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH29               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH29 << 8) /**< Shifted mode APORT1YCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1XCH30               (_ACMP_INPUTSEL_NEGSEL_APORT1XCH30 << 8) /**< Shifted mode APORT1XCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT1YCH31               (_ACMP_INPUTSEL_NEGSEL_APORT1YCH31 << 8) /**< Shifted mode APORT1YCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH0                (_ACMP_INPUTSEL_NEGSEL_APORT2YCH0 << 8)  /**< Shifted mode APORT2YCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH1                (_ACMP_INPUTSEL_NEGSEL_APORT2XCH1 << 8)  /**< Shifted mode APORT2XCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH2                (_ACMP_INPUTSEL_NEGSEL_APORT2YCH2 << 8)  /**< Shifted mode APORT2YCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH3                (_ACMP_INPUTSEL_NEGSEL_APORT2XCH3 << 8)  /**< Shifted mode APORT2XCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH4                (_ACMP_INPUTSEL_NEGSEL_APORT2YCH4 << 8)  /**< Shifted mode APORT2YCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH5                (_ACMP_INPUTSEL_NEGSEL_APORT2XCH5 << 8)  /**< Shifted mode APORT2XCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH6                (_ACMP_INPUTSEL_NEGSEL_APORT2YCH6 << 8)  /**< Shifted mode APORT2YCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH7                (_ACMP_INPUTSEL_NEGSEL_APORT2XCH7 << 8)  /**< Shifted mode APORT2XCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH8                (_ACMP_INPUTSEL_NEGSEL_APORT2YCH8 << 8)  /**< Shifted mode APORT2YCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH9                (_ACMP_INPUTSEL_NEGSEL_APORT2XCH9 << 8)  /**< Shifted mode APORT2XCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH10               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH10 << 8) /**< Shifted mode APORT2YCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH11               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH11 << 8) /**< Shifted mode APORT2XCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH12               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH12 << 8) /**< Shifted mode APORT2YCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH13               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH13 << 8) /**< Shifted mode APORT2XCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH14               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH14 << 8) /**< Shifted mode APORT2YCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH15               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH15 << 8) /**< Shifted mode APORT2XCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH16               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH16 << 8) /**< Shifted mode APORT2YCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH17               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH17 << 8) /**< Shifted mode APORT2XCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH18               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH18 << 8) /**< Shifted mode APORT2YCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH19               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH19 << 8) /**< Shifted mode APORT2XCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH20               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH20 << 8) /**< Shifted mode APORT2YCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH21               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH21 << 8) /**< Shifted mode APORT2XCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH22               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH22 << 8) /**< Shifted mode APORT2YCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH23               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH23 << 8) /**< Shifted mode APORT2XCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH24               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH24 << 8) /**< Shifted mode APORT2YCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH25               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH25 << 8) /**< Shifted mode APORT2XCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH26               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH26 << 8) /**< Shifted mode APORT2YCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH27               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH27 << 8) /**< Shifted mode APORT2XCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH28               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH28 << 8) /**< Shifted mode APORT2YCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH29               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH29 << 8) /**< Shifted mode APORT2XCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2YCH30               (_ACMP_INPUTSEL_NEGSEL_APORT2YCH30 << 8) /**< Shifted mode APORT2YCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT2XCH31               (_ACMP_INPUTSEL_NEGSEL_APORT2XCH31 << 8) /**< Shifted mode APORT2XCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH0                (_ACMP_INPUTSEL_NEGSEL_APORT3XCH0 << 8)  /**< Shifted mode APORT3XCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH1                (_ACMP_INPUTSEL_NEGSEL_APORT3YCH1 << 8)  /**< Shifted mode APORT3YCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH2                (_ACMP_INPUTSEL_NEGSEL_APORT3XCH2 << 8)  /**< Shifted mode APORT3XCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH3                (_ACMP_INPUTSEL_NEGSEL_APORT3YCH3 << 8)  /**< Shifted mode APORT3YCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH4                (_ACMP_INPUTSEL_NEGSEL_APORT3XCH4 << 8)  /**< Shifted mode APORT3XCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH5                (_ACMP_INPUTSEL_NEGSEL_APORT3YCH5 << 8)  /**< Shifted mode APORT3YCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH6                (_ACMP_INPUTSEL_NEGSEL_APORT3XCH6 << 8)  /**< Shifted mode APORT3XCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH7                (_ACMP_INPUTSEL_NEGSEL_APORT3YCH7 << 8)  /**< Shifted mode APORT3YCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH8                (_ACMP_INPUTSEL_NEGSEL_APORT3XCH8 << 8)  /**< Shifted mode APORT3XCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH9                (_ACMP_INPUTSEL_NEGSEL_APORT3YCH9 << 8)  /**< Shifted mode APORT3YCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH10               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH10 << 8) /**< Shifted mode APORT3XCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH11               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH11 << 8) /**< Shifted mode APORT3YCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH12               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH12 << 8) /**< Shifted mode APORT3XCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH13               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH13 << 8) /**< Shifted mode APORT3YCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH14               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH14 << 8) /**< Shifted mode APORT3XCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH15               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH15 << 8) /**< Shifted mode APORT3YCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH16               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH16 << 8) /**< Shifted mode APORT3XCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH17               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH17 << 8) /**< Shifted mode APORT3YCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH18               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH18 << 8) /**< Shifted mode APORT3XCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH19               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH19 << 8) /**< Shifted mode APORT3YCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH20               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH20 << 8) /**< Shifted mode APORT3XCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH21               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH21 << 8) /**< Shifted mode APORT3YCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH22               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH22 << 8) /**< Shifted mode APORT3XCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH23               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH23 << 8) /**< Shifted mode APORT3YCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH24               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH24 << 8) /**< Shifted mode APORT3XCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH25               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH25 << 8) /**< Shifted mode APORT3YCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH26               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH26 << 8) /**< Shifted mode APORT3XCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH27               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH27 << 8) /**< Shifted mode APORT3YCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH28               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH28 << 8) /**< Shifted mode APORT3XCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH29               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH29 << 8) /**< Shifted mode APORT3YCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3XCH30               (_ACMP_INPUTSEL_NEGSEL_APORT3XCH30 << 8) /**< Shifted mode APORT3XCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT3YCH31               (_ACMP_INPUTSEL_NEGSEL_APORT3YCH31 << 8) /**< Shifted mode APORT3YCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH0                (_ACMP_INPUTSEL_NEGSEL_APORT4YCH0 << 8)  /**< Shifted mode APORT4YCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH1                (_ACMP_INPUTSEL_NEGSEL_APORT4XCH1 << 8)  /**< Shifted mode APORT4XCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH2                (_ACMP_INPUTSEL_NEGSEL_APORT4YCH2 << 8)  /**< Shifted mode APORT4YCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH3                (_ACMP_INPUTSEL_NEGSEL_APORT4XCH3 << 8)  /**< Shifted mode APORT4XCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH4                (_ACMP_INPUTSEL_NEGSEL_APORT4YCH4 << 8)  /**< Shifted mode APORT4YCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH5                (_ACMP_INPUTSEL_NEGSEL_APORT4XCH5 << 8)  /**< Shifted mode APORT4XCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH6                (_ACMP_INPUTSEL_NEGSEL_APORT4YCH6 << 8)  /**< Shifted mode APORT4YCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH7                (_ACMP_INPUTSEL_NEGSEL_APORT4XCH7 << 8)  /**< Shifted mode APORT4XCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH8                (_ACMP_INPUTSEL_NEGSEL_APORT4YCH8 << 8)  /**< Shifted mode APORT4YCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH9                (_ACMP_INPUTSEL_NEGSEL_APORT4XCH9 << 8)  /**< Shifted mode APORT4XCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH10               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH10 << 8) /**< Shifted mode APORT4YCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH11               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH11 << 8) /**< Shifted mode APORT4XCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH12               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH12 << 8) /**< Shifted mode APORT4YCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH13               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH13 << 8) /**< Shifted mode APORT4XCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH16               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH16 << 8) /**< Shifted mode APORT4YCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH17               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH17 << 8) /**< Shifted mode APORT4XCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH18               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH18 << 8) /**< Shifted mode APORT4YCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH19               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH19 << 8) /**< Shifted mode APORT4XCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH20               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH20 << 8) /**< Shifted mode APORT4YCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH21               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH21 << 8) /**< Shifted mode APORT4XCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH22               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH22 << 8) /**< Shifted mode APORT4YCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH23               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH23 << 8) /**< Shifted mode APORT4XCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH24               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH24 << 8) /**< Shifted mode APORT4YCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH25               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH25 << 8) /**< Shifted mode APORT4XCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH26               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH26 << 8) /**< Shifted mode APORT4YCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH27               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH27 << 8) /**< Shifted mode APORT4XCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH28               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH28 << 8) /**< Shifted mode APORT4YCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH29               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH29 << 8) /**< Shifted mode APORT4XCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH30               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH30 << 8) /**< Shifted mode APORT4YCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4YCH14               (_ACMP_INPUTSEL_NEGSEL_APORT4YCH14 << 8) /**< Shifted mode APORT4YCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH15               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH15 << 8) /**< Shifted mode APORT4XCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_APORT4XCH31               (_ACMP_INPUTSEL_NEGSEL_APORT4XCH31 << 8) /**< Shifted mode APORT4XCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_VLP                       (_ACMP_INPUTSEL_NEGSEL_VLP << 8)         /**< Shifted mode VLP for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_VBDIV                     (_ACMP_INPUTSEL_NEGSEL_VBDIV << 8)       /**< Shifted mode VBDIV for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_VADIV                     (_ACMP_INPUTSEL_NEGSEL_VADIV << 8)       /**< Shifted mode VADIV for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_VDD                       (_ACMP_INPUTSEL_NEGSEL_VDD << 8)         /**< Shifted mode VDD for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_NEGSEL_VSS                       (_ACMP_INPUTSEL_NEGSEL_VSS << 8)         /**< Shifted mode VSS for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_SHIFT                     16                                       /**< Shift value for ACMP_VASEL */
#define _ACMP_INPUTSEL_VASEL_MASK                      0x3F0000UL                               /**< Bit mask for ACMP_VASEL */
#define _ACMP_INPUTSEL_VASEL_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_VDD                       0x00000000UL                             /**< Mode VDD for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH0                0x00000001UL                             /**< Mode APORT2YCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH2                0x00000003UL                             /**< Mode APORT2YCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH4                0x00000005UL                             /**< Mode APORT2YCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH6                0x00000007UL                             /**< Mode APORT2YCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH8                0x00000009UL                             /**< Mode APORT2YCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH10               0x0000000BUL                             /**< Mode APORT2YCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH12               0x0000000DUL                             /**< Mode APORT2YCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH14               0x0000000FUL                             /**< Mode APORT2YCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH16               0x00000011UL                             /**< Mode APORT2YCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH18               0x00000013UL                             /**< Mode APORT2YCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH20               0x00000015UL                             /**< Mode APORT2YCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH22               0x00000017UL                             /**< Mode APORT2YCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH24               0x00000019UL                             /**< Mode APORT2YCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH26               0x0000001BUL                             /**< Mode APORT2YCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH28               0x0000001DUL                             /**< Mode APORT2YCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT2YCH30               0x0000001FUL                             /**< Mode APORT2YCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH0                0x00000020UL                             /**< Mode APORT1XCH0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH1                0x00000021UL                             /**< Mode APORT1YCH1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH2                0x00000022UL                             /**< Mode APORT1XCH2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH3                0x00000023UL                             /**< Mode APORT1YCH3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH4                0x00000024UL                             /**< Mode APORT1XCH4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH5                0x00000025UL                             /**< Mode APORT1YCH5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH6                0x00000026UL                             /**< Mode APORT1XCH6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH7                0x00000027UL                             /**< Mode APORT1YCH7 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH8                0x00000028UL                             /**< Mode APORT1XCH8 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH9                0x00000029UL                             /**< Mode APORT1YCH9 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH10               0x0000002AUL                             /**< Mode APORT1XCH10 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH11               0x0000002BUL                             /**< Mode APORT1YCH11 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH12               0x0000002CUL                             /**< Mode APORT1XCH12 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH13               0x0000002DUL                             /**< Mode APORT1YCH13 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH14               0x0000002EUL                             /**< Mode APORT1XCH14 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH15               0x0000002FUL                             /**< Mode APORT1YCH15 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH16               0x00000030UL                             /**< Mode APORT1XCH16 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH17               0x00000031UL                             /**< Mode APORT1YCH17 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH18               0x00000032UL                             /**< Mode APORT1XCH18 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH19               0x00000033UL                             /**< Mode APORT1YCH19 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH20               0x00000034UL                             /**< Mode APORT1XCH20 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH21               0x00000035UL                             /**< Mode APORT1YCH21 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH22               0x00000036UL                             /**< Mode APORT1XCH22 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH23               0x00000037UL                             /**< Mode APORT1YCH23 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH24               0x00000038UL                             /**< Mode APORT1XCH24 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH25               0x00000039UL                             /**< Mode APORT1YCH25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH26               0x0000003AUL                             /**< Mode APORT1XCH26 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH27               0x0000003BUL                             /**< Mode APORT1YCH27 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH28               0x0000003CUL                             /**< Mode APORT1XCH28 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH29               0x0000003DUL                             /**< Mode APORT1YCH29 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1XCH30               0x0000003EUL                             /**< Mode APORT1XCH30 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VASEL_APORT1YCH31               0x0000003FUL                             /**< Mode APORT1YCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_DEFAULT                    (_ACMP_INPUTSEL_VASEL_DEFAULT << 16)     /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_VDD                        (_ACMP_INPUTSEL_VASEL_VDD << 16)         /**< Shifted mode VDD for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH0                 (_ACMP_INPUTSEL_VASEL_APORT2YCH0 << 16)  /**< Shifted mode APORT2YCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH2                 (_ACMP_INPUTSEL_VASEL_APORT2YCH2 << 16)  /**< Shifted mode APORT2YCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH4                 (_ACMP_INPUTSEL_VASEL_APORT2YCH4 << 16)  /**< Shifted mode APORT2YCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH6                 (_ACMP_INPUTSEL_VASEL_APORT2YCH6 << 16)  /**< Shifted mode APORT2YCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH8                 (_ACMP_INPUTSEL_VASEL_APORT2YCH8 << 16)  /**< Shifted mode APORT2YCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH10                (_ACMP_INPUTSEL_VASEL_APORT2YCH10 << 16) /**< Shifted mode APORT2YCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH12                (_ACMP_INPUTSEL_VASEL_APORT2YCH12 << 16) /**< Shifted mode APORT2YCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH14                (_ACMP_INPUTSEL_VASEL_APORT2YCH14 << 16) /**< Shifted mode APORT2YCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH16                (_ACMP_INPUTSEL_VASEL_APORT2YCH16 << 16) /**< Shifted mode APORT2YCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH18                (_ACMP_INPUTSEL_VASEL_APORT2YCH18 << 16) /**< Shifted mode APORT2YCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH20                (_ACMP_INPUTSEL_VASEL_APORT2YCH20 << 16) /**< Shifted mode APORT2YCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH22                (_ACMP_INPUTSEL_VASEL_APORT2YCH22 << 16) /**< Shifted mode APORT2YCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH24                (_ACMP_INPUTSEL_VASEL_APORT2YCH24 << 16) /**< Shifted mode APORT2YCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH26                (_ACMP_INPUTSEL_VASEL_APORT2YCH26 << 16) /**< Shifted mode APORT2YCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH28                (_ACMP_INPUTSEL_VASEL_APORT2YCH28 << 16) /**< Shifted mode APORT2YCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT2YCH30                (_ACMP_INPUTSEL_VASEL_APORT2YCH30 << 16) /**< Shifted mode APORT2YCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH0                 (_ACMP_INPUTSEL_VASEL_APORT1XCH0 << 16)  /**< Shifted mode APORT1XCH0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH1                 (_ACMP_INPUTSEL_VASEL_APORT1YCH1 << 16)  /**< Shifted mode APORT1YCH1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH2                 (_ACMP_INPUTSEL_VASEL_APORT1XCH2 << 16)  /**< Shifted mode APORT1XCH2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH3                 (_ACMP_INPUTSEL_VASEL_APORT1YCH3 << 16)  /**< Shifted mode APORT1YCH3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH4                 (_ACMP_INPUTSEL_VASEL_APORT1XCH4 << 16)  /**< Shifted mode APORT1XCH4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH5                 (_ACMP_INPUTSEL_VASEL_APORT1YCH5 << 16)  /**< Shifted mode APORT1YCH5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH6                 (_ACMP_INPUTSEL_VASEL_APORT1XCH6 << 16)  /**< Shifted mode APORT1XCH6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH7                 (_ACMP_INPUTSEL_VASEL_APORT1YCH7 << 16)  /**< Shifted mode APORT1YCH7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH8                 (_ACMP_INPUTSEL_VASEL_APORT1XCH8 << 16)  /**< Shifted mode APORT1XCH8 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH9                 (_ACMP_INPUTSEL_VASEL_APORT1YCH9 << 16)  /**< Shifted mode APORT1YCH9 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH10                (_ACMP_INPUTSEL_VASEL_APORT1XCH10 << 16) /**< Shifted mode APORT1XCH10 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH11                (_ACMP_INPUTSEL_VASEL_APORT1YCH11 << 16) /**< Shifted mode APORT1YCH11 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH12                (_ACMP_INPUTSEL_VASEL_APORT1XCH12 << 16) /**< Shifted mode APORT1XCH12 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH13                (_ACMP_INPUTSEL_VASEL_APORT1YCH13 << 16) /**< Shifted mode APORT1YCH13 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH14                (_ACMP_INPUTSEL_VASEL_APORT1XCH14 << 16) /**< Shifted mode APORT1XCH14 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH15                (_ACMP_INPUTSEL_VASEL_APORT1YCH15 << 16) /**< Shifted mode APORT1YCH15 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH16                (_ACMP_INPUTSEL_VASEL_APORT1XCH16 << 16) /**< Shifted mode APORT1XCH16 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH17                (_ACMP_INPUTSEL_VASEL_APORT1YCH17 << 16) /**< Shifted mode APORT1YCH17 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH18                (_ACMP_INPUTSEL_VASEL_APORT1XCH18 << 16) /**< Shifted mode APORT1XCH18 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH19                (_ACMP_INPUTSEL_VASEL_APORT1YCH19 << 16) /**< Shifted mode APORT1YCH19 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH20                (_ACMP_INPUTSEL_VASEL_APORT1XCH20 << 16) /**< Shifted mode APORT1XCH20 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH21                (_ACMP_INPUTSEL_VASEL_APORT1YCH21 << 16) /**< Shifted mode APORT1YCH21 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH22                (_ACMP_INPUTSEL_VASEL_APORT1XCH22 << 16) /**< Shifted mode APORT1XCH22 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH23                (_ACMP_INPUTSEL_VASEL_APORT1YCH23 << 16) /**< Shifted mode APORT1YCH23 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH24                (_ACMP_INPUTSEL_VASEL_APORT1XCH24 << 16) /**< Shifted mode APORT1XCH24 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH25                (_ACMP_INPUTSEL_VASEL_APORT1YCH25 << 16) /**< Shifted mode APORT1YCH25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH26                (_ACMP_INPUTSEL_VASEL_APORT1XCH26 << 16) /**< Shifted mode APORT1XCH26 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH27                (_ACMP_INPUTSEL_VASEL_APORT1YCH27 << 16) /**< Shifted mode APORT1YCH27 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH28                (_ACMP_INPUTSEL_VASEL_APORT1XCH28 << 16) /**< Shifted mode APORT1XCH28 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH29                (_ACMP_INPUTSEL_VASEL_APORT1YCH29 << 16) /**< Shifted mode APORT1YCH29 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1XCH30                (_ACMP_INPUTSEL_VASEL_APORT1XCH30 << 16) /**< Shifted mode APORT1XCH30 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VASEL_APORT1YCH31                (_ACMP_INPUTSEL_VASEL_APORT1YCH31 << 16) /**< Shifted mode APORT1YCH31 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VBSEL                            (0x1UL << 22)                            /**< VB Selection */
#define _ACMP_INPUTSEL_VBSEL_SHIFT                     22                                       /**< Shift value for ACMP_VBSEL */
#define _ACMP_INPUTSEL_VBSEL_MASK                      0x400000UL                               /**< Bit mask for ACMP_VBSEL */
#define _ACMP_INPUTSEL_VBSEL_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VBSEL_1V25                      0x00000000UL                             /**< Mode 1V25 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VBSEL_2V5                       0x00000001UL                             /**< Mode 2V5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VBSEL_DEFAULT                    (_ACMP_INPUTSEL_VBSEL_DEFAULT << 22)     /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VBSEL_1V25                       (_ACMP_INPUTSEL_VBSEL_1V25 << 22)        /**< Shifted mode 1V25 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VBSEL_2V5                        (_ACMP_INPUTSEL_VBSEL_2V5 << 22)         /**< Shifted mode 2V5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VLPSEL                           (0x1UL << 24)                            /**< Low-Power Sampled Voltage Selection */
#define _ACMP_INPUTSEL_VLPSEL_SHIFT                    24                                       /**< Shift value for ACMP_VLPSEL */
#define _ACMP_INPUTSEL_VLPSEL_MASK                     0x1000000UL                              /**< Bit mask for ACMP_VLPSEL */
#define _ACMP_INPUTSEL_VLPSEL_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VLPSEL_VADIV                    0x00000000UL                             /**< Mode VADIV for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_VLPSEL_VBDIV                    0x00000001UL                             /**< Mode VBDIV for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VLPSEL_DEFAULT                   (_ACMP_INPUTSEL_VLPSEL_DEFAULT << 24)    /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VLPSEL_VADIV                     (_ACMP_INPUTSEL_VLPSEL_VADIV << 24)      /**< Shifted mode VADIV for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_VLPSEL_VBDIV                     (_ACMP_INPUTSEL_VLPSEL_VBDIV << 24)      /**< Shifted mode VBDIV for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESEN                          (0x1UL << 26)                            /**< Capacitive Sense Mode Internal Resistor Enable */
#define _ACMP_INPUTSEL_CSRESEN_SHIFT                   26                                       /**< Shift value for ACMP_CSRESEN */
#define _ACMP_INPUTSEL_CSRESEN_MASK                    0x4000000UL                              /**< Bit mask for ACMP_CSRESEN */
#define _ACMP_INPUTSEL_CSRESEN_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESEN_DEFAULT                  (_ACMP_INPUTSEL_CSRESEN_DEFAULT << 26)   /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_SHIFT                  28                                       /**< Shift value for ACMP_CSRESSEL */
#define _ACMP_INPUTSEL_CSRESSEL_MASK                   0x70000000UL                             /**< Bit mask for ACMP_CSRESSEL */
#define _ACMP_INPUTSEL_CSRESSEL_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES0                   0x00000000UL                             /**< Mode RES0 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES1                   0x00000001UL                             /**< Mode RES1 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES2                   0x00000002UL                             /**< Mode RES2 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES3                   0x00000003UL                             /**< Mode RES3 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES4                   0x00000004UL                             /**< Mode RES4 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES5                   0x00000005UL                             /**< Mode RES5 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES6                   0x00000006UL                             /**< Mode RES6 for ACMP_INPUTSEL */
#define _ACMP_INPUTSEL_CSRESSEL_RES7                   0x00000007UL                             /**< Mode RES7 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_DEFAULT                 (_ACMP_INPUTSEL_CSRESSEL_DEFAULT << 28)  /**< Shifted mode DEFAULT for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES0                    (_ACMP_INPUTSEL_CSRESSEL_RES0 << 28)     /**< Shifted mode RES0 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES1                    (_ACMP_INPUTSEL_CSRESSEL_RES1 << 28)     /**< Shifted mode RES1 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES2                    (_ACMP_INPUTSEL_CSRESSEL_RES2 << 28)     /**< Shifted mode RES2 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES3                    (_ACMP_INPUTSEL_CSRESSEL_RES3 << 28)     /**< Shifted mode RES3 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES4                    (_ACMP_INPUTSEL_CSRESSEL_RES4 << 28)     /**< Shifted mode RES4 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES5                    (_ACMP_INPUTSEL_CSRESSEL_RES5 << 28)     /**< Shifted mode RES5 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES6                    (_ACMP_INPUTSEL_CSRESSEL_RES6 << 28)     /**< Shifted mode RES6 for ACMP_INPUTSEL */
#define ACMP_INPUTSEL_CSRESSEL_RES7                    (_ACMP_INPUTSEL_CSRESSEL_RES7 << 28)     /**< Shifted mode RES7 for ACMP_INPUTSEL */

/* Bit fields for ACMP STATUS */
#define _ACMP_STATUS_RESETVALUE                        0x00000000UL                              /**< Default value for ACMP_STATUS */
#define _ACMP_STATUS_MASK                              0x00000007UL                              /**< Mask for ACMP_STATUS */
#define ACMP_STATUS_ACMPACT                            (0x1UL << 0)                              /**< Analog Comparator Active */
#define _ACMP_STATUS_ACMPACT_SHIFT                     0                                         /**< Shift value for ACMP_ACMPACT */
#define _ACMP_STATUS_ACMPACT_MASK                      0x1UL                                     /**< Bit mask for ACMP_ACMPACT */
#define _ACMP_STATUS_ACMPACT_DEFAULT                   0x00000000UL                              /**< Mode DEFAULT for ACMP_STATUS */
#define ACMP_STATUS_ACMPACT_DEFAULT                    (_ACMP_STATUS_ACMPACT_DEFAULT << 0)       /**< Shifted mode DEFAULT for ACMP_STATUS */
#define ACMP_STATUS_ACMPOUT                            (0x1UL << 1)                              /**< Analog Comparator Output */
#define _ACMP_STATUS_ACMPOUT_SHIFT                     1                                         /**< Shift value for ACMP_ACMPOUT */
#define _ACMP_STATUS_ACMPOUT_MASK                      0x2UL                                     /**< Bit mask for ACMP_ACMPOUT */
#define _ACMP_STATUS_ACMPOUT_DEFAULT                   0x00000000UL                              /**< Mode DEFAULT for ACMP_STATUS */
#define ACMP_STATUS_ACMPOUT_DEFAULT                    (_ACMP_STATUS_ACMPOUT_DEFAULT << 1)       /**< Shifted mode DEFAULT for ACMP_STATUS */
#define ACMP_STATUS_APORTCONFLICT                      (0x1UL << 2)                              /**< APORT Conflict Output */
#define _ACMP_STATUS_APORTCONFLICT_SHIFT               2                                         /**< Shift value for ACMP_APORTCONFLICT */
#define _ACMP_STATUS_APORTCONFLICT_MASK                0x4UL                                     /**< Bit mask for ACMP_APORTCONFLICT */
#define _ACMP_STATUS_APORTCONFLICT_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for ACMP_STATUS */
#define ACMP_STATUS_APORTCONFLICT_DEFAULT              (_ACMP_STATUS_APORTCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for ACMP_STATUS */

/* Bit fields for ACMP IF */
#define _ACMP_IF_RESETVALUE                            0x00000000UL                          /**< Default value for ACMP_IF */
#define _ACMP_IF_MASK                                  0x00000007UL                          /**< Mask for ACMP_IF */
#define ACMP_IF_EDGE                                   (0x1UL << 0)                          /**< Edge Triggered Interrupt Flag */
#define _ACMP_IF_EDGE_SHIFT                            0                                     /**< Shift value for ACMP_EDGE */
#define _ACMP_IF_EDGE_MASK                             0x1UL                                 /**< Bit mask for ACMP_EDGE */
#define _ACMP_IF_EDGE_DEFAULT                          0x00000000UL                          /**< Mode DEFAULT for ACMP_IF */
#define ACMP_IF_EDGE_DEFAULT                           (_ACMP_IF_EDGE_DEFAULT << 0)          /**< Shifted mode DEFAULT for ACMP_IF */
#define ACMP_IF_WARMUP                                 (0x1UL << 1)                          /**< Warm-up Interrupt Flag */
#define _ACMP_IF_WARMUP_SHIFT                          1                                     /**< Shift value for ACMP_WARMUP */
#define _ACMP_IF_WARMUP_MASK                           0x2UL                                 /**< Bit mask for ACMP_WARMUP */
#define _ACMP_IF_WARMUP_DEFAULT                        0x00000000UL                          /**< Mode DEFAULT for ACMP_IF */
#define ACMP_IF_WARMUP_DEFAULT                         (_ACMP_IF_WARMUP_DEFAULT << 1)        /**< Shifted mode DEFAULT for ACMP_IF */
#define ACMP_IF_APORTCONFLICT                          (0x1UL << 2)                          /**< APORT Conflict Interrupt Flag */
#define _ACMP_IF_APORTCONFLICT_SHIFT                   2                                     /**< Shift value for ACMP_APORTCONFLICT */
#define _ACMP_IF_APORTCONFLICT_MASK                    0x4UL                                 /**< Bit mask for ACMP_APORTCONFLICT */
#define _ACMP_IF_APORTCONFLICT_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for ACMP_IF */
#define ACMP_IF_APORTCONFLICT_DEFAULT                  (_ACMP_IF_APORTCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for ACMP_IF */

/* Bit fields for ACMP IFS */
#define _ACMP_IFS_RESETVALUE                           0x00000000UL                           /**< Default value for ACMP_IFS */
#define _ACMP_IFS_MASK                                 0x00000007UL                           /**< Mask for ACMP_IFS */
#define ACMP_IFS_EDGE                                  (0x1UL << 0)                           /**< Set EDGE Interrupt Flag */
#define _ACMP_IFS_EDGE_SHIFT                           0                                      /**< Shift value for ACMP_EDGE */
#define _ACMP_IFS_EDGE_MASK                            0x1UL                                  /**< Bit mask for ACMP_EDGE */
#define _ACMP_IFS_EDGE_DEFAULT                         0x00000000UL                           /**< Mode DEFAULT for ACMP_IFS */
#define ACMP_IFS_EDGE_DEFAULT                          (_ACMP_IFS_EDGE_DEFAULT << 0)          /**< Shifted mode DEFAULT for ACMP_IFS */
#define ACMP_IFS_WARMUP                                (0x1UL << 1)                           /**< Set WARMUP Interrupt Flag */
#define _ACMP_IFS_WARMUP_SHIFT                         1                                      /**< Shift value for ACMP_WARMUP */
#define _ACMP_IFS_WARMUP_MASK                          0x2UL                                  /**< Bit mask for ACMP_WARMUP */
#define _ACMP_IFS_WARMUP_DEFAULT                       0x00000000UL                           /**< Mode DEFAULT for ACMP_IFS */
#define ACMP_IFS_WARMUP_DEFAULT                        (_ACMP_IFS_WARMUP_DEFAULT << 1)        /**< Shifted mode DEFAULT for ACMP_IFS */
#define ACMP_IFS_APORTCONFLICT                         (0x1UL << 2)                           /**< Set APORTCONFLICT Interrupt Flag */
#define _ACMP_IFS_APORTCONFLICT_SHIFT                  2                                      /**< Shift value for ACMP_APORTCONFLICT */
#define _ACMP_IFS_APORTCONFLICT_MASK                   0x4UL                                  /**< Bit mask for ACMP_APORTCONFLICT */
#define _ACMP_IFS_APORTCONFLICT_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ACMP_IFS */
#define ACMP_IFS_APORTCONFLICT_DEFAULT                 (_ACMP_IFS_APORTCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for ACMP_IFS */

/* Bit fields for ACMP IFC */
#define _ACMP_IFC_RESETVALUE                           0x00000000UL                           /**< Default value for ACMP_IFC */
#define _ACMP_IFC_MASK                                 0x00000007UL                           /**< Mask for ACMP_IFC */
#define ACMP_IFC_EDGE                                  (0x1UL << 0)                           /**< Clear EDGE Interrupt Flag */
#define _ACMP_IFC_EDGE_SHIFT                           0                                      /**< Shift value for ACMP_EDGE */
#define _ACMP_IFC_EDGE_MASK                            0x1UL                                  /**< Bit mask for ACMP_EDGE */
#define _ACMP_IFC_EDGE_DEFAULT                         0x00000000UL                           /**< Mode DEFAULT for ACMP_IFC */
#define ACMP_IFC_EDGE_DEFAULT                          (_ACMP_IFC_EDGE_DEFAULT << 0)          /**< Shifted mode DEFAULT for ACMP_IFC */
#define ACMP_IFC_WARMUP                                (0x1UL << 1)                           /**< Clear WARMUP Interrupt Flag */
#define _ACMP_IFC_WARMUP_SHIFT                         1                                      /**< Shift value for ACMP_WARMUP */
#define _ACMP_IFC_WARMUP_MASK                          0x2UL                                  /**< Bit mask for ACMP_WARMUP */
#define _ACMP_IFC_WARMUP_DEFAULT                       0x00000000UL                           /**< Mode DEFAULT for ACMP_IFC */
#define ACMP_IFC_WARMUP_DEFAULT                        (_ACMP_IFC_WARMUP_DEFAULT << 1)        /**< Shifted mode DEFAULT for ACMP_IFC */
#define ACMP_IFC_APORTCONFLICT                         (0x1UL << 2)                           /**< Clear APORTCONFLICT Interrupt Flag */
#define _ACMP_IFC_APORTCONFLICT_SHIFT                  2                                      /**< Shift value for ACMP_APORTCONFLICT */
#define _ACMP_IFC_APORTCONFLICT_MASK                   0x4UL                                  /**< Bit mask for ACMP_APORTCONFLICT */
#define _ACMP_IFC_APORTCONFLICT_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ACMP_IFC */
#define ACMP_IFC_APORTCONFLICT_DEFAULT                 (_ACMP_IFC_APORTCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for ACMP_IFC */

/* Bit fields for ACMP IEN */
#define _ACMP_IEN_RESETVALUE                           0x00000000UL                           /**< Default value for ACMP_IEN */
#define _ACMP_IEN_MASK                                 0x00000007UL                           /**< Mask for ACMP_IEN */
#define ACMP_IEN_EDGE                                  (0x1UL << 0)                           /**< EDGE Interrupt Enable */
#define _ACMP_IEN_EDGE_SHIFT                           0                                      /**< Shift value for ACMP_EDGE */
#define _ACMP_IEN_EDGE_MASK                            0x1UL                                  /**< Bit mask for ACMP_EDGE */
#define _ACMP_IEN_EDGE_DEFAULT                         0x00000000UL                           /**< Mode DEFAULT for ACMP_IEN */
#define ACMP_IEN_EDGE_DEFAULT                          (_ACMP_IEN_EDGE_DEFAULT << 0)          /**< Shifted mode DEFAULT for ACMP_IEN */
#define ACMP_IEN_WARMUP                                (0x1UL << 1)                           /**< WARMUP Interrupt Enable */
#define _ACMP_IEN_WARMUP_SHIFT                         1                                      /**< Shift value for ACMP_WARMUP */
#define _ACMP_IEN_WARMUP_MASK                          0x2UL                                  /**< Bit mask for ACMP_WARMUP */
#define _ACMP_IEN_WARMUP_DEFAULT                       0x00000000UL                           /**< Mode DEFAULT for ACMP_IEN */
#define ACMP_IEN_WARMUP_DEFAULT                        (_ACMP_IEN_WARMUP_DEFAULT << 1)        /**< Shifted mode DEFAULT for ACMP_IEN */
#define ACMP_IEN_APORTCONFLICT                         (0x1UL << 2)                           /**< APORTCONFLICT Interrupt Enable */
#define _ACMP_IEN_APORTCONFLICT_SHIFT                  2                                      /**< Shift value for ACMP_APORTCONFLICT */
#define _ACMP_IEN_APORTCONFLICT_MASK                   0x4UL                                  /**< Bit mask for ACMP_APORTCONFLICT */
#define _ACMP_IEN_APORTCONFLICT_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ACMP_IEN */
#define ACMP_IEN_APORTCONFLICT_DEFAULT                 (_ACMP_IEN_APORTCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for ACMP_IEN */

/* Bit fields for ACMP APORTREQ */
#define _ACMP_APORTREQ_RESETVALUE                      0x00000000UL                             /**< Default value for ACMP_APORTREQ */
#define _ACMP_APORTREQ_MASK                            0x000003FFUL                             /**< Mask for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT0XREQ                       (0x1UL << 0)                             /**< 1 if the bus connected to APORT0X is requested */
#define _ACMP_APORTREQ_APORT0XREQ_SHIFT                0                                        /**< Shift value for ACMP_APORT0XREQ */
#define _ACMP_APORTREQ_APORT0XREQ_MASK                 0x1UL                                    /**< Bit mask for ACMP_APORT0XREQ */
#define _ACMP_APORTREQ_APORT0XREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT0XREQ_DEFAULT               (_ACMP_APORTREQ_APORT0XREQ_DEFAULT << 0) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT0YREQ                       (0x1UL << 1)                             /**< 1 if the bus connected to APORT0Y is requested */
#define _ACMP_APORTREQ_APORT0YREQ_SHIFT                1                                        /**< Shift value for ACMP_APORT0YREQ */
#define _ACMP_APORTREQ_APORT0YREQ_MASK                 0x2UL                                    /**< Bit mask for ACMP_APORT0YREQ */
#define _ACMP_APORTREQ_APORT0YREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT0YREQ_DEFAULT               (_ACMP_APORTREQ_APORT0YREQ_DEFAULT << 1) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT1XREQ                       (0x1UL << 2)                             /**< 1 if the bus connected to APORT2X is requested */
#define _ACMP_APORTREQ_APORT1XREQ_SHIFT                2                                        /**< Shift value for ACMP_APORT1XREQ */
#define _ACMP_APORTREQ_APORT1XREQ_MASK                 0x4UL                                    /**< Bit mask for ACMP_APORT1XREQ */
#define _ACMP_APORTREQ_APORT1XREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT1XREQ_DEFAULT               (_ACMP_APORTREQ_APORT1XREQ_DEFAULT << 2) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT1YREQ                       (0x1UL << 3)                             /**< 1 if the bus connected to APORT1X is requested */
#define _ACMP_APORTREQ_APORT1YREQ_SHIFT                3                                        /**< Shift value for ACMP_APORT1YREQ */
#define _ACMP_APORTREQ_APORT1YREQ_MASK                 0x8UL                                    /**< Bit mask for ACMP_APORT1YREQ */
#define _ACMP_APORTREQ_APORT1YREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT1YREQ_DEFAULT               (_ACMP_APORTREQ_APORT1YREQ_DEFAULT << 3) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT2XREQ                       (0x1UL << 4)                             /**< 1 if the bus connected to APORT2X is requested */
#define _ACMP_APORTREQ_APORT2XREQ_SHIFT                4                                        /**< Shift value for ACMP_APORT2XREQ */
#define _ACMP_APORTREQ_APORT2XREQ_MASK                 0x10UL                                   /**< Bit mask for ACMP_APORT2XREQ */
#define _ACMP_APORTREQ_APORT2XREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT2XREQ_DEFAULT               (_ACMP_APORTREQ_APORT2XREQ_DEFAULT << 4) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT2YREQ                       (0x1UL << 5)                             /**< 1 if the bus connected to APORT2Y is requested */
#define _ACMP_APORTREQ_APORT2YREQ_SHIFT                5                                        /**< Shift value for ACMP_APORT2YREQ */
#define _ACMP_APORTREQ_APORT2YREQ_MASK                 0x20UL                                   /**< Bit mask for ACMP_APORT2YREQ */
#define _ACMP_APORTREQ_APORT2YREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT2YREQ_DEFAULT               (_ACMP_APORTREQ_APORT2YREQ_DEFAULT << 5) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT3XREQ                       (0x1UL << 6)                             /**< 1 if the bus connected to APORT3X is requested */
#define _ACMP_APORTREQ_APORT3XREQ_SHIFT                6                                        /**< Shift value for ACMP_APORT3XREQ */
#define _ACMP_APORTREQ_APORT3XREQ_MASK                 0x40UL                                   /**< Bit mask for ACMP_APORT3XREQ */
#define _ACMP_APORTREQ_APORT3XREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT3XREQ_DEFAULT               (_ACMP_APORTREQ_APORT3XREQ_DEFAULT << 6) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT3YREQ                       (0x1UL << 7)                             /**< 1 if the bus connected to APORT3Y is requested */
#define _ACMP_APORTREQ_APORT3YREQ_SHIFT                7                                        /**< Shift value for ACMP_APORT3YREQ */
#define _ACMP_APORTREQ_APORT3YREQ_MASK                 0x80UL                                   /**< Bit mask for ACMP_APORT3YREQ */
#define _ACMP_APORTREQ_APORT3YREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT3YREQ_DEFAULT               (_ACMP_APORTREQ_APORT3YREQ_DEFAULT << 7) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT4XREQ                       (0x1UL << 8)                             /**< 1 if the bus connected to APORT4X is requested */
#define _ACMP_APORTREQ_APORT4XREQ_SHIFT                8                                        /**< Shift value for ACMP_APORT4XREQ */
#define _ACMP_APORTREQ_APORT4XREQ_MASK                 0x100UL                                  /**< Bit mask for ACMP_APORT4XREQ */
#define _ACMP_APORTREQ_APORT4XREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT4XREQ_DEFAULT               (_ACMP_APORTREQ_APORT4XREQ_DEFAULT << 8) /**< Shifted mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT4YREQ                       (0x1UL << 9)                             /**< 1 if the bus connected to APORT4Y is requested */
#define _ACMP_APORTREQ_APORT4YREQ_SHIFT                9                                        /**< Shift value for ACMP_APORT4YREQ */
#define _ACMP_APORTREQ_APORT4YREQ_MASK                 0x200UL                                  /**< Bit mask for ACMP_APORT4YREQ */
#define _ACMP_APORTREQ_APORT4YREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for ACMP_APORTREQ */
#define ACMP_APORTREQ_APORT4YREQ_DEFAULT               (_ACMP_APORTREQ_APORT4YREQ_DEFAULT << 9) /**< Shifted mode DEFAULT for ACMP_APORTREQ */

/* Bit fields for ACMP APORTCONFLICT */
#define _ACMP_APORTCONFLICT_RESETVALUE                 0x00000000UL                                       /**< Default value for ACMP_APORTCONFLICT */
#define _ACMP_APORTCONFLICT_MASK                       0x000003FFUL                                       /**< Mask for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT0XCONFLICT             (0x1UL << 0)                                       /**< 1 if the bus connected to APORT0X is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT0XCONFLICT_SHIFT      0                                                  /**< Shift value for ACMP_APORT0XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT0XCONFLICT_MASK       0x1UL                                              /**< Bit mask for ACMP_APORT0XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT0XCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT0XCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT0XCONFLICT_DEFAULT << 0) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT0YCONFLICT             (0x1UL << 1)                                       /**< 1 if the bus connected to APORT0Y is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT0YCONFLICT_SHIFT      1                                                  /**< Shift value for ACMP_APORT0YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT0YCONFLICT_MASK       0x2UL                                              /**< Bit mask for ACMP_APORT0YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT0YCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT0YCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT0YCONFLICT_DEFAULT << 1) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT1XCONFLICT             (0x1UL << 2)                                       /**< 1 if the bus connected to APORT1X is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT1XCONFLICT_SHIFT      2                                                  /**< Shift value for ACMP_APORT1XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT1XCONFLICT_MASK       0x4UL                                              /**< Bit mask for ACMP_APORT1XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT1XCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT1XCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT1XCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT1YCONFLICT             (0x1UL << 3)                                       /**< 1 if the bus connected to APORT1X is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT1YCONFLICT_SHIFT      3                                                  /**< Shift value for ACMP_APORT1YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT1YCONFLICT_MASK       0x8UL                                              /**< Bit mask for ACMP_APORT1YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT1YCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT1YCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT1YCONFLICT_DEFAULT << 3) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT2XCONFLICT             (0x1UL << 4)                                       /**< 1 if the bus connected to APORT2X is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT2XCONFLICT_SHIFT      4                                                  /**< Shift value for ACMP_APORT2XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT2XCONFLICT_MASK       0x10UL                                             /**< Bit mask for ACMP_APORT2XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT2XCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT2XCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT2XCONFLICT_DEFAULT << 4) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT2YCONFLICT             (0x1UL << 5)                                       /**< 1 if the bus connected to APORT2Y is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT2YCONFLICT_SHIFT      5                                                  /**< Shift value for ACMP_APORT2YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT2YCONFLICT_MASK       0x20UL                                             /**< Bit mask for ACMP_APORT2YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT2YCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT2YCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT2YCONFLICT_DEFAULT << 5) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT3XCONFLICT             (0x1UL << 6)                                       /**< 1 if the bus connected to APORT3X is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT3XCONFLICT_SHIFT      6                                                  /**< Shift value for ACMP_APORT3XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT3XCONFLICT_MASK       0x40UL                                             /**< Bit mask for ACMP_APORT3XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT3XCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT3XCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT3XCONFLICT_DEFAULT << 6) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT3YCONFLICT             (0x1UL << 7)                                       /**< 1 if the bus connected to APORT3Y is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT3YCONFLICT_SHIFT      7                                                  /**< Shift value for ACMP_APORT3YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT3YCONFLICT_MASK       0x80UL                                             /**< Bit mask for ACMP_APORT3YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT3YCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT3YCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT3YCONFLICT_DEFAULT << 7) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT4XCONFLICT             (0x1UL << 8)                                       /**< 1 if the bus connected to APORT4X is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT4XCONFLICT_SHIFT      8                                                  /**< Shift value for ACMP_APORT4XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT4XCONFLICT_MASK       0x100UL                                            /**< Bit mask for ACMP_APORT4XCONFLICT */
#define _ACMP_APORTCONFLICT_APORT4XCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT4XCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT4XCONFLICT_DEFAULT << 8) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT4YCONFLICT             (0x1UL << 9)                                       /**< 1 if the bus connected to APORT4Y is in conflict with another peripheral */
#define _ACMP_APORTCONFLICT_APORT4YCONFLICT_SHIFT      9                                                  /**< Shift value for ACMP_APORT4YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT4YCONFLICT_MASK       0x200UL                                            /**< Bit mask for ACMP_APORT4YCONFLICT */
#define _ACMP_APORTCONFLICT_APORT4YCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for ACMP_APORTCONFLICT */
#define ACMP_APORTCONFLICT_APORT4YCONFLICT_DEFAULT     (_ACMP_APORTCONFLICT_APORT4YCONFLICT_DEFAULT << 9) /**< Shifted mode DEFAULT for ACMP_APORTCONFLICT */

/* Bit fields for ACMP HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_RESETVALUE                   0x00000000UL                            /**< Default value for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_MASK                         0x3F3F000FUL                            /**< Mask for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_SHIFT                   0                                       /**< Shift value for ACMP_HYST */
#define _ACMP_HYSTERESIS0_HYST_MASK                    0xFUL                                   /**< Bit mask for ACMP_HYST */
#define _ACMP_HYSTERESIS0_HYST_DEFAULT                 0x00000000UL                            /**< Mode DEFAULT for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST0                   0x00000000UL                            /**< Mode HYST0 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST1                   0x00000001UL                            /**< Mode HYST1 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST2                   0x00000002UL                            /**< Mode HYST2 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST3                   0x00000003UL                            /**< Mode HYST3 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST4                   0x00000004UL                            /**< Mode HYST4 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST5                   0x00000005UL                            /**< Mode HYST5 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST6                   0x00000006UL                            /**< Mode HYST6 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST7                   0x00000007UL                            /**< Mode HYST7 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST8                   0x00000008UL                            /**< Mode HYST8 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST9                   0x00000009UL                            /**< Mode HYST9 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST10                  0x0000000AUL                            /**< Mode HYST10 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST11                  0x0000000BUL                            /**< Mode HYST11 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST12                  0x0000000CUL                            /**< Mode HYST12 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST13                  0x0000000DUL                            /**< Mode HYST13 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST14                  0x0000000EUL                            /**< Mode HYST14 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_HYST_HYST15                  0x0000000FUL                            /**< Mode HYST15 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_DEFAULT                  (_ACMP_HYSTERESIS0_HYST_DEFAULT << 0)   /**< Shifted mode DEFAULT for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST0                    (_ACMP_HYSTERESIS0_HYST_HYST0 << 0)     /**< Shifted mode HYST0 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST1                    (_ACMP_HYSTERESIS0_HYST_HYST1 << 0)     /**< Shifted mode HYST1 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST2                    (_ACMP_HYSTERESIS0_HYST_HYST2 << 0)     /**< Shifted mode HYST2 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST3                    (_ACMP_HYSTERESIS0_HYST_HYST3 << 0)     /**< Shifted mode HYST3 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST4                    (_ACMP_HYSTERESIS0_HYST_HYST4 << 0)     /**< Shifted mode HYST4 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST5                    (_ACMP_HYSTERESIS0_HYST_HYST5 << 0)     /**< Shifted mode HYST5 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST6                    (_ACMP_HYSTERESIS0_HYST_HYST6 << 0)     /**< Shifted mode HYST6 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST7                    (_ACMP_HYSTERESIS0_HYST_HYST7 << 0)     /**< Shifted mode HYST7 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST8                    (_ACMP_HYSTERESIS0_HYST_HYST8 << 0)     /**< Shifted mode HYST8 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST9                    (_ACMP_HYSTERESIS0_HYST_HYST9 << 0)     /**< Shifted mode HYST9 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST10                   (_ACMP_HYSTERESIS0_HYST_HYST10 << 0)    /**< Shifted mode HYST10 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST11                   (_ACMP_HYSTERESIS0_HYST_HYST11 << 0)    /**< Shifted mode HYST11 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST12                   (_ACMP_HYSTERESIS0_HYST_HYST12 << 0)    /**< Shifted mode HYST12 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST13                   (_ACMP_HYSTERESIS0_HYST_HYST13 << 0)    /**< Shifted mode HYST13 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST14                   (_ACMP_HYSTERESIS0_HYST_HYST14 << 0)    /**< Shifted mode HYST14 for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_HYST_HYST15                   (_ACMP_HYSTERESIS0_HYST_HYST15 << 0)    /**< Shifted mode HYST15 for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_DIVVA_SHIFT                  16                                      /**< Shift value for ACMP_DIVVA */
#define _ACMP_HYSTERESIS0_DIVVA_MASK                   0x3F0000UL                              /**< Bit mask for ACMP_DIVVA */
#define _ACMP_HYSTERESIS0_DIVVA_DEFAULT                0x00000000UL                            /**< Mode DEFAULT for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_DIVVA_DEFAULT                 (_ACMP_HYSTERESIS0_DIVVA_DEFAULT << 16) /**< Shifted mode DEFAULT for ACMP_HYSTERESIS0 */
#define _ACMP_HYSTERESIS0_DIVVB_SHIFT                  24                                      /**< Shift value for ACMP_DIVVB */
#define _ACMP_HYSTERESIS0_DIVVB_MASK                   0x3F000000UL                            /**< Bit mask for ACMP_DIVVB */
#define _ACMP_HYSTERESIS0_DIVVB_DEFAULT                0x00000000UL                            /**< Mode DEFAULT for ACMP_HYSTERESIS0 */
#define ACMP_HYSTERESIS0_DIVVB_DEFAULT                 (_ACMP_HYSTERESIS0_DIVVB_DEFAULT << 24) /**< Shifted mode DEFAULT for ACMP_HYSTERESIS0 */

/* Bit fields for ACMP HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_RESETVALUE                   0x00000000UL                            /**< Default value for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_MASK                         0x3F3F000FUL                            /**< Mask for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_SHIFT                   0                                       /**< Shift value for ACMP_HYST */
#define _ACMP_HYSTERESIS1_HYST_MASK                    0xFUL                                   /**< Bit mask for ACMP_HYST */
#define _ACMP_HYSTERESIS1_HYST_DEFAULT                 0x00000000UL                            /**< Mode DEFAULT for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST0                   0x00000000UL                            /**< Mode HYST0 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST1                   0x00000001UL                            /**< Mode HYST1 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST2                   0x00000002UL                            /**< Mode HYST2 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST3                   0x00000003UL                            /**< Mode HYST3 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST4                   0x00000004UL                            /**< Mode HYST4 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST5                   0x00000005UL                            /**< Mode HYST5 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST6                   0x00000006UL                            /**< Mode HYST6 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST7                   0x00000007UL                            /**< Mode HYST7 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST8                   0x00000008UL                            /**< Mode HYST8 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST9                   0x00000009UL                            /**< Mode HYST9 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST10                  0x0000000AUL                            /**< Mode HYST10 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST11                  0x0000000BUL                            /**< Mode HYST11 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST12                  0x0000000CUL                            /**< Mode HYST12 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST13                  0x0000000DUL                            /**< Mode HYST13 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST14                  0x0000000EUL                            /**< Mode HYST14 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_HYST_HYST15                  0x0000000FUL                            /**< Mode HYST15 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_DEFAULT                  (_ACMP_HYSTERESIS1_HYST_DEFAULT << 0)   /**< Shifted mode DEFAULT for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST0                    (_ACMP_HYSTERESIS1_HYST_HYST0 << 0)     /**< Shifted mode HYST0 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST1                    (_ACMP_HYSTERESIS1_HYST_HYST1 << 0)     /**< Shifted mode HYST1 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST2                    (_ACMP_HYSTERESIS1_HYST_HYST2 << 0)     /**< Shifted mode HYST2 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST3                    (_ACMP_HYSTERESIS1_HYST_HYST3 << 0)     /**< Shifted mode HYST3 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST4                    (_ACMP_HYSTERESIS1_HYST_HYST4 << 0)     /**< Shifted mode HYST4 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST5                    (_ACMP_HYSTERESIS1_HYST_HYST5 << 0)     /**< Shifted mode HYST5 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST6                    (_ACMP_HYSTERESIS1_HYST_HYST6 << 0)     /**< Shifted mode HYST6 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST7                    (_ACMP_HYSTERESIS1_HYST_HYST7 << 0)     /**< Shifted mode HYST7 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST8                    (_ACMP_HYSTERESIS1_HYST_HYST8 << 0)     /**< Shifted mode HYST8 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST9                    (_ACMP_HYSTERESIS1_HYST_HYST9 << 0)     /**< Shifted mode HYST9 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST10                   (_ACMP_HYSTERESIS1_HYST_HYST10 << 0)    /**< Shifted mode HYST10 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST11                   (_ACMP_HYSTERESIS1_HYST_HYST11 << 0)    /**< Shifted mode HYST11 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST12                   (_ACMP_HYSTERESIS1_HYST_HYST12 << 0)    /**< Shifted mode HYST12 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST13                   (_ACMP_HYSTERESIS1_HYST_HYST13 << 0)    /**< Shifted mode HYST13 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST14                   (_ACMP_HYSTERESIS1_HYST_HYST14 << 0)    /**< Shifted mode HYST14 for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_HYST_HYST15                   (_ACMP_HYSTERESIS1_HYST_HYST15 << 0)    /**< Shifted mode HYST15 for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_DIVVA_SHIFT                  16                                      /**< Shift value for ACMP_DIVVA */
#define _ACMP_HYSTERESIS1_DIVVA_MASK                   0x3F0000UL                              /**< Bit mask for ACMP_DIVVA */
#define _ACMP_HYSTERESIS1_DIVVA_DEFAULT                0x00000000UL                            /**< Mode DEFAULT for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_DIVVA_DEFAULT                 (_ACMP_HYSTERESIS1_DIVVA_DEFAULT << 16) /**< Shifted mode DEFAULT for ACMP_HYSTERESIS1 */
#define _ACMP_HYSTERESIS1_DIVVB_SHIFT                  24                                      /**< Shift value for ACMP_DIVVB */
#define _ACMP_HYSTERESIS1_DIVVB_MASK                   0x3F000000UL                            /**< Bit mask for ACMP_DIVVB */
#define _ACMP_HYSTERESIS1_DIVVB_DEFAULT                0x00000000UL                            /**< Mode DEFAULT for ACMP_HYSTERESIS1 */
#define ACMP_HYSTERESIS1_DIVVB_DEFAULT                 (_ACMP_HYSTERESIS1_DIVVB_DEFAULT << 24) /**< Shifted mode DEFAULT for ACMP_HYSTERESIS1 */

/* Bit fields for ACMP ROUTEPEN */
#define _ACMP_ROUTEPEN_RESETVALUE                      0x00000000UL                         /**< Default value for ACMP_ROUTEPEN */
#define _ACMP_ROUTEPEN_MASK                            0x00000001UL                         /**< Mask for ACMP_ROUTEPEN */
#define ACMP_ROUTEPEN_OUTPEN                           (0x1UL << 0)                         /**< ACMP Output Pin Enable */
#define _ACMP_ROUTEPEN_OUTPEN_SHIFT                    0                                    /**< Shift value for ACMP_OUTPEN */
#define _ACMP_ROUTEPEN_OUTPEN_MASK                     0x1UL                                /**< Bit mask for ACMP_OUTPEN */
#define _ACMP_ROUTEPEN_OUTPEN_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for ACMP_ROUTEPEN */
#define ACMP_ROUTEPEN_OUTPEN_DEFAULT                   (_ACMP_ROUTEPEN_OUTPEN_DEFAULT << 0) /**< Shifted mode DEFAULT for ACMP_ROUTEPEN */

/* Bit fields for ACMP ROUTELOC0 */
#define _ACMP_ROUTELOC0_RESETVALUE                     0x00000000UL                          /**< Default value for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_MASK                           0x0000001FUL                          /**< Mask for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_SHIFT                   0                                     /**< Shift value for ACMP_OUTLOC */
#define _ACMP_ROUTELOC0_OUTLOC_MASK                    0x1FUL                                /**< Bit mask for ACMP_OUTLOC */
#define _ACMP_ROUTELOC0_OUTLOC_LOC0                    0x00000000UL                          /**< Mode LOC0 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC1                    0x00000001UL                          /**< Mode LOC1 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC2                    0x00000002UL                          /**< Mode LOC2 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC3                    0x00000003UL                          /**< Mode LOC3 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC4                    0x00000004UL                          /**< Mode LOC4 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC5                    0x00000005UL                          /**< Mode LOC5 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC6                    0x00000006UL                          /**< Mode LOC6 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC7                    0x00000007UL                          /**< Mode LOC7 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC8                    0x00000008UL                          /**< Mode LOC8 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC9                    0x00000009UL                          /**< Mode LOC9 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC10                   0x0000000AUL                          /**< Mode LOC10 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC11                   0x0000000BUL                          /**< Mode LOC11 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC12                   0x0000000CUL                          /**< Mode LOC12 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC13                   0x0000000DUL                          /**< Mode LOC13 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC14                   0x0000000EUL                          /**< Mode LOC14 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC15                   0x0000000FUL                          /**< Mode LOC15 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC16                   0x00000010UL                          /**< Mode LOC16 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC17                   0x00000011UL                          /**< Mode LOC17 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC18                   0x00000012UL                          /**< Mode LOC18 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC19                   0x00000013UL                          /**< Mode LOC19 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC20                   0x00000014UL                          /**< Mode LOC20 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC21                   0x00000015UL                          /**< Mode LOC21 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC22                   0x00000016UL                          /**< Mode LOC22 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC23                   0x00000017UL                          /**< Mode LOC23 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC24                   0x00000018UL                          /**< Mode LOC24 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC25                   0x00000019UL                          /**< Mode LOC25 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC26                   0x0000001AUL                          /**< Mode LOC26 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC27                   0x0000001BUL                          /**< Mode LOC27 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC28                   0x0000001CUL                          /**< Mode LOC28 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC29                   0x0000001DUL                          /**< Mode LOC29 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC30                   0x0000001EUL                          /**< Mode LOC30 for ACMP_ROUTELOC0 */
#define _ACMP_ROUTELOC0_OUTLOC_LOC31                   0x0000001FUL                          /**< Mode LOC31 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC0                     (_ACMP_ROUTELOC0_OUTLOC_LOC0 << 0)    /**< Shifted mode LOC0 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_DEFAULT                  (_ACMP_ROUTELOC0_OUTLOC_DEFAULT << 0) /**< Shifted mode DEFAULT for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC1                     (_ACMP_ROUTELOC0_OUTLOC_LOC1 << 0)    /**< Shifted mode LOC1 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC2                     (_ACMP_ROUTELOC0_OUTLOC_LOC2 << 0)    /**< Shifted mode LOC2 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC3                     (_ACMP_ROUTELOC0_OUTLOC_LOC3 << 0)    /**< Shifted mode LOC3 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC4                     (_ACMP_ROUTELOC0_OUTLOC_LOC4 << 0)    /**< Shifted mode LOC4 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC5                     (_ACMP_ROUTELOC0_OUTLOC_LOC5 << 0)    /**< Shifted mode LOC5 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC6                     (_ACMP_ROUTELOC0_OUTLOC_LOC6 << 0)    /**< Shifted mode LOC6 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC7                     (_ACMP_ROUTELOC0_OUTLOC_LOC7 << 0)    /**< Shifted mode LOC7 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC8                     (_ACMP_ROUTELOC0_OUTLOC_LOC8 << 0)    /**< Shifted mode LOC8 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC9                     (_ACMP_ROUTELOC0_OUTLOC_LOC9 << 0)    /**< Shifted mode LOC9 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC10                    (_ACMP_ROUTELOC0_OUTLOC_LOC10 << 0)   /**< Shifted mode LOC10 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC11                    (_ACMP_ROUTELOC0_OUTLOC_LOC11 << 0)   /**< Shifted mode LOC11 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC12                    (_ACMP_ROUTELOC0_OUTLOC_LOC12 << 0)   /**< Shifted mode LOC12 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC13                    (_ACMP_ROUTELOC0_OUTLOC_LOC13 << 0)   /**< Shifted mode LOC13 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC14                    (_ACMP_ROUTELOC0_OUTLOC_LOC14 << 0)   /**< Shifted mode LOC14 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC15                    (_ACMP_ROUTELOC0_OUTLOC_LOC15 << 0)   /**< Shifted mode LOC15 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC16                    (_ACMP_ROUTELOC0_OUTLOC_LOC16 << 0)   /**< Shifted mode LOC16 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC17                    (_ACMP_ROUTELOC0_OUTLOC_LOC17 << 0)   /**< Shifted mode LOC17 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC18                    (_ACMP_ROUTELOC0_OUTLOC_LOC18 << 0)   /**< Shifted mode LOC18 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC19                    (_ACMP_ROUTELOC0_OUTLOC_LOC19 << 0)   /**< Shifted mode LOC19 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC20                    (_ACMP_ROUTELOC0_OUTLOC_LOC20 << 0)   /**< Shifted mode LOC20 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC21                    (_ACMP_ROUTELOC0_OUTLOC_LOC21 << 0)   /**< Shifted mode LOC21 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC22                    (_ACMP_ROUTELOC0_OUTLOC_LOC22 << 0)   /**< Shifted mode LOC22 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC23                    (_ACMP_ROUTELOC0_OUTLOC_LOC23 << 0)   /**< Shifted mode LOC23 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC24                    (_ACMP_ROUTELOC0_OUTLOC_LOC24 << 0)   /**< Shifted mode LOC24 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC25                    (_ACMP_ROUTELOC0_OUTLOC_LOC25 << 0)   /**< Shifted mode LOC25 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC26                    (_ACMP_ROUTELOC0_OUTLOC_LOC26 << 0)   /**< Shifted mode LOC26 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC27                    (_ACMP_ROUTELOC0_OUTLOC_LOC27 << 0)   /**< Shifted mode LOC27 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC28                    (_ACMP_ROUTELOC0_OUTLOC_LOC28 << 0)   /**< Shifted mode LOC28 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC29                    (_ACMP_ROUTELOC0_OUTLOC_LOC29 << 0)   /**< Shifted mode LOC29 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC30                    (_ACMP_ROUTELOC0_OUTLOC_LOC30 << 0)   /**< Shifted mode LOC30 for ACMP_ROUTELOC0 */
#define ACMP_ROUTELOC0_OUTLOC_LOC31                    (_ACMP_ROUTELOC0_OUTLOC_LOC31 << 0)   /**< Shifted mode LOC31 for ACMP_ROUTELOC0 */

/** @} End of group EFR32FG1P_ACMP */
/** @} End of group Parts */

