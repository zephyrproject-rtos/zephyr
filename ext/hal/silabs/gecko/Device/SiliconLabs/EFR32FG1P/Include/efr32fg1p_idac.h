/**************************************************************************//**
 * @file efr32fg1p_idac.h
 * @brief EFR32FG1P_IDAC register and bit field definitions
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
 * @defgroup EFR32FG1P_IDAC
 * @{
 * @brief EFR32FG1P_IDAC Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;          /**< Control Register  */
  __IOM uint32_t CURPROG;       /**< Current Programming Register  */
  uint32_t       RESERVED0[1];  /**< Reserved for future use **/
  __IOM uint32_t DUTYCONFIG;    /**< Duty Cycle Configuration Register  */

  uint32_t       RESERVED1[2];  /**< Reserved for future use **/
  __IM uint32_t  STATUS;        /**< Status Register  */
  uint32_t       RESERVED2[1];  /**< Reserved for future use **/
  __IM uint32_t  IF;            /**< Interrupt Flag Register  */
  __IOM uint32_t IFS;           /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;           /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;           /**< Interrupt Enable Register  */
  uint32_t       RESERVED3[1];  /**< Reserved for future use **/
  __IM uint32_t  APORTREQ;      /**< APORT Request Status Register  */
  __IM uint32_t  APORTCONFLICT; /**< APORT Request Status Register  */
} IDAC_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_IDAC_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for IDAC CTRL */
#define _IDAC_CTRL_RESETVALUE                          0x00000000UL                              /**< Default value for IDAC_CTRL */
#define _IDAC_CTRL_MASK                                0x00F17FFFUL                              /**< Mask for IDAC_CTRL */
#define IDAC_CTRL_EN                                   (0x1UL << 0)                              /**< Current DAC Enable */
#define _IDAC_CTRL_EN_SHIFT                            0                                         /**< Shift value for IDAC_EN */
#define _IDAC_CTRL_EN_MASK                             0x1UL                                     /**< Bit mask for IDAC_EN */
#define _IDAC_CTRL_EN_DEFAULT                          0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_EN_DEFAULT                           (_IDAC_CTRL_EN_DEFAULT << 0)              /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_CURSINK                              (0x1UL << 1)                              /**< Current Sink Enable */
#define _IDAC_CTRL_CURSINK_SHIFT                       1                                         /**< Shift value for IDAC_CURSINK */
#define _IDAC_CTRL_CURSINK_MASK                        0x2UL                                     /**< Bit mask for IDAC_CURSINK */
#define _IDAC_CTRL_CURSINK_DEFAULT                     0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_CURSINK_DEFAULT                      (_IDAC_CTRL_CURSINK_DEFAULT << 1)         /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_MINOUTTRANS                          (0x1UL << 2)                              /**< Minimum Output Transition Enable */
#define _IDAC_CTRL_MINOUTTRANS_SHIFT                   2                                         /**< Shift value for IDAC_MINOUTTRANS */
#define _IDAC_CTRL_MINOUTTRANS_MASK                    0x4UL                                     /**< Bit mask for IDAC_MINOUTTRANS */
#define _IDAC_CTRL_MINOUTTRANS_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_MINOUTTRANS_DEFAULT                  (_IDAC_CTRL_MINOUTTRANS_DEFAULT << 2)     /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTEN                           (0x1UL << 3)                              /**< APORT Output Enable */
#define _IDAC_CTRL_APORTOUTEN_SHIFT                    3                                         /**< Shift value for IDAC_APORTOUTEN */
#define _IDAC_CTRL_APORTOUTEN_MASK                     0x8UL                                     /**< Bit mask for IDAC_APORTOUTEN */
#define _IDAC_CTRL_APORTOUTEN_DEFAULT                  0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTEN_DEFAULT                   (_IDAC_CTRL_APORTOUTEN_DEFAULT << 3)      /**< Shifted mode DEFAULT for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_SHIFT                   4                                         /**< Shift value for IDAC_APORTOUTSEL */
#define _IDAC_CTRL_APORTOUTSEL_MASK                    0xFF0UL                                   /**< Bit mask for IDAC_APORTOUTSEL */
#define _IDAC_CTRL_APORTOUTSEL_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH0              0x00000020UL                              /**< Mode APORT1XCH0 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH1              0x00000021UL                              /**< Mode APORT1YCH1 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH2              0x00000022UL                              /**< Mode APORT1XCH2 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH3              0x00000023UL                              /**< Mode APORT1YCH3 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH4              0x00000024UL                              /**< Mode APORT1XCH4 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH5              0x00000025UL                              /**< Mode APORT1YCH5 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH6              0x00000026UL                              /**< Mode APORT1XCH6 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH7              0x00000027UL                              /**< Mode APORT1YCH7 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH8              0x00000028UL                              /**< Mode APORT1XCH8 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH9              0x00000029UL                              /**< Mode APORT1YCH9 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH10             0x0000002AUL                              /**< Mode APORT1XCH10 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH11             0x0000002BUL                              /**< Mode APORT1YCH11 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH12             0x0000002CUL                              /**< Mode APORT1XCH12 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH13             0x0000002DUL                              /**< Mode APORT1YCH13 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH14             0x0000002EUL                              /**< Mode APORT1XCH14 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH15             0x0000002FUL                              /**< Mode APORT1YCH15 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH16             0x00000030UL                              /**< Mode APORT1XCH16 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH17             0x00000031UL                              /**< Mode APORT1YCH17 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH18             0x00000032UL                              /**< Mode APORT1XCH18 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH19             0x00000033UL                              /**< Mode APORT1YCH19 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH20             0x00000034UL                              /**< Mode APORT1XCH20 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH21             0x00000035UL                              /**< Mode APORT1YCH21 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH22             0x00000036UL                              /**< Mode APORT1XCH22 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH23             0x00000037UL                              /**< Mode APORT1YCH23 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH24             0x00000038UL                              /**< Mode APORT1XCH24 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH25             0x00000039UL                              /**< Mode APORT1YCH25 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH26             0x0000003AUL                              /**< Mode APORT1XCH26 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH27             0x0000003BUL                              /**< Mode APORT1YCH27 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH28             0x0000003CUL                              /**< Mode APORT1XCH28 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH29             0x0000003DUL                              /**< Mode APORT1YCH29 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1XCH30             0x0000003EUL                              /**< Mode APORT1XCH30 for IDAC_CTRL */
#define _IDAC_CTRL_APORTOUTSEL_APORT1YCH31             0x0000003FUL                              /**< Mode APORT1YCH31 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_DEFAULT                  (_IDAC_CTRL_APORTOUTSEL_DEFAULT << 4)     /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH0               (_IDAC_CTRL_APORTOUTSEL_APORT1XCH0 << 4)  /**< Shifted mode APORT1XCH0 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH1               (_IDAC_CTRL_APORTOUTSEL_APORT1YCH1 << 4)  /**< Shifted mode APORT1YCH1 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH2               (_IDAC_CTRL_APORTOUTSEL_APORT1XCH2 << 4)  /**< Shifted mode APORT1XCH2 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH3               (_IDAC_CTRL_APORTOUTSEL_APORT1YCH3 << 4)  /**< Shifted mode APORT1YCH3 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH4               (_IDAC_CTRL_APORTOUTSEL_APORT1XCH4 << 4)  /**< Shifted mode APORT1XCH4 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH5               (_IDAC_CTRL_APORTOUTSEL_APORT1YCH5 << 4)  /**< Shifted mode APORT1YCH5 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH6               (_IDAC_CTRL_APORTOUTSEL_APORT1XCH6 << 4)  /**< Shifted mode APORT1XCH6 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH7               (_IDAC_CTRL_APORTOUTSEL_APORT1YCH7 << 4)  /**< Shifted mode APORT1YCH7 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH8               (_IDAC_CTRL_APORTOUTSEL_APORT1XCH8 << 4)  /**< Shifted mode APORT1XCH8 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH9               (_IDAC_CTRL_APORTOUTSEL_APORT1YCH9 << 4)  /**< Shifted mode APORT1YCH9 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH10              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH10 << 4) /**< Shifted mode APORT1XCH10 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH11              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH11 << 4) /**< Shifted mode APORT1YCH11 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH12              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH12 << 4) /**< Shifted mode APORT1XCH12 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH13              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH13 << 4) /**< Shifted mode APORT1YCH13 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH14              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH14 << 4) /**< Shifted mode APORT1XCH14 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH15              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH15 << 4) /**< Shifted mode APORT1YCH15 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH16              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH16 << 4) /**< Shifted mode APORT1XCH16 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH17              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH17 << 4) /**< Shifted mode APORT1YCH17 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH18              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH18 << 4) /**< Shifted mode APORT1XCH18 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH19              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH19 << 4) /**< Shifted mode APORT1YCH19 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH20              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH20 << 4) /**< Shifted mode APORT1XCH20 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH21              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH21 << 4) /**< Shifted mode APORT1YCH21 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH22              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH22 << 4) /**< Shifted mode APORT1XCH22 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH23              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH23 << 4) /**< Shifted mode APORT1YCH23 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH24              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH24 << 4) /**< Shifted mode APORT1XCH24 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH25              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH25 << 4) /**< Shifted mode APORT1YCH25 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH26              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH26 << 4) /**< Shifted mode APORT1XCH26 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH27              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH27 << 4) /**< Shifted mode APORT1YCH27 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH28              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH28 << 4) /**< Shifted mode APORT1XCH28 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH29              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH29 << 4) /**< Shifted mode APORT1YCH29 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1XCH30              (_IDAC_CTRL_APORTOUTSEL_APORT1XCH30 << 4) /**< Shifted mode APORT1XCH30 for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTSEL_APORT1YCH31              (_IDAC_CTRL_APORTOUTSEL_APORT1YCH31 << 4) /**< Shifted mode APORT1YCH31 for IDAC_CTRL */
#define IDAC_CTRL_PWRSEL                               (0x1UL << 12)                             /**< Power Select */
#define _IDAC_CTRL_PWRSEL_SHIFT                        12                                        /**< Shift value for IDAC_PWRSEL */
#define _IDAC_CTRL_PWRSEL_MASK                         0x1000UL                                  /**< Bit mask for IDAC_PWRSEL */
#define _IDAC_CTRL_PWRSEL_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define _IDAC_CTRL_PWRSEL_ANA                          0x00000000UL                              /**< Mode ANA for IDAC_CTRL */
#define _IDAC_CTRL_PWRSEL_IO                           0x00000001UL                              /**< Mode IO for IDAC_CTRL */
#define IDAC_CTRL_PWRSEL_DEFAULT                       (_IDAC_CTRL_PWRSEL_DEFAULT << 12)         /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_PWRSEL_ANA                           (_IDAC_CTRL_PWRSEL_ANA << 12)             /**< Shifted mode ANA for IDAC_CTRL */
#define IDAC_CTRL_PWRSEL_IO                            (_IDAC_CTRL_PWRSEL_IO << 12)              /**< Shifted mode IO for IDAC_CTRL */
#define IDAC_CTRL_EM2DELAY                             (0x1UL << 13)                             /**< EM2 Delay */
#define _IDAC_CTRL_EM2DELAY_SHIFT                      13                                        /**< Shift value for IDAC_EM2DELAY */
#define _IDAC_CTRL_EM2DELAY_MASK                       0x2000UL                                  /**< Bit mask for IDAC_EM2DELAY */
#define _IDAC_CTRL_EM2DELAY_DEFAULT                    0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_EM2DELAY_DEFAULT                     (_IDAC_CTRL_EM2DELAY_DEFAULT << 13)       /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_APORTMASTERDIS                       (0x1UL << 14)                             /**< APORT Bus Master Disable */
#define _IDAC_CTRL_APORTMASTERDIS_SHIFT                14                                        /**< Shift value for IDAC_APORTMASTERDIS */
#define _IDAC_CTRL_APORTMASTERDIS_MASK                 0x4000UL                                  /**< Bit mask for IDAC_APORTMASTERDIS */
#define _IDAC_CTRL_APORTMASTERDIS_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_APORTMASTERDIS_DEFAULT               (_IDAC_CTRL_APORTMASTERDIS_DEFAULT << 14) /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTENPRS                        (0x1UL << 16)                             /**< PRS Controlled APORT Output Enable */
#define _IDAC_CTRL_APORTOUTENPRS_SHIFT                 16                                        /**< Shift value for IDAC_APORTOUTENPRS */
#define _IDAC_CTRL_APORTOUTENPRS_MASK                  0x10000UL                                 /**< Bit mask for IDAC_APORTOUTENPRS */
#define _IDAC_CTRL_APORTOUTENPRS_DEFAULT               0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_APORTOUTENPRS_DEFAULT                (_IDAC_CTRL_APORTOUTENPRS_DEFAULT << 16)  /**< Shifted mode DEFAULT for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_SHIFT                        20                                        /**< Shift value for IDAC_PRSSEL */
#define _IDAC_CTRL_PRSSEL_MASK                         0xF00000UL                                /**< Bit mask for IDAC_PRSSEL */
#define _IDAC_CTRL_PRSSEL_DEFAULT                      0x00000000UL                              /**< Mode DEFAULT for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH0                       0x00000000UL                              /**< Mode PRSCH0 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH1                       0x00000001UL                              /**< Mode PRSCH1 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH2                       0x00000002UL                              /**< Mode PRSCH2 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH3                       0x00000003UL                              /**< Mode PRSCH3 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH4                       0x00000004UL                              /**< Mode PRSCH4 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH5                       0x00000005UL                              /**< Mode PRSCH5 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH6                       0x00000006UL                              /**< Mode PRSCH6 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH7                       0x00000007UL                              /**< Mode PRSCH7 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH8                       0x00000008UL                              /**< Mode PRSCH8 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH9                       0x00000009UL                              /**< Mode PRSCH9 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH10                      0x0000000AUL                              /**< Mode PRSCH10 for IDAC_CTRL */
#define _IDAC_CTRL_PRSSEL_PRSCH11                      0x0000000BUL                              /**< Mode PRSCH11 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_DEFAULT                       (_IDAC_CTRL_PRSSEL_DEFAULT << 20)         /**< Shifted mode DEFAULT for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH0                        (_IDAC_CTRL_PRSSEL_PRSCH0 << 20)          /**< Shifted mode PRSCH0 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH1                        (_IDAC_CTRL_PRSSEL_PRSCH1 << 20)          /**< Shifted mode PRSCH1 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH2                        (_IDAC_CTRL_PRSSEL_PRSCH2 << 20)          /**< Shifted mode PRSCH2 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH3                        (_IDAC_CTRL_PRSSEL_PRSCH3 << 20)          /**< Shifted mode PRSCH3 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH4                        (_IDAC_CTRL_PRSSEL_PRSCH4 << 20)          /**< Shifted mode PRSCH4 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH5                        (_IDAC_CTRL_PRSSEL_PRSCH5 << 20)          /**< Shifted mode PRSCH5 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH6                        (_IDAC_CTRL_PRSSEL_PRSCH6 << 20)          /**< Shifted mode PRSCH6 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH7                        (_IDAC_CTRL_PRSSEL_PRSCH7 << 20)          /**< Shifted mode PRSCH7 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH8                        (_IDAC_CTRL_PRSSEL_PRSCH8 << 20)          /**< Shifted mode PRSCH8 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH9                        (_IDAC_CTRL_PRSSEL_PRSCH9 << 20)          /**< Shifted mode PRSCH9 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH10                       (_IDAC_CTRL_PRSSEL_PRSCH10 << 20)         /**< Shifted mode PRSCH10 for IDAC_CTRL */
#define IDAC_CTRL_PRSSEL_PRSCH11                       (_IDAC_CTRL_PRSSEL_PRSCH11 << 20)         /**< Shifted mode PRSCH11 for IDAC_CTRL */

