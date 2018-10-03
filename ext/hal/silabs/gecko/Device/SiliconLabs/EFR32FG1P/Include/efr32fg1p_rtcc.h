/**************************************************************************//**
 * @file efr32fg1p_rtcc.h
 * @brief EFR32FG1P_RTCC register and bit field definitions
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
 * @defgroup EFR32FG1P_RTCC
 * @{
 * @brief EFR32FG1P_RTCC Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t   CTRL;          /**< Control Register  */
  __IOM uint32_t   PRECNT;        /**< Pre-Counter Value Register  */
  __IOM uint32_t   CNT;           /**< Counter Value Register  */
  __IM uint32_t    COMBCNT;       /**< Combined Pre-Counter and Counter Value Register  */
  __IOM uint32_t   TIME;          /**< Time of day register  */
  __IOM uint32_t   DATE;          /**< Date register  */
  __IM uint32_t    IF;            /**< RTCC Interrupt Flags  */
  __IOM uint32_t   IFS;           /**< Interrupt Flag Set Register  */
  __IOM uint32_t   IFC;           /**< Interrupt Flag Clear Register  */
  __IOM uint32_t   IEN;           /**< Interrupt Enable Register  */
  __IM uint32_t    STATUS;        /**< Status register  */
  __IOM uint32_t   CMD;           /**< Command Register  */
  __IM uint32_t    SYNCBUSY;      /**< Synchronization Busy Register  */
  __IOM uint32_t   POWERDOWN;     /**< Retention RAM power-down register  */
  __IOM uint32_t   LOCK;          /**< Configuration Lock Register  */
  __IOM uint32_t   EM4WUEN;       /**< Wake Up Enable  */

  RTCC_CC_TypeDef  CC[3];         /**< Capture/Compare Channel */

  uint32_t         RESERVED0[37]; /**< Reserved registers */
  RTCC_RET_TypeDef RET[32];       /**< RetentionReg */
} RTCC_TypeDef;                   /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_RTCC_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for RTCC CTRL */
#define _RTCC_CTRL_RESETVALUE               0x00000000UL                            /**< Default value for RTCC_CTRL */
#define _RTCC_CTRL_MASK                     0x00039F35UL                            /**< Mask for RTCC_CTRL */
#define RTCC_CTRL_ENABLE                    (0x1UL << 0)                            /**< RTCC Enable */
#define _RTCC_CTRL_ENABLE_SHIFT             0                                       /**< Shift value for RTCC_ENABLE */
#define _RTCC_CTRL_ENABLE_MASK              0x1UL                                   /**< Bit mask for RTCC_ENABLE */
#define _RTCC_CTRL_ENABLE_DEFAULT           0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_ENABLE_DEFAULT            (_RTCC_CTRL_ENABLE_DEFAULT << 0)        /**< Shifted mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_DEBUGRUN                  (0x1UL << 2)                            /**< Debug Mode Run Enable */
#define _RTCC_CTRL_DEBUGRUN_SHIFT           2                                       /**< Shift value for RTCC_DEBUGRUN */
#define _RTCC_CTRL_DEBUGRUN_MASK            0x4UL                                   /**< Bit mask for RTCC_DEBUGRUN */
#define _RTCC_CTRL_DEBUGRUN_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_DEBUGRUN_DEFAULT          (_RTCC_CTRL_DEBUGRUN_DEFAULT << 2)      /**< Shifted mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_PRECCV0TOP                (0x1UL << 4)                            /**< Pre-counter CCV0 top value enable. */
#define _RTCC_CTRL_PRECCV0TOP_SHIFT         4                                       /**< Shift value for RTCC_PRECCV0TOP */
#define _RTCC_CTRL_PRECCV0TOP_MASK          0x10UL                                  /**< Bit mask for RTCC_PRECCV0TOP */
#define _RTCC_CTRL_PRECCV0TOP_DEFAULT       0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_PRECCV0TOP_DEFAULT        (_RTCC_CTRL_PRECCV0TOP_DEFAULT << 4)    /**< Shifted mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_CCV1TOP                   (0x1UL << 5)                            /**< CCV1 top value enable */
#define _RTCC_CTRL_CCV1TOP_SHIFT            5                                       /**< Shift value for RTCC_CCV1TOP */
#define _RTCC_CTRL_CCV1TOP_MASK             0x20UL                                  /**< Bit mask for RTCC_CCV1TOP */
#define _RTCC_CTRL_CCV1TOP_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_CCV1TOP_DEFAULT           (_RTCC_CTRL_CCV1TOP_DEFAULT << 5)       /**< Shifted mode DEFAULT for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_SHIFT           8                                       /**< Shift value for RTCC_CNTPRESC */
#define _RTCC_CTRL_CNTPRESC_MASK            0xF00UL                                 /**< Bit mask for RTCC_CNTPRESC */
#define _RTCC_CTRL_CNTPRESC_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV1            0x00000000UL                            /**< Mode DIV1 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV2            0x00000001UL                            /**< Mode DIV2 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV4            0x00000002UL                            /**< Mode DIV4 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV8            0x00000003UL                            /**< Mode DIV8 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV16           0x00000004UL                            /**< Mode DIV16 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV32           0x00000005UL                            /**< Mode DIV32 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV64           0x00000006UL                            /**< Mode DIV64 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV128          0x00000007UL                            /**< Mode DIV128 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV256          0x00000008UL                            /**< Mode DIV256 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV512          0x00000009UL                            /**< Mode DIV512 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV1024         0x0000000AUL                            /**< Mode DIV1024 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV2048         0x0000000BUL                            /**< Mode DIV2048 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV4096         0x0000000CUL                            /**< Mode DIV4096 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV8192         0x0000000DUL                            /**< Mode DIV8192 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV16384        0x0000000EUL                            /**< Mode DIV16384 for RTCC_CTRL */
#define _RTCC_CTRL_CNTPRESC_DIV32768        0x0000000FUL                            /**< Mode DIV32768 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DEFAULT          (_RTCC_CTRL_CNTPRESC_DEFAULT << 8)      /**< Shifted mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV1             (_RTCC_CTRL_CNTPRESC_DIV1 << 8)         /**< Shifted mode DIV1 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV2             (_RTCC_CTRL_CNTPRESC_DIV2 << 8)         /**< Shifted mode DIV2 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV4             (_RTCC_CTRL_CNTPRESC_DIV4 << 8)         /**< Shifted mode DIV4 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV8             (_RTCC_CTRL_CNTPRESC_DIV8 << 8)         /**< Shifted mode DIV8 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV16            (_RTCC_CTRL_CNTPRESC_DIV16 << 8)        /**< Shifted mode DIV16 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV32            (_RTCC_CTRL_CNTPRESC_DIV32 << 8)        /**< Shifted mode DIV32 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV64            (_RTCC_CTRL_CNTPRESC_DIV64 << 8)        /**< Shifted mode DIV64 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV128           (_RTCC_CTRL_CNTPRESC_DIV128 << 8)       /**< Shifted mode DIV128 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV256           (_RTCC_CTRL_CNTPRESC_DIV256 << 8)       /**< Shifted mode DIV256 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV512           (_RTCC_CTRL_CNTPRESC_DIV512 << 8)       /**< Shifted mode DIV512 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV1024          (_RTCC_CTRL_CNTPRESC_DIV1024 << 8)      /**< Shifted mode DIV1024 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV2048          (_RTCC_CTRL_CNTPRESC_DIV2048 << 8)      /**< Shifted mode DIV2048 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV4096          (_RTCC_CTRL_CNTPRESC_DIV4096 << 8)      /**< Shifted mode DIV4096 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV8192          (_RTCC_CTRL_CNTPRESC_DIV8192 << 8)      /**< Shifted mode DIV8192 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV16384         (_RTCC_CTRL_CNTPRESC_DIV16384 << 8)     /**< Shifted mode DIV16384 for RTCC_CTRL */
#define RTCC_CTRL_CNTPRESC_DIV32768         (_RTCC_CTRL_CNTPRESC_DIV32768 << 8)     /**< Shifted mode DIV32768 for RTCC_CTRL */
#define RTCC_CTRL_CNTTICK                   (0x1UL << 12)                           /**< Counter prescaler mode. */
#define _RTCC_CTRL_CNTTICK_SHIFT            12                                      /**< Shift value for RTCC_CNTTICK */
#define _RTCC_CTRL_CNTTICK_MASK             0x1000UL                                /**< Bit mask for RTCC_CNTTICK */
#define _RTCC_CTRL_CNTTICK_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define _RTCC_CTRL_CNTTICK_PRESC            0x00000000UL                            /**< Mode PRESC for RTCC_CTRL */
#define _RTCC_CTRL_CNTTICK_CCV0MATCH        0x00000001UL                            /**< Mode CCV0MATCH for RTCC_CTRL */
#define RTCC_CTRL_CNTTICK_DEFAULT           (_RTCC_CTRL_CNTTICK_DEFAULT << 12)      /**< Shifted mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_CNTTICK_PRESC             (_RTCC_CTRL_CNTTICK_PRESC << 12)        /**< Shifted mode PRESC for RTCC_CTRL */
#define RTCC_CTRL_CNTTICK_CCV0MATCH         (_RTCC_CTRL_CNTTICK_CCV0MATCH << 12)    /**< Shifted mode CCV0MATCH for RTCC_CTRL */
#define RTCC_CTRL_OSCFDETEN                 (0x1UL << 15)                           /**< Oscillator failure detection enable */
#define _RTCC_CTRL_OSCFDETEN_SHIFT          15                                      /**< Shift value for RTCC_OSCFDETEN */
#define _RTCC_CTRL_OSCFDETEN_MASK           0x8000UL                                /**< Bit mask for RTCC_OSCFDETEN */
#define _RTCC_CTRL_OSCFDETEN_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_OSCFDETEN_DEFAULT         (_RTCC_CTRL_OSCFDETEN_DEFAULT << 15)    /**< Shifted mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_CNTMODE                   (0x1UL << 16)                           /**< Main counter mode */
#define _RTCC_CTRL_CNTMODE_SHIFT            16                                      /**< Shift value for RTCC_CNTMODE */
#define _RTCC_CTRL_CNTMODE_MASK             0x10000UL                               /**< Bit mask for RTCC_CNTMODE */
#define _RTCC_CTRL_CNTMODE_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define _RTCC_CTRL_CNTMODE_NORMAL           0x00000000UL                            /**< Mode NORMAL for RTCC_CTRL */
#define _RTCC_CTRL_CNTMODE_CALENDAR         0x00000001UL                            /**< Mode CALENDAR for RTCC_CTRL */
#define RTCC_CTRL_CNTMODE_DEFAULT           (_RTCC_CTRL_CNTMODE_DEFAULT << 16)      /**< Shifted mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_CNTMODE_NORMAL            (_RTCC_CTRL_CNTMODE_NORMAL << 16)       /**< Shifted mode NORMAL for RTCC_CTRL */
#define RTCC_CTRL_CNTMODE_CALENDAR          (_RTCC_CTRL_CNTMODE_CALENDAR << 16)     /**< Shifted mode CALENDAR for RTCC_CTRL */
#define RTCC_CTRL_LYEARCORRDIS              (0x1UL << 17)                           /**< Leap year correction disabled. */
#define _RTCC_CTRL_LYEARCORRDIS_SHIFT       17                                      /**< Shift value for RTCC_LYEARCORRDIS */
#define _RTCC_CTRL_LYEARCORRDIS_MASK        0x20000UL                               /**< Bit mask for RTCC_LYEARCORRDIS */
#define _RTCC_CTRL_LYEARCORRDIS_DEFAULT     0x00000000UL                            /**< Mode DEFAULT for RTCC_CTRL */
#define RTCC_CTRL_LYEARCORRDIS_DEFAULT      (_RTCC_CTRL_LYEARCORRDIS_DEFAULT << 17) /**< Shifted mode DEFAULT for RTCC_CTRL */

