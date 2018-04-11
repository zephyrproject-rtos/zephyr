/**************************************************************************//**
 * @file efm32pg12b_trng.h
 * @brief EFM32PG12B_TRNG register and bit field definitions
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
 * @defgroup EFM32PG12B_TRNG
 * @{
 * @brief EFM32PG12B_TRNG Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CONTROL;       /**< Main Control Register  */
  __IM uint32_t  FIFOLEVEL;     /**< FIFO Level Register  */
  uint32_t       RESERVED0[1];  /**< Reserved for future use **/
  __IM uint32_t  FIFODEPTH;     /**< FIFO Depth Register  */
  __IOM uint32_t KEY0;          /**< Key Register 0  */
  __IOM uint32_t KEY1;          /**< Key Register 1  */
  __IOM uint32_t KEY2;          /**< Key Register 2  */
  __IOM uint32_t KEY3;          /**< Key Register 3  */
  __IOM uint32_t TESTDATA;      /**< Test Data Register  */

  uint32_t       RESERVED1[3];  /**< Reserved for future use **/
  __IOM uint32_t STATUS;        /**< Status Register  */
  __IOM uint32_t INITWAITVAL;   /**< Initial Wait Counter  */
  uint32_t       RESERVED2[50]; /**< Reserved for future use **/
  __IM uint32_t  FIFO;          /**< FIFO Data  */
} TRNG_TypeDef;                 /** @} */

/**************************************************************************//**
 * @defgroup EFM32PG12B_TRNG_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for TRNG CONTROL */
