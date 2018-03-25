/**************************************************************************//**
 * @file efr32fg1p_timer.h
 * @brief EFR32FG1P_TIMER register and bit field definitions
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
 * @defgroup EFR32FG1P_TIMER
 * @{
 * @brief EFR32FG1P_TIMER Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t   CTRL;         /**< Control Register  */
  __IOM uint32_t   CMD;          /**< Command Register  */
  __IM uint32_t    STATUS;       /**< Status Register  */
  __IM uint32_t    IF;           /**< Interrupt Flag Register  */
  __IOM uint32_t   IFS;          /**< Interrupt Flag Set Register  */
  __IOM uint32_t   IFC;          /**< Interrupt Flag Clear Register  */
  __IOM uint32_t   IEN;          /**< Interrupt Enable Register  */
  __IOM uint32_t   TOP;          /**< Counter Top Value Register  */
  __IOM uint32_t   TOPB;         /**< Counter Top Value Buffer Register  */
  __IOM uint32_t   CNT;          /**< Counter Value Register  */
  uint32_t         RESERVED0[1]; /**< Reserved for future use **/
  __IOM uint32_t   LOCK;         /**< TIMER Configuration Lock Register  */
  __IOM uint32_t   ROUTEPEN;     /**< I/O Routing Pin Enable Register  */
  __IOM uint32_t   ROUTELOC0;    /**< I/O Routing Location Register  */
  uint32_t         RESERVED1[1]; /**< Reserved for future use **/
  __IOM uint32_t   ROUTELOC2;    /**< I/O Routing Location Register  */

  uint32_t         RESERVED2[8]; /**< Reserved registers */
  TIMER_CC_TypeDef CC[4];        /**< Compare/Capture Channel */

  __IOM uint32_t   DTCTRL;       /**< DTI Control Register  */
  __IOM uint32_t   DTTIME;       /**< DTI Time Control Register  */
  __IOM uint32_t   DTFC;         /**< DTI Fault Configuration Register  */
  __IOM uint32_t   DTOGEN;       /**< DTI Output Generation Enable Register  */
  __IM uint32_t    DTFAULT;      /**< DTI Fault Register  */
  __IOM uint32_t   DTFAULTC;     /**< DTI Fault Clear Register  */
  __IOM uint32_t   DTLOCK;       /**< DTI Configuration Lock Register  */
} TIMER_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_TIMER_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for TIMER CTRL */
#define _TIMER_CTRL_RESETVALUE                     0x00000000UL                             /**< Default value for TIMER_CTRL */
#define _TIMER_CTRL_MASK                           0x3F032FFBUL                             /**< Mask for TIMER_CTRL */
#define _TIMER_CTRL_MODE_SHIFT                     0                                        /**< Shift value for TIMER_MODE */
#define _TIMER_CTRL_MODE_MASK                      0x3UL                                    /**< Bit mask for TIMER_MODE */
#define _TIMER_CTRL_MODE_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define _TIMER_CTRL_MODE_UP                        0x00000000UL                             /**< Mode UP for TIMER_CTRL */
#define _TIMER_CTRL_MODE_DOWN                      0x00000001UL                             /**< Mode DOWN for TIMER_CTRL */
#define _TIMER_CTRL_MODE_UPDOWN                    0x00000002UL                             /**< Mode UPDOWN for TIMER_CTRL */
#define _TIMER_CTRL_MODE_QDEC                      0x00000003UL                             /**< Mode QDEC for TIMER_CTRL */
#define TIMER_CTRL_MODE_DEFAULT                    (_TIMER_CTRL_MODE_DEFAULT << 0)          /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_MODE_UP                         (_TIMER_CTRL_MODE_UP << 0)               /**< Shifted mode UP for TIMER_CTRL */
#define TIMER_CTRL_MODE_DOWN                       (_TIMER_CTRL_MODE_DOWN << 0)             /**< Shifted mode DOWN for TIMER_CTRL */
#define TIMER_CTRL_MODE_UPDOWN                     (_TIMER_CTRL_MODE_UPDOWN << 0)           /**< Shifted mode UPDOWN for TIMER_CTRL */
#define TIMER_CTRL_MODE_QDEC                       (_TIMER_CTRL_MODE_QDEC << 0)             /**< Shifted mode QDEC for TIMER_CTRL */
#define TIMER_CTRL_SYNC                            (0x1UL << 3)                             /**< Timer Start/Stop/Reload Synchronization */
#define _TIMER_CTRL_SYNC_SHIFT                     3                                        /**< Shift value for TIMER_SYNC */
#define _TIMER_CTRL_SYNC_MASK                      0x8UL                                    /**< Bit mask for TIMER_SYNC */
#define _TIMER_CTRL_SYNC_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_SYNC_DEFAULT                    (_TIMER_CTRL_SYNC_DEFAULT << 3)          /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_OSMEN                           (0x1UL << 4)                             /**< One-shot Mode Enable */
#define _TIMER_CTRL_OSMEN_SHIFT                    4                                        /**< Shift value for TIMER_OSMEN */
#define _TIMER_CTRL_OSMEN_MASK                     0x10UL                                   /**< Bit mask for TIMER_OSMEN */
#define _TIMER_CTRL_OSMEN_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_OSMEN_DEFAULT                   (_TIMER_CTRL_OSMEN_DEFAULT << 4)         /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_QDM                             (0x1UL << 5)                             /**< Quadrature Decoder Mode Selection */
#define _TIMER_CTRL_QDM_SHIFT                      5                                        /**< Shift value for TIMER_QDM */
#define _TIMER_CTRL_QDM_MASK                       0x20UL                                   /**< Bit mask for TIMER_QDM */
#define _TIMER_CTRL_QDM_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define _TIMER_CTRL_QDM_X2                         0x00000000UL                             /**< Mode X2 for TIMER_CTRL */
#define _TIMER_CTRL_QDM_X4                         0x00000001UL                             /**< Mode X4 for TIMER_CTRL */
#define TIMER_CTRL_QDM_DEFAULT                     (_TIMER_CTRL_QDM_DEFAULT << 5)           /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_QDM_X2                          (_TIMER_CTRL_QDM_X2 << 5)                /**< Shifted mode X2 for TIMER_CTRL */
#define TIMER_CTRL_QDM_X4                          (_TIMER_CTRL_QDM_X4 << 5)                /**< Shifted mode X4 for TIMER_CTRL */
#define TIMER_CTRL_DEBUGRUN                        (0x1UL << 6)                             /**< Debug Mode Run Enable */
#define _TIMER_CTRL_DEBUGRUN_SHIFT                 6                                        /**< Shift value for TIMER_DEBUGRUN */
#define _TIMER_CTRL_DEBUGRUN_MASK                  0x40UL                                   /**< Bit mask for TIMER_DEBUGRUN */
#define _TIMER_CTRL_DEBUGRUN_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_DEBUGRUN_DEFAULT                (_TIMER_CTRL_DEBUGRUN_DEFAULT << 6)      /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_DMACLRACT                       (0x1UL << 7)                             /**< DMA Request Clear on Active */
#define _TIMER_CTRL_DMACLRACT_SHIFT                7                                        /**< Shift value for TIMER_DMACLRACT */
#define _TIMER_CTRL_DMACLRACT_MASK                 0x80UL                                   /**< Bit mask for TIMER_DMACLRACT */
#define _TIMER_CTRL_DMACLRACT_DEFAULT              0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_DMACLRACT_DEFAULT               (_TIMER_CTRL_DMACLRACT_DEFAULT << 7)     /**< Shifted mode DEFAULT for TIMER_CTRL */
#define _TIMER_CTRL_RISEA_SHIFT                    8                                        /**< Shift value for TIMER_RISEA */
#define _TIMER_CTRL_RISEA_MASK                     0x300UL                                  /**< Bit mask for TIMER_RISEA */
#define _TIMER_CTRL_RISEA_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define _TIMER_CTRL_RISEA_NONE                     0x00000000UL                             /**< Mode NONE for TIMER_CTRL */
#define _TIMER_CTRL_RISEA_START                    0x00000001UL                             /**< Mode START for TIMER_CTRL */
#define _TIMER_CTRL_RISEA_STOP                     0x00000002UL                             /**< Mode STOP for TIMER_CTRL */
#define _TIMER_CTRL_RISEA_RELOADSTART              0x00000003UL                             /**< Mode RELOADSTART for TIMER_CTRL */
#define TIMER_CTRL_RISEA_DEFAULT                   (_TIMER_CTRL_RISEA_DEFAULT << 8)         /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_RISEA_NONE                      (_TIMER_CTRL_RISEA_NONE << 8)            /**< Shifted mode NONE for TIMER_CTRL */
#define TIMER_CTRL_RISEA_START                     (_TIMER_CTRL_RISEA_START << 8)           /**< Shifted mode START for TIMER_CTRL */
#define TIMER_CTRL_RISEA_STOP                      (_TIMER_CTRL_RISEA_STOP << 8)            /**< Shifted mode STOP for TIMER_CTRL */
#define TIMER_CTRL_RISEA_RELOADSTART               (_TIMER_CTRL_RISEA_RELOADSTART << 8)     /**< Shifted mode RELOADSTART for TIMER_CTRL */
#define _TIMER_CTRL_FALLA_SHIFT                    10                                       /**< Shift value for TIMER_FALLA */
#define _TIMER_CTRL_FALLA_MASK                     0xC00UL                                  /**< Bit mask for TIMER_FALLA */
#define _TIMER_CTRL_FALLA_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define _TIMER_CTRL_FALLA_NONE                     0x00000000UL                             /**< Mode NONE for TIMER_CTRL */
#define _TIMER_CTRL_FALLA_START                    0x00000001UL                             /**< Mode START for TIMER_CTRL */
#define _TIMER_CTRL_FALLA_STOP                     0x00000002UL                             /**< Mode STOP for TIMER_CTRL */
#define _TIMER_CTRL_FALLA_RELOADSTART              0x00000003UL                             /**< Mode RELOADSTART for TIMER_CTRL */
#define TIMER_CTRL_FALLA_DEFAULT                   (_TIMER_CTRL_FALLA_DEFAULT << 10)        /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_FALLA_NONE                      (_TIMER_CTRL_FALLA_NONE << 10)           /**< Shifted mode NONE for TIMER_CTRL */
#define TIMER_CTRL_FALLA_START                     (_TIMER_CTRL_FALLA_START << 10)          /**< Shifted mode START for TIMER_CTRL */
#define TIMER_CTRL_FALLA_STOP                      (_TIMER_CTRL_FALLA_STOP << 10)           /**< Shifted mode STOP for TIMER_CTRL */
#define TIMER_CTRL_FALLA_RELOADSTART               (_TIMER_CTRL_FALLA_RELOADSTART << 10)    /**< Shifted mode RELOADSTART for TIMER_CTRL */
#define TIMER_CTRL_X2CNT                           (0x1UL << 13)                            /**< 2x Count Mode */
#define _TIMER_CTRL_X2CNT_SHIFT                    13                                       /**< Shift value for TIMER_X2CNT */
#define _TIMER_CTRL_X2CNT_MASK                     0x2000UL                                 /**< Bit mask for TIMER_X2CNT */
#define _TIMER_CTRL_X2CNT_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_X2CNT_DEFAULT                   (_TIMER_CTRL_X2CNT_DEFAULT << 13)        /**< Shifted mode DEFAULT for TIMER_CTRL */
#define _TIMER_CTRL_CLKSEL_SHIFT                   16                                       /**< Shift value for TIMER_CLKSEL */
#define _TIMER_CTRL_CLKSEL_MASK                    0x30000UL                                /**< Bit mask for TIMER_CLKSEL */
#define _TIMER_CTRL_CLKSEL_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define _TIMER_CTRL_CLKSEL_PRESCHFPERCLK           0x00000000UL                             /**< Mode PRESCHFPERCLK for TIMER_CTRL */
#define _TIMER_CTRL_CLKSEL_CC1                     0x00000001UL                             /**< Mode CC1 for TIMER_CTRL */
#define _TIMER_CTRL_CLKSEL_TIMEROUF                0x00000002UL                             /**< Mode TIMEROUF for TIMER_CTRL */
#define TIMER_CTRL_CLKSEL_DEFAULT                  (_TIMER_CTRL_CLKSEL_DEFAULT << 16)       /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_CLKSEL_PRESCHFPERCLK            (_TIMER_CTRL_CLKSEL_PRESCHFPERCLK << 16) /**< Shifted mode PRESCHFPERCLK for TIMER_CTRL */
#define TIMER_CTRL_CLKSEL_CC1                      (_TIMER_CTRL_CLKSEL_CC1 << 16)           /**< Shifted mode CC1 for TIMER_CTRL */
#define TIMER_CTRL_CLKSEL_TIMEROUF                 (_TIMER_CTRL_CLKSEL_TIMEROUF << 16)      /**< Shifted mode TIMEROUF for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_SHIFT                    24                                       /**< Shift value for TIMER_PRESC */
#define _TIMER_CTRL_PRESC_MASK                     0xF000000UL                              /**< Bit mask for TIMER_PRESC */
#define _TIMER_CTRL_PRESC_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV1                     0x00000000UL                             /**< Mode DIV1 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV2                     0x00000001UL                             /**< Mode DIV2 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV4                     0x00000002UL                             /**< Mode DIV4 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV8                     0x00000003UL                             /**< Mode DIV8 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV16                    0x00000004UL                             /**< Mode DIV16 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV32                    0x00000005UL                             /**< Mode DIV32 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV64                    0x00000006UL                             /**< Mode DIV64 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV128                   0x00000007UL                             /**< Mode DIV128 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV256                   0x00000008UL                             /**< Mode DIV256 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV512                   0x00000009UL                             /**< Mode DIV512 for TIMER_CTRL */
#define _TIMER_CTRL_PRESC_DIV1024                  0x0000000AUL                             /**< Mode DIV1024 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DEFAULT                   (_TIMER_CTRL_PRESC_DEFAULT << 24)        /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV1                      (_TIMER_CTRL_PRESC_DIV1 << 24)           /**< Shifted mode DIV1 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV2                      (_TIMER_CTRL_PRESC_DIV2 << 24)           /**< Shifted mode DIV2 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV4                      (_TIMER_CTRL_PRESC_DIV4 << 24)           /**< Shifted mode DIV4 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV8                      (_TIMER_CTRL_PRESC_DIV8 << 24)           /**< Shifted mode DIV8 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV16                     (_TIMER_CTRL_PRESC_DIV16 << 24)          /**< Shifted mode DIV16 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV32                     (_TIMER_CTRL_PRESC_DIV32 << 24)          /**< Shifted mode DIV32 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV64                     (_TIMER_CTRL_PRESC_DIV64 << 24)          /**< Shifted mode DIV64 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV128                    (_TIMER_CTRL_PRESC_DIV128 << 24)         /**< Shifted mode DIV128 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV256                    (_TIMER_CTRL_PRESC_DIV256 << 24)         /**< Shifted mode DIV256 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV512                    (_TIMER_CTRL_PRESC_DIV512 << 24)         /**< Shifted mode DIV512 for TIMER_CTRL */
#define TIMER_CTRL_PRESC_DIV1024                   (_TIMER_CTRL_PRESC_DIV1024 << 24)        /**< Shifted mode DIV1024 for TIMER_CTRL */
#define TIMER_CTRL_ATI                             (0x1UL << 28)                            /**< Always Track Inputs */
#define _TIMER_CTRL_ATI_SHIFT                      28                                       /**< Shift value for TIMER_ATI */
#define _TIMER_CTRL_ATI_MASK                       0x10000000UL                             /**< Bit mask for TIMER_ATI */
#define _TIMER_CTRL_ATI_DEFAULT                    0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_ATI_DEFAULT                     (_TIMER_CTRL_ATI_DEFAULT << 28)          /**< Shifted mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_RSSCOIST                        (0x1UL << 29)                            /**< Reload-Start Sets Compare Output initial State */
#define _TIMER_CTRL_RSSCOIST_SHIFT                 29                                       /**< Shift value for TIMER_RSSCOIST */
#define _TIMER_CTRL_RSSCOIST_MASK                  0x20000000UL                             /**< Bit mask for TIMER_RSSCOIST */
#define _TIMER_CTRL_RSSCOIST_DEFAULT               0x00000000UL                             /**< Mode DEFAULT for TIMER_CTRL */
#define TIMER_CTRL_RSSCOIST_DEFAULT                (_TIMER_CTRL_RSSCOIST_DEFAULT << 29)     /**< Shifted mode DEFAULT for TIMER_CTRL */

/* Bit fields for TIMER CMD */
#define _TIMER_CMD_RESETVALUE                      0x00000000UL                    /**< Default value for TIMER_CMD */
#define _TIMER_CMD_MASK                            0x00000003UL                    /**< Mask for TIMER_CMD */
#define TIMER_CMD_START                            (0x1UL << 0)                    /**< Start Timer */
#define _TIMER_CMD_START_SHIFT                     0                               /**< Shift value for TIMER_START */
#define _TIMER_CMD_START_MASK                      0x1UL                           /**< Bit mask for TIMER_START */
#define _TIMER_CMD_START_DEFAULT                   0x00000000UL                    /**< Mode DEFAULT for TIMER_CMD */
#define TIMER_CMD_START_DEFAULT                    (_TIMER_CMD_START_DEFAULT << 0) /**< Shifted mode DEFAULT for TIMER_CMD */
#define TIMER_CMD_STOP                             (0x1UL << 1)                    /**< Stop Timer */
#define _TIMER_CMD_STOP_SHIFT                      1                               /**< Shift value for TIMER_STOP */
#define _TIMER_CMD_STOP_MASK                       0x2UL                           /**< Bit mask for TIMER_STOP */
#define _TIMER_CMD_STOP_DEFAULT                    0x00000000UL                    /**< Mode DEFAULT for TIMER_CMD */
#define TIMER_CMD_STOP_DEFAULT                     (_TIMER_CMD_STOP_DEFAULT << 1)  /**< Shifted mode DEFAULT for TIMER_CMD */

