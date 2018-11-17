/***************************************************************************//**
 * @file em_rtcc.h
 * @brief Real Time Counter (RTCC) peripheral API.
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#ifndef EM_RTCC_H
#define EM_RTCC_H

#include "em_device.h"
#if defined(RTCC_COUNT) && (RTCC_COUNT == 1)

#include <stdbool.h>
#include "em_assert.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup RTCC
 * @{
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_84) \
  || defined(_SILICON_LABS_GECKO_INTERNAL_SDID_89)
/* Enable fix for errata "RTCC_E203 - Potential Stability Issue with RTCC
 * Registers". */
#define ERRATA_FIX_RTCC_E203
#endif

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_84)
/* Enable fix for errata "RTCC_E204 - Disabling the RTCC Backup RAM may consume extra
 * current". */
#define ERRATA_FIX_RTCC_E204
#endif
/** @endcond */

/*******************************************************************************
 *********************************   ENUM   ************************************
 ******************************************************************************/

#if defined (_RTCC_CTRL_CNTMODE_MASK)
/** Operational mode of the counter. */
typedef enum {
  /** Normal counter mode. The counter is incremented by 1 for each tick. */
  rtccCntModeNormal = _RTCC_CTRL_CNTMODE_NORMAL,

  /** Calendar mode. Refer to the RTCC chapter of the Reference Manual for more
   *  details on the calendar mode. */
  rtccCntModeCalendar = _RTCC_CTRL_CNTMODE_CALENDAR
} RTCC_CntMode_TypeDef;
#endif

/** Counter prescaler selection. */
#if defined (_RTCC_CTRL_CNTPRESC_DIV1)
typedef enum {
  rtccCntPresc_1     = _RTCC_CTRL_CNTPRESC_DIV1,      /**< Divide clock by 1.     */
  rtccCntPresc_2     = _RTCC_CTRL_CNTPRESC_DIV2,      /**< Divide clock by 2.     */
  rtccCntPresc_4     = _RTCC_CTRL_CNTPRESC_DIV4,      /**< Divide clock by 4.     */
  rtccCntPresc_8     = _RTCC_CTRL_CNTPRESC_DIV8,      /**< Divide clock by 8.     */
  rtccCntPresc_16    = _RTCC_CTRL_CNTPRESC_DIV16,     /**< Divide clock by 16.    */
  rtccCntPresc_32    = _RTCC_CTRL_CNTPRESC_DIV32,     /**< Divide clock by 32.    */
  rtccCntPresc_64    = _RTCC_CTRL_CNTPRESC_DIV64,     /**< Divide clock by 64.    */
  rtccCntPresc_128   = _RTCC_CTRL_CNTPRESC_DIV128,    /**< Divide clock by 128.   */
  rtccCntPresc_256   = _RTCC_CTRL_CNTPRESC_DIV256,    /**< Divide clock by 256.   */
  rtccCntPresc_512   = _RTCC_CTRL_CNTPRESC_DIV512,    /**< Divide clock by 512.   */
  rtccCntPresc_1024  = _RTCC_CTRL_CNTPRESC_DIV1024,   /**< Divide clock by 1024.  */
  rtccCntPresc_2048  = _RTCC_CTRL_CNTPRESC_DIV2048,   /**< Divide clock by 2048.  */
  rtccCntPresc_4096  = _RTCC_CTRL_CNTPRESC_DIV4096,   /**< Divide clock by 4096.  */
  rtccCntPresc_8192  = _RTCC_CTRL_CNTPRESC_DIV8192,   /**< Divide clock by 8192.  */
  rtccCntPresc_16384 = _RTCC_CTRL_CNTPRESC_DIV16384,  /**< Divide clock by 16384. */
  rtccCntPresc_32768 = _RTCC_CTRL_CNTPRESC_DIV32768   /**< Divide clock by 32768. */
} RTCC_CntPresc_TypeDef;

