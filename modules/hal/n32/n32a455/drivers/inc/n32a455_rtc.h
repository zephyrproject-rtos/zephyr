/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32a455_rtc.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_RTC_H__
#define __N32A455_RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup RTC
 * @{
 */

/**
 * @brief  RTC Init structures definition
 */
typedef struct
{
    uint32_t RTC_HourFormat; /*!< Specifies the RTC Hour Format.
                             This parameter can be a value of @ref RTC_Hour_Formats */

    uint32_t RTC_AsynchPrediv; /*!< Specifies the RTC Asynchronous Predivider value.
                               This parameter must be set to a value lower than 0x7F */

    uint32_t RTC_SynchPrediv; /*!< Specifies the RTC Synchronous Predivider value.
                              This parameter must be set to a value lower than 0x7FFF */
} RTC_InitType;

/**
 * @brief  RTC Time structure definition
 */
typedef struct
{
    uint8_t Hours; /*!< Specifies the RTC Time Hour.
                       This parameter must be set to a value in the 0-12 range
                       if the RTC_12HOUR_FORMAT is selected or 0-23 range if
                       the RTC_24HOUR_FORMAT is selected. */

    uint8_t Minutes; /*!< Specifies the RTC Time Minutes.
                         This parameter must be set to a value in the 0-59 range. */

    uint8_t Seconds; /*!< Specifies the RTC Time Seconds.
                         This parameter must be set to a value in the 0-59 range. */

    uint8_t H12; /*!< Specifies the RTC AM/PM Time.
                     This parameter can be a value of @ref RTC_AM_PM_Definitions */
} RTC_TimeType;

/**
 * @brief  RTC Date structure definition
 */
typedef struct
{
    uint8_t WeekDay; /*!< Specifies the RTC Date WeekDay.
                          This parameter can be a value of @ref RTC_WeekDay_Definitions */

    uint8_t Month; /*!< Specifies the RTC Date Month (in BCD format).
                        This parameter can be a value of @ref RTC_Month_Date_Definitions */

    uint8_t Date; /*!< Specifies the RTC Date.
                      This parameter must be set to a value in the 1-31 range. */

    uint8_t Year; /*!< Specifies the RTC Date Year.
                      This parameter must be set to a value in the 0-99 range. */
} RTC_DateType;

/**
 * @brief  RTC Alarm structure definition
 */
typedef struct
{
    RTC_TimeType AlarmTime; /*!< Specifies the RTC Alarm Time members. */

    uint32_t AlarmMask; /*!< Specifies the RTC Alarm Masks.
                            This parameter can be a value of @ref RTC_AlarmMask_Definitions */

    uint32_t DateWeekMode; /*!< Specifies the RTC Alarm is on Date or WeekDay.
                                      This parameter can be a value of @ref RTC_AlarmDateWeekDay_Definitions */

    uint8_t DateWeekValue; /*!< Specifies the RTC Alarm Date/WeekDay.
                                  If the Alarm Date is selected, this parameter
                                  must be set to a value in the 1-31 range.
                                  If the Alarm WeekDay is selected, this
                                  parameter can be a value of @ref RTC_WeekDay_Definitions */
} RTC_AlarmType;

/** @addtogroup RTC_Exported_Constants
 * @{
 */

/** @addtogroup RTC_Hour_Formats
 * @{
 */
#define RTC_24HOUR_FORMAT          ((uint32_t)0x00000000)
#define RTC_12HOUR_FORMAT          ((uint32_t)0x00000040)
#define IS_RTC_HOUR_FORMAT(FORMAT) (((FORMAT) == RTC_12HOUR_FORMAT) || ((FORMAT) == RTC_24HOUR_FORMAT))
/**
 * @}
 */

/** @addtogroup RTC_Asynchronous_Predivider
 * @{
 */
#define IS_RTC_PREDIV_ASYNCH(PREDIV) ((PREDIV) <= 0x7F)

/**
 * @}
 */

/** @addtogroup RTC_Synchronous_Predivider
 * @{
 */
#define IS_RTC_PREDIV_SYNCH(PREDIV) ((PREDIV) <= 0x7FFF)

/**
 * @}
 */

/** @addtogroup RTC_Time_Definitions
 * @{
 */
#define IS_RTC_12HOUR(HOUR)     (((HOUR) > 0) && ((HOUR) <= 12))
#define IS_RTC_24HOUR(HOUR)     ((HOUR) <= 23)
#define IS_RTC_MINUTES(MINUTES) ((MINUTES) <= 59)
#define IS_RTC_SECONDS(SECONDS) ((SECONDS) <= 59)

/**
 * @}
 */

/** @addtogroup RTC_AM_PM_Definitions
 * @{
 */
#define RTC_AM_H12     ((uint8_t)0x00)
#define RTC_PM_H12     ((uint8_t)0x40)
#define IS_RTC_H12(PM) (((PM) == RTC_AM_H12) || ((PM) == RTC_PM_H12))

/**
 * @}
 */

/** @addtogroup RTC_Year_Date_Definitions
 * @{
 */
#define IS_RTC_YEAR(YEAR) ((YEAR) <= 99)

/**
 * @}
 */

/** @addtogroup RTC_Month_Date_Definitions
 * @{
 */

/* Coded in BCD format */
#define RTC_MONTH_JANUARY   ((uint8_t)0x01)
#define RTC_MONTH_FEBRURY   ((uint8_t)0x02)
#define RTC_MONTH_MARCH     ((uint8_t)0x03)
#define RTC_MONTH_APRIL     ((uint8_t)0x04)
#define RTC_MONTH_MAY       ((uint8_t)0x05)
#define RTC_MONTH_JUNE      ((uint8_t)0x06)
#define RTC_MONTH_JULY      ((uint8_t)0x07)
#define RTC_MONTH_AUGUST    ((uint8_t)0x08)
#define RTC_MONTH_SEPTEMBER ((uint8_t)0x09)
#define RTC_MONTH_OCTOBER   ((uint8_t)0x10)
#define RTC_MONTH_NOVEMBER  ((uint8_t)0x11)
#define RTC_MONTH_DECEMBER  ((uint8_t)0x12)
#define IS_RTC_MONTH(MONTH) (((MONTH) >= 1) && ((MONTH) <= 12))
#define IS_RTC_DATE(DATE)   (((DATE) >= 1) && ((DATE) <= 31))

/**
 * @}
 */

/** @addtogroup RTC_WeekDay_Definitions
 * @{
 */

#define RTC_WEEKDAY_MONDAY    ((uint8_t)0x01)
#define RTC_WEEKDAY_TUESDAY   ((uint8_t)0x02)
#define RTC_WEEKDAY_WEDNESDAY ((uint8_t)0x03)
#define RTC_WEEKDAY_THURSDAY  ((uint8_t)0x04)
#define RTC_WEEKDAY_FRIDAY    ((uint8_t)0x05)
#define RTC_WEEKDAY_SATURDAY  ((uint8_t)0x06)
#define RTC_WEEKDAY_SUNDAY    ((uint8_t)0x07)
#define IS_RTC_WEEKDAY(WEEKDAY)                                                                                        \
    (((WEEKDAY) == RTC_WEEKDAY_MONDAY) || ((WEEKDAY) == RTC_WEEKDAY_TUESDAY) || ((WEEKDAY) == RTC_WEEKDAY_WEDNESDAY)   \
     || ((WEEKDAY) == RTC_WEEKDAY_THURSDAY) || ((WEEKDAY) == RTC_WEEKDAY_FRIDAY)                                       \
     || ((WEEKDAY) == RTC_WEEKDAY_SATURDAY) || ((WEEKDAY) == RTC_WEEKDAY_SUNDAY))
/**
 * @}
 */

/** @addtogroup RTC_Alarm_Definitions
 * @{
 */
#define IS_RTC_ALARM_WEEKDAY_DATE(DATE) (((DATE) > 0) && ((DATE) <= 31))
#define IS_RTC_ALARM_WEEKDAY_WEEKDAY(WEEKDAY)                                                                          \
    (((WEEKDAY) == RTC_WEEKDAY_MONDAY) || ((WEEKDAY) == RTC_WEEKDAY_TUESDAY) || ((WEEKDAY) == RTC_WEEKDAY_WEDNESDAY)   \
     || ((WEEKDAY) == RTC_WEEKDAY_THURSDAY) || ((WEEKDAY) == RTC_WEEKDAY_FRIDAY)                                       \
     || ((WEEKDAY) == RTC_WEEKDAY_SATURDAY) || ((WEEKDAY) == RTC_WEEKDAY_SUNDAY))

/**
 * @}
 */

/** @addtogroup RTC_AlarmDateWeekDay_Definitions
 * @{
 */
#define RTC_ALARM_SEL_WEEKDAY_DATE    ((uint32_t)0x00000000)
#define RTC_ALARM_SEL_WEEKDAY_WEEKDAY ((uint32_t)0x40000000)

#define IS_RTC_ALARM_WEEKDAY_SEL(SEL)                                                                                  \
    (((SEL) == RTC_ALARM_SEL_WEEKDAY_DATE) || ((SEL) == RTC_ALARM_SEL_WEEKDAY_WEEKDAY))

/**
 * @}
 */

/** @addtogroup RTC_AlarmMask_Definitions
 * @{
 */
#define RTC_ALARMMASK_NONE    ((uint32_t)0x00000000)
#define RTC_ALARMMASK_WEEKDAY ((uint32_t)0x80000000)
#define RTC_ALARMMASK_HOURS   ((uint32_t)0x00800000)
#define RTC_ALARMMASK_MINUTES ((uint32_t)0x00008000)
#define RTC_ALARMMASK_SECONDS ((uint32_t)0x00000080)
#define RTC_ALARMMASK_ALL     ((uint32_t)0x80808080)
#define IS_ALARM_MASK(INTEN)  (((INTEN)&0x7F7F7F7F) == (uint32_t)RESET)

/**
 * @}
 */

/** @addtogroup RTC_Alarms_Definitions
 * @{
 */
#define RTC_A_ALARM                ((uint32_t)0x00000100)
#define RTC_B_ALARM                ((uint32_t)0x00000200)
#define IS_RTC_ALARM_SEL(ALARM)    (((ALARM) == RTC_A_ALARM) || ((ALARM) == RTC_B_ALARM))
#define IS_RTC_ALARM_ENABLE(ALARM) (((ALARM) & (RTC_A_ALARM | RTC_B_ALARM)) != (uint32_t)RESET)

/**
 * @}
 */

/** @addtogroup RTC_Alarm_Sub_Seconds_Masks_Definitions
 * @{
 */
#define RTC_SUBS_MASK_ALL                                                                                              \
    ((uint32_t)0x00000000) /*!< All Alarm SS fields are masked.                                                        \
                                         There is no comparison on sub seconds                                         \
                                         for Alarm */
#define RTC_SUBS_MASK_SS14_1                                                                                           \
    ((uint32_t)0x01000000) /*!< SS[14:1] are don't care in Alarm                                                       \
                                         comparison. Only SS[0] is compared. */
#define RTC_SUBS_MASK_SS14_2                                                                                           \
    ((uint32_t)0x02000000) /*!< SS[14:2] are don't care in Alarm                                                       \
                                         comparison. Only SS[1:0] are compared */
#define RTC_SUBS_MASK_SS14_3                                                                                           \
    ((uint32_t)0x03000000) /*!< SS[14:3] are don't care in Alarm                                                       \
                                         comparison. Only SS[2:0] are compared */
#define RTC_SUBS_MASK_SS14_4                                                                                           \
    ((uint32_t)0x04000000) /*!< SS[14:4] are don't care in Alarm                                                       \
                                         comparison. Only SS[3:0] are compared */
#define RTC_SUBS_MASK_SS14_5                                                                                           \
    ((uint32_t)0x05000000) /*!< SS[14:5] are don't care in Alarm                                                       \
                                         comparison. Only SS[4:0] are compared */
#define RTC_SUBS_MASK_SS14_6                                                                                           \
    ((uint32_t)0x06000000) /*!< SS[14:6] are don't care in Alarm                                                       \
                                         comparison. Only SS[5:0] are compared */
#define RTC_SUBS_MASK_SS14_7                                                                                           \
    ((uint32_t)0x07000000) /*!< SS[14:7] are don't care in Alarm                                                       \
                                         comparison. Only SS[6:0] are compared */
#define RTC_SUBS_MASK_SS14_8                                                                                           \
    ((uint32_t)0x08000000) /*!< SS[14:8] are don't care in Alarm                                                       \
                                         comparison. Only SS[7:0] are compared */
#define RTC_SUBS_MASK_SS14_9                                                                                           \
    ((uint32_t)0x09000000) /*!< SS[14:9] are don't care in Alarm                                                       \
                                         comparison. Only SS[8:0] are compared */
#define RTC_SUBS_MASK_SS14_10                                                                                          \
    ((uint32_t)0x0A000000) /*!< SS[14:10] are don't care in Alarm                                                      \
                                         comparison. Only SS[9:0] are compared */
#define RTC_SUBS_MASK_SS14_11                                                                                          \
    ((uint32_t)0x0B000000) /*!< SS[14:11] are don't care in Alarm                                                      \
                                         comparison. Only SS[10:0] are compared */
#define RTC_SUBS_MASK_SS14_12                                                                                          \
    ((uint32_t)0x0C000000) /*!< SS[14:12] are don't care in Alarm                                                      \
                                         comparison.Only SS[11:0] are compared */
#define RTC_SUBS_MASK_SS14_13                                                                                          \
    ((uint32_t)0x0D000000) /*!< SS[14:13] are don't care in Alarm                                                      \
                                         comparison. Only SS[12:0] are compared */
#define RTC_SUBS_MASK_SS14_14                                                                                          \
    ((uint32_t)0x0E000000) /*!< SS[14] is don't care in Alarm                                                          \
                                      comparison.Only SS[13:0] are compared */
#define RTC_SUBS_MASK_NONE                                                                                             \
    ((uint32_t)0x0F000000) /*!< SS[14:0] are compared and must match                                                   \
                                         to activate alarm. */
#define IS_RTC_ALARM_SUB_SECOND_MASK_MODE(INTEN)                                                                       \
    (((INTEN) == RTC_SUBS_MASK_ALL) || ((INTEN) == RTC_SUBS_MASK_SS14_1) || ((INTEN) == RTC_SUBS_MASK_SS14_2)          \
     || ((INTEN) == RTC_SUBS_MASK_SS14_3) || ((INTEN) == RTC_SUBS_MASK_SS14_4) || ((INTEN) == RTC_SUBS_MASK_SS14_5)    \
     || ((INTEN) == RTC_SUBS_MASK_SS14_6) || ((INTEN) == RTC_SUBS_MASK_SS14_7) || ((INTEN) == RTC_SUBS_MASK_SS14_8)    \
     || ((INTEN) == RTC_SUBS_MASK_SS14_9) || ((INTEN) == RTC_SUBS_MASK_SS14_10) || ((INTEN) == RTC_SUBS_MASK_SS14_11)  \
     || ((INTEN) == RTC_SUBS_MASK_SS14_12) || ((INTEN) == RTC_SUBS_MASK_SS14_13) || ((INTEN) == RTC_SUBS_MASK_SS14_14) \
     || ((INTEN) == RTC_SUBS_MASK_NONE))
/**
 * @}
 */

/** @addtogroup RTC_Alarm_Sub_Seconds_Value
 * @{
 */

#define IS_RTC_ALARM_SUB_SECOND_VALUE(VALUE) ((VALUE) <= 0x00007FFF)

/**
 * @}
 */

/** @addtogroup RTC_Wakeup_Timer_Definitions
 * @{
 */
#define RTC_WKUPCLK_RTCCLK_DIV16   ((uint32_t)0x00000000)
#define RTC_WKUPCLK_RTCCLK_DIV8    ((uint32_t)0x00000001)
#define RTC_WKUPCLK_RTCCLK_DIV4    ((uint32_t)0x00000002)
#define RTC_WKUPCLK_RTCCLK_DIV2    ((uint32_t)0x00000003)
#define RTC_WKUPCLK_CK_SPRE_16BITS ((uint32_t)0x00000004)

#define IS_RTC_WKUP_CLOCK(CLOCK)                                                                                       \
    (((CLOCK) == RTC_WKUPCLK_RTCCLK_DIV16) || ((CLOCK) == RTC_WKUPCLK_RTCCLK_DIV8)                                     \
     || ((CLOCK) == RTC_WKUPCLK_RTCCLK_DIV4) || ((CLOCK) == RTC_WKUPCLK_RTCCLK_DIV2)                                   \
     || ((CLOCK) == RTC_WKUPCLK_CK_SPRE_16BITS))
#define IS_RTC_WKUP_COUNTER(COUNTER) ((COUNTER) <= 0xFFFF)
/**
 * @}
 */

/** @addtogroup RTC_Time_Stamp_Edges_definitions
 * @{
 */
#define RTC_TIMESTAMP_EDGE_RISING  ((uint32_t)0x00000000)
#define RTC_TIMESTAMP_EDGE_FALLING ((uint32_t)0x00000008)
#define IS_RTC_TIMESTAMP_EDGE_MODE(EDGE)                                                                               \
    (((EDGE) == RTC_TIMESTAMP_EDGE_RISING) || ((EDGE) == RTC_TIMESTAMP_EDGE_FALLING))
/**
 * @}
 */

/** @addtogroup RTC_Output_selection_Definitions
 * @{
 */
#define RTC_OUTPUT_DIS  ((uint32_t)0x00000000)
#define RTC_OUTPUT_ALA  ((uint32_t)0x00200000)
#define RTC_OUTPUT_ALB  ((uint32_t)0x00400000)
#define RTC_OUTPUT_WKUP ((uint32_t)0x00600000)

#define IS_RTC_OUTPUT_MODE(OUTPUT)                                                                                     \
    (((OUTPUT) == RTC_OUTPUT_DIS) || ((OUTPUT) == RTC_OUTPUT_ALA) || ((OUTPUT) == RTC_OUTPUT_ALB)                      \
     || ((OUTPUT) == RTC_OUTPUT_WKUP))

/**
 * @}
 */

/** @addtogroup RTC_Output_Polarity_Definitions
 * @{
 */
#define RTC_OUTPOL_HIGH        ((uint32_t)0x00000000)
#define RTC_OUTPOL_LOW         ((uint32_t)0x00100000)
#define IS_RTC_OUTPUT_POL(POL) (((POL) == RTC_OUTPOL_HIGH) || ((POL) == RTC_OUTPOL_LOW))
/**
 * @}
 */


/** @addtogroup RTC_Calib_Output_selection_Definitions
 * @{
 */
#define RTC_CALIB_OUTPUT_256HZ      ((uint32_t)0x00000000)
#define RTC_CALIB_OUTPUT_1HZ        ((uint32_t)0x00080000)
#define IS_RTC_CALIB_OUTPUT(OUTPUT) (((OUTPUT) == RTC_CALIB_OUTPUT_256HZ) || ((OUTPUT) == RTC_CALIB_OUTPUT_1HZ))
/**
 * @}
 */

/** @addtogroup RTC_Smooth_calib_period_Definitions
 * @{
 */
#define SMOOTH_CALIB_32SEC                                                                                             \
    ((uint32_t)0x00000000) /*!<  if RTCCLK = 32768 Hz, Smooth calibation                                               \
                                    period is 32s,  else 2exp20 RTCCLK seconds */
#define SMOOTH_CALIB_16SEC                                                                                             \
    ((uint32_t)0x00002000) /*!<  if RTCCLK = 32768 Hz, Smooth calibation                                               \
                                    period is 16s, else 2exp19 RTCCLK seconds */
#define SMOOTH_CALIB_8SEC                                                                                              \
    ((uint32_t)0x00004000) /*!<  if RTCCLK = 32768 Hz, Smooth calibation                                               \
                                    period is 8s, else 2exp18 RTCCLK seconds */
#define IS_RTC_SMOOTH_CALIB_PERIOD_SEL(PERIOD)                                                                         \
    (((PERIOD) == SMOOTH_CALIB_32SEC) || ((PERIOD) == SMOOTH_CALIB_16SEC) || ((PERIOD) == SMOOTH_CALIB_8SEC))

/**
 * @}
 */

/** @addtogroup RTC_Smooth_calib_Plus_pulses_Definitions
 * @{
 */
#define RTC_SMOOTH_CALIB_PLUS_PULSES_SET                                                                               \
    ((uint32_t)0x00008000) /*!<  The number of RTCCLK pulses added                                                     \
                        during a X -second window = Y - CALM[8:0].                                                     \
                         with Y = 512, 256, 128 when X = 32, 16, 8 */
#define RTC_SMOOTH_CALIB_PLUS_PULSES__RESET                                                                            \
    ((uint32_t)0x00000000) /*!<  The number of RTCCLK pulses subbstited                                                \
                        during a 32-second window =   CALM[8:0]. */
#define IS_RTC_SMOOTH_CALIB_PLUS(PLUS)                                                                                 \
    (((PLUS) == RTC_SMOOTH_CALIB_PLUS_PULSES_SET) || ((PLUS) == RTC_SMOOTH_CALIB_PLUS_PULSES__RESET))

/**
 * @}
 */

/** @addtogroup RTC_Smooth_calib_Minus_pulses_Definitions
 * @{
 */
#define IS_RTC_SMOOTH_CALIB_MINUS(VALUE) ((VALUE) <= 0x000001FF)

/**
 * @}
 */

/** @addtogroup RTC_DayLightSaving_Definitions
 * @{
 */
#define RTC_DAYLIGHT_SAVING_SUB1H    ((uint32_t)0x00020000)
#define RTC_DAYLIGHT_SAVING_ADD1H    ((uint32_t)0x00010000)
#define IS_RTC_DAYLIGHT_SAVING(SAVE) (((SAVE) == RTC_DAYLIGHT_SAVING_SUB1H) || ((SAVE) == RTC_DAYLIGHT_SAVING_ADD1H))

#define RTC_STORE_OPERATION_RESET ((uint32_t)0x00000000)
#define RTC_STORE_OPERATION_SET   ((uint32_t)0x00040000)
#define IS_RTC_STORE_OPERATION(OPERATION)                                                                              \
    (((OPERATION) == RTC_STORE_OPERATION_RESET) || ((OPERATION) == RTC_STORE_OPERATION_SET))
/**
 * @}
 */

/** @addtogroup RTC_Output_Type_ALARM_OUT
 * @{
 */
#define RTC_OUTPUT_OPENDRAIN     ((uint32_t)0x00000000)
#define RTC_OUTPUT_PUSHPULL      ((uint32_t)0x00000001)
#define IS_RTC_OUTPUT_TYPE(TYPE) (((TYPE) == RTC_OUTPUT_OPENDRAIN) || ((TYPE) == RTC_OUTPUT_PUSHPULL))

/**
 * @}
 */
/** @addtogroup RTC_Add_Fraction_Of_Second_Value
 * @{
 */
#define RTC_SHIFT_SUB1S_DISABLE ((uint32_t)0x00000000)
#define RTC_SHIFT_SUB1S_ENABLE  ((uint32_t)0x80000000)
#define IS_RTC_SHIFT_SUB1S(SEL) (((SEL) == RTC_SHIFT_SUB1S_DISABLE) || ((SEL) == RTC_SHIFT_SUB1S_ENABLE))
/**
 * @}
 */
/** @addtogroup RTC_Substract_1_Second_Parameter_Definitions
 * @{
 */
#define IS_RTC_SHIFT_ADFS(FS) ((FS) <= 0x00007FFF)

/**
 * @}
 */

/** @addtogroup RTC_Input_parameter_format_definitions
 * @{
 */
#define RTC_FORMAT_BIN        ((uint32_t)0x000000000)
#define RTC_FORMAT_BCD        ((uint32_t)0x000000001)
#define IS_RTC_FORMAT(FORMAT) (((FORMAT) == RTC_FORMAT_BIN) || ((FORMAT) == RTC_FORMAT_BCD))

/**
 * @}
 */

/** @addtogroup RTC_Flags_Definitions
 * @{
 */
#define RTC_FLAG_RECPF  ((uint32_t)0x00010000)
#define RTC_FLAG_TISOVF ((uint32_t)0x00001000)
#define RTC_FLAG_TISF   ((uint32_t)0x00000800)
#define RTC_FLAG_WTF    ((uint32_t)0x00000400)
#define RTC_FLAG_ALBF   ((uint32_t)0x00000200)
#define RTC_FLAG_ALAF   ((uint32_t)0x00000100)
#define RTC_FLAG_INITF  ((uint32_t)0x00000040)
#define RTC_FLAG_RSYF   ((uint32_t)0x00000020)
#define RTC_FLAG_INITSF ((uint32_t)0x00000010)
#define RTC_FLAG_SHOPF  ((uint32_t)0x00000008)
#define RTC_FLAG_WTWF   ((uint32_t)0x00000004)
#define RTC_FLAG_ALBWF  ((uint32_t)0x00000002)
#define RTC_FLAG_ALAWF  ((uint32_t)0x00000001)
#define IS_RTC_GET_FLAG(FLAG)                                                                                          \
    (((FLAG) == RTC_FLAG_TISOVF) || ((FLAG) == RTC_FLAG_TISF) || ((FLAG) == RTC_FLAG_WTF) || ((FLAG) == RTC_FLAG_ALBF) \
     || ((FLAG) == RTC_FLAG_ALAF) || ((FLAG) == RTC_FLAG_INITF) || ((FLAG) == RTC_FLAG_RSYF)                           \
     || ((FLAG) == RTC_FLAG_WTWF) || ((FLAG) == RTC_FLAG_ALBWF) || ((FLAG) == RTC_FLAG_ALAWF)                          \
     || ((FLAG) == RTC_FLAG_RECPF) || ((FLAG) == RTC_FLAG_SHOPF) || ((FLAG) == RTC_FLAG_INITSF))
#define IS_RTC_CLEAR_FLAG(FLAG) (((FLAG) != (uint32_t)RESET) && (((FLAG)&0x00011fff) == (uint32_t)SET))

/**
 * @}
 */

/** @addtogroup RTC_Interrupts_Definitions
 * @{
 */

#define RTC_INT_WUT  ((uint32_t)0x00004000)
#define RTC_INT_ALRB ((uint32_t)0x00002000)
#define RTC_INT_ALRA ((uint32_t)0x00001000)

#define IS_RTC_CONFIG_INT(IT) (((IT) != (uint32_t)RESET) && (((IT)&0xFFFF0FFB) == (uint32_t)RESET))
#define IS_RTC_GET_INT(IT)                                                                                             \
    (((IT) == RTC_INT_WUT) || ((IT) == RTC_INT_ALRB) || ((IT) == RTC_INT_ALRA))
#define IS_RTC_CLEAR_INT(IT) (((IT) != (uint32_t)RESET) && (((IT)&0x00007000) == (uint32_t)SET))

/**
 * @}
 */

/** @addtogroup RTC_Legacy
 * @{
 */
#define RTC_DigitalCalibConfig RTC_CoarseCalibConfig
#define RTC_DigitalCalibCmd    RTC_CoarseCalibCmd

/**
 * @}
 */

/**
 * @}
 */

/*  Function used to set the RTC configuration to the default reset state *****/
ErrorStatus RTC_DeInit(void);

/* Initialization and Configuration functions *********************************/
ErrorStatus RTC_Init(RTC_InitType* RTC_InitStruct);
void RTC_StructInit(RTC_InitType* RTC_InitStruct);
void RTC_EnableWriteProtection(FunctionalState Cmd);
ErrorStatus RTC_EnterInitMode(void);
void RTC_ExitInitMode(void);
ErrorStatus RTC_WaitForSynchro(void);
ErrorStatus RTC_EnableRefClock(FunctionalState Cmd);
void RTC_EnableBypassShadow(FunctionalState Cmd);

/* Time and Date configuration functions **************************************/
ErrorStatus RTC_ConfigTime(uint32_t RTC_Format, RTC_TimeType* RTC_TimeStruct);
void RTC_TimeStructInit(RTC_TimeType* RTC_TimeStruct);
void RTC_GetTime(uint32_t RTC_Format, RTC_TimeType* RTC_TimeStruct);
uint32_t RTC_GetSubSecond(void);
ErrorStatus RTC_SetDate(uint32_t RTC_Format, RTC_DateType* RTC_DateStruct);
void RTC_DateStructInit(RTC_DateType* RTC_DateStruct);
void RTC_GetDate(uint32_t RTC_Format, RTC_DateType* RTC_DateStruct);

/* Alarms (Alarm A and Alarm B) configuration functions  **********************/
void RTC_SetAlarm(uint32_t RTC_Format, uint32_t RTC_Alarm, RTC_AlarmType* RTC_AlarmStruct);
void RTC_AlarmStructInit(RTC_AlarmType* RTC_AlarmStruct);
void RTC_GetAlarm(uint32_t RTC_Format, uint32_t RTC_Alarm, RTC_AlarmType* RTC_AlarmStruct);
ErrorStatus RTC_EnableAlarm(uint32_t RTC_Alarm, FunctionalState Cmd);
void RTC_ConfigAlarmSubSecond(uint32_t RTC_Alarm, uint32_t RTC_AlarmSubSecondValue, uint32_t RTC_AlarmSubSecondMask);
uint32_t RTC_GetAlarmSubSecond(uint32_t RTC_Alarm);

/* WakeUp Timer configuration functions ***************************************/
void RTC_ConfigWakeUpClock(uint32_t RTC_WakeUpClock);
void RTC_SetWakeUpCounter(uint32_t RTC_WakeUpCounter);
uint32_t RTC_GetWakeUpCounter(void);
ErrorStatus RTC_EnableWakeUp(FunctionalState Cmd);

/* Daylight Saving configuration functions ************************************/
void RTC_ConfigDayLightSaving(uint32_t RTC_DayLightSaving, uint32_t RTC_StoreOperation);
uint32_t RTC_GetStoreOperation(void);

/* Output pin Configuration function ******************************************/
void RTC_ConfigOutput(uint32_t RTC_Output, uint32_t RTC_OutputPolarity);

/* Coarse and Smooth Calibration configuration functions **********************/
void RTC_EnableCalibOutput(FunctionalState Cmd);
void RTC_ConfigCalibOutput(uint32_t RTC_CalibOutput);
ErrorStatus RTC_ConfigSmoothCalib(uint32_t RTC_SmoothCalibPeriod,
                                  uint32_t RTC_SmoothCalibPlusPulses,
                                  uint32_t RTC_SmouthCalibMinusPulsesValue);

/* TimeStamp configuration functions ******************************************/
void RTC_EnableTimeStamp(uint32_t RTC_TimeStampEdge, FunctionalState Cmd);
void RTC_GetTimeStamp(uint32_t RTC_Format, RTC_TimeType* RTC_StampTimeStruct, RTC_DateType* RTC_StampDateStruct);
uint32_t RTC_GetTimeStampSubSecond(void);

/* Output Type Config configuration functions *********************************/
void RTC_ConfigOutputType(uint32_t RTC_OutputType);

/* RTC_Shift_control_synchonisation_functions *********************************/
ErrorStatus RTC_ConfigSynchroShift(uint32_t RTC_ShiftAddFS, uint32_t RTC_ShiftSub1s);

/* Interrupts and flags management functions **********************************/
void RTC_ConfigInt(uint32_t RTC_INT, FunctionalState Cmd);
FlagStatus RTC_GetFlagStatus(uint32_t RTC_FLAG);
void RTC_ClrFlag(uint32_t RTC_FLAG);
INTStatus RTC_GetITStatus(uint32_t RTC_INT);
void RTC_ClrIntPendingBit(uint32_t RTC_INT);

#ifdef __cplusplus
}
#endif

#endif /*__N32A455_RTC_H__ */

/**
 * @}
 */

/**
 * @}
 */