/* Bit fields for TIMER STATUS */
#define _TIMER_STATUS_RESETVALUE                   0x00000000UL                          /**< Default value for TIMER_STATUS */
#define _TIMER_STATUS_MASK                         0x0F0F0F07UL                          /**< Mask for TIMER_STATUS */
#define TIMER_STATUS_RUNNING                       (0x1UL << 0)                          /**< Running */
#define _TIMER_STATUS_RUNNING_SHIFT                0                                     /**< Shift value for TIMER_RUNNING */
#define _TIMER_STATUS_RUNNING_MASK                 0x1UL                                 /**< Bit mask for TIMER_RUNNING */
#define _TIMER_STATUS_RUNNING_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_RUNNING_DEFAULT               (_TIMER_STATUS_RUNNING_DEFAULT << 0)  /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_DIR                           (0x1UL << 1)                          /**< Direction */
#define _TIMER_STATUS_DIR_SHIFT                    1                                     /**< Shift value for TIMER_DIR */
#define _TIMER_STATUS_DIR_MASK                     0x2UL                                 /**< Bit mask for TIMER_DIR */
#define _TIMER_STATUS_DIR_DEFAULT                  0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define _TIMER_STATUS_DIR_UP                       0x00000000UL                          /**< Mode UP for TIMER_STATUS */
#define _TIMER_STATUS_DIR_DOWN                     0x00000001UL                          /**< Mode DOWN for TIMER_STATUS */
#define TIMER_STATUS_DIR_DEFAULT                   (_TIMER_STATUS_DIR_DEFAULT << 1)      /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_DIR_UP                        (_TIMER_STATUS_DIR_UP << 1)           /**< Shifted mode UP for TIMER_STATUS */
#define TIMER_STATUS_DIR_DOWN                      (_TIMER_STATUS_DIR_DOWN << 1)         /**< Shifted mode DOWN for TIMER_STATUS */
#define TIMER_STATUS_TOPBV                         (0x1UL << 2)                          /**< TOPB Valid */
#define _TIMER_STATUS_TOPBV_SHIFT                  2                                     /**< Shift value for TIMER_TOPBV */
#define _TIMER_STATUS_TOPBV_MASK                   0x4UL                                 /**< Bit mask for TIMER_TOPBV */
#define _TIMER_STATUS_TOPBV_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_TOPBV_DEFAULT                 (_TIMER_STATUS_TOPBV_DEFAULT << 2)    /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCVBV0                        (0x1UL << 8)                          /**< CC0 CCVB Valid */
#define _TIMER_STATUS_CCVBV0_SHIFT                 8                                     /**< Shift value for TIMER_CCVBV0 */
#define _TIMER_STATUS_CCVBV0_MASK                  0x100UL                               /**< Bit mask for TIMER_CCVBV0 */
#define _TIMER_STATUS_CCVBV0_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCVBV0_DEFAULT                (_TIMER_STATUS_CCVBV0_DEFAULT << 8)   /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCVBV1                        (0x1UL << 9)                          /**< CC1 CCVB Valid */
#define _TIMER_STATUS_CCVBV1_SHIFT                 9                                     /**< Shift value for TIMER_CCVBV1 */
#define _TIMER_STATUS_CCVBV1_MASK                  0x200UL                               /**< Bit mask for TIMER_CCVBV1 */
#define _TIMER_STATUS_CCVBV1_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCVBV1_DEFAULT                (_TIMER_STATUS_CCVBV1_DEFAULT << 9)   /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCVBV2                        (0x1UL << 10)                         /**< CC2 CCVB Valid */
#define _TIMER_STATUS_CCVBV2_SHIFT                 10                                    /**< Shift value for TIMER_CCVBV2 */
#define _TIMER_STATUS_CCVBV2_MASK                  0x400UL                               /**< Bit mask for TIMER_CCVBV2 */
#define _TIMER_STATUS_CCVBV2_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCVBV2_DEFAULT                (_TIMER_STATUS_CCVBV2_DEFAULT << 10)  /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCVBV3                        (0x1UL << 11)                         /**< CC3 CCVB Valid */
#define _TIMER_STATUS_CCVBV3_SHIFT                 11                                    /**< Shift value for TIMER_CCVBV3 */
#define _TIMER_STATUS_CCVBV3_MASK                  0x800UL                               /**< Bit mask for TIMER_CCVBV3 */
#define _TIMER_STATUS_CCVBV3_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCVBV3_DEFAULT                (_TIMER_STATUS_CCVBV3_DEFAULT << 11)  /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_ICV0                          (0x1UL << 16)                         /**< CC0 Input Capture Valid */
#define _TIMER_STATUS_ICV0_SHIFT                   16                                    /**< Shift value for TIMER_ICV0 */
#define _TIMER_STATUS_ICV0_MASK                    0x10000UL                             /**< Bit mask for TIMER_ICV0 */
#define _TIMER_STATUS_ICV0_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_ICV0_DEFAULT                  (_TIMER_STATUS_ICV0_DEFAULT << 16)    /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_ICV1                          (0x1UL << 17)                         /**< CC1 Input Capture Valid */
#define _TIMER_STATUS_ICV1_SHIFT                   17                                    /**< Shift value for TIMER_ICV1 */
#define _TIMER_STATUS_ICV1_MASK                    0x20000UL                             /**< Bit mask for TIMER_ICV1 */
#define _TIMER_STATUS_ICV1_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_ICV1_DEFAULT                  (_TIMER_STATUS_ICV1_DEFAULT << 17)    /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_ICV2                          (0x1UL << 18)                         /**< CC2 Input Capture Valid */
#define _TIMER_STATUS_ICV2_SHIFT                   18                                    /**< Shift value for TIMER_ICV2 */
#define _TIMER_STATUS_ICV2_MASK                    0x40000UL                             /**< Bit mask for TIMER_ICV2 */
#define _TIMER_STATUS_ICV2_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_ICV2_DEFAULT                  (_TIMER_STATUS_ICV2_DEFAULT << 18)    /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_ICV3                          (0x1UL << 19)                         /**< CC3 Input Capture Valid */
#define _TIMER_STATUS_ICV3_SHIFT                   19                                    /**< Shift value for TIMER_ICV3 */
#define _TIMER_STATUS_ICV3_MASK                    0x80000UL                             /**< Bit mask for TIMER_ICV3 */
#define _TIMER_STATUS_ICV3_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_ICV3_DEFAULT                  (_TIMER_STATUS_ICV3_DEFAULT << 19)    /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCPOL0                        (0x1UL << 24)                         /**< CC0 Polarity */
#define _TIMER_STATUS_CCPOL0_SHIFT                 24                                    /**< Shift value for TIMER_CCPOL0 */
#define _TIMER_STATUS_CCPOL0_MASK                  0x1000000UL                           /**< Bit mask for TIMER_CCPOL0 */
#define _TIMER_STATUS_CCPOL0_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define _TIMER_STATUS_CCPOL0_LOWRISE               0x00000000UL                          /**< Mode LOWRISE for TIMER_STATUS */
#define _TIMER_STATUS_CCPOL0_HIGHFALL              0x00000001UL                          /**< Mode HIGHFALL for TIMER_STATUS */
#define TIMER_STATUS_CCPOL0_DEFAULT                (_TIMER_STATUS_CCPOL0_DEFAULT << 24)  /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCPOL0_LOWRISE                (_TIMER_STATUS_CCPOL0_LOWRISE << 24)  /**< Shifted mode LOWRISE for TIMER_STATUS */
#define TIMER_STATUS_CCPOL0_HIGHFALL               (_TIMER_STATUS_CCPOL0_HIGHFALL << 24) /**< Shifted mode HIGHFALL for TIMER_STATUS */
#define TIMER_STATUS_CCPOL1                        (0x1UL << 25)                         /**< CC1 Polarity */
#define _TIMER_STATUS_CCPOL1_SHIFT                 25                                    /**< Shift value for TIMER_CCPOL1 */
#define _TIMER_STATUS_CCPOL1_MASK                  0x2000000UL                           /**< Bit mask for TIMER_CCPOL1 */
#define _TIMER_STATUS_CCPOL1_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define _TIMER_STATUS_CCPOL1_LOWRISE               0x00000000UL                          /**< Mode LOWRISE for TIMER_STATUS */
#define _TIMER_STATUS_CCPOL1_HIGHFALL              0x00000001UL                          /**< Mode HIGHFALL for TIMER_STATUS */
#define TIMER_STATUS_CCPOL1_DEFAULT                (_TIMER_STATUS_CCPOL1_DEFAULT << 25)  /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCPOL1_LOWRISE                (_TIMER_STATUS_CCPOL1_LOWRISE << 25)  /**< Shifted mode LOWRISE for TIMER_STATUS */
#define TIMER_STATUS_CCPOL1_HIGHFALL               (_TIMER_STATUS_CCPOL1_HIGHFALL << 25) /**< Shifted mode HIGHFALL for TIMER_STATUS */
#define TIMER_STATUS_CCPOL2                        (0x1UL << 26)                         /**< CC2 Polarity */
#define _TIMER_STATUS_CCPOL2_SHIFT                 26                                    /**< Shift value for TIMER_CCPOL2 */
#define _TIMER_STATUS_CCPOL2_MASK                  0x4000000UL                           /**< Bit mask for TIMER_CCPOL2 */
#define _TIMER_STATUS_CCPOL2_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define _TIMER_STATUS_CCPOL2_LOWRISE               0x00000000UL                          /**< Mode LOWRISE for TIMER_STATUS */
#define _TIMER_STATUS_CCPOL2_HIGHFALL              0x00000001UL                          /**< Mode HIGHFALL for TIMER_STATUS */
#define TIMER_STATUS_CCPOL2_DEFAULT                (_TIMER_STATUS_CCPOL2_DEFAULT << 26)  /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCPOL2_LOWRISE                (_TIMER_STATUS_CCPOL2_LOWRISE << 26)  /**< Shifted mode LOWRISE for TIMER_STATUS */
#define TIMER_STATUS_CCPOL2_HIGHFALL               (_TIMER_STATUS_CCPOL2_HIGHFALL << 26) /**< Shifted mode HIGHFALL for TIMER_STATUS */
#define TIMER_STATUS_CCPOL3                        (0x1UL << 27)                         /**< CC3 Polarity */
#define _TIMER_STATUS_CCPOL3_SHIFT                 27                                    /**< Shift value for TIMER_CCPOL3 */
#define _TIMER_STATUS_CCPOL3_MASK                  0x8000000UL                           /**< Bit mask for TIMER_CCPOL3 */
#define _TIMER_STATUS_CCPOL3_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_STATUS */
#define _TIMER_STATUS_CCPOL3_LOWRISE               0x00000000UL                          /**< Mode LOWRISE for TIMER_STATUS */
#define _TIMER_STATUS_CCPOL3_HIGHFALL              0x00000001UL                          /**< Mode HIGHFALL for TIMER_STATUS */
#define TIMER_STATUS_CCPOL3_DEFAULT                (_TIMER_STATUS_CCPOL3_DEFAULT << 27)  /**< Shifted mode DEFAULT for TIMER_STATUS */
#define TIMER_STATUS_CCPOL3_LOWRISE                (_TIMER_STATUS_CCPOL3_LOWRISE << 27)  /**< Shifted mode LOWRISE for TIMER_STATUS */
#define TIMER_STATUS_CCPOL3_HIGHFALL               (_TIMER_STATUS_CCPOL3_HIGHFALL << 27) /**< Shifted mode HIGHFALL for TIMER_STATUS */

/* Bit fields for TIMER IF */
#define _TIMER_IF_RESETVALUE                       0x00000000UL                     /**< Default value for TIMER_IF */
#define _TIMER_IF_MASK                             0x00000FF7UL                     /**< Mask for TIMER_IF */
#define TIMER_IF_OF                                (0x1UL << 0)                     /**< Overflow Interrupt Flag */
#define _TIMER_IF_OF_SHIFT                         0                                /**< Shift value for TIMER_OF */
#define _TIMER_IF_OF_MASK                          0x1UL                            /**< Bit mask for TIMER_OF */
#define _TIMER_IF_OF_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_OF_DEFAULT                        (_TIMER_IF_OF_DEFAULT << 0)      /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_UF                                (0x1UL << 1)                     /**< Underflow Interrupt Flag */
#define _TIMER_IF_UF_SHIFT                         1                                /**< Shift value for TIMER_UF */
#define _TIMER_IF_UF_MASK                          0x2UL                            /**< Bit mask for TIMER_UF */
#define _TIMER_IF_UF_DEFAULT                       0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_UF_DEFAULT                        (_TIMER_IF_UF_DEFAULT << 1)      /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_DIRCHG                            (0x1UL << 2)                     /**< Direction Change Detect Interrupt Flag */
#define _TIMER_IF_DIRCHG_SHIFT                     2                                /**< Shift value for TIMER_DIRCHG */
#define _TIMER_IF_DIRCHG_MASK                      0x4UL                            /**< Bit mask for TIMER_DIRCHG */
#define _TIMER_IF_DIRCHG_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_DIRCHG_DEFAULT                    (_TIMER_IF_DIRCHG_DEFAULT << 2)  /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_CC0                               (0x1UL << 4)                     /**< CC Channel 0 Interrupt Flag */
#define _TIMER_IF_CC0_SHIFT                        4                                /**< Shift value for TIMER_CC0 */
#define _TIMER_IF_CC0_MASK                         0x10UL                           /**< Bit mask for TIMER_CC0 */
#define _TIMER_IF_CC0_DEFAULT                      0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_CC0_DEFAULT                       (_TIMER_IF_CC0_DEFAULT << 4)     /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_CC1                               (0x1UL << 5)                     /**< CC Channel 1 Interrupt Flag */
#define _TIMER_IF_CC1_SHIFT                        5                                /**< Shift value for TIMER_CC1 */
#define _TIMER_IF_CC1_MASK                         0x20UL                           /**< Bit mask for TIMER_CC1 */
#define _TIMER_IF_CC1_DEFAULT                      0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_CC1_DEFAULT                       (_TIMER_IF_CC1_DEFAULT << 5)     /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_CC2                               (0x1UL << 6)                     /**< CC Channel 2 Interrupt Flag */
#define _TIMER_IF_CC2_SHIFT                        6                                /**< Shift value for TIMER_CC2 */
#define _TIMER_IF_CC2_MASK                         0x40UL                           /**< Bit mask for TIMER_CC2 */
#define _TIMER_IF_CC2_DEFAULT                      0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_CC2_DEFAULT                       (_TIMER_IF_CC2_DEFAULT << 6)     /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_CC3                               (0x1UL << 7)                     /**< CC Channel 3 Interrupt Flag */
#define _TIMER_IF_CC3_SHIFT                        7                                /**< Shift value for TIMER_CC3 */
#define _TIMER_IF_CC3_MASK                         0x80UL                           /**< Bit mask for TIMER_CC3 */
#define _TIMER_IF_CC3_DEFAULT                      0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_CC3_DEFAULT                       (_TIMER_IF_CC3_DEFAULT << 7)     /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_ICBOF0                            (0x1UL << 8)                     /**< CC Channel 0 Input Capture Buffer Overflow Interrupt Flag */
#define _TIMER_IF_ICBOF0_SHIFT                     8                                /**< Shift value for TIMER_ICBOF0 */
#define _TIMER_IF_ICBOF0_MASK                      0x100UL                          /**< Bit mask for TIMER_ICBOF0 */
#define _TIMER_IF_ICBOF0_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_ICBOF0_DEFAULT                    (_TIMER_IF_ICBOF0_DEFAULT << 8)  /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_ICBOF1                            (0x1UL << 9)                     /**< CC Channel 1 Input Capture Buffer Overflow Interrupt Flag */
#define _TIMER_IF_ICBOF1_SHIFT                     9                                /**< Shift value for TIMER_ICBOF1 */
#define _TIMER_IF_ICBOF1_MASK                      0x200UL                          /**< Bit mask for TIMER_ICBOF1 */
#define _TIMER_IF_ICBOF1_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_ICBOF1_DEFAULT                    (_TIMER_IF_ICBOF1_DEFAULT << 9)  /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_ICBOF2                            (0x1UL << 10)                    /**< CC Channel 2 Input Capture Buffer Overflow Interrupt Flag */
#define _TIMER_IF_ICBOF2_SHIFT                     10                               /**< Shift value for TIMER_ICBOF2 */
#define _TIMER_IF_ICBOF2_MASK                      0x400UL                          /**< Bit mask for TIMER_ICBOF2 */
#define _TIMER_IF_ICBOF2_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_ICBOF2_DEFAULT                    (_TIMER_IF_ICBOF2_DEFAULT << 10) /**< Shifted mode DEFAULT for TIMER_IF */
#define TIMER_IF_ICBOF3                            (0x1UL << 11)                    /**< CC Channel 3 Input Capture Buffer Overflow Interrupt Flag */
#define _TIMER_IF_ICBOF3_SHIFT                     11                               /**< Shift value for TIMER_ICBOF3 */
#define _TIMER_IF_ICBOF3_MASK                      0x800UL                          /**< Bit mask for TIMER_ICBOF3 */
#define _TIMER_IF_ICBOF3_DEFAULT                   0x00000000UL                     /**< Mode DEFAULT for TIMER_IF */
#define TIMER_IF_ICBOF3_DEFAULT                    (_TIMER_IF_ICBOF3_DEFAULT << 11) /**< Shifted mode DEFAULT for TIMER_IF */

/* Bit fields for TIMER IFS */
#define _TIMER_IFS_RESETVALUE                      0x00000000UL                      /**< Default value for TIMER_IFS */
#define _TIMER_IFS_MASK                            0x00000FF7UL                      /**< Mask for TIMER_IFS */
#define TIMER_IFS_OF                               (0x1UL << 0)                      /**< Set OF Interrupt Flag */
#define _TIMER_IFS_OF_SHIFT                        0                                 /**< Shift value for TIMER_OF */
#define _TIMER_IFS_OF_MASK                         0x1UL                             /**< Bit mask for TIMER_OF */
#define _TIMER_IFS_OF_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_OF_DEFAULT                       (_TIMER_IFS_OF_DEFAULT << 0)      /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_UF                               (0x1UL << 1)                      /**< Set UF Interrupt Flag */
#define _TIMER_IFS_UF_SHIFT                        1                                 /**< Shift value for TIMER_UF */
#define _TIMER_IFS_UF_MASK                         0x2UL                             /**< Bit mask for TIMER_UF */
#define _TIMER_IFS_UF_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_UF_DEFAULT                       (_TIMER_IFS_UF_DEFAULT << 1)      /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_DIRCHG                           (0x1UL << 2)                      /**< Set DIRCHG Interrupt Flag */
#define _TIMER_IFS_DIRCHG_SHIFT                    2                                 /**< Shift value for TIMER_DIRCHG */
#define _TIMER_IFS_DIRCHG_MASK                     0x4UL                             /**< Bit mask for TIMER_DIRCHG */
#define _TIMER_IFS_DIRCHG_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_DIRCHG_DEFAULT                   (_TIMER_IFS_DIRCHG_DEFAULT << 2)  /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_CC0                              (0x1UL << 4)                      /**< Set CC0 Interrupt Flag */
#define _TIMER_IFS_CC0_SHIFT                       4                                 /**< Shift value for TIMER_CC0 */
#define _TIMER_IFS_CC0_MASK                        0x10UL                            /**< Bit mask for TIMER_CC0 */
#define _TIMER_IFS_CC0_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_CC0_DEFAULT                      (_TIMER_IFS_CC0_DEFAULT << 4)     /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_CC1                              (0x1UL << 5)                      /**< Set CC1 Interrupt Flag */
#define _TIMER_IFS_CC1_SHIFT                       5                                 /**< Shift value for TIMER_CC1 */
#define _TIMER_IFS_CC1_MASK                        0x20UL                            /**< Bit mask for TIMER_CC1 */
#define _TIMER_IFS_CC1_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_CC1_DEFAULT                      (_TIMER_IFS_CC1_DEFAULT << 5)     /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_CC2                              (0x1UL << 6)                      /**< Set CC2 Interrupt Flag */
#define _TIMER_IFS_CC2_SHIFT                       6                                 /**< Shift value for TIMER_CC2 */
#define _TIMER_IFS_CC2_MASK                        0x40UL                            /**< Bit mask for TIMER_CC2 */
#define _TIMER_IFS_CC2_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_CC2_DEFAULT                      (_TIMER_IFS_CC2_DEFAULT << 6)     /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_CC3                              (0x1UL << 7)                      /**< Set CC3 Interrupt Flag */
#define _TIMER_IFS_CC3_SHIFT                       7                                 /**< Shift value for TIMER_CC3 */
#define _TIMER_IFS_CC3_MASK                        0x80UL                            /**< Bit mask for TIMER_CC3 */
#define _TIMER_IFS_CC3_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_CC3_DEFAULT                      (_TIMER_IFS_CC3_DEFAULT << 7)     /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_ICBOF0                           (0x1UL << 8)                      /**< Set ICBOF0 Interrupt Flag */
#define _TIMER_IFS_ICBOF0_SHIFT                    8                                 /**< Shift value for TIMER_ICBOF0 */
#define _TIMER_IFS_ICBOF0_MASK                     0x100UL                           /**< Bit mask for TIMER_ICBOF0 */
#define _TIMER_IFS_ICBOF0_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_ICBOF0_DEFAULT                   (_TIMER_IFS_ICBOF0_DEFAULT << 8)  /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_ICBOF1                           (0x1UL << 9)                      /**< Set ICBOF1 Interrupt Flag */
#define _TIMER_IFS_ICBOF1_SHIFT                    9                                 /**< Shift value for TIMER_ICBOF1 */
#define _TIMER_IFS_ICBOF1_MASK                     0x200UL                           /**< Bit mask for TIMER_ICBOF1 */
#define _TIMER_IFS_ICBOF1_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_ICBOF1_DEFAULT                   (_TIMER_IFS_ICBOF1_DEFAULT << 9)  /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_ICBOF2                           (0x1UL << 10)                     /**< Set ICBOF2 Interrupt Flag */
#define _TIMER_IFS_ICBOF2_SHIFT                    10                                /**< Shift value for TIMER_ICBOF2 */
#define _TIMER_IFS_ICBOF2_MASK                     0x400UL                           /**< Bit mask for TIMER_ICBOF2 */
#define _TIMER_IFS_ICBOF2_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_ICBOF2_DEFAULT                   (_TIMER_IFS_ICBOF2_DEFAULT << 10) /**< Shifted mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_ICBOF3                           (0x1UL << 11)                     /**< Set ICBOF3 Interrupt Flag */
#define _TIMER_IFS_ICBOF3_SHIFT                    11                                /**< Shift value for TIMER_ICBOF3 */
#define _TIMER_IFS_ICBOF3_MASK                     0x800UL                           /**< Bit mask for TIMER_ICBOF3 */
#define _TIMER_IFS_ICBOF3_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFS */
#define TIMER_IFS_ICBOF3_DEFAULT                   (_TIMER_IFS_ICBOF3_DEFAULT << 11) /**< Shifted mode DEFAULT for TIMER_IFS */