#elif defined (_RTCC_CFG_CNTPRESC_DIV1)
typedef enum {
  rtccCntPresc_1     = _RTCC_CFG_CNTPRESC_DIV1,      /**< Divide clock by 1. */
  rtccCntPresc_2     = _RTCC_CFG_CNTPRESC_DIV2,      /**< Divide clock by 2. */
  rtccCntPresc_4     = _RTCC_CFG_CNTPRESC_DIV4,      /**< Divide clock by 4. */
  rtccCntPresc_8     = _RTCC_CFG_CNTPRESC_DIV8,      /**< Divide clock by 8. */
  rtccCntPresc_16    = _RTCC_CFG_CNTPRESC_DIV16,     /**< Divide clock by 16. */
  rtccCntPresc_32    = _RTCC_CFG_CNTPRESC_DIV32,     /**< Divide clock by 32. */
  rtccCntPresc_64    = _RTCC_CFG_CNTPRESC_DIV64,     /**< Divide clock by 64. */
  rtccCntPresc_128   = _RTCC_CFG_CNTPRESC_DIV128,    /**< Divide clock by 128. */
  rtccCntPresc_256   = _RTCC_CFG_CNTPRESC_DIV256,    /**< Divide clock by 256. */
  rtccCntPresc_512   = _RTCC_CFG_CNTPRESC_DIV512,    /**< Divide clock by 512. */
  rtccCntPresc_1024  = _RTCC_CFG_CNTPRESC_DIV1024,   /**< Divide clock by 1024. */
  rtccCntPresc_2048  = _RTCC_CFG_CNTPRESC_DIV2048,   /**< Divide clock by 2048. */
  rtccCntPresc_4096  = _RTCC_CFG_CNTPRESC_DIV4096,   /**< Divide clock by 4096. */
  rtccCntPresc_8192  = _RTCC_CFG_CNTPRESC_DIV8192,   /**< Divide clock by 8192. */
  rtccCntPresc_16384 = _RTCC_CFG_CNTPRESC_DIV16384,  /**< Divide clock by 16384. */
  rtccCntPresc_32768 = _RTCC_CFG_CNTPRESC_DIV32768   /**< Divide clock by 32768. */
} RTCC_CntPresc_TypeDef;
#endif

/** Prescaler mode of the RTCC counter. */
#if defined (_RTCC_CTRL_CNTTICK_MASK)
typedef enum {
  /** CNT register ticks according to prescaler value. */
  rtccCntTickPresc = _RTCC_CTRL_CNTTICK_PRESC,

  /** CNT register ticks when PRECNT matches the 15 least significant bits of
   *  ch. 0 CCV register. */
  rtccCntTickCCV0Match = _RTCC_CTRL_CNTTICK_CCV0MATCH
} RTCC_PrescMode_TypeDef;

#elif defined (_RTCC_CFG_CNTTICK_MASK)
typedef enum {
  /** CNT register ticks according to the prescaler value. */
  rtccCntTickPresc = _RTCC_CFG_CNTTICK_PRESC,

  /** CNT register ticks when PRECNT matches the 15 least significant bits of
   *  ch. 0 CCV register. */
  rtccCntTickCCV0Match = _RTCC_CFG_CNTTICK_CCV0MATCH
} RTCC_PrescMode_TypeDef;
#endif

/** Capture/Compare channel mode. */
typedef enum {
  rtccCapComChModeOff     = _RTCC_CC_CTRL_MODE_OFF,           /**< Capture/Compare channel turned off. */
  rtccCapComChModeCapture = _RTCC_CC_CTRL_MODE_INPUTCAPTURE,  /**< Capture mode. */
  rtccCapComChModeCompare = _RTCC_CC_CTRL_MODE_OUTPUTCOMPARE, /**< Compare mode. */
} RTCC_CapComChMode_TypeDef;

/** Compare match output action mode. */
typedef enum {
  rtccCompMatchOutActionPulse  = _RTCC_CC_CTRL_CMOA_PULSE,  /**< Generate a pulse. */
  rtccCompMatchOutActionToggle = _RTCC_CC_CTRL_CMOA_TOGGLE, /**< Toggle output. */
  rtccCompMatchOutActionClear  = _RTCC_CC_CTRL_CMOA_CLEAR,  /**< Clear output. */
  rtccCompMatchOutActionSet    = _RTCC_CC_CTRL_CMOA_SET     /**< Set output. */
} RTCC_CompMatchOutAction_TypeDef;

/** PRS channel number. This type is used when configuring input capture mode on
 *  a RTCC channel. */
typedef uint8_t RTCC_PRSSel_TypeDef;

/** Deprecated PRS channel values. New code should use an integer instead of
 *  using these deprecated enum values. */
