/**
 * \file
 *
 * \brief Component description for AFEC
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

#ifndef _SAME70_AFEC_COMPONENT_H_
#define _SAME70_AFEC_COMPONENT_H_
#define _SAME70_AFEC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_AFEC Analog Front-End Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR AFEC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define AFEC_11147                      /**< (AFEC) Module ID */
#define REV_AFEC M                      /**< (AFEC) Module revision */

/* -------- AFEC_CR : (AFEC Offset: 0x00) (/W 32) AFEC Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SWRST:1;                   /**< bit:      0  Software Reset                           */
    uint32_t START:1;                   /**< bit:      1  Start Conversion                         */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CR_OFFSET                      (0x00)                                        /**<  (AFEC_CR) AFEC Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CR_SWRST_Pos                   0                                              /**< (AFEC_CR) Software Reset Position */
#define AFEC_CR_SWRST_Msk                   (0x1U << AFEC_CR_SWRST_Pos)                    /**< (AFEC_CR) Software Reset Mask */
#define AFEC_CR_SWRST                       AFEC_CR_SWRST_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CR_SWRST_Msk instead */
#define AFEC_CR_START_Pos                   1                                              /**< (AFEC_CR) Start Conversion Position */
#define AFEC_CR_START_Msk                   (0x1U << AFEC_CR_START_Pos)                    /**< (AFEC_CR) Start Conversion Mask */
#define AFEC_CR_START                       AFEC_CR_START_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CR_START_Msk instead */
#define AFEC_CR_MASK                        (0x03U)                                        /**< \deprecated (AFEC_CR) Register MASK  (Use AFEC_CR_Msk instead)  */
#define AFEC_CR_Msk                         (0x03U)                                        /**< (AFEC_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_MR : (AFEC Offset: 0x04) (R/W 32) AFEC Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TRGEN:1;                   /**< bit:      0  Trigger Enable                           */
    uint32_t TRGSEL:3;                  /**< bit:   1..3  Trigger Selection                        */
    uint32_t :1;                        /**< bit:      4  Reserved */
    uint32_t SLEEP:1;                   /**< bit:      5  Sleep Mode                               */
    uint32_t FWUP:1;                    /**< bit:      6  Fast Wake-up                             */
    uint32_t FREERUN:1;                 /**< bit:      7  Free Run Mode                            */
    uint32_t PRESCAL:8;                 /**< bit:  8..15  Prescaler Rate Selection                 */
    uint32_t STARTUP:4;                 /**< bit: 16..19  Start-up Time                            */
    uint32_t :3;                        /**< bit: 20..22  Reserved */
    uint32_t ONE:1;                     /**< bit:     23  One                                      */
    uint32_t TRACKTIM:4;                /**< bit: 24..27  Tracking Time                            */
    uint32_t TRANSFER:2;                /**< bit: 28..29  Transfer Period                          */
    uint32_t :1;                        /**< bit:     30  Reserved */
    uint32_t USEQ:1;                    /**< bit:     31  User Sequence Enable                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_MR_OFFSET                      (0x04)                                        /**<  (AFEC_MR) AFEC Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_MR_TRGEN_Pos                   0                                              /**< (AFEC_MR) Trigger Enable Position */
#define AFEC_MR_TRGEN_Msk                   (0x1U << AFEC_MR_TRGEN_Pos)                    /**< (AFEC_MR) Trigger Enable Mask */
#define AFEC_MR_TRGEN                       AFEC_MR_TRGEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_MR_TRGEN_Msk instead */
#define   AFEC_MR_TRGEN_DIS_Val             (0x0U)                                         /**< (AFEC_MR) Hardware triggers are disabled. Starting a conversion is only possible by software.  */
#define   AFEC_MR_TRGEN_EN_Val              (0x1U)                                         /**< (AFEC_MR) Hardware trigger selected by TRGSEL field is enabled.  */
#define AFEC_MR_TRGEN_DIS                   (AFEC_MR_TRGEN_DIS_Val << AFEC_MR_TRGEN_Pos)   /**< (AFEC_MR) Hardware triggers are disabled. Starting a conversion is only possible by software. Position  */
#define AFEC_MR_TRGEN_EN                    (AFEC_MR_TRGEN_EN_Val << AFEC_MR_TRGEN_Pos)    /**< (AFEC_MR) Hardware trigger selected by TRGSEL field is enabled. Position  */
#define AFEC_MR_TRGSEL_Pos                  1                                              /**< (AFEC_MR) Trigger Selection Position */
#define AFEC_MR_TRGSEL_Msk                  (0x7U << AFEC_MR_TRGSEL_Pos)                   /**< (AFEC_MR) Trigger Selection Mask */
#define AFEC_MR_TRGSEL(value)               (AFEC_MR_TRGSEL_Msk & ((value) << AFEC_MR_TRGSEL_Pos))
#define   AFEC_MR_TRGSEL_AFEC_TRIG0_Val     (0x0U)                                         /**< (AFEC_MR) AFE0_ADTRG for AFEC0 / AFE1_ADTRG for AFEC1  */
#define   AFEC_MR_TRGSEL_AFEC_TRIG1_Val     (0x1U)                                         /**< (AFEC_MR) TIOA Output of the Timer Counter Channel 0 for AFEC0/TIOA Output of the Timer Counter Channel 3 for AFEC1  */
#define   AFEC_MR_TRGSEL_AFEC_TRIG2_Val     (0x2U)                                         /**< (AFEC_MR) TIOA Output of the Timer Counter Channel 1 for AFEC0/TIOA Output of the Timer Counter Channel 4 for AFEC1  */
#define   AFEC_MR_TRGSEL_AFEC_TRIG3_Val     (0x3U)                                         /**< (AFEC_MR) TIOA Output of the Timer Counter Channel 2 for AFEC0/TIOA Output of the Timer Counter Channel 5 for AFEC1  */
#define   AFEC_MR_TRGSEL_AFEC_TRIG4_Val     (0x4U)                                         /**< (AFEC_MR) PWM0 event line 0 for AFEC0 / PWM1 event line 0 for AFEC1  */
#define   AFEC_MR_TRGSEL_AFEC_TRIG5_Val     (0x5U)                                         /**< (AFEC_MR) PWM0 event line 1 for AFEC0 / PWM1 event line 1 for AFEC1  */
#define   AFEC_MR_TRGSEL_AFEC_TRIG6_Val     (0x6U)                                         /**< (AFEC_MR) Analog Comparator  */
#define AFEC_MR_TRGSEL_AFEC_TRIG0           (AFEC_MR_TRGSEL_AFEC_TRIG0_Val << AFEC_MR_TRGSEL_Pos)  /**< (AFEC_MR) AFE0_ADTRG for AFEC0 / AFE1_ADTRG for AFEC1 Position  */
#define AFEC_MR_TRGSEL_AFEC_TRIG1           (AFEC_MR_TRGSEL_AFEC_TRIG1_Val << AFEC_MR_TRGSEL_Pos)  /**< (AFEC_MR) TIOA Output of the Timer Counter Channel 0 for AFEC0/TIOA Output of the Timer Counter Channel 3 for AFEC1 Position  */
#define AFEC_MR_TRGSEL_AFEC_TRIG2           (AFEC_MR_TRGSEL_AFEC_TRIG2_Val << AFEC_MR_TRGSEL_Pos)  /**< (AFEC_MR) TIOA Output of the Timer Counter Channel 1 for AFEC0/TIOA Output of the Timer Counter Channel 4 for AFEC1 Position  */
#define AFEC_MR_TRGSEL_AFEC_TRIG3           (AFEC_MR_TRGSEL_AFEC_TRIG3_Val << AFEC_MR_TRGSEL_Pos)  /**< (AFEC_MR) TIOA Output of the Timer Counter Channel 2 for AFEC0/TIOA Output of the Timer Counter Channel 5 for AFEC1 Position  */
#define AFEC_MR_TRGSEL_AFEC_TRIG4           (AFEC_MR_TRGSEL_AFEC_TRIG4_Val << AFEC_MR_TRGSEL_Pos)  /**< (AFEC_MR) PWM0 event line 0 for AFEC0 / PWM1 event line 0 for AFEC1 Position  */
#define AFEC_MR_TRGSEL_AFEC_TRIG5           (AFEC_MR_TRGSEL_AFEC_TRIG5_Val << AFEC_MR_TRGSEL_Pos)  /**< (AFEC_MR) PWM0 event line 1 for AFEC0 / PWM1 event line 1 for AFEC1 Position  */
#define AFEC_MR_TRGSEL_AFEC_TRIG6           (AFEC_MR_TRGSEL_AFEC_TRIG6_Val << AFEC_MR_TRGSEL_Pos)  /**< (AFEC_MR) Analog Comparator Position  */
#define AFEC_MR_SLEEP_Pos                   5                                              /**< (AFEC_MR) Sleep Mode Position */
#define AFEC_MR_SLEEP_Msk                   (0x1U << AFEC_MR_SLEEP_Pos)                    /**< (AFEC_MR) Sleep Mode Mask */
#define AFEC_MR_SLEEP                       AFEC_MR_SLEEP_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_MR_SLEEP_Msk instead */
#define   AFEC_MR_SLEEP_NORMAL_Val          (0x0U)                                         /**< (AFEC_MR) Normal mode: The AFE and reference voltage circuitry are kept ON between conversions.  */
#define   AFEC_MR_SLEEP_SLEEP_Val           (0x1U)                                         /**< (AFEC_MR) Sleep mode: The AFE and reference voltage circuitry are OFF between conversions.  */
#define AFEC_MR_SLEEP_NORMAL                (AFEC_MR_SLEEP_NORMAL_Val << AFEC_MR_SLEEP_Pos)  /**< (AFEC_MR) Normal mode: The AFE and reference voltage circuitry are kept ON between conversions. Position  */
#define AFEC_MR_SLEEP_SLEEP                 (AFEC_MR_SLEEP_SLEEP_Val << AFEC_MR_SLEEP_Pos)  /**< (AFEC_MR) Sleep mode: The AFE and reference voltage circuitry are OFF between conversions. Position  */
#define AFEC_MR_FWUP_Pos                    6                                              /**< (AFEC_MR) Fast Wake-up Position */
#define AFEC_MR_FWUP_Msk                    (0x1U << AFEC_MR_FWUP_Pos)                     /**< (AFEC_MR) Fast Wake-up Mask */
#define AFEC_MR_FWUP                        AFEC_MR_FWUP_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_MR_FWUP_Msk instead */
#define   AFEC_MR_FWUP_OFF_Val              (0x0U)                                         /**< (AFEC_MR) Normal Sleep mode: The sleep mode is defined by the SLEEP bit.  */
#define   AFEC_MR_FWUP_ON_Val               (0x1U)                                         /**< (AFEC_MR) Fast wake-up Sleep mode: The voltage reference is ON between conversions and AFE is OFF.  */
#define AFEC_MR_FWUP_OFF                    (AFEC_MR_FWUP_OFF_Val << AFEC_MR_FWUP_Pos)     /**< (AFEC_MR) Normal Sleep mode: The sleep mode is defined by the SLEEP bit. Position  */
#define AFEC_MR_FWUP_ON                     (AFEC_MR_FWUP_ON_Val << AFEC_MR_FWUP_Pos)      /**< (AFEC_MR) Fast wake-up Sleep mode: The voltage reference is ON between conversions and AFE is OFF. Position  */
#define AFEC_MR_FREERUN_Pos                 7                                              /**< (AFEC_MR) Free Run Mode Position */
#define AFEC_MR_FREERUN_Msk                 (0x1U << AFEC_MR_FREERUN_Pos)                  /**< (AFEC_MR) Free Run Mode Mask */
#define AFEC_MR_FREERUN                     AFEC_MR_FREERUN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_MR_FREERUN_Msk instead */
#define   AFEC_MR_FREERUN_OFF_Val           (0x0U)                                         /**< (AFEC_MR) Normal mode  */
#define   AFEC_MR_FREERUN_ON_Val            (0x1U)                                         /**< (AFEC_MR) Free Run mode: Never wait for any trigger.  */
#define AFEC_MR_FREERUN_OFF                 (AFEC_MR_FREERUN_OFF_Val << AFEC_MR_FREERUN_Pos)  /**< (AFEC_MR) Normal mode Position  */
#define AFEC_MR_FREERUN_ON                  (AFEC_MR_FREERUN_ON_Val << AFEC_MR_FREERUN_Pos)  /**< (AFEC_MR) Free Run mode: Never wait for any trigger. Position  */
#define AFEC_MR_PRESCAL_Pos                 8                                              /**< (AFEC_MR) Prescaler Rate Selection Position */
#define AFEC_MR_PRESCAL_Msk                 (0xFFU << AFEC_MR_PRESCAL_Pos)                 /**< (AFEC_MR) Prescaler Rate Selection Mask */
#define AFEC_MR_PRESCAL(value)              (AFEC_MR_PRESCAL_Msk & ((value) << AFEC_MR_PRESCAL_Pos))
#define AFEC_MR_STARTUP_Pos                 16                                             /**< (AFEC_MR) Start-up Time Position */
#define AFEC_MR_STARTUP_Msk                 (0xFU << AFEC_MR_STARTUP_Pos)                  /**< (AFEC_MR) Start-up Time Mask */
#define AFEC_MR_STARTUP(value)              (AFEC_MR_STARTUP_Msk & ((value) << AFEC_MR_STARTUP_Pos))
#define   AFEC_MR_STARTUP_SUT0_Val          (0x0U)                                         /**< (AFEC_MR) 0 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT8_Val          (0x1U)                                         /**< (AFEC_MR) 8 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT16_Val         (0x2U)                                         /**< (AFEC_MR) 16 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT24_Val         (0x3U)                                         /**< (AFEC_MR) 24 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT64_Val         (0x4U)                                         /**< (AFEC_MR) 64 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT80_Val         (0x5U)                                         /**< (AFEC_MR) 80 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT96_Val         (0x6U)                                         /**< (AFEC_MR) 96 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT112_Val        (0x7U)                                         /**< (AFEC_MR) 112 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT512_Val        (0x8U)                                         /**< (AFEC_MR) 512 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT576_Val        (0x9U)                                         /**< (AFEC_MR) 576 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT640_Val        (0xAU)                                         /**< (AFEC_MR) 640 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT704_Val        (0xBU)                                         /**< (AFEC_MR) 704 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT768_Val        (0xCU)                                         /**< (AFEC_MR) 768 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT832_Val        (0xDU)                                         /**< (AFEC_MR) 832 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT896_Val        (0xEU)                                         /**< (AFEC_MR) 896 periods of AFE clock  */
#define   AFEC_MR_STARTUP_SUT960_Val        (0xFU)                                         /**< (AFEC_MR) 960 periods of AFE clock  */
#define AFEC_MR_STARTUP_SUT0                (AFEC_MR_STARTUP_SUT0_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 0 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT8                (AFEC_MR_STARTUP_SUT8_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 8 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT16               (AFEC_MR_STARTUP_SUT16_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 16 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT24               (AFEC_MR_STARTUP_SUT24_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 24 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT64               (AFEC_MR_STARTUP_SUT64_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 64 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT80               (AFEC_MR_STARTUP_SUT80_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 80 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT96               (AFEC_MR_STARTUP_SUT96_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 96 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT112              (AFEC_MR_STARTUP_SUT112_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 112 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT512              (AFEC_MR_STARTUP_SUT512_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 512 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT576              (AFEC_MR_STARTUP_SUT576_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 576 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT640              (AFEC_MR_STARTUP_SUT640_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 640 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT704              (AFEC_MR_STARTUP_SUT704_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 704 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT768              (AFEC_MR_STARTUP_SUT768_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 768 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT832              (AFEC_MR_STARTUP_SUT832_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 832 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT896              (AFEC_MR_STARTUP_SUT896_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 896 periods of AFE clock Position  */
#define AFEC_MR_STARTUP_SUT960              (AFEC_MR_STARTUP_SUT960_Val << AFEC_MR_STARTUP_Pos)  /**< (AFEC_MR) 960 periods of AFE clock Position  */
#define AFEC_MR_ONE_Pos                     23                                             /**< (AFEC_MR) One Position */
#define AFEC_MR_ONE_Msk                     (0x1U << AFEC_MR_ONE_Pos)                      /**< (AFEC_MR) One Mask */
#define AFEC_MR_ONE                         AFEC_MR_ONE_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_MR_ONE_Msk instead */
#define AFEC_MR_TRACKTIM_Pos                24                                             /**< (AFEC_MR) Tracking Time Position */
#define AFEC_MR_TRACKTIM_Msk                (0xFU << AFEC_MR_TRACKTIM_Pos)                 /**< (AFEC_MR) Tracking Time Mask */
#define AFEC_MR_TRACKTIM(value)             (AFEC_MR_TRACKTIM_Msk & ((value) << AFEC_MR_TRACKTIM_Pos))
#define AFEC_MR_TRANSFER_Pos                28                                             /**< (AFEC_MR) Transfer Period Position */
#define AFEC_MR_TRANSFER_Msk                (0x3U << AFEC_MR_TRANSFER_Pos)                 /**< (AFEC_MR) Transfer Period Mask */
#define AFEC_MR_TRANSFER(value)             (AFEC_MR_TRANSFER_Msk & ((value) << AFEC_MR_TRANSFER_Pos))
#define AFEC_MR_USEQ_Pos                    31                                             /**< (AFEC_MR) User Sequence Enable Position */
#define AFEC_MR_USEQ_Msk                    (0x1U << AFEC_MR_USEQ_Pos)                     /**< (AFEC_MR) User Sequence Enable Mask */
#define AFEC_MR_USEQ                        AFEC_MR_USEQ_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_MR_USEQ_Msk instead */
#define   AFEC_MR_USEQ_NUM_ORDER_Val        (0x0U)                                         /**< (AFEC_MR) Normal mode: The controller converts channels in a simple numeric order.  */
#define   AFEC_MR_USEQ_REG_ORDER_Val        (0x1U)                                         /**< (AFEC_MR) User Sequence mode: The sequence respects what is defined in AFEC_SEQ1R and AFEC_SEQ1R.  */
#define AFEC_MR_USEQ_NUM_ORDER              (AFEC_MR_USEQ_NUM_ORDER_Val << AFEC_MR_USEQ_Pos)  /**< (AFEC_MR) Normal mode: The controller converts channels in a simple numeric order. Position  */
#define AFEC_MR_USEQ_REG_ORDER              (AFEC_MR_USEQ_REG_ORDER_Val << AFEC_MR_USEQ_Pos)  /**< (AFEC_MR) User Sequence mode: The sequence respects what is defined in AFEC_SEQ1R and AFEC_SEQ1R. Position  */
#define AFEC_MR_MASK                        (0xBF8FFFEFU)                                  /**< \deprecated (AFEC_MR) Register MASK  (Use AFEC_MR_Msk instead)  */
#define AFEC_MR_Msk                         (0xBF8FFFEFU)                                  /**< (AFEC_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_EMR : (AFEC Offset: 0x08) (R/W 32) AFEC Extended Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CMPMODE:2;                 /**< bit:   0..1  Comparison Mode                          */
    uint32_t :1;                        /**< bit:      2  Reserved */
    uint32_t CMPSEL:5;                  /**< bit:   3..7  Comparison Selected Channel              */
    uint32_t :1;                        /**< bit:      8  Reserved */
    uint32_t CMPALL:1;                  /**< bit:      9  Compare All Channels                     */
    uint32_t :2;                        /**< bit: 10..11  Reserved */
    uint32_t CMPFILTER:2;               /**< bit: 12..13  Compare Event Filtering                  */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t RES:3;                     /**< bit: 16..18  Resolution                               */
    uint32_t :5;                        /**< bit: 19..23  Reserved */
    uint32_t TAG:1;                     /**< bit:     24  TAG of the AFEC_LDCR                     */
    uint32_t STM:1;                     /**< bit:     25  Single Trigger Mode                      */
    uint32_t :2;                        /**< bit: 26..27  Reserved */
    uint32_t SIGNMODE:2;                /**< bit: 28..29  Sign Mode                                */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_EMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_EMR_OFFSET                     (0x08)                                        /**<  (AFEC_EMR) AFEC Extended Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_EMR_CMPMODE_Pos                0                                              /**< (AFEC_EMR) Comparison Mode Position */