/* Bit fields for TIMER IFC */
#define _TIMER_IFC_RESETVALUE                      0x00000000UL                      /**< Default value for TIMER_IFC */
#define _TIMER_IFC_MASK                            0x00000FF7UL                      /**< Mask for TIMER_IFC */
#define TIMER_IFC_OF                               (0x1UL << 0)                      /**< Clear OF Interrupt Flag */
#define _TIMER_IFC_OF_SHIFT                        0                                 /**< Shift value for TIMER_OF */
#define _TIMER_IFC_OF_MASK                         0x1UL                             /**< Bit mask for TIMER_OF */
#define _TIMER_IFC_OF_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_OF_DEFAULT                       (_TIMER_IFC_OF_DEFAULT << 0)      /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_UF                               (0x1UL << 1)                      /**< Clear UF Interrupt Flag */
#define _TIMER_IFC_UF_SHIFT                        1                                 /**< Shift value for TIMER_UF */
#define _TIMER_IFC_UF_MASK                         0x2UL                             /**< Bit mask for TIMER_UF */
#define _TIMER_IFC_UF_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_UF_DEFAULT                       (_TIMER_IFC_UF_DEFAULT << 1)      /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_DIRCHG                           (0x1UL << 2)                      /**< Clear DIRCHG Interrupt Flag */
#define _TIMER_IFC_DIRCHG_SHIFT                    2                                 /**< Shift value for TIMER_DIRCHG */
#define _TIMER_IFC_DIRCHG_MASK                     0x4UL                             /**< Bit mask for TIMER_DIRCHG */
#define _TIMER_IFC_DIRCHG_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_DIRCHG_DEFAULT                   (_TIMER_IFC_DIRCHG_DEFAULT << 2)  /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_CC0                              (0x1UL << 4)                      /**< Clear CC0 Interrupt Flag */
#define _TIMER_IFC_CC0_SHIFT                       4                                 /**< Shift value for TIMER_CC0 */
#define _TIMER_IFC_CC0_MASK                        0x10UL                            /**< Bit mask for TIMER_CC0 */
#define _TIMER_IFC_CC0_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_CC0_DEFAULT                      (_TIMER_IFC_CC0_DEFAULT << 4)     /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_CC1                              (0x1UL << 5)                      /**< Clear CC1 Interrupt Flag */
#define _TIMER_IFC_CC1_SHIFT                       5                                 /**< Shift value for TIMER_CC1 */
#define _TIMER_IFC_CC1_MASK                        0x20UL                            /**< Bit mask for TIMER_CC1 */
#define _TIMER_IFC_CC1_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_CC1_DEFAULT                      (_TIMER_IFC_CC1_DEFAULT << 5)     /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_CC2                              (0x1UL << 6)                      /**< Clear CC2 Interrupt Flag */
#define _TIMER_IFC_CC2_SHIFT                       6                                 /**< Shift value for TIMER_CC2 */
#define _TIMER_IFC_CC2_MASK                        0x40UL                            /**< Bit mask for TIMER_CC2 */
#define _TIMER_IFC_CC2_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_CC2_DEFAULT                      (_TIMER_IFC_CC2_DEFAULT << 6)     /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_CC3                              (0x1UL << 7)                      /**< Clear CC3 Interrupt Flag */
#define _TIMER_IFC_CC3_SHIFT                       7                                 /**< Shift value for TIMER_CC3 */
#define _TIMER_IFC_CC3_MASK                        0x80UL                            /**< Bit mask for TIMER_CC3 */
#define _TIMER_IFC_CC3_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_CC3_DEFAULT                      (_TIMER_IFC_CC3_DEFAULT << 7)     /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_ICBOF0                           (0x1UL << 8)                      /**< Clear ICBOF0 Interrupt Flag */
#define _TIMER_IFC_ICBOF0_SHIFT                    8                                 /**< Shift value for TIMER_ICBOF0 */
#define _TIMER_IFC_ICBOF0_MASK                     0x100UL                           /**< Bit mask for TIMER_ICBOF0 */
#define _TIMER_IFC_ICBOF0_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_ICBOF0_DEFAULT                   (_TIMER_IFC_ICBOF0_DEFAULT << 8)  /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_ICBOF1                           (0x1UL << 9)                      /**< Clear ICBOF1 Interrupt Flag */
#define _TIMER_IFC_ICBOF1_SHIFT                    9                                 /**< Shift value for TIMER_ICBOF1 */
#define _TIMER_IFC_ICBOF1_MASK                     0x200UL                           /**< Bit mask for TIMER_ICBOF1 */
#define _TIMER_IFC_ICBOF1_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_ICBOF1_DEFAULT                   (_TIMER_IFC_ICBOF1_DEFAULT << 9)  /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_ICBOF2                           (0x1UL << 10)                     /**< Clear ICBOF2 Interrupt Flag */
#define _TIMER_IFC_ICBOF2_SHIFT                    10                                /**< Shift value for TIMER_ICBOF2 */
#define _TIMER_IFC_ICBOF2_MASK                     0x400UL                           /**< Bit mask for TIMER_ICBOF2 */
#define _TIMER_IFC_ICBOF2_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_ICBOF2_DEFAULT                   (_TIMER_IFC_ICBOF2_DEFAULT << 10) /**< Shifted mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_ICBOF3                           (0x1UL << 11)                     /**< Clear ICBOF3 Interrupt Flag */
#define _TIMER_IFC_ICBOF3_SHIFT                    11                                /**< Shift value for TIMER_ICBOF3 */
#define _TIMER_IFC_ICBOF3_MASK                     0x800UL                           /**< Bit mask for TIMER_ICBOF3 */
#define _TIMER_IFC_ICBOF3_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IFC */
#define TIMER_IFC_ICBOF3_DEFAULT                   (_TIMER_IFC_ICBOF3_DEFAULT << 11) /**< Shifted mode DEFAULT for TIMER_IFC */

/* Bit fields for TIMER IEN */
#define _TIMER_IEN_RESETVALUE                      0x00000000UL                      /**< Default value for TIMER_IEN */
#define _TIMER_IEN_MASK                            0x00000FF7UL                      /**< Mask for TIMER_IEN */
#define TIMER_IEN_OF                               (0x1UL << 0)                      /**< OF Interrupt Enable */
#define _TIMER_IEN_OF_SHIFT                        0                                 /**< Shift value for TIMER_OF */
#define _TIMER_IEN_OF_MASK                         0x1UL                             /**< Bit mask for TIMER_OF */
#define _TIMER_IEN_OF_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_OF_DEFAULT                       (_TIMER_IEN_OF_DEFAULT << 0)      /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_UF                               (0x1UL << 1)                      /**< UF Interrupt Enable */
#define _TIMER_IEN_UF_SHIFT                        1                                 /**< Shift value for TIMER_UF */
#define _TIMER_IEN_UF_MASK                         0x2UL                             /**< Bit mask for TIMER_UF */
#define _TIMER_IEN_UF_DEFAULT                      0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_UF_DEFAULT                       (_TIMER_IEN_UF_DEFAULT << 1)      /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_DIRCHG                           (0x1UL << 2)                      /**< DIRCHG Interrupt Enable */
#define _TIMER_IEN_DIRCHG_SHIFT                    2                                 /**< Shift value for TIMER_DIRCHG */
#define _TIMER_IEN_DIRCHG_MASK                     0x4UL                             /**< Bit mask for TIMER_DIRCHG */
#define _TIMER_IEN_DIRCHG_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_DIRCHG_DEFAULT                   (_TIMER_IEN_DIRCHG_DEFAULT << 2)  /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_CC0                              (0x1UL << 4)                      /**< CC0 Interrupt Enable */
#define _TIMER_IEN_CC0_SHIFT                       4                                 /**< Shift value for TIMER_CC0 */
#define _TIMER_IEN_CC0_MASK                        0x10UL                            /**< Bit mask for TIMER_CC0 */
#define _TIMER_IEN_CC0_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_CC0_DEFAULT                      (_TIMER_IEN_CC0_DEFAULT << 4)     /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_CC1                              (0x1UL << 5)                      /**< CC1 Interrupt Enable */
#define _TIMER_IEN_CC1_SHIFT                       5                                 /**< Shift value for TIMER_CC1 */
#define _TIMER_IEN_CC1_MASK                        0x20UL                            /**< Bit mask for TIMER_CC1 */
#define _TIMER_IEN_CC1_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_CC1_DEFAULT                      (_TIMER_IEN_CC1_DEFAULT << 5)     /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_CC2                              (0x1UL << 6)                      /**< CC2 Interrupt Enable */
#define _TIMER_IEN_CC2_SHIFT                       6                                 /**< Shift value for TIMER_CC2 */
#define _TIMER_IEN_CC2_MASK                        0x40UL                            /**< Bit mask for TIMER_CC2 */
#define _TIMER_IEN_CC2_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_CC2_DEFAULT                      (_TIMER_IEN_CC2_DEFAULT << 6)     /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_CC3                              (0x1UL << 7)                      /**< CC3 Interrupt Enable */
#define _TIMER_IEN_CC3_SHIFT                       7                                 /**< Shift value for TIMER_CC3 */
#define _TIMER_IEN_CC3_MASK                        0x80UL                            /**< Bit mask for TIMER_CC3 */
#define _TIMER_IEN_CC3_DEFAULT                     0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_CC3_DEFAULT                      (_TIMER_IEN_CC3_DEFAULT << 7)     /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_ICBOF0                           (0x1UL << 8)                      /**< ICBOF0 Interrupt Enable */
#define _TIMER_IEN_ICBOF0_SHIFT                    8                                 /**< Shift value for TIMER_ICBOF0 */
#define _TIMER_IEN_ICBOF0_MASK                     0x100UL                           /**< Bit mask for TIMER_ICBOF0 */
#define _TIMER_IEN_ICBOF0_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_ICBOF0_DEFAULT                   (_TIMER_IEN_ICBOF0_DEFAULT << 8)  /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_ICBOF1                           (0x1UL << 9)                      /**< ICBOF1 Interrupt Enable */
#define _TIMER_IEN_ICBOF1_SHIFT                    9                                 /**< Shift value for TIMER_ICBOF1 */
#define _TIMER_IEN_ICBOF1_MASK                     0x200UL                           /**< Bit mask for TIMER_ICBOF1 */
#define _TIMER_IEN_ICBOF1_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_ICBOF1_DEFAULT                   (_TIMER_IEN_ICBOF1_DEFAULT << 9)  /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_ICBOF2                           (0x1UL << 10)                     /**< ICBOF2 Interrupt Enable */
#define _TIMER_IEN_ICBOF2_SHIFT                    10                                /**< Shift value for TIMER_ICBOF2 */
#define _TIMER_IEN_ICBOF2_MASK                     0x400UL                           /**< Bit mask for TIMER_ICBOF2 */
#define _TIMER_IEN_ICBOF2_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_ICBOF2_DEFAULT                   (_TIMER_IEN_ICBOF2_DEFAULT << 10) /**< Shifted mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_ICBOF3                           (0x1UL << 11)                     /**< ICBOF3 Interrupt Enable */
#define _TIMER_IEN_ICBOF3_SHIFT                    11                                /**< Shift value for TIMER_ICBOF3 */
#define _TIMER_IEN_ICBOF3_MASK                     0x800UL                           /**< Bit mask for TIMER_ICBOF3 */
#define _TIMER_IEN_ICBOF3_DEFAULT                  0x00000000UL                      /**< Mode DEFAULT for TIMER_IEN */
#define TIMER_IEN_ICBOF3_DEFAULT                   (_TIMER_IEN_ICBOF3_DEFAULT << 11) /**< Shifted mode DEFAULT for TIMER_IEN */

/* Bit fields for TIMER TOP */
#define _TIMER_TOP_RESETVALUE                      0x0000FFFFUL                  /**< Default value for TIMER_TOP */
#define _TIMER_TOP_MASK                            0x0000FFFFUL                  /**< Mask for TIMER_TOP */
#define _TIMER_TOP_TOP_SHIFT                       0                             /**< Shift value for TIMER_TOP */
#define _TIMER_TOP_TOP_MASK                        0xFFFFUL                      /**< Bit mask for TIMER_TOP */
#define _TIMER_TOP_TOP_DEFAULT                     0x0000FFFFUL                  /**< Mode DEFAULT for TIMER_TOP */
#define TIMER_TOP_TOP_DEFAULT                      (_TIMER_TOP_TOP_DEFAULT << 0) /**< Shifted mode DEFAULT for TIMER_TOP */

/* Bit fields for TIMER TOPB */
#define _TIMER_TOPB_RESETVALUE                     0x00000000UL                    /**< Default value for TIMER_TOPB */
#define _TIMER_TOPB_MASK                           0x0000FFFFUL                    /**< Mask for TIMER_TOPB */
#define _TIMER_TOPB_TOPB_SHIFT                     0                               /**< Shift value for TIMER_TOPB */
#define _TIMER_TOPB_TOPB_MASK                      0xFFFFUL                        /**< Bit mask for TIMER_TOPB */
#define _TIMER_TOPB_TOPB_DEFAULT                   0x00000000UL                    /**< Mode DEFAULT for TIMER_TOPB */
#define TIMER_TOPB_TOPB_DEFAULT                    (_TIMER_TOPB_TOPB_DEFAULT << 0) /**< Shifted mode DEFAULT for TIMER_TOPB */

/* Bit fields for TIMER CNT */
#define _TIMER_CNT_RESETVALUE                      0x00000000UL                  /**< Default value for TIMER_CNT */
#define _TIMER_CNT_MASK                            0x0000FFFFUL                  /**< Mask for TIMER_CNT */
#define _TIMER_CNT_CNT_SHIFT                       0                             /**< Shift value for TIMER_CNT */
#define _TIMER_CNT_CNT_MASK                        0xFFFFUL                      /**< Bit mask for TIMER_CNT */
#define _TIMER_CNT_CNT_DEFAULT                     0x00000000UL                  /**< Mode DEFAULT for TIMER_CNT */
#define TIMER_CNT_CNT_DEFAULT                      (_TIMER_CNT_CNT_DEFAULT << 0) /**< Shifted mode DEFAULT for TIMER_CNT */

/* Bit fields for TIMER LOCK */
#define _TIMER_LOCK_RESETVALUE                     0x00000000UL                             /**< Default value for TIMER_LOCK */
#define _TIMER_LOCK_MASK                           0x0000FFFFUL                             /**< Mask for TIMER_LOCK */
#define _TIMER_LOCK_TIMERLOCKKEY_SHIFT             0                                        /**< Shift value for TIMER_TIMERLOCKKEY */
#define _TIMER_LOCK_TIMERLOCKKEY_MASK              0xFFFFUL                                 /**< Bit mask for TIMER_TIMERLOCKKEY */
#define _TIMER_LOCK_TIMERLOCKKEY_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for TIMER_LOCK */
#define _TIMER_LOCK_TIMERLOCKKEY_LOCK              0x00000000UL                             /**< Mode LOCK for TIMER_LOCK */
#define _TIMER_LOCK_TIMERLOCKKEY_UNLOCKED          0x00000000UL                             /**< Mode UNLOCKED for TIMER_LOCK */
#define _TIMER_LOCK_TIMERLOCKKEY_LOCKED            0x00000001UL                             /**< Mode LOCKED for TIMER_LOCK */
#define _TIMER_LOCK_TIMERLOCKKEY_UNLOCK            0x0000CE80UL                             /**< Mode UNLOCK for TIMER_LOCK */
#define TIMER_LOCK_TIMERLOCKKEY_DEFAULT            (_TIMER_LOCK_TIMERLOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for TIMER_LOCK */
#define TIMER_LOCK_TIMERLOCKKEY_LOCK               (_TIMER_LOCK_TIMERLOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for TIMER_LOCK */
#define TIMER_LOCK_TIMERLOCKKEY_UNLOCKED           (_TIMER_LOCK_TIMERLOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for TIMER_LOCK */
#define TIMER_LOCK_TIMERLOCKKEY_LOCKED             (_TIMER_LOCK_TIMERLOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for TIMER_LOCK */
#define TIMER_LOCK_TIMERLOCKKEY_UNLOCK             (_TIMER_LOCK_TIMERLOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for TIMER_LOCK */

/* Bit fields for TIMER ROUTEPEN */
#define _TIMER_ROUTEPEN_RESETVALUE                 0x00000000UL                             /**< Default value for TIMER_ROUTEPEN */
#define _TIMER_ROUTEPEN_MASK                       0x0000070FUL                             /**< Mask for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CC0PEN                      (0x1UL << 0)                             /**< CC Channel 0 Pin Enable */
#define _TIMER_ROUTEPEN_CC0PEN_SHIFT               0                                        /**< Shift value for TIMER_CC0PEN */
#define _TIMER_ROUTEPEN_CC0PEN_MASK                0x1UL                                    /**< Bit mask for TIMER_CC0PEN */
#define _TIMER_ROUTEPEN_CC0PEN_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CC0PEN_DEFAULT              (_TIMER_ROUTEPEN_CC0PEN_DEFAULT << 0)    /**< Shifted mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CC1PEN                      (0x1UL << 1)                             /**< CC Channel 1 Pin Enable */
#define _TIMER_ROUTEPEN_CC1PEN_SHIFT               1                                        /**< Shift value for TIMER_CC1PEN */
#define _TIMER_ROUTEPEN_CC1PEN_MASK                0x2UL                                    /**< Bit mask for TIMER_CC1PEN */
#define _TIMER_ROUTEPEN_CC1PEN_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CC1PEN_DEFAULT              (_TIMER_ROUTEPEN_CC1PEN_DEFAULT << 1)    /**< Shifted mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CC2PEN                      (0x1UL << 2)                             /**< CC Channel 2 Pin Enable */
#define _TIMER_ROUTEPEN_CC2PEN_SHIFT               2                                        /**< Shift value for TIMER_CC2PEN */
#define _TIMER_ROUTEPEN_CC2PEN_MASK                0x4UL                                    /**< Bit mask for TIMER_CC2PEN */
#define _TIMER_ROUTEPEN_CC2PEN_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CC2PEN_DEFAULT              (_TIMER_ROUTEPEN_CC2PEN_DEFAULT << 2)    /**< Shifted mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CC3PEN                      (0x1UL << 3)                             /**< CC Channel 3 Pin Enable */
#define _TIMER_ROUTEPEN_CC3PEN_SHIFT               3                                        /**< Shift value for TIMER_CC3PEN */
#define _TIMER_ROUTEPEN_CC3PEN_MASK                0x8UL                                    /**< Bit mask for TIMER_CC3PEN */
#define _TIMER_ROUTEPEN_CC3PEN_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CC3PEN_DEFAULT              (_TIMER_ROUTEPEN_CC3PEN_DEFAULT << 3)    /**< Shifted mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CDTI0PEN                    (0x1UL << 8)                             /**< CC Channel 0 Complementary Dead-Time Insertion Pin Enable */
#define _TIMER_ROUTEPEN_CDTI0PEN_SHIFT             8                                        /**< Shift value for TIMER_CDTI0PEN */
#define _TIMER_ROUTEPEN_CDTI0PEN_MASK              0x100UL                                  /**< Bit mask for TIMER_CDTI0PEN */
#define _TIMER_ROUTEPEN_CDTI0PEN_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CDTI0PEN_DEFAULT            (_TIMER_ROUTEPEN_CDTI0PEN_DEFAULT << 8)  /**< Shifted mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CDTI1PEN                    (0x1UL << 9)                             /**< CC Channel 1 Complementary Dead-Time Insertion Pin Enable */
#define _TIMER_ROUTEPEN_CDTI1PEN_SHIFT             9                                        /**< Shift value for TIMER_CDTI1PEN */
#define _TIMER_ROUTEPEN_CDTI1PEN_MASK              0x200UL                                  /**< Bit mask for TIMER_CDTI1PEN */
#define _TIMER_ROUTEPEN_CDTI1PEN_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CDTI1PEN_DEFAULT            (_TIMER_ROUTEPEN_CDTI1PEN_DEFAULT << 9)  /**< Shifted mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CDTI2PEN                    (0x1UL << 10)                            /**< CC Channel 2 Complementary Dead-Time Insertion Pin Enable */
#define _TIMER_ROUTEPEN_CDTI2PEN_SHIFT             10                                       /**< Shift value for TIMER_CDTI2PEN */
#define _TIMER_ROUTEPEN_CDTI2PEN_MASK              0x400UL                                  /**< Bit mask for TIMER_CDTI2PEN */
#define _TIMER_ROUTEPEN_CDTI2PEN_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for TIMER_ROUTEPEN */
#define TIMER_ROUTEPEN_CDTI2PEN_DEFAULT            (_TIMER_ROUTEPEN_CDTI2PEN_DEFAULT << 10) /**< Shifted mode DEFAULT for TIMER_ROUTEPEN */