#define rtccPRSCh0    0U
#define rtccPRSCh1    1U
#define rtccPRSCh2    2U
#define rtccPRSCh3    3U
#define rtccPRSCh4    4U
#define rtccPRSCh5    5U
#define rtccPRSCh6    6U
#define rtccPRSCh7    7U
#define rtccPRSCh8    8U
#define rtccPRSCh9    9U
#define rtccPRSCh10  10U
#define rtccPRSCh11  11U

/** Input edge select. */
typedef enum {
  rtccInEdgeRising  = _RTCC_CC_CTRL_ICEDGE_RISING,  /**< Rising edges detected. */
  rtccInEdgeFalling = _RTCC_CC_CTRL_ICEDGE_FALLING, /**< Falling edges detected. */
  rtccInEdgeBoth    = _RTCC_CC_CTRL_ICEDGE_BOTH,    /**< Both edges detected. */
  rtccInEdgeNone    = _RTCC_CC_CTRL_ICEDGE_NONE     /**< No edge detection, signal is left as is. */
} RTCC_InEdgeSel_TypeDef;

/** Capture/Compare channel compare mode. */
typedef enum {
  /** CCVx is compared with the CNT register. */
  rtccCompBaseCnt = _RTCC_CC_CTRL_COMPBASE_CNT,

  /** CCVx is compared with a CNT[16:0] and PRECNT[14:0]. */
  rtccCompBasePreCnt = _RTCC_CC_CTRL_COMPBASE_PRECNT
} RTCC_CompBase_TypeDef;

#if defined (_RTCC_CC_CTRL_DAYCC_MASK)
/** Day compare mode. */
typedef enum {
  rtccDayCompareModeMonth = _RTCC_CC_CTRL_DAYCC_MONTH,  /**< Day of month is selected for Capture/Compare. */
  rtccDayCompareModeWeek  = _RTCC_CC_CTRL_DAYCC_WEEK    /**< Day of week is selected for Capture/Compare. */
} RTCC_DayCompareMode_TypeDef;
#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** RTCC initialization structure. */
typedef struct {
  /** Enable/disable counting when initialization is completed. */
  bool                   enable;

  /** Enable/disable timer counting during debug halt. */
  bool                   debugRun;

  /** Enable/disable pre-counter wrap on ch. 0 CCV value. */
  bool                   precntWrapOnCCV0;

  /** Enable/disable counter wrap on ch. 1 CCV value. */
  bool                   cntWrapOnCCV1;

  /** Counter prescaler. */
  RTCC_CntPresc_TypeDef  presc;

  /** Prescaler mode. */
  RTCC_PrescMode_TypeDef prescMode;

#if defined(_RTCC_CTRL_BUMODETSEN_MASK)
  /** Enable/disable storing RTCC counter value in RTCC_CCV2 upon backup mode
   *  entry. */
  bool                   enaBackupModeSet;
#endif

#if defined(_RTCC_CTRL_OSCFDETEN_MASK)
  /** Enable/disable the check that sets OSCFFAIL interrupt flag if no
   *  LFCLK-RTCC ticks are detected within one ULFRCO cycles. */
  bool                   enaOSCFailDetect;
#endif

#if defined (_RTCC_CTRL_CNTMODE_MASK)
  /** Select operational mode of the counter. */
  RTCC_CntMode_TypeDef   cntMode;
#endif

#if defined (_RTCC_CTRL_LYEARCORRDIS_MASK)
  /** Disable leap year correction for calendar mode. When this parameter is
   *  set to false, February has 29 days if (year % 4 == 0). If set to true,
   *  February always has 28 days. */
  bool                   disLeapYearCorr;
#endif
} RTCC_Init_TypeDef;