#define AFEC_EMR_CMPMODE_Msk                (0x3U << AFEC_EMR_CMPMODE_Pos)                 /**< (AFEC_EMR) Comparison Mode Mask */
#define AFEC_EMR_CMPMODE(value)             (AFEC_EMR_CMPMODE_Msk & ((value) << AFEC_EMR_CMPMODE_Pos))
#define   AFEC_EMR_CMPMODE_LOW_Val          (0x0U)                                         /**< (AFEC_EMR) Generates an event when the converted data is lower than the low threshold of the window.  */
#define   AFEC_EMR_CMPMODE_HIGH_Val         (0x1U)                                         /**< (AFEC_EMR) Generates an event when the converted data is higher than the high threshold of the window.  */
#define   AFEC_EMR_CMPMODE_IN_Val           (0x2U)                                         /**< (AFEC_EMR) Generates an event when the converted data is in the comparison window.  */
#define   AFEC_EMR_CMPMODE_OUT_Val          (0x3U)                                         /**< (AFEC_EMR) Generates an event when the converted data is out of the comparison window.  */
#define AFEC_EMR_CMPMODE_LOW                (AFEC_EMR_CMPMODE_LOW_Val << AFEC_EMR_CMPMODE_Pos)  /**< (AFEC_EMR) Generates an event when the converted data is lower than the low threshold of the window. Position  */
#define AFEC_EMR_CMPMODE_HIGH               (AFEC_EMR_CMPMODE_HIGH_Val << AFEC_EMR_CMPMODE_Pos)  /**< (AFEC_EMR) Generates an event when the converted data is higher than the high threshold of the window. Position  */
#define AFEC_EMR_CMPMODE_IN                 (AFEC_EMR_CMPMODE_IN_Val << AFEC_EMR_CMPMODE_Pos)  /**< (AFEC_EMR) Generates an event when the converted data is in the comparison window. Position  */
#define AFEC_EMR_CMPMODE_OUT                (AFEC_EMR_CMPMODE_OUT_Val << AFEC_EMR_CMPMODE_Pos)  /**< (AFEC_EMR) Generates an event when the converted data is out of the comparison window. Position  */
#define AFEC_EMR_CMPSEL_Pos                 3                                              /**< (AFEC_EMR) Comparison Selected Channel Position */
#define AFEC_EMR_CMPSEL_Msk                 (0x1FU << AFEC_EMR_CMPSEL_Pos)                 /**< (AFEC_EMR) Comparison Selected Channel Mask */
#define AFEC_EMR_CMPSEL(value)              (AFEC_EMR_CMPSEL_Msk & ((value) << AFEC_EMR_CMPSEL_Pos))
#define AFEC_EMR_CMPALL_Pos                 9                                              /**< (AFEC_EMR) Compare All Channels Position */
#define AFEC_EMR_CMPALL_Msk                 (0x1U << AFEC_EMR_CMPALL_Pos)                  /**< (AFEC_EMR) Compare All Channels Mask */
#define AFEC_EMR_CMPALL                     AFEC_EMR_CMPALL_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_EMR_CMPALL_Msk instead */
#define AFEC_EMR_CMPFILTER_Pos              12                                             /**< (AFEC_EMR) Compare Event Filtering Position */
#define AFEC_EMR_CMPFILTER_Msk              (0x3U << AFEC_EMR_CMPFILTER_Pos)               /**< (AFEC_EMR) Compare Event Filtering Mask */
#define AFEC_EMR_CMPFILTER(value)           (AFEC_EMR_CMPFILTER_Msk & ((value) << AFEC_EMR_CMPFILTER_Pos))
#define AFEC_EMR_RES_Pos                    16                                             /**< (AFEC_EMR) Resolution Position */
#define AFEC_EMR_RES_Msk                    (0x7U << AFEC_EMR_RES_Pos)                     /**< (AFEC_EMR) Resolution Mask */
#define AFEC_EMR_RES(value)                 (AFEC_EMR_RES_Msk & ((value) << AFEC_EMR_RES_Pos))
#define   AFEC_EMR_RES_NO_AVERAGE_Val       (0x0U)                                         /**< (AFEC_EMR) 12-bit resolution, AFE sample rate is maximum (no averaging).  */
#define   AFEC_EMR_RES_OSR4_Val             (0x2U)                                         /**< (AFEC_EMR) 13-bit resolution, AFE sample rate divided by 4 (averaging).  */
#define   AFEC_EMR_RES_OSR16_Val            (0x3U)                                         /**< (AFEC_EMR) 14-bit resolution, AFE sample rate divided by 16 (averaging).  */
#define   AFEC_EMR_RES_OSR64_Val            (0x4U)                                         /**< (AFEC_EMR) 15-bit resolution, AFE sample rate divided by 64 (averaging).  */
#define   AFEC_EMR_RES_OSR256_Val           (0x5U)                                         /**< (AFEC_EMR) 16-bit resolution, AFE sample rate divided by 256 (averaging).  */
#define AFEC_EMR_RES_NO_AVERAGE             (AFEC_EMR_RES_NO_AVERAGE_Val << AFEC_EMR_RES_Pos)  /**< (AFEC_EMR) 12-bit resolution, AFE sample rate is maximum (no averaging). Position  */
#define AFEC_EMR_RES_OSR4                   (AFEC_EMR_RES_OSR4_Val << AFEC_EMR_RES_Pos)    /**< (AFEC_EMR) 13-bit resolution, AFE sample rate divided by 4 (averaging). Position  */
#define AFEC_EMR_RES_OSR16                  (AFEC_EMR_RES_OSR16_Val << AFEC_EMR_RES_Pos)   /**< (AFEC_EMR) 14-bit resolution, AFE sample rate divided by 16 (averaging). Position  */
#define AFEC_EMR_RES_OSR64                  (AFEC_EMR_RES_OSR64_Val << AFEC_EMR_RES_Pos)   /**< (AFEC_EMR) 15-bit resolution, AFE sample rate divided by 64 (averaging). Position  */
#define AFEC_EMR_RES_OSR256                 (AFEC_EMR_RES_OSR256_Val << AFEC_EMR_RES_Pos)  /**< (AFEC_EMR) 16-bit resolution, AFE sample rate divided by 256 (averaging). Position  */
#define AFEC_EMR_TAG_Pos                    24                                             /**< (AFEC_EMR) TAG of the AFEC_LDCR Position */
#define AFEC_EMR_TAG_Msk                    (0x1U << AFEC_EMR_TAG_Pos)                     /**< (AFEC_EMR) TAG of the AFEC_LDCR Mask */
#define AFEC_EMR_TAG                        AFEC_EMR_TAG_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_EMR_TAG_Msk instead */
#define AFEC_EMR_STM_Pos                    25                                             /**< (AFEC_EMR) Single Trigger Mode Position */
#define AFEC_EMR_STM_Msk                    (0x1U << AFEC_EMR_STM_Pos)                     /**< (AFEC_EMR) Single Trigger Mode Mask */
#define AFEC_EMR_STM                        AFEC_EMR_STM_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_EMR_STM_Msk instead */
#define AFEC_EMR_SIGNMODE_Pos               28                                             /**< (AFEC_EMR) Sign Mode Position */
#define AFEC_EMR_SIGNMODE_Msk               (0x3U << AFEC_EMR_SIGNMODE_Pos)                /**< (AFEC_EMR) Sign Mode Mask */
#define AFEC_EMR_SIGNMODE(value)            (AFEC_EMR_SIGNMODE_Msk & ((value) << AFEC_EMR_SIGNMODE_Pos))
#define   AFEC_EMR_SIGNMODE_SE_UNSG_DF_SIGN_Val (0x0U)                                         /**< (AFEC_EMR) Single-Ended channels: Unsigned conversions.Differential channels: Signed conversions.  */
#define   AFEC_EMR_SIGNMODE_SE_SIGN_DF_UNSG_Val (0x1U)                                         /**< (AFEC_EMR) Single-Ended channels: Signed conversions.Differential channels: Unsigned conversions.  */
#define   AFEC_EMR_SIGNMODE_ALL_UNSIGNED_Val (0x2U)                                         /**< (AFEC_EMR) All channels: Unsigned conversions.  */
#define   AFEC_EMR_SIGNMODE_ALL_SIGNED_Val  (0x3U)                                         /**< (AFEC_EMR) All channels: Signed conversions.  */
#define AFEC_EMR_SIGNMODE_SE_UNSG_DF_SIGN   (AFEC_EMR_SIGNMODE_SE_UNSG_DF_SIGN_Val << AFEC_EMR_SIGNMODE_Pos)  /**< (AFEC_EMR) Single-Ended channels: Unsigned conversions.Differential channels: Signed conversions. Position  */
#define AFEC_EMR_SIGNMODE_SE_SIGN_DF_UNSG   (AFEC_EMR_SIGNMODE_SE_SIGN_DF_UNSG_Val << AFEC_EMR_SIGNMODE_Pos)  /**< (AFEC_EMR) Single-Ended channels: Signed conversions.Differential channels: Unsigned conversions. Position  */
#define AFEC_EMR_SIGNMODE_ALL_UNSIGNED      (AFEC_EMR_SIGNMODE_ALL_UNSIGNED_Val << AFEC_EMR_SIGNMODE_Pos)  /**< (AFEC_EMR) All channels: Unsigned conversions. Position  */
#define AFEC_EMR_SIGNMODE_ALL_SIGNED        (AFEC_EMR_SIGNMODE_ALL_SIGNED_Val << AFEC_EMR_SIGNMODE_Pos)  /**< (AFEC_EMR) All channels: Signed conversions. Position  */
#define AFEC_EMR_MASK                       (0x330732FBU)                                  /**< \deprecated (AFEC_EMR) Register MASK  (Use AFEC_EMR_Msk instead)  */
#define AFEC_EMR_Msk                        (0x330732FBU)                                  /**< (AFEC_EMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_SEQ1R : (AFEC Offset: 0x0c) (R/W 32) AFEC Channel Sequence 1 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t USCH0:4;                   /**< bit:   0..3  User Sequence Number 0                   */
    uint32_t USCH1:4;                   /**< bit:   4..7  User Sequence Number 1                   */
    uint32_t USCH2:4;                   /**< bit:  8..11  User Sequence Number 2                   */
    uint32_t USCH3:4;                   /**< bit: 12..15  User Sequence Number 3                   */
    uint32_t USCH4:4;                   /**< bit: 16..19  User Sequence Number 4                   */
    uint32_t USCH5:4;                   /**< bit: 20..23  User Sequence Number 5                   */
    uint32_t USCH6:4;                   /**< bit: 24..27  User Sequence Number 6                   */
    uint32_t USCH7:4;                   /**< bit: 28..31  User Sequence Number 7                   */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_SEQ1R_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_SEQ1R_OFFSET                   (0x0C)                                        /**<  (AFEC_SEQ1R) AFEC Channel Sequence 1 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_SEQ1R_USCH0_Pos                0                                              /**< (AFEC_SEQ1R) User Sequence Number 0 Position */
#define AFEC_SEQ1R_USCH0_Msk                (0xFU << AFEC_SEQ1R_USCH0_Pos)                 /**< (AFEC_SEQ1R) User Sequence Number 0 Mask */
#define AFEC_SEQ1R_USCH0(value)             (AFEC_SEQ1R_USCH0_Msk & ((value) << AFEC_SEQ1R_USCH0_Pos))
#define AFEC_SEQ1R_USCH1_Pos                4                                              /**< (AFEC_SEQ1R) User Sequence Number 1 Position */
#define AFEC_SEQ1R_USCH1_Msk                (0xFU << AFEC_SEQ1R_USCH1_Pos)                 /**< (AFEC_SEQ1R) User Sequence Number 1 Mask */
#define AFEC_SEQ1R_USCH1(value)             (AFEC_SEQ1R_USCH1_Msk & ((value) << AFEC_SEQ1R_USCH1_Pos))
#define AFEC_SEQ1R_USCH2_Pos                8                                              /**< (AFEC_SEQ1R) User Sequence Number 2 Position */
#define AFEC_SEQ1R_USCH2_Msk                (0xFU << AFEC_SEQ1R_USCH2_Pos)                 /**< (AFEC_SEQ1R) User Sequence Number 2 Mask */
#define AFEC_SEQ1R_USCH2(value)             (AFEC_SEQ1R_USCH2_Msk & ((value) << AFEC_SEQ1R_USCH2_Pos))
#define AFEC_SEQ1R_USCH3_Pos                12                                             /**< (AFEC_SEQ1R) User Sequence Number 3 Position */
#define AFEC_SEQ1R_USCH3_Msk                (0xFU << AFEC_SEQ1R_USCH3_Pos)                 /**< (AFEC_SEQ1R) User Sequence Number 3 Mask */
#define AFEC_SEQ1R_USCH3(value)             (AFEC_SEQ1R_USCH3_Msk & ((value) << AFEC_SEQ1R_USCH3_Pos))
#define AFEC_SEQ1R_USCH4_Pos                16                                             /**< (AFEC_SEQ1R) User Sequence Number 4 Position */
#define AFEC_SEQ1R_USCH4_Msk                (0xFU << AFEC_SEQ1R_USCH4_Pos)                 /**< (AFEC_SEQ1R) User Sequence Number 4 Mask */
#define AFEC_SEQ1R_USCH4(value)             (AFEC_SEQ1R_USCH4_Msk & ((value) << AFEC_SEQ1R_USCH4_Pos))
#define AFEC_SEQ1R_USCH5_Pos                20                                             /**< (AFEC_SEQ1R) User Sequence Number 5 Position */
#define AFEC_SEQ1R_USCH5_Msk                (0xFU << AFEC_SEQ1R_USCH5_Pos)                 /**< (AFEC_SEQ1R) User Sequence Number 5 Mask */
#define AFEC_SEQ1R_USCH5(value)             (AFEC_SEQ1R_USCH5_Msk & ((value) << AFEC_SEQ1R_USCH5_Pos))
#define AFEC_SEQ1R_USCH6_Pos                24                                             /**< (AFEC_SEQ1R) User Sequence Number 6 Position */
#define AFEC_SEQ1R_USCH6_Msk                (0xFU << AFEC_SEQ1R_USCH6_Pos)                 /**< (AFEC_SEQ1R) User Sequence Number 6 Mask */
#define AFEC_SEQ1R_USCH6(value)             (AFEC_SEQ1R_USCH6_Msk & ((value) << AFEC_SEQ1R_USCH6_Pos))
#define AFEC_SEQ1R_USCH7_Pos                28                                             /**< (AFEC_SEQ1R) User Sequence Number 7 Position */
#define AFEC_SEQ1R_USCH7_Msk                (0xFU << AFEC_SEQ1R_USCH7_Pos)                 /**< (AFEC_SEQ1R) User Sequence Number 7 Mask */
#define AFEC_SEQ1R_USCH7(value)             (AFEC_SEQ1R_USCH7_Msk & ((value) << AFEC_SEQ1R_USCH7_Pos))
#define AFEC_SEQ1R_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (AFEC_SEQ1R) Register MASK  (Use AFEC_SEQ1R_Msk instead)  */
#define AFEC_SEQ1R_Msk                      (0xFFFFFFFFU)                                  /**< (AFEC_SEQ1R) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_SEQ2R : (AFEC Offset: 0x10) (R/W 32) AFEC Channel Sequence 2 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t USCH8:4;                   /**< bit:   0..3  User Sequence Number 8                   */
    uint32_t USCH9:4;                   /**< bit:   4..7  User Sequence Number 9                   */
    uint32_t USCH10:4;                  /**< bit:  8..11  User Sequence Number 10                  */
    uint32_t USCH11:4;                  /**< bit: 12..15  User Sequence Number 11                  */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_SEQ2R_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_SEQ2R_OFFSET                   (0x10)                                        /**<  (AFEC_SEQ2R) AFEC Channel Sequence 2 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_SEQ2R_USCH8_Pos                0                                              /**< (AFEC_SEQ2R) User Sequence Number 8 Position */
#define AFEC_SEQ2R_USCH8_Msk                (0xFU << AFEC_SEQ2R_USCH8_Pos)                 /**< (AFEC_SEQ2R) User Sequence Number 8 Mask */
#define AFEC_SEQ2R_USCH8(value)             (AFEC_SEQ2R_USCH8_Msk & ((value) << AFEC_SEQ2R_USCH8_Pos))
#define AFEC_SEQ2R_USCH9_Pos                4                                              /**< (AFEC_SEQ2R) User Sequence Number 9 Position */
#define AFEC_SEQ2R_USCH9_Msk                (0xFU << AFEC_SEQ2R_USCH9_Pos)                 /**< (AFEC_SEQ2R) User Sequence Number 9 Mask */
#define AFEC_SEQ2R_USCH9(value)             (AFEC_SEQ2R_USCH9_Msk & ((value) << AFEC_SEQ2R_USCH9_Pos))
#define AFEC_SEQ2R_USCH10_Pos               8                                              /**< (AFEC_SEQ2R) User Sequence Number 10 Position */
#define AFEC_SEQ2R_USCH10_Msk               (0xFU << AFEC_SEQ2R_USCH10_Pos)                /**< (AFEC_SEQ2R) User Sequence Number 10 Mask */
#define AFEC_SEQ2R_USCH10(value)            (AFEC_SEQ2R_USCH10_Msk & ((value) << AFEC_SEQ2R_USCH10_Pos))
#define AFEC_SEQ2R_USCH11_Pos               12                                             /**< (AFEC_SEQ2R) User Sequence Number 11 Position */
#define AFEC_SEQ2R_USCH11_Msk               (0xFU << AFEC_SEQ2R_USCH11_Pos)                /**< (AFEC_SEQ2R) User Sequence Number 11 Mask */
#define AFEC_SEQ2R_USCH11(value)            (AFEC_SEQ2R_USCH11_Msk & ((value) << AFEC_SEQ2R_USCH11_Pos))
#define AFEC_SEQ2R_MASK                     (0xFFFFU)                                      /**< \deprecated (AFEC_SEQ2R) Register MASK  (Use AFEC_SEQ2R_Msk instead)  */
#define AFEC_SEQ2R_Msk                      (0xFFFFU)                                      /**< (AFEC_SEQ2R) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CHER : (AFEC Offset: 0x14) (/W 32) AFEC Channel Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CH0:1;                     /**< bit:      0  Channel 0 Enable                         */
    uint32_t CH1:1;                     /**< bit:      1  Channel 1 Enable                         */
    uint32_t CH2:1;                     /**< bit:      2  Channel 2 Enable                         */
    uint32_t CH3:1;                     /**< bit:      3  Channel 3 Enable                         */
    uint32_t CH4:1;                     /**< bit:      4  Channel 4 Enable                         */
    uint32_t CH5:1;                     /**< bit:      5  Channel 5 Enable                         */
    uint32_t CH6:1;                     /**< bit:      6  Channel 6 Enable                         */
    uint32_t CH7:1;                     /**< bit:      7  Channel 7 Enable                         */
    uint32_t CH8:1;                     /**< bit:      8  Channel 8 Enable                         */
    uint32_t CH9:1;                     /**< bit:      9  Channel 9 Enable                         */
    uint32_t CH10:1;                    /**< bit:     10  Channel 10 Enable                        */
    uint32_t CH11:1;                    /**< bit:     11  Channel 11 Enable                        */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CH:12;                     /**< bit:  0..11  Channel xx Enable                        */
    uint32_t :20;                       /**< bit: 12..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CHER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CHER_OFFSET                    (0x14)                                        /**<  (AFEC_CHER) AFEC Channel Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CHER_CH0_Pos                   0                                              /**< (AFEC_CHER) Channel 0 Enable Position */
