/**
 * \file
 *
 * \brief Component description for RTC
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_RTC_COMPONENT_H_
#define _SAME70_RTC_COMPONENT_H_
#define _SAME70_RTC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_RTC Real-time Clock
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR RTC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define RTC_6056                       /**< (RTC) Module ID */
#define REV_RTC W                      /**< (RTC) Module revision */

/* -------- RTC_CR : (RTC Offset: 0x00) (R/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t UPDTIM:1;                  /**< bit:      0  Update Request Time Register             */
    uint32_t UPDCAL:1;                  /**< bit:      1  Update Request Calendar Register         */
    uint32_t :6;                        /**< bit:   2..7  Reserved */
    uint32_t TIMEVSEL:2;                /**< bit:   8..9  Time Event Selection                     */
    uint32_t :6;                        /**< bit: 10..15  Reserved */
    uint32_t CALEVSEL:2;                /**< bit: 16..17  Calendar Event Selection                 */
    uint32_t :14;                       /**< bit: 18..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_CR_OFFSET                       (0x00)                                        /**<  (RTC_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_CR_UPDTIM_Pos                   0                                              /**< (RTC_CR) Update Request Time Register Position */
#define RTC_CR_UPDTIM_Msk                   (0x1U << RTC_CR_UPDTIM_Pos)                    /**< (RTC_CR) Update Request Time Register Mask */
#define RTC_CR_UPDTIM                       RTC_CR_UPDTIM_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_CR_UPDTIM_Msk instead */
#define RTC_CR_UPDCAL_Pos                   1                                              /**< (RTC_CR) Update Request Calendar Register Position */
#define RTC_CR_UPDCAL_Msk                   (0x1U << RTC_CR_UPDCAL_Pos)                    /**< (RTC_CR) Update Request Calendar Register Mask */
#define RTC_CR_UPDCAL                       RTC_CR_UPDCAL_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_CR_UPDCAL_Msk instead */
#define RTC_CR_TIMEVSEL_Pos                 8                                              /**< (RTC_CR) Time Event Selection Position */
#define RTC_CR_TIMEVSEL_Msk                 (0x3U << RTC_CR_TIMEVSEL_Pos)                  /**< (RTC_CR) Time Event Selection Mask */
#define RTC_CR_TIMEVSEL(value)              (RTC_CR_TIMEVSEL_Msk & ((value) << RTC_CR_TIMEVSEL_Pos))
#define   RTC_CR_TIMEVSEL_MINUTE_Val        (0x0U)                                         /**< (RTC_CR) Minute change  */
#define   RTC_CR_TIMEVSEL_HOUR_Val          (0x1U)                                         /**< (RTC_CR) Hour change  */
#define   RTC_CR_TIMEVSEL_MIDNIGHT_Val      (0x2U)                                         /**< (RTC_CR) Every day at midnight  */
#define   RTC_CR_TIMEVSEL_NOON_Val          (0x3U)                                         /**< (RTC_CR) Every day at noon  */
#define RTC_CR_TIMEVSEL_MINUTE              (RTC_CR_TIMEVSEL_MINUTE_Val << RTC_CR_TIMEVSEL_Pos)  /**< (RTC_CR) Minute change Position  */
#define RTC_CR_TIMEVSEL_HOUR                (RTC_CR_TIMEVSEL_HOUR_Val << RTC_CR_TIMEVSEL_Pos)  /**< (RTC_CR) Hour change Position  */
#define RTC_CR_TIMEVSEL_MIDNIGHT            (RTC_CR_TIMEVSEL_MIDNIGHT_Val << RTC_CR_TIMEVSEL_Pos)  /**< (RTC_CR) Every day at midnight Position  */
#define RTC_CR_TIMEVSEL_NOON                (RTC_CR_TIMEVSEL_NOON_Val << RTC_CR_TIMEVSEL_Pos)  /**< (RTC_CR) Every day at noon Position  */
#define RTC_CR_CALEVSEL_Pos                 16                                             /**< (RTC_CR) Calendar Event Selection Position */
#define RTC_CR_CALEVSEL_Msk                 (0x3U << RTC_CR_CALEVSEL_Pos)                  /**< (RTC_CR) Calendar Event Selection Mask */
#define RTC_CR_CALEVSEL(value)              (RTC_CR_CALEVSEL_Msk & ((value) << RTC_CR_CALEVSEL_Pos))
#define   RTC_CR_CALEVSEL_WEEK_Val          (0x0U)                                         /**< (RTC_CR) Week change (every Monday at time 00:00:00)  */
#define   RTC_CR_CALEVSEL_MONTH_Val         (0x1U)                                         /**< (RTC_CR) Month change (every 01 of each month at time 00:00:00)  */
#define   RTC_CR_CALEVSEL_YEAR_Val          (0x2U)                                         /**< (RTC_CR) Year change (every January 1 at time 00:00:00)  */
#define RTC_CR_CALEVSEL_WEEK                (RTC_CR_CALEVSEL_WEEK_Val << RTC_CR_CALEVSEL_Pos)  /**< (RTC_CR) Week change (every Monday at time 00:00:00) Position  */
#define RTC_CR_CALEVSEL_MONTH               (RTC_CR_CALEVSEL_MONTH_Val << RTC_CR_CALEVSEL_Pos)  /**< (RTC_CR) Month change (every 01 of each month at time 00:00:00) Position  */
#define RTC_CR_CALEVSEL_YEAR                (RTC_CR_CALEVSEL_YEAR_Val << RTC_CR_CALEVSEL_Pos)  /**< (RTC_CR) Year change (every January 1 at time 00:00:00) Position  */
#define RTC_CR_MASK                         (0x30303U)                                     /**< \deprecated (RTC_CR) Register MASK  (Use RTC_CR_Msk instead)  */
#define RTC_CR_Msk                          (0x30303U)                                     /**< (RTC_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_MR : (RTC Offset: 0x04) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t HRMOD:1;                   /**< bit:      0  12-/24-hour Mode                         */
    uint32_t PERSIAN:1;                 /**< bit:      1  PERSIAN Calendar                         */
    uint32_t :2;                        /**< bit:   2..3  Reserved */
    uint32_t NEGPPM:1;                  /**< bit:      4  NEGative PPM Correction                  */
    uint32_t :3;                        /**< bit:   5..7  Reserved */
    uint32_t CORRECTION:7;              /**< bit:  8..14  Slow Clock Correction                    */
    uint32_t HIGHPPM:1;                 /**< bit:     15  HIGH PPM Correction                      */
    uint32_t OUT0:3;                    /**< bit: 16..18  RTCOUT0 OutputSource Selection           */
    uint32_t :1;                        /**< bit:     19  Reserved */
    uint32_t OUT1:3;                    /**< bit: 20..22  RTCOUT1 Output Source Selection          */
    uint32_t :1;                        /**< bit:     23  Reserved */
    uint32_t THIGH:3;                   /**< bit: 24..26  High Duration of the Output Pulse        */
    uint32_t :1;                        /**< bit:     27  Reserved */
    uint32_t TPERIOD:2;                 /**< bit: 28..29  Period of the Output Pulse               */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_MR_OFFSET                       (0x04)                                        /**<  (RTC_MR) Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_MR_HRMOD_Pos                    0                                              /**< (RTC_MR) 12-/24-hour Mode Position */