#define _TRNG_CONTROL_RESETVALUE             0x00000000UL                             /**< Default value for TRNG_CONTROL */
#define _TRNG_CONTROL_MASK                   0x00003FFDUL                             /**< Mask for TRNG_CONTROL */
#define TRNG_CONTROL_ENABLE                  (0x1UL << 0)                             /**< TRNG Module Enable */
#define _TRNG_CONTROL_ENABLE_SHIFT           0                                        /**< Shift value for TRNG_ENABLE */
#define _TRNG_CONTROL_ENABLE_MASK            0x1UL                                    /**< Bit mask for TRNG_ENABLE */
#define _TRNG_CONTROL_ENABLE_DEFAULT         0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define _TRNG_CONTROL_ENABLE_DISABLED        0x00000000UL                             /**< Mode DISABLED for TRNG_CONTROL */
#define _TRNG_CONTROL_ENABLE_ENABLED         0x00000001UL                             /**< Mode ENABLED for TRNG_CONTROL */
#define TRNG_CONTROL_ENABLE_DEFAULT          (_TRNG_CONTROL_ENABLE_DEFAULT << 0)      /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_ENABLE_DISABLED         (_TRNG_CONTROL_ENABLE_DISABLED << 0)     /**< Shifted mode DISABLED for TRNG_CONTROL */
#define TRNG_CONTROL_ENABLE_ENABLED          (_TRNG_CONTROL_ENABLE_ENABLED << 0)      /**< Shifted mode ENABLED for TRNG_CONTROL */
#define TRNG_CONTROL_TESTEN                  (0x1UL << 2)                             /**< Test Enable */
#define _TRNG_CONTROL_TESTEN_SHIFT           2                                        /**< Shift value for TRNG_TESTEN */
#define _TRNG_CONTROL_TESTEN_MASK            0x4UL                                    /**< Bit mask for TRNG_TESTEN */
#define _TRNG_CONTROL_TESTEN_DEFAULT         0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define _TRNG_CONTROL_TESTEN_NOISE           0x00000000UL                             /**< Mode NOISE for TRNG_CONTROL */
#define _TRNG_CONTROL_TESTEN_TESTDATA        0x00000001UL                             /**< Mode TESTDATA for TRNG_CONTROL */
#define TRNG_CONTROL_TESTEN_DEFAULT          (_TRNG_CONTROL_TESTEN_DEFAULT << 2)      /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_TESTEN_NOISE            (_TRNG_CONTROL_TESTEN_NOISE << 2)        /**< Shifted mode NOISE for TRNG_CONTROL */
#define TRNG_CONTROL_TESTEN_TESTDATA         (_TRNG_CONTROL_TESTEN_TESTDATA << 2)     /**< Shifted mode TESTDATA for TRNG_CONTROL */
#define TRNG_CONTROL_CONDBYPASS              (0x1UL << 3)                             /**< Conditioning Bypass */
#define _TRNG_CONTROL_CONDBYPASS_SHIFT       3                                        /**< Shift value for TRNG_CONDBYPASS */
#define _TRNG_CONTROL_CONDBYPASS_MASK        0x8UL                                    /**< Bit mask for TRNG_CONDBYPASS */
#define _TRNG_CONTROL_CONDBYPASS_DEFAULT     0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define _TRNG_CONTROL_CONDBYPASS_NORMAL      0x00000000UL                             /**< Mode NORMAL for TRNG_CONTROL */
#define _TRNG_CONTROL_CONDBYPASS_BYPASS      0x00000001UL                             /**< Mode BYPASS for TRNG_CONTROL */
#define TRNG_CONTROL_CONDBYPASS_DEFAULT      (_TRNG_CONTROL_CONDBYPASS_DEFAULT << 3)  /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_CONDBYPASS_NORMAL       (_TRNG_CONTROL_CONDBYPASS_NORMAL << 3)   /**< Shifted mode NORMAL for TRNG_CONTROL */
#define TRNG_CONTROL_CONDBYPASS_BYPASS       (_TRNG_CONTROL_CONDBYPASS_BYPASS << 3)   /**< Shifted mode BYPASS for TRNG_CONTROL */
#define TRNG_CONTROL_REPCOUNTIEN             (0x1UL << 4)                             /**< Interrupt enable for Repetition Count Test failure */
#define _TRNG_CONTROL_REPCOUNTIEN_SHIFT      4                                        /**< Shift value for TRNG_REPCOUNTIEN */
#define _TRNG_CONTROL_REPCOUNTIEN_MASK       0x10UL                                   /**< Bit mask for TRNG_REPCOUNTIEN */
#define _TRNG_CONTROL_REPCOUNTIEN_DEFAULT    0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_REPCOUNTIEN_DEFAULT     (_TRNG_CONTROL_REPCOUNTIEN_DEFAULT << 4) /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_APT64IEN                (0x1UL << 5)                             /**< Interrupt enable for Adaptive Proportion Test failure (64-sample window) */
#define _TRNG_CONTROL_APT64IEN_SHIFT         5                                        /**< Shift value for TRNG_APT64IEN */
#define _TRNG_CONTROL_APT64IEN_MASK          0x20UL                                   /**< Bit mask for TRNG_APT64IEN */
#define _TRNG_CONTROL_APT64IEN_DEFAULT       0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_APT64IEN_DEFAULT        (_TRNG_CONTROL_APT64IEN_DEFAULT << 5)    /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_APT4096IEN              (0x1UL << 6)                             /**< Interrupt enable for Adaptive Proportion Test failure (4096-sample window) */
#define _TRNG_CONTROL_APT4096IEN_SHIFT       6                                        /**< Shift value for TRNG_APT4096IEN */
#define _TRNG_CONTROL_APT4096IEN_MASK        0x40UL                                   /**< Bit mask for TRNG_APT4096IEN */
#define _TRNG_CONTROL_APT4096IEN_DEFAULT     0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_APT4096IEN_DEFAULT      (_TRNG_CONTROL_APT4096IEN_DEFAULT << 6)  /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_FULLIEN                 (0x1UL << 7)                             /**< Interrupt enable for FIFO full */
#define _TRNG_CONTROL_FULLIEN_SHIFT          7                                        /**< Shift value for TRNG_FULLIEN */
#define _TRNG_CONTROL_FULLIEN_MASK           0x80UL                                   /**< Bit mask for TRNG_FULLIEN */
#define _TRNG_CONTROL_FULLIEN_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_FULLIEN_DEFAULT         (_TRNG_CONTROL_FULLIEN_DEFAULT << 7)     /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_SOFTRESET               (0x1UL << 8)                             /**< Software Reset */
#define _TRNG_CONTROL_SOFTRESET_SHIFT        8                                        /**< Shift value for TRNG_SOFTRESET */
#define _TRNG_CONTROL_SOFTRESET_MASK         0x100UL                                  /**< Bit mask for TRNG_SOFTRESET */
#define _TRNG_CONTROL_SOFTRESET_DEFAULT      0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define _TRNG_CONTROL_SOFTRESET_NORMAL       0x00000000UL                             /**< Mode NORMAL for TRNG_CONTROL */
#define _TRNG_CONTROL_SOFTRESET_RESET        0x00000001UL                             /**< Mode RESET for TRNG_CONTROL */
#define TRNG_CONTROL_SOFTRESET_DEFAULT       (_TRNG_CONTROL_SOFTRESET_DEFAULT << 8)   /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_SOFTRESET_NORMAL        (_TRNG_CONTROL_SOFTRESET_NORMAL << 8)    /**< Shifted mode NORMAL for TRNG_CONTROL */
#define TRNG_CONTROL_SOFTRESET_RESET         (_TRNG_CONTROL_SOFTRESET_RESET << 8)     /**< Shifted mode RESET for TRNG_CONTROL */
#define TRNG_CONTROL_PREIEN                  (0x1UL << 9)                             /**< Interrupt enable for AIS31 preliminary noise alarm */
#define _TRNG_CONTROL_PREIEN_SHIFT           9                                        /**< Shift value for TRNG_PREIEN */
#define _TRNG_CONTROL_PREIEN_MASK            0x200UL                                  /**< Bit mask for TRNG_PREIEN */
#define _TRNG_CONTROL_PREIEN_DEFAULT         0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_PREIEN_DEFAULT          (_TRNG_CONTROL_PREIEN_DEFAULT << 9)      /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_ALMIEN                  (0x1UL << 10)                            /**< Interrupt enable for AIS31 noise alarm */
#define _TRNG_CONTROL_ALMIEN_SHIFT           10                                       /**< Shift value for TRNG_ALMIEN */
#define _TRNG_CONTROL_ALMIEN_MASK            0x400UL                                  /**< Bit mask for TRNG_ALMIEN */
#define _TRNG_CONTROL_ALMIEN_DEFAULT         0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_ALMIEN_DEFAULT          (_TRNG_CONTROL_ALMIEN_DEFAULT << 10)     /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_FORCERUN                (0x1UL << 11)                            /**< Oscillator Force Run */
#define _TRNG_CONTROL_FORCERUN_SHIFT         11                                       /**< Shift value for TRNG_FORCERUN */
#define _TRNG_CONTROL_FORCERUN_MASK          0x800UL                                  /**< Bit mask for TRNG_FORCERUN */
#define _TRNG_CONTROL_FORCERUN_DEFAULT       0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define _TRNG_CONTROL_FORCERUN_NORMAL        0x00000000UL                             /**< Mode NORMAL for TRNG_CONTROL */
#define _TRNG_CONTROL_FORCERUN_RUN           0x00000001UL                             /**< Mode RUN for TRNG_CONTROL */
#define TRNG_CONTROL_FORCERUN_DEFAULT        (_TRNG_CONTROL_FORCERUN_DEFAULT << 11)   /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_FORCERUN_NORMAL         (_TRNG_CONTROL_FORCERUN_NORMAL << 11)    /**< Shifted mode NORMAL for TRNG_CONTROL */
#define TRNG_CONTROL_FORCERUN_RUN            (_TRNG_CONTROL_FORCERUN_RUN << 11)       /**< Shifted mode RUN for TRNG_CONTROL */
#define TRNG_CONTROL_BYPNIST                 (0x1UL << 12)                            /**< NIST Start-up Test Bypass. */
#define _TRNG_CONTROL_BYPNIST_SHIFT          12                                       /**< Shift value for TRNG_BYPNIST */
#define _TRNG_CONTROL_BYPNIST_MASK           0x1000UL                                 /**< Bit mask for TRNG_BYPNIST */
#define _TRNG_CONTROL_BYPNIST_DEFAULT        0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define _TRNG_CONTROL_BYPNIST_NORMAL         0x00000000UL                             /**< Mode NORMAL for TRNG_CONTROL */
#define _TRNG_CONTROL_BYPNIST_BYPASS         0x00000001UL                             /**< Mode BYPASS for TRNG_CONTROL */
#define TRNG_CONTROL_BYPNIST_DEFAULT         (_TRNG_CONTROL_BYPNIST_DEFAULT << 12)    /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_BYPNIST_NORMAL          (_TRNG_CONTROL_BYPNIST_NORMAL << 12)     /**< Shifted mode NORMAL for TRNG_CONTROL */
#define TRNG_CONTROL_BYPNIST_BYPASS          (_TRNG_CONTROL_BYPNIST_BYPASS << 12)     /**< Shifted mode BYPASS for TRNG_CONTROL */
#define TRNG_CONTROL_BYPAIS31                (0x1UL << 13)                            /**< AIS31 Start-up Test Bypass. */
#define _TRNG_CONTROL_BYPAIS31_SHIFT         13                                       /**< Shift value for TRNG_BYPAIS31 */
#define _TRNG_CONTROL_BYPAIS31_MASK          0x2000UL                                 /**< Bit mask for TRNG_BYPAIS31 */
#define _TRNG_CONTROL_BYPAIS31_DEFAULT       0x00000000UL                             /**< Mode DEFAULT for TRNG_CONTROL */
#define _TRNG_CONTROL_BYPAIS31_NORMAL        0x00000000UL                             /**< Mode NORMAL for TRNG_CONTROL */
#define _TRNG_CONTROL_BYPAIS31_BYPASS        0x00000001UL                             /**< Mode BYPASS for TRNG_CONTROL */
#define TRNG_CONTROL_BYPAIS31_DEFAULT        (_TRNG_CONTROL_BYPAIS31_DEFAULT << 13)   /**< Shifted mode DEFAULT for TRNG_CONTROL */
#define TRNG_CONTROL_BYPAIS31_NORMAL         (_TRNG_CONTROL_BYPAIS31_NORMAL << 13)    /**< Shifted mode NORMAL for TRNG_CONTROL */
#define TRNG_CONTROL_BYPAIS31_BYPASS         (_TRNG_CONTROL_BYPAIS31_BYPASS << 13)    /**< Shifted mode BYPASS for TRNG_CONTROL */