/* Bit fields for RTCC PRECNT */
#define _RTCC_PRECNT_RESETVALUE             0x00000000UL                       /**< Default value for RTCC_PRECNT */
#define _RTCC_PRECNT_MASK                   0x00007FFFUL                       /**< Mask for RTCC_PRECNT */
#define _RTCC_PRECNT_PRECNT_SHIFT           0                                  /**< Shift value for RTCC_PRECNT */
#define _RTCC_PRECNT_PRECNT_MASK            0x7FFFUL                           /**< Bit mask for RTCC_PRECNT */
#define _RTCC_PRECNT_PRECNT_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for RTCC_PRECNT */
#define RTCC_PRECNT_PRECNT_DEFAULT          (_RTCC_PRECNT_PRECNT_DEFAULT << 0) /**< Shifted mode DEFAULT for RTCC_PRECNT */

/* Bit fields for RTCC CNT */
#define _RTCC_CNT_RESETVALUE                0x00000000UL                 /**< Default value for RTCC_CNT */
#define _RTCC_CNT_MASK                      0xFFFFFFFFUL                 /**< Mask for RTCC_CNT */
#define _RTCC_CNT_CNT_SHIFT                 0                            /**< Shift value for RTCC_CNT */
#define _RTCC_CNT_CNT_MASK                  0xFFFFFFFFUL                 /**< Bit mask for RTCC_CNT */
#define _RTCC_CNT_CNT_DEFAULT               0x00000000UL                 /**< Mode DEFAULT for RTCC_CNT */
#define RTCC_CNT_CNT_DEFAULT                (_RTCC_CNT_CNT_DEFAULT << 0) /**< Shifted mode DEFAULT for RTCC_CNT */

/* Bit fields for RTCC COMBCNT */
#define _RTCC_COMBCNT_RESETVALUE            0x00000000UL                         /**< Default value for RTCC_COMBCNT */
#define _RTCC_COMBCNT_MASK                  0xFFFFFFFFUL                         /**< Mask for RTCC_COMBCNT */
#define _RTCC_COMBCNT_PRECNT_SHIFT          0                                    /**< Shift value for RTCC_PRECNT */
#define _RTCC_COMBCNT_PRECNT_MASK           0x7FFFUL                             /**< Bit mask for RTCC_PRECNT */
#define _RTCC_COMBCNT_PRECNT_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for RTCC_COMBCNT */
#define RTCC_COMBCNT_PRECNT_DEFAULT         (_RTCC_COMBCNT_PRECNT_DEFAULT << 0)  /**< Shifted mode DEFAULT for RTCC_COMBCNT */
#define _RTCC_COMBCNT_CNTLSB_SHIFT          15                                   /**< Shift value for RTCC_CNTLSB */
#define _RTCC_COMBCNT_CNTLSB_MASK           0xFFFF8000UL                         /**< Bit mask for RTCC_CNTLSB */
#define _RTCC_COMBCNT_CNTLSB_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for RTCC_COMBCNT */
#define RTCC_COMBCNT_CNTLSB_DEFAULT         (_RTCC_COMBCNT_CNTLSB_DEFAULT << 15) /**< Shifted mode DEFAULT for RTCC_COMBCNT */

/* Bit fields for RTCC TIME */
#define _RTCC_TIME_RESETVALUE               0x00000000UL                     /**< Default value for RTCC_TIME */
#define _RTCC_TIME_MASK                     0x003F7F7FUL                     /**< Mask for RTCC_TIME */
#define _RTCC_TIME_SECU_SHIFT               0                                /**< Shift value for RTCC_SECU */
#define _RTCC_TIME_SECU_MASK                0xFUL                            /**< Bit mask for RTCC_SECU */
#define _RTCC_TIME_SECU_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for RTCC_TIME */
#define RTCC_TIME_SECU_DEFAULT              (_RTCC_TIME_SECU_DEFAULT << 0)   /**< Shifted mode DEFAULT for RTCC_TIME */
#define _RTCC_TIME_SECT_SHIFT               4                                /**< Shift value for RTCC_SECT */
#define _RTCC_TIME_SECT_MASK                0x70UL                           /**< Bit mask for RTCC_SECT */
#define _RTCC_TIME_SECT_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for RTCC_TIME */
#define RTCC_TIME_SECT_DEFAULT              (_RTCC_TIME_SECT_DEFAULT << 4)   /**< Shifted mode DEFAULT for RTCC_TIME */
#define _RTCC_TIME_MINU_SHIFT               8                                /**< Shift value for RTCC_MINU */
#define _RTCC_TIME_MINU_MASK                0xF00UL                          /**< Bit mask for RTCC_MINU */
#define _RTCC_TIME_MINU_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for RTCC_TIME */
#define RTCC_TIME_MINU_DEFAULT              (_RTCC_TIME_MINU_DEFAULT << 8)   /**< Shifted mode DEFAULT for RTCC_TIME */
#define _RTCC_TIME_MINT_SHIFT               12                               /**< Shift value for RTCC_MINT */
#define _RTCC_TIME_MINT_MASK                0x7000UL                         /**< Bit mask for RTCC_MINT */
#define _RTCC_TIME_MINT_DEFAULT             0x00000000UL                     /**< Mode DEFAULT for RTCC_TIME */
#define RTCC_TIME_MINT_DEFAULT              (_RTCC_TIME_MINT_DEFAULT << 12)  /**< Shifted mode DEFAULT for RTCC_TIME */
#define _RTCC_TIME_HOURU_SHIFT              16                               /**< Shift value for RTCC_HOURU */
#define _RTCC_TIME_HOURU_MASK               0xF0000UL                        /**< Bit mask for RTCC_HOURU */
#define _RTCC_TIME_HOURU_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for RTCC_TIME */
#define RTCC_TIME_HOURU_DEFAULT             (_RTCC_TIME_HOURU_DEFAULT << 16) /**< Shifted mode DEFAULT for RTCC_TIME */
#define _RTCC_TIME_HOURT_SHIFT              20                               /**< Shift value for RTCC_HOURT */
#define _RTCC_TIME_HOURT_MASK               0x300000UL                       /**< Bit mask for RTCC_HOURT */
#define _RTCC_TIME_HOURT_DEFAULT            0x00000000UL                     /**< Mode DEFAULT for RTCC_TIME */
#define RTCC_TIME_HOURT_DEFAULT             (_RTCC_TIME_HOURT_DEFAULT << 20) /**< Shifted mode DEFAULT for RTCC_TIME */