#define RTC_MR_HRMOD_Msk                    (0x1U << RTC_MR_HRMOD_Pos)                     /**< (RTC_MR) 12-/24-hour Mode Mask */
#define RTC_MR_HRMOD                        RTC_MR_HRMOD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_MR_HRMOD_Msk instead */
#define RTC_MR_PERSIAN_Pos                  1                                              /**< (RTC_MR) PERSIAN Calendar Position */
#define RTC_MR_PERSIAN_Msk                  (0x1U << RTC_MR_PERSIAN_Pos)                   /**< (RTC_MR) PERSIAN Calendar Mask */
#define RTC_MR_PERSIAN                      RTC_MR_PERSIAN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_MR_PERSIAN_Msk instead */
#define RTC_MR_NEGPPM_Pos                   4                                              /**< (RTC_MR) NEGative PPM Correction Position */
#define RTC_MR_NEGPPM_Msk                   (0x1U << RTC_MR_NEGPPM_Pos)                    /**< (RTC_MR) NEGative PPM Correction Mask */
#define RTC_MR_NEGPPM                       RTC_MR_NEGPPM_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_MR_NEGPPM_Msk instead */
#define RTC_MR_CORRECTION_Pos               8                                              /**< (RTC_MR) Slow Clock Correction Position */
#define RTC_MR_CORRECTION_Msk               (0x7FU << RTC_MR_CORRECTION_Pos)               /**< (RTC_MR) Slow Clock Correction Mask */
#define RTC_MR_CORRECTION(value)            (RTC_MR_CORRECTION_Msk & ((value) << RTC_MR_CORRECTION_Pos))
#define RTC_MR_HIGHPPM_Pos                  15                                             /**< (RTC_MR) HIGH PPM Correction Position */
#define RTC_MR_HIGHPPM_Msk                  (0x1U << RTC_MR_HIGHPPM_Pos)                   /**< (RTC_MR) HIGH PPM Correction Mask */
#define RTC_MR_HIGHPPM                      RTC_MR_HIGHPPM_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_MR_HIGHPPM_Msk instead */
#define RTC_MR_OUT0_Pos                     16                                             /**< (RTC_MR) RTCOUT0 OutputSource Selection Position */
#define RTC_MR_OUT0_Msk                     (0x7U << RTC_MR_OUT0_Pos)                      /**< (RTC_MR) RTCOUT0 OutputSource Selection Mask */
#define RTC_MR_OUT0(value)                  (RTC_MR_OUT0_Msk & ((value) << RTC_MR_OUT0_Pos))
#define   RTC_MR_OUT0_NO_WAVE_Val           (0x0U)                                         /**< (RTC_MR) No waveform, stuck at '0'  */
#define   RTC_MR_OUT0_FREQ1HZ_Val           (0x1U)                                         /**< (RTC_MR) 1 Hz square wave  */
#define   RTC_MR_OUT0_FREQ32HZ_Val          (0x2U)                                         /**< (RTC_MR) 32 Hz square wave  */
#define   RTC_MR_OUT0_FREQ64HZ_Val          (0x3U)                                         /**< (RTC_MR) 64 Hz square wave  */
#define   RTC_MR_OUT0_FREQ512HZ_Val         (0x4U)                                         /**< (RTC_MR) 512 Hz square wave  */
#define   RTC_MR_OUT0_ALARM_TOGGLE_Val      (0x5U)                                         /**< (RTC_MR) Output toggles when alarm flag rises  */
#define   RTC_MR_OUT0_ALARM_FLAG_Val        (0x6U)                                         /**< (RTC_MR) Output is a copy of the alarm flag  */
#define   RTC_MR_OUT0_PROG_PULSE_Val        (0x7U)                                         /**< (RTC_MR) Duty cycle programmable pulse  */
#define RTC_MR_OUT0_NO_WAVE                 (RTC_MR_OUT0_NO_WAVE_Val << RTC_MR_OUT0_Pos)   /**< (RTC_MR) No waveform, stuck at '0' Position  */
#define RTC_MR_OUT0_FREQ1HZ                 (RTC_MR_OUT0_FREQ1HZ_Val << RTC_MR_OUT0_Pos)   /**< (RTC_MR) 1 Hz square wave Position  */
#define RTC_MR_OUT0_FREQ32HZ                (RTC_MR_OUT0_FREQ32HZ_Val << RTC_MR_OUT0_Pos)  /**< (RTC_MR) 32 Hz square wave Position  */
#define RTC_MR_OUT0_FREQ64HZ                (RTC_MR_OUT0_FREQ64HZ_Val << RTC_MR_OUT0_Pos)  /**< (RTC_MR) 64 Hz square wave Position  */
#define RTC_MR_OUT0_FREQ512HZ               (RTC_MR_OUT0_FREQ512HZ_Val << RTC_MR_OUT0_Pos)  /**< (RTC_MR) 512 Hz square wave Position  */
#define RTC_MR_OUT0_ALARM_TOGGLE            (RTC_MR_OUT0_ALARM_TOGGLE_Val << RTC_MR_OUT0_Pos)  /**< (RTC_MR) Output toggles when alarm flag rises Position  */
#define RTC_MR_OUT0_ALARM_FLAG              (RTC_MR_OUT0_ALARM_FLAG_Val << RTC_MR_OUT0_Pos)  /**< (RTC_MR) Output is a copy of the alarm flag Position  */
#define RTC_MR_OUT0_PROG_PULSE              (RTC_MR_OUT0_PROG_PULSE_Val << RTC_MR_OUT0_Pos)  /**< (RTC_MR) Duty cycle programmable pulse Position  */
#define RTC_MR_OUT1_Pos                     20                                             /**< (RTC_MR) RTCOUT1 Output Source Selection Position */
#define RTC_MR_OUT1_Msk                     (0x7U << RTC_MR_OUT1_Pos)                      /**< (RTC_MR) RTCOUT1 Output Source Selection Mask */
#define RTC_MR_OUT1(value)                  (RTC_MR_OUT1_Msk & ((value) << RTC_MR_OUT1_Pos))
#define   RTC_MR_OUT1_NO_WAVE_Val           (0x0U)                                         /**< (RTC_MR) No waveform, stuck at '0'  */
#define   RTC_MR_OUT1_FREQ1HZ_Val           (0x1U)                                         /**< (RTC_MR) 1 Hz square wave  */
#define   RTC_MR_OUT1_FREQ32HZ_Val          (0x2U)                                         /**< (RTC_MR) 32 Hz square wave  */
#define   RTC_MR_OUT1_FREQ64HZ_Val          (0x3U)                                         /**< (RTC_MR) 64 Hz square wave  */
#define   RTC_MR_OUT1_FREQ512HZ_Val         (0x4U)                                         /**< (RTC_MR) 512 Hz square wave  */
#define   RTC_MR_OUT1_ALARM_TOGGLE_Val      (0x5U)                                         /**< (RTC_MR) Output toggles when alarm flag rises  */
#define   RTC_MR_OUT1_ALARM_FLAG_Val        (0x6U)                                         /**< (RTC_MR) Output is a copy of the alarm flag  */
#define   RTC_MR_OUT1_PROG_PULSE_Val        (0x7U)                                         /**< (RTC_MR) Duty cycle programmable pulse  */
#define RTC_MR_OUT1_NO_WAVE                 (RTC_MR_OUT1_NO_WAVE_Val << RTC_MR_OUT1_Pos)   /**< (RTC_MR) No waveform, stuck at '0' Position  */
#define RTC_MR_OUT1_FREQ1HZ                 (RTC_MR_OUT1_FREQ1HZ_Val << RTC_MR_OUT1_Pos)   /**< (RTC_MR) 1 Hz square wave Position  */
#define RTC_MR_OUT1_FREQ32HZ                (RTC_MR_OUT1_FREQ32HZ_Val << RTC_MR_OUT1_Pos)  /**< (RTC_MR) 32 Hz square wave Position  */
#define RTC_MR_OUT1_FREQ64HZ                (RTC_MR_OUT1_FREQ64HZ_Val << RTC_MR_OUT1_Pos)  /**< (RTC_MR) 64 Hz square wave Position  */
#define RTC_MR_OUT1_FREQ512HZ               (RTC_MR_OUT1_FREQ512HZ_Val << RTC_MR_OUT1_Pos)  /**< (RTC_MR) 512 Hz square wave Position  */
#define RTC_MR_OUT1_ALARM_TOGGLE            (RTC_MR_OUT1_ALARM_TOGGLE_Val << RTC_MR_OUT1_Pos)  /**< (RTC_MR) Output toggles when alarm flag rises Position  */
#define RTC_MR_OUT1_ALARM_FLAG              (RTC_MR_OUT1_ALARM_FLAG_Val << RTC_MR_OUT1_Pos)  /**< (RTC_MR) Output is a copy of the alarm flag Position  */
#define RTC_MR_OUT1_PROG_PULSE              (RTC_MR_OUT1_PROG_PULSE_Val << RTC_MR_OUT1_Pos)  /**< (RTC_MR) Duty cycle programmable pulse Position  */
#define RTC_MR_THIGH_Pos                    24                                             /**< (RTC_MR) High Duration of the Output Pulse Position */
#define RTC_MR_THIGH_Msk                    (0x7U << RTC_MR_THIGH_Pos)                     /**< (RTC_MR) High Duration of the Output Pulse Mask */
#define RTC_MR_THIGH(value)                 (RTC_MR_THIGH_Msk & ((value) << RTC_MR_THIGH_Pos))
#define   RTC_MR_THIGH_H_31MS_Val           (0x0U)                                         /**< (RTC_MR) 31.2 ms  */
#define   RTC_MR_THIGH_H_16MS_Val           (0x1U)                                         /**< (RTC_MR) 15.6 ms  */
#define   RTC_MR_THIGH_H_4MS_Val            (0x2U)                                         /**< (RTC_MR) 3.91 ms  */
#define   RTC_MR_THIGH_H_976US_Val          (0x3U)                                         /**< (RTC_MR) 976 us  */
#define   RTC_MR_THIGH_H_488US_Val          (0x4U)                                         /**< (RTC_MR) 488 us  */
#define   RTC_MR_THIGH_H_122US_Val          (0x5U)                                         /**< (RTC_MR) 122 us  */
#define   RTC_MR_THIGH_H_30US_Val           (0x6U)                                         /**< (RTC_MR) 30.5 us  */
#define   RTC_MR_THIGH_H_15US_Val           (0x7U)                                         /**< (RTC_MR) 15.2 us  */
#define RTC_MR_THIGH_H_31MS                 (RTC_MR_THIGH_H_31MS_Val << RTC_MR_THIGH_Pos)  /**< (RTC_MR) 31.2 ms Position  */
#define RTC_MR_THIGH_H_16MS                 (RTC_MR_THIGH_H_16MS_Val << RTC_MR_THIGH_Pos)  /**< (RTC_MR) 15.6 ms Position  */
#define RTC_MR_THIGH_H_4MS                  (RTC_MR_THIGH_H_4MS_Val << RTC_MR_THIGH_Pos)   /**< (RTC_MR) 3.91 ms Position  */
#define RTC_MR_THIGH_H_976US                (RTC_MR_THIGH_H_976US_Val << RTC_MR_THIGH_Pos)  /**< (RTC_MR) 976 us Position  */
#define RTC_MR_THIGH_H_488US                (RTC_MR_THIGH_H_488US_Val << RTC_MR_THIGH_Pos)  /**< (RTC_MR) 488 us Position  */
#define RTC_MR_THIGH_H_122US                (RTC_MR_THIGH_H_122US_Val << RTC_MR_THIGH_Pos)  /**< (RTC_MR) 122 us Position  */
#define RTC_MR_THIGH_H_30US                 (RTC_MR_THIGH_H_30US_Val << RTC_MR_THIGH_Pos)  /**< (RTC_MR) 30.5 us Position  */
#define RTC_MR_THIGH_H_15US                 (RTC_MR_THIGH_H_15US_Val << RTC_MR_THIGH_Pos)  /**< (RTC_MR) 15.2 us Position  */
#define RTC_MR_TPERIOD_Pos                  28                                             /**< (RTC_MR) Period of the Output Pulse Position */
#define RTC_MR_TPERIOD_Msk                  (0x3U << RTC_MR_TPERIOD_Pos)                   /**< (RTC_MR) Period of the Output Pulse Mask */
#define RTC_MR_TPERIOD(value)               (RTC_MR_TPERIOD_Msk & ((value) << RTC_MR_TPERIOD_Pos))
#define   RTC_MR_TPERIOD_P_1S_Val           (0x0U)                                         /**< (RTC_MR) 1 second  */
#define   RTC_MR_TPERIOD_P_500MS_Val        (0x1U)                                         /**< (RTC_MR) 500 ms  */
#define   RTC_MR_TPERIOD_P_250MS_Val        (0x2U)                                         /**< (RTC_MR) 250 ms  */
#define   RTC_MR_TPERIOD_P_125MS_Val        (0x3U)                                         /**< (RTC_MR) 125 ms  */
#define RTC_MR_TPERIOD_P_1S                 (RTC_MR_TPERIOD_P_1S_Val << RTC_MR_TPERIOD_Pos)  /**< (RTC_MR) 1 second Position  */
#define RTC_MR_TPERIOD_P_500MS              (RTC_MR_TPERIOD_P_500MS_Val << RTC_MR_TPERIOD_Pos)  /**< (RTC_MR) 500 ms Position  */
#define RTC_MR_TPERIOD_P_250MS              (RTC_MR_TPERIOD_P_250MS_Val << RTC_MR_TPERIOD_Pos)  /**< (RTC_MR) 250 ms Position  */
#define RTC_MR_TPERIOD_P_125MS              (RTC_MR_TPERIOD_P_125MS_Val << RTC_MR_TPERIOD_Pos)  /**< (RTC_MR) 125 ms Position  */
#define RTC_MR_MASK                         (0x3777FF13U)                                  /**< \deprecated (RTC_MR) Register MASK  (Use RTC_MR_Msk instead)  */
#define RTC_MR_Msk                          (0x3777FF13U)                                  /**< (RTC_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_TIMR : (RTC Offset: 0x08) (R/W 32) Time Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SEC:7;                     /**< bit:   0..6  Current Second                           */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t MIN:7;                     /**< bit:  8..14  Current Minute                           */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t HOUR:6;                    /**< bit: 16..21  Current Hour                             */
    uint32_t AMPM:1;                    /**< bit:     22  Ante Meridiem Post Meridiem Indicator    */
    uint32_t :9;                        /**< bit: 23..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_TIMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_TIMR_OFFSET                     (0x08)                                        /**<  (RTC_TIMR) Time Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_TIMR_SEC_Pos                    0                                              /**< (RTC_TIMR) Current Second Position */