/** RTCC capture/compare channel configuration structure. */
typedef struct {
  /** Select mode of Capture/Compare channel. */
  RTCC_CapComChMode_TypeDef        chMode;

  /** Compare mode channel match output action. */
  RTCC_CompMatchOutAction_TypeDef  compMatchOutAction;

  /** Capture mode channel PRS input channel selection. */
  RTCC_PRSSel_TypeDef              prsSel;

  /** Capture mode channel input edge selection. */
  RTCC_InEdgeSel_TypeDef           inputEdgeSel;

  /** Comparison base of channel in compare mode. */
  RTCC_CompBase_TypeDef            compBase;

#if defined (_RTCC_CC_CTRL_COMPMASK_MASK)
  /** The COMPMASK (5 bit) most significant bits of compare value will not
   *  be subject to comparison.  */
  uint8_t                          compMask;
#endif

#if defined (_RTCC_CC_CTRL_DAYCC_MASK)
  /** Day compare mode. */
  RTCC_DayCompareMode_TypeDef      dayCompMode;
#endif
} RTCC_CCChConf_TypeDef;

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** Default RTCC initialization structure. */
#if defined(_SILICON_LABS_32B_SERIES_2)
#define RTCC_INIT_DEFAULT                                                   \
  {                                                                         \
    true,   /* Start counting when init done.                            */ \
    false,  /* Disable RTCC during debug halt.                           */ \
    false,  /* Disable precounter wrap on ch. 0 CCV value.               */ \
    false,  /* Disable counter wrap on ch. 1 CCV value.                  */ \
    rtccCntPresc_32, /* 977 us per tick.                                 */ \
    rtccCntTickPresc, /* Counter increments according to prescaler value.*/ \
  }

#elif defined(_RTCC_CTRL_BUMODETSEN_MASK)
#define RTCC_INIT_DEFAULT                                                   \
  {                                                                         \
    true,   /* Start counting when initialization is done.               */ \
    false,  /* Disable RTCC during debug halt.                           */ \
    false,  /* Disable pre-counter wrap on ch. 0 CCV value.              */ \
    false,  /* Disable counter wrap on ch. 1 CCV value.                  */ \
    rtccCntPresc_32, /* 977 us per tick.                                 */ \
    rtccCntTickPresc, /* Counter increments according to prescaler value.*/ \
    false,  /* No RTCC storage on backup mode entry.                     */ \
    false,  /* No RTCC oscillator failure detection.                     */ \
    rtccCntModeNormal, /* Normal RTCC mode.                              */ \
    false,  /* No leap year correction.                                  */ \
  }

#else
#define RTCC_INIT_DEFAULT                                                   \
  {                                                                         \
    true,   /* Start counting when initialization is done.               */ \
    false,  /* Disable RTCC during debug halt.                           */ \
    false,  /* Disable pre-counter wrap on ch. 0 CCV value.              */ \
    false,  /* Disable counter wrap on ch. 1 CCV value.                  */ \
    rtccCntPresc_32, /* 977 us per tick.                                 */ \
    rtccCntTickPresc, /* Counter increments according to prescaler value.*/ \
    false,  /* No RTCC oscillator failure detection.                     */ \
    rtccCntModeNormal, /* Normal RTCC mode.                              */ \
    false,  /* No leap year correction.                                  */ \
  }
#endif

#if defined(_SILICON_LABS_32B_SERIES_2)

/** Default RTCC channel output compare initialization structure. */
#define RTCC_CH_INIT_COMPARE_DEFAULT                                  \
  {                                                                   \
    rtccCapComChModeCompare,     /* Select output compare mode.    */ \
    rtccCompMatchOutActionPulse, /* Create pulse on compare match. */ \
    0,                           /* Don't care.                    */ \
    rtccInEdgeNone,              /* Don't care.                    */ \
    rtccCompBaseCnt,             /* Use CNT for comparison.        */ \
  }

/** Default RTCC channel input capture initialization structure. */
#define RTCC_CH_INIT_CAPTURE_DEFAULT                                 \
  {                                                                  \
    rtccCapComChModeCapture,     /* Select input capture mode.    */ \
    rtccCompMatchOutActionPulse, /* Don't care.                   */ \
    0,                           /* Use PRS channel 0 as trigger. */ \
    rtccInEdgeRising,            /* Capture on rising edge.       */ \
    rtccCompBaseCnt,             /* Don't care.                   */ \
  }

#else // Series 1 devices

/** Default RTCC channel output compare initialization structure. */
#define RTCC_CH_INIT_COMPARE_DEFAULT                                 \
  {                                                                  \
    rtccCapComChModeCompare,   /* Select output compare mode.     */ \
    rtccCompMatchOutActionPulse, /* Create pulse on compare match.*/ \
    0,                         /* PRS channel 0 (not used).       */ \
    rtccInEdgeNone,            /* No edge detection.              */ \
    rtccCompBaseCnt,           /* Counter comparison base.        */ \
    0,                         /* No compare mask bits set.       */ \
    rtccDayCompareModeMonth    /* Don't care */                      \
  }