/* Bit fields for TIMER ROUTELOC0 */
#define _TIMER_ROUTELOC0_RESETVALUE                0x00000000UL                            /**< Default value for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_MASK                      0x1F1F1F1FUL                            /**< Mask for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_SHIFT              0                                       /**< Shift value for TIMER_CC0LOC */
#define _TIMER_ROUTELOC0_CC0LOC_MASK               0x1FUL                                  /**< Bit mask for TIMER_CC0LOC */
#define _TIMER_ROUTELOC0_CC0LOC_LOC0               0x00000000UL                            /**< Mode LOC0 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC1               0x00000001UL                            /**< Mode LOC1 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC2               0x00000002UL                            /**< Mode LOC2 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC3               0x00000003UL                            /**< Mode LOC3 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC4               0x00000004UL                            /**< Mode LOC4 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC5               0x00000005UL                            /**< Mode LOC5 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC6               0x00000006UL                            /**< Mode LOC6 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC7               0x00000007UL                            /**< Mode LOC7 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC8               0x00000008UL                            /**< Mode LOC8 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC9               0x00000009UL                            /**< Mode LOC9 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC10              0x0000000AUL                            /**< Mode LOC10 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC11              0x0000000BUL                            /**< Mode LOC11 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC12              0x0000000CUL                            /**< Mode LOC12 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC13              0x0000000DUL                            /**< Mode LOC13 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC14              0x0000000EUL                            /**< Mode LOC14 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC15              0x0000000FUL                            /**< Mode LOC15 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC16              0x00000010UL                            /**< Mode LOC16 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC17              0x00000011UL                            /**< Mode LOC17 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC18              0x00000012UL                            /**< Mode LOC18 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC19              0x00000013UL                            /**< Mode LOC19 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC20              0x00000014UL                            /**< Mode LOC20 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC21              0x00000015UL                            /**< Mode LOC21 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC22              0x00000016UL                            /**< Mode LOC22 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC23              0x00000017UL                            /**< Mode LOC23 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC24              0x00000018UL                            /**< Mode LOC24 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC25              0x00000019UL                            /**< Mode LOC25 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC26              0x0000001AUL                            /**< Mode LOC26 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC27              0x0000001BUL                            /**< Mode LOC27 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC28              0x0000001CUL                            /**< Mode LOC28 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC29              0x0000001DUL                            /**< Mode LOC29 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC30              0x0000001EUL                            /**< Mode LOC30 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC0LOC_LOC31              0x0000001FUL                            /**< Mode LOC31 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC0                (_TIMER_ROUTELOC0_CC0LOC_LOC0 << 0)     /**< Shifted mode LOC0 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_DEFAULT             (_TIMER_ROUTELOC0_CC0LOC_DEFAULT << 0)  /**< Shifted mode DEFAULT for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC1                (_TIMER_ROUTELOC0_CC0LOC_LOC1 << 0)     /**< Shifted mode LOC1 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC2                (_TIMER_ROUTELOC0_CC0LOC_LOC2 << 0)     /**< Shifted mode LOC2 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC3                (_TIMER_ROUTELOC0_CC0LOC_LOC3 << 0)     /**< Shifted mode LOC3 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC4                (_TIMER_ROUTELOC0_CC0LOC_LOC4 << 0)     /**< Shifted mode LOC4 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC5                (_TIMER_ROUTELOC0_CC0LOC_LOC5 << 0)     /**< Shifted mode LOC5 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC6                (_TIMER_ROUTELOC0_CC0LOC_LOC6 << 0)     /**< Shifted mode LOC6 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC7                (_TIMER_ROUTELOC0_CC0LOC_LOC7 << 0)     /**< Shifted mode LOC7 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC8                (_TIMER_ROUTELOC0_CC0LOC_LOC8 << 0)     /**< Shifted mode LOC8 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC9                (_TIMER_ROUTELOC0_CC0LOC_LOC9 << 0)     /**< Shifted mode LOC9 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC10               (_TIMER_ROUTELOC0_CC0LOC_LOC10 << 0)    /**< Shifted mode LOC10 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC11               (_TIMER_ROUTELOC0_CC0LOC_LOC11 << 0)    /**< Shifted mode LOC11 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC12               (_TIMER_ROUTELOC0_CC0LOC_LOC12 << 0)    /**< Shifted mode LOC12 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC13               (_TIMER_ROUTELOC0_CC0LOC_LOC13 << 0)    /**< Shifted mode LOC13 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC14               (_TIMER_ROUTELOC0_CC0LOC_LOC14 << 0)    /**< Shifted mode LOC14 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC15               (_TIMER_ROUTELOC0_CC0LOC_LOC15 << 0)    /**< Shifted mode LOC15 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC16               (_TIMER_ROUTELOC0_CC0LOC_LOC16 << 0)    /**< Shifted mode LOC16 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC17               (_TIMER_ROUTELOC0_CC0LOC_LOC17 << 0)    /**< Shifted mode LOC17 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC18               (_TIMER_ROUTELOC0_CC0LOC_LOC18 << 0)    /**< Shifted mode LOC18 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC19               (_TIMER_ROUTELOC0_CC0LOC_LOC19 << 0)    /**< Shifted mode LOC19 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC20               (_TIMER_ROUTELOC0_CC0LOC_LOC20 << 0)    /**< Shifted mode LOC20 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC21               (_TIMER_ROUTELOC0_CC0LOC_LOC21 << 0)    /**< Shifted mode LOC21 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC22               (_TIMER_ROUTELOC0_CC0LOC_LOC22 << 0)    /**< Shifted mode LOC22 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC23               (_TIMER_ROUTELOC0_CC0LOC_LOC23 << 0)    /**< Shifted mode LOC23 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC24               (_TIMER_ROUTELOC0_CC0LOC_LOC24 << 0)    /**< Shifted mode LOC24 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC25               (_TIMER_ROUTELOC0_CC0LOC_LOC25 << 0)    /**< Shifted mode LOC25 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC26               (_TIMER_ROUTELOC0_CC0LOC_LOC26 << 0)    /**< Shifted mode LOC26 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC27               (_TIMER_ROUTELOC0_CC0LOC_LOC27 << 0)    /**< Shifted mode LOC27 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC28               (_TIMER_ROUTELOC0_CC0LOC_LOC28 << 0)    /**< Shifted mode LOC28 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC29               (_TIMER_ROUTELOC0_CC0LOC_LOC29 << 0)    /**< Shifted mode LOC29 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC30               (_TIMER_ROUTELOC0_CC0LOC_LOC30 << 0)    /**< Shifted mode LOC30 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC0LOC_LOC31               (_TIMER_ROUTELOC0_CC0LOC_LOC31 << 0)    /**< Shifted mode LOC31 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_SHIFT              8                                       /**< Shift value for TIMER_CC1LOC */
#define _TIMER_ROUTELOC0_CC1LOC_MASK               0x1F00UL                                /**< Bit mask for TIMER_CC1LOC */
#define _TIMER_ROUTELOC0_CC1LOC_LOC0               0x00000000UL                            /**< Mode LOC0 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC1               0x00000001UL                            /**< Mode LOC1 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC2               0x00000002UL                            /**< Mode LOC2 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC3               0x00000003UL                            /**< Mode LOC3 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC4               0x00000004UL                            /**< Mode LOC4 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC5               0x00000005UL                            /**< Mode LOC5 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC6               0x00000006UL                            /**< Mode LOC6 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC7               0x00000007UL                            /**< Mode LOC7 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC8               0x00000008UL                            /**< Mode LOC8 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC9               0x00000009UL                            /**< Mode LOC9 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC10              0x0000000AUL                            /**< Mode LOC10 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC11              0x0000000BUL                            /**< Mode LOC11 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC12              0x0000000CUL                            /**< Mode LOC12 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC13              0x0000000DUL                            /**< Mode LOC13 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC14              0x0000000EUL                            /**< Mode LOC14 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC15              0x0000000FUL                            /**< Mode LOC15 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC16              0x00000010UL                            /**< Mode LOC16 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC17              0x00000011UL                            /**< Mode LOC17 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC18              0x00000012UL                            /**< Mode LOC18 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC19              0x00000013UL                            /**< Mode LOC19 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC20              0x00000014UL                            /**< Mode LOC20 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC21              0x00000015UL                            /**< Mode LOC21 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC22              0x00000016UL                            /**< Mode LOC22 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC23              0x00000017UL                            /**< Mode LOC23 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC24              0x00000018UL                            /**< Mode LOC24 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC25              0x00000019UL                            /**< Mode LOC25 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC26              0x0000001AUL                            /**< Mode LOC26 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC27              0x0000001BUL                            /**< Mode LOC27 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC28              0x0000001CUL                            /**< Mode LOC28 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC29              0x0000001DUL                            /**< Mode LOC29 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC30              0x0000001EUL                            /**< Mode LOC30 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC1LOC_LOC31              0x0000001FUL                            /**< Mode LOC31 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC0                (_TIMER_ROUTELOC0_CC1LOC_LOC0 << 8)     /**< Shifted mode LOC0 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_DEFAULT             (_TIMER_ROUTELOC0_CC1LOC_DEFAULT << 8)  /**< Shifted mode DEFAULT for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC1                (_TIMER_ROUTELOC0_CC1LOC_LOC1 << 8)     /**< Shifted mode LOC1 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC2                (_TIMER_ROUTELOC0_CC1LOC_LOC2 << 8)     /**< Shifted mode LOC2 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC3                (_TIMER_ROUTELOC0_CC1LOC_LOC3 << 8)     /**< Shifted mode LOC3 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC4                (_TIMER_ROUTELOC0_CC1LOC_LOC4 << 8)     /**< Shifted mode LOC4 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC5                (_TIMER_ROUTELOC0_CC1LOC_LOC5 << 8)     /**< Shifted mode LOC5 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC6                (_TIMER_ROUTELOC0_CC1LOC_LOC6 << 8)     /**< Shifted mode LOC6 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC7                (_TIMER_ROUTELOC0_CC1LOC_LOC7 << 8)     /**< Shifted mode LOC7 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC8                (_TIMER_ROUTELOC0_CC1LOC_LOC8 << 8)     /**< Shifted mode LOC8 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC9                (_TIMER_ROUTELOC0_CC1LOC_LOC9 << 8)     /**< Shifted mode LOC9 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC10               (_TIMER_ROUTELOC0_CC1LOC_LOC10 << 8)    /**< Shifted mode LOC10 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC11               (_TIMER_ROUTELOC0_CC1LOC_LOC11 << 8)    /**< Shifted mode LOC11 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC12               (_TIMER_ROUTELOC0_CC1LOC_LOC12 << 8)    /**< Shifted mode LOC12 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC13               (_TIMER_ROUTELOC0_CC1LOC_LOC13 << 8)    /**< Shifted mode LOC13 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC14               (_TIMER_ROUTELOC0_CC1LOC_LOC14 << 8)    /**< Shifted mode LOC14 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC15               (_TIMER_ROUTELOC0_CC1LOC_LOC15 << 8)    /**< Shifted mode LOC15 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC16               (_TIMER_ROUTELOC0_CC1LOC_LOC16 << 8)    /**< Shifted mode LOC16 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC17               (_TIMER_ROUTELOC0_CC1LOC_LOC17 << 8)    /**< Shifted mode LOC17 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC18               (_TIMER_ROUTELOC0_CC1LOC_LOC18 << 8)    /**< Shifted mode LOC18 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC19               (_TIMER_ROUTELOC0_CC1LOC_LOC19 << 8)    /**< Shifted mode LOC19 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC20               (_TIMER_ROUTELOC0_CC1LOC_LOC20 << 8)    /**< Shifted mode LOC20 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC21               (_TIMER_ROUTELOC0_CC1LOC_LOC21 << 8)    /**< Shifted mode LOC21 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC22               (_TIMER_ROUTELOC0_CC1LOC_LOC22 << 8)    /**< Shifted mode LOC22 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC23               (_TIMER_ROUTELOC0_CC1LOC_LOC23 << 8)    /**< Shifted mode LOC23 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC24               (_TIMER_ROUTELOC0_CC1LOC_LOC24 << 8)    /**< Shifted mode LOC24 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC25               (_TIMER_ROUTELOC0_CC1LOC_LOC25 << 8)    /**< Shifted mode LOC25 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC26               (_TIMER_ROUTELOC0_CC1LOC_LOC26 << 8)    /**< Shifted mode LOC26 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC27               (_TIMER_ROUTELOC0_CC1LOC_LOC27 << 8)    /**< Shifted mode LOC27 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC28               (_TIMER_ROUTELOC0_CC1LOC_LOC28 << 8)    /**< Shifted mode LOC28 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC29               (_TIMER_ROUTELOC0_CC1LOC_LOC29 << 8)    /**< Shifted mode LOC29 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC30               (_TIMER_ROUTELOC0_CC1LOC_LOC30 << 8)    /**< Shifted mode LOC30 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC1LOC_LOC31               (_TIMER_ROUTELOC0_CC1LOC_LOC31 << 8)    /**< Shifted mode LOC31 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_SHIFT              16                                      /**< Shift value for TIMER_CC2LOC */
#define _TIMER_ROUTELOC0_CC2LOC_MASK               0x1F0000UL                              /**< Bit mask for TIMER_CC2LOC */
#define _TIMER_ROUTELOC0_CC2LOC_LOC0               0x00000000UL                            /**< Mode LOC0 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC1               0x00000001UL                            /**< Mode LOC1 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC2               0x00000002UL                            /**< Mode LOC2 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC3               0x00000003UL                            /**< Mode LOC3 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC4               0x00000004UL                            /**< Mode LOC4 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC5               0x00000005UL                            /**< Mode LOC5 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC6               0x00000006UL                            /**< Mode LOC6 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC7               0x00000007UL                            /**< Mode LOC7 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC8               0x00000008UL                            /**< Mode LOC8 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC9               0x00000009UL                            /**< Mode LOC9 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC10              0x0000000AUL                            /**< Mode LOC10 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC11              0x0000000BUL                            /**< Mode LOC11 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC12              0x0000000CUL                            /**< Mode LOC12 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC13              0x0000000DUL                            /**< Mode LOC13 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC14              0x0000000EUL                            /**< Mode LOC14 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC15              0x0000000FUL                            /**< Mode LOC15 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC16              0x00000010UL                            /**< Mode LOC16 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC17              0x00000011UL                            /**< Mode LOC17 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC18              0x00000012UL                            /**< Mode LOC18 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC19              0x00000013UL                            /**< Mode LOC19 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC20              0x00000014UL                            /**< Mode LOC20 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC21              0x00000015UL                            /**< Mode LOC21 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC22              0x00000016UL                            /**< Mode LOC22 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC23              0x00000017UL                            /**< Mode LOC23 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC24              0x00000018UL                            /**< Mode LOC24 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC25              0x00000019UL                            /**< Mode LOC25 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC26              0x0000001AUL                            /**< Mode LOC26 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC27              0x0000001BUL                            /**< Mode LOC27 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC28              0x0000001CUL                            /**< Mode LOC28 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC29              0x0000001DUL                            /**< Mode LOC29 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC30              0x0000001EUL                            /**< Mode LOC30 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC2LOC_LOC31              0x0000001FUL                            /**< Mode LOC31 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC0                (_TIMER_ROUTELOC0_CC2LOC_LOC0 << 16)    /**< Shifted mode LOC0 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_DEFAULT             (_TIMER_ROUTELOC0_CC2LOC_DEFAULT << 16) /**< Shifted mode DEFAULT for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC1                (_TIMER_ROUTELOC0_CC2LOC_LOC1 << 16)    /**< Shifted mode LOC1 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC2                (_TIMER_ROUTELOC0_CC2LOC_LOC2 << 16)    /**< Shifted mode LOC2 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC3                (_TIMER_ROUTELOC0_CC2LOC_LOC3 << 16)    /**< Shifted mode LOC3 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC4                (_TIMER_ROUTELOC0_CC2LOC_LOC4 << 16)    /**< Shifted mode LOC4 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC5                (_TIMER_ROUTELOC0_CC2LOC_LOC5 << 16)    /**< Shifted mode LOC5 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC6                (_TIMER_ROUTELOC0_CC2LOC_LOC6 << 16)    /**< Shifted mode LOC6 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC7                (_TIMER_ROUTELOC0_CC2LOC_LOC7 << 16)    /**< Shifted mode LOC7 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC8                (_TIMER_ROUTELOC0_CC2LOC_LOC8 << 16)    /**< Shifted mode LOC8 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC9                (_TIMER_ROUTELOC0_CC2LOC_LOC9 << 16)    /**< Shifted mode LOC9 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC10               (_TIMER_ROUTELOC0_CC2LOC_LOC10 << 16)   /**< Shifted mode LOC10 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC11               (_TIMER_ROUTELOC0_CC2LOC_LOC11 << 16)   /**< Shifted mode LOC11 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC12               (_TIMER_ROUTELOC0_CC2LOC_LOC12 << 16)   /**< Shifted mode LOC12 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC13               (_TIMER_ROUTELOC0_CC2LOC_LOC13 << 16)   /**< Shifted mode LOC13 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC14               (_TIMER_ROUTELOC0_CC2LOC_LOC14 << 16)   /**< Shifted mode LOC14 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC15               (_TIMER_ROUTELOC0_CC2LOC_LOC15 << 16)   /**< Shifted mode LOC15 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC16               (_TIMER_ROUTELOC0_CC2LOC_LOC16 << 16)   /**< Shifted mode LOC16 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC17               (_TIMER_ROUTELOC0_CC2LOC_LOC17 << 16)   /**< Shifted mode LOC17 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC18               (_TIMER_ROUTELOC0_CC2LOC_LOC18 << 16)   /**< Shifted mode LOC18 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC19               (_TIMER_ROUTELOC0_CC2LOC_LOC19 << 16)   /**< Shifted mode LOC19 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC20               (_TIMER_ROUTELOC0_CC2LOC_LOC20 << 16)   /**< Shifted mode LOC20 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC21               (_TIMER_ROUTELOC0_CC2LOC_LOC21 << 16)   /**< Shifted mode LOC21 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC22               (_TIMER_ROUTELOC0_CC2LOC_LOC22 << 16)   /**< Shifted mode LOC22 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC23               (_TIMER_ROUTELOC0_CC2LOC_LOC23 << 16)   /**< Shifted mode LOC23 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC24               (_TIMER_ROUTELOC0_CC2LOC_LOC24 << 16)   /**< Shifted mode LOC24 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC25               (_TIMER_ROUTELOC0_CC2LOC_LOC25 << 16)   /**< Shifted mode LOC25 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC26               (_TIMER_ROUTELOC0_CC2LOC_LOC26 << 16)   /**< Shifted mode LOC26 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC27               (_TIMER_ROUTELOC0_CC2LOC_LOC27 << 16)   /**< Shifted mode LOC27 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC28               (_TIMER_ROUTELOC0_CC2LOC_LOC28 << 16)   /**< Shifted mode LOC28 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC29               (_TIMER_ROUTELOC0_CC2LOC_LOC29 << 16)   /**< Shifted mode LOC29 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC30               (_TIMER_ROUTELOC0_CC2LOC_LOC30 << 16)   /**< Shifted mode LOC30 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC2LOC_LOC31               (_TIMER_ROUTELOC0_CC2LOC_LOC31 << 16)   /**< Shifted mode LOC31 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_SHIFT              24                                      /**< Shift value for TIMER_CC3LOC */
#define _TIMER_ROUTELOC0_CC3LOC_MASK               0x1F000000UL                            /**< Bit mask for TIMER_CC3LOC */
#define _TIMER_ROUTELOC0_CC3LOC_LOC0               0x00000000UL                            /**< Mode LOC0 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC1               0x00000001UL                            /**< Mode LOC1 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC2               0x00000002UL                            /**< Mode LOC2 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC3               0x00000003UL                            /**< Mode LOC3 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC4               0x00000004UL                            /**< Mode LOC4 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC5               0x00000005UL                            /**< Mode LOC5 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC6               0x00000006UL                            /**< Mode LOC6 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC7               0x00000007UL                            /**< Mode LOC7 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC8               0x00000008UL                            /**< Mode LOC8 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC9               0x00000009UL                            /**< Mode LOC9 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC10              0x0000000AUL                            /**< Mode LOC10 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC11              0x0000000BUL                            /**< Mode LOC11 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC12              0x0000000CUL                            /**< Mode LOC12 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC13              0x0000000DUL                            /**< Mode LOC13 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC14              0x0000000EUL                            /**< Mode LOC14 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC15              0x0000000FUL                            /**< Mode LOC15 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC16              0x00000010UL                            /**< Mode LOC16 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC17              0x00000011UL                            /**< Mode LOC17 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC18              0x00000012UL                            /**< Mode LOC18 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC19              0x00000013UL                            /**< Mode LOC19 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC20              0x00000014UL                            /**< Mode LOC20 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC21              0x00000015UL                            /**< Mode LOC21 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC22              0x00000016UL                            /**< Mode LOC22 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC23              0x00000017UL                            /**< Mode LOC23 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC24              0x00000018UL                            /**< Mode LOC24 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC25              0x00000019UL                            /**< Mode LOC25 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC26              0x0000001AUL                            /**< Mode LOC26 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC27              0x0000001BUL                            /**< Mode LOC27 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC28              0x0000001CUL                            /**< Mode LOC28 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC29              0x0000001DUL                            /**< Mode LOC29 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC30              0x0000001EUL                            /**< Mode LOC30 for TIMER_ROUTELOC0 */
#define _TIMER_ROUTELOC0_CC3LOC_LOC31              0x0000001FUL                            /**< Mode LOC31 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC0                (_TIMER_ROUTELOC0_CC3LOC_LOC0 << 24)    /**< Shifted mode LOC0 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_DEFAULT             (_TIMER_ROUTELOC0_CC3LOC_DEFAULT << 24) /**< Shifted mode DEFAULT for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC1                (_TIMER_ROUTELOC0_CC3LOC_LOC1 << 24)    /**< Shifted mode LOC1 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC2                (_TIMER_ROUTELOC0_CC3LOC_LOC2 << 24)    /**< Shifted mode LOC2 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC3                (_TIMER_ROUTELOC0_CC3LOC_LOC3 << 24)    /**< Shifted mode LOC3 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC4                (_TIMER_ROUTELOC0_CC3LOC_LOC4 << 24)    /**< Shifted mode LOC4 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC5                (_TIMER_ROUTELOC0_CC3LOC_LOC5 << 24)    /**< Shifted mode LOC5 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC6                (_TIMER_ROUTELOC0_CC3LOC_LOC6 << 24)    /**< Shifted mode LOC6 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC7                (_TIMER_ROUTELOC0_CC3LOC_LOC7 << 24)    /**< Shifted mode LOC7 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC8                (_TIMER_ROUTELOC0_CC3LOC_LOC8 << 24)    /**< Shifted mode LOC8 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC9                (_TIMER_ROUTELOC0_CC3LOC_LOC9 << 24)    /**< Shifted mode LOC9 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC10               (_TIMER_ROUTELOC0_CC3LOC_LOC10 << 24)   /**< Shifted mode LOC10 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC11               (_TIMER_ROUTELOC0_CC3LOC_LOC11 << 24)   /**< Shifted mode LOC11 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC12               (_TIMER_ROUTELOC0_CC3LOC_LOC12 << 24)   /**< Shifted mode LOC12 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC13               (_TIMER_ROUTELOC0_CC3LOC_LOC13 << 24)   /**< Shifted mode LOC13 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC14               (_TIMER_ROUTELOC0_CC3LOC_LOC14 << 24)   /**< Shifted mode LOC14 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC15               (_TIMER_ROUTELOC0_CC3LOC_LOC15 << 24)   /**< Shifted mode LOC15 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC16               (_TIMER_ROUTELOC0_CC3LOC_LOC16 << 24)   /**< Shifted mode LOC16 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC17               (_TIMER_ROUTELOC0_CC3LOC_LOC17 << 24)   /**< Shifted mode LOC17 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC18               (_TIMER_ROUTELOC0_CC3LOC_LOC18 << 24)   /**< Shifted mode LOC18 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC19               (_TIMER_ROUTELOC0_CC3LOC_LOC19 << 24)   /**< Shifted mode LOC19 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC20               (_TIMER_ROUTELOC0_CC3LOC_LOC20 << 24)   /**< Shifted mode LOC20 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC21               (_TIMER_ROUTELOC0_CC3LOC_LOC21 << 24)   /**< Shifted mode LOC21 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC22               (_TIMER_ROUTELOC0_CC3LOC_LOC22 << 24)   /**< Shifted mode LOC22 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC23               (_TIMER_ROUTELOC0_CC3LOC_LOC23 << 24)   /**< Shifted mode LOC23 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC24               (_TIMER_ROUTELOC0_CC3LOC_LOC24 << 24)   /**< Shifted mode LOC24 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC25               (_TIMER_ROUTELOC0_CC3LOC_LOC25 << 24)   /**< Shifted mode LOC25 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC26               (_TIMER_ROUTELOC0_CC3LOC_LOC26 << 24)   /**< Shifted mode LOC26 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC27               (_TIMER_ROUTELOC0_CC3LOC_LOC27 << 24)   /**< Shifted mode LOC27 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC28               (_TIMER_ROUTELOC0_CC3LOC_LOC28 << 24)   /**< Shifted mode LOC28 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC29               (_TIMER_ROUTELOC0_CC3LOC_LOC29 << 24)   /**< Shifted mode LOC29 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC30               (_TIMER_ROUTELOC0_CC3LOC_LOC30 << 24)   /**< Shifted mode LOC30 for TIMER_ROUTELOC0 */
#define TIMER_ROUTELOC0_CC3LOC_LOC31               (_TIMER_ROUTELOC0_CC3LOC_LOC31 << 24)   /**< Shifted mode LOC31 for TIMER_ROUTELOC0 */