/* Bit fields for RTCC DATE */
#define _RTCC_DATE_RESETVALUE               0x00000000UL                      /**< Default value for RTCC_DATE */
#define _RTCC_DATE_MASK                     0x07FF1F3FUL                      /**< Mask for RTCC_DATE */
#define _RTCC_DATE_DAYOMU_SHIFT             0                                 /**< Shift value for RTCC_DAYOMU */
#define _RTCC_DATE_DAYOMU_MASK              0xFUL                             /**< Bit mask for RTCC_DAYOMU */
#define _RTCC_DATE_DAYOMU_DEFAULT           0x00000000UL                      /**< Mode DEFAULT for RTCC_DATE */
#define RTCC_DATE_DAYOMU_DEFAULT            (_RTCC_DATE_DAYOMU_DEFAULT << 0)  /**< Shifted mode DEFAULT for RTCC_DATE */
#define _RTCC_DATE_DAYOMT_SHIFT             4                                 /**< Shift value for RTCC_DAYOMT */
#define _RTCC_DATE_DAYOMT_MASK              0x30UL                            /**< Bit mask for RTCC_DAYOMT */
#define _RTCC_DATE_DAYOMT_DEFAULT           0x00000000UL                      /**< Mode DEFAULT for RTCC_DATE */
#define RTCC_DATE_DAYOMT_DEFAULT            (_RTCC_DATE_DAYOMT_DEFAULT << 4)  /**< Shifted mode DEFAULT for RTCC_DATE */
#define _RTCC_DATE_MONTHU_SHIFT             8                                 /**< Shift value for RTCC_MONTHU */
#define _RTCC_DATE_MONTHU_MASK              0xF00UL                           /**< Bit mask for RTCC_MONTHU */
#define _RTCC_DATE_MONTHU_DEFAULT           0x00000000UL                      /**< Mode DEFAULT for RTCC_DATE */
#define RTCC_DATE_MONTHU_DEFAULT            (_RTCC_DATE_MONTHU_DEFAULT << 8)  /**< Shifted mode DEFAULT for RTCC_DATE */
#define RTCC_DATE_MONTHT                    (0x1UL << 12)                     /**< Month, tens. */
#define _RTCC_DATE_MONTHT_SHIFT             12                                /**< Shift value for RTCC_MONTHT */
#define _RTCC_DATE_MONTHT_MASK              0x1000UL                          /**< Bit mask for RTCC_MONTHT */
#define _RTCC_DATE_MONTHT_DEFAULT           0x00000000UL                      /**< Mode DEFAULT for RTCC_DATE */
#define RTCC_DATE_MONTHT_DEFAULT            (_RTCC_DATE_MONTHT_DEFAULT << 12) /**< Shifted mode DEFAULT for RTCC_DATE */
#define _RTCC_DATE_YEARU_SHIFT              16                                /**< Shift value for RTCC_YEARU */
#define _RTCC_DATE_YEARU_MASK               0xF0000UL                         /**< Bit mask for RTCC_YEARU */
#define _RTCC_DATE_YEARU_DEFAULT            0x00000000UL                      /**< Mode DEFAULT for RTCC_DATE */
#define RTCC_DATE_YEARU_DEFAULT             (_RTCC_DATE_YEARU_DEFAULT << 16)  /**< Shifted mode DEFAULT for RTCC_DATE */
#define _RTCC_DATE_YEART_SHIFT              20                                /**< Shift value for RTCC_YEART */
#define _RTCC_DATE_YEART_MASK               0xF00000UL                        /**< Bit mask for RTCC_YEART */
#define _RTCC_DATE_YEART_DEFAULT            0x00000000UL                      /**< Mode DEFAULT for RTCC_DATE */
#define RTCC_DATE_YEART_DEFAULT             (_RTCC_DATE_YEART_DEFAULT << 20)  /**< Shifted mode DEFAULT for RTCC_DATE */
#define _RTCC_DATE_DAYOW_SHIFT              24                                /**< Shift value for RTCC_DAYOW */
#define _RTCC_DATE_DAYOW_MASK               0x7000000UL                       /**< Bit mask for RTCC_DAYOW */
#define _RTCC_DATE_DAYOW_DEFAULT            0x00000000UL                      /**< Mode DEFAULT for RTCC_DATE */
#define RTCC_DATE_DAYOW_DEFAULT             (_RTCC_DATE_DAYOW_DEFAULT << 24)  /**< Shifted mode DEFAULT for RTCC_DATE */

/* Bit fields for RTCC IF */
#define _RTCC_IF_RESETVALUE                 0x00000000UL                       /**< Default value for RTCC_IF */
#define _RTCC_IF_MASK                       0x000007FFUL                       /**< Mask for RTCC_IF */
#define RTCC_IF_OF                          (0x1UL << 0)                       /**< Overflow Interrupt Flag */
#define _RTCC_IF_OF_SHIFT                   0                                  /**< Shift value for RTCC_OF */
#define _RTCC_IF_OF_MASK                    0x1UL                              /**< Bit mask for RTCC_OF */
#define _RTCC_IF_OF_DEFAULT                 0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_OF_DEFAULT                  (_RTCC_IF_OF_DEFAULT << 0)         /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_CC0                         (0x1UL << 1)                       /**< Channel 0 Interrupt Flag */
#define _RTCC_IF_CC0_SHIFT                  1                                  /**< Shift value for RTCC_CC0 */
#define _RTCC_IF_CC0_MASK                   0x2UL                              /**< Bit mask for RTCC_CC0 */
#define _RTCC_IF_CC0_DEFAULT                0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_CC0_DEFAULT                 (_RTCC_IF_CC0_DEFAULT << 1)        /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_CC1                         (0x1UL << 2)                       /**< Channel 1 Interrupt Flag */
#define _RTCC_IF_CC1_SHIFT                  2                                  /**< Shift value for RTCC_CC1 */
#define _RTCC_IF_CC1_MASK                   0x4UL                              /**< Bit mask for RTCC_CC1 */
#define _RTCC_IF_CC1_DEFAULT                0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_CC1_DEFAULT                 (_RTCC_IF_CC1_DEFAULT << 2)        /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_CC2                         (0x1UL << 3)                       /**< Channel 2 Interrupt Flag */
#define _RTCC_IF_CC2_SHIFT                  3                                  /**< Shift value for RTCC_CC2 */
#define _RTCC_IF_CC2_MASK                   0x8UL                              /**< Bit mask for RTCC_CC2 */
#define _RTCC_IF_CC2_DEFAULT                0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_CC2_DEFAULT                 (_RTCC_IF_CC2_DEFAULT << 3)        /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_OSCFAIL                     (0x1UL << 4)                       /**< Oscillator failure Interrupt Flag */
#define _RTCC_IF_OSCFAIL_SHIFT              4                                  /**< Shift value for RTCC_OSCFAIL */
#define _RTCC_IF_OSCFAIL_MASK               0x10UL                             /**< Bit mask for RTCC_OSCFAIL */
#define _RTCC_IF_OSCFAIL_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_OSCFAIL_DEFAULT             (_RTCC_IF_OSCFAIL_DEFAULT << 4)    /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_CNTTICK                     (0x1UL << 5)                       /**< Main counter tick */
#define _RTCC_IF_CNTTICK_SHIFT              5                                  /**< Shift value for RTCC_CNTTICK */
#define _RTCC_IF_CNTTICK_MASK               0x20UL                             /**< Bit mask for RTCC_CNTTICK */
#define _RTCC_IF_CNTTICK_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_CNTTICK_DEFAULT             (_RTCC_IF_CNTTICK_DEFAULT << 5)    /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_MINTICK                     (0x1UL << 6)                       /**< Minute tick */
#define _RTCC_IF_MINTICK_SHIFT              6                                  /**< Shift value for RTCC_MINTICK */
#define _RTCC_IF_MINTICK_MASK               0x40UL                             /**< Bit mask for RTCC_MINTICK */
#define _RTCC_IF_MINTICK_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_MINTICK_DEFAULT             (_RTCC_IF_MINTICK_DEFAULT << 6)    /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_HOURTICK                    (0x1UL << 7)                       /**< Hour tick */
#define _RTCC_IF_HOURTICK_SHIFT             7                                  /**< Shift value for RTCC_HOURTICK */
#define _RTCC_IF_HOURTICK_MASK              0x80UL                             /**< Bit mask for RTCC_HOURTICK */
#define _RTCC_IF_HOURTICK_DEFAULT           0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_HOURTICK_DEFAULT            (_RTCC_IF_HOURTICK_DEFAULT << 7)   /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_DAYTICK                     (0x1UL << 8)                       /**< Day tick */
#define _RTCC_IF_DAYTICK_SHIFT              8                                  /**< Shift value for RTCC_DAYTICK */
#define _RTCC_IF_DAYTICK_MASK               0x100UL                            /**< Bit mask for RTCC_DAYTICK */
#define _RTCC_IF_DAYTICK_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_DAYTICK_DEFAULT             (_RTCC_IF_DAYTICK_DEFAULT << 8)    /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_DAYOWOF                     (0x1UL << 9)                       /**< Day of week overflow */
#define _RTCC_IF_DAYOWOF_SHIFT              9                                  /**< Shift value for RTCC_DAYOWOF */
#define _RTCC_IF_DAYOWOF_MASK               0x200UL                            /**< Bit mask for RTCC_DAYOWOF */
#define _RTCC_IF_DAYOWOF_DEFAULT            0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_DAYOWOF_DEFAULT             (_RTCC_IF_DAYOWOF_DEFAULT << 9)    /**< Shifted mode DEFAULT for RTCC_IF */
#define RTCC_IF_MONTHTICK                   (0x1UL << 10)                      /**< Month tick */
#define _RTCC_IF_MONTHTICK_SHIFT            10                                 /**< Shift value for RTCC_MONTHTICK */
#define _RTCC_IF_MONTHTICK_MASK             0x400UL                            /**< Bit mask for RTCC_MONTHTICK */
#define _RTCC_IF_MONTHTICK_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for RTCC_IF */
#define RTCC_IF_MONTHTICK_DEFAULT           (_RTCC_IF_MONTHTICK_DEFAULT << 10) /**< Shifted mode DEFAULT for RTCC_IF */