/** Default RTCC channel input capture initialization structure. */
#define RTCC_CH_INIT_CAPTURE_DEFAULT                                 \
  {                                                                  \
    rtccCapComChModeCapture,   /* Select input capture mode.      */ \
    rtccCompMatchOutActionPulse, /* Create pulse on capture.      */ \
    0,                         /* PRS channel 0.                  */ \
    rtccInEdgeRising,          /* Rising edge detection.          */ \
    rtccCompBaseCnt,           /* Don't care.                     */ \
    0,                         /* Don't care.                     */ \
    rtccDayCompareModeMonth    /* Don't care                      */ \
  }

#endif

/** Number of RTCC capture/compare channels */
#if !defined(RTCC_CC_NUM)
#define RTCC_CC_NUM   3UL
#endif

/** Validation of valid RTCC channel for assert statements. */
#define RTCC_CH_VALID(ch)    ((unsigned)(ch) < RTCC_CC_NUM)

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Get RTCC compare register value for selected channel.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @return
 *   Compare register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_ChannelCompareValueGet(int ch)
{
  EFM_ASSERT(RTCC_CH_VALID(ch) );
#if defined (_RTCC_CC_CCV_MASK)
  return RTCC->CC[ch].CCV;
#elif defined (_RTCC_CC_OCVALUE_MASK)
  return RTCC->CC[ch].OCVALUE;
#endif
}

/***************************************************************************//**
 * @brief
 *   Set RTCC compare register value for selected channel.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @param[in] value
 *   Compare register value
 ******************************************************************************/
__STATIC_INLINE void RTCC_ChannelCompareValueSet(int ch, uint32_t value)
{
  EFM_ASSERT(RTCC_CH_VALID(ch) );
#if defined (_RTCC_CC_CCV_MASK)
  RTCC->CC[ch].CCV = value;
#elif defined (_RTCC_CC_OCVALUE_MASK)
  RTCC->CC[ch].OCVALUE = value;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get RTCC input capture register value for selected channel.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @return
 *   Capture register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_ChannelCaptureValueGet(int ch)
{
  EFM_ASSERT(RTCC_CH_VALID(ch) );
#if defined (_RTCC_CC_CCV_MASK)
  return RTCC->CC[ch].CCV;
#elif defined (_RTCC_CC_ICVALUE_MASK)
  return RTCC->CC[ch].ICVALUE;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get RTCC capture/compare register value for selected channel.
 *   For parts with separate capture compare value registers, this function
 *   returns the compare value.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @return
 *   Capture/compare register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_ChannelCCVGet(int ch)
{
  return RTCC_ChannelCompareValueGet(ch);
}

/***************************************************************************//**
 * @brief
 *   Set RTCC capture/compare register value for selected channel.
 *   For parts with separate capture compare value registers, this function
 *   sets the compare value.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @param[in] value
 *   Capture/compare register value
 ******************************************************************************/
__STATIC_INLINE void RTCC_ChannelCCVSet(int ch, uint32_t value)
{
  RTCC_ChannelCompareValueSet(ch, value);
}

#if defined (_RTCC_CC_DATE_MASK)
/***************************************************************************//**
 * @brief
 *   Get the calendar DATE register content for selected channel.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @return
 *   DATE register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_ChannelDateGet(int ch)
{
  EFM_ASSERT(RTCC_CH_VALID(ch) );
  return RTCC->CC[ch].DATE;
}

/***************************************************************************//**
 * @brief
 *   Set calendar DATE register for selected channel.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @param[in] date
 *   DATE value.
 ******************************************************************************/
__STATIC_INLINE void RTCC_ChannelDateSet(int ch, uint32_t date)
{
  EFM_ASSERT(RTCC_CH_VALID(ch) );
  RTCC->CC[ch].DATE = date;
}

/***************************************************************************//**
 * @brief
 *   Get calendar TIME register content for selected channel.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @return
 *   TIME register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_ChannelTimeGet(int ch)
{
  EFM_ASSERT(RTCC_CH_VALID(ch) );
  return RTCC->CC[ch].TIME;
}

/***************************************************************************//**
 * @brief
 *   Set calendar TIME register for selected channel.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @param[in] time
 *   TIME value.
 ******************************************************************************/
__STATIC_INLINE void RTCC_ChannelTimeSet(int ch, uint32_t time)
{
  EFM_ASSERT(RTCC_CH_VALID(ch) );
  RTCC->CC[ch].TIME = time;
}
#endif /* defined (_RTCC_CC_DATE_MASK) */

/***************************************************************************//**
 * @brief
 *   Get combined CNT/PRECNT register content.
 *
 * @return
 *   CNT/PRECNT register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_CombinedCounterGet(void)
{
#if defined (RTCC_SYNCBUSY_CNT) || defined (RTCC_SYNCBUSY_CNT)
  while ((RTCC->SYNCBUSY & (RTCC_SYNCBUSY_CNT | RTCC_SYNCBUSY_PRECNT)) != 0U) {
    /* Wait for CNT and PRECNT to synchronize */
  }
#endif
  return RTCC->COMBCNT;
}