/* Bit fields for TRNG FIFOLEVEL */
#define _TRNG_FIFOLEVEL_RESETVALUE           0x00000000UL                         /**< Default value for TRNG_FIFOLEVEL */
#define _TRNG_FIFOLEVEL_MASK                 0xFFFFFFFFUL                         /**< Mask for TRNG_FIFOLEVEL */
#define _TRNG_FIFOLEVEL_VALUE_SHIFT          0                                    /**< Shift value for TRNG_VALUE */
#define _TRNG_FIFOLEVEL_VALUE_MASK           0xFFFFFFFFUL                         /**< Bit mask for TRNG_VALUE */
#define _TRNG_FIFOLEVEL_VALUE_DEFAULT        0x00000000UL                         /**< Mode DEFAULT for TRNG_FIFOLEVEL */
#define TRNG_FIFOLEVEL_VALUE_DEFAULT         (_TRNG_FIFOLEVEL_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_FIFOLEVEL */

/* Bit fields for TRNG FIFODEPTH */
#define _TRNG_FIFODEPTH_RESETVALUE           0x00000040UL                         /**< Default value for TRNG_FIFODEPTH */
#define _TRNG_FIFODEPTH_MASK                 0xFFFFFFFFUL                         /**< Mask for TRNG_FIFODEPTH */
#define _TRNG_FIFODEPTH_VALUE_SHIFT          0                                    /**< Shift value for TRNG_VALUE */
#define _TRNG_FIFODEPTH_VALUE_MASK           0xFFFFFFFFUL                         /**< Bit mask for TRNG_VALUE */
#define _TRNG_FIFODEPTH_VALUE_DEFAULT        0x00000040UL                         /**< Mode DEFAULT for TRNG_FIFODEPTH */
#define TRNG_FIFODEPTH_VALUE_DEFAULT         (_TRNG_FIFODEPTH_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_FIFODEPTH */