/* Bit fields for RTCC IFS */
#define _RTCC_IFS_RESETVALUE                0x00000000UL                        /**< Default value for RTCC_IFS */
#define _RTCC_IFS_MASK                      0x000007FFUL                        /**< Mask for RTCC_IFS */
#define RTCC_IFS_OF                         (0x1UL << 0)                        /**< Set OF Interrupt Flag */
#define _RTCC_IFS_OF_SHIFT                  0                                   /**< Shift value for RTCC_OF */
#define _RTCC_IFS_OF_MASK                   0x1UL                               /**< Bit mask for RTCC_OF */
#define _RTCC_IFS_OF_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_OF_DEFAULT                 (_RTCC_IFS_OF_DEFAULT << 0)         /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_CC0                        (0x1UL << 1)                        /**< Set CC0 Interrupt Flag */
#define _RTCC_IFS_CC0_SHIFT                 1                                   /**< Shift value for RTCC_CC0 */
#define _RTCC_IFS_CC0_MASK                  0x2UL                               /**< Bit mask for RTCC_CC0 */
#define _RTCC_IFS_CC0_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_CC0_DEFAULT                (_RTCC_IFS_CC0_DEFAULT << 1)        /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_CC1                        (0x1UL << 2)                        /**< Set CC1 Interrupt Flag */
#define _RTCC_IFS_CC1_SHIFT                 2                                   /**< Shift value for RTCC_CC1 */
#define _RTCC_IFS_CC1_MASK                  0x4UL                               /**< Bit mask for RTCC_CC1 */
#define _RTCC_IFS_CC1_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_CC1_DEFAULT                (_RTCC_IFS_CC1_DEFAULT << 2)        /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_CC2                        (0x1UL << 3)                        /**< Set CC2 Interrupt Flag */
#define _RTCC_IFS_CC2_SHIFT                 3                                   /**< Shift value for RTCC_CC2 */
#define _RTCC_IFS_CC2_MASK                  0x8UL                               /**< Bit mask for RTCC_CC2 */
#define _RTCC_IFS_CC2_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_CC2_DEFAULT                (_RTCC_IFS_CC2_DEFAULT << 3)        /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_OSCFAIL                    (0x1UL << 4)                        /**< Set OSCFAIL Interrupt Flag */
#define _RTCC_IFS_OSCFAIL_SHIFT             4                                   /**< Shift value for RTCC_OSCFAIL */
#define _RTCC_IFS_OSCFAIL_MASK              0x10UL                              /**< Bit mask for RTCC_OSCFAIL */
#define _RTCC_IFS_OSCFAIL_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_OSCFAIL_DEFAULT            (_RTCC_IFS_OSCFAIL_DEFAULT << 4)    /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_CNTTICK                    (0x1UL << 5)                        /**< Set CNTTICK Interrupt Flag */
#define _RTCC_IFS_CNTTICK_SHIFT             5                                   /**< Shift value for RTCC_CNTTICK */
#define _RTCC_IFS_CNTTICK_MASK              0x20UL                              /**< Bit mask for RTCC_CNTTICK */
#define _RTCC_IFS_CNTTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_CNTTICK_DEFAULT            (_RTCC_IFS_CNTTICK_DEFAULT << 5)    /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_MINTICK                    (0x1UL << 6)                        /**< Set MINTICK Interrupt Flag */
#define _RTCC_IFS_MINTICK_SHIFT             6                                   /**< Shift value for RTCC_MINTICK */
#define _RTCC_IFS_MINTICK_MASK              0x40UL                              /**< Bit mask for RTCC_MINTICK */
#define _RTCC_IFS_MINTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_MINTICK_DEFAULT            (_RTCC_IFS_MINTICK_DEFAULT << 6)    /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_HOURTICK                   (0x1UL << 7)                        /**< Set HOURTICK Interrupt Flag */
#define _RTCC_IFS_HOURTICK_SHIFT            7                                   /**< Shift value for RTCC_HOURTICK */
#define _RTCC_IFS_HOURTICK_MASK             0x80UL                              /**< Bit mask for RTCC_HOURTICK */
#define _RTCC_IFS_HOURTICK_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_HOURTICK_DEFAULT           (_RTCC_IFS_HOURTICK_DEFAULT << 7)   /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_DAYTICK                    (0x1UL << 8)                        /**< Set DAYTICK Interrupt Flag */
#define _RTCC_IFS_DAYTICK_SHIFT             8                                   /**< Shift value for RTCC_DAYTICK */
#define _RTCC_IFS_DAYTICK_MASK              0x100UL                             /**< Bit mask for RTCC_DAYTICK */
#define _RTCC_IFS_DAYTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_DAYTICK_DEFAULT            (_RTCC_IFS_DAYTICK_DEFAULT << 8)    /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_DAYOWOF                    (0x1UL << 9)                        /**< Set DAYOWOF Interrupt Flag */
#define _RTCC_IFS_DAYOWOF_SHIFT             9                                   /**< Shift value for RTCC_DAYOWOF */
#define _RTCC_IFS_DAYOWOF_MASK              0x200UL                             /**< Bit mask for RTCC_DAYOWOF */
#define _RTCC_IFS_DAYOWOF_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_DAYOWOF_DEFAULT            (_RTCC_IFS_DAYOWOF_DEFAULT << 9)    /**< Shifted mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_MONTHTICK                  (0x1UL << 10)                       /**< Set MONTHTICK Interrupt Flag */
#define _RTCC_IFS_MONTHTICK_SHIFT           10                                  /**< Shift value for RTCC_MONTHTICK */
#define _RTCC_IFS_MONTHTICK_MASK            0x400UL                             /**< Bit mask for RTCC_MONTHTICK */
#define _RTCC_IFS_MONTHTICK_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for RTCC_IFS */
#define RTCC_IFS_MONTHTICK_DEFAULT          (_RTCC_IFS_MONTHTICK_DEFAULT << 10) /**< Shifted mode DEFAULT for RTCC_IFS */