#define AFEC_CHER_CH0_Msk                   (0x1U << AFEC_CHER_CH0_Pos)                    /**< (AFEC_CHER) Channel 0 Enable Mask */
#define AFEC_CHER_CH0                       AFEC_CHER_CH0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH0_Msk instead */
#define AFEC_CHER_CH1_Pos                   1                                              /**< (AFEC_CHER) Channel 1 Enable Position */
#define AFEC_CHER_CH1_Msk                   (0x1U << AFEC_CHER_CH1_Pos)                    /**< (AFEC_CHER) Channel 1 Enable Mask */
#define AFEC_CHER_CH1                       AFEC_CHER_CH1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH1_Msk instead */
#define AFEC_CHER_CH2_Pos                   2                                              /**< (AFEC_CHER) Channel 2 Enable Position */
#define AFEC_CHER_CH2_Msk                   (0x1U << AFEC_CHER_CH2_Pos)                    /**< (AFEC_CHER) Channel 2 Enable Mask */
#define AFEC_CHER_CH2                       AFEC_CHER_CH2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH2_Msk instead */
#define AFEC_CHER_CH3_Pos                   3                                              /**< (AFEC_CHER) Channel 3 Enable Position */
#define AFEC_CHER_CH3_Msk                   (0x1U << AFEC_CHER_CH3_Pos)                    /**< (AFEC_CHER) Channel 3 Enable Mask */
#define AFEC_CHER_CH3                       AFEC_CHER_CH3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH3_Msk instead */
#define AFEC_CHER_CH4_Pos                   4                                              /**< (AFEC_CHER) Channel 4 Enable Position */
#define AFEC_CHER_CH4_Msk                   (0x1U << AFEC_CHER_CH4_Pos)                    /**< (AFEC_CHER) Channel 4 Enable Mask */
#define AFEC_CHER_CH4                       AFEC_CHER_CH4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH4_Msk instead */
#define AFEC_CHER_CH5_Pos                   5                                              /**< (AFEC_CHER) Channel 5 Enable Position */
#define AFEC_CHER_CH5_Msk                   (0x1U << AFEC_CHER_CH5_Pos)                    /**< (AFEC_CHER) Channel 5 Enable Mask */
#define AFEC_CHER_CH5                       AFEC_CHER_CH5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH5_Msk instead */
#define AFEC_CHER_CH6_Pos                   6                                              /**< (AFEC_CHER) Channel 6 Enable Position */
#define AFEC_CHER_CH6_Msk                   (0x1U << AFEC_CHER_CH6_Pos)                    /**< (AFEC_CHER) Channel 6 Enable Mask */
#define AFEC_CHER_CH6                       AFEC_CHER_CH6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH6_Msk instead */
#define AFEC_CHER_CH7_Pos                   7                                              /**< (AFEC_CHER) Channel 7 Enable Position */
#define AFEC_CHER_CH7_Msk                   (0x1U << AFEC_CHER_CH7_Pos)                    /**< (AFEC_CHER) Channel 7 Enable Mask */
#define AFEC_CHER_CH7                       AFEC_CHER_CH7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH7_Msk instead */
#define AFEC_CHER_CH8_Pos                   8                                              /**< (AFEC_CHER) Channel 8 Enable Position */
#define AFEC_CHER_CH8_Msk                   (0x1U << AFEC_CHER_CH8_Pos)                    /**< (AFEC_CHER) Channel 8 Enable Mask */
#define AFEC_CHER_CH8                       AFEC_CHER_CH8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH8_Msk instead */
#define AFEC_CHER_CH9_Pos                   9                                              /**< (AFEC_CHER) Channel 9 Enable Position */
#define AFEC_CHER_CH9_Msk                   (0x1U << AFEC_CHER_CH9_Pos)                    /**< (AFEC_CHER) Channel 9 Enable Mask */
#define AFEC_CHER_CH9                       AFEC_CHER_CH9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH9_Msk instead */
#define AFEC_CHER_CH10_Pos                  10                                             /**< (AFEC_CHER) Channel 10 Enable Position */
#define AFEC_CHER_CH10_Msk                  (0x1U << AFEC_CHER_CH10_Pos)                   /**< (AFEC_CHER) Channel 10 Enable Mask */
#define AFEC_CHER_CH10                      AFEC_CHER_CH10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH10_Msk instead */
#define AFEC_CHER_CH11_Pos                  11                                             /**< (AFEC_CHER) Channel 11 Enable Position */
#define AFEC_CHER_CH11_Msk                  (0x1U << AFEC_CHER_CH11_Pos)                   /**< (AFEC_CHER) Channel 11 Enable Mask */
#define AFEC_CHER_CH11                      AFEC_CHER_CH11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHER_CH11_Msk instead */
#define AFEC_CHER_CH_Pos                    0                                              /**< (AFEC_CHER Position) Channel xx Enable */
#define AFEC_CHER_CH_Msk                    (0xFFFU << AFEC_CHER_CH_Pos)                   /**< (AFEC_CHER Mask) CH */
#define AFEC_CHER_CH(value)                 (AFEC_CHER_CH_Msk & ((value) << AFEC_CHER_CH_Pos))  
#define AFEC_CHER_MASK                      (0xFFFU)                                       /**< \deprecated (AFEC_CHER) Register MASK  (Use AFEC_CHER_Msk instead)  */
#define AFEC_CHER_Msk                       (0xFFFU)                                       /**< (AFEC_CHER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CHDR : (AFEC Offset: 0x18) (/W 32) AFEC Channel Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CH0:1;                     /**< bit:      0  Channel 0 Disable                        */
    uint32_t CH1:1;                     /**< bit:      1  Channel 1 Disable                        */
    uint32_t CH2:1;                     /**< bit:      2  Channel 2 Disable                        */
    uint32_t CH3:1;                     /**< bit:      3  Channel 3 Disable                        */
    uint32_t CH4:1;                     /**< bit:      4  Channel 4 Disable                        */
    uint32_t CH5:1;                     /**< bit:      5  Channel 5 Disable                        */
    uint32_t CH6:1;                     /**< bit:      6  Channel 6 Disable                        */
    uint32_t CH7:1;                     /**< bit:      7  Channel 7 Disable                        */
    uint32_t CH8:1;                     /**< bit:      8  Channel 8 Disable                        */
    uint32_t CH9:1;                     /**< bit:      9  Channel 9 Disable                        */
    uint32_t CH10:1;                    /**< bit:     10  Channel 10 Disable                       */
    uint32_t CH11:1;                    /**< bit:     11  Channel 11 Disable                       */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CH:12;                     /**< bit:  0..11  Channel xx Disable                       */
    uint32_t :20;                       /**< bit: 12..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CHDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CHDR_OFFSET                    (0x18)                                        /**<  (AFEC_CHDR) AFEC Channel Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CHDR_CH0_Pos                   0                                              /**< (AFEC_CHDR) Channel 0 Disable Position */
#define AFEC_CHDR_CH0_Msk                   (0x1U << AFEC_CHDR_CH0_Pos)                    /**< (AFEC_CHDR) Channel 0 Disable Mask */
#define AFEC_CHDR_CH0                       AFEC_CHDR_CH0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH0_Msk instead */
#define AFEC_CHDR_CH1_Pos                   1                                              /**< (AFEC_CHDR) Channel 1 Disable Position */
#define AFEC_CHDR_CH1_Msk                   (0x1U << AFEC_CHDR_CH1_Pos)                    /**< (AFEC_CHDR) Channel 1 Disable Mask */
#define AFEC_CHDR_CH1                       AFEC_CHDR_CH1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH1_Msk instead */
#define AFEC_CHDR_CH2_Pos                   2                                              /**< (AFEC_CHDR) Channel 2 Disable Position */
#define AFEC_CHDR_CH2_Msk                   (0x1U << AFEC_CHDR_CH2_Pos)                    /**< (AFEC_CHDR) Channel 2 Disable Mask */
#define AFEC_CHDR_CH2                       AFEC_CHDR_CH2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH2_Msk instead */
#define AFEC_CHDR_CH3_Pos                   3                                              /**< (AFEC_CHDR) Channel 3 Disable Position */
#define AFEC_CHDR_CH3_Msk                   (0x1U << AFEC_CHDR_CH3_Pos)                    /**< (AFEC_CHDR) Channel 3 Disable Mask */
#define AFEC_CHDR_CH3                       AFEC_CHDR_CH3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH3_Msk instead */
#define AFEC_CHDR_CH4_Pos                   4                                              /**< (AFEC_CHDR) Channel 4 Disable Position */
#define AFEC_CHDR_CH4_Msk                   (0x1U << AFEC_CHDR_CH4_Pos)                    /**< (AFEC_CHDR) Channel 4 Disable Mask */
#define AFEC_CHDR_CH4                       AFEC_CHDR_CH4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH4_Msk instead */
#define AFEC_CHDR_CH5_Pos                   5                                              /**< (AFEC_CHDR) Channel 5 Disable Position */
#define AFEC_CHDR_CH5_Msk                   (0x1U << AFEC_CHDR_CH5_Pos)                    /**< (AFEC_CHDR) Channel 5 Disable Mask */
#define AFEC_CHDR_CH5                       AFEC_CHDR_CH5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH5_Msk instead */
#define AFEC_CHDR_CH6_Pos                   6                                              /**< (AFEC_CHDR) Channel 6 Disable Position */
#define AFEC_CHDR_CH6_Msk                   (0x1U << AFEC_CHDR_CH6_Pos)                    /**< (AFEC_CHDR) Channel 6 Disable Mask */
#define AFEC_CHDR_CH6                       AFEC_CHDR_CH6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH6_Msk instead */
#define AFEC_CHDR_CH7_Pos                   7                                              /**< (AFEC_CHDR) Channel 7 Disable Position */
#define AFEC_CHDR_CH7_Msk                   (0x1U << AFEC_CHDR_CH7_Pos)                    /**< (AFEC_CHDR) Channel 7 Disable Mask */
#define AFEC_CHDR_CH7                       AFEC_CHDR_CH7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH7_Msk instead */
#define AFEC_CHDR_CH8_Pos                   8                                              /**< (AFEC_CHDR) Channel 8 Disable Position */
#define AFEC_CHDR_CH8_Msk                   (0x1U << AFEC_CHDR_CH8_Pos)                    /**< (AFEC_CHDR) Channel 8 Disable Mask */
#define AFEC_CHDR_CH8                       AFEC_CHDR_CH8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH8_Msk instead */
#define AFEC_CHDR_CH9_Pos                   9                                              /**< (AFEC_CHDR) Channel 9 Disable Position */
#define AFEC_CHDR_CH9_Msk                   (0x1U << AFEC_CHDR_CH9_Pos)                    /**< (AFEC_CHDR) Channel 9 Disable Mask */
#define AFEC_CHDR_CH9                       AFEC_CHDR_CH9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH9_Msk instead */
#define AFEC_CHDR_CH10_Pos                  10                                             /**< (AFEC_CHDR) Channel 10 Disable Position */
#define AFEC_CHDR_CH10_Msk                  (0x1U << AFEC_CHDR_CH10_Pos)                   /**< (AFEC_CHDR) Channel 10 Disable Mask */
#define AFEC_CHDR_CH10                      AFEC_CHDR_CH10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH10_Msk instead */
#define AFEC_CHDR_CH11_Pos                  11                                             /**< (AFEC_CHDR) Channel 11 Disable Position */
#define AFEC_CHDR_CH11_Msk                  (0x1U << AFEC_CHDR_CH11_Pos)                   /**< (AFEC_CHDR) Channel 11 Disable Mask */
#define AFEC_CHDR_CH11                      AFEC_CHDR_CH11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHDR_CH11_Msk instead */
#define AFEC_CHDR_CH_Pos                    0                                              /**< (AFEC_CHDR Position) Channel xx Disable */
#define AFEC_CHDR_CH_Msk                    (0xFFFU << AFEC_CHDR_CH_Pos)                   /**< (AFEC_CHDR Mask) CH */
#define AFEC_CHDR_CH(value)                 (AFEC_CHDR_CH_Msk & ((value) << AFEC_CHDR_CH_Pos))  
#define AFEC_CHDR_MASK                      (0xFFFU)                                       /**< \deprecated (AFEC_CHDR) Register MASK  (Use AFEC_CHDR_Msk instead)  */
#define AFEC_CHDR_Msk                       (0xFFFU)                                       /**< (AFEC_CHDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CHSR : (AFEC Offset: 0x1c) (R/ 32) AFEC Channel Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CH0:1;                     /**< bit:      0  Channel 0 Status                         */
    uint32_t CH1:1;                     /**< bit:      1  Channel 1 Status                         */
    uint32_t CH2:1;                     /**< bit:      2  Channel 2 Status                         */
    uint32_t CH3:1;                     /**< bit:      3  Channel 3 Status                         */
    uint32_t CH4:1;                     /**< bit:      4  Channel 4 Status                         */
    uint32_t CH5:1;                     /**< bit:      5  Channel 5 Status                         */
    uint32_t CH6:1;                     /**< bit:      6  Channel 6 Status                         */
    uint32_t CH7:1;                     /**< bit:      7  Channel 7 Status                         */
    uint32_t CH8:1;                     /**< bit:      8  Channel 8 Status                         */
    uint32_t CH9:1;                     /**< bit:      9  Channel 9 Status                         */
    uint32_t CH10:1;                    /**< bit:     10  Channel 10 Status                        */
    uint32_t CH11:1;                    /**< bit:     11  Channel 11 Status                        */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CH:12;                     /**< bit:  0..11  Channel xx Status                        */
    uint32_t :20;                       /**< bit: 12..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CHSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CHSR_OFFSET                    (0x1C)                                        /**<  (AFEC_CHSR) AFEC Channel Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CHSR_CH0_Pos                   0                                              /**< (AFEC_CHSR) Channel 0 Status Position */
#define AFEC_CHSR_CH0_Msk                   (0x1U << AFEC_CHSR_CH0_Pos)                    /**< (AFEC_CHSR) Channel 0 Status Mask */
#define AFEC_CHSR_CH0                       AFEC_CHSR_CH0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH0_Msk instead */
#define AFEC_CHSR_CH1_Pos                   1                                              /**< (AFEC_CHSR) Channel 1 Status Position */
#define AFEC_CHSR_CH1_Msk                   (0x1U << AFEC_CHSR_CH1_Pos)                    /**< (AFEC_CHSR) Channel 1 Status Mask */
#define AFEC_CHSR_CH1                       AFEC_CHSR_CH1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH1_Msk instead */
#define AFEC_CHSR_CH2_Pos                   2                                              /**< (AFEC_CHSR) Channel 2 Status Position */
#define AFEC_CHSR_CH2_Msk                   (0x1U << AFEC_CHSR_CH2_Pos)                    /**< (AFEC_CHSR) Channel 2 Status Mask */
#define AFEC_CHSR_CH2                       AFEC_CHSR_CH2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH2_Msk instead */
#define AFEC_CHSR_CH3_Pos                   3                                              /**< (AFEC_CHSR) Channel 3 Status Position */
#define AFEC_CHSR_CH3_Msk                   (0x1U << AFEC_CHSR_CH3_Pos)                    /**< (AFEC_CHSR) Channel 3 Status Mask */
#define AFEC_CHSR_CH3                       AFEC_CHSR_CH3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH3_Msk instead */
#define AFEC_CHSR_CH4_Pos                   4                                              /**< (AFEC_CHSR) Channel 4 Status Position */
#define AFEC_CHSR_CH4_Msk                   (0x1U << AFEC_CHSR_CH4_Pos)                    /**< (AFEC_CHSR) Channel 4 Status Mask */
#define AFEC_CHSR_CH4                       AFEC_CHSR_CH4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH4_Msk instead */
#define AFEC_CHSR_CH5_Pos                   5                                              /**< (AFEC_CHSR) Channel 5 Status Position */
#define AFEC_CHSR_CH5_Msk                   (0x1U << AFEC_CHSR_CH5_Pos)                    /**< (AFEC_CHSR) Channel 5 Status Mask */
#define AFEC_CHSR_CH5                       AFEC_CHSR_CH5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH5_Msk instead */
#define AFEC_CHSR_CH6_Pos                   6                                              /**< (AFEC_CHSR) Channel 6 Status Position */
#define AFEC_CHSR_CH6_Msk                   (0x1U << AFEC_CHSR_CH6_Pos)                    /**< (AFEC_CHSR) Channel 6 Status Mask */
#define AFEC_CHSR_CH6                       AFEC_CHSR_CH6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH6_Msk instead */
#define AFEC_CHSR_CH7_Pos                   7                                              /**< (AFEC_CHSR) Channel 7 Status Position */
#define AFEC_CHSR_CH7_Msk                   (0x1U << AFEC_CHSR_CH7_Pos)                    /**< (AFEC_CHSR) Channel 7 Status Mask */
#define AFEC_CHSR_CH7                       AFEC_CHSR_CH7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH7_Msk instead */
#define AFEC_CHSR_CH8_Pos                   8                                              /**< (AFEC_CHSR) Channel 8 Status Position */
#define AFEC_CHSR_CH8_Msk                   (0x1U << AFEC_CHSR_CH8_Pos)                    /**< (AFEC_CHSR) Channel 8 Status Mask */
#define AFEC_CHSR_CH8                       AFEC_CHSR_CH8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH8_Msk instead */
#define AFEC_CHSR_CH9_Pos                   9                                              /**< (AFEC_CHSR) Channel 9 Status Position */
#define AFEC_CHSR_CH9_Msk                   (0x1U << AFEC_CHSR_CH9_Pos)                    /**< (AFEC_CHSR) Channel 9 Status Mask */
#define AFEC_CHSR_CH9                       AFEC_CHSR_CH9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH9_Msk instead */
#define AFEC_CHSR_CH10_Pos                  10                                             /**< (AFEC_CHSR) Channel 10 Status Position */
#define AFEC_CHSR_CH10_Msk                  (0x1U << AFEC_CHSR_CH10_Pos)                   /**< (AFEC_CHSR) Channel 10 Status Mask */
#define AFEC_CHSR_CH10                      AFEC_CHSR_CH10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH10_Msk instead */
#define AFEC_CHSR_CH11_Pos                  11                                             /**< (AFEC_CHSR) Channel 11 Status Position */
#define AFEC_CHSR_CH11_Msk                  (0x1U << AFEC_CHSR_CH11_Pos)                   /**< (AFEC_CHSR) Channel 11 Status Mask */
#define AFEC_CHSR_CH11                      AFEC_CHSR_CH11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CHSR_CH11_Msk instead */
#define AFEC_CHSR_CH_Pos                    0                                              /**< (AFEC_CHSR Position) Channel xx Status */
#define AFEC_CHSR_CH_Msk                    (0xFFFU << AFEC_CHSR_CH_Pos)                   /**< (AFEC_CHSR Mask) CH */
#define AFEC_CHSR_CH(value)                 (AFEC_CHSR_CH_Msk & ((value) << AFEC_CHSR_CH_Pos))  
#define AFEC_CHSR_MASK                      (0xFFFU)                                       /**< \deprecated (AFEC_CHSR) Register MASK  (Use AFEC_CHSR_Msk instead)  */
#define AFEC_CHSR_Msk                       (0xFFFU)                                       /**< (AFEC_CHSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_LCDR : (AFEC Offset: 0x20) (R/ 32) AFEC Last Converted Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t LDATA:16;                  /**< bit:  0..15  Last Data Converted                      */
    uint32_t :8;                        /**< bit: 16..23  Reserved */
    uint32_t CHNB:4;                    /**< bit: 24..27  Channel Number                           */
    uint32_t :4;                        /**< bit: 28..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_LCDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_LCDR_OFFSET                    (0x20)                                        /**<  (AFEC_LCDR) AFEC Last Converted Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_LCDR_LDATA_Pos                 0                                              /**< (AFEC_LCDR) Last Data Converted Position */