/* Bit fields for TRNG KEY0 */
#define _TRNG_KEY0_RESETVALUE                0x00000000UL                    /**< Default value for TRNG_KEY0 */
#define _TRNG_KEY0_MASK                      0xFFFFFFFFUL                    /**< Mask for TRNG_KEY0 */
#define _TRNG_KEY0_VALUE_SHIFT               0                               /**< Shift value for TRNG_VALUE */
#define _TRNG_KEY0_VALUE_MASK                0xFFFFFFFFUL                    /**< Bit mask for TRNG_VALUE */
#define _TRNG_KEY0_VALUE_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for TRNG_KEY0 */
#define TRNG_KEY0_VALUE_DEFAULT              (_TRNG_KEY0_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_KEY0 */

/* Bit fields for TRNG KEY1 */
#define _TRNG_KEY1_RESETVALUE                0x00000000UL                    /**< Default value for TRNG_KEY1 */
#define _TRNG_KEY1_MASK                      0xFFFFFFFFUL                    /**< Mask for TRNG_KEY1 */
#define _TRNG_KEY1_VALUE_SHIFT               0                               /**< Shift value for TRNG_VALUE */
#define _TRNG_KEY1_VALUE_MASK                0xFFFFFFFFUL                    /**< Bit mask for TRNG_VALUE */
#define _TRNG_KEY1_VALUE_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for TRNG_KEY1 */
#define TRNG_KEY1_VALUE_DEFAULT              (_TRNG_KEY1_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_KEY1 */