/* Bit fields for RTCC IFC */
#define _RTCC_IFC_RESETVALUE                0x00000000UL                        /**< Default value for RTCC_IFC */
#define _RTCC_IFC_MASK                      0x000007FFUL                        /**< Mask for RTCC_IFC */
#define RTCC_IFC_OF                         (0x1UL << 0)                        /**< Clear OF Interrupt Flag */
#define _RTCC_IFC_OF_SHIFT                  0                                   /**< Shift value for RTCC_OF */
#define _RTCC_IFC_OF_MASK                   0x1UL                               /**< Bit mask for RTCC_OF */
#define _RTCC_IFC_OF_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_OF_DEFAULT                 (_RTCC_IFC_OF_DEFAULT << 0)         /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_CC0                        (0x1UL << 1)                        /**< Clear CC0 Interrupt Flag */
#define _RTCC_IFC_CC0_SHIFT                 1                                   /**< Shift value for RTCC_CC0 */
#define _RTCC_IFC_CC0_MASK                  0x2UL                               /**< Bit mask for RTCC_CC0 */
#define _RTCC_IFC_CC0_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_CC0_DEFAULT                (_RTCC_IFC_CC0_DEFAULT << 1)        /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_CC1                        (0x1UL << 2)                        /**< Clear CC1 Interrupt Flag */
#define _RTCC_IFC_CC1_SHIFT                 2                                   /**< Shift value for RTCC_CC1 */
#define _RTCC_IFC_CC1_MASK                  0x4UL                               /**< Bit mask for RTCC_CC1 */
#define _RTCC_IFC_CC1_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_CC1_DEFAULT                (_RTCC_IFC_CC1_DEFAULT << 2)        /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_CC2                        (0x1UL << 3)                        /**< Clear CC2 Interrupt Flag */
#define _RTCC_IFC_CC2_SHIFT                 3                                   /**< Shift value for RTCC_CC2 */
#define _RTCC_IFC_CC2_MASK                  0x8UL                               /**< Bit mask for RTCC_CC2 */
#define _RTCC_IFC_CC2_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_CC2_DEFAULT                (_RTCC_IFC_CC2_DEFAULT << 3)        /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_OSCFAIL                    (0x1UL << 4)                        /**< Clear OSCFAIL Interrupt Flag */
#define _RTCC_IFC_OSCFAIL_SHIFT             4                                   /**< Shift value for RTCC_OSCFAIL */
#define _RTCC_IFC_OSCFAIL_MASK              0x10UL                              /**< Bit mask for RTCC_OSCFAIL */
#define _RTCC_IFC_OSCFAIL_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_OSCFAIL_DEFAULT            (_RTCC_IFC_OSCFAIL_DEFAULT << 4)    /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_CNTTICK                    (0x1UL << 5)                        /**< Clear CNTTICK Interrupt Flag */
#define _RTCC_IFC_CNTTICK_SHIFT             5                                   /**< Shift value for RTCC_CNTTICK */
#define _RTCC_IFC_CNTTICK_MASK              0x20UL                              /**< Bit mask for RTCC_CNTTICK */
#define _RTCC_IFC_CNTTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_CNTTICK_DEFAULT            (_RTCC_IFC_CNTTICK_DEFAULT << 5)    /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_MINTICK                    (0x1UL << 6)                        /**< Clear MINTICK Interrupt Flag */
#define _RTCC_IFC_MINTICK_SHIFT             6                                   /**< Shift value for RTCC_MINTICK */
#define _RTCC_IFC_MINTICK_MASK              0x40UL                              /**< Bit mask for RTCC_MINTICK */
#define _RTCC_IFC_MINTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_MINTICK_DEFAULT            (_RTCC_IFC_MINTICK_DEFAULT << 6)    /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_HOURTICK                   (0x1UL << 7)                        /**< Clear HOURTICK Interrupt Flag */
#define _RTCC_IFC_HOURTICK_SHIFT            7                                   /**< Shift value for RTCC_HOURTICK */
#define _RTCC_IFC_HOURTICK_MASK             0x80UL                              /**< Bit mask for RTCC_HOURTICK */
#define _RTCC_IFC_HOURTICK_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_HOURTICK_DEFAULT           (_RTCC_IFC_HOURTICK_DEFAULT << 7)   /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_DAYTICK                    (0x1UL << 8)                        /**< Clear DAYTICK Interrupt Flag */
#define _RTCC_IFC_DAYTICK_SHIFT             8                                   /**< Shift value for RTCC_DAYTICK */
#define _RTCC_IFC_DAYTICK_MASK              0x100UL                             /**< Bit mask for RTCC_DAYTICK */
#define _RTCC_IFC_DAYTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_DAYTICK_DEFAULT            (_RTCC_IFC_DAYTICK_DEFAULT << 8)    /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_DAYOWOF                    (0x1UL << 9)                        /**< Clear DAYOWOF Interrupt Flag */
#define _RTCC_IFC_DAYOWOF_SHIFT             9                                   /**< Shift value for RTCC_DAYOWOF */
#define _RTCC_IFC_DAYOWOF_MASK              0x200UL                             /**< Bit mask for RTCC_DAYOWOF */
#define _RTCC_IFC_DAYOWOF_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_DAYOWOF_DEFAULT            (_RTCC_IFC_DAYOWOF_DEFAULT << 9)    /**< Shifted mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_MONTHTICK                  (0x1UL << 10)                       /**< Clear MONTHTICK Interrupt Flag */
#define _RTCC_IFC_MONTHTICK_SHIFT           10                                  /**< Shift value for RTCC_MONTHTICK */
#define _RTCC_IFC_MONTHTICK_MASK            0x400UL                             /**< Bit mask for RTCC_MONTHTICK */
#define _RTCC_IFC_MONTHTICK_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for RTCC_IFC */
#define RTCC_IFC_MONTHTICK_DEFAULT          (_RTCC_IFC_MONTHTICK_DEFAULT << 10) /**< Shifted mode DEFAULT for RTCC_IFC */

/* Bit fields for RTCC IEN */
#define _RTCC_IEN_RESETVALUE                0x00000000UL                        /**< Default value for RTCC_IEN */
#define _RTCC_IEN_MASK                      0x000007FFUL                        /**< Mask for RTCC_IEN */
#define RTCC_IEN_OF                         (0x1UL << 0)                        /**< OF Interrupt Enable */
#define _RTCC_IEN_OF_SHIFT                  0                                   /**< Shift value for RTCC_OF */
#define _RTCC_IEN_OF_MASK                   0x1UL                               /**< Bit mask for RTCC_OF */
#define _RTCC_IEN_OF_DEFAULT                0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_OF_DEFAULT                 (_RTCC_IEN_OF_DEFAULT << 0)         /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_CC0                        (0x1UL << 1)                        /**< CC0 Interrupt Enable */
#define _RTCC_IEN_CC0_SHIFT                 1                                   /**< Shift value for RTCC_CC0 */
#define _RTCC_IEN_CC0_MASK                  0x2UL                               /**< Bit mask for RTCC_CC0 */
#define _RTCC_IEN_CC0_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_CC0_DEFAULT                (_RTCC_IEN_CC0_DEFAULT << 1)        /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_CC1                        (0x1UL << 2)                        /**< CC1 Interrupt Enable */
#define _RTCC_IEN_CC1_SHIFT                 2                                   /**< Shift value for RTCC_CC1 */
#define _RTCC_IEN_CC1_MASK                  0x4UL                               /**< Bit mask for RTCC_CC1 */
#define _RTCC_IEN_CC1_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_CC1_DEFAULT                (_RTCC_IEN_CC1_DEFAULT << 2)        /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_CC2                        (0x1UL << 3)                        /**< CC2 Interrupt Enable */
#define _RTCC_IEN_CC2_SHIFT                 3                                   /**< Shift value for RTCC_CC2 */
#define _RTCC_IEN_CC2_MASK                  0x8UL                               /**< Bit mask for RTCC_CC2 */
#define _RTCC_IEN_CC2_DEFAULT               0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_CC2_DEFAULT                (_RTCC_IEN_CC2_DEFAULT << 3)        /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_OSCFAIL                    (0x1UL << 4)                        /**< OSCFAIL Interrupt Enable */
#define _RTCC_IEN_OSCFAIL_SHIFT             4                                   /**< Shift value for RTCC_OSCFAIL */
#define _RTCC_IEN_OSCFAIL_MASK              0x10UL                              /**< Bit mask for RTCC_OSCFAIL */
#define _RTCC_IEN_OSCFAIL_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_OSCFAIL_DEFAULT            (_RTCC_IEN_OSCFAIL_DEFAULT << 4)    /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_CNTTICK                    (0x1UL << 5)                        /**< CNTTICK Interrupt Enable */
#define _RTCC_IEN_CNTTICK_SHIFT             5                                   /**< Shift value for RTCC_CNTTICK */
#define _RTCC_IEN_CNTTICK_MASK              0x20UL                              /**< Bit mask for RTCC_CNTTICK */
#define _RTCC_IEN_CNTTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_CNTTICK_DEFAULT            (_RTCC_IEN_CNTTICK_DEFAULT << 5)    /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_MINTICK                    (0x1UL << 6)                        /**< MINTICK Interrupt Enable */
#define _RTCC_IEN_MINTICK_SHIFT             6                                   /**< Shift value for RTCC_MINTICK */
#define _RTCC_IEN_MINTICK_MASK              0x40UL                              /**< Bit mask for RTCC_MINTICK */
#define _RTCC_IEN_MINTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_MINTICK_DEFAULT            (_RTCC_IEN_MINTICK_DEFAULT << 6)    /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_HOURTICK                   (0x1UL << 7)                        /**< HOURTICK Interrupt Enable */
#define _RTCC_IEN_HOURTICK_SHIFT            7                                   /**< Shift value for RTCC_HOURTICK */
#define _RTCC_IEN_HOURTICK_MASK             0x80UL                              /**< Bit mask for RTCC_HOURTICK */
#define _RTCC_IEN_HOURTICK_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_HOURTICK_DEFAULT           (_RTCC_IEN_HOURTICK_DEFAULT << 7)   /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_DAYTICK                    (0x1UL << 8)                        /**< DAYTICK Interrupt Enable */
#define _RTCC_IEN_DAYTICK_SHIFT             8                                   /**< Shift value for RTCC_DAYTICK */
#define _RTCC_IEN_DAYTICK_MASK              0x100UL                             /**< Bit mask for RTCC_DAYTICK */
#define _RTCC_IEN_DAYTICK_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_DAYTICK_DEFAULT            (_RTCC_IEN_DAYTICK_DEFAULT << 8)    /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_DAYOWOF                    (0x1UL << 9)                        /**< DAYOWOF Interrupt Enable */
#define _RTCC_IEN_DAYOWOF_SHIFT             9                                   /**< Shift value for RTCC_DAYOWOF */
#define _RTCC_IEN_DAYOWOF_MASK              0x200UL                             /**< Bit mask for RTCC_DAYOWOF */
#define _RTCC_IEN_DAYOWOF_DEFAULT           0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_DAYOWOF_DEFAULT            (_RTCC_IEN_DAYOWOF_DEFAULT << 9)    /**< Shifted mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_MONTHTICK                  (0x1UL << 10)                       /**< MONTHTICK Interrupt Enable */
#define _RTCC_IEN_MONTHTICK_SHIFT           10                                  /**< Shift value for RTCC_MONTHTICK */
#define _RTCC_IEN_MONTHTICK_MASK            0x400UL                             /**< Bit mask for RTCC_MONTHTICK */
#define _RTCC_IEN_MONTHTICK_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for RTCC_IEN */
#define RTCC_IEN_MONTHTICK_DEFAULT          (_RTCC_IEN_MONTHTICK_DEFAULT << 10) /**< Shifted mode DEFAULT for RTCC_IEN */