/* Bit fields for IDAC CURPROG */
#define _IDAC_CURPROG_RESETVALUE                       0x009B0000UL                          /**< Default value for IDAC_CURPROG */
#define _IDAC_CURPROG_MASK                             0x00FF1F03UL                          /**< Mask for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_SHIFT                   0                                     /**< Shift value for IDAC_RANGESEL */
#define _IDAC_CURPROG_RANGESEL_MASK                    0x3UL                                 /**< Bit mask for IDAC_RANGESEL */
#define _IDAC_CURPROG_RANGESEL_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_RANGE0                  0x00000000UL                          /**< Mode RANGE0 for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_RANGE1                  0x00000001UL                          /**< Mode RANGE1 for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_RANGE2                  0x00000002UL                          /**< Mode RANGE2 for IDAC_CURPROG */
#define _IDAC_CURPROG_RANGESEL_RANGE3                  0x00000003UL                          /**< Mode RANGE3 for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_DEFAULT                  (_IDAC_CURPROG_RANGESEL_DEFAULT << 0) /**< Shifted mode DEFAULT for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_RANGE0                   (_IDAC_CURPROG_RANGESEL_RANGE0 << 0)  /**< Shifted mode RANGE0 for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_RANGE1                   (_IDAC_CURPROG_RANGESEL_RANGE1 << 0)  /**< Shifted mode RANGE1 for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_RANGE2                   (_IDAC_CURPROG_RANGESEL_RANGE2 << 0)  /**< Shifted mode RANGE2 for IDAC_CURPROG */
#define IDAC_CURPROG_RANGESEL_RANGE3                   (_IDAC_CURPROG_RANGESEL_RANGE3 << 0)  /**< Shifted mode RANGE3 for IDAC_CURPROG */
#define _IDAC_CURPROG_STEPSEL_SHIFT                    8                                     /**< Shift value for IDAC_STEPSEL */
#define _IDAC_CURPROG_STEPSEL_MASK                     0x1F00UL                              /**< Bit mask for IDAC_STEPSEL */
#define _IDAC_CURPROG_STEPSEL_DEFAULT                  0x00000000UL                          /**< Mode DEFAULT for IDAC_CURPROG */
#define IDAC_CURPROG_STEPSEL_DEFAULT                   (_IDAC_CURPROG_STEPSEL_DEFAULT << 8)  /**< Shifted mode DEFAULT for IDAC_CURPROG */
#define _IDAC_CURPROG_TUNING_SHIFT                     16                                    /**< Shift value for IDAC_TUNING */
#define _IDAC_CURPROG_TUNING_MASK                      0xFF0000UL                            /**< Bit mask for IDAC_TUNING */
#define _IDAC_CURPROG_TUNING_DEFAULT                   0x0000009BUL                          /**< Mode DEFAULT for IDAC_CURPROG */
#define IDAC_CURPROG_TUNING_DEFAULT                    (_IDAC_CURPROG_TUNING_DEFAULT << 16)  /**< Shifted mode DEFAULT for IDAC_CURPROG */