#define RTC_TIMR_SEC_Msk                    (0x7FU << RTC_TIMR_SEC_Pos)                    /**< (RTC_TIMR) Current Second Mask */
#define RTC_TIMR_SEC(value)                 (RTC_TIMR_SEC_Msk & ((value) << RTC_TIMR_SEC_Pos))
#define RTC_TIMR_MIN_Pos                    8                                              /**< (RTC_TIMR) Current Minute Position */
#define RTC_TIMR_MIN_Msk                    (0x7FU << RTC_TIMR_MIN_Pos)                    /**< (RTC_TIMR) Current Minute Mask */
#define RTC_TIMR_MIN(value)                 (RTC_TIMR_MIN_Msk & ((value) << RTC_TIMR_MIN_Pos))
#define RTC_TIMR_HOUR_Pos                   16                                             /**< (RTC_TIMR) Current Hour Position */
#define RTC_TIMR_HOUR_Msk                   (0x3FU << RTC_TIMR_HOUR_Pos)                   /**< (RTC_TIMR) Current Hour Mask */
#define RTC_TIMR_HOUR(value)                (RTC_TIMR_HOUR_Msk & ((value) << RTC_TIMR_HOUR_Pos))
#define RTC_TIMR_AMPM_Pos                   22                                             /**< (RTC_TIMR) Ante Meridiem Post Meridiem Indicator Position */
#define RTC_TIMR_AMPM_Msk                   (0x1U << RTC_TIMR_AMPM_Pos)                    /**< (RTC_TIMR) Ante Meridiem Post Meridiem Indicator Mask */
#define RTC_TIMR_AMPM                       RTC_TIMR_AMPM_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_TIMR_AMPM_Msk instead */
#define RTC_TIMR_MASK                       (0x7F7F7FU)                                    /**< \deprecated (RTC_TIMR) Register MASK  (Use RTC_TIMR_Msk instead)  */
#define RTC_TIMR_Msk                        (0x7F7F7FU)                                    /**< (RTC_TIMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_CALR : (RTC Offset: 0x0c) (R/W 32) Calendar Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CENT:7;                    /**< bit:   0..6  Current Century                          */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t YEAR:8;                    /**< bit:  8..15  Current Year                             */
    uint32_t MONTH:5;                   /**< bit: 16..20  Current Month                            */
    uint32_t DAY:3;                     /**< bit: 21..23  Current Day in Current Week              */
    uint32_t DATE:6;                    /**< bit: 24..29  Current Day in Current Month             */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_CALR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_CALR_OFFSET                     (0x0C)                                        /**<  (RTC_CALR) Calendar Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_CALR_CENT_Pos                   0                                              /**< (RTC_CALR) Current Century Position */