/* Bit fields for TRNG KEY2 */
#define _TRNG_KEY2_RESETVALUE                0x00000000UL                    /**< Default value for TRNG_KEY2 */
#define _TRNG_KEY2_MASK                      0xFFFFFFFFUL                    /**< Mask for TRNG_KEY2 */
#define _TRNG_KEY2_VALUE_SHIFT               0                               /**< Shift value for TRNG_VALUE */
#define _TRNG_KEY2_VALUE_MASK                0xFFFFFFFFUL                    /**< Bit mask for TRNG_VALUE */
#define _TRNG_KEY2_VALUE_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for TRNG_KEY2 */
#define TRNG_KEY2_VALUE_DEFAULT              (_TRNG_KEY2_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_KEY2 */

/* Bit fields for TRNG KEY3 */
#define _TRNG_KEY3_RESETVALUE                0x00000000UL                    /**< Default value for TRNG_KEY3 */
#define _TRNG_KEY3_MASK                      0xFFFFFFFFUL                    /**< Mask for TRNG_KEY3 */
#define _TRNG_KEY3_VALUE_SHIFT               0                               /**< Shift value for TRNG_VALUE */
#define _TRNG_KEY3_VALUE_MASK                0xFFFFFFFFUL                    /**< Bit mask for TRNG_VALUE */
#define _TRNG_KEY3_VALUE_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for TRNG_KEY3 */
#define TRNG_KEY3_VALUE_DEFAULT              (_TRNG_KEY3_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_KEY3 */