/* Bit fields for TIMER ROUTELOC2 */
#define _TIMER_ROUTELOC2_RESETVALUE                0x00000000UL                              /**< Default value for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_MASK                      0x001F1F1FUL                              /**< Mask for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_SHIFT            0                                         /**< Shift value for TIMER_CDTI0LOC */
#define _TIMER_ROUTELOC2_CDTI0LOC_MASK             0x1FUL                                    /**< Bit mask for TIMER_CDTI0LOC */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC0             0x00000000UL                              /**< Mode LOC0 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC1             0x00000001UL                              /**< Mode LOC1 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC2             0x00000002UL                              /**< Mode LOC2 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC3             0x00000003UL                              /**< Mode LOC3 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC4             0x00000004UL                              /**< Mode LOC4 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC5             0x00000005UL                              /**< Mode LOC5 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC6             0x00000006UL                              /**< Mode LOC6 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC7             0x00000007UL                              /**< Mode LOC7 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC8             0x00000008UL                              /**< Mode LOC8 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC9             0x00000009UL                              /**< Mode LOC9 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC10            0x0000000AUL                              /**< Mode LOC10 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC11            0x0000000BUL                              /**< Mode LOC11 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC12            0x0000000CUL                              /**< Mode LOC12 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC13            0x0000000DUL                              /**< Mode LOC13 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC14            0x0000000EUL                              /**< Mode LOC14 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC15            0x0000000FUL                              /**< Mode LOC15 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC16            0x00000010UL                              /**< Mode LOC16 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC17            0x00000011UL                              /**< Mode LOC17 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC18            0x00000012UL                              /**< Mode LOC18 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC19            0x00000013UL                              /**< Mode LOC19 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC20            0x00000014UL                              /**< Mode LOC20 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC21            0x00000015UL                              /**< Mode LOC21 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC22            0x00000016UL                              /**< Mode LOC22 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC23            0x00000017UL                              /**< Mode LOC23 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC24            0x00000018UL                              /**< Mode LOC24 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC25            0x00000019UL                              /**< Mode LOC25 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC26            0x0000001AUL                              /**< Mode LOC26 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC27            0x0000001BUL                              /**< Mode LOC27 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC28            0x0000001CUL                              /**< Mode LOC28 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC29            0x0000001DUL                              /**< Mode LOC29 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC30            0x0000001EUL                              /**< Mode LOC30 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI0LOC_LOC31            0x0000001FUL                              /**< Mode LOC31 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC0              (_TIMER_ROUTELOC2_CDTI0LOC_LOC0 << 0)     /**< Shifted mode LOC0 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_DEFAULT           (_TIMER_ROUTELOC2_CDTI0LOC_DEFAULT << 0)  /**< Shifted mode DEFAULT for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC1              (_TIMER_ROUTELOC2_CDTI0LOC_LOC1 << 0)     /**< Shifted mode LOC1 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC2              (_TIMER_ROUTELOC2_CDTI0LOC_LOC2 << 0)     /**< Shifted mode LOC2 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC3              (_TIMER_ROUTELOC2_CDTI0LOC_LOC3 << 0)     /**< Shifted mode LOC3 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC4              (_TIMER_ROUTELOC2_CDTI0LOC_LOC4 << 0)     /**< Shifted mode LOC4 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC5              (_TIMER_ROUTELOC2_CDTI0LOC_LOC5 << 0)     /**< Shifted mode LOC5 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC6              (_TIMER_ROUTELOC2_CDTI0LOC_LOC6 << 0)     /**< Shifted mode LOC6 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC7              (_TIMER_ROUTELOC2_CDTI0LOC_LOC7 << 0)     /**< Shifted mode LOC7 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC8              (_TIMER_ROUTELOC2_CDTI0LOC_LOC8 << 0)     /**< Shifted mode LOC8 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC9              (_TIMER_ROUTELOC2_CDTI0LOC_LOC9 << 0)     /**< Shifted mode LOC9 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC10             (_TIMER_ROUTELOC2_CDTI0LOC_LOC10 << 0)    /**< Shifted mode LOC10 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC11             (_TIMER_ROUTELOC2_CDTI0LOC_LOC11 << 0)    /**< Shifted mode LOC11 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC12             (_TIMER_ROUTELOC2_CDTI0LOC_LOC12 << 0)    /**< Shifted mode LOC12 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC13             (_TIMER_ROUTELOC2_CDTI0LOC_LOC13 << 0)    /**< Shifted mode LOC13 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC14             (_TIMER_ROUTELOC2_CDTI0LOC_LOC14 << 0)    /**< Shifted mode LOC14 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC15             (_TIMER_ROUTELOC2_CDTI0LOC_LOC15 << 0)    /**< Shifted mode LOC15 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC16             (_TIMER_ROUTELOC2_CDTI0LOC_LOC16 << 0)    /**< Shifted mode LOC16 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC17             (_TIMER_ROUTELOC2_CDTI0LOC_LOC17 << 0)    /**< Shifted mode LOC17 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC18             (_TIMER_ROUTELOC2_CDTI0LOC_LOC18 << 0)    /**< Shifted mode LOC18 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC19             (_TIMER_ROUTELOC2_CDTI0LOC_LOC19 << 0)    /**< Shifted mode LOC19 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC20             (_TIMER_ROUTELOC2_CDTI0LOC_LOC20 << 0)    /**< Shifted mode LOC20 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC21             (_TIMER_ROUTELOC2_CDTI0LOC_LOC21 << 0)    /**< Shifted mode LOC21 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC22             (_TIMER_ROUTELOC2_CDTI0LOC_LOC22 << 0)    /**< Shifted mode LOC22 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC23             (_TIMER_ROUTELOC2_CDTI0LOC_LOC23 << 0)    /**< Shifted mode LOC23 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC24             (_TIMER_ROUTELOC2_CDTI0LOC_LOC24 << 0)    /**< Shifted mode LOC24 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC25             (_TIMER_ROUTELOC2_CDTI0LOC_LOC25 << 0)    /**< Shifted mode LOC25 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC26             (_TIMER_ROUTELOC2_CDTI0LOC_LOC26 << 0)    /**< Shifted mode LOC26 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC27             (_TIMER_ROUTELOC2_CDTI0LOC_LOC27 << 0)    /**< Shifted mode LOC27 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC28             (_TIMER_ROUTELOC2_CDTI0LOC_LOC28 << 0)    /**< Shifted mode LOC28 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC29             (_TIMER_ROUTELOC2_CDTI0LOC_LOC29 << 0)    /**< Shifted mode LOC29 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC30             (_TIMER_ROUTELOC2_CDTI0LOC_LOC30 << 0)    /**< Shifted mode LOC30 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI0LOC_LOC31             (_TIMER_ROUTELOC2_CDTI0LOC_LOC31 << 0)    /**< Shifted mode LOC31 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_SHIFT            8                                         /**< Shift value for TIMER_CDTI1LOC */
#define _TIMER_ROUTELOC2_CDTI1LOC_MASK             0x1F00UL                                  /**< Bit mask for TIMER_CDTI1LOC */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC0             0x00000000UL                              /**< Mode LOC0 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC1             0x00000001UL                              /**< Mode LOC1 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC2             0x00000002UL                              /**< Mode LOC2 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC3             0x00000003UL                              /**< Mode LOC3 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC4             0x00000004UL                              /**< Mode LOC4 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC5             0x00000005UL                              /**< Mode LOC5 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC6             0x00000006UL                              /**< Mode LOC6 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC7             0x00000007UL                              /**< Mode LOC7 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC8             0x00000008UL                              /**< Mode LOC8 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC9             0x00000009UL                              /**< Mode LOC9 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC10            0x0000000AUL                              /**< Mode LOC10 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC11            0x0000000BUL                              /**< Mode LOC11 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC12            0x0000000CUL                              /**< Mode LOC12 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC13            0x0000000DUL                              /**< Mode LOC13 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC14            0x0000000EUL                              /**< Mode LOC14 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC15            0x0000000FUL                              /**< Mode LOC15 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC16            0x00000010UL                              /**< Mode LOC16 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC17            0x00000011UL                              /**< Mode LOC17 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC18            0x00000012UL                              /**< Mode LOC18 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC19            0x00000013UL                              /**< Mode LOC19 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC20            0x00000014UL                              /**< Mode LOC20 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC21            0x00000015UL                              /**< Mode LOC21 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC22            0x00000016UL                              /**< Mode LOC22 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC23            0x00000017UL                              /**< Mode LOC23 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC24            0x00000018UL                              /**< Mode LOC24 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC25            0x00000019UL                              /**< Mode LOC25 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC26            0x0000001AUL                              /**< Mode LOC26 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC27            0x0000001BUL                              /**< Mode LOC27 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC28            0x0000001CUL                              /**< Mode LOC28 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC29            0x0000001DUL                              /**< Mode LOC29 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC30            0x0000001EUL                              /**< Mode LOC30 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI1LOC_LOC31            0x0000001FUL                              /**< Mode LOC31 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC0              (_TIMER_ROUTELOC2_CDTI1LOC_LOC0 << 8)     /**< Shifted mode LOC0 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_DEFAULT           (_TIMER_ROUTELOC2_CDTI1LOC_DEFAULT << 8)  /**< Shifted mode DEFAULT for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC1              (_TIMER_ROUTELOC2_CDTI1LOC_LOC1 << 8)     /**< Shifted mode LOC1 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC2              (_TIMER_ROUTELOC2_CDTI1LOC_LOC2 << 8)     /**< Shifted mode LOC2 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC3              (_TIMER_ROUTELOC2_CDTI1LOC_LOC3 << 8)     /**< Shifted mode LOC3 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC4              (_TIMER_ROUTELOC2_CDTI1LOC_LOC4 << 8)     /**< Shifted mode LOC4 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC5              (_TIMER_ROUTELOC2_CDTI1LOC_LOC5 << 8)     /**< Shifted mode LOC5 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC6              (_TIMER_ROUTELOC2_CDTI1LOC_LOC6 << 8)     /**< Shifted mode LOC6 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC7              (_TIMER_ROUTELOC2_CDTI1LOC_LOC7 << 8)     /**< Shifted mode LOC7 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC8              (_TIMER_ROUTELOC2_CDTI1LOC_LOC8 << 8)     /**< Shifted mode LOC8 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC9              (_TIMER_ROUTELOC2_CDTI1LOC_LOC9 << 8)     /**< Shifted mode LOC9 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC10             (_TIMER_ROUTELOC2_CDTI1LOC_LOC10 << 8)    /**< Shifted mode LOC10 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC11             (_TIMER_ROUTELOC2_CDTI1LOC_LOC11 << 8)    /**< Shifted mode LOC11 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC12             (_TIMER_ROUTELOC2_CDTI1LOC_LOC12 << 8)    /**< Shifted mode LOC12 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC13             (_TIMER_ROUTELOC2_CDTI1LOC_LOC13 << 8)    /**< Shifted mode LOC13 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC14             (_TIMER_ROUTELOC2_CDTI1LOC_LOC14 << 8)    /**< Shifted mode LOC14 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC15             (_TIMER_ROUTELOC2_CDTI1LOC_LOC15 << 8)    /**< Shifted mode LOC15 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC16             (_TIMER_ROUTELOC2_CDTI1LOC_LOC16 << 8)    /**< Shifted mode LOC16 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC17             (_TIMER_ROUTELOC2_CDTI1LOC_LOC17 << 8)    /**< Shifted mode LOC17 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC18             (_TIMER_ROUTELOC2_CDTI1LOC_LOC18 << 8)    /**< Shifted mode LOC18 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC19             (_TIMER_ROUTELOC2_CDTI1LOC_LOC19 << 8)    /**< Shifted mode LOC19 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC20             (_TIMER_ROUTELOC2_CDTI1LOC_LOC20 << 8)    /**< Shifted mode LOC20 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC21             (_TIMER_ROUTELOC2_CDTI1LOC_LOC21 << 8)    /**< Shifted mode LOC21 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC22             (_TIMER_ROUTELOC2_CDTI1LOC_LOC22 << 8)    /**< Shifted mode LOC22 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC23             (_TIMER_ROUTELOC2_CDTI1LOC_LOC23 << 8)    /**< Shifted mode LOC23 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC24             (_TIMER_ROUTELOC2_CDTI1LOC_LOC24 << 8)    /**< Shifted mode LOC24 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC25             (_TIMER_ROUTELOC2_CDTI1LOC_LOC25 << 8)    /**< Shifted mode LOC25 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC26             (_TIMER_ROUTELOC2_CDTI1LOC_LOC26 << 8)    /**< Shifted mode LOC26 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC27             (_TIMER_ROUTELOC2_CDTI1LOC_LOC27 << 8)    /**< Shifted mode LOC27 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC28             (_TIMER_ROUTELOC2_CDTI1LOC_LOC28 << 8)    /**< Shifted mode LOC28 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC29             (_TIMER_ROUTELOC2_CDTI1LOC_LOC29 << 8)    /**< Shifted mode LOC29 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC30             (_TIMER_ROUTELOC2_CDTI1LOC_LOC30 << 8)    /**< Shifted mode LOC30 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI1LOC_LOC31             (_TIMER_ROUTELOC2_CDTI1LOC_LOC31 << 8)    /**< Shifted mode LOC31 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_SHIFT            16                                        /**< Shift value for TIMER_CDTI2LOC */
#define _TIMER_ROUTELOC2_CDTI2LOC_MASK             0x1F0000UL                                /**< Bit mask for TIMER_CDTI2LOC */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC0             0x00000000UL                              /**< Mode LOC0 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_DEFAULT          0x00000000UL                              /**< Mode DEFAULT for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC1             0x00000001UL                              /**< Mode LOC1 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC2             0x00000002UL                              /**< Mode LOC2 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC3             0x00000003UL                              /**< Mode LOC3 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC4             0x00000004UL                              /**< Mode LOC4 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC5             0x00000005UL                              /**< Mode LOC5 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC6             0x00000006UL                              /**< Mode LOC6 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC7             0x00000007UL                              /**< Mode LOC7 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC8             0x00000008UL                              /**< Mode LOC8 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC9             0x00000009UL                              /**< Mode LOC9 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC10            0x0000000AUL                              /**< Mode LOC10 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC11            0x0000000BUL                              /**< Mode LOC11 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC12            0x0000000CUL                              /**< Mode LOC12 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC13            0x0000000DUL                              /**< Mode LOC13 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC14            0x0000000EUL                              /**< Mode LOC14 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC15            0x0000000FUL                              /**< Mode LOC15 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC16            0x00000010UL                              /**< Mode LOC16 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC17            0x00000011UL                              /**< Mode LOC17 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC18            0x00000012UL                              /**< Mode LOC18 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC19            0x00000013UL                              /**< Mode LOC19 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC20            0x00000014UL                              /**< Mode LOC20 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC21            0x00000015UL                              /**< Mode LOC21 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC22            0x00000016UL                              /**< Mode LOC22 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC23            0x00000017UL                              /**< Mode LOC23 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC24            0x00000018UL                              /**< Mode LOC24 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC25            0x00000019UL                              /**< Mode LOC25 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC26            0x0000001AUL                              /**< Mode LOC26 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC27            0x0000001BUL                              /**< Mode LOC27 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC28            0x0000001CUL                              /**< Mode LOC28 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC29            0x0000001DUL                              /**< Mode LOC29 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC30            0x0000001EUL                              /**< Mode LOC30 for TIMER_ROUTELOC2 */
#define _TIMER_ROUTELOC2_CDTI2LOC_LOC31            0x0000001FUL                              /**< Mode LOC31 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC0              (_TIMER_ROUTELOC2_CDTI2LOC_LOC0 << 16)    /**< Shifted mode LOC0 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_DEFAULT           (_TIMER_ROUTELOC2_CDTI2LOC_DEFAULT << 16) /**< Shifted mode DEFAULT for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC1              (_TIMER_ROUTELOC2_CDTI2LOC_LOC1 << 16)    /**< Shifted mode LOC1 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC2              (_TIMER_ROUTELOC2_CDTI2LOC_LOC2 << 16)    /**< Shifted mode LOC2 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC3              (_TIMER_ROUTELOC2_CDTI2LOC_LOC3 << 16)    /**< Shifted mode LOC3 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC4              (_TIMER_ROUTELOC2_CDTI2LOC_LOC4 << 16)    /**< Shifted mode LOC4 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC5              (_TIMER_ROUTELOC2_CDTI2LOC_LOC5 << 16)    /**< Shifted mode LOC5 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC6              (_TIMER_ROUTELOC2_CDTI2LOC_LOC6 << 16)    /**< Shifted mode LOC6 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC7              (_TIMER_ROUTELOC2_CDTI2LOC_LOC7 << 16)    /**< Shifted mode LOC7 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC8              (_TIMER_ROUTELOC2_CDTI2LOC_LOC8 << 16)    /**< Shifted mode LOC8 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC9              (_TIMER_ROUTELOC2_CDTI2LOC_LOC9 << 16)    /**< Shifted mode LOC9 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC10             (_TIMER_ROUTELOC2_CDTI2LOC_LOC10 << 16)   /**< Shifted mode LOC10 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC11             (_TIMER_ROUTELOC2_CDTI2LOC_LOC11 << 16)   /**< Shifted mode LOC11 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC12             (_TIMER_ROUTELOC2_CDTI2LOC_LOC12 << 16)   /**< Shifted mode LOC12 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC13             (_TIMER_ROUTELOC2_CDTI2LOC_LOC13 << 16)   /**< Shifted mode LOC13 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC14             (_TIMER_ROUTELOC2_CDTI2LOC_LOC14 << 16)   /**< Shifted mode LOC14 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC15             (_TIMER_ROUTELOC2_CDTI2LOC_LOC15 << 16)   /**< Shifted mode LOC15 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC16             (_TIMER_ROUTELOC2_CDTI2LOC_LOC16 << 16)   /**< Shifted mode LOC16 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC17             (_TIMER_ROUTELOC2_CDTI2LOC_LOC17 << 16)   /**< Shifted mode LOC17 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC18             (_TIMER_ROUTELOC2_CDTI2LOC_LOC18 << 16)   /**< Shifted mode LOC18 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC19             (_TIMER_ROUTELOC2_CDTI2LOC_LOC19 << 16)   /**< Shifted mode LOC19 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC20             (_TIMER_ROUTELOC2_CDTI2LOC_LOC20 << 16)   /**< Shifted mode LOC20 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC21             (_TIMER_ROUTELOC2_CDTI2LOC_LOC21 << 16)   /**< Shifted mode LOC21 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC22             (_TIMER_ROUTELOC2_CDTI2LOC_LOC22 << 16)   /**< Shifted mode LOC22 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC23             (_TIMER_ROUTELOC2_CDTI2LOC_LOC23 << 16)   /**< Shifted mode LOC23 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC24             (_TIMER_ROUTELOC2_CDTI2LOC_LOC24 << 16)   /**< Shifted mode LOC24 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC25             (_TIMER_ROUTELOC2_CDTI2LOC_LOC25 << 16)   /**< Shifted mode LOC25 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC26             (_TIMER_ROUTELOC2_CDTI2LOC_LOC26 << 16)   /**< Shifted mode LOC26 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC27             (_TIMER_ROUTELOC2_CDTI2LOC_LOC27 << 16)   /**< Shifted mode LOC27 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC28             (_TIMER_ROUTELOC2_CDTI2LOC_LOC28 << 16)   /**< Shifted mode LOC28 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC29             (_TIMER_ROUTELOC2_CDTI2LOC_LOC29 << 16)   /**< Shifted mode LOC29 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC30             (_TIMER_ROUTELOC2_CDTI2LOC_LOC30 << 16)   /**< Shifted mode LOC30 for TIMER_ROUTELOC2 */
#define TIMER_ROUTELOC2_CDTI2LOC_LOC31             (_TIMER_ROUTELOC2_CDTI2LOC_LOC31 << 16)   /**< Shifted mode LOC31 for TIMER_ROUTELOC2 */