/* Bit fields for RTCC STATUS */
#define _RTCC_STATUS_RESETVALUE             0x00000000UL /**< Default value for RTCC_STATUS */
#define _RTCC_STATUS_MASK                   0x00000000UL /**< Mask for RTCC_STATUS */

/* Bit fields for RTCC CMD */
#define _RTCC_CMD_RESETVALUE                0x00000000UL                       /**< Default value for RTCC_CMD */
#define _RTCC_CMD_MASK                      0x00000001UL                       /**< Mask for RTCC_CMD */
#define RTCC_CMD_CLRSTATUS                  (0x1UL << 0)                       /**< Clear RTCC_STATUS register. */
#define _RTCC_CMD_CLRSTATUS_SHIFT           0                                  /**< Shift value for RTCC_CLRSTATUS */
#define _RTCC_CMD_CLRSTATUS_MASK            0x1UL                              /**< Bit mask for RTCC_CLRSTATUS */
#define _RTCC_CMD_CLRSTATUS_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for RTCC_CMD */
#define RTCC_CMD_CLRSTATUS_DEFAULT          (_RTCC_CMD_CLRSTATUS_DEFAULT << 0) /**< Shifted mode DEFAULT for RTCC_CMD */

/* Bit fields for RTCC SYNCBUSY */
#define _RTCC_SYNCBUSY_RESETVALUE           0x00000000UL                      /**< Default value for RTCC_SYNCBUSY */
#define _RTCC_SYNCBUSY_MASK                 0x00000020UL                      /**< Mask for RTCC_SYNCBUSY */
#define RTCC_SYNCBUSY_CMD                   (0x1UL << 5)                      /**< CMD Register Busy */
#define _RTCC_SYNCBUSY_CMD_SHIFT            5                                 /**< Shift value for RTCC_CMD */
#define _RTCC_SYNCBUSY_CMD_MASK             0x20UL                            /**< Bit mask for RTCC_CMD */
#define _RTCC_SYNCBUSY_CMD_DEFAULT          0x00000000UL                      /**< Mode DEFAULT for RTCC_SYNCBUSY */
#define RTCC_SYNCBUSY_CMD_DEFAULT           (_RTCC_SYNCBUSY_CMD_DEFAULT << 5) /**< Shifted mode DEFAULT for RTCC_SYNCBUSY */

/* Bit fields for RTCC POWERDOWN */
#define _RTCC_POWERDOWN_RESETVALUE          0x00000000UL                       /**< Default value for RTCC_POWERDOWN */
#define _RTCC_POWERDOWN_MASK                0x00000001UL                       /**< Mask for RTCC_POWERDOWN */
#define RTCC_POWERDOWN_RAM                  (0x1UL << 0)                       /**< Retention RAM power-down */
#define _RTCC_POWERDOWN_RAM_SHIFT           0                                  /**< Shift value for RTCC_RAM */
#define _RTCC_POWERDOWN_RAM_MASK            0x1UL                              /**< Bit mask for RTCC_RAM */
#define _RTCC_POWERDOWN_RAM_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for RTCC_POWERDOWN */
#define RTCC_POWERDOWN_RAM_DEFAULT          (_RTCC_POWERDOWN_RAM_DEFAULT << 0) /**< Shifted mode DEFAULT for RTCC_POWERDOWN */

/* Bit fields for RTCC LOCK */
#define _RTCC_LOCK_RESETVALUE               0x00000000UL                       /**< Default value for RTCC_LOCK */
#define _RTCC_LOCK_MASK                     0x0000FFFFUL                       /**< Mask for RTCC_LOCK */
#define _RTCC_LOCK_LOCKKEY_SHIFT            0                                  /**< Shift value for RTCC_LOCKKEY */
#define _RTCC_LOCK_LOCKKEY_MASK             0xFFFFUL                           /**< Bit mask for RTCC_LOCKKEY */
#define _RTCC_LOCK_LOCKKEY_DEFAULT          0x00000000UL                       /**< Mode DEFAULT for RTCC_LOCK */
#define _RTCC_LOCK_LOCKKEY_LOCK             0x00000000UL                       /**< Mode LOCK for RTCC_LOCK */
#define _RTCC_LOCK_LOCKKEY_UNLOCKED         0x00000000UL                       /**< Mode UNLOCKED for RTCC_LOCK */
#define _RTCC_LOCK_LOCKKEY_LOCKED           0x00000001UL                       /**< Mode LOCKED for RTCC_LOCK */
#define _RTCC_LOCK_LOCKKEY_UNLOCK           0x0000AEE8UL                       /**< Mode UNLOCK for RTCC_LOCK */
#define RTCC_LOCK_LOCKKEY_DEFAULT           (_RTCC_LOCK_LOCKKEY_DEFAULT << 0)  /**< Shifted mode DEFAULT for RTCC_LOCK */
#define RTCC_LOCK_LOCKKEY_LOCK              (_RTCC_LOCK_LOCKKEY_LOCK << 0)     /**< Shifted mode LOCK for RTCC_LOCK */
#define RTCC_LOCK_LOCKKEY_UNLOCKED          (_RTCC_LOCK_LOCKKEY_UNLOCKED << 0) /**< Shifted mode UNLOCKED for RTCC_LOCK */
#define RTCC_LOCK_LOCKKEY_LOCKED            (_RTCC_LOCK_LOCKKEY_LOCKED << 0)   /**< Shifted mode LOCKED for RTCC_LOCK */
#define RTCC_LOCK_LOCKKEY_UNLOCK            (_RTCC_LOCK_LOCKKEY_UNLOCK << 0)   /**< Shifted mode UNLOCK for RTCC_LOCK */

/* Bit fields for RTCC EM4WUEN */
#define _RTCC_EM4WUEN_RESETVALUE            0x00000000UL                       /**< Default value for RTCC_EM4WUEN */
#define _RTCC_EM4WUEN_MASK                  0x00000001UL                       /**< Mask for RTCC_EM4WUEN */
#define RTCC_EM4WUEN_EM4WU                  (0x1UL << 0)                       /**< EM4 Wake-up enable */
#define _RTCC_EM4WUEN_EM4WU_SHIFT           0                                  /**< Shift value for RTCC_EM4WU */
#define _RTCC_EM4WUEN_EM4WU_MASK            0x1UL                              /**< Bit mask for RTCC_EM4WU */
#define _RTCC_EM4WUEN_EM4WU_DEFAULT         0x00000000UL                       /**< Mode DEFAULT for RTCC_EM4WUEN */
#define RTCC_EM4WUEN_EM4WU_DEFAULT          (_RTCC_EM4WUEN_EM4WU_DEFAULT << 0) /**< Shifted mode DEFAULT for RTCC_EM4WUEN */