#define AFEC_LCDR_LDATA_Msk                 (0xFFFFU << AFEC_LCDR_LDATA_Pos)               /**< (AFEC_LCDR) Last Data Converted Mask */
#define AFEC_LCDR_LDATA(value)              (AFEC_LCDR_LDATA_Msk & ((value) << AFEC_LCDR_LDATA_Pos))
#define AFEC_LCDR_CHNB_Pos                  24                                             /**< (AFEC_LCDR) Channel Number Position */
#define AFEC_LCDR_CHNB_Msk                  (0xFU << AFEC_LCDR_CHNB_Pos)                   /**< (AFEC_LCDR) Channel Number Mask */
#define AFEC_LCDR_CHNB(value)               (AFEC_LCDR_CHNB_Msk & ((value) << AFEC_LCDR_CHNB_Pos))
#define AFEC_LCDR_MASK                      (0xF00FFFFU)                                   /**< \deprecated (AFEC_LCDR) Register MASK  (Use AFEC_LCDR_Msk instead)  */
#define AFEC_LCDR_Msk                       (0xF00FFFFU)                                   /**< (AFEC_LCDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_IER : (AFEC Offset: 0x24) (/W 32) AFEC Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t EOC0:1;                    /**< bit:      0  End of Conversion Interrupt Enable 0     */
    uint32_t EOC1:1;                    /**< bit:      1  End of Conversion Interrupt Enable 1     */
    uint32_t EOC2:1;                    /**< bit:      2  End of Conversion Interrupt Enable 2     */
    uint32_t EOC3:1;                    /**< bit:      3  End of Conversion Interrupt Enable 3     */
    uint32_t EOC4:1;                    /**< bit:      4  End of Conversion Interrupt Enable 4     */
    uint32_t EOC5:1;                    /**< bit:      5  End of Conversion Interrupt Enable 5     */
    uint32_t EOC6:1;                    /**< bit:      6  End of Conversion Interrupt Enable 6     */
    uint32_t EOC7:1;                    /**< bit:      7  End of Conversion Interrupt Enable 7     */
    uint32_t EOC8:1;                    /**< bit:      8  End of Conversion Interrupt Enable 8     */
    uint32_t EOC9:1;                    /**< bit:      9  End of Conversion Interrupt Enable 9     */
    uint32_t EOC10:1;                   /**< bit:     10  End of Conversion Interrupt Enable 10    */
    uint32_t EOC11:1;                   /**< bit:     11  End of Conversion Interrupt Enable 11    */
    uint32_t :12;                       /**< bit: 12..23  Reserved */
    uint32_t DRDY:1;                    /**< bit:     24  Data Ready Interrupt Enable              */
    uint32_t GOVRE:1;                   /**< bit:     25  General Overrun Error Interrupt Enable   */
    uint32_t COMPE:1;                   /**< bit:     26  Comparison Event Interrupt Enable        */
    uint32_t :3;                        /**< bit: 27..29  Reserved */
    uint32_t TEMPCHG:1;                 /**< bit:     30  Temperature Change Interrupt Enable      */
    uint32_t :1;                        /**< bit:     31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_IER_OFFSET                     (0x24)                                        /**<  (AFEC_IER) AFEC Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_IER_EOC0_Pos                   0                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 0 Position */
#define AFEC_IER_EOC0_Msk                   (0x1U << AFEC_IER_EOC0_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 0 Mask */
#define AFEC_IER_EOC0                       AFEC_IER_EOC0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC0_Msk instead */
#define AFEC_IER_EOC1_Pos                   1                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 1 Position */
#define AFEC_IER_EOC1_Msk                   (0x1U << AFEC_IER_EOC1_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 1 Mask */
#define AFEC_IER_EOC1                       AFEC_IER_EOC1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC1_Msk instead */
#define AFEC_IER_EOC2_Pos                   2                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 2 Position */
#define AFEC_IER_EOC2_Msk                   (0x1U << AFEC_IER_EOC2_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 2 Mask */
#define AFEC_IER_EOC2                       AFEC_IER_EOC2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC2_Msk instead */
#define AFEC_IER_EOC3_Pos                   3                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 3 Position */
#define AFEC_IER_EOC3_Msk                   (0x1U << AFEC_IER_EOC3_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 3 Mask */
#define AFEC_IER_EOC3                       AFEC_IER_EOC3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC3_Msk instead */
#define AFEC_IER_EOC4_Pos                   4                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 4 Position */
#define AFEC_IER_EOC4_Msk                   (0x1U << AFEC_IER_EOC4_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 4 Mask */
#define AFEC_IER_EOC4                       AFEC_IER_EOC4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC4_Msk instead */
#define AFEC_IER_EOC5_Pos                   5                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 5 Position */
#define AFEC_IER_EOC5_Msk                   (0x1U << AFEC_IER_EOC5_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 5 Mask */
#define AFEC_IER_EOC5                       AFEC_IER_EOC5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC5_Msk instead */
#define AFEC_IER_EOC6_Pos                   6                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 6 Position */
#define AFEC_IER_EOC6_Msk                   (0x1U << AFEC_IER_EOC6_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 6 Mask */
#define AFEC_IER_EOC6                       AFEC_IER_EOC6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC6_Msk instead */
#define AFEC_IER_EOC7_Pos                   7                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 7 Position */
#define AFEC_IER_EOC7_Msk                   (0x1U << AFEC_IER_EOC7_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 7 Mask */
#define AFEC_IER_EOC7                       AFEC_IER_EOC7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC7_Msk instead */
#define AFEC_IER_EOC8_Pos                   8                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 8 Position */
#define AFEC_IER_EOC8_Msk                   (0x1U << AFEC_IER_EOC8_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 8 Mask */
#define AFEC_IER_EOC8                       AFEC_IER_EOC8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC8_Msk instead */
#define AFEC_IER_EOC9_Pos                   9                                              /**< (AFEC_IER) End of Conversion Interrupt Enable 9 Position */
#define AFEC_IER_EOC9_Msk                   (0x1U << AFEC_IER_EOC9_Pos)                    /**< (AFEC_IER) End of Conversion Interrupt Enable 9 Mask */
#define AFEC_IER_EOC9                       AFEC_IER_EOC9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC9_Msk instead */
#define AFEC_IER_EOC10_Pos                  10                                             /**< (AFEC_IER) End of Conversion Interrupt Enable 10 Position */
#define AFEC_IER_EOC10_Msk                  (0x1U << AFEC_IER_EOC10_Pos)                   /**< (AFEC_IER) End of Conversion Interrupt Enable 10 Mask */
#define AFEC_IER_EOC10                      AFEC_IER_EOC10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC10_Msk instead */
#define AFEC_IER_EOC11_Pos                  11                                             /**< (AFEC_IER) End of Conversion Interrupt Enable 11 Position */
#define AFEC_IER_EOC11_Msk                  (0x1U << AFEC_IER_EOC11_Pos)                   /**< (AFEC_IER) End of Conversion Interrupt Enable 11 Mask */
#define AFEC_IER_EOC11                      AFEC_IER_EOC11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_EOC11_Msk instead */
#define AFEC_IER_DRDY_Pos                   24                                             /**< (AFEC_IER) Data Ready Interrupt Enable Position */
#define AFEC_IER_DRDY_Msk                   (0x1U << AFEC_IER_DRDY_Pos)                    /**< (AFEC_IER) Data Ready Interrupt Enable Mask */
#define AFEC_IER_DRDY                       AFEC_IER_DRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_DRDY_Msk instead */
#define AFEC_IER_GOVRE_Pos                  25                                             /**< (AFEC_IER) General Overrun Error Interrupt Enable Position */
#define AFEC_IER_GOVRE_Msk                  (0x1U << AFEC_IER_GOVRE_Pos)                   /**< (AFEC_IER) General Overrun Error Interrupt Enable Mask */
#define AFEC_IER_GOVRE                      AFEC_IER_GOVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_GOVRE_Msk instead */
#define AFEC_IER_COMPE_Pos                  26                                             /**< (AFEC_IER) Comparison Event Interrupt Enable Position */
#define AFEC_IER_COMPE_Msk                  (0x1U << AFEC_IER_COMPE_Pos)                   /**< (AFEC_IER) Comparison Event Interrupt Enable Mask */
#define AFEC_IER_COMPE                      AFEC_IER_COMPE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_COMPE_Msk instead */
#define AFEC_IER_TEMPCHG_Pos                30                                             /**< (AFEC_IER) Temperature Change Interrupt Enable Position */
#define AFEC_IER_TEMPCHG_Msk                (0x1U << AFEC_IER_TEMPCHG_Pos)                 /**< (AFEC_IER) Temperature Change Interrupt Enable Mask */
#define AFEC_IER_TEMPCHG                    AFEC_IER_TEMPCHG_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IER_TEMPCHG_Msk instead */
#define AFEC_IER_MASK                       (0x47000FFFU)                                  /**< \deprecated (AFEC_IER) Register MASK  (Use AFEC_IER_Msk instead)  */
#define AFEC_IER_Msk                        (0x47000FFFU)                                  /**< (AFEC_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_IDR : (AFEC Offset: 0x28) (/W 32) AFEC Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t EOC0:1;                    /**< bit:      0  End of Conversion Interrupt Disable 0    */
    uint32_t EOC1:1;                    /**< bit:      1  End of Conversion Interrupt Disable 1    */
    uint32_t EOC2:1;                    /**< bit:      2  End of Conversion Interrupt Disable 2    */
    uint32_t EOC3:1;                    /**< bit:      3  End of Conversion Interrupt Disable 3    */
    uint32_t EOC4:1;                    /**< bit:      4  End of Conversion Interrupt Disable 4    */
    uint32_t EOC5:1;                    /**< bit:      5  End of Conversion Interrupt Disable 5    */
    uint32_t EOC6:1;                    /**< bit:      6  End of Conversion Interrupt Disable 6    */
    uint32_t EOC7:1;                    /**< bit:      7  End of Conversion Interrupt Disable 7    */
    uint32_t EOC8:1;                    /**< bit:      8  End of Conversion Interrupt Disable 8    */
    uint32_t EOC9:1;                    /**< bit:      9  End of Conversion Interrupt Disable 9    */
    uint32_t EOC10:1;                   /**< bit:     10  End of Conversion Interrupt Disable 10   */
    uint32_t EOC11:1;                   /**< bit:     11  End of Conversion Interrupt Disable 11   */
    uint32_t :12;                       /**< bit: 12..23  Reserved */
    uint32_t DRDY:1;                    /**< bit:     24  Data Ready Interrupt Disable             */
    uint32_t GOVRE:1;                   /**< bit:     25  General Overrun Error Interrupt Disable  */
    uint32_t COMPE:1;                   /**< bit:     26  Comparison Event Interrupt Disable       */
    uint32_t :3;                        /**< bit: 27..29  Reserved */
    uint32_t TEMPCHG:1;                 /**< bit:     30  Temperature Change Interrupt Disable     */
    uint32_t :1;                        /**< bit:     31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_IDR_OFFSET                     (0x28)                                        /**<  (AFEC_IDR) AFEC Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_IDR_EOC0_Pos                   0                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 0 Position */
#define AFEC_IDR_EOC0_Msk                   (0x1U << AFEC_IDR_EOC0_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 0 Mask */
#define AFEC_IDR_EOC0                       AFEC_IDR_EOC0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC0_Msk instead */
#define AFEC_IDR_EOC1_Pos                   1                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 1 Position */
#define AFEC_IDR_EOC1_Msk                   (0x1U << AFEC_IDR_EOC1_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 1 Mask */
#define AFEC_IDR_EOC1                       AFEC_IDR_EOC1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC1_Msk instead */
#define AFEC_IDR_EOC2_Pos                   2                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 2 Position */
#define AFEC_IDR_EOC2_Msk                   (0x1U << AFEC_IDR_EOC2_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 2 Mask */
#define AFEC_IDR_EOC2                       AFEC_IDR_EOC2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC2_Msk instead */
#define AFEC_IDR_EOC3_Pos                   3                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 3 Position */
#define AFEC_IDR_EOC3_Msk                   (0x1U << AFEC_IDR_EOC3_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 3 Mask */
#define AFEC_IDR_EOC3                       AFEC_IDR_EOC3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC3_Msk instead */
#define AFEC_IDR_EOC4_Pos                   4                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 4 Position */
#define AFEC_IDR_EOC4_Msk                   (0x1U << AFEC_IDR_EOC4_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 4 Mask */
#define AFEC_IDR_EOC4                       AFEC_IDR_EOC4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC4_Msk instead */
#define AFEC_IDR_EOC5_Pos                   5                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 5 Position */
#define AFEC_IDR_EOC5_Msk                   (0x1U << AFEC_IDR_EOC5_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 5 Mask */
#define AFEC_IDR_EOC5                       AFEC_IDR_EOC5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC5_Msk instead */
#define AFEC_IDR_EOC6_Pos                   6                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 6 Position */
#define AFEC_IDR_EOC6_Msk                   (0x1U << AFEC_IDR_EOC6_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 6 Mask */
#define AFEC_IDR_EOC6                       AFEC_IDR_EOC6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC6_Msk instead */
#define AFEC_IDR_EOC7_Pos                   7                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 7 Position */
#define AFEC_IDR_EOC7_Msk                   (0x1U << AFEC_IDR_EOC7_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 7 Mask */
#define AFEC_IDR_EOC7                       AFEC_IDR_EOC7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC7_Msk instead */
#define AFEC_IDR_EOC8_Pos                   8                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 8 Position */
#define AFEC_IDR_EOC8_Msk                   (0x1U << AFEC_IDR_EOC8_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 8 Mask */
#define AFEC_IDR_EOC8                       AFEC_IDR_EOC8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC8_Msk instead */
#define AFEC_IDR_EOC9_Pos                   9                                              /**< (AFEC_IDR) End of Conversion Interrupt Disable 9 Position */
#define AFEC_IDR_EOC9_Msk                   (0x1U << AFEC_IDR_EOC9_Pos)                    /**< (AFEC_IDR) End of Conversion Interrupt Disable 9 Mask */
#define AFEC_IDR_EOC9                       AFEC_IDR_EOC9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC9_Msk instead */
#define AFEC_IDR_EOC10_Pos                  10                                             /**< (AFEC_IDR) End of Conversion Interrupt Disable 10 Position */
#define AFEC_IDR_EOC10_Msk                  (0x1U << AFEC_IDR_EOC10_Pos)                   /**< (AFEC_IDR) End of Conversion Interrupt Disable 10 Mask */
#define AFEC_IDR_EOC10                      AFEC_IDR_EOC10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC10_Msk instead */
#define AFEC_IDR_EOC11_Pos                  11                                             /**< (AFEC_IDR) End of Conversion Interrupt Disable 11 Position */
#define AFEC_IDR_EOC11_Msk                  (0x1U << AFEC_IDR_EOC11_Pos)                   /**< (AFEC_IDR) End of Conversion Interrupt Disable 11 Mask */
#define AFEC_IDR_EOC11                      AFEC_IDR_EOC11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_EOC11_Msk instead */
#define AFEC_IDR_DRDY_Pos                   24                                             /**< (AFEC_IDR) Data Ready Interrupt Disable Position */
#define AFEC_IDR_DRDY_Msk                   (0x1U << AFEC_IDR_DRDY_Pos)                    /**< (AFEC_IDR) Data Ready Interrupt Disable Mask */
#define AFEC_IDR_DRDY                       AFEC_IDR_DRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_DRDY_Msk instead */
#define AFEC_IDR_GOVRE_Pos                  25                                             /**< (AFEC_IDR) General Overrun Error Interrupt Disable Position */
#define AFEC_IDR_GOVRE_Msk                  (0x1U << AFEC_IDR_GOVRE_Pos)                   /**< (AFEC_IDR) General Overrun Error Interrupt Disable Mask */
#define AFEC_IDR_GOVRE                      AFEC_IDR_GOVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_GOVRE_Msk instead */
#define AFEC_IDR_COMPE_Pos                  26                                             /**< (AFEC_IDR) Comparison Event Interrupt Disable Position */
#define AFEC_IDR_COMPE_Msk                  (0x1U << AFEC_IDR_COMPE_Pos)                   /**< (AFEC_IDR) Comparison Event Interrupt Disable Mask */
#define AFEC_IDR_COMPE                      AFEC_IDR_COMPE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_COMPE_Msk instead */
#define AFEC_IDR_TEMPCHG_Pos                30                                             /**< (AFEC_IDR) Temperature Change Interrupt Disable Position */
#define AFEC_IDR_TEMPCHG_Msk                (0x1U << AFEC_IDR_TEMPCHG_Pos)                 /**< (AFEC_IDR) Temperature Change Interrupt Disable Mask */
#define AFEC_IDR_TEMPCHG                    AFEC_IDR_TEMPCHG_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IDR_TEMPCHG_Msk instead */
#define AFEC_IDR_MASK                       (0x47000FFFU)                                  /**< \deprecated (AFEC_IDR) Register MASK  (Use AFEC_IDR_Msk instead)  */
#define AFEC_IDR_Msk                        (0x47000FFFU)                                  /**< (AFEC_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_IMR : (AFEC Offset: 0x2c) (R/ 32) AFEC Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t EOC0:1;                    /**< bit:      0  End of Conversion Interrupt Mask 0       */
    uint32_t EOC1:1;                    /**< bit:      1  End of Conversion Interrupt Mask 1       */
    uint32_t EOC2:1;                    /**< bit:      2  End of Conversion Interrupt Mask 2       */
    uint32_t EOC3:1;                    /**< bit:      3  End of Conversion Interrupt Mask 3       */
    uint32_t EOC4:1;                    /**< bit:      4  End of Conversion Interrupt Mask 4       */
    uint32_t EOC5:1;                    /**< bit:      5  End of Conversion Interrupt Mask 5       */
    uint32_t EOC6:1;                    /**< bit:      6  End of Conversion Interrupt Mask 6       */
    uint32_t EOC7:1;                    /**< bit:      7  End of Conversion Interrupt Mask 7       */
    uint32_t EOC8:1;                    /**< bit:      8  End of Conversion Interrupt Mask 8       */
    uint32_t EOC9:1;                    /**< bit:      9  End of Conversion Interrupt Mask 9       */
    uint32_t EOC10:1;                   /**< bit:     10  End of Conversion Interrupt Mask 10      */
    uint32_t EOC11:1;                   /**< bit:     11  End of Conversion Interrupt Mask 11      */
    uint32_t :12;                       /**< bit: 12..23  Reserved */
    uint32_t DRDY:1;                    /**< bit:     24  Data Ready Interrupt Mask                */
    uint32_t GOVRE:1;                   /**< bit:     25  General Overrun Error Interrupt Mask     */
    uint32_t COMPE:1;                   /**< bit:     26  Comparison Event Interrupt Mask          */
    uint32_t :3;                        /**< bit: 27..29  Reserved */
    uint32_t TEMPCHG:1;                 /**< bit:     30  Temperature Change Interrupt Mask        */
    uint32_t :1;                        /**< bit:     31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_IMR_OFFSET                     (0x2C)                                        /**<  (AFEC_IMR) AFEC Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_IMR_EOC0_Pos                   0                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 0 Position */