/* Bit fields for IDAC DUTYCONFIG */
#define _IDAC_DUTYCONFIG_RESETVALUE                    0x00000000UL                                    /**< Default value for IDAC_DUTYCONFIG */
#define _IDAC_DUTYCONFIG_MASK                          0x00000002UL                                    /**< Mask for IDAC_DUTYCONFIG */
#define IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS                (0x1UL << 1)                                    /**< Duty Cycle Enable. */
#define _IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_SHIFT         1                                               /**< Shift value for IDAC_EM2DUTYCYCLEDIS */
#define _IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_MASK          0x2UL                                           /**< Bit mask for IDAC_EM2DUTYCYCLEDIS */
#define _IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_DEFAULT       0x00000000UL                                    /**< Mode DEFAULT for IDAC_DUTYCONFIG */
#define IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_DEFAULT        (_IDAC_DUTYCONFIG_EM2DUTYCYCLEDIS_DEFAULT << 1) /**< Shifted mode DEFAULT for IDAC_DUTYCONFIG */

/* Bit fields for IDAC STATUS */
#define _IDAC_STATUS_RESETVALUE                        0x00000000UL                              /**< Default value for IDAC_STATUS */
#define _IDAC_STATUS_MASK                              0x00000002UL                              /**< Mask for IDAC_STATUS */
#define IDAC_STATUS_APORTCONFLICT                      (0x1UL << 1)                              /**< APORT Conflict Output */
#define _IDAC_STATUS_APORTCONFLICT_SHIFT               1                                         /**< Shift value for IDAC_APORTCONFLICT */
#define _IDAC_STATUS_APORTCONFLICT_MASK                0x2UL                                     /**< Bit mask for IDAC_APORTCONFLICT */
#define _IDAC_STATUS_APORTCONFLICT_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for IDAC_STATUS */
#define IDAC_STATUS_APORTCONFLICT_DEFAULT              (_IDAC_STATUS_APORTCONFLICT_DEFAULT << 1) /**< Shifted mode DEFAULT for IDAC_STATUS */