#define RTC_CALR_CENT_Msk                   (0x7FU << RTC_CALR_CENT_Pos)                   /**< (RTC_CALR) Current Century Mask */
#define RTC_CALR_CENT(value)                (RTC_CALR_CENT_Msk & ((value) << RTC_CALR_CENT_Pos))
#define RTC_CALR_YEAR_Pos                   8                                              /**< (RTC_CALR) Current Year Position */
#define RTC_CALR_YEAR_Msk                   (0xFFU << RTC_CALR_YEAR_Pos)                   /**< (RTC_CALR) Current Year Mask */
#define RTC_CALR_YEAR(value)                (RTC_CALR_YEAR_Msk & ((value) << RTC_CALR_YEAR_Pos))
#define RTC_CALR_MONTH_Pos                  16                                             /**< (RTC_CALR) Current Month Position */
#define RTC_CALR_MONTH_Msk                  (0x1FU << RTC_CALR_MONTH_Pos)                  /**< (RTC_CALR) Current Month Mask */
#define RTC_CALR_MONTH(value)               (RTC_CALR_MONTH_Msk & ((value) << RTC_CALR_MONTH_Pos))
#define RTC_CALR_DAY_Pos                    21                                             /**< (RTC_CALR) Current Day in Current Week Position */
#define RTC_CALR_DAY_Msk                    (0x7U << RTC_CALR_DAY_Pos)                     /**< (RTC_CALR) Current Day in Current Week Mask */
#define RTC_CALR_DAY(value)                 (RTC_CALR_DAY_Msk & ((value) << RTC_CALR_DAY_Pos))
#define RTC_CALR_DATE_Pos                   24                                             /**< (RTC_CALR) Current Day in Current Month Position */
#define RTC_CALR_DATE_Msk                   (0x3FU << RTC_CALR_DATE_Pos)                   /**< (RTC_CALR) Current Day in Current Month Mask */
#define RTC_CALR_DATE(value)                (RTC_CALR_DATE_Msk & ((value) << RTC_CALR_DATE_Pos))
#define RTC_CALR_MASK                       (0x3FFFFF7FU)                                  /**< \deprecated (RTC_CALR) Register MASK  (Use RTC_CALR_Msk instead)  */
#define RTC_CALR_Msk                        (0x3FFFFF7FU)                                  /**< (RTC_CALR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_TIMALR : (RTC Offset: 0x10) (R/W 32) Time Alarm Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SEC:7;                     /**< bit:   0..6  Second Alarm                             */
    uint32_t SECEN:1;                   /**< bit:      7  Second Alarm Enable                      */
    uint32_t MIN:7;                     /**< bit:  8..14  Minute Alarm                             */
    uint32_t MINEN:1;                   /**< bit:     15  Minute Alarm Enable                      */
    uint32_t HOUR:6;                    /**< bit: 16..21  Hour Alarm                               */
    uint32_t AMPM:1;                    /**< bit:     22  AM/PM Indicator                          */
    uint32_t HOUREN:1;                  /**< bit:     23  Hour Alarm Enable                        */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_TIMALR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_TIMALR_OFFSET                   (0x10)                                        /**<  (RTC_TIMALR) Time Alarm Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_TIMALR_SEC_Pos                  0                                              /**< (RTC_TIMALR) Second Alarm Position */
#define RTC_TIMALR_SEC_Msk                  (0x7FU << RTC_TIMALR_SEC_Pos)                  /**< (RTC_TIMALR) Second Alarm Mask */
#define RTC_TIMALR_SEC(value)               (RTC_TIMALR_SEC_Msk & ((value) << RTC_TIMALR_SEC_Pos))
#define RTC_TIMALR_SECEN_Pos                7                                              /**< (RTC_TIMALR) Second Alarm Enable Position */
#define RTC_TIMALR_SECEN_Msk                (0x1U << RTC_TIMALR_SECEN_Pos)                 /**< (RTC_TIMALR) Second Alarm Enable Mask */
#define RTC_TIMALR_SECEN                    RTC_TIMALR_SECEN_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_TIMALR_SECEN_Msk instead */
#define RTC_TIMALR_MIN_Pos                  8                                              /**< (RTC_TIMALR) Minute Alarm Position */
#define RTC_TIMALR_MIN_Msk                  (0x7FU << RTC_TIMALR_MIN_Pos)                  /**< (RTC_TIMALR) Minute Alarm Mask */
#define RTC_TIMALR_MIN(value)               (RTC_TIMALR_MIN_Msk & ((value) << RTC_TIMALR_MIN_Pos))
#define RTC_TIMALR_MINEN_Pos                15                                             /**< (RTC_TIMALR) Minute Alarm Enable Position */
#define RTC_TIMALR_MINEN_Msk                (0x1U << RTC_TIMALR_MINEN_Pos)                 /**< (RTC_TIMALR) Minute Alarm Enable Mask */
#define RTC_TIMALR_MINEN                    RTC_TIMALR_MINEN_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_TIMALR_MINEN_Msk instead */
#define RTC_TIMALR_HOUR_Pos                 16                                             /**< (RTC_TIMALR) Hour Alarm Position */
#define RTC_TIMALR_HOUR_Msk                 (0x3FU << RTC_TIMALR_HOUR_Pos)                 /**< (RTC_TIMALR) Hour Alarm Mask */
#define RTC_TIMALR_HOUR(value)              (RTC_TIMALR_HOUR_Msk & ((value) << RTC_TIMALR_HOUR_Pos))
#define RTC_TIMALR_AMPM_Pos                 22                                             /**< (RTC_TIMALR) AM/PM Indicator Position */
#define RTC_TIMALR_AMPM_Msk                 (0x1U << RTC_TIMALR_AMPM_Pos)                  /**< (RTC_TIMALR) AM/PM Indicator Mask */
#define RTC_TIMALR_AMPM                     RTC_TIMALR_AMPM_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_TIMALR_AMPM_Msk instead */
#define RTC_TIMALR_HOUREN_Pos               23                                             /**< (RTC_TIMALR) Hour Alarm Enable Position */
#define RTC_TIMALR_HOUREN_Msk               (0x1U << RTC_TIMALR_HOUREN_Pos)                /**< (RTC_TIMALR) Hour Alarm Enable Mask */
#define RTC_TIMALR_HOUREN                   RTC_TIMALR_HOUREN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_TIMALR_HOUREN_Msk instead */
#define RTC_TIMALR_MASK                     (0xFFFFFFU)                                    /**< \deprecated (RTC_TIMALR) Register MASK  (Use RTC_TIMALR_Msk instead)  */
#define RTC_TIMALR_Msk                      (0xFFFFFFU)                                    /**< (RTC_TIMALR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_CALALR : (RTC Offset: 0x14) (R/W 32) Calendar Alarm Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :16;                       /**< bit:  0..15  Reserved */
    uint32_t MONTH:5;                   /**< bit: 16..20  Month Alarm                              */
    uint32_t :2;                        /**< bit: 21..22  Reserved */
    uint32_t MTHEN:1;                   /**< bit:     23  Month Alarm Enable                       */
    uint32_t DATE:6;                    /**< bit: 24..29  Date Alarm                               */
    uint32_t :1;                        /**< bit:     30  Reserved */
    uint32_t DATEEN:1;                  /**< bit:     31  Date Alarm Enable                        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_CALALR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_CALALR_OFFSET                   (0x14)                                        /**<  (RTC_CALALR) Calendar Alarm Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_CALALR_MONTH_Pos                16                                             /**< (RTC_CALALR) Month Alarm Position */