#define AFEC_IMR_EOC0_Msk                   (0x1U << AFEC_IMR_EOC0_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 0 Mask */
#define AFEC_IMR_EOC0                       AFEC_IMR_EOC0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC0_Msk instead */
#define AFEC_IMR_EOC1_Pos                   1                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 1 Position */
#define AFEC_IMR_EOC1_Msk                   (0x1U << AFEC_IMR_EOC1_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 1 Mask */
#define AFEC_IMR_EOC1                       AFEC_IMR_EOC1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC1_Msk instead */
#define AFEC_IMR_EOC2_Pos                   2                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 2 Position */
#define AFEC_IMR_EOC2_Msk                   (0x1U << AFEC_IMR_EOC2_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 2 Mask */
#define AFEC_IMR_EOC2                       AFEC_IMR_EOC2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC2_Msk instead */
#define AFEC_IMR_EOC3_Pos                   3                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 3 Position */
#define AFEC_IMR_EOC3_Msk                   (0x1U << AFEC_IMR_EOC3_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 3 Mask */
#define AFEC_IMR_EOC3                       AFEC_IMR_EOC3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC3_Msk instead */
#define AFEC_IMR_EOC4_Pos                   4                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 4 Position */
#define AFEC_IMR_EOC4_Msk                   (0x1U << AFEC_IMR_EOC4_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 4 Mask */
#define AFEC_IMR_EOC4                       AFEC_IMR_EOC4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC4_Msk instead */
#define AFEC_IMR_EOC5_Pos                   5                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 5 Position */
#define AFEC_IMR_EOC5_Msk                   (0x1U << AFEC_IMR_EOC5_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 5 Mask */
#define AFEC_IMR_EOC5                       AFEC_IMR_EOC5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC5_Msk instead */
#define AFEC_IMR_EOC6_Pos                   6                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 6 Position */
#define AFEC_IMR_EOC6_Msk                   (0x1U << AFEC_IMR_EOC6_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 6 Mask */
#define AFEC_IMR_EOC6                       AFEC_IMR_EOC6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC6_Msk instead */
#define AFEC_IMR_EOC7_Pos                   7                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 7 Position */
#define AFEC_IMR_EOC7_Msk                   (0x1U << AFEC_IMR_EOC7_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 7 Mask */
#define AFEC_IMR_EOC7                       AFEC_IMR_EOC7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC7_Msk instead */
#define AFEC_IMR_EOC8_Pos                   8                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 8 Position */
#define AFEC_IMR_EOC8_Msk                   (0x1U << AFEC_IMR_EOC8_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 8 Mask */
#define AFEC_IMR_EOC8                       AFEC_IMR_EOC8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC8_Msk instead */
#define AFEC_IMR_EOC9_Pos                   9                                              /**< (AFEC_IMR) End of Conversion Interrupt Mask 9 Position */
#define AFEC_IMR_EOC9_Msk                   (0x1U << AFEC_IMR_EOC9_Pos)                    /**< (AFEC_IMR) End of Conversion Interrupt Mask 9 Mask */
#define AFEC_IMR_EOC9                       AFEC_IMR_EOC9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC9_Msk instead */
#define AFEC_IMR_EOC10_Pos                  10                                             /**< (AFEC_IMR) End of Conversion Interrupt Mask 10 Position */
#define AFEC_IMR_EOC10_Msk                  (0x1U << AFEC_IMR_EOC10_Pos)                   /**< (AFEC_IMR) End of Conversion Interrupt Mask 10 Mask */
#define AFEC_IMR_EOC10                      AFEC_IMR_EOC10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC10_Msk instead */
#define AFEC_IMR_EOC11_Pos                  11                                             /**< (AFEC_IMR) End of Conversion Interrupt Mask 11 Position */
#define AFEC_IMR_EOC11_Msk                  (0x1U << AFEC_IMR_EOC11_Pos)                   /**< (AFEC_IMR) End of Conversion Interrupt Mask 11 Mask */
#define AFEC_IMR_EOC11                      AFEC_IMR_EOC11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_EOC11_Msk instead */
#define AFEC_IMR_DRDY_Pos                   24                                             /**< (AFEC_IMR) Data Ready Interrupt Mask Position */
#define AFEC_IMR_DRDY_Msk                   (0x1U << AFEC_IMR_DRDY_Pos)                    /**< (AFEC_IMR) Data Ready Interrupt Mask Mask */
#define AFEC_IMR_DRDY                       AFEC_IMR_DRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_DRDY_Msk instead */
#define AFEC_IMR_GOVRE_Pos                  25                                             /**< (AFEC_IMR) General Overrun Error Interrupt Mask Position */
#define AFEC_IMR_GOVRE_Msk                  (0x1U << AFEC_IMR_GOVRE_Pos)                   /**< (AFEC_IMR) General Overrun Error Interrupt Mask Mask */
#define AFEC_IMR_GOVRE                      AFEC_IMR_GOVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_GOVRE_Msk instead */
#define AFEC_IMR_COMPE_Pos                  26                                             /**< (AFEC_IMR) Comparison Event Interrupt Mask Position */
#define AFEC_IMR_COMPE_Msk                  (0x1U << AFEC_IMR_COMPE_Pos)                   /**< (AFEC_IMR) Comparison Event Interrupt Mask Mask */
#define AFEC_IMR_COMPE                      AFEC_IMR_COMPE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_COMPE_Msk instead */
#define AFEC_IMR_TEMPCHG_Pos                30                                             /**< (AFEC_IMR) Temperature Change Interrupt Mask Position */
#define AFEC_IMR_TEMPCHG_Msk                (0x1U << AFEC_IMR_TEMPCHG_Pos)                 /**< (AFEC_IMR) Temperature Change Interrupt Mask Mask */
#define AFEC_IMR_TEMPCHG                    AFEC_IMR_TEMPCHG_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_IMR_TEMPCHG_Msk instead */
#define AFEC_IMR_MASK                       (0x47000FFFU)                                  /**< \deprecated (AFEC_IMR) Register MASK  (Use AFEC_IMR_Msk instead)  */
#define AFEC_IMR_Msk                        (0x47000FFFU)                                  /**< (AFEC_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_ISR : (AFEC Offset: 0x30) (R/ 32) AFEC Interrupt Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t EOC0:1;                    /**< bit:      0  End of Conversion 0 (cleared by reading AFEC_CDRx) */
    uint32_t EOC1:1;                    /**< bit:      1  End of Conversion 1 (cleared by reading AFEC_CDRx) */
    uint32_t EOC2:1;                    /**< bit:      2  End of Conversion 2 (cleared by reading AFEC_CDRx) */
    uint32_t EOC3:1;                    /**< bit:      3  End of Conversion 3 (cleared by reading AFEC_CDRx) */
    uint32_t EOC4:1;                    /**< bit:      4  End of Conversion 4 (cleared by reading AFEC_CDRx) */
    uint32_t EOC5:1;                    /**< bit:      5  End of Conversion 5 (cleared by reading AFEC_CDRx) */
    uint32_t EOC6:1;                    /**< bit:      6  End of Conversion 6 (cleared by reading AFEC_CDRx) */
    uint32_t EOC7:1;                    /**< bit:      7  End of Conversion 7 (cleared by reading AFEC_CDRx) */
    uint32_t EOC8:1;                    /**< bit:      8  End of Conversion 8 (cleared by reading AFEC_CDRx) */
    uint32_t EOC9:1;                    /**< bit:      9  End of Conversion 9 (cleared by reading AFEC_CDRx) */
    uint32_t EOC10:1;                   /**< bit:     10  End of Conversion 10 (cleared by reading AFEC_CDRx) */
    uint32_t EOC11:1;                   /**< bit:     11  End of Conversion 11 (cleared by reading AFEC_CDRx) */
    uint32_t :12;                       /**< bit: 12..23  Reserved */
    uint32_t DRDY:1;                    /**< bit:     24  Data Ready (cleared by reading AFEC_LCDR) */
    uint32_t GOVRE:1;                   /**< bit:     25  General Overrun Error (cleared by reading AFEC_ISR) */
    uint32_t COMPE:1;                   /**< bit:     26  Comparison Error (cleared by reading AFEC_ISR) */
    uint32_t :3;                        /**< bit: 27..29  Reserved */
    uint32_t TEMPCHG:1;                 /**< bit:     30  Temperature Change (cleared on read)     */
    uint32_t :1;                        /**< bit:     31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_ISR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_ISR_OFFSET                     (0x30)                                        /**<  (AFEC_ISR) AFEC Interrupt Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_ISR_EOC0_Pos                   0                                              /**< (AFEC_ISR) End of Conversion 0 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC0_Msk                   (0x1U << AFEC_ISR_EOC0_Pos)                    /**< (AFEC_ISR) End of Conversion 0 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC0                       AFEC_ISR_EOC0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC0_Msk instead */
#define AFEC_ISR_EOC1_Pos                   1                                              /**< (AFEC_ISR) End of Conversion 1 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC1_Msk                   (0x1U << AFEC_ISR_EOC1_Pos)                    /**< (AFEC_ISR) End of Conversion 1 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC1                       AFEC_ISR_EOC1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC1_Msk instead */
#define AFEC_ISR_EOC2_Pos                   2                                              /**< (AFEC_ISR) End of Conversion 2 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC2_Msk                   (0x1U << AFEC_ISR_EOC2_Pos)                    /**< (AFEC_ISR) End of Conversion 2 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC2                       AFEC_ISR_EOC2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC2_Msk instead */
#define AFEC_ISR_EOC3_Pos                   3                                              /**< (AFEC_ISR) End of Conversion 3 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC3_Msk                   (0x1U << AFEC_ISR_EOC3_Pos)                    /**< (AFEC_ISR) End of Conversion 3 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC3                       AFEC_ISR_EOC3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC3_Msk instead */
#define AFEC_ISR_EOC4_Pos                   4                                              /**< (AFEC_ISR) End of Conversion 4 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC4_Msk                   (0x1U << AFEC_ISR_EOC4_Pos)                    /**< (AFEC_ISR) End of Conversion 4 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC4                       AFEC_ISR_EOC4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC4_Msk instead */
#define AFEC_ISR_EOC5_Pos                   5                                              /**< (AFEC_ISR) End of Conversion 5 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC5_Msk                   (0x1U << AFEC_ISR_EOC5_Pos)                    /**< (AFEC_ISR) End of Conversion 5 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC5                       AFEC_ISR_EOC5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC5_Msk instead */
#define AFEC_ISR_EOC6_Pos                   6                                              /**< (AFEC_ISR) End of Conversion 6 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC6_Msk                   (0x1U << AFEC_ISR_EOC6_Pos)                    /**< (AFEC_ISR) End of Conversion 6 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC6                       AFEC_ISR_EOC6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC6_Msk instead */
#define AFEC_ISR_EOC7_Pos                   7                                              /**< (AFEC_ISR) End of Conversion 7 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC7_Msk                   (0x1U << AFEC_ISR_EOC7_Pos)                    /**< (AFEC_ISR) End of Conversion 7 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC7                       AFEC_ISR_EOC7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC7_Msk instead */
#define AFEC_ISR_EOC8_Pos                   8                                              /**< (AFEC_ISR) End of Conversion 8 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC8_Msk                   (0x1U << AFEC_ISR_EOC8_Pos)                    /**< (AFEC_ISR) End of Conversion 8 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC8                       AFEC_ISR_EOC8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC8_Msk instead */
#define AFEC_ISR_EOC9_Pos                   9                                              /**< (AFEC_ISR) End of Conversion 9 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC9_Msk                   (0x1U << AFEC_ISR_EOC9_Pos)                    /**< (AFEC_ISR) End of Conversion 9 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC9                       AFEC_ISR_EOC9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC9_Msk instead */
#define AFEC_ISR_EOC10_Pos                  10                                             /**< (AFEC_ISR) End of Conversion 10 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC10_Msk                  (0x1U << AFEC_ISR_EOC10_Pos)                   /**< (AFEC_ISR) End of Conversion 10 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC10                      AFEC_ISR_EOC10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC10_Msk instead */
#define AFEC_ISR_EOC11_Pos                  11                                             /**< (AFEC_ISR) End of Conversion 11 (cleared by reading AFEC_CDRx) Position */
#define AFEC_ISR_EOC11_Msk                  (0x1U << AFEC_ISR_EOC11_Pos)                   /**< (AFEC_ISR) End of Conversion 11 (cleared by reading AFEC_CDRx) Mask */
#define AFEC_ISR_EOC11                      AFEC_ISR_EOC11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_EOC11_Msk instead */
#define AFEC_ISR_DRDY_Pos                   24                                             /**< (AFEC_ISR) Data Ready (cleared by reading AFEC_LCDR) Position */
#define AFEC_ISR_DRDY_Msk                   (0x1U << AFEC_ISR_DRDY_Pos)                    /**< (AFEC_ISR) Data Ready (cleared by reading AFEC_LCDR) Mask */
#define AFEC_ISR_DRDY                       AFEC_ISR_DRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_DRDY_Msk instead */
#define AFEC_ISR_GOVRE_Pos                  25                                             /**< (AFEC_ISR) General Overrun Error (cleared by reading AFEC_ISR) Position */
#define AFEC_ISR_GOVRE_Msk                  (0x1U << AFEC_ISR_GOVRE_Pos)                   /**< (AFEC_ISR) General Overrun Error (cleared by reading AFEC_ISR) Mask */
#define AFEC_ISR_GOVRE                      AFEC_ISR_GOVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_GOVRE_Msk instead */
#define AFEC_ISR_COMPE_Pos                  26                                             /**< (AFEC_ISR) Comparison Error (cleared by reading AFEC_ISR) Position */
#define AFEC_ISR_COMPE_Msk                  (0x1U << AFEC_ISR_COMPE_Pos)                   /**< (AFEC_ISR) Comparison Error (cleared by reading AFEC_ISR) Mask */
#define AFEC_ISR_COMPE                      AFEC_ISR_COMPE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_COMPE_Msk instead */
#define AFEC_ISR_TEMPCHG_Pos                30                                             /**< (AFEC_ISR) Temperature Change (cleared on read) Position */
#define AFEC_ISR_TEMPCHG_Msk                (0x1U << AFEC_ISR_TEMPCHG_Pos)                 /**< (AFEC_ISR) Temperature Change (cleared on read) Mask */
#define AFEC_ISR_TEMPCHG                    AFEC_ISR_TEMPCHG_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ISR_TEMPCHG_Msk instead */
#define AFEC_ISR_MASK                       (0x47000FFFU)                                  /**< \deprecated (AFEC_ISR) Register MASK  (Use AFEC_ISR_Msk instead)  */
#define AFEC_ISR_Msk                        (0x47000FFFU)                                  /**< (AFEC_ISR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_OVER : (AFEC Offset: 0x4c) (R/ 32) AFEC Overrun Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OVRE0:1;                   /**< bit:      0  Overrun Error 0                          */
    uint32_t OVRE1:1;                   /**< bit:      1  Overrun Error 1                          */
    uint32_t OVRE2:1;                   /**< bit:      2  Overrun Error 2                          */
    uint32_t OVRE3:1;                   /**< bit:      3  Overrun Error 3                          */
    uint32_t OVRE4:1;                   /**< bit:      4  Overrun Error 4                          */
    uint32_t OVRE5:1;                   /**< bit:      5  Overrun Error 5                          */
    uint32_t OVRE6:1;                   /**< bit:      6  Overrun Error 6                          */
    uint32_t OVRE7:1;                   /**< bit:      7  Overrun Error 7                          */
    uint32_t OVRE8:1;                   /**< bit:      8  Overrun Error 8                          */
    uint32_t OVRE9:1;                   /**< bit:      9  Overrun Error 9                          */
    uint32_t OVRE10:1;                  /**< bit:     10  Overrun Error 10                         */
    uint32_t OVRE11:1;                  /**< bit:     11  Overrun Error 11                         */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t OVRE:12;                   /**< bit:  0..11  Overrun Error xx                         */
    uint32_t :20;                       /**< bit: 12..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_OVER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_OVER_OFFSET                    (0x4C)                                        /**<  (AFEC_OVER) AFEC Overrun Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_OVER_OVRE0_Pos                 0                                              /**< (AFEC_OVER) Overrun Error 0 Position */