/* Bit fields for TRNG TESTDATA */
#define _TRNG_TESTDATA_RESETVALUE            0x00000000UL                        /**< Default value for TRNG_TESTDATA */
#define _TRNG_TESTDATA_MASK                  0xFFFFFFFFUL                        /**< Mask for TRNG_TESTDATA */
#define _TRNG_TESTDATA_VALUE_SHIFT           0                                   /**< Shift value for TRNG_VALUE */
#define _TRNG_TESTDATA_VALUE_MASK            0xFFFFFFFFUL                        /**< Bit mask for TRNG_VALUE */
#define _TRNG_TESTDATA_VALUE_DEFAULT         0x00000000UL                        /**< Mode DEFAULT for TRNG_TESTDATA */
#define TRNG_TESTDATA_VALUE_DEFAULT          (_TRNG_TESTDATA_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_TESTDATA */

/* Bit fields for TRNG STATUS */
#define _TRNG_STATUS_RESETVALUE              0x00000000UL                             /**< Default value for TRNG_STATUS */
#define _TRNG_STATUS_MASK                    0x000003F1UL                             /**< Mask for TRNG_STATUS */
#define TRNG_STATUS_TESTDATABUSY             (0x1UL << 0)                             /**< Test Data Busy */
#define _TRNG_STATUS_TESTDATABUSY_SHIFT      0                                        /**< Shift value for TRNG_TESTDATABUSY */
#define _TRNG_STATUS_TESTDATABUSY_MASK       0x1UL                                    /**< Bit mask for TRNG_TESTDATABUSY */
#define _TRNG_STATUS_TESTDATABUSY_DEFAULT    0x00000000UL                             /**< Mode DEFAULT for TRNG_STATUS */
#define _TRNG_STATUS_TESTDATABUSY_IDLE       0x00000000UL                             /**< Mode IDLE for TRNG_STATUS */
#define _TRNG_STATUS_TESTDATABUSY_BUSY       0x00000001UL                             /**< Mode BUSY for TRNG_STATUS */
#define TRNG_STATUS_TESTDATABUSY_DEFAULT     (_TRNG_STATUS_TESTDATABUSY_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_TESTDATABUSY_IDLE        (_TRNG_STATUS_TESTDATABUSY_IDLE << 0)    /**< Shifted mode IDLE for TRNG_STATUS */
#define TRNG_STATUS_TESTDATABUSY_BUSY        (_TRNG_STATUS_TESTDATABUSY_BUSY << 0)    /**< Shifted mode BUSY for TRNG_STATUS */
#define TRNG_STATUS_REPCOUNTIF               (0x1UL << 4)                             /**< Repetition Count Test interrupt status */
#define _TRNG_STATUS_REPCOUNTIF_SHIFT        4                                        /**< Shift value for TRNG_REPCOUNTIF */
#define _TRNG_STATUS_REPCOUNTIF_MASK         0x10UL                                   /**< Bit mask for TRNG_REPCOUNTIF */
#define _TRNG_STATUS_REPCOUNTIF_DEFAULT      0x00000000UL                             /**< Mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_REPCOUNTIF_DEFAULT       (_TRNG_STATUS_REPCOUNTIF_DEFAULT << 4)   /**< Shifted mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_APT64IF                  (0x1UL << 5)                             /**< Adaptive Proportion test failure (64-sample window) interrupt status */
#define _TRNG_STATUS_APT64IF_SHIFT           5                                        /**< Shift value for TRNG_APT64IF */
#define _TRNG_STATUS_APT64IF_MASK            0x20UL                                   /**< Bit mask for TRNG_APT64IF */
#define _TRNG_STATUS_APT64IF_DEFAULT         0x00000000UL                             /**< Mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_APT64IF_DEFAULT          (_TRNG_STATUS_APT64IF_DEFAULT << 5)      /**< Shifted mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_APT4096IF                (0x1UL << 6)                             /**< Adaptive Proportion test failure (4096-sample window) interrupt status */
#define _TRNG_STATUS_APT4096IF_SHIFT         6                                        /**< Shift value for TRNG_APT4096IF */
#define _TRNG_STATUS_APT4096IF_MASK          0x40UL                                   /**< Bit mask for TRNG_APT4096IF */
#define _TRNG_STATUS_APT4096IF_DEFAULT       0x00000000UL                             /**< Mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_APT4096IF_DEFAULT        (_TRNG_STATUS_APT4096IF_DEFAULT << 6)    /**< Shifted mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_FULLIF                   (0x1UL << 7)                             /**< FIFO full interrupt status */
#define _TRNG_STATUS_FULLIF_SHIFT            7                                        /**< Shift value for TRNG_FULLIF */
#define _TRNG_STATUS_FULLIF_MASK             0x80UL                                   /**< Bit mask for TRNG_FULLIF */
#define _TRNG_STATUS_FULLIF_DEFAULT          0x00000000UL                             /**< Mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_FULLIF_DEFAULT           (_TRNG_STATUS_FULLIF_DEFAULT << 7)       /**< Shifted mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_PREIF                    (0x1UL << 8)                             /**< AIS31 Preliminary Noise Alarm interrupt status */
#define _TRNG_STATUS_PREIF_SHIFT             8                                        /**< Shift value for TRNG_PREIF */
#define _TRNG_STATUS_PREIF_MASK              0x100UL                                  /**< Bit mask for TRNG_PREIF */
#define _TRNG_STATUS_PREIF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_PREIF_DEFAULT            (_TRNG_STATUS_PREIF_DEFAULT << 8)        /**< Shifted mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_ALMIF                    (0x1UL << 9)                             /**< AIS31 Noise Alarm interrupt status */
#define _TRNG_STATUS_ALMIF_SHIFT             9                                        /**< Shift value for TRNG_ALMIF */
#define _TRNG_STATUS_ALMIF_MASK              0x200UL                                  /**< Bit mask for TRNG_ALMIF */
#define _TRNG_STATUS_ALMIF_DEFAULT           0x00000000UL                             /**< Mode DEFAULT for TRNG_STATUS */
#define TRNG_STATUS_ALMIF_DEFAULT            (_TRNG_STATUS_ALMIF_DEFAULT << 9)        /**< Shifted mode DEFAULT for TRNG_STATUS */