#define RTC_CALALR_MONTH_Msk                (0x1FU << RTC_CALALR_MONTH_Pos)                /**< (RTC_CALALR) Month Alarm Mask */
#define RTC_CALALR_MONTH(value)             (RTC_CALALR_MONTH_Msk & ((value) << RTC_CALALR_MONTH_Pos))
#define RTC_CALALR_MTHEN_Pos                23                                             /**< (RTC_CALALR) Month Alarm Enable Position */
#define RTC_CALALR_MTHEN_Msk                (0x1U << RTC_CALALR_MTHEN_Pos)                 /**< (RTC_CALALR) Month Alarm Enable Mask */
#define RTC_CALALR_MTHEN                    RTC_CALALR_MTHEN_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_CALALR_MTHEN_Msk instead */
#define RTC_CALALR_DATE_Pos                 24                                             /**< (RTC_CALALR) Date Alarm Position */
#define RTC_CALALR_DATE_Msk                 (0x3FU << RTC_CALALR_DATE_Pos)                 /**< (RTC_CALALR) Date Alarm Mask */
#define RTC_CALALR_DATE(value)              (RTC_CALALR_DATE_Msk & ((value) << RTC_CALALR_DATE_Pos))
#define RTC_CALALR_DATEEN_Pos               31                                             /**< (RTC_CALALR) Date Alarm Enable Position */
#define RTC_CALALR_DATEEN_Msk               (0x1U << RTC_CALALR_DATEEN_Pos)                /**< (RTC_CALALR) Date Alarm Enable Mask */
#define RTC_CALALR_DATEEN                   RTC_CALALR_DATEEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_CALALR_DATEEN_Msk instead */
#define RTC_CALALR_MASK                     (0xBF9F0000U)                                  /**< \deprecated (RTC_CALALR) Register MASK  (Use RTC_CALALR_Msk instead)  */
#define RTC_CALALR_Msk                      (0xBF9F0000U)                                  /**< (RTC_CALALR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_SR : (RTC Offset: 0x18) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ACKUPD:1;                  /**< bit:      0  Acknowledge for Update                   */
    uint32_t ALARM:1;                   /**< bit:      1  Alarm Flag                               */
    uint32_t SEC:1;                     /**< bit:      2  Second Event                             */
    uint32_t TIMEV:1;                   /**< bit:      3  Time Event                               */
    uint32_t CALEV:1;                   /**< bit:      4  Calendar Event                           */
    uint32_t TDERR:1;                   /**< bit:      5  Time and/or Date Free Running Error      */
    uint32_t :26;                       /**< bit:  6..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_SR_OFFSET                       (0x18)                                        /**<  (RTC_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_SR_ACKUPD_Pos                   0                                              /**< (RTC_SR) Acknowledge for Update Position */
#define RTC_SR_ACKUPD_Msk                   (0x1U << RTC_SR_ACKUPD_Pos)                    /**< (RTC_SR) Acknowledge for Update Mask */
#define RTC_SR_ACKUPD                       RTC_SR_ACKUPD_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SR_ACKUPD_Msk instead */
#define   RTC_SR_ACKUPD_FREERUN_Val         (0x0U)                                         /**< (RTC_SR) Time and calendar registers cannot be updated.  */
#define   RTC_SR_ACKUPD_UPDATE_Val          (0x1U)                                         /**< (RTC_SR) Time and calendar registers can be updated.  */
#define RTC_SR_ACKUPD_FREERUN               (RTC_SR_ACKUPD_FREERUN_Val << RTC_SR_ACKUPD_Pos)  /**< (RTC_SR) Time and calendar registers cannot be updated. Position  */
#define RTC_SR_ACKUPD_UPDATE                (RTC_SR_ACKUPD_UPDATE_Val << RTC_SR_ACKUPD_Pos)  /**< (RTC_SR) Time and calendar registers can be updated. Position  */
#define RTC_SR_ALARM_Pos                    1                                              /**< (RTC_SR) Alarm Flag Position */
#define RTC_SR_ALARM_Msk                    (0x1U << RTC_SR_ALARM_Pos)                     /**< (RTC_SR) Alarm Flag Mask */
#define RTC_SR_ALARM                        RTC_SR_ALARM_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SR_ALARM_Msk instead */
#define   RTC_SR_ALARM_NO_ALARMEVENT_Val    (0x0U)                                         /**< (RTC_SR) No alarm matching condition occurred.  */
#define   RTC_SR_ALARM_ALARMEVENT_Val       (0x1U)                                         /**< (RTC_SR) An alarm matching condition has occurred.  */
#define RTC_SR_ALARM_NO_ALARMEVENT          (RTC_SR_ALARM_NO_ALARMEVENT_Val << RTC_SR_ALARM_Pos)  /**< (RTC_SR) No alarm matching condition occurred. Position  */
#define RTC_SR_ALARM_ALARMEVENT             (RTC_SR_ALARM_ALARMEVENT_Val << RTC_SR_ALARM_Pos)  /**< (RTC_SR) An alarm matching condition has occurred. Position  */
#define RTC_SR_SEC_Pos                      2                                              /**< (RTC_SR) Second Event Position */
#define RTC_SR_SEC_Msk                      (0x1U << RTC_SR_SEC_Pos)                       /**< (RTC_SR) Second Event Mask */
#define RTC_SR_SEC                          RTC_SR_SEC_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SR_SEC_Msk instead */
#define   RTC_SR_SEC_NO_SECEVENT_Val        (0x0U)                                         /**< (RTC_SR) No second event has occurred since the last clear.  */
#define   RTC_SR_SEC_SECEVENT_Val           (0x1U)                                         /**< (RTC_SR) At least one second event has occurred since the last clear.  */
#define RTC_SR_SEC_NO_SECEVENT              (RTC_SR_SEC_NO_SECEVENT_Val << RTC_SR_SEC_Pos)  /**< (RTC_SR) No second event has occurred since the last clear. Position  */
#define RTC_SR_SEC_SECEVENT                 (RTC_SR_SEC_SECEVENT_Val << RTC_SR_SEC_Pos)    /**< (RTC_SR) At least one second event has occurred since the last clear. Position  */
#define RTC_SR_TIMEV_Pos                    3                                              /**< (RTC_SR) Time Event Position */
#define RTC_SR_TIMEV_Msk                    (0x1U << RTC_SR_TIMEV_Pos)                     /**< (RTC_SR) Time Event Mask */
#define RTC_SR_TIMEV                        RTC_SR_TIMEV_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SR_TIMEV_Msk instead */
#define   RTC_SR_TIMEV_NO_TIMEVENT_Val      (0x0U)                                         /**< (RTC_SR) No time event has occurred since the last clear.  */
#define   RTC_SR_TIMEV_TIMEVENT_Val         (0x1U)                                         /**< (RTC_SR) At least one time event has occurred since the last clear.  */
#define RTC_SR_TIMEV_NO_TIMEVENT            (RTC_SR_TIMEV_NO_TIMEVENT_Val << RTC_SR_TIMEV_Pos)  /**< (RTC_SR) No time event has occurred since the last clear. Position  */
#define RTC_SR_TIMEV_TIMEVENT               (RTC_SR_TIMEV_TIMEVENT_Val << RTC_SR_TIMEV_Pos)  /**< (RTC_SR) At least one time event has occurred since the last clear. Position  */
#define RTC_SR_CALEV_Pos                    4                                              /**< (RTC_SR) Calendar Event Position */
#define RTC_SR_CALEV_Msk                    (0x1U << RTC_SR_CALEV_Pos)                     /**< (RTC_SR) Calendar Event Mask */
#define RTC_SR_CALEV                        RTC_SR_CALEV_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SR_CALEV_Msk instead */
#define   RTC_SR_CALEV_NO_CALEVENT_Val      (0x0U)                                         /**< (RTC_SR) No calendar event has occurred since the last clear.  */
#define   RTC_SR_CALEV_CALEVENT_Val         (0x1U)                                         /**< (RTC_SR) At least one calendar event has occurred since the last clear.  */
#define RTC_SR_CALEV_NO_CALEVENT            (RTC_SR_CALEV_NO_CALEVENT_Val << RTC_SR_CALEV_Pos)  /**< (RTC_SR) No calendar event has occurred since the last clear. Position  */
#define RTC_SR_CALEV_CALEVENT               (RTC_SR_CALEV_CALEVENT_Val << RTC_SR_CALEV_Pos)  /**< (RTC_SR) At least one calendar event has occurred since the last clear. Position  */
#define RTC_SR_TDERR_Pos                    5                                              /**< (RTC_SR) Time and/or Date Free Running Error Position */
#define RTC_SR_TDERR_Msk                    (0x1U << RTC_SR_TDERR_Pos)                     /**< (RTC_SR) Time and/or Date Free Running Error Mask */
#define RTC_SR_TDERR                        RTC_SR_TDERR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SR_TDERR_Msk instead */
#define   RTC_SR_TDERR_CORRECT_Val          (0x0U)                                         /**< (RTC_SR) The internal free running counters are carrying valid values since the last read of the Status Register (RTC_SR).  */
#define   RTC_SR_TDERR_ERR_TIMEDATE_Val     (0x1U)                                         /**< (RTC_SR) The internal free running counters have been corrupted (invalid date or time, non-BCD values) since the last read and/or they are still invalid.  */
#define RTC_SR_TDERR_CORRECT                (RTC_SR_TDERR_CORRECT_Val << RTC_SR_TDERR_Pos)  /**< (RTC_SR) The internal free running counters are carrying valid values since the last read of the Status Register (RTC_SR). Position  */
#define RTC_SR_TDERR_ERR_TIMEDATE           (RTC_SR_TDERR_ERR_TIMEDATE_Val << RTC_SR_TDERR_Pos)  /**< (RTC_SR) The internal free running counters have been corrupted (invalid date or time, non-BCD values) since the last read and/or they are still invalid. Position  */
#define RTC_SR_MASK                         (0x3FU)                                        /**< \deprecated (RTC_SR) Register MASK  (Use RTC_SR_Msk instead)  */
#define RTC_SR_Msk                          (0x3FU)                                        /**< (RTC_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_SCCR : (RTC Offset: 0x1c) (/W 32) Status Clear Command Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ACKCLR:1;                  /**< bit:      0  Acknowledge Clear                        */
    uint32_t ALRCLR:1;                  /**< bit:      1  Alarm Clear                              */
    uint32_t SECCLR:1;                  /**< bit:      2  Second Clear                             */
    uint32_t TIMCLR:1;                  /**< bit:      3  Time Clear                               */
    uint32_t CALCLR:1;                  /**< bit:      4  Calendar Clear                           */
    uint32_t TDERRCLR:1;                /**< bit:      5  Time and/or Date Free Running Error Clear */
    uint32_t :26;                       /**< bit:  6..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_SCCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_SCCR_OFFSET                     (0x1C)                                        /**<  (RTC_SCCR) Status Clear Command Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_SCCR_ACKCLR_Pos                 0                                              /**< (RTC_SCCR) Acknowledge Clear Position */