#define AFEC_OVER_OVRE0_Msk                 (0x1U << AFEC_OVER_OVRE0_Pos)                  /**< (AFEC_OVER) Overrun Error 0 Mask */
#define AFEC_OVER_OVRE0                     AFEC_OVER_OVRE0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE0_Msk instead */
#define AFEC_OVER_OVRE1_Pos                 1                                              /**< (AFEC_OVER) Overrun Error 1 Position */
#define AFEC_OVER_OVRE1_Msk                 (0x1U << AFEC_OVER_OVRE1_Pos)                  /**< (AFEC_OVER) Overrun Error 1 Mask */
#define AFEC_OVER_OVRE1                     AFEC_OVER_OVRE1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE1_Msk instead */
#define AFEC_OVER_OVRE2_Pos                 2                                              /**< (AFEC_OVER) Overrun Error 2 Position */
#define AFEC_OVER_OVRE2_Msk                 (0x1U << AFEC_OVER_OVRE2_Pos)                  /**< (AFEC_OVER) Overrun Error 2 Mask */
#define AFEC_OVER_OVRE2                     AFEC_OVER_OVRE2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE2_Msk instead */
#define AFEC_OVER_OVRE3_Pos                 3                                              /**< (AFEC_OVER) Overrun Error 3 Position */
#define AFEC_OVER_OVRE3_Msk                 (0x1U << AFEC_OVER_OVRE3_Pos)                  /**< (AFEC_OVER) Overrun Error 3 Mask */
#define AFEC_OVER_OVRE3                     AFEC_OVER_OVRE3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE3_Msk instead */
#define AFEC_OVER_OVRE4_Pos                 4                                              /**< (AFEC_OVER) Overrun Error 4 Position */
#define AFEC_OVER_OVRE4_Msk                 (0x1U << AFEC_OVER_OVRE4_Pos)                  /**< (AFEC_OVER) Overrun Error 4 Mask */
#define AFEC_OVER_OVRE4                     AFEC_OVER_OVRE4_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE4_Msk instead */
#define AFEC_OVER_OVRE5_Pos                 5                                              /**< (AFEC_OVER) Overrun Error 5 Position */
#define AFEC_OVER_OVRE5_Msk                 (0x1U << AFEC_OVER_OVRE5_Pos)                  /**< (AFEC_OVER) Overrun Error 5 Mask */
#define AFEC_OVER_OVRE5                     AFEC_OVER_OVRE5_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE5_Msk instead */
#define AFEC_OVER_OVRE6_Pos                 6                                              /**< (AFEC_OVER) Overrun Error 6 Position */
#define AFEC_OVER_OVRE6_Msk                 (0x1U << AFEC_OVER_OVRE6_Pos)                  /**< (AFEC_OVER) Overrun Error 6 Mask */
#define AFEC_OVER_OVRE6                     AFEC_OVER_OVRE6_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE6_Msk instead */
#define AFEC_OVER_OVRE7_Pos                 7                                              /**< (AFEC_OVER) Overrun Error 7 Position */
#define AFEC_OVER_OVRE7_Msk                 (0x1U << AFEC_OVER_OVRE7_Pos)                  /**< (AFEC_OVER) Overrun Error 7 Mask */
#define AFEC_OVER_OVRE7                     AFEC_OVER_OVRE7_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE7_Msk instead */
#define AFEC_OVER_OVRE8_Pos                 8                                              /**< (AFEC_OVER) Overrun Error 8 Position */
#define AFEC_OVER_OVRE8_Msk                 (0x1U << AFEC_OVER_OVRE8_Pos)                  /**< (AFEC_OVER) Overrun Error 8 Mask */
#define AFEC_OVER_OVRE8                     AFEC_OVER_OVRE8_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE8_Msk instead */
#define AFEC_OVER_OVRE9_Pos                 9                                              /**< (AFEC_OVER) Overrun Error 9 Position */
#define AFEC_OVER_OVRE9_Msk                 (0x1U << AFEC_OVER_OVRE9_Pos)                  /**< (AFEC_OVER) Overrun Error 9 Mask */
#define AFEC_OVER_OVRE9                     AFEC_OVER_OVRE9_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE9_Msk instead */
#define AFEC_OVER_OVRE10_Pos                10                                             /**< (AFEC_OVER) Overrun Error 10 Position */
#define AFEC_OVER_OVRE10_Msk                (0x1U << AFEC_OVER_OVRE10_Pos)                 /**< (AFEC_OVER) Overrun Error 10 Mask */
#define AFEC_OVER_OVRE10                    AFEC_OVER_OVRE10_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE10_Msk instead */
#define AFEC_OVER_OVRE11_Pos                11                                             /**< (AFEC_OVER) Overrun Error 11 Position */
#define AFEC_OVER_OVRE11_Msk                (0x1U << AFEC_OVER_OVRE11_Pos)                 /**< (AFEC_OVER) Overrun Error 11 Mask */
#define AFEC_OVER_OVRE11                    AFEC_OVER_OVRE11_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_OVER_OVRE11_Msk instead */
#define AFEC_OVER_OVRE_Pos                  0                                              /**< (AFEC_OVER Position) Overrun Error xx */
#define AFEC_OVER_OVRE_Msk                  (0xFFFU << AFEC_OVER_OVRE_Pos)                 /**< (AFEC_OVER Mask) OVRE */
#define AFEC_OVER_OVRE(value)               (AFEC_OVER_OVRE_Msk & ((value) << AFEC_OVER_OVRE_Pos))  
#define AFEC_OVER_MASK                      (0xFFFU)                                       /**< \deprecated (AFEC_OVER) Register MASK  (Use AFEC_OVER_Msk instead)  */
#define AFEC_OVER_Msk                       (0xFFFU)                                       /**< (AFEC_OVER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CWR : (AFEC Offset: 0x50) (R/W 32) AFEC Compare Window Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t LOWTHRES:16;               /**< bit:  0..15  Low Threshold                            */
    uint32_t HIGHTHRES:16;              /**< bit: 16..31  High Threshold                           */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CWR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CWR_OFFSET                     (0x50)                                        /**<  (AFEC_CWR) AFEC Compare Window Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CWR_LOWTHRES_Pos               0                                              /**< (AFEC_CWR) Low Threshold Position */
#define AFEC_CWR_LOWTHRES_Msk               (0xFFFFU << AFEC_CWR_LOWTHRES_Pos)             /**< (AFEC_CWR) Low Threshold Mask */
#define AFEC_CWR_LOWTHRES(value)            (AFEC_CWR_LOWTHRES_Msk & ((value) << AFEC_CWR_LOWTHRES_Pos))
#define AFEC_CWR_HIGHTHRES_Pos              16                                             /**< (AFEC_CWR) High Threshold Position */
#define AFEC_CWR_HIGHTHRES_Msk              (0xFFFFU << AFEC_CWR_HIGHTHRES_Pos)            /**< (AFEC_CWR) High Threshold Mask */
#define AFEC_CWR_HIGHTHRES(value)           (AFEC_CWR_HIGHTHRES_Msk & ((value) << AFEC_CWR_HIGHTHRES_Pos))
#define AFEC_CWR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (AFEC_CWR) Register MASK  (Use AFEC_CWR_Msk instead)  */
#define AFEC_CWR_Msk                        (0xFFFFFFFFU)                                  /**< (AFEC_CWR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CGR : (AFEC Offset: 0x54) (R/W 32) AFEC Channel Gain Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t GAIN0:2;                   /**< bit:   0..1  Gain for Channel 0                       */
    uint32_t GAIN1:2;                   /**< bit:   2..3  Gain for Channel 1                       */
    uint32_t GAIN2:2;                   /**< bit:   4..5  Gain for Channel 2                       */
    uint32_t GAIN3:2;                   /**< bit:   6..7  Gain for Channel 3                       */
    uint32_t GAIN4:2;                   /**< bit:   8..9  Gain for Channel 4                       */
    uint32_t GAIN5:2;                   /**< bit: 10..11  Gain for Channel 5                       */
    uint32_t GAIN6:2;                   /**< bit: 12..13  Gain for Channel 6                       */
    uint32_t GAIN7:2;                   /**< bit: 14..15  Gain for Channel 7                       */
    uint32_t GAIN8:2;                   /**< bit: 16..17  Gain for Channel 8                       */
    uint32_t GAIN9:2;                   /**< bit: 18..19  Gain for Channel 9                       */
    uint32_t GAIN10:2;                  /**< bit: 20..21  Gain for Channel 10                      */
    uint32_t GAIN11:2;                  /**< bit: 22..23  Gain for Channel 11                      */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CGR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CGR_OFFSET                     (0x54)                                        /**<  (AFEC_CGR) AFEC Channel Gain Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CGR_GAIN0_Pos                  0                                              /**< (AFEC_CGR) Gain for Channel 0 Position */
#define AFEC_CGR_GAIN0_Msk                  (0x3U << AFEC_CGR_GAIN0_Pos)                   /**< (AFEC_CGR) Gain for Channel 0 Mask */
#define AFEC_CGR_GAIN0(value)               (AFEC_CGR_GAIN0_Msk & ((value) << AFEC_CGR_GAIN0_Pos))
#define AFEC_CGR_GAIN1_Pos                  2                                              /**< (AFEC_CGR) Gain for Channel 1 Position */
#define AFEC_CGR_GAIN1_Msk                  (0x3U << AFEC_CGR_GAIN1_Pos)                   /**< (AFEC_CGR) Gain for Channel 1 Mask */
#define AFEC_CGR_GAIN1(value)               (AFEC_CGR_GAIN1_Msk & ((value) << AFEC_CGR_GAIN1_Pos))
#define AFEC_CGR_GAIN2_Pos                  4                                              /**< (AFEC_CGR) Gain for Channel 2 Position */
#define AFEC_CGR_GAIN2_Msk                  (0x3U << AFEC_CGR_GAIN2_Pos)                   /**< (AFEC_CGR) Gain for Channel 2 Mask */
#define AFEC_CGR_GAIN2(value)               (AFEC_CGR_GAIN2_Msk & ((value) << AFEC_CGR_GAIN2_Pos))
#define AFEC_CGR_GAIN3_Pos                  6                                              /**< (AFEC_CGR) Gain for Channel 3 Position */
#define AFEC_CGR_GAIN3_Msk                  (0x3U << AFEC_CGR_GAIN3_Pos)                   /**< (AFEC_CGR) Gain for Channel 3 Mask */
#define AFEC_CGR_GAIN3(value)               (AFEC_CGR_GAIN3_Msk & ((value) << AFEC_CGR_GAIN3_Pos))
#define AFEC_CGR_GAIN4_Pos                  8                                              /**< (AFEC_CGR) Gain for Channel 4 Position */
#define AFEC_CGR_GAIN4_Msk                  (0x3U << AFEC_CGR_GAIN4_Pos)                   /**< (AFEC_CGR) Gain for Channel 4 Mask */
#define AFEC_CGR_GAIN4(value)               (AFEC_CGR_GAIN4_Msk & ((value) << AFEC_CGR_GAIN4_Pos))
#define AFEC_CGR_GAIN5_Pos                  10                                             /**< (AFEC_CGR) Gain for Channel 5 Position */
#define AFEC_CGR_GAIN5_Msk                  (0x3U << AFEC_CGR_GAIN5_Pos)                   /**< (AFEC_CGR) Gain for Channel 5 Mask */
#define AFEC_CGR_GAIN5(value)               (AFEC_CGR_GAIN5_Msk & ((value) << AFEC_CGR_GAIN5_Pos))
#define AFEC_CGR_GAIN6_Pos                  12                                             /**< (AFEC_CGR) Gain for Channel 6 Position */
#define AFEC_CGR_GAIN6_Msk                  (0x3U << AFEC_CGR_GAIN6_Pos)                   /**< (AFEC_CGR) Gain for Channel 6 Mask */
#define AFEC_CGR_GAIN6(value)               (AFEC_CGR_GAIN6_Msk & ((value) << AFEC_CGR_GAIN6_Pos))
#define AFEC_CGR_GAIN7_Pos                  14                                             /**< (AFEC_CGR) Gain for Channel 7 Position */
#define AFEC_CGR_GAIN7_Msk                  (0x3U << AFEC_CGR_GAIN7_Pos)                   /**< (AFEC_CGR) Gain for Channel 7 Mask */
#define AFEC_CGR_GAIN7(value)               (AFEC_CGR_GAIN7_Msk & ((value) << AFEC_CGR_GAIN7_Pos))
#define AFEC_CGR_GAIN8_Pos                  16                                             /**< (AFEC_CGR) Gain for Channel 8 Position */
#define AFEC_CGR_GAIN8_Msk                  (0x3U << AFEC_CGR_GAIN8_Pos)                   /**< (AFEC_CGR) Gain for Channel 8 Mask */
#define AFEC_CGR_GAIN8(value)               (AFEC_CGR_GAIN8_Msk & ((value) << AFEC_CGR_GAIN8_Pos))
#define AFEC_CGR_GAIN9_Pos                  18                                             /**< (AFEC_CGR) Gain for Channel 9 Position */
#define AFEC_CGR_GAIN9_Msk                  (0x3U << AFEC_CGR_GAIN9_Pos)                   /**< (AFEC_CGR) Gain for Channel 9 Mask */
#define AFEC_CGR_GAIN9(value)               (AFEC_CGR_GAIN9_Msk & ((value) << AFEC_CGR_GAIN9_Pos))
#define AFEC_CGR_GAIN10_Pos                 20                                             /**< (AFEC_CGR) Gain for Channel 10 Position */
#define AFEC_CGR_GAIN10_Msk                 (0x3U << AFEC_CGR_GAIN10_Pos)                  /**< (AFEC_CGR) Gain for Channel 10 Mask */
#define AFEC_CGR_GAIN10(value)              (AFEC_CGR_GAIN10_Msk & ((value) << AFEC_CGR_GAIN10_Pos))
#define AFEC_CGR_GAIN11_Pos                 22                                             /**< (AFEC_CGR) Gain for Channel 11 Position */
#define AFEC_CGR_GAIN11_Msk                 (0x3U << AFEC_CGR_GAIN11_Pos)                  /**< (AFEC_CGR) Gain for Channel 11 Mask */
#define AFEC_CGR_GAIN11(value)              (AFEC_CGR_GAIN11_Msk & ((value) << AFEC_CGR_GAIN11_Pos))
#define AFEC_CGR_MASK                       (0xFFFFFFU)                                    /**< \deprecated (AFEC_CGR) Register MASK  (Use AFEC_CGR_Msk instead)  */
#define AFEC_CGR_Msk                        (0xFFFFFFU)                                    /**< (AFEC_CGR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_DIFFR : (AFEC Offset: 0x60) (R/W 32) AFEC Channel Differential Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DIFF0:1;                   /**< bit:      0  Differential inputs for channel 0        */
    uint32_t DIFF1:1;                   /**< bit:      1  Differential inputs for channel 1        */
    uint32_t DIFF2:1;                   /**< bit:      2  Differential inputs for channel 2        */
    uint32_t DIFF3:1;                   /**< bit:      3  Differential inputs for channel 3        */
    uint32_t DIFF4:1;                   /**< bit:      4  Differential inputs for channel 4        */
    uint32_t DIFF5:1;                   /**< bit:      5  Differential inputs for channel 5        */
    uint32_t DIFF6:1;                   /**< bit:      6  Differential inputs for channel 6        */
    uint32_t DIFF7:1;                   /**< bit:      7  Differential inputs for channel 7        */
    uint32_t DIFF8:1;                   /**< bit:      8  Differential inputs for channel 8        */
    uint32_t DIFF9:1;                   /**< bit:      9  Differential inputs for channel 9        */
    uint32_t DIFF10:1;                  /**< bit:     10  Differential inputs for channel 10       */
    uint32_t DIFF11:1;                  /**< bit:     11  Differential inputs for channel 11       */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t DIFF:12;                   /**< bit:  0..11  Differential inputs for channel xx       */
    uint32_t :20;                       /**< bit: 12..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_DIFFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_DIFFR_OFFSET                   (0x60)                                        /**<  (AFEC_DIFFR) AFEC Channel Differential Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_DIFFR_DIFF0_Pos                0                                              /**< (AFEC_DIFFR) Differential inputs for channel 0 Position */
#define AFEC_DIFFR_DIFF0_Msk                (0x1U << AFEC_DIFFR_DIFF0_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 0 Mask */
#define AFEC_DIFFR_DIFF0                    AFEC_DIFFR_DIFF0_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF0_Msk instead */
#define AFEC_DIFFR_DIFF1_Pos                1                                              /**< (AFEC_DIFFR) Differential inputs for channel 1 Position */
#define AFEC_DIFFR_DIFF1_Msk                (0x1U << AFEC_DIFFR_DIFF1_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 1 Mask */
#define AFEC_DIFFR_DIFF1                    AFEC_DIFFR_DIFF1_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF1_Msk instead */
#define AFEC_DIFFR_DIFF2_Pos                2                                              /**< (AFEC_DIFFR) Differential inputs for channel 2 Position */
#define AFEC_DIFFR_DIFF2_Msk                (0x1U << AFEC_DIFFR_DIFF2_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 2 Mask */
#define AFEC_DIFFR_DIFF2                    AFEC_DIFFR_DIFF2_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF2_Msk instead */
#define AFEC_DIFFR_DIFF3_Pos                3                                              /**< (AFEC_DIFFR) Differential inputs for channel 3 Position */
#define AFEC_DIFFR_DIFF3_Msk                (0x1U << AFEC_DIFFR_DIFF3_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 3 Mask */
#define AFEC_DIFFR_DIFF3                    AFEC_DIFFR_DIFF3_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF3_Msk instead */
#define AFEC_DIFFR_DIFF4_Pos                4                                              /**< (AFEC_DIFFR) Differential inputs for channel 4 Position */
#define AFEC_DIFFR_DIFF4_Msk                (0x1U << AFEC_DIFFR_DIFF4_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 4 Mask */
#define AFEC_DIFFR_DIFF4                    AFEC_DIFFR_DIFF4_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF4_Msk instead */
#define AFEC_DIFFR_DIFF5_Pos                5                                              /**< (AFEC_DIFFR) Differential inputs for channel 5 Position */
#define AFEC_DIFFR_DIFF5_Msk                (0x1U << AFEC_DIFFR_DIFF5_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 5 Mask */
#define AFEC_DIFFR_DIFF5                    AFEC_DIFFR_DIFF5_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF5_Msk instead */
#define AFEC_DIFFR_DIFF6_Pos                6                                              /**< (AFEC_DIFFR) Differential inputs for channel 6 Position */
#define AFEC_DIFFR_DIFF6_Msk                (0x1U << AFEC_DIFFR_DIFF6_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 6 Mask */
#define AFEC_DIFFR_DIFF6                    AFEC_DIFFR_DIFF6_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF6_Msk instead */
#define AFEC_DIFFR_DIFF7_Pos                7                                              /**< (AFEC_DIFFR) Differential inputs for channel 7 Position */
#define AFEC_DIFFR_DIFF7_Msk                (0x1U << AFEC_DIFFR_DIFF7_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 7 Mask */
#define AFEC_DIFFR_DIFF7                    AFEC_DIFFR_DIFF7_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF7_Msk instead */
#define AFEC_DIFFR_DIFF8_Pos                8                                              /**< (AFEC_DIFFR) Differential inputs for channel 8 Position */
#define AFEC_DIFFR_DIFF8_Msk                (0x1U << AFEC_DIFFR_DIFF8_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 8 Mask */
#define AFEC_DIFFR_DIFF8                    AFEC_DIFFR_DIFF8_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF8_Msk instead */
#define AFEC_DIFFR_DIFF9_Pos                9                                              /**< (AFEC_DIFFR) Differential inputs for channel 9 Position */
#define AFEC_DIFFR_DIFF9_Msk                (0x1U << AFEC_DIFFR_DIFF9_Pos)                 /**< (AFEC_DIFFR) Differential inputs for channel 9 Mask */
#define AFEC_DIFFR_DIFF9                    AFEC_DIFFR_DIFF9_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF9_Msk instead */
#define AFEC_DIFFR_DIFF10_Pos               10                                             /**< (AFEC_DIFFR) Differential inputs for channel 10 Position */
#define AFEC_DIFFR_DIFF10_Msk               (0x1U << AFEC_DIFFR_DIFF10_Pos)                /**< (AFEC_DIFFR) Differential inputs for channel 10 Mask */
#define AFEC_DIFFR_DIFF10                   AFEC_DIFFR_DIFF10_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF10_Msk instead */
#define AFEC_DIFFR_DIFF11_Pos               11                                             /**< (AFEC_DIFFR) Differential inputs for channel 11 Position */
#define AFEC_DIFFR_DIFF11_Msk               (0x1U << AFEC_DIFFR_DIFF11_Pos)                /**< (AFEC_DIFFR) Differential inputs for channel 11 Mask */
#define AFEC_DIFFR_DIFF11                   AFEC_DIFFR_DIFF11_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_DIFFR_DIFF11_Msk instead */
#define AFEC_DIFFR_DIFF_Pos                 0                                              /**< (AFEC_DIFFR Position) Differential inputs for channel xx */
#define AFEC_DIFFR_DIFF_Msk                 (0xFFFU << AFEC_DIFFR_DIFF_Pos)                /**< (AFEC_DIFFR Mask) DIFF */
#define AFEC_DIFFR_DIFF(value)              (AFEC_DIFFR_DIFF_Msk & ((value) << AFEC_DIFFR_DIFF_Pos))  
#define AFEC_DIFFR_MASK                     (0xFFFU)                                       /**< \deprecated (AFEC_DIFFR) Register MASK  (Use AFEC_DIFFR_Msk instead)  */
#define AFEC_DIFFR_Msk                      (0xFFFU)                                       /**< (AFEC_DIFFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CSELR : (AFEC Offset: 0x64) (R/W 32) AFEC Channel Selection Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CSEL:4;                    /**< bit:   0..3  Channel Selection                        */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CSELR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CSELR_OFFSET                   (0x64)                                        /**<  (AFEC_CSELR) AFEC Channel Selection Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CSELR_CSEL_Pos                 0                                              /**< (AFEC_CSELR) Channel Selection Position */
#define AFEC_CSELR_CSEL_Msk                 (0xFU << AFEC_CSELR_CSEL_Pos)                  /**< (AFEC_CSELR) Channel Selection Mask */
#define AFEC_CSELR_CSEL(value)              (AFEC_CSELR_CSEL_Msk & ((value) << AFEC_CSELR_CSEL_Pos))
#define AFEC_CSELR_MASK                     (0x0FU)                                        /**< \deprecated (AFEC_CSELR) Register MASK  (Use AFEC_CSELR_Msk instead)  */
#define AFEC_CSELR_Msk                      (0x0FU)                                        /**< (AFEC_CSELR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CDR : (AFEC Offset: 0x68) (R/ 32) AFEC Channel Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATA:16;                   /**< bit:  0..15  Converted Data                           */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CDR_OFFSET                     (0x68)                                        /**<  (AFEC_CDR) AFEC Channel Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CDR_DATA_Pos                   0                                              /**< (AFEC_CDR) Converted Data Position */
#define AFEC_CDR_DATA_Msk                   (0xFFFFU << AFEC_CDR_DATA_Pos)                 /**< (AFEC_CDR) Converted Data Mask */
#define AFEC_CDR_DATA(value)                (AFEC_CDR_DATA_Msk & ((value) << AFEC_CDR_DATA_Pos))
#define AFEC_CDR_MASK                       (0xFFFFU)                                      /**< \deprecated (AFEC_CDR) Register MASK  (Use AFEC_CDR_Msk instead)  */
#define AFEC_CDR_Msk                        (0xFFFFU)                                      /**< (AFEC_CDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_COCR : (AFEC Offset: 0x6c) (R/W 32) AFEC Channel Offset Compensation Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t AOFF:10;                   /**< bit:   0..9  Analog Offset                            */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_COCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_COCR_OFFSET                    (0x6C)                                        /**<  (AFEC_COCR) AFEC Channel Offset Compensation Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_COCR_AOFF_Pos                  0                                              /**< (AFEC_COCR) Analog Offset Position */
#define AFEC_COCR_AOFF_Msk                  (0x3FFU << AFEC_COCR_AOFF_Pos)                 /**< (AFEC_COCR) Analog Offset Mask */
#define AFEC_COCR_AOFF(value)               (AFEC_COCR_AOFF_Msk & ((value) << AFEC_COCR_AOFF_Pos))
#define AFEC_COCR_MASK                      (0x3FFU)                                       /**< \deprecated (AFEC_COCR) Register MASK  (Use AFEC_COCR_Msk instead)  */
#define AFEC_COCR_Msk                       (0x3FFU)                                       /**< (AFEC_COCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_TEMPMR : (AFEC Offset: 0x70) (R/W 32) AFEC Temperature Sensor Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RTCT:1;                    /**< bit:      0  Temperature Sensor RTC Trigger Mode      */
    uint32_t :3;                        /**< bit:   1..3  Reserved */
    uint32_t TEMPCMPMOD:2;              /**< bit:   4..5  Temperature Comparison Mode              */
    uint32_t :26;                       /**< bit:  6..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_TEMPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_TEMPMR_OFFSET                  (0x70)                                        /**<  (AFEC_TEMPMR) AFEC Temperature Sensor Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_TEMPMR_RTCT_Pos                0                                              /**< (AFEC_TEMPMR) Temperature Sensor RTC Trigger Mode Position */