/* Bit fields for TIMER CC_CTRL */
#define _TIMER_CC_CTRL_RESETVALUE                  0x00000000UL                                    /**< Default value for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_MASK                        0x7F0F3F17UL                                    /**< Mask for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_MODE_SHIFT                  0                                               /**< Shift value for TIMER_MODE */
#define _TIMER_CC_CTRL_MODE_MASK                   0x3UL                                           /**< Bit mask for TIMER_MODE */
#define _TIMER_CC_CTRL_MODE_DEFAULT                0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_MODE_OFF                    0x00000000UL                                    /**< Mode OFF for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_MODE_INPUTCAPTURE           0x00000001UL                                    /**< Mode INPUTCAPTURE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_MODE_OUTPUTCOMPARE          0x00000002UL                                    /**< Mode OUTPUTCOMPARE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_MODE_PWM                    0x00000003UL                                    /**< Mode PWM for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_MODE_DEFAULT                 (_TIMER_CC_CTRL_MODE_DEFAULT << 0)              /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_MODE_OFF                     (_TIMER_CC_CTRL_MODE_OFF << 0)                  /**< Shifted mode OFF for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_MODE_INPUTCAPTURE            (_TIMER_CC_CTRL_MODE_INPUTCAPTURE << 0)         /**< Shifted mode INPUTCAPTURE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_MODE_OUTPUTCOMPARE           (_TIMER_CC_CTRL_MODE_OUTPUTCOMPARE << 0)        /**< Shifted mode OUTPUTCOMPARE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_MODE_PWM                     (_TIMER_CC_CTRL_MODE_PWM << 0)                  /**< Shifted mode PWM for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_OUTINV                       (0x1UL << 2)                                    /**< Output Invert */
#define _TIMER_CC_CTRL_OUTINV_SHIFT                2                                               /**< Shift value for TIMER_OUTINV */
#define _TIMER_CC_CTRL_OUTINV_MASK                 0x4UL                                           /**< Bit mask for TIMER_OUTINV */
#define _TIMER_CC_CTRL_OUTINV_DEFAULT              0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_OUTINV_DEFAULT               (_TIMER_CC_CTRL_OUTINV_DEFAULT << 2)            /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_COIST                        (0x1UL << 4)                                    /**< Compare Output Initial State */
#define _TIMER_CC_CTRL_COIST_SHIFT                 4                                               /**< Shift value for TIMER_COIST */
#define _TIMER_CC_CTRL_COIST_MASK                  0x10UL                                          /**< Bit mask for TIMER_COIST */
#define _TIMER_CC_CTRL_COIST_DEFAULT               0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_COIST_DEFAULT                (_TIMER_CC_CTRL_COIST_DEFAULT << 4)             /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CMOA_SHIFT                  8                                               /**< Shift value for TIMER_CMOA */
#define _TIMER_CC_CTRL_CMOA_MASK                   0x300UL                                         /**< Bit mask for TIMER_CMOA */
#define _TIMER_CC_CTRL_CMOA_DEFAULT                0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CMOA_NONE                   0x00000000UL                                    /**< Mode NONE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CMOA_TOGGLE                 0x00000001UL                                    /**< Mode TOGGLE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CMOA_CLEAR                  0x00000002UL                                    /**< Mode CLEAR for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CMOA_SET                    0x00000003UL                                    /**< Mode SET for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CMOA_DEFAULT                 (_TIMER_CC_CTRL_CMOA_DEFAULT << 8)              /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CMOA_NONE                    (_TIMER_CC_CTRL_CMOA_NONE << 8)                 /**< Shifted mode NONE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CMOA_TOGGLE                  (_TIMER_CC_CTRL_CMOA_TOGGLE << 8)               /**< Shifted mode TOGGLE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CMOA_CLEAR                   (_TIMER_CC_CTRL_CMOA_CLEAR << 8)                /**< Shifted mode CLEAR for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CMOA_SET                     (_TIMER_CC_CTRL_CMOA_SET << 8)                  /**< Shifted mode SET for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_COFOA_SHIFT                 10                                              /**< Shift value for TIMER_COFOA */
#define _TIMER_CC_CTRL_COFOA_MASK                  0xC00UL                                         /**< Bit mask for TIMER_COFOA */
#define _TIMER_CC_CTRL_COFOA_DEFAULT               0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_COFOA_NONE                  0x00000000UL                                    /**< Mode NONE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_COFOA_TOGGLE                0x00000001UL                                    /**< Mode TOGGLE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_COFOA_CLEAR                 0x00000002UL                                    /**< Mode CLEAR for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_COFOA_SET                   0x00000003UL                                    /**< Mode SET for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_COFOA_DEFAULT                (_TIMER_CC_CTRL_COFOA_DEFAULT << 10)            /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_COFOA_NONE                   (_TIMER_CC_CTRL_COFOA_NONE << 10)               /**< Shifted mode NONE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_COFOA_TOGGLE                 (_TIMER_CC_CTRL_COFOA_TOGGLE << 10)             /**< Shifted mode TOGGLE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_COFOA_CLEAR                  (_TIMER_CC_CTRL_COFOA_CLEAR << 10)              /**< Shifted mode CLEAR for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_COFOA_SET                    (_TIMER_CC_CTRL_COFOA_SET << 10)                /**< Shifted mode SET for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CUFOA_SHIFT                 12                                              /**< Shift value for TIMER_CUFOA */
#define _TIMER_CC_CTRL_CUFOA_MASK                  0x3000UL                                        /**< Bit mask for TIMER_CUFOA */
#define _TIMER_CC_CTRL_CUFOA_DEFAULT               0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CUFOA_NONE                  0x00000000UL                                    /**< Mode NONE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CUFOA_TOGGLE                0x00000001UL                                    /**< Mode TOGGLE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CUFOA_CLEAR                 0x00000002UL                                    /**< Mode CLEAR for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_CUFOA_SET                   0x00000003UL                                    /**< Mode SET for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CUFOA_DEFAULT                (_TIMER_CC_CTRL_CUFOA_DEFAULT << 12)            /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CUFOA_NONE                   (_TIMER_CC_CTRL_CUFOA_NONE << 12)               /**< Shifted mode NONE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CUFOA_TOGGLE                 (_TIMER_CC_CTRL_CUFOA_TOGGLE << 12)             /**< Shifted mode TOGGLE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CUFOA_CLEAR                  (_TIMER_CC_CTRL_CUFOA_CLEAR << 12)              /**< Shifted mode CLEAR for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_CUFOA_SET                    (_TIMER_CC_CTRL_CUFOA_SET << 12)                /**< Shifted mode SET for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_SHIFT                16                                              /**< Shift value for TIMER_PRSSEL */
#define _TIMER_CC_CTRL_PRSSEL_MASK                 0xF0000UL                                       /**< Bit mask for TIMER_PRSSEL */
#define _TIMER_CC_CTRL_PRSSEL_DEFAULT              0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH0               0x00000000UL                                    /**< Mode PRSCH0 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH1               0x00000001UL                                    /**< Mode PRSCH1 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH2               0x00000002UL                                    /**< Mode PRSCH2 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH3               0x00000003UL                                    /**< Mode PRSCH3 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH4               0x00000004UL                                    /**< Mode PRSCH4 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH5               0x00000005UL                                    /**< Mode PRSCH5 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH6               0x00000006UL                                    /**< Mode PRSCH6 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH7               0x00000007UL                                    /**< Mode PRSCH7 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH8               0x00000008UL                                    /**< Mode PRSCH8 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH9               0x00000009UL                                    /**< Mode PRSCH9 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH10              0x0000000AUL                                    /**< Mode PRSCH10 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSSEL_PRSCH11              0x0000000BUL                                    /**< Mode PRSCH11 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_DEFAULT               (_TIMER_CC_CTRL_PRSSEL_DEFAULT << 16)           /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH0                (_TIMER_CC_CTRL_PRSSEL_PRSCH0 << 16)            /**< Shifted mode PRSCH0 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH1                (_TIMER_CC_CTRL_PRSSEL_PRSCH1 << 16)            /**< Shifted mode PRSCH1 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH2                (_TIMER_CC_CTRL_PRSSEL_PRSCH2 << 16)            /**< Shifted mode PRSCH2 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH3                (_TIMER_CC_CTRL_PRSSEL_PRSCH3 << 16)            /**< Shifted mode PRSCH3 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH4                (_TIMER_CC_CTRL_PRSSEL_PRSCH4 << 16)            /**< Shifted mode PRSCH4 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH5                (_TIMER_CC_CTRL_PRSSEL_PRSCH5 << 16)            /**< Shifted mode PRSCH5 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH6                (_TIMER_CC_CTRL_PRSSEL_PRSCH6 << 16)            /**< Shifted mode PRSCH6 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH7                (_TIMER_CC_CTRL_PRSSEL_PRSCH7 << 16)            /**< Shifted mode PRSCH7 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH8                (_TIMER_CC_CTRL_PRSSEL_PRSCH8 << 16)            /**< Shifted mode PRSCH8 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH9                (_TIMER_CC_CTRL_PRSSEL_PRSCH9 << 16)            /**< Shifted mode PRSCH9 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH10               (_TIMER_CC_CTRL_PRSSEL_PRSCH10 << 16)           /**< Shifted mode PRSCH10 for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSSEL_PRSCH11               (_TIMER_CC_CTRL_PRSSEL_PRSCH11 << 16)           /**< Shifted mode PRSCH11 for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEDGE_SHIFT                24                                              /**< Shift value for TIMER_ICEDGE */
#define _TIMER_CC_CTRL_ICEDGE_MASK                 0x3000000UL                                     /**< Bit mask for TIMER_ICEDGE */
#define _TIMER_CC_CTRL_ICEDGE_DEFAULT              0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEDGE_RISING               0x00000000UL                                    /**< Mode RISING for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEDGE_FALLING              0x00000001UL                                    /**< Mode FALLING for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEDGE_BOTH                 0x00000002UL                                    /**< Mode BOTH for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEDGE_NONE                 0x00000003UL                                    /**< Mode NONE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEDGE_DEFAULT               (_TIMER_CC_CTRL_ICEDGE_DEFAULT << 24)           /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEDGE_RISING                (_TIMER_CC_CTRL_ICEDGE_RISING << 24)            /**< Shifted mode RISING for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEDGE_FALLING               (_TIMER_CC_CTRL_ICEDGE_FALLING << 24)           /**< Shifted mode FALLING for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEDGE_BOTH                  (_TIMER_CC_CTRL_ICEDGE_BOTH << 24)              /**< Shifted mode BOTH for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEDGE_NONE                  (_TIMER_CC_CTRL_ICEDGE_NONE << 24)              /**< Shifted mode NONE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEVCTRL_SHIFT              26                                              /**< Shift value for TIMER_ICEVCTRL */
#define _TIMER_CC_CTRL_ICEVCTRL_MASK               0xC000000UL                                     /**< Bit mask for TIMER_ICEVCTRL */
#define _TIMER_CC_CTRL_ICEVCTRL_DEFAULT            0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEVCTRL_EVERYEDGE          0x00000000UL                                    /**< Mode EVERYEDGE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEVCTRL_EVERYSECONDEDGE    0x00000001UL                                    /**< Mode EVERYSECONDEDGE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEVCTRL_RISING             0x00000002UL                                    /**< Mode RISING for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_ICEVCTRL_FALLING            0x00000003UL                                    /**< Mode FALLING for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEVCTRL_DEFAULT             (_TIMER_CC_CTRL_ICEVCTRL_DEFAULT << 26)         /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEVCTRL_EVERYEDGE           (_TIMER_CC_CTRL_ICEVCTRL_EVERYEDGE << 26)       /**< Shifted mode EVERYEDGE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEVCTRL_EVERYSECONDEDGE     (_TIMER_CC_CTRL_ICEVCTRL_EVERYSECONDEDGE << 26) /**< Shifted mode EVERYSECONDEDGE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEVCTRL_RISING              (_TIMER_CC_CTRL_ICEVCTRL_RISING << 26)          /**< Shifted mode RISING for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_ICEVCTRL_FALLING             (_TIMER_CC_CTRL_ICEVCTRL_FALLING << 26)         /**< Shifted mode FALLING for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSCONF                      (0x1UL << 28)                                   /**< PRS Configuration */
#define _TIMER_CC_CTRL_PRSCONF_SHIFT               28                                              /**< Shift value for TIMER_PRSCONF */
#define _TIMER_CC_CTRL_PRSCONF_MASK                0x10000000UL                                    /**< Bit mask for TIMER_PRSCONF */
#define _TIMER_CC_CTRL_PRSCONF_DEFAULT             0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSCONF_PULSE               0x00000000UL                                    /**< Mode PULSE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_PRSCONF_LEVEL               0x00000001UL                                    /**< Mode LEVEL for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSCONF_DEFAULT              (_TIMER_CC_CTRL_PRSCONF_DEFAULT << 28)          /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSCONF_PULSE                (_TIMER_CC_CTRL_PRSCONF_PULSE << 28)            /**< Shifted mode PULSE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_PRSCONF_LEVEL                (_TIMER_CC_CTRL_PRSCONF_LEVEL << 28)            /**< Shifted mode LEVEL for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_INSEL                        (0x1UL << 29)                                   /**< Input Selection */
#define _TIMER_CC_CTRL_INSEL_SHIFT                 29                                              /**< Shift value for TIMER_INSEL */
#define _TIMER_CC_CTRL_INSEL_MASK                  0x20000000UL                                    /**< Bit mask for TIMER_INSEL */
#define _TIMER_CC_CTRL_INSEL_DEFAULT               0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_INSEL_PIN                   0x00000000UL                                    /**< Mode PIN for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_INSEL_PRS                   0x00000001UL                                    /**< Mode PRS for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_INSEL_DEFAULT                (_TIMER_CC_CTRL_INSEL_DEFAULT << 29)            /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_INSEL_PIN                    (_TIMER_CC_CTRL_INSEL_PIN << 29)                /**< Shifted mode PIN for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_INSEL_PRS                    (_TIMER_CC_CTRL_INSEL_PRS << 29)                /**< Shifted mode PRS for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_FILT                         (0x1UL << 30)                                   /**< Digital Filter */
#define _TIMER_CC_CTRL_FILT_SHIFT                  30                                              /**< Shift value for TIMER_FILT */
#define _TIMER_CC_CTRL_FILT_MASK                   0x40000000UL                                    /**< Bit mask for TIMER_FILT */
#define _TIMER_CC_CTRL_FILT_DEFAULT                0x00000000UL                                    /**< Mode DEFAULT for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_FILT_DISABLE                0x00000000UL                                    /**< Mode DISABLE for TIMER_CC_CTRL */
#define _TIMER_CC_CTRL_FILT_ENABLE                 0x00000001UL                                    /**< Mode ENABLE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_FILT_DEFAULT                 (_TIMER_CC_CTRL_FILT_DEFAULT << 30)             /**< Shifted mode DEFAULT for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_FILT_DISABLE                 (_TIMER_CC_CTRL_FILT_DISABLE << 30)             /**< Shifted mode DISABLE for TIMER_CC_CTRL */
#define TIMER_CC_CTRL_FILT_ENABLE                  (_TIMER_CC_CTRL_FILT_ENABLE << 30)              /**< Shifted mode ENABLE for TIMER_CC_CTRL */