/* Bit fields for IDAC IF */
#define _IDAC_IF_RESETVALUE                            0x00000000UL                          /**< Default value for IDAC_IF */
#define _IDAC_IF_MASK                                  0x00000002UL                          /**< Mask for IDAC_IF */
#define IDAC_IF_APORTCONFLICT                          (0x1UL << 1)                          /**< APORT Conflict Interrupt Flag */
#define _IDAC_IF_APORTCONFLICT_SHIFT                   1                                     /**< Shift value for IDAC_APORTCONFLICT */
#define _IDAC_IF_APORTCONFLICT_MASK                    0x2UL                                 /**< Bit mask for IDAC_APORTCONFLICT */
#define _IDAC_IF_APORTCONFLICT_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for IDAC_IF */
#define IDAC_IF_APORTCONFLICT_DEFAULT                  (_IDAC_IF_APORTCONFLICT_DEFAULT << 1) /**< Shifted mode DEFAULT for IDAC_IF */

/* Bit fields for IDAC IFS */
#define _IDAC_IFS_RESETVALUE                           0x00000000UL                           /**< Default value for IDAC_IFS */
#define _IDAC_IFS_MASK                                 0x00000002UL                           /**< Mask for IDAC_IFS */
#define IDAC_IFS_APORTCONFLICT                         (0x1UL << 1)                           /**< Set APORTCONFLICT Interrupt Flag */
#define _IDAC_IFS_APORTCONFLICT_SHIFT                  1                                      /**< Shift value for IDAC_APORTCONFLICT */
#define _IDAC_IFS_APORTCONFLICT_MASK                   0x2UL                                  /**< Bit mask for IDAC_APORTCONFLICT */
#define _IDAC_IFS_APORTCONFLICT_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for IDAC_IFS */
#define IDAC_IFS_APORTCONFLICT_DEFAULT                 (_IDAC_IFS_APORTCONFLICT_DEFAULT << 1) /**< Shifted mode DEFAULT for IDAC_IFS */