/***************************************************************************//**
 * @brief
 *   Get RTCC counter value.
 *
 * @return
 *   Current RTCC counter value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_CounterGet(void)
{
#if defined (RTCC_SYNCBUSY_CNT)
  while ((RTCC->SYNCBUSY & RTCC_SYNCBUSY_CNT) != 0U) {
    /* Wait for CNT to synchronize before getting value */
  }
#endif
  return RTCC->CNT;
}

/***************************************************************************//**
 * @brief
 *   Set RTCC CNT counter.
 *
 * @param[in] value
 *   CNT value.
 ******************************************************************************/
__STATIC_INLINE void RTCC_CounterSet(uint32_t value)
{
#if defined (RTCC_SYNCBUSY_CNT)
  while ((RTCC->SYNCBUSY & RTCC_SYNCBUSY_CNT) != 0U) {
    /* Wait for CNT to synchronize before setting new value */
  }
#endif

  RTCC->CNT = value;
}

#if defined (_RTCC_CC_DATE_MASK)
/***************************************************************************//**
 * @brief
 *   Get DATE register value.
 *
 * @return
 *   Current DATE register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_DateGet(void)
{
  return RTCC->DATE;
}

/***************************************************************************//**
 * @brief
 *   Set RTCC DATE register.
 *
 * @param[in] date
 *   DATE value.
 ******************************************************************************/
__STATIC_INLINE void RTCC_DateSet(uint32_t date)
{
  RTCC->DATE = date;
}
#endif /* defined (_RTCC_CC_DATE_MASK) */

#if defined (RTCC_EM4WUEN_EM4WU)
/***************************************************************************//**
 * @brief
 *   Enable/disable EM4 wakeup capability.
 *
 * @param[in] enable
 *   Set to true to enable EM4 wakeup, set to false otherwise.
 ******************************************************************************/
__STATIC_INLINE void RTCC_EM4WakeupEnable(bool enable)
{
  if ( enable ) {
    RTCC->EM4WUEN = RTCC_EM4WUEN_EM4WU;
  } else {
    RTCC->EM4WUEN = 0;
  }
}
#endif

void RTCC_Enable(bool enable);

void RTCC_Init(const RTCC_Init_TypeDef *init);

void RTCC_ChannelInit(int ch, RTCC_CCChConf_TypeDef const *confPtr);

/***************************************************************************//**
 * @brief
 *   Clear one or more pending RTCC interrupts.
 *
 * @param[in] flags
 *   RTCC interrupt sources to clear. Use a set of interrupt flags OR-ed
 *   together to clear multiple interrupt sources.
 ******************************************************************************/