/* Bit fields for RTCC CC_CTRL */
#define _RTCC_CC_CTRL_RESETVALUE            0x00000000UL                            /**< Default value for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_MASK                  0x0003FBFFUL                            /**< Mask for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_MODE_SHIFT            0                                       /**< Shift value for CC_MODE */
#define _RTCC_CC_CTRL_MODE_MASK             0x3UL                                   /**< Bit mask for CC_MODE */
#define _RTCC_CC_CTRL_MODE_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_MODE_OFF              0x00000000UL                            /**< Mode OFF for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_MODE_INPUTCAPTURE     0x00000001UL                            /**< Mode INPUTCAPTURE for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_MODE_OUTPUTCOMPARE    0x00000002UL                            /**< Mode OUTPUTCOMPARE for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_MODE_DEFAULT           (_RTCC_CC_CTRL_MODE_DEFAULT << 0)       /**< Shifted mode DEFAULT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_MODE_OFF               (_RTCC_CC_CTRL_MODE_OFF << 0)           /**< Shifted mode OFF for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_MODE_INPUTCAPTURE      (_RTCC_CC_CTRL_MODE_INPUTCAPTURE << 0)  /**< Shifted mode INPUTCAPTURE for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_MODE_OUTPUTCOMPARE     (_RTCC_CC_CTRL_MODE_OUTPUTCOMPARE << 0) /**< Shifted mode OUTPUTCOMPARE for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_CMOA_SHIFT            2                                       /**< Shift value for CC_CMOA */
#define _RTCC_CC_CTRL_CMOA_MASK             0xCUL                                   /**< Bit mask for CC_CMOA */
#define _RTCC_CC_CTRL_CMOA_DEFAULT          0x00000000UL                            /**< Mode DEFAULT for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_CMOA_PULSE            0x00000000UL                            /**< Mode PULSE for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_CMOA_TOGGLE           0x00000001UL                            /**< Mode TOGGLE for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_CMOA_CLEAR            0x00000002UL                            /**< Mode CLEAR for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_CMOA_SET              0x00000003UL                            /**< Mode SET for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_CMOA_DEFAULT           (_RTCC_CC_CTRL_CMOA_DEFAULT << 2)       /**< Shifted mode DEFAULT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_CMOA_PULSE             (_RTCC_CC_CTRL_CMOA_PULSE << 2)         /**< Shifted mode PULSE for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_CMOA_TOGGLE            (_RTCC_CC_CTRL_CMOA_TOGGLE << 2)        /**< Shifted mode TOGGLE for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_CMOA_CLEAR             (_RTCC_CC_CTRL_CMOA_CLEAR << 2)         /**< Shifted mode CLEAR for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_CMOA_SET               (_RTCC_CC_CTRL_CMOA_SET << 2)           /**< Shifted mode SET for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_ICEDGE_SHIFT          4                                       /**< Shift value for CC_ICEDGE */
#define _RTCC_CC_CTRL_ICEDGE_MASK           0x30UL                                  /**< Bit mask for CC_ICEDGE */
#define _RTCC_CC_CTRL_ICEDGE_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_ICEDGE_RISING         0x00000000UL                            /**< Mode RISING for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_ICEDGE_FALLING        0x00000001UL                            /**< Mode FALLING for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_ICEDGE_BOTH           0x00000002UL                            /**< Mode BOTH for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_ICEDGE_NONE           0x00000003UL                            /**< Mode NONE for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_ICEDGE_DEFAULT         (_RTCC_CC_CTRL_ICEDGE_DEFAULT << 4)     /**< Shifted mode DEFAULT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_ICEDGE_RISING          (_RTCC_CC_CTRL_ICEDGE_RISING << 4)      /**< Shifted mode RISING for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_ICEDGE_FALLING         (_RTCC_CC_CTRL_ICEDGE_FALLING << 4)     /**< Shifted mode FALLING for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_ICEDGE_BOTH            (_RTCC_CC_CTRL_ICEDGE_BOTH << 4)        /**< Shifted mode BOTH for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_ICEDGE_NONE            (_RTCC_CC_CTRL_ICEDGE_NONE << 4)        /**< Shifted mode NONE for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_SHIFT          6                                       /**< Shift value for CC_PRSSEL */
#define _RTCC_CC_CTRL_PRSSEL_MASK           0x3C0UL                                 /**< Bit mask for CC_PRSSEL */
#define _RTCC_CC_CTRL_PRSSEL_DEFAULT        0x00000000UL                            /**< Mode DEFAULT for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH0         0x00000000UL                            /**< Mode PRSCH0 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH1         0x00000001UL                            /**< Mode PRSCH1 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH2         0x00000002UL                            /**< Mode PRSCH2 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH3         0x00000003UL                            /**< Mode PRSCH3 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH4         0x00000004UL                            /**< Mode PRSCH4 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH5         0x00000005UL                            /**< Mode PRSCH5 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH6         0x00000006UL                            /**< Mode PRSCH6 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH7         0x00000007UL                            /**< Mode PRSCH7 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH8         0x00000008UL                            /**< Mode PRSCH8 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH9         0x00000009UL                            /**< Mode PRSCH9 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH10        0x0000000AUL                            /**< Mode PRSCH10 for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_PRSSEL_PRSCH11        0x0000000BUL                            /**< Mode PRSCH11 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_DEFAULT         (_RTCC_CC_CTRL_PRSSEL_DEFAULT << 6)     /**< Shifted mode DEFAULT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH0          (_RTCC_CC_CTRL_PRSSEL_PRSCH0 << 6)      /**< Shifted mode PRSCH0 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH1          (_RTCC_CC_CTRL_PRSSEL_PRSCH1 << 6)      /**< Shifted mode PRSCH1 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH2          (_RTCC_CC_CTRL_PRSSEL_PRSCH2 << 6)      /**< Shifted mode PRSCH2 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH3          (_RTCC_CC_CTRL_PRSSEL_PRSCH3 << 6)      /**< Shifted mode PRSCH3 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH4          (_RTCC_CC_CTRL_PRSSEL_PRSCH4 << 6)      /**< Shifted mode PRSCH4 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH5          (_RTCC_CC_CTRL_PRSSEL_PRSCH5 << 6)      /**< Shifted mode PRSCH5 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH6          (_RTCC_CC_CTRL_PRSSEL_PRSCH6 << 6)      /**< Shifted mode PRSCH6 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH7          (_RTCC_CC_CTRL_PRSSEL_PRSCH7 << 6)      /**< Shifted mode PRSCH7 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH8          (_RTCC_CC_CTRL_PRSSEL_PRSCH8 << 6)      /**< Shifted mode PRSCH8 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH9          (_RTCC_CC_CTRL_PRSSEL_PRSCH9 << 6)      /**< Shifted mode PRSCH9 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH10         (_RTCC_CC_CTRL_PRSSEL_PRSCH10 << 6)     /**< Shifted mode PRSCH10 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_PRSSEL_PRSCH11         (_RTCC_CC_CTRL_PRSSEL_PRSCH11 << 6)     /**< Shifted mode PRSCH11 for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_COMPBASE               (0x1UL << 11)                           /**< Capture compare channel comparison base. */
#define _RTCC_CC_CTRL_COMPBASE_SHIFT        11                                      /**< Shift value for CC_COMPBASE */
#define _RTCC_CC_CTRL_COMPBASE_MASK         0x800UL                                 /**< Bit mask for CC_COMPBASE */
#define _RTCC_CC_CTRL_COMPBASE_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_COMPBASE_CNT          0x00000000UL                            /**< Mode CNT for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_COMPBASE_PRECNT       0x00000001UL                            /**< Mode PRECNT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_COMPBASE_DEFAULT       (_RTCC_CC_CTRL_COMPBASE_DEFAULT << 11)  /**< Shifted mode DEFAULT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_COMPBASE_CNT           (_RTCC_CC_CTRL_COMPBASE_CNT << 11)      /**< Shifted mode CNT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_COMPBASE_PRECNT        (_RTCC_CC_CTRL_COMPBASE_PRECNT << 11)   /**< Shifted mode PRECNT for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_COMPMASK_SHIFT        12                                      /**< Shift value for CC_COMPMASK */
#define _RTCC_CC_CTRL_COMPMASK_MASK         0x1F000UL                               /**< Bit mask for CC_COMPMASK */
#define _RTCC_CC_CTRL_COMPMASK_DEFAULT      0x00000000UL                            /**< Mode DEFAULT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_COMPMASK_DEFAULT       (_RTCC_CC_CTRL_COMPMASK_DEFAULT << 12)  /**< Shifted mode DEFAULT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_DAYCC                  (0x1UL << 17)                           /**< Day Capture/Compare selection */
#define _RTCC_CC_CTRL_DAYCC_SHIFT           17                                      /**< Shift value for CC_DAYCC */
#define _RTCC_CC_CTRL_DAYCC_MASK            0x20000UL                               /**< Bit mask for CC_DAYCC */
#define _RTCC_CC_CTRL_DAYCC_DEFAULT         0x00000000UL                            /**< Mode DEFAULT for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_DAYCC_MONTH           0x00000000UL                            /**< Mode MONTH for RTCC_CC_CTRL */
#define _RTCC_CC_CTRL_DAYCC_WEEK            0x00000001UL                            /**< Mode WEEK for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_DAYCC_DEFAULT          (_RTCC_CC_CTRL_DAYCC_DEFAULT << 17)     /**< Shifted mode DEFAULT for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_DAYCC_MONTH            (_RTCC_CC_CTRL_DAYCC_MONTH << 17)       /**< Shifted mode MONTH for RTCC_CC_CTRL */
#define RTCC_CC_CTRL_DAYCC_WEEK             (_RTCC_CC_CTRL_DAYCC_WEEK << 17)        /**< Shifted mode WEEK for RTCC_CC_CTRL */