#define RTC_SCCR_ACKCLR_Msk                 (0x1U << RTC_SCCR_ACKCLR_Pos)                  /**< (RTC_SCCR) Acknowledge Clear Mask */
#define RTC_SCCR_ACKCLR                     RTC_SCCR_ACKCLR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SCCR_ACKCLR_Msk instead */
#define RTC_SCCR_ALRCLR_Pos                 1                                              /**< (RTC_SCCR) Alarm Clear Position */
#define RTC_SCCR_ALRCLR_Msk                 (0x1U << RTC_SCCR_ALRCLR_Pos)                  /**< (RTC_SCCR) Alarm Clear Mask */
#define RTC_SCCR_ALRCLR                     RTC_SCCR_ALRCLR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SCCR_ALRCLR_Msk instead */
#define RTC_SCCR_SECCLR_Pos                 2                                              /**< (RTC_SCCR) Second Clear Position */
#define RTC_SCCR_SECCLR_Msk                 (0x1U << RTC_SCCR_SECCLR_Pos)                  /**< (RTC_SCCR) Second Clear Mask */
#define RTC_SCCR_SECCLR                     RTC_SCCR_SECCLR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SCCR_SECCLR_Msk instead */
#define RTC_SCCR_TIMCLR_Pos                 3                                              /**< (RTC_SCCR) Time Clear Position */
#define RTC_SCCR_TIMCLR_Msk                 (0x1U << RTC_SCCR_TIMCLR_Pos)                  /**< (RTC_SCCR) Time Clear Mask */
#define RTC_SCCR_TIMCLR                     RTC_SCCR_TIMCLR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SCCR_TIMCLR_Msk instead */
#define RTC_SCCR_CALCLR_Pos                 4                                              /**< (RTC_SCCR) Calendar Clear Position */
#define RTC_SCCR_CALCLR_Msk                 (0x1U << RTC_SCCR_CALCLR_Pos)                  /**< (RTC_SCCR) Calendar Clear Mask */
#define RTC_SCCR_CALCLR                     RTC_SCCR_CALCLR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SCCR_CALCLR_Msk instead */
#define RTC_SCCR_TDERRCLR_Pos               5                                              /**< (RTC_SCCR) Time and/or Date Free Running Error Clear Position */
#define RTC_SCCR_TDERRCLR_Msk               (0x1U << RTC_SCCR_TDERRCLR_Pos)                /**< (RTC_SCCR) Time and/or Date Free Running Error Clear Mask */
#define RTC_SCCR_TDERRCLR                   RTC_SCCR_TDERRCLR_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_SCCR_TDERRCLR_Msk instead */
#define RTC_SCCR_MASK                       (0x3FU)                                        /**< \deprecated (RTC_SCCR) Register MASK  (Use RTC_SCCR_Msk instead)  */
#define RTC_SCCR_Msk                        (0x3FU)                                        /**< (RTC_SCCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_IER : (RTC Offset: 0x20) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ACKEN:1;                   /**< bit:      0  Acknowledge Update Interrupt Enable      */
    uint32_t ALREN:1;                   /**< bit:      1  Alarm Interrupt Enable                   */
    uint32_t SECEN:1;                   /**< bit:      2  Second Event Interrupt Enable            */
    uint32_t TIMEN:1;                   /**< bit:      3  Time Event Interrupt Enable              */
    uint32_t CALEN:1;                   /**< bit:      4  Calendar Event Interrupt Enable          */
    uint32_t TDERREN:1;                 /**< bit:      5  Time and/or Date Error Interrupt Enable  */
    uint32_t :26;                       /**< bit:  6..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_IER_OFFSET                      (0x20)                                        /**<  (RTC_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_IER_ACKEN_Pos                   0                                              /**< (RTC_IER) Acknowledge Update Interrupt Enable Position */