__STATIC_INLINE void RTCC_IntClear(uint32_t flags)
{
#if defined (RTCC_HAS_SET_CLEAR)
  RTCC->IF_CLR = flags;
#else
  RTCC->IFC = flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Disable one or more RTCC interrupts.
 *
 * @param[in] flags
 *   RTCC interrupt sources to disable. Use a set of interrupt flags OR-ed
 *   together to disable multiple interrupt.
 ******************************************************************************/
__STATIC_INLINE void RTCC_IntDisable(uint32_t flags)
{
  RTCC->IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable one or more RTCC interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. To ignore a pending interrupt, consider using
 *   RTCC_IntClear() prior to enabling the interrupt.
 *
 * @param[in] flags
 *   RTCC interrupt sources to enable. Use a set of interrupt flags OR-ed
 *   together to set multiple interrupt.
 ******************************************************************************/
__STATIC_INLINE void RTCC_IntEnable(uint32_t flags)
{
  RTCC->IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending RTCC interrupt flags.
 *
 * @note
 *   Event bits are not cleared by using this function.
 *
 * @return
 *   Pending RTCC interrupt sources. Returns a set of interrupt flags OR-ed
 *   together for the interrupt sources set.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_IntGet(void)
{
  return RTCC->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending RTCC interrupt flags.
 *
 * @details
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @return
 *   Pending and enabled RTCC interrupt sources. Returns a set of interrupt
 *   flags OR-ed together for the interrupt sources set.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_IntGetEnabled(void)
{
  uint32_t tmp;

  tmp = RTCC->IEN;

  /* Bitwise AND of pending and enabled interrupt flags. */
  return RTCC->IF & tmp;
}

/***************************************************************************//**
 * @brief
 *   Set one or more pending RTCC interrupts from SW.
 *
 * @param[in] flags
 *   RTCC interrupt sources to set to pending. Use a set of interrupt flags
 *   (RTCC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void RTCC_IntSet(uint32_t flags)
{
#if defined (RTCC_HAS_SET_CLEAR)
  RTCC->IF_SET = flags;
#else
  RTCC->IFS = flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Lock RTCC registers.
 *
 * @note
 *   When RTCC registers are locked, RTCC_CTRL, RTCC_PRECNT, RTCC_CNT,
 *   RTCC_TIME, RTCC_DATE, RTCC_IEN, RTCC_POWERDOWN and RTCC_CCx_XXX registers
 *   cannot be written to.
 ******************************************************************************/
__STATIC_INLINE void RTCC_Lock(void)
{
#if defined(ERRATA_FIX_RTCC_E203)
  /* RTCC_E203 - Potential Stability Issue with RTCC Registers.
   * RTCC_LOCK register must be modified while RTCC clock is disabled. */
  uint32_t lfeReg = CMU->LFECLKEN0;
  bool cmuLocked = (CMU->LOCK == CMU_LOCK_LOCKKEY_LOCKED);
  if (cmuLocked) {
    CMU->LOCK = CMU_LOCK_LOCKKEY_UNLOCK;
  }
  CMU->LFECLKEN0 = 0x0;
#endif
  RTCC->LOCK = ~RTCC_LOCK_LOCKKEY_UNLOCK;
#if defined(ERRATA_FIX_RTCC_E203)
  /* Restore clock state after RTCC_E203 fix. */
  CMU->LFECLKEN0 = lfeReg;
  if (cmuLocked) {
    CMU->LOCK = CMU_LOCK_LOCKKEY_LOCK;
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Get RTCC pre-counter value.
 *
 * @return
 *   Current RTCC pre-counter value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_PreCounterGet(void)
{
#if defined (RTCC_SYNCBUSY_PRECNT)
  while ((RTCC->SYNCBUSY & RTCC_SYNCBUSY_PRECNT) != 0U) {
    /* Wait for PRECNT to synchronize */
  }
#endif
  return RTCC->PRECNT;
}

/***************************************************************************//**
 * @brief
 *   Set RTCC pre-counter value.
 *
 * @param[in] preCntVal
 *   RTCC pre-counter value to be set.
 ******************************************************************************/
__STATIC_INLINE void RTCC_PreCounterSet(uint32_t preCntVal)
{
#if defined (RTCC_SYNCBUSY_PRECNT)
  while ((RTCC->SYNCBUSY & RTCC_SYNCBUSY_PRECNT) != 0U) {
    /* Wait for PRECNT to synchronize */
  }
#endif

  RTCC->PRECNT = preCntVal;
}

void RTCC_Reset(void);

#if defined (_RTCC_POWERDOWN_MASK)
/***************************************************************************//**
 * @brief
 *   Power down retention RAM.
 *
 * @note
 *   Once retention RAM is powered down, it cannot be powered up again.
 ******************************************************************************/
__STATIC_INLINE void RTCC_RetentionRamPowerDown(void)
{
#if !defined(ERRATA_FIX_RTCC_E204)
  /* Devices that are affected by RTCC_E204 should always keep RTCC
   * backup RAM retained. */
  RTCC->POWERDOWN = RTCC_POWERDOWN_RAM;
#endif
}
#endif

void RTCC_StatusClear(void);

/***************************************************************************//**
 * @brief
 *   Get STATUS register value.
 *
 * @return
 *   Current STATUS register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_StatusGet(void)
{
#if defined (RTCC_SYNCBUSY_CMD)
  while ((RTCC->SYNCBUSY & RTCC_SYNCBUSY_CMD) != 0U) {
    // Wait for synchronization.
  }
#elif defined (RTCC_SYNCBUSY_START)
  while ((RTCC->SYNCBUSY & (RTCC_SYNCBUSY_START | RTCC_SYNCBUSY_STOP)) != 0U) {
    /* Wait for synchronization. */
  }
#endif
  return RTCC->STATUS;
}

#if defined(_SILICON_LABS_32B_SERIES_2)
/***************************************************************************//**
 * @brief
 *   Wait for the RTCC to complete all synchronization of register changes
 *   and commands.
 ******************************************************************************/
__STATIC_INLINE void RTCC_SyncWait(void)
{
  while ((RTCC->EN != 0U) && (RTCC->SYNCBUSY != 0U)) {
    /* Wait for synchronization to finish */
  }
}

/***************************************************************************//**
 * @brief
 *   Start RTCC counter.
 *
 * @detail
 *   This function will send a start command to the RTCC peripheral. The RTCC
 *   peripheral will use some LF clock ticks before the command is executed.
 *   The @ref RTCC_SyncWait() function can be used to wait for the start command
 *   to be executed.
 *
 * @note
 *   This function requires the RTCC to be enabled.
 ******************************************************************************/
__STATIC_INLINE void RTCC_Start(void)
{
  RTCC_SyncWait();
  RTCC->CMD = RTCC_CMD_START;
}

/***************************************************************************//**
 * @brief
 *   Stop the RTCC counter.
 *
 * @detail
 *   This function will send a stop command to the RTCC peripheral. The RTCC
 *   peripheral will use some LF clock ticks before the command is executed.
 *   The @ref RTCC_SyncWait() function can be used to wait for the stop command
 *   to be executed.
 *
 * @note
 *   This function requires the RTCC to be enabled.
 ******************************************************************************/
__STATIC_INLINE void RTCC_Stop(void)
{
  RTCC_SyncWait();
  RTCC->CMD = RTCC_CMD_STOP;
}
#endif

#if defined (_RTCC_TIME_MASK)
/***************************************************************************//**
 * @brief
 *   Get TIME register value.
 *
 * @return
 *   Current TIME register value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTCC_TimeGet(void)
{
  return RTCC->TIME;
}

/***************************************************************************//**
 * @brief
 *   Set RTCC TIME register.
 *
 * @param[in] time
 *   TIME value.
 ******************************************************************************/
__STATIC_INLINE void RTCC_TimeSet(uint32_t time)
{
  RTCC->TIME = time;
}
#endif

/***************************************************************************//**
 * @brief
 *   Unlock RTCC registers.
 *
 * @note
 *   When RTCC registers are locked, RTCC_CTRL, RTCC_PRECNT, RTCC_CNT,
 *   RTCC_TIME, RTCC_DATE, RTCC_IEN, RTCC_POWERDOWN and RTCC_CCx_XXX registers
 *   cannot be written to.
 ******************************************************************************/
__STATIC_INLINE void RTCC_Unlock(void)
{
#if defined(ERRATA_FIX_RTCC_E203)
  /* RTCC_E203 - Potential Stability Issue with RTCC Registers.
   * RTCC_LOCK register must be modified while RTCC clock is disabled. */
  uint32_t lfeReg = CMU->LFECLKEN0;
  bool cmuLocked = (CMU->LOCK == CMU_LOCK_LOCKKEY_LOCKED);
  if (cmuLocked) {
    CMU->LOCK = CMU_LOCK_LOCKKEY_UNLOCK;
  }
  CMU->LFECLKEN0 = 0x0;
#endif
  RTCC->LOCK = RTCC_LOCK_LOCKKEY_UNLOCK;
#if defined(ERRATA_FIX_RTCC_E203)
  /* Restore clock state after RTCC_E203 fix. */
  CMU->LFECLKEN0 = lfeReg;
  if (cmuLocked) {
    CMU->LOCK = CMU_LOCK_LOCKKEY_LOCK;
  }
#endif
}

/** @} (end addtogroup RTCC) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined( RTCC_COUNT ) && ( RTC_COUNT == 1 ) */
#endif /* EM_RTCC_H */