/* Bit fields for RTCC CC_CCV */
#define _RTCC_CC_CCV_RESETVALUE             0x00000000UL                    /**< Default value for RTCC_CC_CCV */
#define _RTCC_CC_CCV_MASK                   0xFFFFFFFFUL                    /**< Mask for RTCC_CC_CCV */
#define _RTCC_CC_CCV_CCV_SHIFT              0                               /**< Shift value for CC_CCV */
#define _RTCC_CC_CCV_CCV_MASK               0xFFFFFFFFUL                    /**< Bit mask for CC_CCV */
#define _RTCC_CC_CCV_CCV_DEFAULT            0x00000000UL                    /**< Mode DEFAULT for RTCC_CC_CCV */
#define RTCC_CC_CCV_CCV_DEFAULT             (_RTCC_CC_CCV_CCV_DEFAULT << 0) /**< Shifted mode DEFAULT for RTCC_CC_CCV */

/* Bit fields for RTCC CC_TIME */
#define _RTCC_CC_TIME_RESETVALUE            0x00000000UL                        /**< Default value for RTCC_CC_TIME */
#define _RTCC_CC_TIME_MASK                  0x003F7F7FUL                        /**< Mask for RTCC_CC_TIME */
#define _RTCC_CC_TIME_SECU_SHIFT            0                                   /**< Shift value for CC_SECU */
#define _RTCC_CC_TIME_SECU_MASK             0xFUL                               /**< Bit mask for CC_SECU */
#define _RTCC_CC_TIME_SECU_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for RTCC_CC_TIME */
#define RTCC_CC_TIME_SECU_DEFAULT           (_RTCC_CC_TIME_SECU_DEFAULT << 0)   /**< Shifted mode DEFAULT for RTCC_CC_TIME */
#define _RTCC_CC_TIME_SECT_SHIFT            4                                   /**< Shift value for CC_SECT */
#define _RTCC_CC_TIME_SECT_MASK             0x70UL                              /**< Bit mask for CC_SECT */
#define _RTCC_CC_TIME_SECT_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for RTCC_CC_TIME */
#define RTCC_CC_TIME_SECT_DEFAULT           (_RTCC_CC_TIME_SECT_DEFAULT << 4)   /**< Shifted mode DEFAULT for RTCC_CC_TIME */
#define _RTCC_CC_TIME_MINU_SHIFT            8                                   /**< Shift value for CC_MINU */
#define _RTCC_CC_TIME_MINU_MASK             0xF00UL                             /**< Bit mask for CC_MINU */
#define _RTCC_CC_TIME_MINU_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for RTCC_CC_TIME */
#define RTCC_CC_TIME_MINU_DEFAULT           (_RTCC_CC_TIME_MINU_DEFAULT << 8)   /**< Shifted mode DEFAULT for RTCC_CC_TIME */
#define _RTCC_CC_TIME_MINT_SHIFT            12                                  /**< Shift value for CC_MINT */
#define _RTCC_CC_TIME_MINT_MASK             0x7000UL                            /**< Bit mask for CC_MINT */
#define _RTCC_CC_TIME_MINT_DEFAULT          0x00000000UL                        /**< Mode DEFAULT for RTCC_CC_TIME */
#define RTCC_CC_TIME_MINT_DEFAULT           (_RTCC_CC_TIME_MINT_DEFAULT << 12)  /**< Shifted mode DEFAULT for RTCC_CC_TIME */
#define _RTCC_CC_TIME_HOURU_SHIFT           16                                  /**< Shift value for CC_HOURU */
#define _RTCC_CC_TIME_HOURU_MASK            0xF0000UL                           /**< Bit mask for CC_HOURU */
#define _RTCC_CC_TIME_HOURU_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for RTCC_CC_TIME */
#define RTCC_CC_TIME_HOURU_DEFAULT          (_RTCC_CC_TIME_HOURU_DEFAULT << 16) /**< Shifted mode DEFAULT for RTCC_CC_TIME */
#define _RTCC_CC_TIME_HOURT_SHIFT           20                                  /**< Shift value for CC_HOURT */
#define _RTCC_CC_TIME_HOURT_MASK            0x300000UL                          /**< Bit mask for CC_HOURT */
#define _RTCC_CC_TIME_HOURT_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for RTCC_CC_TIME */
#define RTCC_CC_TIME_HOURT_DEFAULT          (_RTCC_CC_TIME_HOURT_DEFAULT << 20) /**< Shifted mode DEFAULT for RTCC_CC_TIME */

/* Bit fields for RTCC CC_DATE */
#define _RTCC_CC_DATE_RESETVALUE            0x00000000UL                         /**< Default value for RTCC_CC_DATE */
#define _RTCC_CC_DATE_MASK                  0x00001F3FUL                         /**< Mask for RTCC_CC_DATE */
#define _RTCC_CC_DATE_DAYU_SHIFT            0                                    /**< Shift value for CC_DAYU */
#define _RTCC_CC_DATE_DAYU_MASK             0xFUL                                /**< Bit mask for CC_DAYU */
#define _RTCC_CC_DATE_DAYU_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for RTCC_CC_DATE */
#define RTCC_CC_DATE_DAYU_DEFAULT           (_RTCC_CC_DATE_DAYU_DEFAULT << 0)    /**< Shifted mode DEFAULT for RTCC_CC_DATE */
#define _RTCC_CC_DATE_DAYT_SHIFT            4                                    /**< Shift value for CC_DAYT */
#define _RTCC_CC_DATE_DAYT_MASK             0x30UL                               /**< Bit mask for CC_DAYT */
#define _RTCC_CC_DATE_DAYT_DEFAULT          0x00000000UL                         /**< Mode DEFAULT for RTCC_CC_DATE */
#define RTCC_CC_DATE_DAYT_DEFAULT           (_RTCC_CC_DATE_DAYT_DEFAULT << 4)    /**< Shifted mode DEFAULT for RTCC_CC_DATE */
#define _RTCC_CC_DATE_MONTHU_SHIFT          8                                    /**< Shift value for CC_MONTHU */
#define _RTCC_CC_DATE_MONTHU_MASK           0xF00UL                              /**< Bit mask for CC_MONTHU */
#define _RTCC_CC_DATE_MONTHU_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for RTCC_CC_DATE */
#define RTCC_CC_DATE_MONTHU_DEFAULT         (_RTCC_CC_DATE_MONTHU_DEFAULT << 8)  /**< Shifted mode DEFAULT for RTCC_CC_DATE */
#define RTCC_CC_DATE_MONTHT                 (0x1UL << 12)                        /**< Month, tens. */
#define _RTCC_CC_DATE_MONTHT_SHIFT          12                                   /**< Shift value for CC_MONTHT */
#define _RTCC_CC_DATE_MONTHT_MASK           0x1000UL                             /**< Bit mask for CC_MONTHT */
#define _RTCC_CC_DATE_MONTHT_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for RTCC_CC_DATE */
#define RTCC_CC_DATE_MONTHT_DEFAULT         (_RTCC_CC_DATE_MONTHT_DEFAULT << 12) /**< Shifted mode DEFAULT for RTCC_CC_DATE */

/* Bit fields for RTCC RET_REG */
#define _RTCC_RET_REG_RESETVALUE            0x00000000UL                     /**< Default value for RTCC_RET_REG */
#define _RTCC_RET_REG_MASK                  0xFFFFFFFFUL                     /**< Mask for RTCC_RET_REG */
#define _RTCC_RET_REG_REG_SHIFT             0                                /**< Shift value for RET_REG */
#define _RTCC_RET_REG_REG_MASK              0xFFFFFFFFUL                     /**< Bit mask for RET_REG */
#define _RTCC_RET_REG_REG_DEFAULT           0x00000000UL                     /**< Mode DEFAULT for RTCC_RET_REG */
#define RTCC_RET_REG_REG_DEFAULT            (_RTCC_RET_REG_REG_DEFAULT << 0) /**< Shifted mode DEFAULT for RTCC_RET_REG */

/** @} End of group EFR32FG1P_RTCC */
/** @} End of group Parts */