#define RTC_IER_ACKEN_Msk                   (0x1U << RTC_IER_ACKEN_Pos)                    /**< (RTC_IER) Acknowledge Update Interrupt Enable Mask */
#define RTC_IER_ACKEN                       RTC_IER_ACKEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IER_ACKEN_Msk instead */
#define RTC_IER_ALREN_Pos                   1                                              /**< (RTC_IER) Alarm Interrupt Enable Position */
#define RTC_IER_ALREN_Msk                   (0x1U << RTC_IER_ALREN_Pos)                    /**< (RTC_IER) Alarm Interrupt Enable Mask */
#define RTC_IER_ALREN                       RTC_IER_ALREN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IER_ALREN_Msk instead */
#define RTC_IER_SECEN_Pos                   2                                              /**< (RTC_IER) Second Event Interrupt Enable Position */
#define RTC_IER_SECEN_Msk                   (0x1U << RTC_IER_SECEN_Pos)                    /**< (RTC_IER) Second Event Interrupt Enable Mask */
#define RTC_IER_SECEN                       RTC_IER_SECEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IER_SECEN_Msk instead */
#define RTC_IER_TIMEN_Pos                   3                                              /**< (RTC_IER) Time Event Interrupt Enable Position */
#define RTC_IER_TIMEN_Msk                   (0x1U << RTC_IER_TIMEN_Pos)                    /**< (RTC_IER) Time Event Interrupt Enable Mask */
#define RTC_IER_TIMEN                       RTC_IER_TIMEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IER_TIMEN_Msk instead */
#define RTC_IER_CALEN_Pos                   4                                              /**< (RTC_IER) Calendar Event Interrupt Enable Position */
#define RTC_IER_CALEN_Msk                   (0x1U << RTC_IER_CALEN_Pos)                    /**< (RTC_IER) Calendar Event Interrupt Enable Mask */
#define RTC_IER_CALEN                       RTC_IER_CALEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IER_CALEN_Msk instead */
#define RTC_IER_TDERREN_Pos                 5                                              /**< (RTC_IER) Time and/or Date Error Interrupt Enable Position */
#define RTC_IER_TDERREN_Msk                 (0x1U << RTC_IER_TDERREN_Pos)                  /**< (RTC_IER) Time and/or Date Error Interrupt Enable Mask */
#define RTC_IER_TDERREN                     RTC_IER_TDERREN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IER_TDERREN_Msk instead */
#define RTC_IER_MASK                        (0x3FU)                                        /**< \deprecated (RTC_IER) Register MASK  (Use RTC_IER_Msk instead)  */
#define RTC_IER_Msk                         (0x3FU)                                        /**< (RTC_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_IDR : (RTC Offset: 0x24) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ACKDIS:1;                  /**< bit:      0  Acknowledge Update Interrupt Disable     */
    uint32_t ALRDIS:1;                  /**< bit:      1  Alarm Interrupt Disable                  */
    uint32_t SECDIS:1;                  /**< bit:      2  Second Event Interrupt Disable           */
    uint32_t TIMDIS:1;                  /**< bit:      3  Time Event Interrupt Disable             */
    uint32_t CALDIS:1;                  /**< bit:      4  Calendar Event Interrupt Disable         */
    uint32_t TDERRDIS:1;                /**< bit:      5  Time and/or Date Error Interrupt Disable */
    uint32_t :26;                       /**< bit:  6..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_IDR_OFFSET                      (0x24)                                        /**<  (RTC_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_IDR_ACKDIS_Pos                  0                                              /**< (RTC_IDR) Acknowledge Update Interrupt Disable Position */
#define RTC_IDR_ACKDIS_Msk                  (0x1U << RTC_IDR_ACKDIS_Pos)                   /**< (RTC_IDR) Acknowledge Update Interrupt Disable Mask */
#define RTC_IDR_ACKDIS                      RTC_IDR_ACKDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IDR_ACKDIS_Msk instead */
#define RTC_IDR_ALRDIS_Pos                  1                                              /**< (RTC_IDR) Alarm Interrupt Disable Position */
#define RTC_IDR_ALRDIS_Msk                  (0x1U << RTC_IDR_ALRDIS_Pos)                   /**< (RTC_IDR) Alarm Interrupt Disable Mask */
#define RTC_IDR_ALRDIS                      RTC_IDR_ALRDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IDR_ALRDIS_Msk instead */
#define RTC_IDR_SECDIS_Pos                  2                                              /**< (RTC_IDR) Second Event Interrupt Disable Position */
#define RTC_IDR_SECDIS_Msk                  (0x1U << RTC_IDR_SECDIS_Pos)                   /**< (RTC_IDR) Second Event Interrupt Disable Mask */
#define RTC_IDR_SECDIS                      RTC_IDR_SECDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IDR_SECDIS_Msk instead */
#define RTC_IDR_TIMDIS_Pos                  3                                              /**< (RTC_IDR) Time Event Interrupt Disable Position */
#define RTC_IDR_TIMDIS_Msk                  (0x1U << RTC_IDR_TIMDIS_Pos)                   /**< (RTC_IDR) Time Event Interrupt Disable Mask */
#define RTC_IDR_TIMDIS                      RTC_IDR_TIMDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IDR_TIMDIS_Msk instead */
#define RTC_IDR_CALDIS_Pos                  4                                              /**< (RTC_IDR) Calendar Event Interrupt Disable Position */
#define RTC_IDR_CALDIS_Msk                  (0x1U << RTC_IDR_CALDIS_Pos)                   /**< (RTC_IDR) Calendar Event Interrupt Disable Mask */
#define RTC_IDR_CALDIS                      RTC_IDR_CALDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IDR_CALDIS_Msk instead */
#define RTC_IDR_TDERRDIS_Pos                5                                              /**< (RTC_IDR) Time and/or Date Error Interrupt Disable Position */
#define RTC_IDR_TDERRDIS_Msk                (0x1U << RTC_IDR_TDERRDIS_Pos)                 /**< (RTC_IDR) Time and/or Date Error Interrupt Disable Mask */
#define RTC_IDR_TDERRDIS                    RTC_IDR_TDERRDIS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IDR_TDERRDIS_Msk instead */
#define RTC_IDR_MASK                        (0x3FU)                                        /**< \deprecated (RTC_IDR) Register MASK  (Use RTC_IDR_Msk instead)  */
#define RTC_IDR_Msk                         (0x3FU)                                        /**< (RTC_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_IMR : (RTC Offset: 0x28) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ACK:1;                     /**< bit:      0  Acknowledge Update Interrupt Mask        */
    uint32_t ALR:1;                     /**< bit:      1  Alarm Interrupt Mask                     */
    uint32_t SEC:1;                     /**< bit:      2  Second Event Interrupt Mask              */
    uint32_t TIM:1;                     /**< bit:      3  Time Event Interrupt Mask                */
    uint32_t CAL:1;                     /**< bit:      4  Calendar Event Interrupt Mask            */
    uint32_t TDERR:1;                   /**< bit:      5  Time and/or Date Error Mask              */
    uint32_t :26;                       /**< bit:  6..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_IMR_OFFSET                      (0x28)                                        /**<  (RTC_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_IMR_ACK_Pos                     0                                              /**< (RTC_IMR) Acknowledge Update Interrupt Mask Position */
#define RTC_IMR_ACK_Msk                     (0x1U << RTC_IMR_ACK_Pos)                      /**< (RTC_IMR) Acknowledge Update Interrupt Mask Mask */
#define RTC_IMR_ACK                         RTC_IMR_ACK_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IMR_ACK_Msk instead */
#define RTC_IMR_ALR_Pos                     1                                              /**< (RTC_IMR) Alarm Interrupt Mask Position */
#define RTC_IMR_ALR_Msk                     (0x1U << RTC_IMR_ALR_Pos)                      /**< (RTC_IMR) Alarm Interrupt Mask Mask */
#define RTC_IMR_ALR                         RTC_IMR_ALR_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IMR_ALR_Msk instead */
#define RTC_IMR_SEC_Pos                     2                                              /**< (RTC_IMR) Second Event Interrupt Mask Position */
#define RTC_IMR_SEC_Msk                     (0x1U << RTC_IMR_SEC_Pos)                      /**< (RTC_IMR) Second Event Interrupt Mask Mask */
#define RTC_IMR_SEC                         RTC_IMR_SEC_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IMR_SEC_Msk instead */
#define RTC_IMR_TIM_Pos                     3                                              /**< (RTC_IMR) Time Event Interrupt Mask Position */
#define RTC_IMR_TIM_Msk                     (0x1U << RTC_IMR_TIM_Pos)                      /**< (RTC_IMR) Time Event Interrupt Mask Mask */
#define RTC_IMR_TIM                         RTC_IMR_TIM_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IMR_TIM_Msk instead */
#define RTC_IMR_CAL_Pos                     4                                              /**< (RTC_IMR) Calendar Event Interrupt Mask Position */
#define RTC_IMR_CAL_Msk                     (0x1U << RTC_IMR_CAL_Pos)                      /**< (RTC_IMR) Calendar Event Interrupt Mask Mask */
#define RTC_IMR_CAL                         RTC_IMR_CAL_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IMR_CAL_Msk instead */
#define RTC_IMR_TDERR_Pos                   5                                              /**< (RTC_IMR) Time and/or Date Error Mask Position */
#define RTC_IMR_TDERR_Msk                   (0x1U << RTC_IMR_TDERR_Pos)                    /**< (RTC_IMR) Time and/or Date Error Mask Mask */
#define RTC_IMR_TDERR                       RTC_IMR_TDERR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_IMR_TDERR_Msk instead */
#define RTC_IMR_MASK                        (0x3FU)                                        /**< \deprecated (RTC_IMR) Register MASK  (Use RTC_IMR_Msk instead)  */
#define RTC_IMR_Msk                         (0x3FU)                                        /**< (RTC_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_VER : (RTC Offset: 0x2c) (R/ 32) Valid Entry Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NVTIM:1;                   /**< bit:      0  Non-valid Time                           */
    uint32_t NVCAL:1;                   /**< bit:      1  Non-valid Calendar                       */
    uint32_t NVTIMALR:1;                /**< bit:      2  Non-valid Time Alarm                     */
    uint32_t NVCALALR:1;                /**< bit:      3  Non-valid Calendar Alarm                 */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_VER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_VER_OFFSET                      (0x2C)                                        /**<  (RTC_VER) Valid Entry Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_VER_NVTIM_Pos                   0                                              /**< (RTC_VER) Non-valid Time Position */