/* Bit fields for IDAC IFC */
#define _IDAC_IFC_RESETVALUE                           0x00000000UL                           /**< Default value for IDAC_IFC */
#define _IDAC_IFC_MASK                                 0x00000002UL                           /**< Mask for IDAC_IFC */
#define IDAC_IFC_APORTCONFLICT                         (0x1UL << 1)                           /**< Clear APORTCONFLICT Interrupt Flag */
#define _IDAC_IFC_APORTCONFLICT_SHIFT                  1                                      /**< Shift value for IDAC_APORTCONFLICT */
#define _IDAC_IFC_APORTCONFLICT_MASK                   0x2UL                                  /**< Bit mask for IDAC_APORTCONFLICT */
#define _IDAC_IFC_APORTCONFLICT_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for IDAC_IFC */
#define IDAC_IFC_APORTCONFLICT_DEFAULT                 (_IDAC_IFC_APORTCONFLICT_DEFAULT << 1) /**< Shifted mode DEFAULT for IDAC_IFC */

/* Bit fields for IDAC IEN */
#define _IDAC_IEN_RESETVALUE                           0x00000000UL                           /**< Default value for IDAC_IEN */
#define _IDAC_IEN_MASK                                 0x00000002UL                           /**< Mask for IDAC_IEN */
#define IDAC_IEN_APORTCONFLICT                         (0x1UL << 1)                           /**< APORTCONFLICT Interrupt Enable */
#define _IDAC_IEN_APORTCONFLICT_SHIFT                  1                                      /**< Shift value for IDAC_APORTCONFLICT */
#define _IDAC_IEN_APORTCONFLICT_MASK                   0x2UL                                  /**< Bit mask for IDAC_APORTCONFLICT */
#define _IDAC_IEN_APORTCONFLICT_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for IDAC_IEN */
#define IDAC_IEN_APORTCONFLICT_DEFAULT                 (_IDAC_IEN_APORTCONFLICT_DEFAULT << 1) /**< Shifted mode DEFAULT for IDAC_IEN */