/* Bit fields for TIMER CC_CCV */
#define _TIMER_CC_CCV_RESETVALUE                   0x00000000UL                     /**< Default value for TIMER_CC_CCV */
#define _TIMER_CC_CCV_MASK                         0x0000FFFFUL                     /**< Mask for TIMER_CC_CCV */
#define _TIMER_CC_CCV_CCV_SHIFT                    0                                /**< Shift value for TIMER_CCV */
#define _TIMER_CC_CCV_CCV_MASK                     0xFFFFUL                         /**< Bit mask for TIMER_CCV */
#define _TIMER_CC_CCV_CCV_DEFAULT                  0x00000000UL                     /**< Mode DEFAULT for TIMER_CC_CCV */
#define TIMER_CC_CCV_CCV_DEFAULT                   (_TIMER_CC_CCV_CCV_DEFAULT << 0) /**< Shifted mode DEFAULT for TIMER_CC_CCV */

/* Bit fields for TIMER CC_CCVP */
#define _TIMER_CC_CCVP_RESETVALUE                  0x00000000UL                       /**< Default value for TIMER_CC_CCVP */
#define _TIMER_CC_CCVP_MASK                        0x0000FFFFUL                       /**< Mask for TIMER_CC_CCVP */
#define _TIMER_CC_CCVP_CCVP_SHIFT                  0                                  /**< Shift value for TIMER_CCVP */
#define _TIMER_CC_CCVP_CCVP_MASK                   0xFFFFUL                           /**< Bit mask for TIMER_CCVP */
#define _TIMER_CC_CCVP_CCVP_DEFAULT                0x00000000UL                       /**< Mode DEFAULT for TIMER_CC_CCVP */
#define TIMER_CC_CCVP_CCVP_DEFAULT                 (_TIMER_CC_CCVP_CCVP_DEFAULT << 0) /**< Shifted mode DEFAULT for TIMER_CC_CCVP */

/* Bit fields for TIMER CC_CCVB */
#define _TIMER_CC_CCVB_RESETVALUE                  0x00000000UL                       /**< Default value for TIMER_CC_CCVB */
#define _TIMER_CC_CCVB_MASK                        0x0000FFFFUL                       /**< Mask for TIMER_CC_CCVB */
#define _TIMER_CC_CCVB_CCVB_SHIFT                  0                                  /**< Shift value for TIMER_CCVB */
#define _TIMER_CC_CCVB_CCVB_MASK                   0xFFFFUL                           /**< Bit mask for TIMER_CCVB */
#define _TIMER_CC_CCVB_CCVB_DEFAULT                0x00000000UL                       /**< Mode DEFAULT for TIMER_CC_CCVB */
#define TIMER_CC_CCVB_CCVB_DEFAULT                 (_TIMER_CC_CCVB_CCVB_DEFAULT << 0) /**< Shifted mode DEFAULT for TIMER_CC_CCVB */

/* Bit fields for TIMER DTCTRL */
#define _TIMER_DTCTRL_RESETVALUE                   0x00000000UL                          /**< Default value for TIMER_DTCTRL */
#define _TIMER_DTCTRL_MASK                         0x010006FFUL                          /**< Mask for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTEN                          (0x1UL << 0)                          /**< DTI Enable */
#define _TIMER_DTCTRL_DTEN_SHIFT                   0                                     /**< Shift value for TIMER_DTEN */
#define _TIMER_DTCTRL_DTEN_MASK                    0x1UL                                 /**< Bit mask for TIMER_DTEN */
#define _TIMER_DTCTRL_DTEN_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTEN_DEFAULT                  (_TIMER_DTCTRL_DTEN_DEFAULT << 0)     /**< Shifted mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTDAS                         (0x1UL << 1)                          /**< DTI Automatic Start-up Functionality */
#define _TIMER_DTCTRL_DTDAS_SHIFT                  1                                     /**< Shift value for TIMER_DTDAS */
#define _TIMER_DTCTRL_DTDAS_MASK                   0x2UL                                 /**< Bit mask for TIMER_DTDAS */
#define _TIMER_DTCTRL_DTDAS_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTDAS_NORESTART              0x00000000UL                          /**< Mode NORESTART for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTDAS_RESTART                0x00000001UL                          /**< Mode RESTART for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTDAS_DEFAULT                 (_TIMER_DTCTRL_DTDAS_DEFAULT << 1)    /**< Shifted mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTDAS_NORESTART               (_TIMER_DTCTRL_DTDAS_NORESTART << 1)  /**< Shifted mode NORESTART for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTDAS_RESTART                 (_TIMER_DTCTRL_DTDAS_RESTART << 1)    /**< Shifted mode RESTART for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTIPOL                        (0x1UL << 2)                          /**< DTI Inactive Polarity */
#define _TIMER_DTCTRL_DTIPOL_SHIFT                 2                                     /**< Shift value for TIMER_DTIPOL */
#define _TIMER_DTCTRL_DTIPOL_MASK                  0x4UL                                 /**< Bit mask for TIMER_DTIPOL */
#define _TIMER_DTCTRL_DTIPOL_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTIPOL_DEFAULT                (_TIMER_DTCTRL_DTIPOL_DEFAULT << 2)   /**< Shifted mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTCINV                        (0x1UL << 3)                          /**< DTI Complementary Output Invert. */
#define _TIMER_DTCTRL_DTCINV_SHIFT                 3                                     /**< Shift value for TIMER_DTCINV */
#define _TIMER_DTCTRL_DTCINV_MASK                  0x8UL                                 /**< Bit mask for TIMER_DTCINV */
#define _TIMER_DTCTRL_DTCINV_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTCINV_DEFAULT                (_TIMER_DTCTRL_DTCINV_DEFAULT << 3)   /**< Shifted mode DEFAULT for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_SHIFT               4                                     /**< Shift value for TIMER_DTPRSSEL */
#define _TIMER_DTCTRL_DTPRSSEL_MASK                0xF0UL                                /**< Bit mask for TIMER_DTPRSSEL */
#define _TIMER_DTCTRL_DTPRSSEL_DEFAULT             0x00000000UL                          /**< Mode DEFAULT for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH0              0x00000000UL                          /**< Mode PRSCH0 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH1              0x00000001UL                          /**< Mode PRSCH1 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH2              0x00000002UL                          /**< Mode PRSCH2 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH3              0x00000003UL                          /**< Mode PRSCH3 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH4              0x00000004UL                          /**< Mode PRSCH4 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH5              0x00000005UL                          /**< Mode PRSCH5 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH6              0x00000006UL                          /**< Mode PRSCH6 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH7              0x00000007UL                          /**< Mode PRSCH7 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH8              0x00000008UL                          /**< Mode PRSCH8 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH9              0x00000009UL                          /**< Mode PRSCH9 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH10             0x0000000AUL                          /**< Mode PRSCH10 for TIMER_DTCTRL */
#define _TIMER_DTCTRL_DTPRSSEL_PRSCH11             0x0000000BUL                          /**< Mode PRSCH11 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_DEFAULT              (_TIMER_DTCTRL_DTPRSSEL_DEFAULT << 4) /**< Shifted mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH0               (_TIMER_DTCTRL_DTPRSSEL_PRSCH0 << 4)  /**< Shifted mode PRSCH0 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH1               (_TIMER_DTCTRL_DTPRSSEL_PRSCH1 << 4)  /**< Shifted mode PRSCH1 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH2               (_TIMER_DTCTRL_DTPRSSEL_PRSCH2 << 4)  /**< Shifted mode PRSCH2 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH3               (_TIMER_DTCTRL_DTPRSSEL_PRSCH3 << 4)  /**< Shifted mode PRSCH3 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH4               (_TIMER_DTCTRL_DTPRSSEL_PRSCH4 << 4)  /**< Shifted mode PRSCH4 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH5               (_TIMER_DTCTRL_DTPRSSEL_PRSCH5 << 4)  /**< Shifted mode PRSCH5 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH6               (_TIMER_DTCTRL_DTPRSSEL_PRSCH6 << 4)  /**< Shifted mode PRSCH6 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH7               (_TIMER_DTCTRL_DTPRSSEL_PRSCH7 << 4)  /**< Shifted mode PRSCH7 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH8               (_TIMER_DTCTRL_DTPRSSEL_PRSCH8 << 4)  /**< Shifted mode PRSCH8 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH9               (_TIMER_DTCTRL_DTPRSSEL_PRSCH9 << 4)  /**< Shifted mode PRSCH9 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH10              (_TIMER_DTCTRL_DTPRSSEL_PRSCH10 << 4) /**< Shifted mode PRSCH10 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSSEL_PRSCH11              (_TIMER_DTCTRL_DTPRSSEL_PRSCH11 << 4) /**< Shifted mode PRSCH11 for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTAR                          (0x1UL << 9)                          /**< DTI Always Run */
#define _TIMER_DTCTRL_DTAR_SHIFT                   9                                     /**< Shift value for TIMER_DTAR */
#define _TIMER_DTCTRL_DTAR_MASK                    0x200UL                               /**< Bit mask for TIMER_DTAR */
#define _TIMER_DTCTRL_DTAR_DEFAULT                 0x00000000UL                          /**< Mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTAR_DEFAULT                  (_TIMER_DTCTRL_DTAR_DEFAULT << 9)     /**< Shifted mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTFATS                        (0x1UL << 10)                         /**< DTI Fault Action on Timer Stop */
#define _TIMER_DTCTRL_DTFATS_SHIFT                 10                                    /**< Shift value for TIMER_DTFATS */
#define _TIMER_DTCTRL_DTFATS_MASK                  0x400UL                               /**< Bit mask for TIMER_DTFATS */
#define _TIMER_DTCTRL_DTFATS_DEFAULT               0x00000000UL                          /**< Mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTFATS_DEFAULT                (_TIMER_DTCTRL_DTFATS_DEFAULT << 10)  /**< Shifted mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSEN                       (0x1UL << 24)                         /**< DTI PRS Source Enable */
#define _TIMER_DTCTRL_DTPRSEN_SHIFT                24                                    /**< Shift value for TIMER_DTPRSEN */
#define _TIMER_DTCTRL_DTPRSEN_MASK                 0x1000000UL                           /**< Bit mask for TIMER_DTPRSEN */
#define _TIMER_DTCTRL_DTPRSEN_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for TIMER_DTCTRL */
#define TIMER_DTCTRL_DTPRSEN_DEFAULT               (_TIMER_DTCTRL_DTPRSEN_DEFAULT << 24) /**< Shifted mode DEFAULT for TIMER_DTCTRL */

/* Bit fields for TIMER DTTIME */
#define _TIMER_DTTIME_RESETVALUE                   0x00000000UL                          /**< Default value for TIMER_DTTIME */
#define _TIMER_DTTIME_MASK                         0x003F3F0FUL                          /**< Mask for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_SHIFT                0                                     /**< Shift value for TIMER_DTPRESC */
#define _TIMER_DTTIME_DTPRESC_MASK                 0xFUL                                 /**< Bit mask for TIMER_DTPRESC */
#define _TIMER_DTTIME_DTPRESC_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV1                 0x00000000UL                          /**< Mode DIV1 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV2                 0x00000001UL                          /**< Mode DIV2 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV4                 0x00000002UL                          /**< Mode DIV4 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV8                 0x00000003UL                          /**< Mode DIV8 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV16                0x00000004UL                          /**< Mode DIV16 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV32                0x00000005UL                          /**< Mode DIV32 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV64                0x00000006UL                          /**< Mode DIV64 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV128               0x00000007UL                          /**< Mode DIV128 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV256               0x00000008UL                          /**< Mode DIV256 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV512               0x00000009UL                          /**< Mode DIV512 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTPRESC_DIV1024              0x0000000AUL                          /**< Mode DIV1024 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DEFAULT               (_TIMER_DTTIME_DTPRESC_DEFAULT << 0)  /**< Shifted mode DEFAULT for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV1                  (_TIMER_DTTIME_DTPRESC_DIV1 << 0)     /**< Shifted mode DIV1 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV2                  (_TIMER_DTTIME_DTPRESC_DIV2 << 0)     /**< Shifted mode DIV2 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV4                  (_TIMER_DTTIME_DTPRESC_DIV4 << 0)     /**< Shifted mode DIV4 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV8                  (_TIMER_DTTIME_DTPRESC_DIV8 << 0)     /**< Shifted mode DIV8 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV16                 (_TIMER_DTTIME_DTPRESC_DIV16 << 0)    /**< Shifted mode DIV16 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV32                 (_TIMER_DTTIME_DTPRESC_DIV32 << 0)    /**< Shifted mode DIV32 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV64                 (_TIMER_DTTIME_DTPRESC_DIV64 << 0)    /**< Shifted mode DIV64 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV128                (_TIMER_DTTIME_DTPRESC_DIV128 << 0)   /**< Shifted mode DIV128 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV256                (_TIMER_DTTIME_DTPRESC_DIV256 << 0)   /**< Shifted mode DIV256 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV512                (_TIMER_DTTIME_DTPRESC_DIV512 << 0)   /**< Shifted mode DIV512 for TIMER_DTTIME */
#define TIMER_DTTIME_DTPRESC_DIV1024               (_TIMER_DTTIME_DTPRESC_DIV1024 << 0)  /**< Shifted mode DIV1024 for TIMER_DTTIME */
#define _TIMER_DTTIME_DTRISET_SHIFT                8                                     /**< Shift value for TIMER_DTRISET */
#define _TIMER_DTTIME_DTRISET_MASK                 0x3F00UL                              /**< Bit mask for TIMER_DTRISET */
#define _TIMER_DTTIME_DTRISET_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for TIMER_DTTIME */
#define TIMER_DTTIME_DTRISET_DEFAULT               (_TIMER_DTTIME_DTRISET_DEFAULT << 8)  /**< Shifted mode DEFAULT for TIMER_DTTIME */
#define _TIMER_DTTIME_DTFALLT_SHIFT                16                                    /**< Shift value for TIMER_DTFALLT */
#define _TIMER_DTTIME_DTFALLT_MASK                 0x3F0000UL                            /**< Bit mask for TIMER_DTFALLT */
#define _TIMER_DTTIME_DTFALLT_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for TIMER_DTTIME */
#define TIMER_DTTIME_DTFALLT_DEFAULT               (_TIMER_DTTIME_DTFALLT_DEFAULT << 16) /**< Shifted mode DEFAULT for TIMER_DTTIME */