/* Bit fields for TRNG INITWAITVAL */
#define _TRNG_INITWAITVAL_RESETVALUE         0x000000FFUL                           /**< Default value for TRNG_INITWAITVAL */
#define _TRNG_INITWAITVAL_MASK               0x000000FFUL                           /**< Mask for TRNG_INITWAITVAL */
#define _TRNG_INITWAITVAL_VALUE_SHIFT        0                                      /**< Shift value for TRNG_VALUE */
#define _TRNG_INITWAITVAL_VALUE_MASK         0xFFUL                                 /**< Bit mask for TRNG_VALUE */
#define _TRNG_INITWAITVAL_VALUE_DEFAULT      0x000000FFUL                           /**< Mode DEFAULT for TRNG_INITWAITVAL */
#define TRNG_INITWAITVAL_VALUE_DEFAULT       (_TRNG_INITWAITVAL_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_INITWAITVAL */

/* Bit fields for TRNG FIFO */
#define _TRNG_FIFO_RESETVALUE                0x00000000UL                    /**< Default value for TRNG_FIFO */
#define _TRNG_FIFO_MASK                      0xFFFFFFFFUL                    /**< Mask for TRNG_FIFO */
#define _TRNG_FIFO_VALUE_SHIFT               0                               /**< Shift value for TRNG_VALUE */
#define _TRNG_FIFO_VALUE_MASK                0xFFFFFFFFUL                    /**< Bit mask for TRNG_VALUE */
#define _TRNG_FIFO_VALUE_DEFAULT             0x00000000UL                    /**< Mode DEFAULT for TRNG_FIFO */
#define TRNG_FIFO_VALUE_DEFAULT              (_TRNG_FIFO_VALUE_DEFAULT << 0) /**< Shifted mode DEFAULT for TRNG_FIFO */

/** @} End of group EFM32PG12B_TRNG */
/** @} End of group Parts */