/* Bit fields for IDAC APORTREQ */
#define _IDAC_APORTREQ_RESETVALUE                      0x00000000UL                             /**< Default value for IDAC_APORTREQ */
#define _IDAC_APORTREQ_MASK                            0x0000000CUL                             /**< Mask for IDAC_APORTREQ */
#define IDAC_APORTREQ_APORT1XREQ                       (0x1UL << 2)                             /**< 1 if the APORT bus connected to APORT1X is requested */
#define _IDAC_APORTREQ_APORT1XREQ_SHIFT                2                                        /**< Shift value for IDAC_APORT1XREQ */
#define _IDAC_APORTREQ_APORT1XREQ_MASK                 0x4UL                                    /**< Bit mask for IDAC_APORT1XREQ */
#define _IDAC_APORTREQ_APORT1XREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for IDAC_APORTREQ */
#define IDAC_APORTREQ_APORT1XREQ_DEFAULT               (_IDAC_APORTREQ_APORT1XREQ_DEFAULT << 2) /**< Shifted mode DEFAULT for IDAC_APORTREQ */
#define IDAC_APORTREQ_APORT1YREQ                       (0x1UL << 3)                             /**< 1 if the bus connected to APORT1Y is requested */
#define _IDAC_APORTREQ_APORT1YREQ_SHIFT                3                                        /**< Shift value for IDAC_APORT1YREQ */
#define _IDAC_APORTREQ_APORT1YREQ_MASK                 0x8UL                                    /**< Bit mask for IDAC_APORT1YREQ */
#define _IDAC_APORTREQ_APORT1YREQ_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for IDAC_APORTREQ */
#define IDAC_APORTREQ_APORT1YREQ_DEFAULT               (_IDAC_APORTREQ_APORT1YREQ_DEFAULT << 3) /**< Shifted mode DEFAULT for IDAC_APORTREQ */