/* Bit fields for TIMER DTFC */
#define _TIMER_DTFC_RESETVALUE                     0x00000000UL                            /**< Default value for TIMER_DTFC */
#define _TIMER_DTFC_MASK                           0x0F030F0FUL                            /**< Mask for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_SHIFT               0                                       /**< Shift value for TIMER_DTPRS0FSEL */
#define _TIMER_DTFC_DTPRS0FSEL_MASK                0xFUL                                   /**< Bit mask for TIMER_DTPRS0FSEL */
#define _TIMER_DTFC_DTPRS0FSEL_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH0              0x00000000UL                            /**< Mode PRSCH0 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH1              0x00000001UL                            /**< Mode PRSCH1 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH2              0x00000002UL                            /**< Mode PRSCH2 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH3              0x00000003UL                            /**< Mode PRSCH3 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH4              0x00000004UL                            /**< Mode PRSCH4 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH5              0x00000005UL                            /**< Mode PRSCH5 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH6              0x00000006UL                            /**< Mode PRSCH6 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH7              0x00000007UL                            /**< Mode PRSCH7 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH8              0x00000008UL                            /**< Mode PRSCH8 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH9              0x00000009UL                            /**< Mode PRSCH9 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH10             0x0000000AUL                            /**< Mode PRSCH10 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS0FSEL_PRSCH11             0x0000000BUL                            /**< Mode PRSCH11 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_DEFAULT              (_TIMER_DTFC_DTPRS0FSEL_DEFAULT << 0)   /**< Shifted mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH0               (_TIMER_DTFC_DTPRS0FSEL_PRSCH0 << 0)    /**< Shifted mode PRSCH0 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH1               (_TIMER_DTFC_DTPRS0FSEL_PRSCH1 << 0)    /**< Shifted mode PRSCH1 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH2               (_TIMER_DTFC_DTPRS0FSEL_PRSCH2 << 0)    /**< Shifted mode PRSCH2 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH3               (_TIMER_DTFC_DTPRS0FSEL_PRSCH3 << 0)    /**< Shifted mode PRSCH3 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH4               (_TIMER_DTFC_DTPRS0FSEL_PRSCH4 << 0)    /**< Shifted mode PRSCH4 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH5               (_TIMER_DTFC_DTPRS0FSEL_PRSCH5 << 0)    /**< Shifted mode PRSCH5 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH6               (_TIMER_DTFC_DTPRS0FSEL_PRSCH6 << 0)    /**< Shifted mode PRSCH6 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH7               (_TIMER_DTFC_DTPRS0FSEL_PRSCH7 << 0)    /**< Shifted mode PRSCH7 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH8               (_TIMER_DTFC_DTPRS0FSEL_PRSCH8 << 0)    /**< Shifted mode PRSCH8 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH9               (_TIMER_DTFC_DTPRS0FSEL_PRSCH9 << 0)    /**< Shifted mode PRSCH9 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH10              (_TIMER_DTFC_DTPRS0FSEL_PRSCH10 << 0)   /**< Shifted mode PRSCH10 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FSEL_PRSCH11              (_TIMER_DTFC_DTPRS0FSEL_PRSCH11 << 0)   /**< Shifted mode PRSCH11 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_SHIFT               8                                       /**< Shift value for TIMER_DTPRS1FSEL */
#define _TIMER_DTFC_DTPRS1FSEL_MASK                0xF00UL                                 /**< Bit mask for TIMER_DTPRS1FSEL */
#define _TIMER_DTFC_DTPRS1FSEL_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH0              0x00000000UL                            /**< Mode PRSCH0 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH1              0x00000001UL                            /**< Mode PRSCH1 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH2              0x00000002UL                            /**< Mode PRSCH2 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH3              0x00000003UL                            /**< Mode PRSCH3 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH4              0x00000004UL                            /**< Mode PRSCH4 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH5              0x00000005UL                            /**< Mode PRSCH5 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH6              0x00000006UL                            /**< Mode PRSCH6 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH7              0x00000007UL                            /**< Mode PRSCH7 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH8              0x00000008UL                            /**< Mode PRSCH8 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH9              0x00000009UL                            /**< Mode PRSCH9 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH10             0x0000000AUL                            /**< Mode PRSCH10 for TIMER_DTFC */
#define _TIMER_DTFC_DTPRS1FSEL_PRSCH11             0x0000000BUL                            /**< Mode PRSCH11 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_DEFAULT              (_TIMER_DTFC_DTPRS1FSEL_DEFAULT << 8)   /**< Shifted mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH0               (_TIMER_DTFC_DTPRS1FSEL_PRSCH0 << 8)    /**< Shifted mode PRSCH0 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH1               (_TIMER_DTFC_DTPRS1FSEL_PRSCH1 << 8)    /**< Shifted mode PRSCH1 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH2               (_TIMER_DTFC_DTPRS1FSEL_PRSCH2 << 8)    /**< Shifted mode PRSCH2 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH3               (_TIMER_DTFC_DTPRS1FSEL_PRSCH3 << 8)    /**< Shifted mode PRSCH3 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH4               (_TIMER_DTFC_DTPRS1FSEL_PRSCH4 << 8)    /**< Shifted mode PRSCH4 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH5               (_TIMER_DTFC_DTPRS1FSEL_PRSCH5 << 8)    /**< Shifted mode PRSCH5 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH6               (_TIMER_DTFC_DTPRS1FSEL_PRSCH6 << 8)    /**< Shifted mode PRSCH6 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH7               (_TIMER_DTFC_DTPRS1FSEL_PRSCH7 << 8)    /**< Shifted mode PRSCH7 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH8               (_TIMER_DTFC_DTPRS1FSEL_PRSCH8 << 8)    /**< Shifted mode PRSCH8 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH9               (_TIMER_DTFC_DTPRS1FSEL_PRSCH9 << 8)    /**< Shifted mode PRSCH9 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH10              (_TIMER_DTFC_DTPRS1FSEL_PRSCH10 << 8)   /**< Shifted mode PRSCH10 for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FSEL_PRSCH11              (_TIMER_DTFC_DTPRS1FSEL_PRSCH11 << 8)   /**< Shifted mode PRSCH11 for TIMER_DTFC */
#define _TIMER_DTFC_DTFA_SHIFT                     16                                      /**< Shift value for TIMER_DTFA */
#define _TIMER_DTFC_DTFA_MASK                      0x30000UL                               /**< Bit mask for TIMER_DTFA */
#define _TIMER_DTFC_DTFA_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFC */
#define _TIMER_DTFC_DTFA_NONE                      0x00000000UL                            /**< Mode NONE for TIMER_DTFC */
#define _TIMER_DTFC_DTFA_INACTIVE                  0x00000001UL                            /**< Mode INACTIVE for TIMER_DTFC */
#define _TIMER_DTFC_DTFA_CLEAR                     0x00000002UL                            /**< Mode CLEAR for TIMER_DTFC */
#define _TIMER_DTFC_DTFA_TRISTATE                  0x00000003UL                            /**< Mode TRISTATE for TIMER_DTFC */
#define TIMER_DTFC_DTFA_DEFAULT                    (_TIMER_DTFC_DTFA_DEFAULT << 16)        /**< Shifted mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTFA_NONE                       (_TIMER_DTFC_DTFA_NONE << 16)           /**< Shifted mode NONE for TIMER_DTFC */
#define TIMER_DTFC_DTFA_INACTIVE                   (_TIMER_DTFC_DTFA_INACTIVE << 16)       /**< Shifted mode INACTIVE for TIMER_DTFC */
#define TIMER_DTFC_DTFA_CLEAR                      (_TIMER_DTFC_DTFA_CLEAR << 16)          /**< Shifted mode CLEAR for TIMER_DTFC */
#define TIMER_DTFC_DTFA_TRISTATE                   (_TIMER_DTFC_DTFA_TRISTATE << 16)       /**< Shifted mode TRISTATE for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FEN                       (0x1UL << 24)                           /**< DTI PRS 0 Fault Enable */
#define _TIMER_DTFC_DTPRS0FEN_SHIFT                24                                      /**< Shift value for TIMER_DTPRS0FEN */
#define _TIMER_DTFC_DTPRS0FEN_MASK                 0x1000000UL                             /**< Bit mask for TIMER_DTPRS0FEN */
#define _TIMER_DTFC_DTPRS0FEN_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTPRS0FEN_DEFAULT               (_TIMER_DTFC_DTPRS0FEN_DEFAULT << 24)   /**< Shifted mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FEN                       (0x1UL << 25)                           /**< DTI PRS 1 Fault Enable */
#define _TIMER_DTFC_DTPRS1FEN_SHIFT                25                                      /**< Shift value for TIMER_DTPRS1FEN */
#define _TIMER_DTFC_DTPRS1FEN_MASK                 0x2000000UL                             /**< Bit mask for TIMER_DTPRS1FEN */
#define _TIMER_DTFC_DTPRS1FEN_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTPRS1FEN_DEFAULT               (_TIMER_DTFC_DTPRS1FEN_DEFAULT << 25)   /**< Shifted mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTDBGFEN                        (0x1UL << 26)                           /**< DTI Debugger Fault Enable */
#define _TIMER_DTFC_DTDBGFEN_SHIFT                 26                                      /**< Shift value for TIMER_DTDBGFEN */
#define _TIMER_DTFC_DTDBGFEN_MASK                  0x4000000UL                             /**< Bit mask for TIMER_DTDBGFEN */
#define _TIMER_DTFC_DTDBGFEN_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTDBGFEN_DEFAULT                (_TIMER_DTFC_DTDBGFEN_DEFAULT << 26)    /**< Shifted mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTLOCKUPFEN                     (0x1UL << 27)                           /**< DTI Lockup Fault Enable */
#define _TIMER_DTFC_DTLOCKUPFEN_SHIFT              27                                      /**< Shift value for TIMER_DTLOCKUPFEN */
#define _TIMER_DTFC_DTLOCKUPFEN_MASK               0x8000000UL                             /**< Bit mask for TIMER_DTLOCKUPFEN */
#define _TIMER_DTFC_DTLOCKUPFEN_DEFAULT            0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFC */
#define TIMER_DTFC_DTLOCKUPFEN_DEFAULT             (_TIMER_DTFC_DTLOCKUPFEN_DEFAULT << 27) /**< Shifted mode DEFAULT for TIMER_DTFC */

/* Bit fields for TIMER DTOGEN */
#define _TIMER_DTOGEN_RESETVALUE                   0x00000000UL                             /**< Default value for TIMER_DTOGEN */
#define _TIMER_DTOGEN_MASK                         0x0000003FUL                             /**< Mask for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCC0EN                     (0x1UL << 0)                             /**< DTI CC0 Output Generation Enable */
#define _TIMER_DTOGEN_DTOGCC0EN_SHIFT              0                                        /**< Shift value for TIMER_DTOGCC0EN */
#define _TIMER_DTOGEN_DTOGCC0EN_MASK               0x1UL                                    /**< Bit mask for TIMER_DTOGCC0EN */
#define _TIMER_DTOGEN_DTOGCC0EN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCC0EN_DEFAULT             (_TIMER_DTOGEN_DTOGCC0EN_DEFAULT << 0)   /**< Shifted mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCC1EN                     (0x1UL << 1)                             /**< DTI CC1 Output Generation Enable */
#define _TIMER_DTOGEN_DTOGCC1EN_SHIFT              1                                        /**< Shift value for TIMER_DTOGCC1EN */
#define _TIMER_DTOGEN_DTOGCC1EN_MASK               0x2UL                                    /**< Bit mask for TIMER_DTOGCC1EN */
#define _TIMER_DTOGEN_DTOGCC1EN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCC1EN_DEFAULT             (_TIMER_DTOGEN_DTOGCC1EN_DEFAULT << 1)   /**< Shifted mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCC2EN                     (0x1UL << 2)                             /**< DTI CC2 Output Generation Enable */
#define _TIMER_DTOGEN_DTOGCC2EN_SHIFT              2                                        /**< Shift value for TIMER_DTOGCC2EN */
#define _TIMER_DTOGEN_DTOGCC2EN_MASK               0x4UL                                    /**< Bit mask for TIMER_DTOGCC2EN */
#define _TIMER_DTOGEN_DTOGCC2EN_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCC2EN_DEFAULT             (_TIMER_DTOGEN_DTOGCC2EN_DEFAULT << 2)   /**< Shifted mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCDTI0EN                   (0x1UL << 3)                             /**< DTI CDTI0 Output Generation Enable */
#define _TIMER_DTOGEN_DTOGCDTI0EN_SHIFT            3                                        /**< Shift value for TIMER_DTOGCDTI0EN */
#define _TIMER_DTOGEN_DTOGCDTI0EN_MASK             0x8UL                                    /**< Bit mask for TIMER_DTOGCDTI0EN */
#define _TIMER_DTOGEN_DTOGCDTI0EN_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCDTI0EN_DEFAULT           (_TIMER_DTOGEN_DTOGCDTI0EN_DEFAULT << 3) /**< Shifted mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCDTI1EN                   (0x1UL << 4)                             /**< DTI CDTI1 Output Generation Enable */
#define _TIMER_DTOGEN_DTOGCDTI1EN_SHIFT            4                                        /**< Shift value for TIMER_DTOGCDTI1EN */
#define _TIMER_DTOGEN_DTOGCDTI1EN_MASK             0x10UL                                   /**< Bit mask for TIMER_DTOGCDTI1EN */
#define _TIMER_DTOGEN_DTOGCDTI1EN_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCDTI1EN_DEFAULT           (_TIMER_DTOGEN_DTOGCDTI1EN_DEFAULT << 4) /**< Shifted mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCDTI2EN                   (0x1UL << 5)                             /**< DTI CDTI2 Output Generation Enable */
#define _TIMER_DTOGEN_DTOGCDTI2EN_SHIFT            5                                        /**< Shift value for TIMER_DTOGCDTI2EN */
#define _TIMER_DTOGEN_DTOGCDTI2EN_MASK             0x20UL                                   /**< Bit mask for TIMER_DTOGCDTI2EN */
#define _TIMER_DTOGEN_DTOGCDTI2EN_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for TIMER_DTOGEN */
#define TIMER_DTOGEN_DTOGCDTI2EN_DEFAULT           (_TIMER_DTOGEN_DTOGCDTI2EN_DEFAULT << 5) /**< Shifted mode DEFAULT for TIMER_DTOGEN */

/* Bit fields for TIMER DTFAULT */
#define _TIMER_DTFAULT_RESETVALUE                  0x00000000UL                            /**< Default value for TIMER_DTFAULT */
#define _TIMER_DTFAULT_MASK                        0x0000000FUL                            /**< Mask for TIMER_DTFAULT */
#define TIMER_DTFAULT_DTPRS0F                      (0x1UL << 0)                            /**< DTI PRS 0 Fault */
#define _TIMER_DTFAULT_DTPRS0F_SHIFT               0                                       /**< Shift value for TIMER_DTPRS0F */
#define _TIMER_DTFAULT_DTPRS0F_MASK                0x1UL                                   /**< Bit mask for TIMER_DTPRS0F */
#define _TIMER_DTFAULT_DTPRS0F_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFAULT */
#define TIMER_DTFAULT_DTPRS0F_DEFAULT              (_TIMER_DTFAULT_DTPRS0F_DEFAULT << 0)   /**< Shifted mode DEFAULT for TIMER_DTFAULT */
#define TIMER_DTFAULT_DTPRS1F                      (0x1UL << 1)                            /**< DTI PRS 1 Fault */
#define _TIMER_DTFAULT_DTPRS1F_SHIFT               1                                       /**< Shift value for TIMER_DTPRS1F */
#define _TIMER_DTFAULT_DTPRS1F_MASK                0x2UL                                   /**< Bit mask for TIMER_DTPRS1F */
#define _TIMER_DTFAULT_DTPRS1F_DEFAULT             0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFAULT */
#define TIMER_DTFAULT_DTPRS1F_DEFAULT              (_TIMER_DTFAULT_DTPRS1F_DEFAULT << 1)   /**< Shifted mode DEFAULT for TIMER_DTFAULT */
#define TIMER_DTFAULT_DTDBGF                       (0x1UL << 2)                            /**< DTI Debugger Fault */
#define _TIMER_DTFAULT_DTDBGF_SHIFT                2                                       /**< Shift value for TIMER_DTDBGF */
#define _TIMER_DTFAULT_DTDBGF_MASK                 0x4UL                                   /**< Bit mask for TIMER_DTDBGF */
#define _TIMER_DTFAULT_DTDBGF_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFAULT */
#define TIMER_DTFAULT_DTDBGF_DEFAULT               (_TIMER_DTFAULT_DTDBGF_DEFAULT << 2)    /**< Shifted mode DEFAULT for TIMER_DTFAULT */
#define TIMER_DTFAULT_DTLOCKUPF                    (0x1UL << 3)                            /**< DTI Lockup Fault */
#define _TIMER_DTFAULT_DTLOCKUPF_SHIFT             3                                       /**< Shift value for TIMER_DTLOCKUPF */
#define _TIMER_DTFAULT_DTLOCKUPF_MASK              0x8UL                                   /**< Bit mask for TIMER_DTLOCKUPF */
#define _TIMER_DTFAULT_DTLOCKUPF_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for TIMER_DTFAULT */
#define TIMER_DTFAULT_DTLOCKUPF_DEFAULT            (_TIMER_DTFAULT_DTLOCKUPF_DEFAULT << 3) /**< Shifted mode DEFAULT for TIMER_DTFAULT */

/* Bit fields for TIMER DTFAULTC */
#define _TIMER_DTFAULTC_RESETVALUE                 0x00000000UL                             /**< Default value for TIMER_DTFAULTC */
#define _TIMER_DTFAULTC_MASK                       0x0000000FUL                             /**< Mask for TIMER_DTFAULTC */
#define TIMER_DTFAULTC_DTPRS0FC                    (0x1UL << 0)                             /**< DTI PRS0 Fault Clear */
#define _TIMER_DTFAULTC_DTPRS0FC_SHIFT             0                                        /**< Shift value for TIMER_DTPRS0FC */
#define _TIMER_DTFAULTC_DTPRS0FC_MASK              0x1UL                                    /**< Bit mask for TIMER_DTPRS0FC */
#define _TIMER_DTFAULTC_DTPRS0FC_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for TIMER_DTFAULTC */
#define TIMER_DTFAULTC_DTPRS0FC_DEFAULT            (_TIMER_DTFAULTC_DTPRS0FC_DEFAULT << 0)  /**< Shifted mode DEFAULT for TIMER_DTFAULTC */
#define TIMER_DTFAULTC_DTPRS1FC                    (0x1UL << 1)                             /**< DTI PRS1 Fault Clear */
#define _TIMER_DTFAULTC_DTPRS1FC_SHIFT             1                                        /**< Shift value for TIMER_DTPRS1FC */
#define _TIMER_DTFAULTC_DTPRS1FC_MASK              0x2UL                                    /**< Bit mask for TIMER_DTPRS1FC */
#define _TIMER_DTFAULTC_DTPRS1FC_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for TIMER_DTFAULTC */
#define TIMER_DTFAULTC_DTPRS1FC_DEFAULT            (_TIMER_DTFAULTC_DTPRS1FC_DEFAULT << 1)  /**< Shifted mode DEFAULT for TIMER_DTFAULTC */
#define TIMER_DTFAULTC_DTDBGFC                     (0x1UL << 2)                             /**< DTI Debugger Fault Clear */
#define _TIMER_DTFAULTC_DTDBGFC_SHIFT              2                                        /**< Shift value for TIMER_DTDBGFC */
#define _TIMER_DTFAULTC_DTDBGFC_MASK               0x4UL                                    /**< Bit mask for TIMER_DTDBGFC */
#define _TIMER_DTFAULTC_DTDBGFC_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for TIMER_DTFAULTC */
#define TIMER_DTFAULTC_DTDBGFC_DEFAULT             (_TIMER_DTFAULTC_DTDBGFC_DEFAULT << 2)   /**< Shifted mode DEFAULT for TIMER_DTFAULTC */
#define TIMER_DTFAULTC_TLOCKUPFC                   (0x1UL << 3)                             /**< DTI Lockup Fault Clear */
#define _TIMER_DTFAULTC_TLOCKUPFC_SHIFT            3                                        /**< Shift value for TIMER_TLOCKUPFC */
#define _TIMER_DTFAULTC_TLOCKUPFC_MASK             0x8UL                                    /**< Bit mask for TIMER_TLOCKUPFC */
#define _TIMER_DTFAULTC_TLOCKUPFC_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for TIMER_DTFAULTC */
#define TIMER_DTFAULTC_TLOCKUPFC_DEFAULT           (_TIMER_DTFAULTC_TLOCKUPFC_DEFAULT << 3) /**< Shifted mode DEFAULT for TIMER_DTFAULTC */

/* Bit fields for TIMER DTLOCK */
#define _TIMER_DTLOCK_RESETVALUE                   0x00000000UL                          /**< Default value for TIMER_DTLOCK */
#define _TIMER_DTLOCK_MASK                         0x0000FFFFUL                          /**< Mask for TIMER_DTLOCK */
#define _TIMER_DTLOCK_LOCKKEY_SHIFT                0                                     /**< Shift value for TIMER_LOCKKEY */
#define _TIMER_DTLOCK_LOCKKEY_MASK                 0xFFFFUL                              /**< Bit mask for TIMER_LOCKKEY */
#define _TIMER_DTLOCK_LOCKKEY_DEFAULT              0x00000000UL                          /**< Mode DEFAULT for TIMER_DTLOCK */
#define _TIMER_DTLOCK_LOCKKEY_LOCK                 0x00000000UL                          /**< Mode LOCK for TIMER_DTLOCK */
#define _TIMER_DTLOCK_LOCKKEY_UNLOCKED             0x00000000UL                          /**< Mode UNLOCKED for TIMER_DTLOCK */
#define _TIMER_DTLOCK_LOCKKEY_LOCKED               0x00000001UL                          /**< Mode LOCKED for TIMER_DTLOCK */
#define _TIMER_DTLOCK_LOCKKEY_UNLOCK               0x0000CE80UL                          /**< Mode UNLOCK for TIMER_DTLOCK */
#define TIMER_DTLOCK_LOCKKEY_DEFAULT               (_TIMER_DTLOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for TIMER_DTLOCK */
#define TIMER_DTLOCK_LOCKKEY_LOCK                  (_TIMER_DTLOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for TIMER_DTLOCK */
#define TIMER_DTLOCK_LOCKKEY_UNLOCKED              (_TIMER_DTLOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for TIMER_DTLOCK */
#define TIMER_DTLOCK_LOCKKEY_LOCKED                (_TIMER_DTLOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for TIMER_DTLOCK */
#define TIMER_DTLOCK_LOCKKEY_UNLOCK                (_TIMER_DTLOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for TIMER_DTLOCK */

/** @} End of group EFR32FG1P_TIMER */
/** @} End of group Parts */