#define RTC_VER_NVTIM_Msk                   (0x1U << RTC_VER_NVTIM_Pos)                    /**< (RTC_VER) Non-valid Time Mask */
#define RTC_VER_NVTIM                       RTC_VER_NVTIM_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_VER_NVTIM_Msk instead */
#define RTC_VER_NVCAL_Pos                   1                                              /**< (RTC_VER) Non-valid Calendar Position */
#define RTC_VER_NVCAL_Msk                   (0x1U << RTC_VER_NVCAL_Pos)                    /**< (RTC_VER) Non-valid Calendar Mask */
#define RTC_VER_NVCAL                       RTC_VER_NVCAL_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_VER_NVCAL_Msk instead */
#define RTC_VER_NVTIMALR_Pos                2                                              /**< (RTC_VER) Non-valid Time Alarm Position */
#define RTC_VER_NVTIMALR_Msk                (0x1U << RTC_VER_NVTIMALR_Pos)                 /**< (RTC_VER) Non-valid Time Alarm Mask */
#define RTC_VER_NVTIMALR                    RTC_VER_NVTIMALR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_VER_NVTIMALR_Msk instead */
#define RTC_VER_NVCALALR_Pos                3                                              /**< (RTC_VER) Non-valid Calendar Alarm Position */
#define RTC_VER_NVCALALR_Msk                (0x1U << RTC_VER_NVCALALR_Pos)                 /**< (RTC_VER) Non-valid Calendar Alarm Mask */
#define RTC_VER_NVCALALR                    RTC_VER_NVCALALR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_VER_NVCALALR_Msk instead */
#define RTC_VER_MASK                        (0x0FU)                                        /**< \deprecated (RTC_VER) Register MASK  (Use RTC_VER_Msk instead)  */
#define RTC_VER_Msk                         (0x0FU)                                        /**< (RTC_VER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RTC_WPMR : (RTC Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RTC_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_WPMR_OFFSET                     (0xE4)                                        /**<  (RTC_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RTC_WPMR_WPEN_Pos                   0                                              /**< (RTC_WPMR) Write Protection Enable Position */
#define RTC_WPMR_WPEN_Msk                   (0x1U << RTC_WPMR_WPEN_Pos)                    /**< (RTC_WPMR) Write Protection Enable Mask */
#define RTC_WPMR_WPEN                       RTC_WPMR_WPEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RTC_WPMR_WPEN_Msk instead */
#define RTC_WPMR_WPKEY_Pos                  8                                              /**< (RTC_WPMR) Write Protection Key Position */
#define RTC_WPMR_WPKEY_Msk                  (0xFFFFFFU << RTC_WPMR_WPKEY_Pos)              /**< (RTC_WPMR) Write Protection Key Mask */
#define RTC_WPMR_WPKEY(value)               (RTC_WPMR_WPKEY_Msk & ((value) << RTC_WPMR_WPKEY_Pos))
#define   RTC_WPMR_WPKEY_PASSWD_Val         (0x525443U)                                    /**< (RTC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0.  */
#define RTC_WPMR_WPKEY_PASSWD               (RTC_WPMR_WPKEY_PASSWD_Val << RTC_WPMR_WPKEY_Pos)  /**< (RTC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. Position  */
#define RTC_WPMR_MASK                       (0xFFFFFF01U)                                  /**< \deprecated (RTC_WPMR) Register MASK  (Use RTC_WPMR_Msk instead)  */
#define RTC_WPMR_Msk                        (0xFFFFFF01U)                                  /**< (RTC_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief RTC hardware registers */
typedef struct {  
  __IO uint32_t RTC_CR;         /**< (RTC Offset: 0x00) Control Register */
  __IO uint32_t RTC_MR;         /**< (RTC Offset: 0x04) Mode Register */
  __IO uint32_t RTC_TIMR;       /**< (RTC Offset: 0x08) Time Register */
  __IO uint32_t RTC_CALR;       /**< (RTC Offset: 0x0C) Calendar Register */
  __IO uint32_t RTC_TIMALR;     /**< (RTC Offset: 0x10) Time Alarm Register */
  __IO uint32_t RTC_CALALR;     /**< (RTC Offset: 0x14) Calendar Alarm Register */
  __I  uint32_t RTC_SR;         /**< (RTC Offset: 0x18) Status Register */
  __O  uint32_t RTC_SCCR;       /**< (RTC Offset: 0x1C) Status Clear Command Register */
  __O  uint32_t RTC_IER;        /**< (RTC Offset: 0x20) Interrupt Enable Register */
  __O  uint32_t RTC_IDR;        /**< (RTC Offset: 0x24) Interrupt Disable Register */
  __I  uint32_t RTC_IMR;        /**< (RTC Offset: 0x28) Interrupt Mask Register */
  __I  uint32_t RTC_VER;        /**< (RTC Offset: 0x2C) Valid Entry Register */
  __I  uint32_t Reserved1[45];
  __IO uint32_t RTC_WPMR;       /**< (RTC Offset: 0xE4) Write Protection Mode Register */
} Rtc;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief RTC hardware registers */
typedef struct {  
  __IO RTC_CR_Type                    RTC_CR;         /**< Offset: 0x00 (R/W  32) Control Register */
  __IO RTC_MR_Type                    RTC_MR;         /**< Offset: 0x04 (R/W  32) Mode Register */
  __IO RTC_TIMR_Type                  RTC_TIMR;       /**< Offset: 0x08 (R/W  32) Time Register */
  __IO RTC_CALR_Type                  RTC_CALR;       /**< Offset: 0x0C (R/W  32) Calendar Register */
  __IO RTC_TIMALR_Type                RTC_TIMALR;     /**< Offset: 0x10 (R/W  32) Time Alarm Register */
  __IO RTC_CALALR_Type                RTC_CALALR;     /**< Offset: 0x14 (R/W  32) Calendar Alarm Register */
  __I  RTC_SR_Type                    RTC_SR;         /**< Offset: 0x18 (R/   32) Status Register */
  __O  RTC_SCCR_Type                  RTC_SCCR;       /**< Offset: 0x1C ( /W  32) Status Clear Command Register */
  __O  RTC_IER_Type                   RTC_IER;        /**< Offset: 0x20 ( /W  32) Interrupt Enable Register */
  __O  RTC_IDR_Type                   RTC_IDR;        /**< Offset: 0x24 ( /W  32) Interrupt Disable Register */
  __I  RTC_IMR_Type                   RTC_IMR;        /**< Offset: 0x28 (R/   32) Interrupt Mask Register */
  __I  RTC_VER_Type                   RTC_VER;        /**< Offset: 0x2C (R/   32) Valid Entry Register */
       RoReg8                         Reserved1[0xB4];
  __IO RTC_WPMR_Type                  RTC_WPMR;       /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
} Rtc;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Real-time Clock */

#endif /* _SAME70_RTC_COMPONENT_H_ */