/* Bit fields for IDAC APORTCONFLICT */
#define _IDAC_APORTCONFLICT_RESETVALUE                 0x00000000UL                                       /**< Default value for IDAC_APORTCONFLICT */
#define _IDAC_APORTCONFLICT_MASK                       0x0000000CUL                                       /**< Mask for IDAC_APORTCONFLICT */
#define IDAC_APORTCONFLICT_APORT1XCONFLICT             (0x1UL << 2)                                       /**< 1 if the bus connected to APORT1X is in conflict with another peripheral */
#define _IDAC_APORTCONFLICT_APORT1XCONFLICT_SHIFT      2                                                  /**< Shift value for IDAC_APORT1XCONFLICT */
#define _IDAC_APORTCONFLICT_APORT1XCONFLICT_MASK       0x4UL                                              /**< Bit mask for IDAC_APORT1XCONFLICT */
#define _IDAC_APORTCONFLICT_APORT1XCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for IDAC_APORTCONFLICT */
#define IDAC_APORTCONFLICT_APORT1XCONFLICT_DEFAULT     (_IDAC_APORTCONFLICT_APORT1XCONFLICT_DEFAULT << 2) /**< Shifted mode DEFAULT for IDAC_APORTCONFLICT */
#define IDAC_APORTCONFLICT_APORT1YCONFLICT             (0x1UL << 3)                                       /**< 1 if the bus connected to APORT1Y is in conflict with another peripheral */
#define _IDAC_APORTCONFLICT_APORT1YCONFLICT_SHIFT      3                                                  /**< Shift value for IDAC_APORT1YCONFLICT */
#define _IDAC_APORTCONFLICT_APORT1YCONFLICT_MASK       0x8UL                                              /**< Bit mask for IDAC_APORT1YCONFLICT */
#define _IDAC_APORTCONFLICT_APORT1YCONFLICT_DEFAULT    0x00000000UL                                       /**< Mode DEFAULT for IDAC_APORTCONFLICT */
#define IDAC_APORTCONFLICT_APORT1YCONFLICT_DEFAULT     (_IDAC_APORTCONFLICT_APORT1YCONFLICT_DEFAULT << 3) /**< Shifted mode DEFAULT for IDAC_APORTCONFLICT */

/** @} End of group EFR32FG1P_IDAC */
/** @} End of group Parts */