#define AFEC_TEMPMR_RTCT_Msk                (0x1U << AFEC_TEMPMR_RTCT_Pos)                 /**< (AFEC_TEMPMR) Temperature Sensor RTC Trigger Mode Mask */
#define AFEC_TEMPMR_RTCT                    AFEC_TEMPMR_RTCT_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_TEMPMR_RTCT_Msk instead */
#define AFEC_TEMPMR_TEMPCMPMOD_Pos          4                                              /**< (AFEC_TEMPMR) Temperature Comparison Mode Position */
#define AFEC_TEMPMR_TEMPCMPMOD_Msk          (0x3U << AFEC_TEMPMR_TEMPCMPMOD_Pos)           /**< (AFEC_TEMPMR) Temperature Comparison Mode Mask */
#define AFEC_TEMPMR_TEMPCMPMOD(value)       (AFEC_TEMPMR_TEMPCMPMOD_Msk & ((value) << AFEC_TEMPMR_TEMPCMPMOD_Pos))
#define   AFEC_TEMPMR_TEMPCMPMOD_LOW_Val    (0x0U)                                         /**< (AFEC_TEMPMR) Generates an event when the converted data is lower than the low threshold of the window.  */
#define   AFEC_TEMPMR_TEMPCMPMOD_HIGH_Val   (0x1U)                                         /**< (AFEC_TEMPMR) Generates an event when the converted data is higher than the high threshold of the window.  */
#define   AFEC_TEMPMR_TEMPCMPMOD_IN_Val     (0x2U)                                         /**< (AFEC_TEMPMR) Generates an event when the converted data is in the comparison window.  */
#define   AFEC_TEMPMR_TEMPCMPMOD_OUT_Val    (0x3U)                                         /**< (AFEC_TEMPMR) Generates an event when the converted data is out of the comparison window.  */
#define AFEC_TEMPMR_TEMPCMPMOD_LOW          (AFEC_TEMPMR_TEMPCMPMOD_LOW_Val << AFEC_TEMPMR_TEMPCMPMOD_Pos)  /**< (AFEC_TEMPMR) Generates an event when the converted data is lower than the low threshold of the window. Position  */
#define AFEC_TEMPMR_TEMPCMPMOD_HIGH         (AFEC_TEMPMR_TEMPCMPMOD_HIGH_Val << AFEC_TEMPMR_TEMPCMPMOD_Pos)  /**< (AFEC_TEMPMR) Generates an event when the converted data is higher than the high threshold of the window. Position  */
#define AFEC_TEMPMR_TEMPCMPMOD_IN           (AFEC_TEMPMR_TEMPCMPMOD_IN_Val << AFEC_TEMPMR_TEMPCMPMOD_Pos)  /**< (AFEC_TEMPMR) Generates an event when the converted data is in the comparison window. Position  */
#define AFEC_TEMPMR_TEMPCMPMOD_OUT          (AFEC_TEMPMR_TEMPCMPMOD_OUT_Val << AFEC_TEMPMR_TEMPCMPMOD_Pos)  /**< (AFEC_TEMPMR) Generates an event when the converted data is out of the comparison window. Position  */
#define AFEC_TEMPMR_MASK                    (0x31U)                                        /**< \deprecated (AFEC_TEMPMR) Register MASK  (Use AFEC_TEMPMR_Msk instead)  */
#define AFEC_TEMPMR_Msk                     (0x31U)                                        /**< (AFEC_TEMPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_TEMPCWR : (AFEC Offset: 0x74) (R/W 32) AFEC Temperature Compare Window Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TLOWTHRES:16;              /**< bit:  0..15  Temperature Low Threshold                */
    uint32_t THIGHTHRES:16;             /**< bit: 16..31  Temperature High Threshold               */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_TEMPCWR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_TEMPCWR_OFFSET                 (0x74)                                        /**<  (AFEC_TEMPCWR) AFEC Temperature Compare Window Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_TEMPCWR_TLOWTHRES_Pos          0                                              /**< (AFEC_TEMPCWR) Temperature Low Threshold Position */
#define AFEC_TEMPCWR_TLOWTHRES_Msk          (0xFFFFU << AFEC_TEMPCWR_TLOWTHRES_Pos)        /**< (AFEC_TEMPCWR) Temperature Low Threshold Mask */
#define AFEC_TEMPCWR_TLOWTHRES(value)       (AFEC_TEMPCWR_TLOWTHRES_Msk & ((value) << AFEC_TEMPCWR_TLOWTHRES_Pos))
#define AFEC_TEMPCWR_THIGHTHRES_Pos         16                                             /**< (AFEC_TEMPCWR) Temperature High Threshold Position */
#define AFEC_TEMPCWR_THIGHTHRES_Msk         (0xFFFFU << AFEC_TEMPCWR_THIGHTHRES_Pos)       /**< (AFEC_TEMPCWR) Temperature High Threshold Mask */
#define AFEC_TEMPCWR_THIGHTHRES(value)      (AFEC_TEMPCWR_THIGHTHRES_Msk & ((value) << AFEC_TEMPCWR_THIGHTHRES_Pos))
#define AFEC_TEMPCWR_MASK                   (0xFFFFFFFFU)                                  /**< \deprecated (AFEC_TEMPCWR) Register MASK  (Use AFEC_TEMPCWR_Msk instead)  */
#define AFEC_TEMPCWR_Msk                    (0xFFFFFFFFU)                                  /**< (AFEC_TEMPCWR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_ACR : (AFEC Offset: 0x94) (R/W 32) AFEC Analog Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :2;                        /**< bit:   0..1  Reserved */
    uint32_t PGA0EN:1;                  /**< bit:      2  PGA0 Enable                              */
    uint32_t PGA1EN:1;                  /**< bit:      3  PGA1 Enable                              */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t IBCTL:2;                   /**< bit:   8..9  AFE Bias Current Control                 */
    uint32_t :22;                       /**< bit: 10..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_ACR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_ACR_OFFSET                     (0x94)                                        /**<  (AFEC_ACR) AFEC Analog Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_ACR_PGA0EN_Pos                 2                                              /**< (AFEC_ACR) PGA0 Enable Position */
#define AFEC_ACR_PGA0EN_Msk                 (0x1U << AFEC_ACR_PGA0EN_Pos)                  /**< (AFEC_ACR) PGA0 Enable Mask */
#define AFEC_ACR_PGA0EN                     AFEC_ACR_PGA0EN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ACR_PGA0EN_Msk instead */
#define AFEC_ACR_PGA1EN_Pos                 3                                              /**< (AFEC_ACR) PGA1 Enable Position */
#define AFEC_ACR_PGA1EN_Msk                 (0x1U << AFEC_ACR_PGA1EN_Pos)                  /**< (AFEC_ACR) PGA1 Enable Mask */
#define AFEC_ACR_PGA1EN                     AFEC_ACR_PGA1EN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_ACR_PGA1EN_Msk instead */
#define AFEC_ACR_IBCTL_Pos                  8                                              /**< (AFEC_ACR) AFE Bias Current Control Position */
#define AFEC_ACR_IBCTL_Msk                  (0x3U << AFEC_ACR_IBCTL_Pos)                   /**< (AFEC_ACR) AFE Bias Current Control Mask */
#define AFEC_ACR_IBCTL(value)               (AFEC_ACR_IBCTL_Msk & ((value) << AFEC_ACR_IBCTL_Pos))
#define AFEC_ACR_MASK                       (0x30CU)                                       /**< \deprecated (AFEC_ACR) Register MASK  (Use AFEC_ACR_Msk instead)  */
#define AFEC_ACR_Msk                        (0x30CU)                                       /**< (AFEC_ACR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_SHMR : (AFEC Offset: 0xa0) (R/W 32) AFEC Sample & Hold Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DUAL0:1;                   /**< bit:      0  Dual Sample & Hold for channel 0         */
    uint32_t DUAL1:1;                   /**< bit:      1  Dual Sample & Hold for channel 1         */
    uint32_t DUAL2:1;                   /**< bit:      2  Dual Sample & Hold for channel 2         */
    uint32_t DUAL3:1;                   /**< bit:      3  Dual Sample & Hold for channel 3         */
    uint32_t DUAL4:1;                   /**< bit:      4  Dual Sample & Hold for channel 4         */
    uint32_t DUAL5:1;                   /**< bit:      5  Dual Sample & Hold for channel 5         */
    uint32_t DUAL6:1;                   /**< bit:      6  Dual Sample & Hold for channel 6         */
    uint32_t DUAL7:1;                   /**< bit:      7  Dual Sample & Hold for channel 7         */
    uint32_t DUAL8:1;                   /**< bit:      8  Dual Sample & Hold for channel 8         */
    uint32_t DUAL9:1;                   /**< bit:      9  Dual Sample & Hold for channel 9         */
    uint32_t DUAL10:1;                  /**< bit:     10  Dual Sample & Hold for channel 10        */
    uint32_t DUAL11:1;                  /**< bit:     11  Dual Sample & Hold for channel 11        */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t DUAL:12;                   /**< bit:  0..11  Dual Sample & Hold for channel xx        */
    uint32_t :20;                       /**< bit: 12..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_SHMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_SHMR_OFFSET                    (0xA0)                                        /**<  (AFEC_SHMR) AFEC Sample & Hold Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_SHMR_DUAL0_Pos                 0                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 0 Position */
#define AFEC_SHMR_DUAL0_Msk                 (0x1U << AFEC_SHMR_DUAL0_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 0 Mask */
#define AFEC_SHMR_DUAL0                     AFEC_SHMR_DUAL0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL0_Msk instead */
#define AFEC_SHMR_DUAL1_Pos                 1                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 1 Position */
#define AFEC_SHMR_DUAL1_Msk                 (0x1U << AFEC_SHMR_DUAL1_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 1 Mask */
#define AFEC_SHMR_DUAL1                     AFEC_SHMR_DUAL1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL1_Msk instead */
#define AFEC_SHMR_DUAL2_Pos                 2                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 2 Position */
#define AFEC_SHMR_DUAL2_Msk                 (0x1U << AFEC_SHMR_DUAL2_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 2 Mask */
#define AFEC_SHMR_DUAL2                     AFEC_SHMR_DUAL2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL2_Msk instead */
#define AFEC_SHMR_DUAL3_Pos                 3                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 3 Position */
#define AFEC_SHMR_DUAL3_Msk                 (0x1U << AFEC_SHMR_DUAL3_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 3 Mask */
#define AFEC_SHMR_DUAL3                     AFEC_SHMR_DUAL3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL3_Msk instead */
#define AFEC_SHMR_DUAL4_Pos                 4                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 4 Position */
#define AFEC_SHMR_DUAL4_Msk                 (0x1U << AFEC_SHMR_DUAL4_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 4 Mask */
#define AFEC_SHMR_DUAL4                     AFEC_SHMR_DUAL4_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL4_Msk instead */
#define AFEC_SHMR_DUAL5_Pos                 5                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 5 Position */
#define AFEC_SHMR_DUAL5_Msk                 (0x1U << AFEC_SHMR_DUAL5_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 5 Mask */
#define AFEC_SHMR_DUAL5                     AFEC_SHMR_DUAL5_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL5_Msk instead */
#define AFEC_SHMR_DUAL6_Pos                 6                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 6 Position */
#define AFEC_SHMR_DUAL6_Msk                 (0x1U << AFEC_SHMR_DUAL6_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 6 Mask */
#define AFEC_SHMR_DUAL6                     AFEC_SHMR_DUAL6_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL6_Msk instead */
#define AFEC_SHMR_DUAL7_Pos                 7                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 7 Position */
#define AFEC_SHMR_DUAL7_Msk                 (0x1U << AFEC_SHMR_DUAL7_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 7 Mask */
#define AFEC_SHMR_DUAL7                     AFEC_SHMR_DUAL7_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL7_Msk instead */
#define AFEC_SHMR_DUAL8_Pos                 8                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 8 Position */
#define AFEC_SHMR_DUAL8_Msk                 (0x1U << AFEC_SHMR_DUAL8_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 8 Mask */
#define AFEC_SHMR_DUAL8                     AFEC_SHMR_DUAL8_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL8_Msk instead */
#define AFEC_SHMR_DUAL9_Pos                 9                                              /**< (AFEC_SHMR) Dual Sample & Hold for channel 9 Position */
#define AFEC_SHMR_DUAL9_Msk                 (0x1U << AFEC_SHMR_DUAL9_Pos)                  /**< (AFEC_SHMR) Dual Sample & Hold for channel 9 Mask */
#define AFEC_SHMR_DUAL9                     AFEC_SHMR_DUAL9_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL9_Msk instead */
#define AFEC_SHMR_DUAL10_Pos                10                                             /**< (AFEC_SHMR) Dual Sample & Hold for channel 10 Position */
#define AFEC_SHMR_DUAL10_Msk                (0x1U << AFEC_SHMR_DUAL10_Pos)                 /**< (AFEC_SHMR) Dual Sample & Hold for channel 10 Mask */
#define AFEC_SHMR_DUAL10                    AFEC_SHMR_DUAL10_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL10_Msk instead */
#define AFEC_SHMR_DUAL11_Pos                11                                             /**< (AFEC_SHMR) Dual Sample & Hold for channel 11 Position */
#define AFEC_SHMR_DUAL11_Msk                (0x1U << AFEC_SHMR_DUAL11_Pos)                 /**< (AFEC_SHMR) Dual Sample & Hold for channel 11 Mask */
#define AFEC_SHMR_DUAL11                    AFEC_SHMR_DUAL11_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_SHMR_DUAL11_Msk instead */
#define AFEC_SHMR_DUAL_Pos                  0                                              /**< (AFEC_SHMR Position) Dual Sample & Hold for channel xx */
#define AFEC_SHMR_DUAL_Msk                  (0xFFFU << AFEC_SHMR_DUAL_Pos)                 /**< (AFEC_SHMR Mask) DUAL */
#define AFEC_SHMR_DUAL(value)               (AFEC_SHMR_DUAL_Msk & ((value) << AFEC_SHMR_DUAL_Pos))  
#define AFEC_SHMR_MASK                      (0xFFFU)                                       /**< \deprecated (AFEC_SHMR) Register MASK  (Use AFEC_SHMR_Msk instead)  */
#define AFEC_SHMR_Msk                       (0xFFFU)                                       /**< (AFEC_SHMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_COSR : (AFEC Offset: 0xd0) (R/W 32) AFEC Correction Select Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CSEL:1;                    /**< bit:      0  Sample & Hold unit Correction Select     */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_COSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_COSR_OFFSET                    (0xD0)                                        /**<  (AFEC_COSR) AFEC Correction Select Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_COSR_CSEL_Pos                  0                                              /**< (AFEC_COSR) Sample & Hold unit Correction Select Position */
#define AFEC_COSR_CSEL_Msk                  (0x1U << AFEC_COSR_CSEL_Pos)                   /**< (AFEC_COSR) Sample & Hold unit Correction Select Mask */
#define AFEC_COSR_CSEL                      AFEC_COSR_CSEL_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_COSR_CSEL_Msk instead */
#define AFEC_COSR_MASK                      (0x01U)                                        /**< \deprecated (AFEC_COSR) Register MASK  (Use AFEC_COSR_Msk instead)  */
#define AFEC_COSR_Msk                       (0x01U)                                        /**< (AFEC_COSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CVR : (AFEC Offset: 0xd4) (R/W 32) AFEC Correction Values Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OFFSETCORR:16;             /**< bit:  0..15  Offset Correction                        */
    uint32_t GAINCORR:16;               /**< bit: 16..31  Gain Correction                          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CVR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CVR_OFFSET                     (0xD4)                                        /**<  (AFEC_CVR) AFEC Correction Values Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CVR_OFFSETCORR_Pos             0                                              /**< (AFEC_CVR) Offset Correction Position */
#define AFEC_CVR_OFFSETCORR_Msk             (0xFFFFU << AFEC_CVR_OFFSETCORR_Pos)           /**< (AFEC_CVR) Offset Correction Mask */
#define AFEC_CVR_OFFSETCORR(value)          (AFEC_CVR_OFFSETCORR_Msk & ((value) << AFEC_CVR_OFFSETCORR_Pos))
#define AFEC_CVR_GAINCORR_Pos               16                                             /**< (AFEC_CVR) Gain Correction Position */
#define AFEC_CVR_GAINCORR_Msk               (0xFFFFU << AFEC_CVR_GAINCORR_Pos)             /**< (AFEC_CVR) Gain Correction Mask */
#define AFEC_CVR_GAINCORR(value)            (AFEC_CVR_GAINCORR_Msk & ((value) << AFEC_CVR_GAINCORR_Pos))
#define AFEC_CVR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (AFEC_CVR) Register MASK  (Use AFEC_CVR_Msk instead)  */
#define AFEC_CVR_Msk                        (0xFFFFFFFFU)                                  /**< (AFEC_CVR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_CECR : (AFEC Offset: 0xd8) (R/W 32) AFEC Channel Error Correction Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ECORR0:1;                  /**< bit:      0  Error Correction Enable for channel 0    */
    uint32_t ECORR1:1;                  /**< bit:      1  Error Correction Enable for channel 1    */
    uint32_t ECORR2:1;                  /**< bit:      2  Error Correction Enable for channel 2    */
    uint32_t ECORR3:1;                  /**< bit:      3  Error Correction Enable for channel 3    */
    uint32_t ECORR4:1;                  /**< bit:      4  Error Correction Enable for channel 4    */
    uint32_t ECORR5:1;                  /**< bit:      5  Error Correction Enable for channel 5    */
    uint32_t ECORR6:1;                  /**< bit:      6  Error Correction Enable for channel 6    */
    uint32_t ECORR7:1;                  /**< bit:      7  Error Correction Enable for channel 7    */
    uint32_t ECORR8:1;                  /**< bit:      8  Error Correction Enable for channel 8    */
    uint32_t ECORR9:1;                  /**< bit:      9  Error Correction Enable for channel 9    */
    uint32_t ECORR10:1;                 /**< bit:     10  Error Correction Enable for channel 10   */
    uint32_t ECORR11:1;                 /**< bit:     11  Error Correction Enable for channel 11   */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t ECORR:12;                  /**< bit:  0..11  Error Correction Enable for channel xx   */
    uint32_t :20;                       /**< bit: 12..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_CECR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_CECR_OFFSET                    (0xD8)                                        /**<  (AFEC_CECR) AFEC Channel Error Correction Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_CECR_ECORR0_Pos                0                                              /**< (AFEC_CECR) Error Correction Enable for channel 0 Position */
#define AFEC_CECR_ECORR0_Msk                (0x1U << AFEC_CECR_ECORR0_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 0 Mask */
#define AFEC_CECR_ECORR0                    AFEC_CECR_ECORR0_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR0_Msk instead */
#define AFEC_CECR_ECORR1_Pos                1                                              /**< (AFEC_CECR) Error Correction Enable for channel 1 Position */
#define AFEC_CECR_ECORR1_Msk                (0x1U << AFEC_CECR_ECORR1_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 1 Mask */
#define AFEC_CECR_ECORR1                    AFEC_CECR_ECORR1_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR1_Msk instead */
#define AFEC_CECR_ECORR2_Pos                2                                              /**< (AFEC_CECR) Error Correction Enable for channel 2 Position */
#define AFEC_CECR_ECORR2_Msk                (0x1U << AFEC_CECR_ECORR2_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 2 Mask */
#define AFEC_CECR_ECORR2                    AFEC_CECR_ECORR2_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR2_Msk instead */
#define AFEC_CECR_ECORR3_Pos                3                                              /**< (AFEC_CECR) Error Correction Enable for channel 3 Position */
#define AFEC_CECR_ECORR3_Msk                (0x1U << AFEC_CECR_ECORR3_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 3 Mask */
#define AFEC_CECR_ECORR3                    AFEC_CECR_ECORR3_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR3_Msk instead */
#define AFEC_CECR_ECORR4_Pos                4                                              /**< (AFEC_CECR) Error Correction Enable for channel 4 Position */
#define AFEC_CECR_ECORR4_Msk                (0x1U << AFEC_CECR_ECORR4_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 4 Mask */
#define AFEC_CECR_ECORR4                    AFEC_CECR_ECORR4_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR4_Msk instead */
#define AFEC_CECR_ECORR5_Pos                5                                              /**< (AFEC_CECR) Error Correction Enable for channel 5 Position */
#define AFEC_CECR_ECORR5_Msk                (0x1U << AFEC_CECR_ECORR5_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 5 Mask */
#define AFEC_CECR_ECORR5                    AFEC_CECR_ECORR5_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR5_Msk instead */
#define AFEC_CECR_ECORR6_Pos                6                                              /**< (AFEC_CECR) Error Correction Enable for channel 6 Position */
#define AFEC_CECR_ECORR6_Msk                (0x1U << AFEC_CECR_ECORR6_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 6 Mask */
#define AFEC_CECR_ECORR6                    AFEC_CECR_ECORR6_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR6_Msk instead */
#define AFEC_CECR_ECORR7_Pos                7                                              /**< (AFEC_CECR) Error Correction Enable for channel 7 Position */
#define AFEC_CECR_ECORR7_Msk                (0x1U << AFEC_CECR_ECORR7_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 7 Mask */
#define AFEC_CECR_ECORR7                    AFEC_CECR_ECORR7_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR7_Msk instead */
#define AFEC_CECR_ECORR8_Pos                8                                              /**< (AFEC_CECR) Error Correction Enable for channel 8 Position */
#define AFEC_CECR_ECORR8_Msk                (0x1U << AFEC_CECR_ECORR8_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 8 Mask */
#define AFEC_CECR_ECORR8                    AFEC_CECR_ECORR8_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR8_Msk instead */
#define AFEC_CECR_ECORR9_Pos                9                                              /**< (AFEC_CECR) Error Correction Enable for channel 9 Position */
#define AFEC_CECR_ECORR9_Msk                (0x1U << AFEC_CECR_ECORR9_Pos)                 /**< (AFEC_CECR) Error Correction Enable for channel 9 Mask */
#define AFEC_CECR_ECORR9                    AFEC_CECR_ECORR9_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR9_Msk instead */
#define AFEC_CECR_ECORR10_Pos               10                                             /**< (AFEC_CECR) Error Correction Enable for channel 10 Position */
#define AFEC_CECR_ECORR10_Msk               (0x1U << AFEC_CECR_ECORR10_Pos)                /**< (AFEC_CECR) Error Correction Enable for channel 10 Mask */
#define AFEC_CECR_ECORR10                   AFEC_CECR_ECORR10_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR10_Msk instead */
#define AFEC_CECR_ECORR11_Pos               11                                             /**< (AFEC_CECR) Error Correction Enable for channel 11 Position */
#define AFEC_CECR_ECORR11_Msk               (0x1U << AFEC_CECR_ECORR11_Pos)                /**< (AFEC_CECR) Error Correction Enable for channel 11 Mask */
#define AFEC_CECR_ECORR11                   AFEC_CECR_ECORR11_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_CECR_ECORR11_Msk instead */
#define AFEC_CECR_ECORR_Pos                 0                                              /**< (AFEC_CECR Position) Error Correction Enable for channel xx */
#define AFEC_CECR_ECORR_Msk                 (0xFFFU << AFEC_CECR_ECORR_Pos)                /**< (AFEC_CECR Mask) ECORR */
#define AFEC_CECR_ECORR(value)              (AFEC_CECR_ECORR_Msk & ((value) << AFEC_CECR_ECORR_Pos))  
#define AFEC_CECR_MASK                      (0xFFFU)                                       /**< \deprecated (AFEC_CECR) Register MASK  (Use AFEC_CECR_Msk instead)  */
#define AFEC_CECR_Msk                       (0xFFFU)                                       /**< (AFEC_CECR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_WPMR : (AFEC Offset: 0xe4) (R/W 32) AFEC Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protect KEY                        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_WPMR_OFFSET                    (0xE4)                                        /**<  (AFEC_WPMR) AFEC Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_WPMR_WPEN_Pos                  0                                              /**< (AFEC_WPMR) Write Protection Enable Position */
#define AFEC_WPMR_WPEN_Msk                  (0x1U << AFEC_WPMR_WPEN_Pos)                   /**< (AFEC_WPMR) Write Protection Enable Mask */
#define AFEC_WPMR_WPEN                      AFEC_WPMR_WPEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_WPMR_WPEN_Msk instead */
#define AFEC_WPMR_WPKEY_Pos                 8                                              /**< (AFEC_WPMR) Write Protect KEY Position */
#define AFEC_WPMR_WPKEY_Msk                 (0xFFFFFFU << AFEC_WPMR_WPKEY_Pos)             /**< (AFEC_WPMR) Write Protect KEY Mask */
#define AFEC_WPMR_WPKEY(value)              (AFEC_WPMR_WPKEY_Msk & ((value) << AFEC_WPMR_WPKEY_Pos))
#define   AFEC_WPMR_WPKEY_PASSWD_Val        (0x414443U)                                    /**< (AFEC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0.  */
#define AFEC_WPMR_WPKEY_PASSWD              (AFEC_WPMR_WPKEY_PASSWD_Val << AFEC_WPMR_WPKEY_Pos)  /**< (AFEC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0. Position  */
#define AFEC_WPMR_MASK                      (0xFFFFFF01U)                                  /**< \deprecated (AFEC_WPMR) Register MASK  (Use AFEC_WPMR_Msk instead)  */
#define AFEC_WPMR_Msk                       (0xFFFFFF01U)                                  /**< (AFEC_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AFEC_WPSR : (AFEC Offset: 0xe8) (R/ 32) AFEC Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protect Violation Status           */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:16;                 /**< bit:  8..23  Write Protect Violation Source           */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AFEC_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AFEC_WPSR_OFFSET                    (0xE8)                                        /**<  (AFEC_WPSR) AFEC Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AFEC_WPSR_WPVS_Pos                  0                                              /**< (AFEC_WPSR) Write Protect Violation Status Position */
#define AFEC_WPSR_WPVS_Msk                  (0x1U << AFEC_WPSR_WPVS_Pos)                   /**< (AFEC_WPSR) Write Protect Violation Status Mask */
#define AFEC_WPSR_WPVS                      AFEC_WPSR_WPVS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AFEC_WPSR_WPVS_Msk instead */
#define AFEC_WPSR_WPVSRC_Pos                8                                              /**< (AFEC_WPSR) Write Protect Violation Source Position */
#define AFEC_WPSR_WPVSRC_Msk                (0xFFFFU << AFEC_WPSR_WPVSRC_Pos)              /**< (AFEC_WPSR) Write Protect Violation Source Mask */
#define AFEC_WPSR_WPVSRC(value)             (AFEC_WPSR_WPVSRC_Msk & ((value) << AFEC_WPSR_WPVSRC_Pos))
#define AFEC_WPSR_MASK                      (0xFFFF01U)                                    /**< \deprecated (AFEC_WPSR) Register MASK  (Use AFEC_WPSR_Msk instead)  */
#define AFEC_WPSR_Msk                       (0xFFFF01U)                                    /**< (AFEC_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief AFEC hardware registers */
typedef struct {  
  __O  uint32_t AFEC_CR;        /**< (AFEC Offset: 0x00) AFEC Control Register */
  __IO uint32_t AFEC_MR;        /**< (AFEC Offset: 0x04) AFEC Mode Register */
  __IO uint32_t AFEC_EMR;       /**< (AFEC Offset: 0x08) AFEC Extended Mode Register */
  __IO uint32_t AFEC_SEQ1R;     /**< (AFEC Offset: 0x0C) AFEC Channel Sequence 1 Register */
  __IO uint32_t AFEC_SEQ2R;     /**< (AFEC Offset: 0x10) AFEC Channel Sequence 2 Register */
  __O  uint32_t AFEC_CHER;      /**< (AFEC Offset: 0x14) AFEC Channel Enable Register */
  __O  uint32_t AFEC_CHDR;      /**< (AFEC Offset: 0x18) AFEC Channel Disable Register */
  __I  uint32_t AFEC_CHSR;      /**< (AFEC Offset: 0x1C) AFEC Channel Status Register */
  __I  uint32_t AFEC_LCDR;      /**< (AFEC Offset: 0x20) AFEC Last Converted Data Register */
  __O  uint32_t AFEC_IER;       /**< (AFEC Offset: 0x24) AFEC Interrupt Enable Register */
  __O  uint32_t AFEC_IDR;       /**< (AFEC Offset: 0x28) AFEC Interrupt Disable Register */
  __I  uint32_t AFEC_IMR;       /**< (AFEC Offset: 0x2C) AFEC Interrupt Mask Register */
  __I  uint32_t AFEC_ISR;       /**< (AFEC Offset: 0x30) AFEC Interrupt Status Register */
  __I  uint32_t Reserved1[6];
  __I  uint32_t AFEC_OVER;      /**< (AFEC Offset: 0x4C) AFEC Overrun Status Register */
  __IO uint32_t AFEC_CWR;       /**< (AFEC Offset: 0x50) AFEC Compare Window Register */
  __IO uint32_t AFEC_CGR;       /**< (AFEC Offset: 0x54) AFEC Channel Gain Register */
  __I  uint32_t Reserved2[2];
  __IO uint32_t AFEC_DIFFR;     /**< (AFEC Offset: 0x60) AFEC Channel Differential Register */
  __IO uint32_t AFEC_CSELR;     /**< (AFEC Offset: 0x64) AFEC Channel Selection Register */
  __I  uint32_t AFEC_CDR;       /**< (AFEC Offset: 0x68) AFEC Channel Data Register */
  __IO uint32_t AFEC_COCR;      /**< (AFEC Offset: 0x6C) AFEC Channel Offset Compensation Register */
  __IO uint32_t AFEC_TEMPMR;    /**< (AFEC Offset: 0x70) AFEC Temperature Sensor Mode Register */
  __IO uint32_t AFEC_TEMPCWR;   /**< (AFEC Offset: 0x74) AFEC Temperature Compare Window Register */
  __I  uint32_t Reserved3[7];
  __IO uint32_t AFEC_ACR;       /**< (AFEC Offset: 0x94) AFEC Analog Control Register */
  __I  uint32_t Reserved4[2];
  __IO uint32_t AFEC_SHMR;      /**< (AFEC Offset: 0xA0) AFEC Sample & Hold Mode Register */
  __I  uint32_t Reserved5[11];
  __IO uint32_t AFEC_COSR;      /**< (AFEC Offset: 0xD0) AFEC Correction Select Register */
  __IO uint32_t AFEC_CVR;       /**< (AFEC Offset: 0xD4) AFEC Correction Values Register */
  __IO uint32_t AFEC_CECR;      /**< (AFEC Offset: 0xD8) AFEC Channel Error Correction Register */
  __I  uint32_t Reserved6[2];
  __IO uint32_t AFEC_WPMR;      /**< (AFEC Offset: 0xE4) AFEC Write Protection Mode Register */
  __I  uint32_t AFEC_WPSR;      /**< (AFEC Offset: 0xE8) AFEC Write Protection Status Register */
} Afec;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief AFEC hardware registers */
typedef struct {  
  __O  AFEC_CR_Type                   AFEC_CR;        /**< Offset: 0x00 ( /W  32) AFEC Control Register */
  __IO AFEC_MR_Type                   AFEC_MR;        /**< Offset: 0x04 (R/W  32) AFEC Mode Register */
  __IO AFEC_EMR_Type                  AFEC_EMR;       /**< Offset: 0x08 (R/W  32) AFEC Extended Mode Register */
  __IO AFEC_SEQ1R_Type                AFEC_SEQ1R;     /**< Offset: 0x0C (R/W  32) AFEC Channel Sequence 1 Register */
  __IO AFEC_SEQ2R_Type                AFEC_SEQ2R;     /**< Offset: 0x10 (R/W  32) AFEC Channel Sequence 2 Register */
  __O  AFEC_CHER_Type                 AFEC_CHER;      /**< Offset: 0x14 ( /W  32) AFEC Channel Enable Register */
  __O  AFEC_CHDR_Type                 AFEC_CHDR;      /**< Offset: 0x18 ( /W  32) AFEC Channel Disable Register */
  __I  AFEC_CHSR_Type                 AFEC_CHSR;      /**< Offset: 0x1C (R/   32) AFEC Channel Status Register */
  __I  AFEC_LCDR_Type                 AFEC_LCDR;      /**< Offset: 0x20 (R/   32) AFEC Last Converted Data Register */
  __O  AFEC_IER_Type                  AFEC_IER;       /**< Offset: 0x24 ( /W  32) AFEC Interrupt Enable Register */
  __O  AFEC_IDR_Type                  AFEC_IDR;       /**< Offset: 0x28 ( /W  32) AFEC Interrupt Disable Register */
  __I  AFEC_IMR_Type                  AFEC_IMR;       /**< Offset: 0x2C (R/   32) AFEC Interrupt Mask Register */
  __I  AFEC_ISR_Type                  AFEC_ISR;       /**< Offset: 0x30 (R/   32) AFEC Interrupt Status Register */
       RoReg8                         Reserved1[0x18];
  __I  AFEC_OVER_Type                 AFEC_OVER;      /**< Offset: 0x4C (R/   32) AFEC Overrun Status Register */
  __IO AFEC_CWR_Type                  AFEC_CWR;       /**< Offset: 0x50 (R/W  32) AFEC Compare Window Register */
  __IO AFEC_CGR_Type                  AFEC_CGR;       /**< Offset: 0x54 (R/W  32) AFEC Channel Gain Register */
       RoReg8                         Reserved2[0x8];
  __IO AFEC_DIFFR_Type                AFEC_DIFFR;     /**< Offset: 0x60 (R/W  32) AFEC Channel Differential Register */
  __IO AFEC_CSELR_Type                AFEC_CSELR;     /**< Offset: 0x64 (R/W  32) AFEC Channel Selection Register */
  __I  AFEC_CDR_Type                  AFEC_CDR;       /**< Offset: 0x68 (R/   32) AFEC Channel Data Register */
  __IO AFEC_COCR_Type                 AFEC_COCR;      /**< Offset: 0x6C (R/W  32) AFEC Channel Offset Compensation Register */
  __IO AFEC_TEMPMR_Type               AFEC_TEMPMR;    /**< Offset: 0x70 (R/W  32) AFEC Temperature Sensor Mode Register */
  __IO AFEC_TEMPCWR_Type              AFEC_TEMPCWR;   /**< Offset: 0x74 (R/W  32) AFEC Temperature Compare Window Register */
       RoReg8                         Reserved3[0x1C];
  __IO AFEC_ACR_Type                  AFEC_ACR;       /**< Offset: 0x94 (R/W  32) AFEC Analog Control Register */
       RoReg8                         Reserved4[0x8];
  __IO AFEC_SHMR_Type                 AFEC_SHMR;      /**< Offset: 0xA0 (R/W  32) AFEC Sample & Hold Mode Register */
       RoReg8                         Reserved5[0x2C];
  __IO AFEC_COSR_Type                 AFEC_COSR;      /**< Offset: 0xD0 (R/W  32) AFEC Correction Select Register */
  __IO AFEC_CVR_Type                  AFEC_CVR;       /**< Offset: 0xD4 (R/W  32) AFEC Correction Values Register */
  __IO AFEC_CECR_Type                 AFEC_CECR;      /**< Offset: 0xD8 (R/W  32) AFEC Channel Error Correction Register */
       RoReg8                         Reserved6[0x8];
  __IO AFEC_WPMR_Type                 AFEC_WPMR;      /**< Offset: 0xE4 (R/W  32) AFEC Write Protection Mode Register */
  __I  AFEC_WPSR_Type                 AFEC_WPSR;      /**< Offset: 0xE8 (R/   32) AFEC Write Protection Status Register */
} Afec;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Analog Front-End Controller */

#endif /* _SAME70_AFEC_COMPONENT_H_ */
