/**
 * \file
 *
 * \brief Component description for PWM
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

#ifndef _SAME70_PWM_COMPONENT_H_
#define _SAME70_PWM_COMPONENT_H_
#define _SAME70_PWM_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_PWM Pulse Width Modulation Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR PWM */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define PWM_6343                       /**< (PWM) Module ID */
#define REV_PWM U                      /**< (PWM) Module revision */

/* -------- PWM_CMR : (PWM Offset: 0x00) (R/W 32) PWM Channel Mode Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CPRE:4;                    /**< bit:   0..3  Channel Pre-scaler                       */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CALG:1;                    /**< bit:      8  Channel Alignment                        */
    uint32_t CPOL:1;                    /**< bit:      9  Channel Polarity                         */
    uint32_t CES:1;                     /**< bit:     10  Counter Event Selection                  */
    uint32_t UPDS:1;                    /**< bit:     11  Update Selection                         */
    uint32_t DPOLI:1;                   /**< bit:     12  Disabled Polarity Inverted               */
    uint32_t TCTS:1;                    /**< bit:     13  Timer Counter Trigger Selection          */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t DTE:1;                     /**< bit:     16  Dead-Time Generator Enable               */
    uint32_t DTHI:1;                    /**< bit:     17  Dead-Time PWMHx Output Inverted          */
    uint32_t DTLI:1;                    /**< bit:     18  Dead-Time PWMLx Output Inverted          */
    uint32_t PPM:1;                     /**< bit:     19  Push-Pull Mode                           */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMR_OFFSET                      (0x00)                                        /**<  (PWM_CMR) PWM Channel Mode Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMR_CPRE_Pos                    0                                              /**< (PWM_CMR) Channel Pre-scaler Position */
#define PWM_CMR_CPRE_Msk                    (0xFU << PWM_CMR_CPRE_Pos)                     /**< (PWM_CMR) Channel Pre-scaler Mask */
#define PWM_CMR_CPRE(value)                 (PWM_CMR_CPRE_Msk & ((value) << PWM_CMR_CPRE_Pos))
#define   PWM_CMR_CPRE_MCK_Val              (0x0U)                                         /**< (PWM_CMR) Peripheral clock  */
#define   PWM_CMR_CPRE_MCK_DIV_2_Val        (0x1U)                                         /**< (PWM_CMR) Peripheral clock/2  */
#define   PWM_CMR_CPRE_MCK_DIV_4_Val        (0x2U)                                         /**< (PWM_CMR) Peripheral clock/4  */
#define   PWM_CMR_CPRE_MCK_DIV_8_Val        (0x3U)                                         /**< (PWM_CMR) Peripheral clock/8  */
#define   PWM_CMR_CPRE_MCK_DIV_16_Val       (0x4U)                                         /**< (PWM_CMR) Peripheral clock/16  */
#define   PWM_CMR_CPRE_MCK_DIV_32_Val       (0x5U)                                         /**< (PWM_CMR) Peripheral clock/32  */
#define   PWM_CMR_CPRE_MCK_DIV_64_Val       (0x6U)                                         /**< (PWM_CMR) Peripheral clock/64  */
#define   PWM_CMR_CPRE_MCK_DIV_128_Val      (0x7U)                                         /**< (PWM_CMR) Peripheral clock/128  */
#define   PWM_CMR_CPRE_MCK_DIV_256_Val      (0x8U)                                         /**< (PWM_CMR) Peripheral clock/256  */
#define   PWM_CMR_CPRE_MCK_DIV_512_Val      (0x9U)                                         /**< (PWM_CMR) Peripheral clock/512  */
#define   PWM_CMR_CPRE_MCK_DIV_1024_Val     (0xAU)                                         /**< (PWM_CMR) Peripheral clock/1024  */
#define   PWM_CMR_CPRE_CLKA_Val             (0xBU)                                         /**< (PWM_CMR) Clock A  */
#define   PWM_CMR_CPRE_CLKB_Val             (0xCU)                                         /**< (PWM_CMR) Clock B  */
#define PWM_CMR_CPRE_MCK                    (PWM_CMR_CPRE_MCK_Val << PWM_CMR_CPRE_Pos)     /**< (PWM_CMR) Peripheral clock Position  */
#define PWM_CMR_CPRE_MCK_DIV_2              (PWM_CMR_CPRE_MCK_DIV_2_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/2 Position  */
#define PWM_CMR_CPRE_MCK_DIV_4              (PWM_CMR_CPRE_MCK_DIV_4_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/4 Position  */
#define PWM_CMR_CPRE_MCK_DIV_8              (PWM_CMR_CPRE_MCK_DIV_8_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/8 Position  */
#define PWM_CMR_CPRE_MCK_DIV_16             (PWM_CMR_CPRE_MCK_DIV_16_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/16 Position  */
#define PWM_CMR_CPRE_MCK_DIV_32             (PWM_CMR_CPRE_MCK_DIV_32_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/32 Position  */
#define PWM_CMR_CPRE_MCK_DIV_64             (PWM_CMR_CPRE_MCK_DIV_64_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/64 Position  */
#define PWM_CMR_CPRE_MCK_DIV_128            (PWM_CMR_CPRE_MCK_DIV_128_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/128 Position  */
#define PWM_CMR_CPRE_MCK_DIV_256            (PWM_CMR_CPRE_MCK_DIV_256_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/256 Position  */
#define PWM_CMR_CPRE_MCK_DIV_512            (PWM_CMR_CPRE_MCK_DIV_512_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/512 Position  */
#define PWM_CMR_CPRE_MCK_DIV_1024           (PWM_CMR_CPRE_MCK_DIV_1024_Val << PWM_CMR_CPRE_Pos)  /**< (PWM_CMR) Peripheral clock/1024 Position  */
#define PWM_CMR_CPRE_CLKA                   (PWM_CMR_CPRE_CLKA_Val << PWM_CMR_CPRE_Pos)    /**< (PWM_CMR) Clock A Position  */
#define PWM_CMR_CPRE_CLKB                   (PWM_CMR_CPRE_CLKB_Val << PWM_CMR_CPRE_Pos)    /**< (PWM_CMR) Clock B Position  */
#define PWM_CMR_CALG_Pos                    8                                              /**< (PWM_CMR) Channel Alignment Position */
#define PWM_CMR_CALG_Msk                    (0x1U << PWM_CMR_CALG_Pos)                     /**< (PWM_CMR) Channel Alignment Mask */
#define PWM_CMR_CALG                        PWM_CMR_CALG_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_CALG_Msk instead */
#define PWM_CMR_CPOL_Pos                    9                                              /**< (PWM_CMR) Channel Polarity Position */
#define PWM_CMR_CPOL_Msk                    (0x1U << PWM_CMR_CPOL_Pos)                     /**< (PWM_CMR) Channel Polarity Mask */
#define PWM_CMR_CPOL                        PWM_CMR_CPOL_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_CPOL_Msk instead */
#define PWM_CMR_CES_Pos                     10                                             /**< (PWM_CMR) Counter Event Selection Position */
#define PWM_CMR_CES_Msk                     (0x1U << PWM_CMR_CES_Pos)                      /**< (PWM_CMR) Counter Event Selection Mask */
#define PWM_CMR_CES                         PWM_CMR_CES_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_CES_Msk instead */
#define PWM_CMR_UPDS_Pos                    11                                             /**< (PWM_CMR) Update Selection Position */
#define PWM_CMR_UPDS_Msk                    (0x1U << PWM_CMR_UPDS_Pos)                     /**< (PWM_CMR) Update Selection Mask */
#define PWM_CMR_UPDS                        PWM_CMR_UPDS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_UPDS_Msk instead */
#define PWM_CMR_DPOLI_Pos                   12                                             /**< (PWM_CMR) Disabled Polarity Inverted Position */
#define PWM_CMR_DPOLI_Msk                   (0x1U << PWM_CMR_DPOLI_Pos)                    /**< (PWM_CMR) Disabled Polarity Inverted Mask */
#define PWM_CMR_DPOLI                       PWM_CMR_DPOLI_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_DPOLI_Msk instead */
#define PWM_CMR_TCTS_Pos                    13                                             /**< (PWM_CMR) Timer Counter Trigger Selection Position */
#define PWM_CMR_TCTS_Msk                    (0x1U << PWM_CMR_TCTS_Pos)                     /**< (PWM_CMR) Timer Counter Trigger Selection Mask */
#define PWM_CMR_TCTS                        PWM_CMR_TCTS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_TCTS_Msk instead */
#define PWM_CMR_DTE_Pos                     16                                             /**< (PWM_CMR) Dead-Time Generator Enable Position */
#define PWM_CMR_DTE_Msk                     (0x1U << PWM_CMR_DTE_Pos)                      /**< (PWM_CMR) Dead-Time Generator Enable Mask */
#define PWM_CMR_DTE                         PWM_CMR_DTE_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_DTE_Msk instead */
#define PWM_CMR_DTHI_Pos                    17                                             /**< (PWM_CMR) Dead-Time PWMHx Output Inverted Position */
#define PWM_CMR_DTHI_Msk                    (0x1U << PWM_CMR_DTHI_Pos)                     /**< (PWM_CMR) Dead-Time PWMHx Output Inverted Mask */
#define PWM_CMR_DTHI                        PWM_CMR_DTHI_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_DTHI_Msk instead */
#define PWM_CMR_DTLI_Pos                    18                                             /**< (PWM_CMR) Dead-Time PWMLx Output Inverted Position */
#define PWM_CMR_DTLI_Msk                    (0x1U << PWM_CMR_DTLI_Pos)                     /**< (PWM_CMR) Dead-Time PWMLx Output Inverted Mask */
#define PWM_CMR_DTLI                        PWM_CMR_DTLI_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_DTLI_Msk instead */
#define PWM_CMR_PPM_Pos                     19                                             /**< (PWM_CMR) Push-Pull Mode Position */
#define PWM_CMR_PPM_Msk                     (0x1U << PWM_CMR_PPM_Pos)                      /**< (PWM_CMR) Push-Pull Mode Mask */
#define PWM_CMR_PPM                         PWM_CMR_PPM_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMR_PPM_Msk instead */
#define PWM_CMR_MASK                        (0xF3F0FU)                                     /**< \deprecated (PWM_CMR) Register MASK  (Use PWM_CMR_Msk instead)  */
#define PWM_CMR_Msk                         (0xF3F0FU)                                     /**< (PWM_CMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CDTY : (PWM Offset: 0x04) (R/W 32) PWM Channel Duty Cycle Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CDTY:24;                   /**< bit:  0..23  Channel Duty-Cycle                       */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CDTY_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CDTY_OFFSET                     (0x04)                                        /**<  (PWM_CDTY) PWM Channel Duty Cycle Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CDTY_CDTY_Pos                   0                                              /**< (PWM_CDTY) Channel Duty-Cycle Position */
#define PWM_CDTY_CDTY_Msk                   (0xFFFFFFU << PWM_CDTY_CDTY_Pos)               /**< (PWM_CDTY) Channel Duty-Cycle Mask */
#define PWM_CDTY_CDTY(value)                (PWM_CDTY_CDTY_Msk & ((value) << PWM_CDTY_CDTY_Pos))
#define PWM_CDTY_MASK                       (0xFFFFFFU)                                    /**< \deprecated (PWM_CDTY) Register MASK  (Use PWM_CDTY_Msk instead)  */
#define PWM_CDTY_Msk                        (0xFFFFFFU)                                    /**< (PWM_CDTY) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CDTYUPD : (PWM Offset: 0x08) (/W 32) PWM Channel Duty Cycle Update Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CDTYUPD:24;                /**< bit:  0..23  Channel Duty-Cycle Update                */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CDTYUPD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CDTYUPD_OFFSET                  (0x08)                                        /**<  (PWM_CDTYUPD) PWM Channel Duty Cycle Update Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CDTYUPD_CDTYUPD_Pos             0                                              /**< (PWM_CDTYUPD) Channel Duty-Cycle Update Position */
#define PWM_CDTYUPD_CDTYUPD_Msk             (0xFFFFFFU << PWM_CDTYUPD_CDTYUPD_Pos)         /**< (PWM_CDTYUPD) Channel Duty-Cycle Update Mask */
#define PWM_CDTYUPD_CDTYUPD(value)          (PWM_CDTYUPD_CDTYUPD_Msk & ((value) << PWM_CDTYUPD_CDTYUPD_Pos))
#define PWM_CDTYUPD_MASK                    (0xFFFFFFU)                                    /**< \deprecated (PWM_CDTYUPD) Register MASK  (Use PWM_CDTYUPD_Msk instead)  */
#define PWM_CDTYUPD_Msk                     (0xFFFFFFU)                                    /**< (PWM_CDTYUPD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CPRD : (PWM Offset: 0x0c) (R/W 32) PWM Channel Period Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CPRD:24;                   /**< bit:  0..23  Channel Period                           */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CPRD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CPRD_OFFSET                     (0x0C)                                        /**<  (PWM_CPRD) PWM Channel Period Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CPRD_CPRD_Pos                   0                                              /**< (PWM_CPRD) Channel Period Position */
#define PWM_CPRD_CPRD_Msk                   (0xFFFFFFU << PWM_CPRD_CPRD_Pos)               /**< (PWM_CPRD) Channel Period Mask */
#define PWM_CPRD_CPRD(value)                (PWM_CPRD_CPRD_Msk & ((value) << PWM_CPRD_CPRD_Pos))
#define PWM_CPRD_MASK                       (0xFFFFFFU)                                    /**< \deprecated (PWM_CPRD) Register MASK  (Use PWM_CPRD_Msk instead)  */
#define PWM_CPRD_Msk                        (0xFFFFFFU)                                    /**< (PWM_CPRD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CPRDUPD : (PWM Offset: 0x10) (/W 32) PWM Channel Period Update Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CPRDUPD:24;                /**< bit:  0..23  Channel Period Update                    */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CPRDUPD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CPRDUPD_OFFSET                  (0x10)                                        /**<  (PWM_CPRDUPD) PWM Channel Period Update Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CPRDUPD_CPRDUPD_Pos             0                                              /**< (PWM_CPRDUPD) Channel Period Update Position */
#define PWM_CPRDUPD_CPRDUPD_Msk             (0xFFFFFFU << PWM_CPRDUPD_CPRDUPD_Pos)         /**< (PWM_CPRDUPD) Channel Period Update Mask */
#define PWM_CPRDUPD_CPRDUPD(value)          (PWM_CPRDUPD_CPRDUPD_Msk & ((value) << PWM_CPRDUPD_CPRDUPD_Pos))
#define PWM_CPRDUPD_MASK                    (0xFFFFFFU)                                    /**< \deprecated (PWM_CPRDUPD) Register MASK  (Use PWM_CPRDUPD_Msk instead)  */
#define PWM_CPRDUPD_Msk                     (0xFFFFFFU)                                    /**< (PWM_CPRDUPD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CCNT : (PWM Offset: 0x14) (R/ 32) PWM Channel Counter Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CNT:24;                    /**< bit:  0..23  Channel Counter Register                 */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CCNT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CCNT_OFFSET                     (0x14)                                        /**<  (PWM_CCNT) PWM Channel Counter Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CCNT_CNT_Pos                    0                                              /**< (PWM_CCNT) Channel Counter Register Position */
#define PWM_CCNT_CNT_Msk                    (0xFFFFFFU << PWM_CCNT_CNT_Pos)                /**< (PWM_CCNT) Channel Counter Register Mask */
#define PWM_CCNT_CNT(value)                 (PWM_CCNT_CNT_Msk & ((value) << PWM_CCNT_CNT_Pos))
#define PWM_CCNT_MASK                       (0xFFFFFFU)                                    /**< \deprecated (PWM_CCNT) Register MASK  (Use PWM_CCNT_Msk instead)  */
#define PWM_CCNT_Msk                        (0xFFFFFFU)                                    /**< (PWM_CCNT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_DT : (PWM Offset: 0x18) (R/W 32) PWM Channel Dead Time Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DTH:16;                    /**< bit:  0..15  Dead-Time Value for PWMHx Output         */
    uint32_t DTL:16;                    /**< bit: 16..31  Dead-Time Value for PWMLx Output         */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_DT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_DT_OFFSET                       (0x18)                                        /**<  (PWM_DT) PWM Channel Dead Time Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_DT_DTH_Pos                      0                                              /**< (PWM_DT) Dead-Time Value for PWMHx Output Position */
#define PWM_DT_DTH_Msk                      (0xFFFFU << PWM_DT_DTH_Pos)                    /**< (PWM_DT) Dead-Time Value for PWMHx Output Mask */
#define PWM_DT_DTH(value)                   (PWM_DT_DTH_Msk & ((value) << PWM_DT_DTH_Pos))
#define PWM_DT_DTL_Pos                      16                                             /**< (PWM_DT) Dead-Time Value for PWMLx Output Position */
#define PWM_DT_DTL_Msk                      (0xFFFFU << PWM_DT_DTL_Pos)                    /**< (PWM_DT) Dead-Time Value for PWMLx Output Mask */
#define PWM_DT_DTL(value)                   (PWM_DT_DTL_Msk & ((value) << PWM_DT_DTL_Pos))
#define PWM_DT_MASK                         (0xFFFFFFFFU)                                  /**< \deprecated (PWM_DT) Register MASK  (Use PWM_DT_Msk instead)  */
#define PWM_DT_Msk                          (0xFFFFFFFFU)                                  /**< (PWM_DT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_DTUPD : (PWM Offset: 0x1c) (/W 32) PWM Channel Dead Time Update Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DTHUPD:16;                 /**< bit:  0..15  Dead-Time Value Update for PWMHx Output  */
    uint32_t DTLUPD:16;                 /**< bit: 16..31  Dead-Time Value Update for PWMLx Output  */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_DTUPD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_DTUPD_OFFSET                    (0x1C)                                        /**<  (PWM_DTUPD) PWM Channel Dead Time Update Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_DTUPD_DTHUPD_Pos                0                                              /**< (PWM_DTUPD) Dead-Time Value Update for PWMHx Output Position */
#define PWM_DTUPD_DTHUPD_Msk                (0xFFFFU << PWM_DTUPD_DTHUPD_Pos)              /**< (PWM_DTUPD) Dead-Time Value Update for PWMHx Output Mask */
#define PWM_DTUPD_DTHUPD(value)             (PWM_DTUPD_DTHUPD_Msk & ((value) << PWM_DTUPD_DTHUPD_Pos))
#define PWM_DTUPD_DTLUPD_Pos                16                                             /**< (PWM_DTUPD) Dead-Time Value Update for PWMLx Output Position */
#define PWM_DTUPD_DTLUPD_Msk                (0xFFFFU << PWM_DTUPD_DTLUPD_Pos)              /**< (PWM_DTUPD) Dead-Time Value Update for PWMLx Output Mask */
#define PWM_DTUPD_DTLUPD(value)             (PWM_DTUPD_DTLUPD_Msk & ((value) << PWM_DTUPD_DTLUPD_Pos))
#define PWM_DTUPD_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (PWM_DTUPD) Register MASK  (Use PWM_DTUPD_Msk instead)  */
#define PWM_DTUPD_Msk                       (0xFFFFFFFFU)                                  /**< (PWM_DTUPD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CMPV : (PWM Offset: 0x00) (R/W 32) PWM Comparison 0 Value Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CV:24;                     /**< bit:  0..23  Comparison x Value                       */
    uint32_t CVM:1;                     /**< bit:     24  Comparison x Value Mode                  */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMPV_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMPV_OFFSET                     (0x00)                                        /**<  (PWM_CMPV) PWM Comparison 0 Value Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMPV_CV_Pos                     0                                              /**< (PWM_CMPV) Comparison x Value Position */
#define PWM_CMPV_CV_Msk                     (0xFFFFFFU << PWM_CMPV_CV_Pos)                 /**< (PWM_CMPV) Comparison x Value Mask */
#define PWM_CMPV_CV(value)                  (PWM_CMPV_CV_Msk & ((value) << PWM_CMPV_CV_Pos))
#define PWM_CMPV_CVM_Pos                    24                                             /**< (PWM_CMPV) Comparison x Value Mode Position */
#define PWM_CMPV_CVM_Msk                    (0x1U << PWM_CMPV_CVM_Pos)                     /**< (PWM_CMPV) Comparison x Value Mode Mask */
#define PWM_CMPV_CVM                        PWM_CMPV_CVM_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMPV_CVM_Msk instead */
#define PWM_CMPV_MASK                       (0x1FFFFFFU)                                   /**< \deprecated (PWM_CMPV) Register MASK  (Use PWM_CMPV_Msk instead)  */
#define PWM_CMPV_Msk                        (0x1FFFFFFU)                                   /**< (PWM_CMPV) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CMPVUPD : (PWM Offset: 0x04) (/W 32) PWM Comparison 0 Value Update Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CVUPD:24;                  /**< bit:  0..23  Comparison x Value Update                */
    uint32_t CVMUPD:1;                  /**< bit:     24  Comparison x Value Mode Update           */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMPVUPD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMPVUPD_OFFSET                  (0x04)                                        /**<  (PWM_CMPVUPD) PWM Comparison 0 Value Update Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMPVUPD_CVUPD_Pos               0                                              /**< (PWM_CMPVUPD) Comparison x Value Update Position */
#define PWM_CMPVUPD_CVUPD_Msk               (0xFFFFFFU << PWM_CMPVUPD_CVUPD_Pos)           /**< (PWM_CMPVUPD) Comparison x Value Update Mask */
#define PWM_CMPVUPD_CVUPD(value)            (PWM_CMPVUPD_CVUPD_Msk & ((value) << PWM_CMPVUPD_CVUPD_Pos))
#define PWM_CMPVUPD_CVMUPD_Pos              24                                             /**< (PWM_CMPVUPD) Comparison x Value Mode Update Position */
#define PWM_CMPVUPD_CVMUPD_Msk              (0x1U << PWM_CMPVUPD_CVMUPD_Pos)               /**< (PWM_CMPVUPD) Comparison x Value Mode Update Mask */
#define PWM_CMPVUPD_CVMUPD                  PWM_CMPVUPD_CVMUPD_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMPVUPD_CVMUPD_Msk instead */
#define PWM_CMPVUPD_MASK                    (0x1FFFFFFU)                                   /**< \deprecated (PWM_CMPVUPD) Register MASK  (Use PWM_CMPVUPD_Msk instead)  */
#define PWM_CMPVUPD_Msk                     (0x1FFFFFFU)                                   /**< (PWM_CMPVUPD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CMPM : (PWM Offset: 0x08) (R/W 32) PWM Comparison 0 Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CEN:1;                     /**< bit:      0  Comparison x Enable                      */
    uint32_t :3;                        /**< bit:   1..3  Reserved */
    uint32_t CTR:4;                     /**< bit:   4..7  Comparison x Trigger                     */
    uint32_t CPR:4;                     /**< bit:  8..11  Comparison x Period                      */
    uint32_t CPRCNT:4;                  /**< bit: 12..15  Comparison x Period Counter              */
    uint32_t CUPR:4;                    /**< bit: 16..19  Comparison x Update Period               */
    uint32_t CUPRCNT:4;                 /**< bit: 20..23  Comparison x Update Period Counter       */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMPM_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMPM_OFFSET                     (0x08)                                        /**<  (PWM_CMPM) PWM Comparison 0 Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMPM_CEN_Pos                    0                                              /**< (PWM_CMPM) Comparison x Enable Position */
#define PWM_CMPM_CEN_Msk                    (0x1U << PWM_CMPM_CEN_Pos)                     /**< (PWM_CMPM) Comparison x Enable Mask */
#define PWM_CMPM_CEN                        PWM_CMPM_CEN_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMPM_CEN_Msk instead */
#define PWM_CMPM_CTR_Pos                    4                                              /**< (PWM_CMPM) Comparison x Trigger Position */
#define PWM_CMPM_CTR_Msk                    (0xFU << PWM_CMPM_CTR_Pos)                     /**< (PWM_CMPM) Comparison x Trigger Mask */
#define PWM_CMPM_CTR(value)                 (PWM_CMPM_CTR_Msk & ((value) << PWM_CMPM_CTR_Pos))
#define PWM_CMPM_CPR_Pos                    8                                              /**< (PWM_CMPM) Comparison x Period Position */
#define PWM_CMPM_CPR_Msk                    (0xFU << PWM_CMPM_CPR_Pos)                     /**< (PWM_CMPM) Comparison x Period Mask */
#define PWM_CMPM_CPR(value)                 (PWM_CMPM_CPR_Msk & ((value) << PWM_CMPM_CPR_Pos))
#define PWM_CMPM_CPRCNT_Pos                 12                                             /**< (PWM_CMPM) Comparison x Period Counter Position */
#define PWM_CMPM_CPRCNT_Msk                 (0xFU << PWM_CMPM_CPRCNT_Pos)                  /**< (PWM_CMPM) Comparison x Period Counter Mask */
#define PWM_CMPM_CPRCNT(value)              (PWM_CMPM_CPRCNT_Msk & ((value) << PWM_CMPM_CPRCNT_Pos))
#define PWM_CMPM_CUPR_Pos                   16                                             /**< (PWM_CMPM) Comparison x Update Period Position */
#define PWM_CMPM_CUPR_Msk                   (0xFU << PWM_CMPM_CUPR_Pos)                    /**< (PWM_CMPM) Comparison x Update Period Mask */
#define PWM_CMPM_CUPR(value)                (PWM_CMPM_CUPR_Msk & ((value) << PWM_CMPM_CUPR_Pos))
#define PWM_CMPM_CUPRCNT_Pos                20                                             /**< (PWM_CMPM) Comparison x Update Period Counter Position */
#define PWM_CMPM_CUPRCNT_Msk                (0xFU << PWM_CMPM_CUPRCNT_Pos)                 /**< (PWM_CMPM) Comparison x Update Period Counter Mask */
#define PWM_CMPM_CUPRCNT(value)             (PWM_CMPM_CUPRCNT_Msk & ((value) << PWM_CMPM_CUPRCNT_Pos))
#define PWM_CMPM_MASK                       (0xFFFFF1U)                                    /**< \deprecated (PWM_CMPM) Register MASK  (Use PWM_CMPM_Msk instead)  */
#define PWM_CMPM_Msk                        (0xFFFFF1U)                                    /**< (PWM_CMPM) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CMPMUPD : (PWM Offset: 0x0c) (/W 32) PWM Comparison 0 Mode Update Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CENUPD:1;                  /**< bit:      0  Comparison x Enable Update               */
    uint32_t :3;                        /**< bit:   1..3  Reserved */
    uint32_t CTRUPD:4;                  /**< bit:   4..7  Comparison x Trigger Update              */
    uint32_t CPRUPD:4;                  /**< bit:  8..11  Comparison x Period Update               */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t CUPRUPD:4;                 /**< bit: 16..19  Comparison x Update Period Update        */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMPMUPD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMPMUPD_OFFSET                  (0x0C)                                        /**<  (PWM_CMPMUPD) PWM Comparison 0 Mode Update Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMPMUPD_CENUPD_Pos              0                                              /**< (PWM_CMPMUPD) Comparison x Enable Update Position */
#define PWM_CMPMUPD_CENUPD_Msk              (0x1U << PWM_CMPMUPD_CENUPD_Pos)               /**< (PWM_CMPMUPD) Comparison x Enable Update Mask */
#define PWM_CMPMUPD_CENUPD                  PWM_CMPMUPD_CENUPD_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMPMUPD_CENUPD_Msk instead */
#define PWM_CMPMUPD_CTRUPD_Pos              4                                              /**< (PWM_CMPMUPD) Comparison x Trigger Update Position */
#define PWM_CMPMUPD_CTRUPD_Msk              (0xFU << PWM_CMPMUPD_CTRUPD_Pos)               /**< (PWM_CMPMUPD) Comparison x Trigger Update Mask */
#define PWM_CMPMUPD_CTRUPD(value)           (PWM_CMPMUPD_CTRUPD_Msk & ((value) << PWM_CMPMUPD_CTRUPD_Pos))
#define PWM_CMPMUPD_CPRUPD_Pos              8                                              /**< (PWM_CMPMUPD) Comparison x Period Update Position */
#define PWM_CMPMUPD_CPRUPD_Msk              (0xFU << PWM_CMPMUPD_CPRUPD_Pos)               /**< (PWM_CMPMUPD) Comparison x Period Update Mask */
#define PWM_CMPMUPD_CPRUPD(value)           (PWM_CMPMUPD_CPRUPD_Msk & ((value) << PWM_CMPMUPD_CPRUPD_Pos))
#define PWM_CMPMUPD_CUPRUPD_Pos             16                                             /**< (PWM_CMPMUPD) Comparison x Update Period Update Position */
#define PWM_CMPMUPD_CUPRUPD_Msk             (0xFU << PWM_CMPMUPD_CUPRUPD_Pos)              /**< (PWM_CMPMUPD) Comparison x Update Period Update Mask */
#define PWM_CMPMUPD_CUPRUPD(value)          (PWM_CMPMUPD_CUPRUPD_Msk & ((value) << PWM_CMPMUPD_CUPRUPD_Pos))
#define PWM_CMPMUPD_MASK                    (0xF0FF1U)                                     /**< \deprecated (PWM_CMPMUPD) Register MASK  (Use PWM_CMPMUPD_Msk instead)  */
#define PWM_CMPMUPD_Msk                     (0xF0FF1U)                                     /**< (PWM_CMPMUPD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CLK : (PWM Offset: 0x00) (R/W 32) PWM Clock Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DIVA:8;                    /**< bit:   0..7  CLKA Divide Factor                       */
    uint32_t PREA:4;                    /**< bit:  8..11  CLKA Source Clock Selection              */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t DIVB:8;                    /**< bit: 16..23  CLKB Divide Factor                       */
    uint32_t PREB:4;                    /**< bit: 24..27  CLKB Source Clock Selection              */
    uint32_t :4;                        /**< bit: 28..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CLK_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CLK_OFFSET                      (0x00)                                        /**<  (PWM_CLK) PWM Clock Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CLK_DIVA_Pos                    0                                              /**< (PWM_CLK) CLKA Divide Factor Position */
#define PWM_CLK_DIVA_Msk                    (0xFFU << PWM_CLK_DIVA_Pos)                    /**< (PWM_CLK) CLKA Divide Factor Mask */
#define PWM_CLK_DIVA(value)                 (PWM_CLK_DIVA_Msk & ((value) << PWM_CLK_DIVA_Pos))
#define   PWM_CLK_DIVA_CLKA_POFF_Val        (0x0U)                                         /**< (PWM_CLK) CLKA clock is turned off  */
#define   PWM_CLK_DIVA_PREA_Val             (0x1U)                                         /**< (PWM_CLK) CLKA clock is clock selected by PREA  */
#define PWM_CLK_DIVA_CLKA_POFF              (PWM_CLK_DIVA_CLKA_POFF_Val << PWM_CLK_DIVA_Pos)  /**< (PWM_CLK) CLKA clock is turned off Position  */
#define PWM_CLK_DIVA_PREA                   (PWM_CLK_DIVA_PREA_Val << PWM_CLK_DIVA_Pos)    /**< (PWM_CLK) CLKA clock is clock selected by PREA Position  */
#define PWM_CLK_PREA_Pos                    8                                              /**< (PWM_CLK) CLKA Source Clock Selection Position */
#define PWM_CLK_PREA_Msk                    (0xFU << PWM_CLK_PREA_Pos)                     /**< (PWM_CLK) CLKA Source Clock Selection Mask */
#define PWM_CLK_PREA(value)                 (PWM_CLK_PREA_Msk & ((value) << PWM_CLK_PREA_Pos))
#define   PWM_CLK_PREA_CLK_Val              (0x0U)                                         /**< (PWM_CLK) Peripheral clock  */
#define   PWM_CLK_PREA_CLK_DIV2_Val         (0x1U)                                         /**< (PWM_CLK) Peripheral clock/2  */
#define   PWM_CLK_PREA_CLK_DIV4_Val         (0x2U)                                         /**< (PWM_CLK) Peripheral clock/4  */
#define   PWM_CLK_PREA_CLK_DIV8_Val         (0x3U)                                         /**< (PWM_CLK) Peripheral clock/8  */
#define   PWM_CLK_PREA_CLK_DIV16_Val        (0x4U)                                         /**< (PWM_CLK) Peripheral clock/16  */
#define   PWM_CLK_PREA_CLK_DIV32_Val        (0x5U)                                         /**< (PWM_CLK) Peripheral clock/32  */
#define   PWM_CLK_PREA_CLK_DIV64_Val        (0x6U)                                         /**< (PWM_CLK) Peripheral clock/64  */
#define   PWM_CLK_PREA_CLK_DIV128_Val       (0x7U)                                         /**< (PWM_CLK) Peripheral clock/128  */
#define   PWM_CLK_PREA_CLK_DIV256_Val       (0x8U)                                         /**< (PWM_CLK) Peripheral clock/256  */
#define   PWM_CLK_PREA_CLK_DIV512_Val       (0x9U)                                         /**< (PWM_CLK) Peripheral clock/512  */
#define   PWM_CLK_PREA_CLK_DIV1024_Val      (0xAU)                                         /**< (PWM_CLK) Peripheral clock/1024  */
#define PWM_CLK_PREA_CLK                    (PWM_CLK_PREA_CLK_Val << PWM_CLK_PREA_Pos)     /**< (PWM_CLK) Peripheral clock Position  */
#define PWM_CLK_PREA_CLK_DIV2               (PWM_CLK_PREA_CLK_DIV2_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/2 Position  */
#define PWM_CLK_PREA_CLK_DIV4               (PWM_CLK_PREA_CLK_DIV4_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/4 Position  */
#define PWM_CLK_PREA_CLK_DIV8               (PWM_CLK_PREA_CLK_DIV8_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/8 Position  */
#define PWM_CLK_PREA_CLK_DIV16              (PWM_CLK_PREA_CLK_DIV16_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/16 Position  */
#define PWM_CLK_PREA_CLK_DIV32              (PWM_CLK_PREA_CLK_DIV32_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/32 Position  */
#define PWM_CLK_PREA_CLK_DIV64              (PWM_CLK_PREA_CLK_DIV64_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/64 Position  */
#define PWM_CLK_PREA_CLK_DIV128             (PWM_CLK_PREA_CLK_DIV128_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/128 Position  */
#define PWM_CLK_PREA_CLK_DIV256             (PWM_CLK_PREA_CLK_DIV256_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/256 Position  */
#define PWM_CLK_PREA_CLK_DIV512             (PWM_CLK_PREA_CLK_DIV512_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/512 Position  */
#define PWM_CLK_PREA_CLK_DIV1024            (PWM_CLK_PREA_CLK_DIV1024_Val << PWM_CLK_PREA_Pos)  /**< (PWM_CLK) Peripheral clock/1024 Position  */
#define PWM_CLK_DIVB_Pos                    16                                             /**< (PWM_CLK) CLKB Divide Factor Position */
#define PWM_CLK_DIVB_Msk                    (0xFFU << PWM_CLK_DIVB_Pos)                    /**< (PWM_CLK) CLKB Divide Factor Mask */
#define PWM_CLK_DIVB(value)                 (PWM_CLK_DIVB_Msk & ((value) << PWM_CLK_DIVB_Pos))
#define   PWM_CLK_DIVB_CLKB_POFF_Val        (0x0U)                                         /**< (PWM_CLK) CLKB clock is turned off  */
#define   PWM_CLK_DIVB_PREB_Val             (0x1U)                                         /**< (PWM_CLK) CLKB clock is clock selected by PREB  */
#define PWM_CLK_DIVB_CLKB_POFF              (PWM_CLK_DIVB_CLKB_POFF_Val << PWM_CLK_DIVB_Pos)  /**< (PWM_CLK) CLKB clock is turned off Position  */
#define PWM_CLK_DIVB_PREB                   (PWM_CLK_DIVB_PREB_Val << PWM_CLK_DIVB_Pos)    /**< (PWM_CLK) CLKB clock is clock selected by PREB Position  */
#define PWM_CLK_PREB_Pos                    24                                             /**< (PWM_CLK) CLKB Source Clock Selection Position */
#define PWM_CLK_PREB_Msk                    (0xFU << PWM_CLK_PREB_Pos)                     /**< (PWM_CLK) CLKB Source Clock Selection Mask */
#define PWM_CLK_PREB(value)                 (PWM_CLK_PREB_Msk & ((value) << PWM_CLK_PREB_Pos))
#define   PWM_CLK_PREB_CLK_Val              (0x0U)                                         /**< (PWM_CLK) Peripheral clock  */
#define   PWM_CLK_PREB_CLK_DIV2_Val         (0x1U)                                         /**< (PWM_CLK) Peripheral clock/2  */
#define   PWM_CLK_PREB_CLK_DIV4_Val         (0x2U)                                         /**< (PWM_CLK) Peripheral clock/4  */
#define   PWM_CLK_PREB_CLK_DIV8_Val         (0x3U)                                         /**< (PWM_CLK) Peripheral clock/8  */
#define   PWM_CLK_PREB_CLK_DIV16_Val        (0x4U)                                         /**< (PWM_CLK) Peripheral clock/16  */
#define   PWM_CLK_PREB_CLK_DIV32_Val        (0x5U)                                         /**< (PWM_CLK) Peripheral clock/32  */
#define   PWM_CLK_PREB_CLK_DIV64_Val        (0x6U)                                         /**< (PWM_CLK) Peripheral clock/64  */
#define   PWM_CLK_PREB_CLK_DIV128_Val       (0x7U)                                         /**< (PWM_CLK) Peripheral clock/128  */
#define   PWM_CLK_PREB_CLK_DIV256_Val       (0x8U)                                         /**< (PWM_CLK) Peripheral clock/256  */
#define   PWM_CLK_PREB_CLK_DIV512_Val       (0x9U)                                         /**< (PWM_CLK) Peripheral clock/512  */
#define   PWM_CLK_PREB_CLK_DIV1024_Val      (0xAU)                                         /**< (PWM_CLK) Peripheral clock/1024  */
#define PWM_CLK_PREB_CLK                    (PWM_CLK_PREB_CLK_Val << PWM_CLK_PREB_Pos)     /**< (PWM_CLK) Peripheral clock Position  */
#define PWM_CLK_PREB_CLK_DIV2               (PWM_CLK_PREB_CLK_DIV2_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/2 Position  */
#define PWM_CLK_PREB_CLK_DIV4               (PWM_CLK_PREB_CLK_DIV4_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/4 Position  */
#define PWM_CLK_PREB_CLK_DIV8               (PWM_CLK_PREB_CLK_DIV8_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/8 Position  */
#define PWM_CLK_PREB_CLK_DIV16              (PWM_CLK_PREB_CLK_DIV16_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/16 Position  */
#define PWM_CLK_PREB_CLK_DIV32              (PWM_CLK_PREB_CLK_DIV32_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/32 Position  */
#define PWM_CLK_PREB_CLK_DIV64              (PWM_CLK_PREB_CLK_DIV64_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/64 Position  */
#define PWM_CLK_PREB_CLK_DIV128             (PWM_CLK_PREB_CLK_DIV128_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/128 Position  */
#define PWM_CLK_PREB_CLK_DIV256             (PWM_CLK_PREB_CLK_DIV256_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/256 Position  */
#define PWM_CLK_PREB_CLK_DIV512             (PWM_CLK_PREB_CLK_DIV512_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/512 Position  */
#define PWM_CLK_PREB_CLK_DIV1024            (PWM_CLK_PREB_CLK_DIV1024_Val << PWM_CLK_PREB_Pos)  /**< (PWM_CLK) Peripheral clock/1024 Position  */
#define PWM_CLK_MASK                        (0xFFF0FFFU)                                   /**< \deprecated (PWM_CLK) Register MASK  (Use PWM_CLK_Msk instead)  */
#define PWM_CLK_Msk                         (0xFFF0FFFU)                                   /**< (PWM_CLK) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_ENA : (PWM Offset: 0x04) (/W 32) PWM Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CHID0:1;                   /**< bit:      0  Channel ID                               */
    uint32_t CHID1:1;                   /**< bit:      1  Channel ID                               */
    uint32_t CHID2:1;                   /**< bit:      2  Channel ID                               */
    uint32_t CHID3:1;                   /**< bit:      3  Channel ID                               */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CHID:4;                    /**< bit:   0..3  Channel ID                               */
    uint32_t :28;                       /**< bit:  4..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_ENA_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_ENA_OFFSET                      (0x04)                                        /**<  (PWM_ENA) PWM Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_ENA_CHID0_Pos                   0                                              /**< (PWM_ENA) Channel ID Position */
#define PWM_ENA_CHID0_Msk                   (0x1U << PWM_ENA_CHID0_Pos)                    /**< (PWM_ENA) Channel ID Mask */
#define PWM_ENA_CHID0                       PWM_ENA_CHID0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ENA_CHID0_Msk instead */
#define PWM_ENA_CHID1_Pos                   1                                              /**< (PWM_ENA) Channel ID Position */
#define PWM_ENA_CHID1_Msk                   (0x1U << PWM_ENA_CHID1_Pos)                    /**< (PWM_ENA) Channel ID Mask */
#define PWM_ENA_CHID1                       PWM_ENA_CHID1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ENA_CHID1_Msk instead */
#define PWM_ENA_CHID2_Pos                   2                                              /**< (PWM_ENA) Channel ID Position */
#define PWM_ENA_CHID2_Msk                   (0x1U << PWM_ENA_CHID2_Pos)                    /**< (PWM_ENA) Channel ID Mask */
#define PWM_ENA_CHID2                       PWM_ENA_CHID2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ENA_CHID2_Msk instead */
#define PWM_ENA_CHID3_Pos                   3                                              /**< (PWM_ENA) Channel ID Position */
#define PWM_ENA_CHID3_Msk                   (0x1U << PWM_ENA_CHID3_Pos)                    /**< (PWM_ENA) Channel ID Mask */
#define PWM_ENA_CHID3                       PWM_ENA_CHID3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ENA_CHID3_Msk instead */
#define PWM_ENA_CHID_Pos                    0                                              /**< (PWM_ENA Position) Channel ID */
#define PWM_ENA_CHID_Msk                    (0xFU << PWM_ENA_CHID_Pos)                     /**< (PWM_ENA Mask) CHID */
#define PWM_ENA_CHID(value)                 (PWM_ENA_CHID_Msk & ((value) << PWM_ENA_CHID_Pos))  
#define PWM_ENA_MASK                        (0x0FU)                                        /**< \deprecated (PWM_ENA) Register MASK  (Use PWM_ENA_Msk instead)  */
#define PWM_ENA_Msk                         (0x0FU)                                        /**< (PWM_ENA) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_DIS : (PWM Offset: 0x08) (/W 32) PWM Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CHID0:1;                   /**< bit:      0  Channel ID                               */
    uint32_t CHID1:1;                   /**< bit:      1  Channel ID                               */
    uint32_t CHID2:1;                   /**< bit:      2  Channel ID                               */
    uint32_t CHID3:1;                   /**< bit:      3  Channel ID                               */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CHID:4;                    /**< bit:   0..3  Channel ID                               */
    uint32_t :28;                       /**< bit:  4..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_DIS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_DIS_OFFSET                      (0x08)                                        /**<  (PWM_DIS) PWM Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_DIS_CHID0_Pos                   0                                              /**< (PWM_DIS) Channel ID Position */
#define PWM_DIS_CHID0_Msk                   (0x1U << PWM_DIS_CHID0_Pos)                    /**< (PWM_DIS) Channel ID Mask */
#define PWM_DIS_CHID0                       PWM_DIS_CHID0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_DIS_CHID0_Msk instead */
#define PWM_DIS_CHID1_Pos                   1                                              /**< (PWM_DIS) Channel ID Position */
#define PWM_DIS_CHID1_Msk                   (0x1U << PWM_DIS_CHID1_Pos)                    /**< (PWM_DIS) Channel ID Mask */
#define PWM_DIS_CHID1                       PWM_DIS_CHID1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_DIS_CHID1_Msk instead */
#define PWM_DIS_CHID2_Pos                   2                                              /**< (PWM_DIS) Channel ID Position */
#define PWM_DIS_CHID2_Msk                   (0x1U << PWM_DIS_CHID2_Pos)                    /**< (PWM_DIS) Channel ID Mask */
#define PWM_DIS_CHID2                       PWM_DIS_CHID2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_DIS_CHID2_Msk instead */
#define PWM_DIS_CHID3_Pos                   3                                              /**< (PWM_DIS) Channel ID Position */
#define PWM_DIS_CHID3_Msk                   (0x1U << PWM_DIS_CHID3_Pos)                    /**< (PWM_DIS) Channel ID Mask */
#define PWM_DIS_CHID3                       PWM_DIS_CHID3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_DIS_CHID3_Msk instead */
#define PWM_DIS_CHID_Pos                    0                                              /**< (PWM_DIS Position) Channel ID */
#define PWM_DIS_CHID_Msk                    (0xFU << PWM_DIS_CHID_Pos)                     /**< (PWM_DIS Mask) CHID */
#define PWM_DIS_CHID(value)                 (PWM_DIS_CHID_Msk & ((value) << PWM_DIS_CHID_Pos))  
#define PWM_DIS_MASK                        (0x0FU)                                        /**< \deprecated (PWM_DIS) Register MASK  (Use PWM_DIS_Msk instead)  */
#define PWM_DIS_Msk                         (0x0FU)                                        /**< (PWM_DIS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_SR : (PWM Offset: 0x0c) (R/ 32) PWM Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CHID0:1;                   /**< bit:      0  Channel ID                               */
    uint32_t CHID1:1;                   /**< bit:      1  Channel ID                               */
    uint32_t CHID2:1;                   /**< bit:      2  Channel ID                               */
    uint32_t CHID3:1;                   /**< bit:      3  Channel ID                               */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CHID:4;                    /**< bit:   0..3  Channel ID                               */
    uint32_t :28;                       /**< bit:  4..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_SR_OFFSET                       (0x0C)                                        /**<  (PWM_SR) PWM Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_SR_CHID0_Pos                    0                                              /**< (PWM_SR) Channel ID Position */
#define PWM_SR_CHID0_Msk                    (0x1U << PWM_SR_CHID0_Pos)                     /**< (PWM_SR) Channel ID Mask */
#define PWM_SR_CHID0                        PWM_SR_CHID0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SR_CHID0_Msk instead */
#define PWM_SR_CHID1_Pos                    1                                              /**< (PWM_SR) Channel ID Position */
#define PWM_SR_CHID1_Msk                    (0x1U << PWM_SR_CHID1_Pos)                     /**< (PWM_SR) Channel ID Mask */
#define PWM_SR_CHID1                        PWM_SR_CHID1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SR_CHID1_Msk instead */
#define PWM_SR_CHID2_Pos                    2                                              /**< (PWM_SR) Channel ID Position */
#define PWM_SR_CHID2_Msk                    (0x1U << PWM_SR_CHID2_Pos)                     /**< (PWM_SR) Channel ID Mask */
#define PWM_SR_CHID2                        PWM_SR_CHID2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SR_CHID2_Msk instead */
#define PWM_SR_CHID3_Pos                    3                                              /**< (PWM_SR) Channel ID Position */
#define PWM_SR_CHID3_Msk                    (0x1U << PWM_SR_CHID3_Pos)                     /**< (PWM_SR) Channel ID Mask */
#define PWM_SR_CHID3                        PWM_SR_CHID3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SR_CHID3_Msk instead */
#define PWM_SR_CHID_Pos                     0                                              /**< (PWM_SR Position) Channel ID */
#define PWM_SR_CHID_Msk                     (0xFU << PWM_SR_CHID_Pos)                      /**< (PWM_SR Mask) CHID */
#define PWM_SR_CHID(value)                  (PWM_SR_CHID_Msk & ((value) << PWM_SR_CHID_Pos))  
#define PWM_SR_MASK                         (0x0FU)                                        /**< \deprecated (PWM_SR) Register MASK  (Use PWM_SR_Msk instead)  */
#define PWM_SR_Msk                          (0x0FU)                                        /**< (PWM_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_IER1 : (PWM Offset: 0x10) (/W 32) PWM Interrupt Enable Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CHID0:1;                   /**< bit:      0  Counter Event on Channel 0 Interrupt Enable */
    uint32_t CHID1:1;                   /**< bit:      1  Counter Event on Channel 1 Interrupt Enable */
    uint32_t CHID2:1;                   /**< bit:      2  Counter Event on Channel 2 Interrupt Enable */
    uint32_t CHID3:1;                   /**< bit:      3  Counter Event on Channel 3 Interrupt Enable */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FCHID0:1;                  /**< bit:     16  Fault Protection Trigger on Channel 0 Interrupt Enable */
    uint32_t FCHID1:1;                  /**< bit:     17  Fault Protection Trigger on Channel 1 Interrupt Enable */
    uint32_t FCHID2:1;                  /**< bit:     18  Fault Protection Trigger on Channel 2 Interrupt Enable */
    uint32_t FCHID3:1;                  /**< bit:     19  Fault Protection Trigger on Channel 3 Interrupt Enable */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CHID:4;                    /**< bit:   0..3  Counter Event on Channel x Interrupt Enable */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FCHID:4;                   /**< bit: 16..19  Fault Protection Trigger on Channel 3 Interrupt Enable */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_IER1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_IER1_OFFSET                     (0x10)                                        /**<  (PWM_IER1) PWM Interrupt Enable Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_IER1_CHID0_Pos                  0                                              /**< (PWM_IER1) Counter Event on Channel 0 Interrupt Enable Position */
#define PWM_IER1_CHID0_Msk                  (0x1U << PWM_IER1_CHID0_Pos)                   /**< (PWM_IER1) Counter Event on Channel 0 Interrupt Enable Mask */
#define PWM_IER1_CHID0                      PWM_IER1_CHID0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER1_CHID0_Msk instead */
#define PWM_IER1_CHID1_Pos                  1                                              /**< (PWM_IER1) Counter Event on Channel 1 Interrupt Enable Position */
#define PWM_IER1_CHID1_Msk                  (0x1U << PWM_IER1_CHID1_Pos)                   /**< (PWM_IER1) Counter Event on Channel 1 Interrupt Enable Mask */
#define PWM_IER1_CHID1                      PWM_IER1_CHID1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER1_CHID1_Msk instead */
#define PWM_IER1_CHID2_Pos                  2                                              /**< (PWM_IER1) Counter Event on Channel 2 Interrupt Enable Position */
#define PWM_IER1_CHID2_Msk                  (0x1U << PWM_IER1_CHID2_Pos)                   /**< (PWM_IER1) Counter Event on Channel 2 Interrupt Enable Mask */
#define PWM_IER1_CHID2                      PWM_IER1_CHID2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER1_CHID2_Msk instead */
#define PWM_IER1_CHID3_Pos                  3                                              /**< (PWM_IER1) Counter Event on Channel 3 Interrupt Enable Position */
#define PWM_IER1_CHID3_Msk                  (0x1U << PWM_IER1_CHID3_Pos)                   /**< (PWM_IER1) Counter Event on Channel 3 Interrupt Enable Mask */
#define PWM_IER1_CHID3                      PWM_IER1_CHID3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER1_CHID3_Msk instead */
#define PWM_IER1_FCHID0_Pos                 16                                             /**< (PWM_IER1) Fault Protection Trigger on Channel 0 Interrupt Enable Position */
#define PWM_IER1_FCHID0_Msk                 (0x1U << PWM_IER1_FCHID0_Pos)                  /**< (PWM_IER1) Fault Protection Trigger on Channel 0 Interrupt Enable Mask */
#define PWM_IER1_FCHID0                     PWM_IER1_FCHID0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER1_FCHID0_Msk instead */
#define PWM_IER1_FCHID1_Pos                 17                                             /**< (PWM_IER1) Fault Protection Trigger on Channel 1 Interrupt Enable Position */
#define PWM_IER1_FCHID1_Msk                 (0x1U << PWM_IER1_FCHID1_Pos)                  /**< (PWM_IER1) Fault Protection Trigger on Channel 1 Interrupt Enable Mask */
#define PWM_IER1_FCHID1                     PWM_IER1_FCHID1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER1_FCHID1_Msk instead */
#define PWM_IER1_FCHID2_Pos                 18                                             /**< (PWM_IER1) Fault Protection Trigger on Channel 2 Interrupt Enable Position */
#define PWM_IER1_FCHID2_Msk                 (0x1U << PWM_IER1_FCHID2_Pos)                  /**< (PWM_IER1) Fault Protection Trigger on Channel 2 Interrupt Enable Mask */
#define PWM_IER1_FCHID2                     PWM_IER1_FCHID2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER1_FCHID2_Msk instead */
#define PWM_IER1_FCHID3_Pos                 19                                             /**< (PWM_IER1) Fault Protection Trigger on Channel 3 Interrupt Enable Position */
#define PWM_IER1_FCHID3_Msk                 (0x1U << PWM_IER1_FCHID3_Pos)                  /**< (PWM_IER1) Fault Protection Trigger on Channel 3 Interrupt Enable Mask */
#define PWM_IER1_FCHID3                     PWM_IER1_FCHID3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER1_FCHID3_Msk instead */
#define PWM_IER1_CHID_Pos                   0                                              /**< (PWM_IER1 Position) Counter Event on Channel x Interrupt Enable */
#define PWM_IER1_CHID_Msk                   (0xFU << PWM_IER1_CHID_Pos)                    /**< (PWM_IER1 Mask) CHID */
#define PWM_IER1_CHID(value)                (PWM_IER1_CHID_Msk & ((value) << PWM_IER1_CHID_Pos))  
#define PWM_IER1_FCHID_Pos                  16                                             /**< (PWM_IER1 Position) Fault Protection Trigger on Channel 3 Interrupt Enable */
#define PWM_IER1_FCHID_Msk                  (0xFU << PWM_IER1_FCHID_Pos)                   /**< (PWM_IER1 Mask) FCHID */
#define PWM_IER1_FCHID(value)               (PWM_IER1_FCHID_Msk & ((value) << PWM_IER1_FCHID_Pos))  
#define PWM_IER1_MASK                       (0xF000FU)                                     /**< \deprecated (PWM_IER1) Register MASK  (Use PWM_IER1_Msk instead)  */
#define PWM_IER1_Msk                        (0xF000FU)                                     /**< (PWM_IER1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_IDR1 : (PWM Offset: 0x14) (/W 32) PWM Interrupt Disable Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CHID0:1;                   /**< bit:      0  Counter Event on Channel 0 Interrupt Disable */
    uint32_t CHID1:1;                   /**< bit:      1  Counter Event on Channel 1 Interrupt Disable */
    uint32_t CHID2:1;                   /**< bit:      2  Counter Event on Channel 2 Interrupt Disable */
    uint32_t CHID3:1;                   /**< bit:      3  Counter Event on Channel 3 Interrupt Disable */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FCHID0:1;                  /**< bit:     16  Fault Protection Trigger on Channel 0 Interrupt Disable */
    uint32_t FCHID1:1;                  /**< bit:     17  Fault Protection Trigger on Channel 1 Interrupt Disable */
    uint32_t FCHID2:1;                  /**< bit:     18  Fault Protection Trigger on Channel 2 Interrupt Disable */
    uint32_t FCHID3:1;                  /**< bit:     19  Fault Protection Trigger on Channel 3 Interrupt Disable */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CHID:4;                    /**< bit:   0..3  Counter Event on Channel x Interrupt Disable */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FCHID:4;                   /**< bit: 16..19  Fault Protection Trigger on Channel 3 Interrupt Disable */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_IDR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_IDR1_OFFSET                     (0x14)                                        /**<  (PWM_IDR1) PWM Interrupt Disable Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_IDR1_CHID0_Pos                  0                                              /**< (PWM_IDR1) Counter Event on Channel 0 Interrupt Disable Position */
#define PWM_IDR1_CHID0_Msk                  (0x1U << PWM_IDR1_CHID0_Pos)                   /**< (PWM_IDR1) Counter Event on Channel 0 Interrupt Disable Mask */
#define PWM_IDR1_CHID0                      PWM_IDR1_CHID0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR1_CHID0_Msk instead */
#define PWM_IDR1_CHID1_Pos                  1                                              /**< (PWM_IDR1) Counter Event on Channel 1 Interrupt Disable Position */
#define PWM_IDR1_CHID1_Msk                  (0x1U << PWM_IDR1_CHID1_Pos)                   /**< (PWM_IDR1) Counter Event on Channel 1 Interrupt Disable Mask */
#define PWM_IDR1_CHID1                      PWM_IDR1_CHID1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR1_CHID1_Msk instead */
#define PWM_IDR1_CHID2_Pos                  2                                              /**< (PWM_IDR1) Counter Event on Channel 2 Interrupt Disable Position */
#define PWM_IDR1_CHID2_Msk                  (0x1U << PWM_IDR1_CHID2_Pos)                   /**< (PWM_IDR1) Counter Event on Channel 2 Interrupt Disable Mask */
#define PWM_IDR1_CHID2                      PWM_IDR1_CHID2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR1_CHID2_Msk instead */
#define PWM_IDR1_CHID3_Pos                  3                                              /**< (PWM_IDR1) Counter Event on Channel 3 Interrupt Disable Position */
#define PWM_IDR1_CHID3_Msk                  (0x1U << PWM_IDR1_CHID3_Pos)                   /**< (PWM_IDR1) Counter Event on Channel 3 Interrupt Disable Mask */
#define PWM_IDR1_CHID3                      PWM_IDR1_CHID3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR1_CHID3_Msk instead */
#define PWM_IDR1_FCHID0_Pos                 16                                             /**< (PWM_IDR1) Fault Protection Trigger on Channel 0 Interrupt Disable Position */
#define PWM_IDR1_FCHID0_Msk                 (0x1U << PWM_IDR1_FCHID0_Pos)                  /**< (PWM_IDR1) Fault Protection Trigger on Channel 0 Interrupt Disable Mask */
#define PWM_IDR1_FCHID0                     PWM_IDR1_FCHID0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR1_FCHID0_Msk instead */
#define PWM_IDR1_FCHID1_Pos                 17                                             /**< (PWM_IDR1) Fault Protection Trigger on Channel 1 Interrupt Disable Position */
#define PWM_IDR1_FCHID1_Msk                 (0x1U << PWM_IDR1_FCHID1_Pos)                  /**< (PWM_IDR1) Fault Protection Trigger on Channel 1 Interrupt Disable Mask */
#define PWM_IDR1_FCHID1                     PWM_IDR1_FCHID1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR1_FCHID1_Msk instead */
#define PWM_IDR1_FCHID2_Pos                 18                                             /**< (PWM_IDR1) Fault Protection Trigger on Channel 2 Interrupt Disable Position */
#define PWM_IDR1_FCHID2_Msk                 (0x1U << PWM_IDR1_FCHID2_Pos)                  /**< (PWM_IDR1) Fault Protection Trigger on Channel 2 Interrupt Disable Mask */
#define PWM_IDR1_FCHID2                     PWM_IDR1_FCHID2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR1_FCHID2_Msk instead */
#define PWM_IDR1_FCHID3_Pos                 19                                             /**< (PWM_IDR1) Fault Protection Trigger on Channel 3 Interrupt Disable Position */
#define PWM_IDR1_FCHID3_Msk                 (0x1U << PWM_IDR1_FCHID3_Pos)                  /**< (PWM_IDR1) Fault Protection Trigger on Channel 3 Interrupt Disable Mask */
#define PWM_IDR1_FCHID3                     PWM_IDR1_FCHID3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR1_FCHID3_Msk instead */
#define PWM_IDR1_CHID_Pos                   0                                              /**< (PWM_IDR1 Position) Counter Event on Channel x Interrupt Disable */
#define PWM_IDR1_CHID_Msk                   (0xFU << PWM_IDR1_CHID_Pos)                    /**< (PWM_IDR1 Mask) CHID */
#define PWM_IDR1_CHID(value)                (PWM_IDR1_CHID_Msk & ((value) << PWM_IDR1_CHID_Pos))  
#define PWM_IDR1_FCHID_Pos                  16                                             /**< (PWM_IDR1 Position) Fault Protection Trigger on Channel 3 Interrupt Disable */
#define PWM_IDR1_FCHID_Msk                  (0xFU << PWM_IDR1_FCHID_Pos)                   /**< (PWM_IDR1 Mask) FCHID */
#define PWM_IDR1_FCHID(value)               (PWM_IDR1_FCHID_Msk & ((value) << PWM_IDR1_FCHID_Pos))  
#define PWM_IDR1_MASK                       (0xF000FU)                                     /**< \deprecated (PWM_IDR1) Register MASK  (Use PWM_IDR1_Msk instead)  */
#define PWM_IDR1_Msk                        (0xF000FU)                                     /**< (PWM_IDR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_IMR1 : (PWM Offset: 0x18) (R/ 32) PWM Interrupt Mask Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CHID0:1;                   /**< bit:      0  Counter Event on Channel 0 Interrupt Mask */
    uint32_t CHID1:1;                   /**< bit:      1  Counter Event on Channel 1 Interrupt Mask */
    uint32_t CHID2:1;                   /**< bit:      2  Counter Event on Channel 2 Interrupt Mask */
    uint32_t CHID3:1;                   /**< bit:      3  Counter Event on Channel 3 Interrupt Mask */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FCHID0:1;                  /**< bit:     16  Fault Protection Trigger on Channel 0 Interrupt Mask */
    uint32_t FCHID1:1;                  /**< bit:     17  Fault Protection Trigger on Channel 1 Interrupt Mask */
    uint32_t FCHID2:1;                  /**< bit:     18  Fault Protection Trigger on Channel 2 Interrupt Mask */
    uint32_t FCHID3:1;                  /**< bit:     19  Fault Protection Trigger on Channel 3 Interrupt Mask */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CHID:4;                    /**< bit:   0..3  Counter Event on Channel x Interrupt Mask */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FCHID:4;                   /**< bit: 16..19  Fault Protection Trigger on Channel 3 Interrupt Mask */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_IMR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_IMR1_OFFSET                     (0x18)                                        /**<  (PWM_IMR1) PWM Interrupt Mask Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_IMR1_CHID0_Pos                  0                                              /**< (PWM_IMR1) Counter Event on Channel 0 Interrupt Mask Position */
#define PWM_IMR1_CHID0_Msk                  (0x1U << PWM_IMR1_CHID0_Pos)                   /**< (PWM_IMR1) Counter Event on Channel 0 Interrupt Mask Mask */
#define PWM_IMR1_CHID0                      PWM_IMR1_CHID0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR1_CHID0_Msk instead */
#define PWM_IMR1_CHID1_Pos                  1                                              /**< (PWM_IMR1) Counter Event on Channel 1 Interrupt Mask Position */
#define PWM_IMR1_CHID1_Msk                  (0x1U << PWM_IMR1_CHID1_Pos)                   /**< (PWM_IMR1) Counter Event on Channel 1 Interrupt Mask Mask */
#define PWM_IMR1_CHID1                      PWM_IMR1_CHID1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR1_CHID1_Msk instead */
#define PWM_IMR1_CHID2_Pos                  2                                              /**< (PWM_IMR1) Counter Event on Channel 2 Interrupt Mask Position */
#define PWM_IMR1_CHID2_Msk                  (0x1U << PWM_IMR1_CHID2_Pos)                   /**< (PWM_IMR1) Counter Event on Channel 2 Interrupt Mask Mask */
#define PWM_IMR1_CHID2                      PWM_IMR1_CHID2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR1_CHID2_Msk instead */
#define PWM_IMR1_CHID3_Pos                  3                                              /**< (PWM_IMR1) Counter Event on Channel 3 Interrupt Mask Position */
#define PWM_IMR1_CHID3_Msk                  (0x1U << PWM_IMR1_CHID3_Pos)                   /**< (PWM_IMR1) Counter Event on Channel 3 Interrupt Mask Mask */
#define PWM_IMR1_CHID3                      PWM_IMR1_CHID3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR1_CHID3_Msk instead */
#define PWM_IMR1_FCHID0_Pos                 16                                             /**< (PWM_IMR1) Fault Protection Trigger on Channel 0 Interrupt Mask Position */
#define PWM_IMR1_FCHID0_Msk                 (0x1U << PWM_IMR1_FCHID0_Pos)                  /**< (PWM_IMR1) Fault Protection Trigger on Channel 0 Interrupt Mask Mask */
#define PWM_IMR1_FCHID0                     PWM_IMR1_FCHID0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR1_FCHID0_Msk instead */
#define PWM_IMR1_FCHID1_Pos                 17                                             /**< (PWM_IMR1) Fault Protection Trigger on Channel 1 Interrupt Mask Position */
#define PWM_IMR1_FCHID1_Msk                 (0x1U << PWM_IMR1_FCHID1_Pos)                  /**< (PWM_IMR1) Fault Protection Trigger on Channel 1 Interrupt Mask Mask */
#define PWM_IMR1_FCHID1                     PWM_IMR1_FCHID1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR1_FCHID1_Msk instead */
#define PWM_IMR1_FCHID2_Pos                 18                                             /**< (PWM_IMR1) Fault Protection Trigger on Channel 2 Interrupt Mask Position */
#define PWM_IMR1_FCHID2_Msk                 (0x1U << PWM_IMR1_FCHID2_Pos)                  /**< (PWM_IMR1) Fault Protection Trigger on Channel 2 Interrupt Mask Mask */
#define PWM_IMR1_FCHID2                     PWM_IMR1_FCHID2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR1_FCHID2_Msk instead */
#define PWM_IMR1_FCHID3_Pos                 19                                             /**< (PWM_IMR1) Fault Protection Trigger on Channel 3 Interrupt Mask Position */
#define PWM_IMR1_FCHID3_Msk                 (0x1U << PWM_IMR1_FCHID3_Pos)                  /**< (PWM_IMR1) Fault Protection Trigger on Channel 3 Interrupt Mask Mask */
#define PWM_IMR1_FCHID3                     PWM_IMR1_FCHID3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR1_FCHID3_Msk instead */
#define PWM_IMR1_CHID_Pos                   0                                              /**< (PWM_IMR1 Position) Counter Event on Channel x Interrupt Mask */
#define PWM_IMR1_CHID_Msk                   (0xFU << PWM_IMR1_CHID_Pos)                    /**< (PWM_IMR1 Mask) CHID */
#define PWM_IMR1_CHID(value)                (PWM_IMR1_CHID_Msk & ((value) << PWM_IMR1_CHID_Pos))  
#define PWM_IMR1_FCHID_Pos                  16                                             /**< (PWM_IMR1 Position) Fault Protection Trigger on Channel 3 Interrupt Mask */
#define PWM_IMR1_FCHID_Msk                  (0xFU << PWM_IMR1_FCHID_Pos)                   /**< (PWM_IMR1 Mask) FCHID */
#define PWM_IMR1_FCHID(value)               (PWM_IMR1_FCHID_Msk & ((value) << PWM_IMR1_FCHID_Pos))  
#define PWM_IMR1_MASK                       (0xF000FU)                                     /**< \deprecated (PWM_IMR1) Register MASK  (Use PWM_IMR1_Msk instead)  */
#define PWM_IMR1_Msk                        (0xF000FU)                                     /**< (PWM_IMR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_ISR1 : (PWM Offset: 0x1c) (R/ 32) PWM Interrupt Status Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CHID0:1;                   /**< bit:      0  Counter Event on Channel 0               */
    uint32_t CHID1:1;                   /**< bit:      1  Counter Event on Channel 1               */
    uint32_t CHID2:1;                   /**< bit:      2  Counter Event on Channel 2               */
    uint32_t CHID3:1;                   /**< bit:      3  Counter Event on Channel 3               */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FCHID0:1;                  /**< bit:     16  Fault Protection Trigger on Channel 0    */
    uint32_t FCHID1:1;                  /**< bit:     17  Fault Protection Trigger on Channel 1    */
    uint32_t FCHID2:1;                  /**< bit:     18  Fault Protection Trigger on Channel 2    */
    uint32_t FCHID3:1;                  /**< bit:     19  Fault Protection Trigger on Channel 3    */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CHID:4;                    /**< bit:   0..3  Counter Event on Channel x               */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FCHID:4;                   /**< bit: 16..19  Fault Protection Trigger on Channel 3    */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_ISR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_ISR1_OFFSET                     (0x1C)                                        /**<  (PWM_ISR1) PWM Interrupt Status Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_ISR1_CHID0_Pos                  0                                              /**< (PWM_ISR1) Counter Event on Channel 0 Position */
#define PWM_ISR1_CHID0_Msk                  (0x1U << PWM_ISR1_CHID0_Pos)                   /**< (PWM_ISR1) Counter Event on Channel 0 Mask */
#define PWM_ISR1_CHID0                      PWM_ISR1_CHID0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR1_CHID0_Msk instead */
#define PWM_ISR1_CHID1_Pos                  1                                              /**< (PWM_ISR1) Counter Event on Channel 1 Position */
#define PWM_ISR1_CHID1_Msk                  (0x1U << PWM_ISR1_CHID1_Pos)                   /**< (PWM_ISR1) Counter Event on Channel 1 Mask */
#define PWM_ISR1_CHID1                      PWM_ISR1_CHID1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR1_CHID1_Msk instead */
#define PWM_ISR1_CHID2_Pos                  2                                              /**< (PWM_ISR1) Counter Event on Channel 2 Position */
#define PWM_ISR1_CHID2_Msk                  (0x1U << PWM_ISR1_CHID2_Pos)                   /**< (PWM_ISR1) Counter Event on Channel 2 Mask */
#define PWM_ISR1_CHID2                      PWM_ISR1_CHID2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR1_CHID2_Msk instead */
#define PWM_ISR1_CHID3_Pos                  3                                              /**< (PWM_ISR1) Counter Event on Channel 3 Position */
#define PWM_ISR1_CHID3_Msk                  (0x1U << PWM_ISR1_CHID3_Pos)                   /**< (PWM_ISR1) Counter Event on Channel 3 Mask */
#define PWM_ISR1_CHID3                      PWM_ISR1_CHID3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR1_CHID3_Msk instead */
#define PWM_ISR1_FCHID0_Pos                 16                                             /**< (PWM_ISR1) Fault Protection Trigger on Channel 0 Position */
#define PWM_ISR1_FCHID0_Msk                 (0x1U << PWM_ISR1_FCHID0_Pos)                  /**< (PWM_ISR1) Fault Protection Trigger on Channel 0 Mask */
#define PWM_ISR1_FCHID0                     PWM_ISR1_FCHID0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR1_FCHID0_Msk instead */
#define PWM_ISR1_FCHID1_Pos                 17                                             /**< (PWM_ISR1) Fault Protection Trigger on Channel 1 Position */
#define PWM_ISR1_FCHID1_Msk                 (0x1U << PWM_ISR1_FCHID1_Pos)                  /**< (PWM_ISR1) Fault Protection Trigger on Channel 1 Mask */
#define PWM_ISR1_FCHID1                     PWM_ISR1_FCHID1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR1_FCHID1_Msk instead */
#define PWM_ISR1_FCHID2_Pos                 18                                             /**< (PWM_ISR1) Fault Protection Trigger on Channel 2 Position */
#define PWM_ISR1_FCHID2_Msk                 (0x1U << PWM_ISR1_FCHID2_Pos)                  /**< (PWM_ISR1) Fault Protection Trigger on Channel 2 Mask */
#define PWM_ISR1_FCHID2                     PWM_ISR1_FCHID2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR1_FCHID2_Msk instead */
#define PWM_ISR1_FCHID3_Pos                 19                                             /**< (PWM_ISR1) Fault Protection Trigger on Channel 3 Position */
#define PWM_ISR1_FCHID3_Msk                 (0x1U << PWM_ISR1_FCHID3_Pos)                  /**< (PWM_ISR1) Fault Protection Trigger on Channel 3 Mask */
#define PWM_ISR1_FCHID3                     PWM_ISR1_FCHID3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR1_FCHID3_Msk instead */
#define PWM_ISR1_CHID_Pos                   0                                              /**< (PWM_ISR1 Position) Counter Event on Channel x */
#define PWM_ISR1_CHID_Msk                   (0xFU << PWM_ISR1_CHID_Pos)                    /**< (PWM_ISR1 Mask) CHID */
#define PWM_ISR1_CHID(value)                (PWM_ISR1_CHID_Msk & ((value) << PWM_ISR1_CHID_Pos))  
#define PWM_ISR1_FCHID_Pos                  16                                             /**< (PWM_ISR1 Position) Fault Protection Trigger on Channel 3 */
#define PWM_ISR1_FCHID_Msk                  (0xFU << PWM_ISR1_FCHID_Pos)                   /**< (PWM_ISR1 Mask) FCHID */
#define PWM_ISR1_FCHID(value)               (PWM_ISR1_FCHID_Msk & ((value) << PWM_ISR1_FCHID_Pos))  
#define PWM_ISR1_MASK                       (0xF000FU)                                     /**< \deprecated (PWM_ISR1) Register MASK  (Use PWM_ISR1_Msk instead)  */
#define PWM_ISR1_Msk                        (0xF000FU)                                     /**< (PWM_ISR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_SCM : (PWM Offset: 0x20) (R/W 32) PWM Sync Channels Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SYNC0:1;                   /**< bit:      0  Synchronous Channel 0                    */
    uint32_t SYNC1:1;                   /**< bit:      1  Synchronous Channel 1                    */
    uint32_t SYNC2:1;                   /**< bit:      2  Synchronous Channel 2                    */
    uint32_t SYNC3:1;                   /**< bit:      3  Synchronous Channel 3                    */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t UPDM:2;                    /**< bit: 16..17  Synchronous Channels Update Mode         */
    uint32_t :2;                        /**< bit: 18..19  Reserved */
    uint32_t PTRM:1;                    /**< bit:     20  DMA Controller Transfer Request Mode     */
    uint32_t PTRCS:3;                   /**< bit: 21..23  DMA Controller Transfer Request Comparison Selection */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_SCM_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_SCM_OFFSET                      (0x20)                                        /**<  (PWM_SCM) PWM Sync Channels Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_SCM_SYNC0_Pos                   0                                              /**< (PWM_SCM) Synchronous Channel 0 Position */
#define PWM_SCM_SYNC0_Msk                   (0x1U << PWM_SCM_SYNC0_Pos)                    /**< (PWM_SCM) Synchronous Channel 0 Mask */
#define PWM_SCM_SYNC0                       PWM_SCM_SYNC0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SCM_SYNC0_Msk instead */
#define PWM_SCM_SYNC1_Pos                   1                                              /**< (PWM_SCM) Synchronous Channel 1 Position */
#define PWM_SCM_SYNC1_Msk                   (0x1U << PWM_SCM_SYNC1_Pos)                    /**< (PWM_SCM) Synchronous Channel 1 Mask */
#define PWM_SCM_SYNC1                       PWM_SCM_SYNC1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SCM_SYNC1_Msk instead */
#define PWM_SCM_SYNC2_Pos                   2                                              /**< (PWM_SCM) Synchronous Channel 2 Position */
#define PWM_SCM_SYNC2_Msk                   (0x1U << PWM_SCM_SYNC2_Pos)                    /**< (PWM_SCM) Synchronous Channel 2 Mask */
#define PWM_SCM_SYNC2                       PWM_SCM_SYNC2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SCM_SYNC2_Msk instead */
#define PWM_SCM_SYNC3_Pos                   3                                              /**< (PWM_SCM) Synchronous Channel 3 Position */
#define PWM_SCM_SYNC3_Msk                   (0x1U << PWM_SCM_SYNC3_Pos)                    /**< (PWM_SCM) Synchronous Channel 3 Mask */
#define PWM_SCM_SYNC3                       PWM_SCM_SYNC3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SCM_SYNC3_Msk instead */
#define PWM_SCM_UPDM_Pos                    16                                             /**< (PWM_SCM) Synchronous Channels Update Mode Position */
#define PWM_SCM_UPDM_Msk                    (0x3U << PWM_SCM_UPDM_Pos)                     /**< (PWM_SCM) Synchronous Channels Update Mode Mask */
#define PWM_SCM_UPDM(value)                 (PWM_SCM_UPDM_Msk & ((value) << PWM_SCM_UPDM_Pos))
#define   PWM_SCM_UPDM_MODE0_Val            (0x0U)                                         /**< (PWM_SCM) Manual write of double buffer registers and manual update of synchronous channels  */
#define   PWM_SCM_UPDM_MODE1_Val            (0x1U)                                         /**< (PWM_SCM) Manual write of double buffer registers and automatic update of synchronous channels  */
#define   PWM_SCM_UPDM_MODE2_Val            (0x2U)                                         /**< (PWM_SCM) Automatic write of duty-cycle update registers by the DMA Controller and automatic update of synchronous channels  */
#define PWM_SCM_UPDM_MODE0                  (PWM_SCM_UPDM_MODE0_Val << PWM_SCM_UPDM_Pos)   /**< (PWM_SCM) Manual write of double buffer registers and manual update of synchronous channels Position  */
#define PWM_SCM_UPDM_MODE1                  (PWM_SCM_UPDM_MODE1_Val << PWM_SCM_UPDM_Pos)   /**< (PWM_SCM) Manual write of double buffer registers and automatic update of synchronous channels Position  */
#define PWM_SCM_UPDM_MODE2                  (PWM_SCM_UPDM_MODE2_Val << PWM_SCM_UPDM_Pos)   /**< (PWM_SCM) Automatic write of duty-cycle update registers by the DMA Controller and automatic update of synchronous channels Position  */
#define PWM_SCM_PTRM_Pos                    20                                             /**< (PWM_SCM) DMA Controller Transfer Request Mode Position */
#define PWM_SCM_PTRM_Msk                    (0x1U << PWM_SCM_PTRM_Pos)                     /**< (PWM_SCM) DMA Controller Transfer Request Mode Mask */
#define PWM_SCM_PTRM                        PWM_SCM_PTRM_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SCM_PTRM_Msk instead */
#define PWM_SCM_PTRCS_Pos                   21                                             /**< (PWM_SCM) DMA Controller Transfer Request Comparison Selection Position */
#define PWM_SCM_PTRCS_Msk                   (0x7U << PWM_SCM_PTRCS_Pos)                    /**< (PWM_SCM) DMA Controller Transfer Request Comparison Selection Mask */
#define PWM_SCM_PTRCS(value)                (PWM_SCM_PTRCS_Msk & ((value) << PWM_SCM_PTRCS_Pos))
#define PWM_SCM_MASK                        (0xF3000FU)                                    /**< \deprecated (PWM_SCM) Register MASK  (Use PWM_SCM_Msk instead)  */
#define PWM_SCM_Msk                         (0xF3000FU)                                    /**< (PWM_SCM) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_DMAR : (PWM Offset: 0x24) (/W 32) PWM DMA Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DMADUTY:24;                /**< bit:  0..23  Duty-Cycle Holding Register for DMA Access */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_DMAR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_DMAR_OFFSET                     (0x24)                                        /**<  (PWM_DMAR) PWM DMA Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_DMAR_DMADUTY_Pos                0                                              /**< (PWM_DMAR) Duty-Cycle Holding Register for DMA Access Position */
#define PWM_DMAR_DMADUTY_Msk                (0xFFFFFFU << PWM_DMAR_DMADUTY_Pos)            /**< (PWM_DMAR) Duty-Cycle Holding Register for DMA Access Mask */
#define PWM_DMAR_DMADUTY(value)             (PWM_DMAR_DMADUTY_Msk & ((value) << PWM_DMAR_DMADUTY_Pos))
#define PWM_DMAR_MASK                       (0xFFFFFFU)                                    /**< \deprecated (PWM_DMAR) Register MASK  (Use PWM_DMAR_Msk instead)  */
#define PWM_DMAR_Msk                        (0xFFFFFFU)                                    /**< (PWM_DMAR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_SCUC : (PWM Offset: 0x28) (R/W 32) PWM Sync Channels Update Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t UPDULOCK:1;                /**< bit:      0  Synchronous Channels Update Unlock       */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_SCUC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_SCUC_OFFSET                     (0x28)                                        /**<  (PWM_SCUC) PWM Sync Channels Update Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_SCUC_UPDULOCK_Pos               0                                              /**< (PWM_SCUC) Synchronous Channels Update Unlock Position */
#define PWM_SCUC_UPDULOCK_Msk               (0x1U << PWM_SCUC_UPDULOCK_Pos)                /**< (PWM_SCUC) Synchronous Channels Update Unlock Mask */
#define PWM_SCUC_UPDULOCK                   PWM_SCUC_UPDULOCK_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SCUC_UPDULOCK_Msk instead */
#define PWM_SCUC_MASK                       (0x01U)                                        /**< \deprecated (PWM_SCUC) Register MASK  (Use PWM_SCUC_Msk instead)  */
#define PWM_SCUC_Msk                        (0x01U)                                        /**< (PWM_SCUC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_SCUP : (PWM Offset: 0x2c) (R/W 32) PWM Sync Channels Update Period Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t UPR:4;                     /**< bit:   0..3  Update Period                            */
    uint32_t UPRCNT:4;                  /**< bit:   4..7  Update Period Counter                    */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_SCUP_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_SCUP_OFFSET                     (0x2C)                                        /**<  (PWM_SCUP) PWM Sync Channels Update Period Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_SCUP_UPR_Pos                    0                                              /**< (PWM_SCUP) Update Period Position */
#define PWM_SCUP_UPR_Msk                    (0xFU << PWM_SCUP_UPR_Pos)                     /**< (PWM_SCUP) Update Period Mask */
#define PWM_SCUP_UPR(value)                 (PWM_SCUP_UPR_Msk & ((value) << PWM_SCUP_UPR_Pos))
#define PWM_SCUP_UPRCNT_Pos                 4                                              /**< (PWM_SCUP) Update Period Counter Position */
#define PWM_SCUP_UPRCNT_Msk                 (0xFU << PWM_SCUP_UPRCNT_Pos)                  /**< (PWM_SCUP) Update Period Counter Mask */
#define PWM_SCUP_UPRCNT(value)              (PWM_SCUP_UPRCNT_Msk & ((value) << PWM_SCUP_UPRCNT_Pos))
#define PWM_SCUP_MASK                       (0xFFU)                                        /**< \deprecated (PWM_SCUP) Register MASK  (Use PWM_SCUP_Msk instead)  */
#define PWM_SCUP_Msk                        (0xFFU)                                        /**< (PWM_SCUP) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_SCUPUPD : (PWM Offset: 0x30) (/W 32) PWM Sync Channels Update Period Update Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t UPRUPD:4;                  /**< bit:   0..3  Update Period Update                     */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_SCUPUPD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_SCUPUPD_OFFSET                  (0x30)                                        /**<  (PWM_SCUPUPD) PWM Sync Channels Update Period Update Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_SCUPUPD_UPRUPD_Pos              0                                              /**< (PWM_SCUPUPD) Update Period Update Position */
#define PWM_SCUPUPD_UPRUPD_Msk              (0xFU << PWM_SCUPUPD_UPRUPD_Pos)               /**< (PWM_SCUPUPD) Update Period Update Mask */
#define PWM_SCUPUPD_UPRUPD(value)           (PWM_SCUPUPD_UPRUPD_Msk & ((value) << PWM_SCUPUPD_UPRUPD_Pos))
#define PWM_SCUPUPD_MASK                    (0x0FU)                                        /**< \deprecated (PWM_SCUPUPD) Register MASK  (Use PWM_SCUPUPD_Msk instead)  */
#define PWM_SCUPUPD_Msk                     (0x0FU)                                        /**< (PWM_SCUPUPD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_IER2 : (PWM Offset: 0x34) (/W 32) PWM Interrupt Enable Register 2 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WRDY:1;                    /**< bit:      0  Write Ready for Synchronous Channels Update Interrupt Enable */
    uint32_t :2;                        /**< bit:   1..2  Reserved */
    uint32_t UNRE:1;                    /**< bit:      3  Synchronous Channels Update Underrun Error Interrupt Enable */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CMPM0:1;                   /**< bit:      8  Comparison 0 Match Interrupt Enable      */
    uint32_t CMPM1:1;                   /**< bit:      9  Comparison 1 Match Interrupt Enable      */
    uint32_t CMPM2:1;                   /**< bit:     10  Comparison 2 Match Interrupt Enable      */
    uint32_t CMPM3:1;                   /**< bit:     11  Comparison 3 Match Interrupt Enable      */
    uint32_t CMPM4:1;                   /**< bit:     12  Comparison 4 Match Interrupt Enable      */
    uint32_t CMPM5:1;                   /**< bit:     13  Comparison 5 Match Interrupt Enable      */
    uint32_t CMPM6:1;                   /**< bit:     14  Comparison 6 Match Interrupt Enable      */
    uint32_t CMPM7:1;                   /**< bit:     15  Comparison 7 Match Interrupt Enable      */
    uint32_t CMPU0:1;                   /**< bit:     16  Comparison 0 Update Interrupt Enable     */
    uint32_t CMPU1:1;                   /**< bit:     17  Comparison 1 Update Interrupt Enable     */
    uint32_t CMPU2:1;                   /**< bit:     18  Comparison 2 Update Interrupt Enable     */
    uint32_t CMPU3:1;                   /**< bit:     19  Comparison 3 Update Interrupt Enable     */
    uint32_t CMPU4:1;                   /**< bit:     20  Comparison 4 Update Interrupt Enable     */
    uint32_t CMPU5:1;                   /**< bit:     21  Comparison 5 Update Interrupt Enable     */
    uint32_t CMPU6:1;                   /**< bit:     22  Comparison 6 Update Interrupt Enable     */
    uint32_t CMPU7:1;                   /**< bit:     23  Comparison 7 Update Interrupt Enable     */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :8;                        /**< bit:   0..7  Reserved */
    uint32_t CMPM:8;                    /**< bit:  8..15  Comparison x Match Interrupt Enable      */
    uint32_t CMPU:8;                    /**< bit: 16..23  Comparison 7 Update Interrupt Enable     */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_IER2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_IER2_OFFSET                     (0x34)                                        /**<  (PWM_IER2) PWM Interrupt Enable Register 2  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_IER2_WRDY_Pos                   0                                              /**< (PWM_IER2) Write Ready for Synchronous Channels Update Interrupt Enable Position */
#define PWM_IER2_WRDY_Msk                   (0x1U << PWM_IER2_WRDY_Pos)                    /**< (PWM_IER2) Write Ready for Synchronous Channels Update Interrupt Enable Mask */
#define PWM_IER2_WRDY                       PWM_IER2_WRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_WRDY_Msk instead */
#define PWM_IER2_UNRE_Pos                   3                                              /**< (PWM_IER2) Synchronous Channels Update Underrun Error Interrupt Enable Position */
#define PWM_IER2_UNRE_Msk                   (0x1U << PWM_IER2_UNRE_Pos)                    /**< (PWM_IER2) Synchronous Channels Update Underrun Error Interrupt Enable Mask */
#define PWM_IER2_UNRE                       PWM_IER2_UNRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_UNRE_Msk instead */
#define PWM_IER2_CMPM0_Pos                  8                                              /**< (PWM_IER2) Comparison 0 Match Interrupt Enable Position */
#define PWM_IER2_CMPM0_Msk                  (0x1U << PWM_IER2_CMPM0_Pos)                   /**< (PWM_IER2) Comparison 0 Match Interrupt Enable Mask */
#define PWM_IER2_CMPM0                      PWM_IER2_CMPM0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPM0_Msk instead */
#define PWM_IER2_CMPM1_Pos                  9                                              /**< (PWM_IER2) Comparison 1 Match Interrupt Enable Position */
#define PWM_IER2_CMPM1_Msk                  (0x1U << PWM_IER2_CMPM1_Pos)                   /**< (PWM_IER2) Comparison 1 Match Interrupt Enable Mask */
#define PWM_IER2_CMPM1                      PWM_IER2_CMPM1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPM1_Msk instead */
#define PWM_IER2_CMPM2_Pos                  10                                             /**< (PWM_IER2) Comparison 2 Match Interrupt Enable Position */
#define PWM_IER2_CMPM2_Msk                  (0x1U << PWM_IER2_CMPM2_Pos)                   /**< (PWM_IER2) Comparison 2 Match Interrupt Enable Mask */
#define PWM_IER2_CMPM2                      PWM_IER2_CMPM2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPM2_Msk instead */
#define PWM_IER2_CMPM3_Pos                  11                                             /**< (PWM_IER2) Comparison 3 Match Interrupt Enable Position */
#define PWM_IER2_CMPM3_Msk                  (0x1U << PWM_IER2_CMPM3_Pos)                   /**< (PWM_IER2) Comparison 3 Match Interrupt Enable Mask */
#define PWM_IER2_CMPM3                      PWM_IER2_CMPM3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPM3_Msk instead */
#define PWM_IER2_CMPM4_Pos                  12                                             /**< (PWM_IER2) Comparison 4 Match Interrupt Enable Position */
#define PWM_IER2_CMPM4_Msk                  (0x1U << PWM_IER2_CMPM4_Pos)                   /**< (PWM_IER2) Comparison 4 Match Interrupt Enable Mask */
#define PWM_IER2_CMPM4                      PWM_IER2_CMPM4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPM4_Msk instead */
#define PWM_IER2_CMPM5_Pos                  13                                             /**< (PWM_IER2) Comparison 5 Match Interrupt Enable Position */
#define PWM_IER2_CMPM5_Msk                  (0x1U << PWM_IER2_CMPM5_Pos)                   /**< (PWM_IER2) Comparison 5 Match Interrupt Enable Mask */
#define PWM_IER2_CMPM5                      PWM_IER2_CMPM5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPM5_Msk instead */
#define PWM_IER2_CMPM6_Pos                  14                                             /**< (PWM_IER2) Comparison 6 Match Interrupt Enable Position */
#define PWM_IER2_CMPM6_Msk                  (0x1U << PWM_IER2_CMPM6_Pos)                   /**< (PWM_IER2) Comparison 6 Match Interrupt Enable Mask */
#define PWM_IER2_CMPM6                      PWM_IER2_CMPM6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPM6_Msk instead */
#define PWM_IER2_CMPM7_Pos                  15                                             /**< (PWM_IER2) Comparison 7 Match Interrupt Enable Position */
#define PWM_IER2_CMPM7_Msk                  (0x1U << PWM_IER2_CMPM7_Pos)                   /**< (PWM_IER2) Comparison 7 Match Interrupt Enable Mask */
#define PWM_IER2_CMPM7                      PWM_IER2_CMPM7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPM7_Msk instead */
#define PWM_IER2_CMPU0_Pos                  16                                             /**< (PWM_IER2) Comparison 0 Update Interrupt Enable Position */
#define PWM_IER2_CMPU0_Msk                  (0x1U << PWM_IER2_CMPU0_Pos)                   /**< (PWM_IER2) Comparison 0 Update Interrupt Enable Mask */
#define PWM_IER2_CMPU0                      PWM_IER2_CMPU0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPU0_Msk instead */
#define PWM_IER2_CMPU1_Pos                  17                                             /**< (PWM_IER2) Comparison 1 Update Interrupt Enable Position */
#define PWM_IER2_CMPU1_Msk                  (0x1U << PWM_IER2_CMPU1_Pos)                   /**< (PWM_IER2) Comparison 1 Update Interrupt Enable Mask */
#define PWM_IER2_CMPU1                      PWM_IER2_CMPU1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPU1_Msk instead */
#define PWM_IER2_CMPU2_Pos                  18                                             /**< (PWM_IER2) Comparison 2 Update Interrupt Enable Position */
#define PWM_IER2_CMPU2_Msk                  (0x1U << PWM_IER2_CMPU2_Pos)                   /**< (PWM_IER2) Comparison 2 Update Interrupt Enable Mask */
#define PWM_IER2_CMPU2                      PWM_IER2_CMPU2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPU2_Msk instead */
#define PWM_IER2_CMPU3_Pos                  19                                             /**< (PWM_IER2) Comparison 3 Update Interrupt Enable Position */
#define PWM_IER2_CMPU3_Msk                  (0x1U << PWM_IER2_CMPU3_Pos)                   /**< (PWM_IER2) Comparison 3 Update Interrupt Enable Mask */
#define PWM_IER2_CMPU3                      PWM_IER2_CMPU3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPU3_Msk instead */
#define PWM_IER2_CMPU4_Pos                  20                                             /**< (PWM_IER2) Comparison 4 Update Interrupt Enable Position */
#define PWM_IER2_CMPU4_Msk                  (0x1U << PWM_IER2_CMPU4_Pos)                   /**< (PWM_IER2) Comparison 4 Update Interrupt Enable Mask */
#define PWM_IER2_CMPU4                      PWM_IER2_CMPU4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPU4_Msk instead */
#define PWM_IER2_CMPU5_Pos                  21                                             /**< (PWM_IER2) Comparison 5 Update Interrupt Enable Position */
#define PWM_IER2_CMPU5_Msk                  (0x1U << PWM_IER2_CMPU5_Pos)                   /**< (PWM_IER2) Comparison 5 Update Interrupt Enable Mask */
#define PWM_IER2_CMPU5                      PWM_IER2_CMPU5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPU5_Msk instead */
#define PWM_IER2_CMPU6_Pos                  22                                             /**< (PWM_IER2) Comparison 6 Update Interrupt Enable Position */
#define PWM_IER2_CMPU6_Msk                  (0x1U << PWM_IER2_CMPU6_Pos)                   /**< (PWM_IER2) Comparison 6 Update Interrupt Enable Mask */
#define PWM_IER2_CMPU6                      PWM_IER2_CMPU6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPU6_Msk instead */
#define PWM_IER2_CMPU7_Pos                  23                                             /**< (PWM_IER2) Comparison 7 Update Interrupt Enable Position */
#define PWM_IER2_CMPU7_Msk                  (0x1U << PWM_IER2_CMPU7_Pos)                   /**< (PWM_IER2) Comparison 7 Update Interrupt Enable Mask */
#define PWM_IER2_CMPU7                      PWM_IER2_CMPU7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IER2_CMPU7_Msk instead */
#define PWM_IER2_CMPM_Pos                   8                                              /**< (PWM_IER2 Position) Comparison x Match Interrupt Enable */
#define PWM_IER2_CMPM_Msk                   (0xFFU << PWM_IER2_CMPM_Pos)                   /**< (PWM_IER2 Mask) CMPM */
#define PWM_IER2_CMPM(value)                (PWM_IER2_CMPM_Msk & ((value) << PWM_IER2_CMPM_Pos))  
#define PWM_IER2_CMPU_Pos                   16                                             /**< (PWM_IER2 Position) Comparison 7 Update Interrupt Enable */
#define PWM_IER2_CMPU_Msk                   (0xFFU << PWM_IER2_CMPU_Pos)                   /**< (PWM_IER2 Mask) CMPU */
#define PWM_IER2_CMPU(value)                (PWM_IER2_CMPU_Msk & ((value) << PWM_IER2_CMPU_Pos))  
#define PWM_IER2_MASK                       (0xFFFF09U)                                    /**< \deprecated (PWM_IER2) Register MASK  (Use PWM_IER2_Msk instead)  */
#define PWM_IER2_Msk                        (0xFFFF09U)                                    /**< (PWM_IER2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_IDR2 : (PWM Offset: 0x38) (/W 32) PWM Interrupt Disable Register 2 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WRDY:1;                    /**< bit:      0  Write Ready for Synchronous Channels Update Interrupt Disable */
    uint32_t :2;                        /**< bit:   1..2  Reserved */
    uint32_t UNRE:1;                    /**< bit:      3  Synchronous Channels Update Underrun Error Interrupt Disable */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CMPM0:1;                   /**< bit:      8  Comparison 0 Match Interrupt Disable     */
    uint32_t CMPM1:1;                   /**< bit:      9  Comparison 1 Match Interrupt Disable     */
    uint32_t CMPM2:1;                   /**< bit:     10  Comparison 2 Match Interrupt Disable     */
    uint32_t CMPM3:1;                   /**< bit:     11  Comparison 3 Match Interrupt Disable     */
    uint32_t CMPM4:1;                   /**< bit:     12  Comparison 4 Match Interrupt Disable     */
    uint32_t CMPM5:1;                   /**< bit:     13  Comparison 5 Match Interrupt Disable     */
    uint32_t CMPM6:1;                   /**< bit:     14  Comparison 6 Match Interrupt Disable     */
    uint32_t CMPM7:1;                   /**< bit:     15  Comparison 7 Match Interrupt Disable     */
    uint32_t CMPU0:1;                   /**< bit:     16  Comparison 0 Update Interrupt Disable    */
    uint32_t CMPU1:1;                   /**< bit:     17  Comparison 1 Update Interrupt Disable    */
    uint32_t CMPU2:1;                   /**< bit:     18  Comparison 2 Update Interrupt Disable    */
    uint32_t CMPU3:1;                   /**< bit:     19  Comparison 3 Update Interrupt Disable    */
    uint32_t CMPU4:1;                   /**< bit:     20  Comparison 4 Update Interrupt Disable    */
    uint32_t CMPU5:1;                   /**< bit:     21  Comparison 5 Update Interrupt Disable    */
    uint32_t CMPU6:1;                   /**< bit:     22  Comparison 6 Update Interrupt Disable    */
    uint32_t CMPU7:1;                   /**< bit:     23  Comparison 7 Update Interrupt Disable    */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :8;                        /**< bit:   0..7  Reserved */
    uint32_t CMPM:8;                    /**< bit:  8..15  Comparison x Match Interrupt Disable     */
    uint32_t CMPU:8;                    /**< bit: 16..23  Comparison 7 Update Interrupt Disable    */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_IDR2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_IDR2_OFFSET                     (0x38)                                        /**<  (PWM_IDR2) PWM Interrupt Disable Register 2  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_IDR2_WRDY_Pos                   0                                              /**< (PWM_IDR2) Write Ready for Synchronous Channels Update Interrupt Disable Position */
#define PWM_IDR2_WRDY_Msk                   (0x1U << PWM_IDR2_WRDY_Pos)                    /**< (PWM_IDR2) Write Ready for Synchronous Channels Update Interrupt Disable Mask */
#define PWM_IDR2_WRDY                       PWM_IDR2_WRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_WRDY_Msk instead */
#define PWM_IDR2_UNRE_Pos                   3                                              /**< (PWM_IDR2) Synchronous Channels Update Underrun Error Interrupt Disable Position */
#define PWM_IDR2_UNRE_Msk                   (0x1U << PWM_IDR2_UNRE_Pos)                    /**< (PWM_IDR2) Synchronous Channels Update Underrun Error Interrupt Disable Mask */
#define PWM_IDR2_UNRE                       PWM_IDR2_UNRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_UNRE_Msk instead */
#define PWM_IDR2_CMPM0_Pos                  8                                              /**< (PWM_IDR2) Comparison 0 Match Interrupt Disable Position */
#define PWM_IDR2_CMPM0_Msk                  (0x1U << PWM_IDR2_CMPM0_Pos)                   /**< (PWM_IDR2) Comparison 0 Match Interrupt Disable Mask */
#define PWM_IDR2_CMPM0                      PWM_IDR2_CMPM0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPM0_Msk instead */
#define PWM_IDR2_CMPM1_Pos                  9                                              /**< (PWM_IDR2) Comparison 1 Match Interrupt Disable Position */
#define PWM_IDR2_CMPM1_Msk                  (0x1U << PWM_IDR2_CMPM1_Pos)                   /**< (PWM_IDR2) Comparison 1 Match Interrupt Disable Mask */
#define PWM_IDR2_CMPM1                      PWM_IDR2_CMPM1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPM1_Msk instead */
#define PWM_IDR2_CMPM2_Pos                  10                                             /**< (PWM_IDR2) Comparison 2 Match Interrupt Disable Position */
#define PWM_IDR2_CMPM2_Msk                  (0x1U << PWM_IDR2_CMPM2_Pos)                   /**< (PWM_IDR2) Comparison 2 Match Interrupt Disable Mask */
#define PWM_IDR2_CMPM2                      PWM_IDR2_CMPM2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPM2_Msk instead */
#define PWM_IDR2_CMPM3_Pos                  11                                             /**< (PWM_IDR2) Comparison 3 Match Interrupt Disable Position */
#define PWM_IDR2_CMPM3_Msk                  (0x1U << PWM_IDR2_CMPM3_Pos)                   /**< (PWM_IDR2) Comparison 3 Match Interrupt Disable Mask */
#define PWM_IDR2_CMPM3                      PWM_IDR2_CMPM3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPM3_Msk instead */
#define PWM_IDR2_CMPM4_Pos                  12                                             /**< (PWM_IDR2) Comparison 4 Match Interrupt Disable Position */
#define PWM_IDR2_CMPM4_Msk                  (0x1U << PWM_IDR2_CMPM4_Pos)                   /**< (PWM_IDR2) Comparison 4 Match Interrupt Disable Mask */
#define PWM_IDR2_CMPM4                      PWM_IDR2_CMPM4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPM4_Msk instead */
#define PWM_IDR2_CMPM5_Pos                  13                                             /**< (PWM_IDR2) Comparison 5 Match Interrupt Disable Position */
#define PWM_IDR2_CMPM5_Msk                  (0x1U << PWM_IDR2_CMPM5_Pos)                   /**< (PWM_IDR2) Comparison 5 Match Interrupt Disable Mask */
#define PWM_IDR2_CMPM5                      PWM_IDR2_CMPM5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPM5_Msk instead */
#define PWM_IDR2_CMPM6_Pos                  14                                             /**< (PWM_IDR2) Comparison 6 Match Interrupt Disable Position */
#define PWM_IDR2_CMPM6_Msk                  (0x1U << PWM_IDR2_CMPM6_Pos)                   /**< (PWM_IDR2) Comparison 6 Match Interrupt Disable Mask */
#define PWM_IDR2_CMPM6                      PWM_IDR2_CMPM6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPM6_Msk instead */
#define PWM_IDR2_CMPM7_Pos                  15                                             /**< (PWM_IDR2) Comparison 7 Match Interrupt Disable Position */
#define PWM_IDR2_CMPM7_Msk                  (0x1U << PWM_IDR2_CMPM7_Pos)                   /**< (PWM_IDR2) Comparison 7 Match Interrupt Disable Mask */
#define PWM_IDR2_CMPM7                      PWM_IDR2_CMPM7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPM7_Msk instead */
#define PWM_IDR2_CMPU0_Pos                  16                                             /**< (PWM_IDR2) Comparison 0 Update Interrupt Disable Position */
#define PWM_IDR2_CMPU0_Msk                  (0x1U << PWM_IDR2_CMPU0_Pos)                   /**< (PWM_IDR2) Comparison 0 Update Interrupt Disable Mask */
#define PWM_IDR2_CMPU0                      PWM_IDR2_CMPU0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPU0_Msk instead */
#define PWM_IDR2_CMPU1_Pos                  17                                             /**< (PWM_IDR2) Comparison 1 Update Interrupt Disable Position */
#define PWM_IDR2_CMPU1_Msk                  (0x1U << PWM_IDR2_CMPU1_Pos)                   /**< (PWM_IDR2) Comparison 1 Update Interrupt Disable Mask */
#define PWM_IDR2_CMPU1                      PWM_IDR2_CMPU1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPU1_Msk instead */
#define PWM_IDR2_CMPU2_Pos                  18                                             /**< (PWM_IDR2) Comparison 2 Update Interrupt Disable Position */
#define PWM_IDR2_CMPU2_Msk                  (0x1U << PWM_IDR2_CMPU2_Pos)                   /**< (PWM_IDR2) Comparison 2 Update Interrupt Disable Mask */
#define PWM_IDR2_CMPU2                      PWM_IDR2_CMPU2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPU2_Msk instead */
#define PWM_IDR2_CMPU3_Pos                  19                                             /**< (PWM_IDR2) Comparison 3 Update Interrupt Disable Position */
#define PWM_IDR2_CMPU3_Msk                  (0x1U << PWM_IDR2_CMPU3_Pos)                   /**< (PWM_IDR2) Comparison 3 Update Interrupt Disable Mask */
#define PWM_IDR2_CMPU3                      PWM_IDR2_CMPU3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPU3_Msk instead */
#define PWM_IDR2_CMPU4_Pos                  20                                             /**< (PWM_IDR2) Comparison 4 Update Interrupt Disable Position */
#define PWM_IDR2_CMPU4_Msk                  (0x1U << PWM_IDR2_CMPU4_Pos)                   /**< (PWM_IDR2) Comparison 4 Update Interrupt Disable Mask */
#define PWM_IDR2_CMPU4                      PWM_IDR2_CMPU4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPU4_Msk instead */
#define PWM_IDR2_CMPU5_Pos                  21                                             /**< (PWM_IDR2) Comparison 5 Update Interrupt Disable Position */
#define PWM_IDR2_CMPU5_Msk                  (0x1U << PWM_IDR2_CMPU5_Pos)                   /**< (PWM_IDR2) Comparison 5 Update Interrupt Disable Mask */
#define PWM_IDR2_CMPU5                      PWM_IDR2_CMPU5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPU5_Msk instead */
#define PWM_IDR2_CMPU6_Pos                  22                                             /**< (PWM_IDR2) Comparison 6 Update Interrupt Disable Position */
#define PWM_IDR2_CMPU6_Msk                  (0x1U << PWM_IDR2_CMPU6_Pos)                   /**< (PWM_IDR2) Comparison 6 Update Interrupt Disable Mask */
#define PWM_IDR2_CMPU6                      PWM_IDR2_CMPU6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPU6_Msk instead */
#define PWM_IDR2_CMPU7_Pos                  23                                             /**< (PWM_IDR2) Comparison 7 Update Interrupt Disable Position */
#define PWM_IDR2_CMPU7_Msk                  (0x1U << PWM_IDR2_CMPU7_Pos)                   /**< (PWM_IDR2) Comparison 7 Update Interrupt Disable Mask */
#define PWM_IDR2_CMPU7                      PWM_IDR2_CMPU7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IDR2_CMPU7_Msk instead */
#define PWM_IDR2_CMPM_Pos                   8                                              /**< (PWM_IDR2 Position) Comparison x Match Interrupt Disable */
#define PWM_IDR2_CMPM_Msk                   (0xFFU << PWM_IDR2_CMPM_Pos)                   /**< (PWM_IDR2 Mask) CMPM */
#define PWM_IDR2_CMPM(value)                (PWM_IDR2_CMPM_Msk & ((value) << PWM_IDR2_CMPM_Pos))  
#define PWM_IDR2_CMPU_Pos                   16                                             /**< (PWM_IDR2 Position) Comparison 7 Update Interrupt Disable */
#define PWM_IDR2_CMPU_Msk                   (0xFFU << PWM_IDR2_CMPU_Pos)                   /**< (PWM_IDR2 Mask) CMPU */
#define PWM_IDR2_CMPU(value)                (PWM_IDR2_CMPU_Msk & ((value) << PWM_IDR2_CMPU_Pos))  
#define PWM_IDR2_MASK                       (0xFFFF09U)                                    /**< \deprecated (PWM_IDR2) Register MASK  (Use PWM_IDR2_Msk instead)  */
#define PWM_IDR2_Msk                        (0xFFFF09U)                                    /**< (PWM_IDR2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_IMR2 : (PWM Offset: 0x3c) (R/ 32) PWM Interrupt Mask Register 2 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WRDY:1;                    /**< bit:      0  Write Ready for Synchronous Channels Update Interrupt Mask */
    uint32_t :2;                        /**< bit:   1..2  Reserved */
    uint32_t UNRE:1;                    /**< bit:      3  Synchronous Channels Update Underrun Error Interrupt Mask */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CMPM0:1;                   /**< bit:      8  Comparison 0 Match Interrupt Mask        */
    uint32_t CMPM1:1;                   /**< bit:      9  Comparison 1 Match Interrupt Mask        */
    uint32_t CMPM2:1;                   /**< bit:     10  Comparison 2 Match Interrupt Mask        */
    uint32_t CMPM3:1;                   /**< bit:     11  Comparison 3 Match Interrupt Mask        */
    uint32_t CMPM4:1;                   /**< bit:     12  Comparison 4 Match Interrupt Mask        */
    uint32_t CMPM5:1;                   /**< bit:     13  Comparison 5 Match Interrupt Mask        */
    uint32_t CMPM6:1;                   /**< bit:     14  Comparison 6 Match Interrupt Mask        */
    uint32_t CMPM7:1;                   /**< bit:     15  Comparison 7 Match Interrupt Mask        */
    uint32_t CMPU0:1;                   /**< bit:     16  Comparison 0 Update Interrupt Mask       */
    uint32_t CMPU1:1;                   /**< bit:     17  Comparison 1 Update Interrupt Mask       */
    uint32_t CMPU2:1;                   /**< bit:     18  Comparison 2 Update Interrupt Mask       */
    uint32_t CMPU3:1;                   /**< bit:     19  Comparison 3 Update Interrupt Mask       */
    uint32_t CMPU4:1;                   /**< bit:     20  Comparison 4 Update Interrupt Mask       */
    uint32_t CMPU5:1;                   /**< bit:     21  Comparison 5 Update Interrupt Mask       */
    uint32_t CMPU6:1;                   /**< bit:     22  Comparison 6 Update Interrupt Mask       */
    uint32_t CMPU7:1;                   /**< bit:     23  Comparison 7 Update Interrupt Mask       */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :8;                        /**< bit:   0..7  Reserved */
    uint32_t CMPM:8;                    /**< bit:  8..15  Comparison x Match Interrupt Mask        */
    uint32_t CMPU:8;                    /**< bit: 16..23  Comparison 7 Update Interrupt Mask       */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_IMR2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_IMR2_OFFSET                     (0x3C)                                        /**<  (PWM_IMR2) PWM Interrupt Mask Register 2  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_IMR2_WRDY_Pos                   0                                              /**< (PWM_IMR2) Write Ready for Synchronous Channels Update Interrupt Mask Position */
#define PWM_IMR2_WRDY_Msk                   (0x1U << PWM_IMR2_WRDY_Pos)                    /**< (PWM_IMR2) Write Ready for Synchronous Channels Update Interrupt Mask Mask */
#define PWM_IMR2_WRDY                       PWM_IMR2_WRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_WRDY_Msk instead */
#define PWM_IMR2_UNRE_Pos                   3                                              /**< (PWM_IMR2) Synchronous Channels Update Underrun Error Interrupt Mask Position */
#define PWM_IMR2_UNRE_Msk                   (0x1U << PWM_IMR2_UNRE_Pos)                    /**< (PWM_IMR2) Synchronous Channels Update Underrun Error Interrupt Mask Mask */
#define PWM_IMR2_UNRE                       PWM_IMR2_UNRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_UNRE_Msk instead */
#define PWM_IMR2_CMPM0_Pos                  8                                              /**< (PWM_IMR2) Comparison 0 Match Interrupt Mask Position */
#define PWM_IMR2_CMPM0_Msk                  (0x1U << PWM_IMR2_CMPM0_Pos)                   /**< (PWM_IMR2) Comparison 0 Match Interrupt Mask Mask */
#define PWM_IMR2_CMPM0                      PWM_IMR2_CMPM0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPM0_Msk instead */
#define PWM_IMR2_CMPM1_Pos                  9                                              /**< (PWM_IMR2) Comparison 1 Match Interrupt Mask Position */
#define PWM_IMR2_CMPM1_Msk                  (0x1U << PWM_IMR2_CMPM1_Pos)                   /**< (PWM_IMR2) Comparison 1 Match Interrupt Mask Mask */
#define PWM_IMR2_CMPM1                      PWM_IMR2_CMPM1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPM1_Msk instead */
#define PWM_IMR2_CMPM2_Pos                  10                                             /**< (PWM_IMR2) Comparison 2 Match Interrupt Mask Position */
#define PWM_IMR2_CMPM2_Msk                  (0x1U << PWM_IMR2_CMPM2_Pos)                   /**< (PWM_IMR2) Comparison 2 Match Interrupt Mask Mask */
#define PWM_IMR2_CMPM2                      PWM_IMR2_CMPM2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPM2_Msk instead */
#define PWM_IMR2_CMPM3_Pos                  11                                             /**< (PWM_IMR2) Comparison 3 Match Interrupt Mask Position */
#define PWM_IMR2_CMPM3_Msk                  (0x1U << PWM_IMR2_CMPM3_Pos)                   /**< (PWM_IMR2) Comparison 3 Match Interrupt Mask Mask */
#define PWM_IMR2_CMPM3                      PWM_IMR2_CMPM3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPM3_Msk instead */
#define PWM_IMR2_CMPM4_Pos                  12                                             /**< (PWM_IMR2) Comparison 4 Match Interrupt Mask Position */
#define PWM_IMR2_CMPM4_Msk                  (0x1U << PWM_IMR2_CMPM4_Pos)                   /**< (PWM_IMR2) Comparison 4 Match Interrupt Mask Mask */
#define PWM_IMR2_CMPM4                      PWM_IMR2_CMPM4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPM4_Msk instead */
#define PWM_IMR2_CMPM5_Pos                  13                                             /**< (PWM_IMR2) Comparison 5 Match Interrupt Mask Position */
#define PWM_IMR2_CMPM5_Msk                  (0x1U << PWM_IMR2_CMPM5_Pos)                   /**< (PWM_IMR2) Comparison 5 Match Interrupt Mask Mask */
#define PWM_IMR2_CMPM5                      PWM_IMR2_CMPM5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPM5_Msk instead */
#define PWM_IMR2_CMPM6_Pos                  14                                             /**< (PWM_IMR2) Comparison 6 Match Interrupt Mask Position */
#define PWM_IMR2_CMPM6_Msk                  (0x1U << PWM_IMR2_CMPM6_Pos)                   /**< (PWM_IMR2) Comparison 6 Match Interrupt Mask Mask */
#define PWM_IMR2_CMPM6                      PWM_IMR2_CMPM6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPM6_Msk instead */
#define PWM_IMR2_CMPM7_Pos                  15                                             /**< (PWM_IMR2) Comparison 7 Match Interrupt Mask Position */
#define PWM_IMR2_CMPM7_Msk                  (0x1U << PWM_IMR2_CMPM7_Pos)                   /**< (PWM_IMR2) Comparison 7 Match Interrupt Mask Mask */
#define PWM_IMR2_CMPM7                      PWM_IMR2_CMPM7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPM7_Msk instead */
#define PWM_IMR2_CMPU0_Pos                  16                                             /**< (PWM_IMR2) Comparison 0 Update Interrupt Mask Position */
#define PWM_IMR2_CMPU0_Msk                  (0x1U << PWM_IMR2_CMPU0_Pos)                   /**< (PWM_IMR2) Comparison 0 Update Interrupt Mask Mask */
#define PWM_IMR2_CMPU0                      PWM_IMR2_CMPU0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPU0_Msk instead */
#define PWM_IMR2_CMPU1_Pos                  17                                             /**< (PWM_IMR2) Comparison 1 Update Interrupt Mask Position */
#define PWM_IMR2_CMPU1_Msk                  (0x1U << PWM_IMR2_CMPU1_Pos)                   /**< (PWM_IMR2) Comparison 1 Update Interrupt Mask Mask */
#define PWM_IMR2_CMPU1                      PWM_IMR2_CMPU1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPU1_Msk instead */
#define PWM_IMR2_CMPU2_Pos                  18                                             /**< (PWM_IMR2) Comparison 2 Update Interrupt Mask Position */
#define PWM_IMR2_CMPU2_Msk                  (0x1U << PWM_IMR2_CMPU2_Pos)                   /**< (PWM_IMR2) Comparison 2 Update Interrupt Mask Mask */
#define PWM_IMR2_CMPU2                      PWM_IMR2_CMPU2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPU2_Msk instead */
#define PWM_IMR2_CMPU3_Pos                  19                                             /**< (PWM_IMR2) Comparison 3 Update Interrupt Mask Position */
#define PWM_IMR2_CMPU3_Msk                  (0x1U << PWM_IMR2_CMPU3_Pos)                   /**< (PWM_IMR2) Comparison 3 Update Interrupt Mask Mask */
#define PWM_IMR2_CMPU3                      PWM_IMR2_CMPU3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPU3_Msk instead */
#define PWM_IMR2_CMPU4_Pos                  20                                             /**< (PWM_IMR2) Comparison 4 Update Interrupt Mask Position */
#define PWM_IMR2_CMPU4_Msk                  (0x1U << PWM_IMR2_CMPU4_Pos)                   /**< (PWM_IMR2) Comparison 4 Update Interrupt Mask Mask */
#define PWM_IMR2_CMPU4                      PWM_IMR2_CMPU4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPU4_Msk instead */
#define PWM_IMR2_CMPU5_Pos                  21                                             /**< (PWM_IMR2) Comparison 5 Update Interrupt Mask Position */
#define PWM_IMR2_CMPU5_Msk                  (0x1U << PWM_IMR2_CMPU5_Pos)                   /**< (PWM_IMR2) Comparison 5 Update Interrupt Mask Mask */
#define PWM_IMR2_CMPU5                      PWM_IMR2_CMPU5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPU5_Msk instead */
#define PWM_IMR2_CMPU6_Pos                  22                                             /**< (PWM_IMR2) Comparison 6 Update Interrupt Mask Position */
#define PWM_IMR2_CMPU6_Msk                  (0x1U << PWM_IMR2_CMPU6_Pos)                   /**< (PWM_IMR2) Comparison 6 Update Interrupt Mask Mask */
#define PWM_IMR2_CMPU6                      PWM_IMR2_CMPU6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPU6_Msk instead */
#define PWM_IMR2_CMPU7_Pos                  23                                             /**< (PWM_IMR2) Comparison 7 Update Interrupt Mask Position */
#define PWM_IMR2_CMPU7_Msk                  (0x1U << PWM_IMR2_CMPU7_Pos)                   /**< (PWM_IMR2) Comparison 7 Update Interrupt Mask Mask */
#define PWM_IMR2_CMPU7                      PWM_IMR2_CMPU7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_IMR2_CMPU7_Msk instead */
#define PWM_IMR2_CMPM_Pos                   8                                              /**< (PWM_IMR2 Position) Comparison x Match Interrupt Mask */
#define PWM_IMR2_CMPM_Msk                   (0xFFU << PWM_IMR2_CMPM_Pos)                   /**< (PWM_IMR2 Mask) CMPM */
#define PWM_IMR2_CMPM(value)                (PWM_IMR2_CMPM_Msk & ((value) << PWM_IMR2_CMPM_Pos))  
#define PWM_IMR2_CMPU_Pos                   16                                             /**< (PWM_IMR2 Position) Comparison 7 Update Interrupt Mask */
#define PWM_IMR2_CMPU_Msk                   (0xFFU << PWM_IMR2_CMPU_Pos)                   /**< (PWM_IMR2 Mask) CMPU */
#define PWM_IMR2_CMPU(value)                (PWM_IMR2_CMPU_Msk & ((value) << PWM_IMR2_CMPU_Pos))  
#define PWM_IMR2_MASK                       (0xFFFF09U)                                    /**< \deprecated (PWM_IMR2) Register MASK  (Use PWM_IMR2_Msk instead)  */
#define PWM_IMR2_Msk                        (0xFFFF09U)                                    /**< (PWM_IMR2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_ISR2 : (PWM Offset: 0x40) (R/ 32) PWM Interrupt Status Register 2 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WRDY:1;                    /**< bit:      0  Write Ready for Synchronous Channels Update */
    uint32_t :2;                        /**< bit:   1..2  Reserved */
    uint32_t UNRE:1;                    /**< bit:      3  Synchronous Channels Update Underrun Error */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CMPM0:1;                   /**< bit:      8  Comparison 0 Match                       */
    uint32_t CMPM1:1;                   /**< bit:      9  Comparison 1 Match                       */
    uint32_t CMPM2:1;                   /**< bit:     10  Comparison 2 Match                       */
    uint32_t CMPM3:1;                   /**< bit:     11  Comparison 3 Match                       */
    uint32_t CMPM4:1;                   /**< bit:     12  Comparison 4 Match                       */
    uint32_t CMPM5:1;                   /**< bit:     13  Comparison 5 Match                       */
    uint32_t CMPM6:1;                   /**< bit:     14  Comparison 6 Match                       */
    uint32_t CMPM7:1;                   /**< bit:     15  Comparison 7 Match                       */
    uint32_t CMPU0:1;                   /**< bit:     16  Comparison 0 Update                      */
    uint32_t CMPU1:1;                   /**< bit:     17  Comparison 1 Update                      */
    uint32_t CMPU2:1;                   /**< bit:     18  Comparison 2 Update                      */
    uint32_t CMPU3:1;                   /**< bit:     19  Comparison 3 Update                      */
    uint32_t CMPU4:1;                   /**< bit:     20  Comparison 4 Update                      */
    uint32_t CMPU5:1;                   /**< bit:     21  Comparison 5 Update                      */
    uint32_t CMPU6:1;                   /**< bit:     22  Comparison 6 Update                      */
    uint32_t CMPU7:1;                   /**< bit:     23  Comparison 7 Update                      */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :8;                        /**< bit:   0..7  Reserved */
    uint32_t CMPM:8;                    /**< bit:  8..15  Comparison x Match                       */
    uint32_t CMPU:8;                    /**< bit: 16..23  Comparison 7 Update                      */
    uint32_t :8;                        /**< bit: 24..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_ISR2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_ISR2_OFFSET                     (0x40)                                        /**<  (PWM_ISR2) PWM Interrupt Status Register 2  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_ISR2_WRDY_Pos                   0                                              /**< (PWM_ISR2) Write Ready for Synchronous Channels Update Position */
#define PWM_ISR2_WRDY_Msk                   (0x1U << PWM_ISR2_WRDY_Pos)                    /**< (PWM_ISR2) Write Ready for Synchronous Channels Update Mask */
#define PWM_ISR2_WRDY                       PWM_ISR2_WRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_WRDY_Msk instead */
#define PWM_ISR2_UNRE_Pos                   3                                              /**< (PWM_ISR2) Synchronous Channels Update Underrun Error Position */
#define PWM_ISR2_UNRE_Msk                   (0x1U << PWM_ISR2_UNRE_Pos)                    /**< (PWM_ISR2) Synchronous Channels Update Underrun Error Mask */
#define PWM_ISR2_UNRE                       PWM_ISR2_UNRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_UNRE_Msk instead */
#define PWM_ISR2_CMPM0_Pos                  8                                              /**< (PWM_ISR2) Comparison 0 Match Position */
#define PWM_ISR2_CMPM0_Msk                  (0x1U << PWM_ISR2_CMPM0_Pos)                   /**< (PWM_ISR2) Comparison 0 Match Mask */
#define PWM_ISR2_CMPM0                      PWM_ISR2_CMPM0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPM0_Msk instead */
#define PWM_ISR2_CMPM1_Pos                  9                                              /**< (PWM_ISR2) Comparison 1 Match Position */
#define PWM_ISR2_CMPM1_Msk                  (0x1U << PWM_ISR2_CMPM1_Pos)                   /**< (PWM_ISR2) Comparison 1 Match Mask */
#define PWM_ISR2_CMPM1                      PWM_ISR2_CMPM1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPM1_Msk instead */
#define PWM_ISR2_CMPM2_Pos                  10                                             /**< (PWM_ISR2) Comparison 2 Match Position */
#define PWM_ISR2_CMPM2_Msk                  (0x1U << PWM_ISR2_CMPM2_Pos)                   /**< (PWM_ISR2) Comparison 2 Match Mask */
#define PWM_ISR2_CMPM2                      PWM_ISR2_CMPM2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPM2_Msk instead */
#define PWM_ISR2_CMPM3_Pos                  11                                             /**< (PWM_ISR2) Comparison 3 Match Position */
#define PWM_ISR2_CMPM3_Msk                  (0x1U << PWM_ISR2_CMPM3_Pos)                   /**< (PWM_ISR2) Comparison 3 Match Mask */
#define PWM_ISR2_CMPM3                      PWM_ISR2_CMPM3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPM3_Msk instead */
#define PWM_ISR2_CMPM4_Pos                  12                                             /**< (PWM_ISR2) Comparison 4 Match Position */
#define PWM_ISR2_CMPM4_Msk                  (0x1U << PWM_ISR2_CMPM4_Pos)                   /**< (PWM_ISR2) Comparison 4 Match Mask */
#define PWM_ISR2_CMPM4                      PWM_ISR2_CMPM4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPM4_Msk instead */
#define PWM_ISR2_CMPM5_Pos                  13                                             /**< (PWM_ISR2) Comparison 5 Match Position */
#define PWM_ISR2_CMPM5_Msk                  (0x1U << PWM_ISR2_CMPM5_Pos)                   /**< (PWM_ISR2) Comparison 5 Match Mask */
#define PWM_ISR2_CMPM5                      PWM_ISR2_CMPM5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPM5_Msk instead */
#define PWM_ISR2_CMPM6_Pos                  14                                             /**< (PWM_ISR2) Comparison 6 Match Position */
#define PWM_ISR2_CMPM6_Msk                  (0x1U << PWM_ISR2_CMPM6_Pos)                   /**< (PWM_ISR2) Comparison 6 Match Mask */
#define PWM_ISR2_CMPM6                      PWM_ISR2_CMPM6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPM6_Msk instead */
#define PWM_ISR2_CMPM7_Pos                  15                                             /**< (PWM_ISR2) Comparison 7 Match Position */
#define PWM_ISR2_CMPM7_Msk                  (0x1U << PWM_ISR2_CMPM7_Pos)                   /**< (PWM_ISR2) Comparison 7 Match Mask */
#define PWM_ISR2_CMPM7                      PWM_ISR2_CMPM7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPM7_Msk instead */
#define PWM_ISR2_CMPU0_Pos                  16                                             /**< (PWM_ISR2) Comparison 0 Update Position */
#define PWM_ISR2_CMPU0_Msk                  (0x1U << PWM_ISR2_CMPU0_Pos)                   /**< (PWM_ISR2) Comparison 0 Update Mask */
#define PWM_ISR2_CMPU0                      PWM_ISR2_CMPU0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPU0_Msk instead */
#define PWM_ISR2_CMPU1_Pos                  17                                             /**< (PWM_ISR2) Comparison 1 Update Position */
#define PWM_ISR2_CMPU1_Msk                  (0x1U << PWM_ISR2_CMPU1_Pos)                   /**< (PWM_ISR2) Comparison 1 Update Mask */
#define PWM_ISR2_CMPU1                      PWM_ISR2_CMPU1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPU1_Msk instead */
#define PWM_ISR2_CMPU2_Pos                  18                                             /**< (PWM_ISR2) Comparison 2 Update Position */
#define PWM_ISR2_CMPU2_Msk                  (0x1U << PWM_ISR2_CMPU2_Pos)                   /**< (PWM_ISR2) Comparison 2 Update Mask */
#define PWM_ISR2_CMPU2                      PWM_ISR2_CMPU2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPU2_Msk instead */
#define PWM_ISR2_CMPU3_Pos                  19                                             /**< (PWM_ISR2) Comparison 3 Update Position */
#define PWM_ISR2_CMPU3_Msk                  (0x1U << PWM_ISR2_CMPU3_Pos)                   /**< (PWM_ISR2) Comparison 3 Update Mask */
#define PWM_ISR2_CMPU3                      PWM_ISR2_CMPU3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPU3_Msk instead */
#define PWM_ISR2_CMPU4_Pos                  20                                             /**< (PWM_ISR2) Comparison 4 Update Position */
#define PWM_ISR2_CMPU4_Msk                  (0x1U << PWM_ISR2_CMPU4_Pos)                   /**< (PWM_ISR2) Comparison 4 Update Mask */
#define PWM_ISR2_CMPU4                      PWM_ISR2_CMPU4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPU4_Msk instead */
#define PWM_ISR2_CMPU5_Pos                  21                                             /**< (PWM_ISR2) Comparison 5 Update Position */
#define PWM_ISR2_CMPU5_Msk                  (0x1U << PWM_ISR2_CMPU5_Pos)                   /**< (PWM_ISR2) Comparison 5 Update Mask */
#define PWM_ISR2_CMPU5                      PWM_ISR2_CMPU5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPU5_Msk instead */
#define PWM_ISR2_CMPU6_Pos                  22                                             /**< (PWM_ISR2) Comparison 6 Update Position */
#define PWM_ISR2_CMPU6_Msk                  (0x1U << PWM_ISR2_CMPU6_Pos)                   /**< (PWM_ISR2) Comparison 6 Update Mask */
#define PWM_ISR2_CMPU6                      PWM_ISR2_CMPU6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPU6_Msk instead */
#define PWM_ISR2_CMPU7_Pos                  23                                             /**< (PWM_ISR2) Comparison 7 Update Position */
#define PWM_ISR2_CMPU7_Msk                  (0x1U << PWM_ISR2_CMPU7_Pos)                   /**< (PWM_ISR2) Comparison 7 Update Mask */
#define PWM_ISR2_CMPU7                      PWM_ISR2_CMPU7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ISR2_CMPU7_Msk instead */
#define PWM_ISR2_CMPM_Pos                   8                                              /**< (PWM_ISR2 Position) Comparison x Match */
#define PWM_ISR2_CMPM_Msk                   (0xFFU << PWM_ISR2_CMPM_Pos)                   /**< (PWM_ISR2 Mask) CMPM */
#define PWM_ISR2_CMPM(value)                (PWM_ISR2_CMPM_Msk & ((value) << PWM_ISR2_CMPM_Pos))  
#define PWM_ISR2_CMPU_Pos                   16                                             /**< (PWM_ISR2 Position) Comparison 7 Update */
#define PWM_ISR2_CMPU_Msk                   (0xFFU << PWM_ISR2_CMPU_Pos)                   /**< (PWM_ISR2 Mask) CMPU */
#define PWM_ISR2_CMPU(value)                (PWM_ISR2_CMPU_Msk & ((value) << PWM_ISR2_CMPU_Pos))  
#define PWM_ISR2_MASK                       (0xFFFF09U)                                    /**< \deprecated (PWM_ISR2) Register MASK  (Use PWM_ISR2_Msk instead)  */
#define PWM_ISR2_Msk                        (0xFFFF09U)                                    /**< (PWM_ISR2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_OOV : (PWM Offset: 0x44) (R/W 32) PWM Output Override Value Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OOVH0:1;                   /**< bit:      0  Output Override Value for PWMH output of the channel 0 */
    uint32_t OOVH1:1;                   /**< bit:      1  Output Override Value for PWMH output of the channel 1 */
    uint32_t OOVH2:1;                   /**< bit:      2  Output Override Value for PWMH output of the channel 2 */
    uint32_t OOVH3:1;                   /**< bit:      3  Output Override Value for PWMH output of the channel 3 */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OOVL0:1;                   /**< bit:     16  Output Override Value for PWML output of the channel 0 */
    uint32_t OOVL1:1;                   /**< bit:     17  Output Override Value for PWML output of the channel 1 */
    uint32_t OOVL2:1;                   /**< bit:     18  Output Override Value for PWML output of the channel 2 */
    uint32_t OOVL3:1;                   /**< bit:     19  Output Override Value for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t OOVH:4;                    /**< bit:   0..3  Output Override Value for PWMH output of the channel x */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OOVL:4;                    /**< bit: 16..19  Output Override Value for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_OOV_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_OOV_OFFSET                      (0x44)                                        /**<  (PWM_OOV) PWM Output Override Value Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_OOV_OOVH0_Pos                   0                                              /**< (PWM_OOV) Output Override Value for PWMH output of the channel 0 Position */
#define PWM_OOV_OOVH0_Msk                   (0x1U << PWM_OOV_OOVH0_Pos)                    /**< (PWM_OOV) Output Override Value for PWMH output of the channel 0 Mask */
#define PWM_OOV_OOVH0                       PWM_OOV_OOVH0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OOV_OOVH0_Msk instead */
#define PWM_OOV_OOVH1_Pos                   1                                              /**< (PWM_OOV) Output Override Value for PWMH output of the channel 1 Position */
#define PWM_OOV_OOVH1_Msk                   (0x1U << PWM_OOV_OOVH1_Pos)                    /**< (PWM_OOV) Output Override Value for PWMH output of the channel 1 Mask */
#define PWM_OOV_OOVH1                       PWM_OOV_OOVH1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OOV_OOVH1_Msk instead */
#define PWM_OOV_OOVH2_Pos                   2                                              /**< (PWM_OOV) Output Override Value for PWMH output of the channel 2 Position */
#define PWM_OOV_OOVH2_Msk                   (0x1U << PWM_OOV_OOVH2_Pos)                    /**< (PWM_OOV) Output Override Value for PWMH output of the channel 2 Mask */
#define PWM_OOV_OOVH2                       PWM_OOV_OOVH2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OOV_OOVH2_Msk instead */
#define PWM_OOV_OOVH3_Pos                   3                                              /**< (PWM_OOV) Output Override Value for PWMH output of the channel 3 Position */
#define PWM_OOV_OOVH3_Msk                   (0x1U << PWM_OOV_OOVH3_Pos)                    /**< (PWM_OOV) Output Override Value for PWMH output of the channel 3 Mask */
#define PWM_OOV_OOVH3                       PWM_OOV_OOVH3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OOV_OOVH3_Msk instead */
#define PWM_OOV_OOVL0_Pos                   16                                             /**< (PWM_OOV) Output Override Value for PWML output of the channel 0 Position */
#define PWM_OOV_OOVL0_Msk                   (0x1U << PWM_OOV_OOVL0_Pos)                    /**< (PWM_OOV) Output Override Value for PWML output of the channel 0 Mask */
#define PWM_OOV_OOVL0                       PWM_OOV_OOVL0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OOV_OOVL0_Msk instead */
#define PWM_OOV_OOVL1_Pos                   17                                             /**< (PWM_OOV) Output Override Value for PWML output of the channel 1 Position */
#define PWM_OOV_OOVL1_Msk                   (0x1U << PWM_OOV_OOVL1_Pos)                    /**< (PWM_OOV) Output Override Value for PWML output of the channel 1 Mask */
#define PWM_OOV_OOVL1                       PWM_OOV_OOVL1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OOV_OOVL1_Msk instead */
#define PWM_OOV_OOVL2_Pos                   18                                             /**< (PWM_OOV) Output Override Value for PWML output of the channel 2 Position */
#define PWM_OOV_OOVL2_Msk                   (0x1U << PWM_OOV_OOVL2_Pos)                    /**< (PWM_OOV) Output Override Value for PWML output of the channel 2 Mask */
#define PWM_OOV_OOVL2                       PWM_OOV_OOVL2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OOV_OOVL2_Msk instead */
#define PWM_OOV_OOVL3_Pos                   19                                             /**< (PWM_OOV) Output Override Value for PWML output of the channel 3 Position */
#define PWM_OOV_OOVL3_Msk                   (0x1U << PWM_OOV_OOVL3_Pos)                    /**< (PWM_OOV) Output Override Value for PWML output of the channel 3 Mask */
#define PWM_OOV_OOVL3                       PWM_OOV_OOVL3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OOV_OOVL3_Msk instead */
#define PWM_OOV_OOVH_Pos                    0                                              /**< (PWM_OOV Position) Output Override Value for PWMH output of the channel x */
#define PWM_OOV_OOVH_Msk                    (0xFU << PWM_OOV_OOVH_Pos)                     /**< (PWM_OOV Mask) OOVH */
#define PWM_OOV_OOVH(value)                 (PWM_OOV_OOVH_Msk & ((value) << PWM_OOV_OOVH_Pos))  
#define PWM_OOV_OOVL_Pos                    16                                             /**< (PWM_OOV Position) Output Override Value for PWML output of the channel 3 */
#define PWM_OOV_OOVL_Msk                    (0xFU << PWM_OOV_OOVL_Pos)                     /**< (PWM_OOV Mask) OOVL */
#define PWM_OOV_OOVL(value)                 (PWM_OOV_OOVL_Msk & ((value) << PWM_OOV_OOVL_Pos))  
#define PWM_OOV_MASK                        (0xF000FU)                                     /**< \deprecated (PWM_OOV) Register MASK  (Use PWM_OOV_Msk instead)  */
#define PWM_OOV_Msk                         (0xF000FU)                                     /**< (PWM_OOV) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_OS : (PWM Offset: 0x48) (R/W 32) PWM Output Selection Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OSH0:1;                    /**< bit:      0  Output Selection for PWMH output of the channel 0 */
    uint32_t OSH1:1;                    /**< bit:      1  Output Selection for PWMH output of the channel 1 */
    uint32_t OSH2:1;                    /**< bit:      2  Output Selection for PWMH output of the channel 2 */
    uint32_t OSH3:1;                    /**< bit:      3  Output Selection for PWMH output of the channel 3 */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSL0:1;                    /**< bit:     16  Output Selection for PWML output of the channel 0 */
    uint32_t OSL1:1;                    /**< bit:     17  Output Selection for PWML output of the channel 1 */
    uint32_t OSL2:1;                    /**< bit:     18  Output Selection for PWML output of the channel 2 */
    uint32_t OSL3:1;                    /**< bit:     19  Output Selection for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t OSH:4;                     /**< bit:   0..3  Output Selection for PWMH output of the channel x */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSL:4;                     /**< bit: 16..19  Output Selection for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_OS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_OS_OFFSET                       (0x48)                                        /**<  (PWM_OS) PWM Output Selection Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_OS_OSH0_Pos                     0                                              /**< (PWM_OS) Output Selection for PWMH output of the channel 0 Position */
#define PWM_OS_OSH0_Msk                     (0x1U << PWM_OS_OSH0_Pos)                      /**< (PWM_OS) Output Selection for PWMH output of the channel 0 Mask */
#define PWM_OS_OSH0                         PWM_OS_OSH0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OS_OSH0_Msk instead */
#define PWM_OS_OSH1_Pos                     1                                              /**< (PWM_OS) Output Selection for PWMH output of the channel 1 Position */
#define PWM_OS_OSH1_Msk                     (0x1U << PWM_OS_OSH1_Pos)                      /**< (PWM_OS) Output Selection for PWMH output of the channel 1 Mask */
#define PWM_OS_OSH1                         PWM_OS_OSH1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OS_OSH1_Msk instead */
#define PWM_OS_OSH2_Pos                     2                                              /**< (PWM_OS) Output Selection for PWMH output of the channel 2 Position */
#define PWM_OS_OSH2_Msk                     (0x1U << PWM_OS_OSH2_Pos)                      /**< (PWM_OS) Output Selection for PWMH output of the channel 2 Mask */
#define PWM_OS_OSH2                         PWM_OS_OSH2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OS_OSH2_Msk instead */
#define PWM_OS_OSH3_Pos                     3                                              /**< (PWM_OS) Output Selection for PWMH output of the channel 3 Position */
#define PWM_OS_OSH3_Msk                     (0x1U << PWM_OS_OSH3_Pos)                      /**< (PWM_OS) Output Selection for PWMH output of the channel 3 Mask */
#define PWM_OS_OSH3                         PWM_OS_OSH3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OS_OSH3_Msk instead */
#define PWM_OS_OSL0_Pos                     16                                             /**< (PWM_OS) Output Selection for PWML output of the channel 0 Position */
#define PWM_OS_OSL0_Msk                     (0x1U << PWM_OS_OSL0_Pos)                      /**< (PWM_OS) Output Selection for PWML output of the channel 0 Mask */
#define PWM_OS_OSL0                         PWM_OS_OSL0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OS_OSL0_Msk instead */
#define PWM_OS_OSL1_Pos                     17                                             /**< (PWM_OS) Output Selection for PWML output of the channel 1 Position */
#define PWM_OS_OSL1_Msk                     (0x1U << PWM_OS_OSL1_Pos)                      /**< (PWM_OS) Output Selection for PWML output of the channel 1 Mask */
#define PWM_OS_OSL1                         PWM_OS_OSL1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OS_OSL1_Msk instead */
#define PWM_OS_OSL2_Pos                     18                                             /**< (PWM_OS) Output Selection for PWML output of the channel 2 Position */
#define PWM_OS_OSL2_Msk                     (0x1U << PWM_OS_OSL2_Pos)                      /**< (PWM_OS) Output Selection for PWML output of the channel 2 Mask */
#define PWM_OS_OSL2                         PWM_OS_OSL2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OS_OSL2_Msk instead */
#define PWM_OS_OSL3_Pos                     19                                             /**< (PWM_OS) Output Selection for PWML output of the channel 3 Position */
#define PWM_OS_OSL3_Msk                     (0x1U << PWM_OS_OSL3_Pos)                      /**< (PWM_OS) Output Selection for PWML output of the channel 3 Mask */
#define PWM_OS_OSL3                         PWM_OS_OSL3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OS_OSL3_Msk instead */
#define PWM_OS_OSH_Pos                      0                                              /**< (PWM_OS Position) Output Selection for PWMH output of the channel x */
#define PWM_OS_OSH_Msk                      (0xFU << PWM_OS_OSH_Pos)                       /**< (PWM_OS Mask) OSH */
#define PWM_OS_OSH(value)                   (PWM_OS_OSH_Msk & ((value) << PWM_OS_OSH_Pos))  
#define PWM_OS_OSL_Pos                      16                                             /**< (PWM_OS Position) Output Selection for PWML output of the channel 3 */
#define PWM_OS_OSL_Msk                      (0xFU << PWM_OS_OSL_Pos)                       /**< (PWM_OS Mask) OSL */
#define PWM_OS_OSL(value)                   (PWM_OS_OSL_Msk & ((value) << PWM_OS_OSL_Pos))  
#define PWM_OS_MASK                         (0xF000FU)                                     /**< \deprecated (PWM_OS) Register MASK  (Use PWM_OS_Msk instead)  */
#define PWM_OS_Msk                          (0xF000FU)                                     /**< (PWM_OS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_OSS : (PWM Offset: 0x4c) (/W 32) PWM Output Selection Set Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OSSH0:1;                   /**< bit:      0  Output Selection Set for PWMH output of the channel 0 */
    uint32_t OSSH1:1;                   /**< bit:      1  Output Selection Set for PWMH output of the channel 1 */
    uint32_t OSSH2:1;                   /**< bit:      2  Output Selection Set for PWMH output of the channel 2 */
    uint32_t OSSH3:1;                   /**< bit:      3  Output Selection Set for PWMH output of the channel 3 */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSSL0:1;                   /**< bit:     16  Output Selection Set for PWML output of the channel 0 */
    uint32_t OSSL1:1;                   /**< bit:     17  Output Selection Set for PWML output of the channel 1 */
    uint32_t OSSL2:1;                   /**< bit:     18  Output Selection Set for PWML output of the channel 2 */
    uint32_t OSSL3:1;                   /**< bit:     19  Output Selection Set for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t OSSH:4;                    /**< bit:   0..3  Output Selection Set for PWMH output of the channel x */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSSL:4;                    /**< bit: 16..19  Output Selection Set for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_OSS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_OSS_OFFSET                      (0x4C)                                        /**<  (PWM_OSS) PWM Output Selection Set Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_OSS_OSSH0_Pos                   0                                              /**< (PWM_OSS) Output Selection Set for PWMH output of the channel 0 Position */
#define PWM_OSS_OSSH0_Msk                   (0x1U << PWM_OSS_OSSH0_Pos)                    /**< (PWM_OSS) Output Selection Set for PWMH output of the channel 0 Mask */
#define PWM_OSS_OSSH0                       PWM_OSS_OSSH0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSS_OSSH0_Msk instead */
#define PWM_OSS_OSSH1_Pos                   1                                              /**< (PWM_OSS) Output Selection Set for PWMH output of the channel 1 Position */
#define PWM_OSS_OSSH1_Msk                   (0x1U << PWM_OSS_OSSH1_Pos)                    /**< (PWM_OSS) Output Selection Set for PWMH output of the channel 1 Mask */
#define PWM_OSS_OSSH1                       PWM_OSS_OSSH1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSS_OSSH1_Msk instead */
#define PWM_OSS_OSSH2_Pos                   2                                              /**< (PWM_OSS) Output Selection Set for PWMH output of the channel 2 Position */
#define PWM_OSS_OSSH2_Msk                   (0x1U << PWM_OSS_OSSH2_Pos)                    /**< (PWM_OSS) Output Selection Set for PWMH output of the channel 2 Mask */
#define PWM_OSS_OSSH2                       PWM_OSS_OSSH2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSS_OSSH2_Msk instead */
#define PWM_OSS_OSSH3_Pos                   3                                              /**< (PWM_OSS) Output Selection Set for PWMH output of the channel 3 Position */
#define PWM_OSS_OSSH3_Msk                   (0x1U << PWM_OSS_OSSH3_Pos)                    /**< (PWM_OSS) Output Selection Set for PWMH output of the channel 3 Mask */
#define PWM_OSS_OSSH3                       PWM_OSS_OSSH3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSS_OSSH3_Msk instead */
#define PWM_OSS_OSSL0_Pos                   16                                             /**< (PWM_OSS) Output Selection Set for PWML output of the channel 0 Position */
#define PWM_OSS_OSSL0_Msk                   (0x1U << PWM_OSS_OSSL0_Pos)                    /**< (PWM_OSS) Output Selection Set for PWML output of the channel 0 Mask */
#define PWM_OSS_OSSL0                       PWM_OSS_OSSL0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSS_OSSL0_Msk instead */
#define PWM_OSS_OSSL1_Pos                   17                                             /**< (PWM_OSS) Output Selection Set for PWML output of the channel 1 Position */
#define PWM_OSS_OSSL1_Msk                   (0x1U << PWM_OSS_OSSL1_Pos)                    /**< (PWM_OSS) Output Selection Set for PWML output of the channel 1 Mask */
#define PWM_OSS_OSSL1                       PWM_OSS_OSSL1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSS_OSSL1_Msk instead */
#define PWM_OSS_OSSL2_Pos                   18                                             /**< (PWM_OSS) Output Selection Set for PWML output of the channel 2 Position */
#define PWM_OSS_OSSL2_Msk                   (0x1U << PWM_OSS_OSSL2_Pos)                    /**< (PWM_OSS) Output Selection Set for PWML output of the channel 2 Mask */
#define PWM_OSS_OSSL2                       PWM_OSS_OSSL2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSS_OSSL2_Msk instead */
#define PWM_OSS_OSSL3_Pos                   19                                             /**< (PWM_OSS) Output Selection Set for PWML output of the channel 3 Position */
#define PWM_OSS_OSSL3_Msk                   (0x1U << PWM_OSS_OSSL3_Pos)                    /**< (PWM_OSS) Output Selection Set for PWML output of the channel 3 Mask */
#define PWM_OSS_OSSL3                       PWM_OSS_OSSL3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSS_OSSL3_Msk instead */
#define PWM_OSS_OSSH_Pos                    0                                              /**< (PWM_OSS Position) Output Selection Set for PWMH output of the channel x */
#define PWM_OSS_OSSH_Msk                    (0xFU << PWM_OSS_OSSH_Pos)                     /**< (PWM_OSS Mask) OSSH */
#define PWM_OSS_OSSH(value)                 (PWM_OSS_OSSH_Msk & ((value) << PWM_OSS_OSSH_Pos))  
#define PWM_OSS_OSSL_Pos                    16                                             /**< (PWM_OSS Position) Output Selection Set for PWML output of the channel 3 */
#define PWM_OSS_OSSL_Msk                    (0xFU << PWM_OSS_OSSL_Pos)                     /**< (PWM_OSS Mask) OSSL */
#define PWM_OSS_OSSL(value)                 (PWM_OSS_OSSL_Msk & ((value) << PWM_OSS_OSSL_Pos))  
#define PWM_OSS_MASK                        (0xF000FU)                                     /**< \deprecated (PWM_OSS) Register MASK  (Use PWM_OSS_Msk instead)  */
#define PWM_OSS_Msk                         (0xF000FU)                                     /**< (PWM_OSS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_OSC : (PWM Offset: 0x50) (/W 32) PWM Output Selection Clear Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OSCH0:1;                   /**< bit:      0  Output Selection Clear for PWMH output of the channel 0 */
    uint32_t OSCH1:1;                   /**< bit:      1  Output Selection Clear for PWMH output of the channel 1 */
    uint32_t OSCH2:1;                   /**< bit:      2  Output Selection Clear for PWMH output of the channel 2 */
    uint32_t OSCH3:1;                   /**< bit:      3  Output Selection Clear for PWMH output of the channel 3 */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSCL0:1;                   /**< bit:     16  Output Selection Clear for PWML output of the channel 0 */
    uint32_t OSCL1:1;                   /**< bit:     17  Output Selection Clear for PWML output of the channel 1 */
    uint32_t OSCL2:1;                   /**< bit:     18  Output Selection Clear for PWML output of the channel 2 */
    uint32_t OSCL3:1;                   /**< bit:     19  Output Selection Clear for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t OSCH:4;                    /**< bit:   0..3  Output Selection Clear for PWMH output of the channel x */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSCL:4;                    /**< bit: 16..19  Output Selection Clear for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_OSC_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_OSC_OFFSET                      (0x50)                                        /**<  (PWM_OSC) PWM Output Selection Clear Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_OSC_OSCH0_Pos                   0                                              /**< (PWM_OSC) Output Selection Clear for PWMH output of the channel 0 Position */
#define PWM_OSC_OSCH0_Msk                   (0x1U << PWM_OSC_OSCH0_Pos)                    /**< (PWM_OSC) Output Selection Clear for PWMH output of the channel 0 Mask */
#define PWM_OSC_OSCH0                       PWM_OSC_OSCH0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSC_OSCH0_Msk instead */
#define PWM_OSC_OSCH1_Pos                   1                                              /**< (PWM_OSC) Output Selection Clear for PWMH output of the channel 1 Position */
#define PWM_OSC_OSCH1_Msk                   (0x1U << PWM_OSC_OSCH1_Pos)                    /**< (PWM_OSC) Output Selection Clear for PWMH output of the channel 1 Mask */
#define PWM_OSC_OSCH1                       PWM_OSC_OSCH1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSC_OSCH1_Msk instead */
#define PWM_OSC_OSCH2_Pos                   2                                              /**< (PWM_OSC) Output Selection Clear for PWMH output of the channel 2 Position */
#define PWM_OSC_OSCH2_Msk                   (0x1U << PWM_OSC_OSCH2_Pos)                    /**< (PWM_OSC) Output Selection Clear for PWMH output of the channel 2 Mask */
#define PWM_OSC_OSCH2                       PWM_OSC_OSCH2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSC_OSCH2_Msk instead */
#define PWM_OSC_OSCH3_Pos                   3                                              /**< (PWM_OSC) Output Selection Clear for PWMH output of the channel 3 Position */
#define PWM_OSC_OSCH3_Msk                   (0x1U << PWM_OSC_OSCH3_Pos)                    /**< (PWM_OSC) Output Selection Clear for PWMH output of the channel 3 Mask */
#define PWM_OSC_OSCH3                       PWM_OSC_OSCH3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSC_OSCH3_Msk instead */
#define PWM_OSC_OSCL0_Pos                   16                                             /**< (PWM_OSC) Output Selection Clear for PWML output of the channel 0 Position */
#define PWM_OSC_OSCL0_Msk                   (0x1U << PWM_OSC_OSCL0_Pos)                    /**< (PWM_OSC) Output Selection Clear for PWML output of the channel 0 Mask */
#define PWM_OSC_OSCL0                       PWM_OSC_OSCL0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSC_OSCL0_Msk instead */
#define PWM_OSC_OSCL1_Pos                   17                                             /**< (PWM_OSC) Output Selection Clear for PWML output of the channel 1 Position */
#define PWM_OSC_OSCL1_Msk                   (0x1U << PWM_OSC_OSCL1_Pos)                    /**< (PWM_OSC) Output Selection Clear for PWML output of the channel 1 Mask */
#define PWM_OSC_OSCL1                       PWM_OSC_OSCL1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSC_OSCL1_Msk instead */
#define PWM_OSC_OSCL2_Pos                   18                                             /**< (PWM_OSC) Output Selection Clear for PWML output of the channel 2 Position */
#define PWM_OSC_OSCL2_Msk                   (0x1U << PWM_OSC_OSCL2_Pos)                    /**< (PWM_OSC) Output Selection Clear for PWML output of the channel 2 Mask */
#define PWM_OSC_OSCL2                       PWM_OSC_OSCL2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSC_OSCL2_Msk instead */
#define PWM_OSC_OSCL3_Pos                   19                                             /**< (PWM_OSC) Output Selection Clear for PWML output of the channel 3 Position */
#define PWM_OSC_OSCL3_Msk                   (0x1U << PWM_OSC_OSCL3_Pos)                    /**< (PWM_OSC) Output Selection Clear for PWML output of the channel 3 Mask */
#define PWM_OSC_OSCL3                       PWM_OSC_OSCL3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSC_OSCL3_Msk instead */
#define PWM_OSC_OSCH_Pos                    0                                              /**< (PWM_OSC Position) Output Selection Clear for PWMH output of the channel x */
#define PWM_OSC_OSCH_Msk                    (0xFU << PWM_OSC_OSCH_Pos)                     /**< (PWM_OSC Mask) OSCH */
#define PWM_OSC_OSCH(value)                 (PWM_OSC_OSCH_Msk & ((value) << PWM_OSC_OSCH_Pos))  
#define PWM_OSC_OSCL_Pos                    16                                             /**< (PWM_OSC Position) Output Selection Clear for PWML output of the channel 3 */
#define PWM_OSC_OSCL_Msk                    (0xFU << PWM_OSC_OSCL_Pos)                     /**< (PWM_OSC Mask) OSCL */
#define PWM_OSC_OSCL(value)                 (PWM_OSC_OSCL_Msk & ((value) << PWM_OSC_OSCL_Pos))  
#define PWM_OSC_MASK                        (0xF000FU)                                     /**< \deprecated (PWM_OSC) Register MASK  (Use PWM_OSC_Msk instead)  */
#define PWM_OSC_Msk                         (0xF000FU)                                     /**< (PWM_OSC) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_OSSUPD : (PWM Offset: 0x54) (/W 32) PWM Output Selection Set Update Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OSSUPH0:1;                 /**< bit:      0  Output Selection Set for PWMH output of the channel 0 */
    uint32_t OSSUPH1:1;                 /**< bit:      1  Output Selection Set for PWMH output of the channel 1 */
    uint32_t OSSUPH2:1;                 /**< bit:      2  Output Selection Set for PWMH output of the channel 2 */
    uint32_t OSSUPH3:1;                 /**< bit:      3  Output Selection Set for PWMH output of the channel 3 */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSSUPL0:1;                 /**< bit:     16  Output Selection Set for PWML output of the channel 0 */
    uint32_t OSSUPL1:1;                 /**< bit:     17  Output Selection Set for PWML output of the channel 1 */
    uint32_t OSSUPL2:1;                 /**< bit:     18  Output Selection Set for PWML output of the channel 2 */
    uint32_t OSSUPL3:1;                 /**< bit:     19  Output Selection Set for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t OSSUPH:4;                  /**< bit:   0..3  Output Selection Set for PWMH output of the channel x */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSSUPL:4;                  /**< bit: 16..19  Output Selection Set for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_OSSUPD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_OSSUPD_OFFSET                   (0x54)                                        /**<  (PWM_OSSUPD) PWM Output Selection Set Update Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_OSSUPD_OSSUPH0_Pos              0                                              /**< (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 0 Position */
#define PWM_OSSUPD_OSSUPH0_Msk              (0x1U << PWM_OSSUPD_OSSUPH0_Pos)               /**< (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 0 Mask */
#define PWM_OSSUPD_OSSUPH0                  PWM_OSSUPD_OSSUPH0_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSSUPD_OSSUPH0_Msk instead */
#define PWM_OSSUPD_OSSUPH1_Pos              1                                              /**< (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 1 Position */
#define PWM_OSSUPD_OSSUPH1_Msk              (0x1U << PWM_OSSUPD_OSSUPH1_Pos)               /**< (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 1 Mask */
#define PWM_OSSUPD_OSSUPH1                  PWM_OSSUPD_OSSUPH1_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSSUPD_OSSUPH1_Msk instead */
#define PWM_OSSUPD_OSSUPH2_Pos              2                                              /**< (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 2 Position */
#define PWM_OSSUPD_OSSUPH2_Msk              (0x1U << PWM_OSSUPD_OSSUPH2_Pos)               /**< (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 2 Mask */
#define PWM_OSSUPD_OSSUPH2                  PWM_OSSUPD_OSSUPH2_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSSUPD_OSSUPH2_Msk instead */
#define PWM_OSSUPD_OSSUPH3_Pos              3                                              /**< (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 3 Position */
#define PWM_OSSUPD_OSSUPH3_Msk              (0x1U << PWM_OSSUPD_OSSUPH3_Pos)               /**< (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 3 Mask */
#define PWM_OSSUPD_OSSUPH3                  PWM_OSSUPD_OSSUPH3_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSSUPD_OSSUPH3_Msk instead */
#define PWM_OSSUPD_OSSUPL0_Pos              16                                             /**< (PWM_OSSUPD) Output Selection Set for PWML output of the channel 0 Position */
#define PWM_OSSUPD_OSSUPL0_Msk              (0x1U << PWM_OSSUPD_OSSUPL0_Pos)               /**< (PWM_OSSUPD) Output Selection Set for PWML output of the channel 0 Mask */
#define PWM_OSSUPD_OSSUPL0                  PWM_OSSUPD_OSSUPL0_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSSUPD_OSSUPL0_Msk instead */
#define PWM_OSSUPD_OSSUPL1_Pos              17                                             /**< (PWM_OSSUPD) Output Selection Set for PWML output of the channel 1 Position */
#define PWM_OSSUPD_OSSUPL1_Msk              (0x1U << PWM_OSSUPD_OSSUPL1_Pos)               /**< (PWM_OSSUPD) Output Selection Set for PWML output of the channel 1 Mask */
#define PWM_OSSUPD_OSSUPL1                  PWM_OSSUPD_OSSUPL1_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSSUPD_OSSUPL1_Msk instead */
#define PWM_OSSUPD_OSSUPL2_Pos              18                                             /**< (PWM_OSSUPD) Output Selection Set for PWML output of the channel 2 Position */
#define PWM_OSSUPD_OSSUPL2_Msk              (0x1U << PWM_OSSUPD_OSSUPL2_Pos)               /**< (PWM_OSSUPD) Output Selection Set for PWML output of the channel 2 Mask */
#define PWM_OSSUPD_OSSUPL2                  PWM_OSSUPD_OSSUPL2_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSSUPD_OSSUPL2_Msk instead */
#define PWM_OSSUPD_OSSUPL3_Pos              19                                             /**< (PWM_OSSUPD) Output Selection Set for PWML output of the channel 3 Position */
#define PWM_OSSUPD_OSSUPL3_Msk              (0x1U << PWM_OSSUPD_OSSUPL3_Pos)               /**< (PWM_OSSUPD) Output Selection Set for PWML output of the channel 3 Mask */
#define PWM_OSSUPD_OSSUPL3                  PWM_OSSUPD_OSSUPL3_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSSUPD_OSSUPL3_Msk instead */
#define PWM_OSSUPD_OSSUPH_Pos               0                                              /**< (PWM_OSSUPD Position) Output Selection Set for PWMH output of the channel x */
#define PWM_OSSUPD_OSSUPH_Msk               (0xFU << PWM_OSSUPD_OSSUPH_Pos)                /**< (PWM_OSSUPD Mask) OSSUPH */
#define PWM_OSSUPD_OSSUPH(value)            (PWM_OSSUPD_OSSUPH_Msk & ((value) << PWM_OSSUPD_OSSUPH_Pos))  
#define PWM_OSSUPD_OSSUPL_Pos               16                                             /**< (PWM_OSSUPD Position) Output Selection Set for PWML output of the channel 3 */
#define PWM_OSSUPD_OSSUPL_Msk               (0xFU << PWM_OSSUPD_OSSUPL_Pos)                /**< (PWM_OSSUPD Mask) OSSUPL */
#define PWM_OSSUPD_OSSUPL(value)            (PWM_OSSUPD_OSSUPL_Msk & ((value) << PWM_OSSUPD_OSSUPL_Pos))  
#define PWM_OSSUPD_MASK                     (0xF000FU)                                     /**< \deprecated (PWM_OSSUPD) Register MASK  (Use PWM_OSSUPD_Msk instead)  */
#define PWM_OSSUPD_Msk                      (0xF000FU)                                     /**< (PWM_OSSUPD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_OSCUPD : (PWM Offset: 0x58) (/W 32) PWM Output Selection Clear Update Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t OSCUPH0:1;                 /**< bit:      0  Output Selection Clear for PWMH output of the channel 0 */
    uint32_t OSCUPH1:1;                 /**< bit:      1  Output Selection Clear for PWMH output of the channel 1 */
    uint32_t OSCUPH2:1;                 /**< bit:      2  Output Selection Clear for PWMH output of the channel 2 */
    uint32_t OSCUPH3:1;                 /**< bit:      3  Output Selection Clear for PWMH output of the channel 3 */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSCUPL0:1;                 /**< bit:     16  Output Selection Clear for PWML output of the channel 0 */
    uint32_t OSCUPL1:1;                 /**< bit:     17  Output Selection Clear for PWML output of the channel 1 */
    uint32_t OSCUPL2:1;                 /**< bit:     18  Output Selection Clear for PWML output of the channel 2 */
    uint32_t OSCUPL3:1;                 /**< bit:     19  Output Selection Clear for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t OSCUPH:4;                  /**< bit:   0..3  Output Selection Clear for PWMH output of the channel x */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t OSCUPL:4;                  /**< bit: 16..19  Output Selection Clear for PWML output of the channel 3 */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_OSCUPD_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_OSCUPD_OFFSET                   (0x58)                                        /**<  (PWM_OSCUPD) PWM Output Selection Clear Update Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_OSCUPD_OSCUPH0_Pos              0                                              /**< (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 0 Position */
#define PWM_OSCUPD_OSCUPH0_Msk              (0x1U << PWM_OSCUPD_OSCUPH0_Pos)               /**< (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 0 Mask */
#define PWM_OSCUPD_OSCUPH0                  PWM_OSCUPD_OSCUPH0_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSCUPD_OSCUPH0_Msk instead */
#define PWM_OSCUPD_OSCUPH1_Pos              1                                              /**< (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 1 Position */
#define PWM_OSCUPD_OSCUPH1_Msk              (0x1U << PWM_OSCUPD_OSCUPH1_Pos)               /**< (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 1 Mask */
#define PWM_OSCUPD_OSCUPH1                  PWM_OSCUPD_OSCUPH1_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSCUPD_OSCUPH1_Msk instead */
#define PWM_OSCUPD_OSCUPH2_Pos              2                                              /**< (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 2 Position */
#define PWM_OSCUPD_OSCUPH2_Msk              (0x1U << PWM_OSCUPD_OSCUPH2_Pos)               /**< (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 2 Mask */
#define PWM_OSCUPD_OSCUPH2                  PWM_OSCUPD_OSCUPH2_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSCUPD_OSCUPH2_Msk instead */
#define PWM_OSCUPD_OSCUPH3_Pos              3                                              /**< (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 3 Position */
#define PWM_OSCUPD_OSCUPH3_Msk              (0x1U << PWM_OSCUPD_OSCUPH3_Pos)               /**< (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 3 Mask */
#define PWM_OSCUPD_OSCUPH3                  PWM_OSCUPD_OSCUPH3_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSCUPD_OSCUPH3_Msk instead */
#define PWM_OSCUPD_OSCUPL0_Pos              16                                             /**< (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 0 Position */
#define PWM_OSCUPD_OSCUPL0_Msk              (0x1U << PWM_OSCUPD_OSCUPL0_Pos)               /**< (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 0 Mask */
#define PWM_OSCUPD_OSCUPL0                  PWM_OSCUPD_OSCUPL0_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSCUPD_OSCUPL0_Msk instead */
#define PWM_OSCUPD_OSCUPL1_Pos              17                                             /**< (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 1 Position */
#define PWM_OSCUPD_OSCUPL1_Msk              (0x1U << PWM_OSCUPD_OSCUPL1_Pos)               /**< (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 1 Mask */
#define PWM_OSCUPD_OSCUPL1                  PWM_OSCUPD_OSCUPL1_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSCUPD_OSCUPL1_Msk instead */
#define PWM_OSCUPD_OSCUPL2_Pos              18                                             /**< (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 2 Position */
#define PWM_OSCUPD_OSCUPL2_Msk              (0x1U << PWM_OSCUPD_OSCUPL2_Pos)               /**< (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 2 Mask */
#define PWM_OSCUPD_OSCUPL2                  PWM_OSCUPD_OSCUPL2_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSCUPD_OSCUPL2_Msk instead */
#define PWM_OSCUPD_OSCUPL3_Pos              19                                             /**< (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 3 Position */
#define PWM_OSCUPD_OSCUPL3_Msk              (0x1U << PWM_OSCUPD_OSCUPL3_Pos)               /**< (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 3 Mask */
#define PWM_OSCUPD_OSCUPL3                  PWM_OSCUPD_OSCUPL3_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_OSCUPD_OSCUPL3_Msk instead */
#define PWM_OSCUPD_OSCUPH_Pos               0                                              /**< (PWM_OSCUPD Position) Output Selection Clear for PWMH output of the channel x */
#define PWM_OSCUPD_OSCUPH_Msk               (0xFU << PWM_OSCUPD_OSCUPH_Pos)                /**< (PWM_OSCUPD Mask) OSCUPH */
#define PWM_OSCUPD_OSCUPH(value)            (PWM_OSCUPD_OSCUPH_Msk & ((value) << PWM_OSCUPD_OSCUPH_Pos))  
#define PWM_OSCUPD_OSCUPL_Pos               16                                             /**< (PWM_OSCUPD Position) Output Selection Clear for PWML output of the channel 3 */
#define PWM_OSCUPD_OSCUPL_Msk               (0xFU << PWM_OSCUPD_OSCUPL_Pos)                /**< (PWM_OSCUPD Mask) OSCUPL */
#define PWM_OSCUPD_OSCUPL(value)            (PWM_OSCUPD_OSCUPL_Msk & ((value) << PWM_OSCUPD_OSCUPL_Pos))  
#define PWM_OSCUPD_MASK                     (0xF000FU)                                     /**< \deprecated (PWM_OSCUPD) Register MASK  (Use PWM_OSCUPD_Msk instead)  */
#define PWM_OSCUPD_Msk                      (0xF000FU)                                     /**< (PWM_OSCUPD) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_FMR : (PWM Offset: 0x5c) (R/W 32) PWM Fault Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FPOL:8;                    /**< bit:   0..7  Fault Polarity                           */
    uint32_t FMOD:8;                    /**< bit:  8..15  Fault Activation Mode                    */
    uint32_t FFIL:8;                    /**< bit: 16..23  Fault Filtering                          */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_FMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_FMR_OFFSET                      (0x5C)                                        /**<  (PWM_FMR) PWM Fault Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_FMR_FPOL_Pos                    0                                              /**< (PWM_FMR) Fault Polarity Position */
#define PWM_FMR_FPOL_Msk                    (0xFFU << PWM_FMR_FPOL_Pos)                    /**< (PWM_FMR) Fault Polarity Mask */
#define PWM_FMR_FPOL(value)                 (PWM_FMR_FPOL_Msk & ((value) << PWM_FMR_FPOL_Pos))
#define PWM_FMR_FMOD_Pos                    8                                              /**< (PWM_FMR) Fault Activation Mode Position */
#define PWM_FMR_FMOD_Msk                    (0xFFU << PWM_FMR_FMOD_Pos)                    /**< (PWM_FMR) Fault Activation Mode Mask */
#define PWM_FMR_FMOD(value)                 (PWM_FMR_FMOD_Msk & ((value) << PWM_FMR_FMOD_Pos))
#define PWM_FMR_FFIL_Pos                    16                                             /**< (PWM_FMR) Fault Filtering Position */
#define PWM_FMR_FFIL_Msk                    (0xFFU << PWM_FMR_FFIL_Pos)                    /**< (PWM_FMR) Fault Filtering Mask */
#define PWM_FMR_FFIL(value)                 (PWM_FMR_FFIL_Msk & ((value) << PWM_FMR_FFIL_Pos))
#define PWM_FMR_MASK                        (0xFFFFFFU)                                    /**< \deprecated (PWM_FMR) Register MASK  (Use PWM_FMR_Msk instead)  */
#define PWM_FMR_Msk                         (0xFFFFFFU)                                    /**< (PWM_FMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_FSR : (PWM Offset: 0x60) (R/ 32) PWM Fault Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FIV:8;                     /**< bit:   0..7  Fault Input Value                        */
    uint32_t FS:8;                      /**< bit:  8..15  Fault Status                             */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_FSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_FSR_OFFSET                      (0x60)                                        /**<  (PWM_FSR) PWM Fault Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_FSR_FIV_Pos                     0                                              /**< (PWM_FSR) Fault Input Value Position */
#define PWM_FSR_FIV_Msk                     (0xFFU << PWM_FSR_FIV_Pos)                     /**< (PWM_FSR) Fault Input Value Mask */
#define PWM_FSR_FIV(value)                  (PWM_FSR_FIV_Msk & ((value) << PWM_FSR_FIV_Pos))
#define PWM_FSR_FS_Pos                      8                                              /**< (PWM_FSR) Fault Status Position */
#define PWM_FSR_FS_Msk                      (0xFFU << PWM_FSR_FS_Pos)                      /**< (PWM_FSR) Fault Status Mask */
#define PWM_FSR_FS(value)                   (PWM_FSR_FS_Msk & ((value) << PWM_FSR_FS_Pos))
#define PWM_FSR_MASK                        (0xFFFFU)                                      /**< \deprecated (PWM_FSR) Register MASK  (Use PWM_FSR_Msk instead)  */
#define PWM_FSR_Msk                         (0xFFFFU)                                      /**< (PWM_FSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_FCR : (PWM Offset: 0x64) (/W 32) PWM Fault Clear Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FCLR:8;                    /**< bit:   0..7  Fault Clear                              */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_FCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_FCR_OFFSET                      (0x64)                                        /**<  (PWM_FCR) PWM Fault Clear Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_FCR_FCLR_Pos                    0                                              /**< (PWM_FCR) Fault Clear Position */
#define PWM_FCR_FCLR_Msk                    (0xFFU << PWM_FCR_FCLR_Pos)                    /**< (PWM_FCR) Fault Clear Mask */
#define PWM_FCR_FCLR(value)                 (PWM_FCR_FCLR_Msk & ((value) << PWM_FCR_FCLR_Pos))
#define PWM_FCR_MASK                        (0xFFU)                                        /**< \deprecated (PWM_FCR) Register MASK  (Use PWM_FCR_Msk instead)  */
#define PWM_FCR_Msk                         (0xFFU)                                        /**< (PWM_FCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_FPV1 : (PWM Offset: 0x68) (R/W 32) PWM Fault Protection Value Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FPVH0:1;                   /**< bit:      0  Fault Protection Value for PWMH output on channel 0 */
    uint32_t FPVH1:1;                   /**< bit:      1  Fault Protection Value for PWMH output on channel 1 */
    uint32_t FPVH2:1;                   /**< bit:      2  Fault Protection Value for PWMH output on channel 2 */
    uint32_t FPVH3:1;                   /**< bit:      3  Fault Protection Value for PWMH output on channel 3 */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FPVL0:1;                   /**< bit:     16  Fault Protection Value for PWML output on channel 0 */
    uint32_t FPVL1:1;                   /**< bit:     17  Fault Protection Value for PWML output on channel 1 */
    uint32_t FPVL2:1;                   /**< bit:     18  Fault Protection Value for PWML output on channel 2 */
    uint32_t FPVL3:1;                   /**< bit:     19  Fault Protection Value for PWML output on channel 3 */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t FPVH:4;                    /**< bit:   0..3  Fault Protection Value for PWMH output on channel x */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FPVL:4;                    /**< bit: 16..19  Fault Protection Value for PWML output on channel 3 */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_FPV1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_FPV1_OFFSET                     (0x68)                                        /**<  (PWM_FPV1) PWM Fault Protection Value Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_FPV1_FPVH0_Pos                  0                                              /**< (PWM_FPV1) Fault Protection Value for PWMH output on channel 0 Position */
#define PWM_FPV1_FPVH0_Msk                  (0x1U << PWM_FPV1_FPVH0_Pos)                   /**< (PWM_FPV1) Fault Protection Value for PWMH output on channel 0 Mask */
#define PWM_FPV1_FPVH0                      PWM_FPV1_FPVH0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV1_FPVH0_Msk instead */
#define PWM_FPV1_FPVH1_Pos                  1                                              /**< (PWM_FPV1) Fault Protection Value for PWMH output on channel 1 Position */
#define PWM_FPV1_FPVH1_Msk                  (0x1U << PWM_FPV1_FPVH1_Pos)                   /**< (PWM_FPV1) Fault Protection Value for PWMH output on channel 1 Mask */
#define PWM_FPV1_FPVH1                      PWM_FPV1_FPVH1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV1_FPVH1_Msk instead */
#define PWM_FPV1_FPVH2_Pos                  2                                              /**< (PWM_FPV1) Fault Protection Value for PWMH output on channel 2 Position */
#define PWM_FPV1_FPVH2_Msk                  (0x1U << PWM_FPV1_FPVH2_Pos)                   /**< (PWM_FPV1) Fault Protection Value for PWMH output on channel 2 Mask */
#define PWM_FPV1_FPVH2                      PWM_FPV1_FPVH2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV1_FPVH2_Msk instead */
#define PWM_FPV1_FPVH3_Pos                  3                                              /**< (PWM_FPV1) Fault Protection Value for PWMH output on channel 3 Position */
#define PWM_FPV1_FPVH3_Msk                  (0x1U << PWM_FPV1_FPVH3_Pos)                   /**< (PWM_FPV1) Fault Protection Value for PWMH output on channel 3 Mask */
#define PWM_FPV1_FPVH3                      PWM_FPV1_FPVH3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV1_FPVH3_Msk instead */
#define PWM_FPV1_FPVL0_Pos                  16                                             /**< (PWM_FPV1) Fault Protection Value for PWML output on channel 0 Position */
#define PWM_FPV1_FPVL0_Msk                  (0x1U << PWM_FPV1_FPVL0_Pos)                   /**< (PWM_FPV1) Fault Protection Value for PWML output on channel 0 Mask */
#define PWM_FPV1_FPVL0                      PWM_FPV1_FPVL0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV1_FPVL0_Msk instead */
#define PWM_FPV1_FPVL1_Pos                  17                                             /**< (PWM_FPV1) Fault Protection Value for PWML output on channel 1 Position */
#define PWM_FPV1_FPVL1_Msk                  (0x1U << PWM_FPV1_FPVL1_Pos)                   /**< (PWM_FPV1) Fault Protection Value for PWML output on channel 1 Mask */
#define PWM_FPV1_FPVL1                      PWM_FPV1_FPVL1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV1_FPVL1_Msk instead */
#define PWM_FPV1_FPVL2_Pos                  18                                             /**< (PWM_FPV1) Fault Protection Value for PWML output on channel 2 Position */
#define PWM_FPV1_FPVL2_Msk                  (0x1U << PWM_FPV1_FPVL2_Pos)                   /**< (PWM_FPV1) Fault Protection Value for PWML output on channel 2 Mask */
#define PWM_FPV1_FPVL2                      PWM_FPV1_FPVL2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV1_FPVL2_Msk instead */
#define PWM_FPV1_FPVL3_Pos                  19                                             /**< (PWM_FPV1) Fault Protection Value for PWML output on channel 3 Position */
#define PWM_FPV1_FPVL3_Msk                  (0x1U << PWM_FPV1_FPVL3_Pos)                   /**< (PWM_FPV1) Fault Protection Value for PWML output on channel 3 Mask */
#define PWM_FPV1_FPVL3                      PWM_FPV1_FPVL3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV1_FPVL3_Msk instead */
#define PWM_FPV1_FPVH_Pos                   0                                              /**< (PWM_FPV1 Position) Fault Protection Value for PWMH output on channel x */
#define PWM_FPV1_FPVH_Msk                   (0xFU << PWM_FPV1_FPVH_Pos)                    /**< (PWM_FPV1 Mask) FPVH */
#define PWM_FPV1_FPVH(value)                (PWM_FPV1_FPVH_Msk & ((value) << PWM_FPV1_FPVH_Pos))  
#define PWM_FPV1_FPVL_Pos                   16                                             /**< (PWM_FPV1 Position) Fault Protection Value for PWML output on channel 3 */
#define PWM_FPV1_FPVL_Msk                   (0xFU << PWM_FPV1_FPVL_Pos)                    /**< (PWM_FPV1 Mask) FPVL */
#define PWM_FPV1_FPVL(value)                (PWM_FPV1_FPVL_Msk & ((value) << PWM_FPV1_FPVL_Pos))  
#define PWM_FPV1_MASK                       (0xF000FU)                                     /**< \deprecated (PWM_FPV1) Register MASK  (Use PWM_FPV1_Msk instead)  */
#define PWM_FPV1_Msk                        (0xF000FU)                                     /**< (PWM_FPV1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_FPE : (PWM Offset: 0x6c) (R/W 32) PWM Fault Protection Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FPE0:8;                    /**< bit:   0..7  Fault Protection Enable for channel 0    */
    uint32_t FPE1:8;                    /**< bit:  8..15  Fault Protection Enable for channel 1    */
    uint32_t FPE2:8;                    /**< bit: 16..23  Fault Protection Enable for channel 2    */
    uint32_t FPE3:8;                    /**< bit: 24..31  Fault Protection Enable for channel 3    */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_FPE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_FPE_OFFSET                      (0x6C)                                        /**<  (PWM_FPE) PWM Fault Protection Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_FPE_FPE0_Pos                    0                                              /**< (PWM_FPE) Fault Protection Enable for channel 0 Position */
#define PWM_FPE_FPE0_Msk                    (0xFFU << PWM_FPE_FPE0_Pos)                    /**< (PWM_FPE) Fault Protection Enable for channel 0 Mask */
#define PWM_FPE_FPE0(value)                 (PWM_FPE_FPE0_Msk & ((value) << PWM_FPE_FPE0_Pos))
#define PWM_FPE_FPE1_Pos                    8                                              /**< (PWM_FPE) Fault Protection Enable for channel 1 Position */
#define PWM_FPE_FPE1_Msk                    (0xFFU << PWM_FPE_FPE1_Pos)                    /**< (PWM_FPE) Fault Protection Enable for channel 1 Mask */
#define PWM_FPE_FPE1(value)                 (PWM_FPE_FPE1_Msk & ((value) << PWM_FPE_FPE1_Pos))
#define PWM_FPE_FPE2_Pos                    16                                             /**< (PWM_FPE) Fault Protection Enable for channel 2 Position */
#define PWM_FPE_FPE2_Msk                    (0xFFU << PWM_FPE_FPE2_Pos)                    /**< (PWM_FPE) Fault Protection Enable for channel 2 Mask */
#define PWM_FPE_FPE2(value)                 (PWM_FPE_FPE2_Msk & ((value) << PWM_FPE_FPE2_Pos))
#define PWM_FPE_FPE3_Pos                    24                                             /**< (PWM_FPE) Fault Protection Enable for channel 3 Position */
#define PWM_FPE_FPE3_Msk                    (0xFFU << PWM_FPE_FPE3_Pos)                    /**< (PWM_FPE) Fault Protection Enable for channel 3 Mask */
#define PWM_FPE_FPE3(value)                 (PWM_FPE_FPE3_Msk & ((value) << PWM_FPE_FPE3_Pos))
#define PWM_FPE_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PWM_FPE) Register MASK  (Use PWM_FPE_Msk instead)  */
#define PWM_FPE_Msk                         (0xFFFFFFFFU)                                  /**< (PWM_FPE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_ELMR : (PWM Offset: 0x7c) (R/W 32) PWM Event Line 0 Mode Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CSEL0:1;                   /**< bit:      0  Comparison 0 Selection                   */
    uint32_t CSEL1:1;                   /**< bit:      1  Comparison 1 Selection                   */
    uint32_t CSEL2:1;                   /**< bit:      2  Comparison 2 Selection                   */
    uint32_t CSEL3:1;                   /**< bit:      3  Comparison 3 Selection                   */
    uint32_t CSEL4:1;                   /**< bit:      4  Comparison 4 Selection                   */
    uint32_t CSEL5:1;                   /**< bit:      5  Comparison 5 Selection                   */
    uint32_t CSEL6:1;                   /**< bit:      6  Comparison 6 Selection                   */
    uint32_t CSEL7:1;                   /**< bit:      7  Comparison 7 Selection                   */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t CSEL:8;                    /**< bit:   0..7  Comparison 7 Selection                   */
    uint32_t :24;                       /**< bit:  8..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_ELMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_ELMR_OFFSET                     (0x7C)                                        /**<  (PWM_ELMR) PWM Event Line 0 Mode Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_ELMR_CSEL0_Pos                  0                                              /**< (PWM_ELMR) Comparison 0 Selection Position */
#define PWM_ELMR_CSEL0_Msk                  (0x1U << PWM_ELMR_CSEL0_Pos)                   /**< (PWM_ELMR) Comparison 0 Selection Mask */
#define PWM_ELMR_CSEL0                      PWM_ELMR_CSEL0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ELMR_CSEL0_Msk instead */
#define PWM_ELMR_CSEL1_Pos                  1                                              /**< (PWM_ELMR) Comparison 1 Selection Position */
#define PWM_ELMR_CSEL1_Msk                  (0x1U << PWM_ELMR_CSEL1_Pos)                   /**< (PWM_ELMR) Comparison 1 Selection Mask */
#define PWM_ELMR_CSEL1                      PWM_ELMR_CSEL1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ELMR_CSEL1_Msk instead */
#define PWM_ELMR_CSEL2_Pos                  2                                              /**< (PWM_ELMR) Comparison 2 Selection Position */
#define PWM_ELMR_CSEL2_Msk                  (0x1U << PWM_ELMR_CSEL2_Pos)                   /**< (PWM_ELMR) Comparison 2 Selection Mask */
#define PWM_ELMR_CSEL2                      PWM_ELMR_CSEL2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ELMR_CSEL2_Msk instead */
#define PWM_ELMR_CSEL3_Pos                  3                                              /**< (PWM_ELMR) Comparison 3 Selection Position */
#define PWM_ELMR_CSEL3_Msk                  (0x1U << PWM_ELMR_CSEL3_Pos)                   /**< (PWM_ELMR) Comparison 3 Selection Mask */
#define PWM_ELMR_CSEL3                      PWM_ELMR_CSEL3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ELMR_CSEL3_Msk instead */
#define PWM_ELMR_CSEL4_Pos                  4                                              /**< (PWM_ELMR) Comparison 4 Selection Position */
#define PWM_ELMR_CSEL4_Msk                  (0x1U << PWM_ELMR_CSEL4_Pos)                   /**< (PWM_ELMR) Comparison 4 Selection Mask */
#define PWM_ELMR_CSEL4                      PWM_ELMR_CSEL4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ELMR_CSEL4_Msk instead */
#define PWM_ELMR_CSEL5_Pos                  5                                              /**< (PWM_ELMR) Comparison 5 Selection Position */
#define PWM_ELMR_CSEL5_Msk                  (0x1U << PWM_ELMR_CSEL5_Pos)                   /**< (PWM_ELMR) Comparison 5 Selection Mask */
#define PWM_ELMR_CSEL5                      PWM_ELMR_CSEL5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ELMR_CSEL5_Msk instead */
#define PWM_ELMR_CSEL6_Pos                  6                                              /**< (PWM_ELMR) Comparison 6 Selection Position */
#define PWM_ELMR_CSEL6_Msk                  (0x1U << PWM_ELMR_CSEL6_Pos)                   /**< (PWM_ELMR) Comparison 6 Selection Mask */
#define PWM_ELMR_CSEL6                      PWM_ELMR_CSEL6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ELMR_CSEL6_Msk instead */
#define PWM_ELMR_CSEL7_Pos                  7                                              /**< (PWM_ELMR) Comparison 7 Selection Position */
#define PWM_ELMR_CSEL7_Msk                  (0x1U << PWM_ELMR_CSEL7_Pos)                   /**< (PWM_ELMR) Comparison 7 Selection Mask */
#define PWM_ELMR_CSEL7                      PWM_ELMR_CSEL7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ELMR_CSEL7_Msk instead */
#define PWM_ELMR_CSEL_Pos                   0                                              /**< (PWM_ELMR Position) Comparison 7 Selection */
#define PWM_ELMR_CSEL_Msk                   (0xFFU << PWM_ELMR_CSEL_Pos)                   /**< (PWM_ELMR Mask) CSEL */
#define PWM_ELMR_CSEL(value)                (PWM_ELMR_CSEL_Msk & ((value) << PWM_ELMR_CSEL_Pos))  
#define PWM_ELMR_MASK                       (0xFFU)                                        /**< \deprecated (PWM_ELMR) Register MASK  (Use PWM_ELMR_Msk instead)  */
#define PWM_ELMR_Msk                        (0xFFU)                                        /**< (PWM_ELMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_SSPR : (PWM Offset: 0xa0) (R/W 32) PWM Spread Spectrum Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SPRD:24;                   /**< bit:  0..23  Spread Spectrum Limit Value              */
    uint32_t SPRDM:1;                   /**< bit:     24  Spread Spectrum Counter Mode             */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_SSPR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_SSPR_OFFSET                     (0xA0)                                        /**<  (PWM_SSPR) PWM Spread Spectrum Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_SSPR_SPRD_Pos                   0                                              /**< (PWM_SSPR) Spread Spectrum Limit Value Position */
#define PWM_SSPR_SPRD_Msk                   (0xFFFFFFU << PWM_SSPR_SPRD_Pos)               /**< (PWM_SSPR) Spread Spectrum Limit Value Mask */
#define PWM_SSPR_SPRD(value)                (PWM_SSPR_SPRD_Msk & ((value) << PWM_SSPR_SPRD_Pos))
#define PWM_SSPR_SPRDM_Pos                  24                                             /**< (PWM_SSPR) Spread Spectrum Counter Mode Position */
#define PWM_SSPR_SPRDM_Msk                  (0x1U << PWM_SSPR_SPRDM_Pos)                   /**< (PWM_SSPR) Spread Spectrum Counter Mode Mask */
#define PWM_SSPR_SPRDM                      PWM_SSPR_SPRDM_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SSPR_SPRDM_Msk instead */
#define PWM_SSPR_MASK                       (0x1FFFFFFU)                                   /**< \deprecated (PWM_SSPR) Register MASK  (Use PWM_SSPR_Msk instead)  */
#define PWM_SSPR_Msk                        (0x1FFFFFFU)                                   /**< (PWM_SSPR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_SSPUP : (PWM Offset: 0xa4) (/W 32) PWM Spread Spectrum Update Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SPRDUP:24;                 /**< bit:  0..23  Spread Spectrum Limit Value Update       */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_SSPUP_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_SSPUP_OFFSET                    (0xA4)                                        /**<  (PWM_SSPUP) PWM Spread Spectrum Update Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_SSPUP_SPRDUP_Pos                0                                              /**< (PWM_SSPUP) Spread Spectrum Limit Value Update Position */
#define PWM_SSPUP_SPRDUP_Msk                (0xFFFFFFU << PWM_SSPUP_SPRDUP_Pos)            /**< (PWM_SSPUP) Spread Spectrum Limit Value Update Mask */
#define PWM_SSPUP_SPRDUP(value)             (PWM_SSPUP_SPRDUP_Msk & ((value) << PWM_SSPUP_SPRDUP_Pos))
#define PWM_SSPUP_MASK                      (0xFFFFFFU)                                    /**< \deprecated (PWM_SSPUP) Register MASK  (Use PWM_SSPUP_Msk instead)  */
#define PWM_SSPUP_Msk                       (0xFFFFFFU)                                    /**< (PWM_SSPUP) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_SMMR : (PWM Offset: 0xb0) (R/W 32) PWM Stepper Motor Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t GCEN0:1;                   /**< bit:      0  Gray Count ENable                        */
    uint32_t GCEN1:1;                   /**< bit:      1  Gray Count ENable                        */
    uint32_t :14;                       /**< bit:  2..15  Reserved */
    uint32_t DOWN0:1;                   /**< bit:     16  DOWN Count                               */
    uint32_t DOWN1:1;                   /**< bit:     17  DOWN Count                               */
    uint32_t :14;                       /**< bit: 18..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t GCEN:2;                    /**< bit:   0..1  Gray Count ENable                        */
    uint32_t :14;                       /**< bit:  2..15  Reserved */
    uint32_t DOWN:2;                    /**< bit: 16..17  DOWN Count                               */
    uint32_t :14;                       /**< bit: 18..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_SMMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_SMMR_OFFSET                     (0xB0)                                        /**<  (PWM_SMMR) PWM Stepper Motor Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_SMMR_GCEN0_Pos                  0                                              /**< (PWM_SMMR) Gray Count ENable Position */
#define PWM_SMMR_GCEN0_Msk                  (0x1U << PWM_SMMR_GCEN0_Pos)                   /**< (PWM_SMMR) Gray Count ENable Mask */
#define PWM_SMMR_GCEN0                      PWM_SMMR_GCEN0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SMMR_GCEN0_Msk instead */
#define PWM_SMMR_GCEN1_Pos                  1                                              /**< (PWM_SMMR) Gray Count ENable Position */
#define PWM_SMMR_GCEN1_Msk                  (0x1U << PWM_SMMR_GCEN1_Pos)                   /**< (PWM_SMMR) Gray Count ENable Mask */
#define PWM_SMMR_GCEN1                      PWM_SMMR_GCEN1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SMMR_GCEN1_Msk instead */
#define PWM_SMMR_DOWN0_Pos                  16                                             /**< (PWM_SMMR) DOWN Count Position */
#define PWM_SMMR_DOWN0_Msk                  (0x1U << PWM_SMMR_DOWN0_Pos)                   /**< (PWM_SMMR) DOWN Count Mask */
#define PWM_SMMR_DOWN0                      PWM_SMMR_DOWN0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SMMR_DOWN0_Msk instead */
#define PWM_SMMR_DOWN1_Pos                  17                                             /**< (PWM_SMMR) DOWN Count Position */
#define PWM_SMMR_DOWN1_Msk                  (0x1U << PWM_SMMR_DOWN1_Pos)                   /**< (PWM_SMMR) DOWN Count Mask */
#define PWM_SMMR_DOWN1                      PWM_SMMR_DOWN1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_SMMR_DOWN1_Msk instead */
#define PWM_SMMR_GCEN_Pos                   0                                              /**< (PWM_SMMR Position) Gray Count ENable */
#define PWM_SMMR_GCEN_Msk                   (0x3U << PWM_SMMR_GCEN_Pos)                    /**< (PWM_SMMR Mask) GCEN */
#define PWM_SMMR_GCEN(value)                (PWM_SMMR_GCEN_Msk & ((value) << PWM_SMMR_GCEN_Pos))  
#define PWM_SMMR_DOWN_Pos                   16                                             /**< (PWM_SMMR Position) DOWN Count */
#define PWM_SMMR_DOWN_Msk                   (0x3U << PWM_SMMR_DOWN_Pos)                    /**< (PWM_SMMR Mask) DOWN */
#define PWM_SMMR_DOWN(value)                (PWM_SMMR_DOWN_Msk & ((value) << PWM_SMMR_DOWN_Pos))  
#define PWM_SMMR_MASK                       (0x30003U)                                     /**< \deprecated (PWM_SMMR) Register MASK  (Use PWM_SMMR_Msk instead)  */
#define PWM_SMMR_Msk                        (0x30003U)                                     /**< (PWM_SMMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_FPV2 : (PWM Offset: 0xc0) (R/W 32) PWM Fault Protection Value 2 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FPZH0:1;                   /**< bit:      0  Fault Protection to Hi-Z for PWMH output on channel 0 */
    uint32_t FPZH1:1;                   /**< bit:      1  Fault Protection to Hi-Z for PWMH output on channel 1 */
    uint32_t FPZH2:1;                   /**< bit:      2  Fault Protection to Hi-Z for PWMH output on channel 2 */
    uint32_t FPZH3:1;                   /**< bit:      3  Fault Protection to Hi-Z for PWMH output on channel 3 */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FPZL0:1;                   /**< bit:     16  Fault Protection to Hi-Z for PWML output on channel 0 */
    uint32_t FPZL1:1;                   /**< bit:     17  Fault Protection to Hi-Z for PWML output on channel 1 */
    uint32_t FPZL2:1;                   /**< bit:     18  Fault Protection to Hi-Z for PWML output on channel 2 */
    uint32_t FPZL3:1;                   /**< bit:     19  Fault Protection to Hi-Z for PWML output on channel 3 */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t FPZH:4;                    /**< bit:   0..3  Fault Protection to Hi-Z for PWMH output on channel x */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t FPZL:4;                    /**< bit: 16..19  Fault Protection to Hi-Z for PWML output on channel 3 */
    uint32_t :12;                       /**< bit: 20..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PWM_FPV2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_FPV2_OFFSET                     (0xC0)                                        /**<  (PWM_FPV2) PWM Fault Protection Value 2 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_FPV2_FPZH0_Pos                  0                                              /**< (PWM_FPV2) Fault Protection to Hi-Z for PWMH output on channel 0 Position */
#define PWM_FPV2_FPZH0_Msk                  (0x1U << PWM_FPV2_FPZH0_Pos)                   /**< (PWM_FPV2) Fault Protection to Hi-Z for PWMH output on channel 0 Mask */
#define PWM_FPV2_FPZH0                      PWM_FPV2_FPZH0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV2_FPZH0_Msk instead */
#define PWM_FPV2_FPZH1_Pos                  1                                              /**< (PWM_FPV2) Fault Protection to Hi-Z for PWMH output on channel 1 Position */
#define PWM_FPV2_FPZH1_Msk                  (0x1U << PWM_FPV2_FPZH1_Pos)                   /**< (PWM_FPV2) Fault Protection to Hi-Z for PWMH output on channel 1 Mask */
#define PWM_FPV2_FPZH1                      PWM_FPV2_FPZH1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV2_FPZH1_Msk instead */
#define PWM_FPV2_FPZH2_Pos                  2                                              /**< (PWM_FPV2) Fault Protection to Hi-Z for PWMH output on channel 2 Position */
#define PWM_FPV2_FPZH2_Msk                  (0x1U << PWM_FPV2_FPZH2_Pos)                   /**< (PWM_FPV2) Fault Protection to Hi-Z for PWMH output on channel 2 Mask */
#define PWM_FPV2_FPZH2                      PWM_FPV2_FPZH2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV2_FPZH2_Msk instead */
#define PWM_FPV2_FPZH3_Pos                  3                                              /**< (PWM_FPV2) Fault Protection to Hi-Z for PWMH output on channel 3 Position */
#define PWM_FPV2_FPZH3_Msk                  (0x1U << PWM_FPV2_FPZH3_Pos)                   /**< (PWM_FPV2) Fault Protection to Hi-Z for PWMH output on channel 3 Mask */
#define PWM_FPV2_FPZH3                      PWM_FPV2_FPZH3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV2_FPZH3_Msk instead */
#define PWM_FPV2_FPZL0_Pos                  16                                             /**< (PWM_FPV2) Fault Protection to Hi-Z for PWML output on channel 0 Position */
#define PWM_FPV2_FPZL0_Msk                  (0x1U << PWM_FPV2_FPZL0_Pos)                   /**< (PWM_FPV2) Fault Protection to Hi-Z for PWML output on channel 0 Mask */
#define PWM_FPV2_FPZL0                      PWM_FPV2_FPZL0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV2_FPZL0_Msk instead */
#define PWM_FPV2_FPZL1_Pos                  17                                             /**< (PWM_FPV2) Fault Protection to Hi-Z for PWML output on channel 1 Position */
#define PWM_FPV2_FPZL1_Msk                  (0x1U << PWM_FPV2_FPZL1_Pos)                   /**< (PWM_FPV2) Fault Protection to Hi-Z for PWML output on channel 1 Mask */
#define PWM_FPV2_FPZL1                      PWM_FPV2_FPZL1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV2_FPZL1_Msk instead */
#define PWM_FPV2_FPZL2_Pos                  18                                             /**< (PWM_FPV2) Fault Protection to Hi-Z for PWML output on channel 2 Position */
#define PWM_FPV2_FPZL2_Msk                  (0x1U << PWM_FPV2_FPZL2_Pos)                   /**< (PWM_FPV2) Fault Protection to Hi-Z for PWML output on channel 2 Mask */
#define PWM_FPV2_FPZL2                      PWM_FPV2_FPZL2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV2_FPZL2_Msk instead */
#define PWM_FPV2_FPZL3_Pos                  19                                             /**< (PWM_FPV2) Fault Protection to Hi-Z for PWML output on channel 3 Position */
#define PWM_FPV2_FPZL3_Msk                  (0x1U << PWM_FPV2_FPZL3_Pos)                   /**< (PWM_FPV2) Fault Protection to Hi-Z for PWML output on channel 3 Mask */
#define PWM_FPV2_FPZL3                      PWM_FPV2_FPZL3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_FPV2_FPZL3_Msk instead */
#define PWM_FPV2_FPZH_Pos                   0                                              /**< (PWM_FPV2 Position) Fault Protection to Hi-Z for PWMH output on channel x */
#define PWM_FPV2_FPZH_Msk                   (0xFU << PWM_FPV2_FPZH_Pos)                    /**< (PWM_FPV2 Mask) FPZH */
#define PWM_FPV2_FPZH(value)                (PWM_FPV2_FPZH_Msk & ((value) << PWM_FPV2_FPZH_Pos))  
#define PWM_FPV2_FPZL_Pos                   16                                             /**< (PWM_FPV2 Position) Fault Protection to Hi-Z for PWML output on channel 3 */
#define PWM_FPV2_FPZL_Msk                   (0xFU << PWM_FPV2_FPZL_Pos)                    /**< (PWM_FPV2 Mask) FPZL */
#define PWM_FPV2_FPZL(value)                (PWM_FPV2_FPZL_Msk & ((value) << PWM_FPV2_FPZL_Pos))  
#define PWM_FPV2_MASK                       (0xF000FU)                                     /**< \deprecated (PWM_FPV2) Register MASK  (Use PWM_FPV2_Msk instead)  */
#define PWM_FPV2_Msk                        (0xF000FU)                                     /**< (PWM_FPV2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_WPCR : (PWM Offset: 0xe4) (/W 32) PWM Write Protection Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPCMD:2;                   /**< bit:   0..1  Write Protection Command                 */
    uint32_t WPRG0:1;                   /**< bit:      2  Write Protection Register Group 0        */
    uint32_t WPRG1:1;                   /**< bit:      3  Write Protection Register Group 1        */
    uint32_t WPRG2:1;                   /**< bit:      4  Write Protection Register Group 2        */
    uint32_t WPRG3:1;                   /**< bit:      5  Write Protection Register Group 3        */
    uint32_t WPRG4:1;                   /**< bit:      6  Write Protection Register Group 4        */
    uint32_t WPRG5:1;                   /**< bit:      7  Write Protection Register Group 5        */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_WPCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_WPCR_OFFSET                     (0xE4)                                        /**<  (PWM_WPCR) PWM Write Protection Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_WPCR_WPCMD_Pos                  0                                              /**< (PWM_WPCR) Write Protection Command Position */
#define PWM_WPCR_WPCMD_Msk                  (0x3U << PWM_WPCR_WPCMD_Pos)                   /**< (PWM_WPCR) Write Protection Command Mask */
#define PWM_WPCR_WPCMD(value)               (PWM_WPCR_WPCMD_Msk & ((value) << PWM_WPCR_WPCMD_Pos))
#define   PWM_WPCR_WPCMD_DISABLE_SW_PROT_Val (0x0U)                                         /**< (PWM_WPCR) Disables the software write protection of the register groups of which the bit WPRGx is at '1'.  */
#define   PWM_WPCR_WPCMD_ENABLE_SW_PROT_Val (0x1U)                                         /**< (PWM_WPCR) Enables the software write protection of the register groups of which the bit WPRGx is at '1'.  */
#define   PWM_WPCR_WPCMD_ENABLE_HW_PROT_Val (0x2U)                                         /**< (PWM_WPCR) Enables the hardware write protection of the register groups of which the bit WPRGx is at '1'. Only a hardware reset of the PWM controller can disable the hardware write protection. Moreover, to meet security requirements, the PIO lines associated with the PWM can not be configured through the PIO interface.  */
#define PWM_WPCR_WPCMD_DISABLE_SW_PROT      (PWM_WPCR_WPCMD_DISABLE_SW_PROT_Val << PWM_WPCR_WPCMD_Pos)  /**< (PWM_WPCR) Disables the software write protection of the register groups of which the bit WPRGx is at '1'. Position  */
#define PWM_WPCR_WPCMD_ENABLE_SW_PROT       (PWM_WPCR_WPCMD_ENABLE_SW_PROT_Val << PWM_WPCR_WPCMD_Pos)  /**< (PWM_WPCR) Enables the software write protection of the register groups of which the bit WPRGx is at '1'. Position  */
#define PWM_WPCR_WPCMD_ENABLE_HW_PROT       (PWM_WPCR_WPCMD_ENABLE_HW_PROT_Val << PWM_WPCR_WPCMD_Pos)  /**< (PWM_WPCR) Enables the hardware write protection of the register groups of which the bit WPRGx is at '1'. Only a hardware reset of the PWM controller can disable the hardware write protection. Moreover, to meet security requirements, the PIO lines associated with the PWM can not be configured through the PIO interface. Position  */
#define PWM_WPCR_WPRG0_Pos                  2                                              /**< (PWM_WPCR) Write Protection Register Group 0 Position */
#define PWM_WPCR_WPRG0_Msk                  (0x1U << PWM_WPCR_WPRG0_Pos)                   /**< (PWM_WPCR) Write Protection Register Group 0 Mask */
#define PWM_WPCR_WPRG0                      PWM_WPCR_WPRG0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPCR_WPRG0_Msk instead */
#define PWM_WPCR_WPRG1_Pos                  3                                              /**< (PWM_WPCR) Write Protection Register Group 1 Position */
#define PWM_WPCR_WPRG1_Msk                  (0x1U << PWM_WPCR_WPRG1_Pos)                   /**< (PWM_WPCR) Write Protection Register Group 1 Mask */
#define PWM_WPCR_WPRG1                      PWM_WPCR_WPRG1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPCR_WPRG1_Msk instead */
#define PWM_WPCR_WPRG2_Pos                  4                                              /**< (PWM_WPCR) Write Protection Register Group 2 Position */
#define PWM_WPCR_WPRG2_Msk                  (0x1U << PWM_WPCR_WPRG2_Pos)                   /**< (PWM_WPCR) Write Protection Register Group 2 Mask */
#define PWM_WPCR_WPRG2                      PWM_WPCR_WPRG2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPCR_WPRG2_Msk instead */
#define PWM_WPCR_WPRG3_Pos                  5                                              /**< (PWM_WPCR) Write Protection Register Group 3 Position */
#define PWM_WPCR_WPRG3_Msk                  (0x1U << PWM_WPCR_WPRG3_Pos)                   /**< (PWM_WPCR) Write Protection Register Group 3 Mask */
#define PWM_WPCR_WPRG3                      PWM_WPCR_WPRG3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPCR_WPRG3_Msk instead */
#define PWM_WPCR_WPRG4_Pos                  6                                              /**< (PWM_WPCR) Write Protection Register Group 4 Position */
#define PWM_WPCR_WPRG4_Msk                  (0x1U << PWM_WPCR_WPRG4_Pos)                   /**< (PWM_WPCR) Write Protection Register Group 4 Mask */
#define PWM_WPCR_WPRG4                      PWM_WPCR_WPRG4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPCR_WPRG4_Msk instead */
#define PWM_WPCR_WPRG5_Pos                  7                                              /**< (PWM_WPCR) Write Protection Register Group 5 Position */
#define PWM_WPCR_WPRG5_Msk                  (0x1U << PWM_WPCR_WPRG5_Pos)                   /**< (PWM_WPCR) Write Protection Register Group 5 Mask */
#define PWM_WPCR_WPRG5                      PWM_WPCR_WPRG5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPCR_WPRG5_Msk instead */
#define PWM_WPCR_WPKEY_Pos                  8                                              /**< (PWM_WPCR) Write Protection Key Position */
#define PWM_WPCR_WPKEY_Msk                  (0xFFFFFFU << PWM_WPCR_WPKEY_Pos)              /**< (PWM_WPCR) Write Protection Key Mask */
#define PWM_WPCR_WPKEY(value)               (PWM_WPCR_WPKEY_Msk & ((value) << PWM_WPCR_WPKEY_Pos))
#define   PWM_WPCR_WPKEY_PASSWD_Val         (0x50574DU)                                    /**< (PWM_WPCR) Writing any other value in this field aborts the write operation of the WPCMD field.Always reads as 0  */
#define PWM_WPCR_WPKEY_PASSWD               (PWM_WPCR_WPKEY_PASSWD_Val << PWM_WPCR_WPKEY_Pos)  /**< (PWM_WPCR) Writing any other value in this field aborts the write operation of the WPCMD field.Always reads as 0 Position  */
#define PWM_WPCR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PWM_WPCR) Register MASK  (Use PWM_WPCR_Msk instead)  */
#define PWM_WPCR_Msk                        (0xFFFFFFFFU)                                  /**< (PWM_WPCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_WPSR : (PWM Offset: 0xe8) (R/ 32) PWM Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPSWS0:1;                  /**< bit:      0  Write Protect SW Status                  */
    uint32_t WPSWS1:1;                  /**< bit:      1  Write Protect SW Status                  */
    uint32_t WPSWS2:1;                  /**< bit:      2  Write Protect SW Status                  */
    uint32_t WPSWS3:1;                  /**< bit:      3  Write Protect SW Status                  */
    uint32_t WPSWS4:1;                  /**< bit:      4  Write Protect SW Status                  */
    uint32_t WPSWS5:1;                  /**< bit:      5  Write Protect SW Status                  */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t WPVS:1;                    /**< bit:      7  Write Protect Violation Status           */
    uint32_t WPHWS0:1;                  /**< bit:      8  Write Protect HW Status                  */
    uint32_t WPHWS1:1;                  /**< bit:      9  Write Protect HW Status                  */
    uint32_t WPHWS2:1;                  /**< bit:     10  Write Protect HW Status                  */
    uint32_t WPHWS3:1;                  /**< bit:     11  Write Protect HW Status                  */
    uint32_t WPHWS4:1;                  /**< bit:     12  Write Protect HW Status                  */
    uint32_t WPHWS5:1;                  /**< bit:     13  Write Protect HW Status                  */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t WPVSRC:16;                 /**< bit: 16..31  Write Protect Violation Source           */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_WPSR_OFFSET                     (0xE8)                                        /**<  (PWM_WPSR) PWM Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_WPSR_WPSWS0_Pos                 0                                              /**< (PWM_WPSR) Write Protect SW Status Position */
#define PWM_WPSR_WPSWS0_Msk                 (0x1U << PWM_WPSR_WPSWS0_Pos)                  /**< (PWM_WPSR) Write Protect SW Status Mask */
#define PWM_WPSR_WPSWS0                     PWM_WPSR_WPSWS0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPSWS0_Msk instead */
#define PWM_WPSR_WPSWS1_Pos                 1                                              /**< (PWM_WPSR) Write Protect SW Status Position */
#define PWM_WPSR_WPSWS1_Msk                 (0x1U << PWM_WPSR_WPSWS1_Pos)                  /**< (PWM_WPSR) Write Protect SW Status Mask */
#define PWM_WPSR_WPSWS1                     PWM_WPSR_WPSWS1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPSWS1_Msk instead */
#define PWM_WPSR_WPSWS2_Pos                 2                                              /**< (PWM_WPSR) Write Protect SW Status Position */
#define PWM_WPSR_WPSWS2_Msk                 (0x1U << PWM_WPSR_WPSWS2_Pos)                  /**< (PWM_WPSR) Write Protect SW Status Mask */
#define PWM_WPSR_WPSWS2                     PWM_WPSR_WPSWS2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPSWS2_Msk instead */
#define PWM_WPSR_WPSWS3_Pos                 3                                              /**< (PWM_WPSR) Write Protect SW Status Position */
#define PWM_WPSR_WPSWS3_Msk                 (0x1U << PWM_WPSR_WPSWS3_Pos)                  /**< (PWM_WPSR) Write Protect SW Status Mask */
#define PWM_WPSR_WPSWS3                     PWM_WPSR_WPSWS3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPSWS3_Msk instead */
#define PWM_WPSR_WPSWS4_Pos                 4                                              /**< (PWM_WPSR) Write Protect SW Status Position */
#define PWM_WPSR_WPSWS4_Msk                 (0x1U << PWM_WPSR_WPSWS4_Pos)                  /**< (PWM_WPSR) Write Protect SW Status Mask */
#define PWM_WPSR_WPSWS4                     PWM_WPSR_WPSWS4_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPSWS4_Msk instead */
#define PWM_WPSR_WPSWS5_Pos                 5                                              /**< (PWM_WPSR) Write Protect SW Status Position */
#define PWM_WPSR_WPSWS5_Msk                 (0x1U << PWM_WPSR_WPSWS5_Pos)                  /**< (PWM_WPSR) Write Protect SW Status Mask */
#define PWM_WPSR_WPSWS5                     PWM_WPSR_WPSWS5_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPSWS5_Msk instead */
#define PWM_WPSR_WPVS_Pos                   7                                              /**< (PWM_WPSR) Write Protect Violation Status Position */
#define PWM_WPSR_WPVS_Msk                   (0x1U << PWM_WPSR_WPVS_Pos)                    /**< (PWM_WPSR) Write Protect Violation Status Mask */
#define PWM_WPSR_WPVS                       PWM_WPSR_WPVS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPVS_Msk instead */
#define PWM_WPSR_WPHWS0_Pos                 8                                              /**< (PWM_WPSR) Write Protect HW Status Position */
#define PWM_WPSR_WPHWS0_Msk                 (0x1U << PWM_WPSR_WPHWS0_Pos)                  /**< (PWM_WPSR) Write Protect HW Status Mask */
#define PWM_WPSR_WPHWS0                     PWM_WPSR_WPHWS0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPHWS0_Msk instead */
#define PWM_WPSR_WPHWS1_Pos                 9                                              /**< (PWM_WPSR) Write Protect HW Status Position */
#define PWM_WPSR_WPHWS1_Msk                 (0x1U << PWM_WPSR_WPHWS1_Pos)                  /**< (PWM_WPSR) Write Protect HW Status Mask */
#define PWM_WPSR_WPHWS1                     PWM_WPSR_WPHWS1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPHWS1_Msk instead */
#define PWM_WPSR_WPHWS2_Pos                 10                                             /**< (PWM_WPSR) Write Protect HW Status Position */
#define PWM_WPSR_WPHWS2_Msk                 (0x1U << PWM_WPSR_WPHWS2_Pos)                  /**< (PWM_WPSR) Write Protect HW Status Mask */
#define PWM_WPSR_WPHWS2                     PWM_WPSR_WPHWS2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPHWS2_Msk instead */
#define PWM_WPSR_WPHWS3_Pos                 11                                             /**< (PWM_WPSR) Write Protect HW Status Position */
#define PWM_WPSR_WPHWS3_Msk                 (0x1U << PWM_WPSR_WPHWS3_Pos)                  /**< (PWM_WPSR) Write Protect HW Status Mask */
#define PWM_WPSR_WPHWS3                     PWM_WPSR_WPHWS3_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPHWS3_Msk instead */
#define PWM_WPSR_WPHWS4_Pos                 12                                             /**< (PWM_WPSR) Write Protect HW Status Position */
#define PWM_WPSR_WPHWS4_Msk                 (0x1U << PWM_WPSR_WPHWS4_Pos)                  /**< (PWM_WPSR) Write Protect HW Status Mask */
#define PWM_WPSR_WPHWS4                     PWM_WPSR_WPHWS4_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPHWS4_Msk instead */
#define PWM_WPSR_WPHWS5_Pos                 13                                             /**< (PWM_WPSR) Write Protect HW Status Position */
#define PWM_WPSR_WPHWS5_Msk                 (0x1U << PWM_WPSR_WPHWS5_Pos)                  /**< (PWM_WPSR) Write Protect HW Status Mask */
#define PWM_WPSR_WPHWS5                     PWM_WPSR_WPHWS5_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_WPSR_WPHWS5_Msk instead */
#define PWM_WPSR_WPVSRC_Pos                 16                                             /**< (PWM_WPSR) Write Protect Violation Source Position */
#define PWM_WPSR_WPVSRC_Msk                 (0xFFFFU << PWM_WPSR_WPVSRC_Pos)               /**< (PWM_WPSR) Write Protect Violation Source Mask */
#define PWM_WPSR_WPVSRC(value)              (PWM_WPSR_WPVSRC_Msk & ((value) << PWM_WPSR_WPVSRC_Pos))
#define PWM_WPSR_MASK                       (0xFFFF3FBFU)                                  /**< \deprecated (PWM_WPSR) Register MASK  (Use PWM_WPSR_Msk instead)  */
#define PWM_WPSR_Msk                        (0xFFFF3FBFU)                                  /**< (PWM_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CMUPD0 : (PWM Offset: 0x400) (/W 32) PWM Channel Mode Update Register (ch_num = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :9;                        /**< bit:   0..8  Reserved */
    uint32_t CPOLUP:1;                  /**< bit:      9  Channel Polarity Update                  */
    uint32_t :3;                        /**< bit: 10..12  Reserved */
    uint32_t CPOLINVUP:1;               /**< bit:     13  Channel Polarity Inversion Update        */
    uint32_t :18;                       /**< bit: 14..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMUPD0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMUPD0_OFFSET                   (0x400)                                       /**<  (PWM_CMUPD0) PWM Channel Mode Update Register (ch_num = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMUPD0_CPOLUP_Pos               9                                              /**< (PWM_CMUPD0) Channel Polarity Update Position */
#define PWM_CMUPD0_CPOLUP_Msk               (0x1U << PWM_CMUPD0_CPOLUP_Pos)                /**< (PWM_CMUPD0) Channel Polarity Update Mask */
#define PWM_CMUPD0_CPOLUP                   PWM_CMUPD0_CPOLUP_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMUPD0_CPOLUP_Msk instead */
#define PWM_CMUPD0_CPOLINVUP_Pos            13                                             /**< (PWM_CMUPD0) Channel Polarity Inversion Update Position */
#define PWM_CMUPD0_CPOLINVUP_Msk            (0x1U << PWM_CMUPD0_CPOLINVUP_Pos)             /**< (PWM_CMUPD0) Channel Polarity Inversion Update Mask */
#define PWM_CMUPD0_CPOLINVUP                PWM_CMUPD0_CPOLINVUP_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMUPD0_CPOLINVUP_Msk instead */
#define PWM_CMUPD0_MASK                     (0x2200U)                                      /**< \deprecated (PWM_CMUPD0) Register MASK  (Use PWM_CMUPD0_Msk instead)  */
#define PWM_CMUPD0_Msk                      (0x2200U)                                      /**< (PWM_CMUPD0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CMUPD1 : (PWM Offset: 0x420) (/W 32) PWM Channel Mode Update Register (ch_num = 1) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :9;                        /**< bit:   0..8  Reserved */
    uint32_t CPOLUP:1;                  /**< bit:      9  Channel Polarity Update                  */
    uint32_t :3;                        /**< bit: 10..12  Reserved */
    uint32_t CPOLINVUP:1;               /**< bit:     13  Channel Polarity Inversion Update        */
    uint32_t :18;                       /**< bit: 14..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMUPD1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMUPD1_OFFSET                   (0x420)                                       /**<  (PWM_CMUPD1) PWM Channel Mode Update Register (ch_num = 1)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMUPD1_CPOLUP_Pos               9                                              /**< (PWM_CMUPD1) Channel Polarity Update Position */
#define PWM_CMUPD1_CPOLUP_Msk               (0x1U << PWM_CMUPD1_CPOLUP_Pos)                /**< (PWM_CMUPD1) Channel Polarity Update Mask */
#define PWM_CMUPD1_CPOLUP                   PWM_CMUPD1_CPOLUP_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMUPD1_CPOLUP_Msk instead */
#define PWM_CMUPD1_CPOLINVUP_Pos            13                                             /**< (PWM_CMUPD1) Channel Polarity Inversion Update Position */
#define PWM_CMUPD1_CPOLINVUP_Msk            (0x1U << PWM_CMUPD1_CPOLINVUP_Pos)             /**< (PWM_CMUPD1) Channel Polarity Inversion Update Mask */
#define PWM_CMUPD1_CPOLINVUP                PWM_CMUPD1_CPOLINVUP_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMUPD1_CPOLINVUP_Msk instead */
#define PWM_CMUPD1_MASK                     (0x2200U)                                      /**< \deprecated (PWM_CMUPD1) Register MASK  (Use PWM_CMUPD1_Msk instead)  */
#define PWM_CMUPD1_Msk                      (0x2200U)                                      /**< (PWM_CMUPD1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_ETRG1 : (PWM Offset: 0x42c) (R/W 32) PWM External Trigger Register (trg_num = 1) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MAXCNT:24;                 /**< bit:  0..23  Maximum Counter value                    */
    uint32_t TRGMODE:2;                 /**< bit: 24..25  External Trigger Mode                    */
    uint32_t :2;                        /**< bit: 26..27  Reserved */
    uint32_t TRGEDGE:1;                 /**< bit:     28  Edge Selection                           */
    uint32_t TRGFILT:1;                 /**< bit:     29  Filtered input                           */
    uint32_t TRGSRC:1;                  /**< bit:     30  Trigger Source                           */
    uint32_t RFEN:1;                    /**< bit:     31  Recoverable Fault Enable                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_ETRG1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_ETRG1_OFFSET                    (0x42C)                                       /**<  (PWM_ETRG1) PWM External Trigger Register (trg_num = 1)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_ETRG1_MAXCNT_Pos                0                                              /**< (PWM_ETRG1) Maximum Counter value Position */
#define PWM_ETRG1_MAXCNT_Msk                (0xFFFFFFU << PWM_ETRG1_MAXCNT_Pos)            /**< (PWM_ETRG1) Maximum Counter value Mask */
#define PWM_ETRG1_MAXCNT(value)             (PWM_ETRG1_MAXCNT_Msk & ((value) << PWM_ETRG1_MAXCNT_Pos))
#define PWM_ETRG1_TRGMODE_Pos               24                                             /**< (PWM_ETRG1) External Trigger Mode Position */
#define PWM_ETRG1_TRGMODE_Msk               (0x3U << PWM_ETRG1_TRGMODE_Pos)                /**< (PWM_ETRG1) External Trigger Mode Mask */
#define PWM_ETRG1_TRGMODE(value)            (PWM_ETRG1_TRGMODE_Msk & ((value) << PWM_ETRG1_TRGMODE_Pos))
#define   PWM_ETRG1_TRGMODE_OFF_Val         (0x0U)                                         /**< (PWM_ETRG1) External trigger is not enabled.  */
#define   PWM_ETRG1_TRGMODE_MODE1_Val       (0x1U)                                         /**< (PWM_ETRG1) External PWM Reset Mode  */
#define   PWM_ETRG1_TRGMODE_MODE2_Val       (0x2U)                                         /**< (PWM_ETRG1) External PWM Start Mode  */
#define   PWM_ETRG1_TRGMODE_MODE3_Val       (0x3U)                                         /**< (PWM_ETRG1) Cycle-by-cycle Duty Mode  */
#define PWM_ETRG1_TRGMODE_OFF               (PWM_ETRG1_TRGMODE_OFF_Val << PWM_ETRG1_TRGMODE_Pos)  /**< (PWM_ETRG1) External trigger is not enabled. Position  */
#define PWM_ETRG1_TRGMODE_MODE1             (PWM_ETRG1_TRGMODE_MODE1_Val << PWM_ETRG1_TRGMODE_Pos)  /**< (PWM_ETRG1) External PWM Reset Mode Position  */
#define PWM_ETRG1_TRGMODE_MODE2             (PWM_ETRG1_TRGMODE_MODE2_Val << PWM_ETRG1_TRGMODE_Pos)  /**< (PWM_ETRG1) External PWM Start Mode Position  */
#define PWM_ETRG1_TRGMODE_MODE3             (PWM_ETRG1_TRGMODE_MODE3_Val << PWM_ETRG1_TRGMODE_Pos)  /**< (PWM_ETRG1) Cycle-by-cycle Duty Mode Position  */
#define PWM_ETRG1_TRGEDGE_Pos               28                                             /**< (PWM_ETRG1) Edge Selection Position */
#define PWM_ETRG1_TRGEDGE_Msk               (0x1U << PWM_ETRG1_TRGEDGE_Pos)                /**< (PWM_ETRG1) Edge Selection Mask */
#define PWM_ETRG1_TRGEDGE                   PWM_ETRG1_TRGEDGE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ETRG1_TRGEDGE_Msk instead */
#define   PWM_ETRG1_TRGEDGE_FALLING_ZERO_Val (0x0U)                                         /**< (PWM_ETRG1) TRGMODE = 1: TRGINx event detection on falling edge.TRGMODE = 2, 3: TRGINx active level is 0  */
#define   PWM_ETRG1_TRGEDGE_RISING_ONE_Val  (0x1U)                                         /**< (PWM_ETRG1) TRGMODE = 1: TRGINx event detection on rising edge.TRGMODE = 2, 3: TRGINx active level is 1  */
#define PWM_ETRG1_TRGEDGE_FALLING_ZERO      (PWM_ETRG1_TRGEDGE_FALLING_ZERO_Val << PWM_ETRG1_TRGEDGE_Pos)  /**< (PWM_ETRG1) TRGMODE = 1: TRGINx event detection on falling edge.TRGMODE = 2, 3: TRGINx active level is 0 Position  */
#define PWM_ETRG1_TRGEDGE_RISING_ONE        (PWM_ETRG1_TRGEDGE_RISING_ONE_Val << PWM_ETRG1_TRGEDGE_Pos)  /**< (PWM_ETRG1) TRGMODE = 1: TRGINx event detection on rising edge.TRGMODE = 2, 3: TRGINx active level is 1 Position  */
#define PWM_ETRG1_TRGFILT_Pos               29                                             /**< (PWM_ETRG1) Filtered input Position */
#define PWM_ETRG1_TRGFILT_Msk               (0x1U << PWM_ETRG1_TRGFILT_Pos)                /**< (PWM_ETRG1) Filtered input Mask */
#define PWM_ETRG1_TRGFILT                   PWM_ETRG1_TRGFILT_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ETRG1_TRGFILT_Msk instead */
#define PWM_ETRG1_TRGSRC_Pos                30                                             /**< (PWM_ETRG1) Trigger Source Position */
#define PWM_ETRG1_TRGSRC_Msk                (0x1U << PWM_ETRG1_TRGSRC_Pos)                 /**< (PWM_ETRG1) Trigger Source Mask */
#define PWM_ETRG1_TRGSRC                    PWM_ETRG1_TRGSRC_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ETRG1_TRGSRC_Msk instead */
#define PWM_ETRG1_RFEN_Pos                  31                                             /**< (PWM_ETRG1) Recoverable Fault Enable Position */
#define PWM_ETRG1_RFEN_Msk                  (0x1U << PWM_ETRG1_RFEN_Pos)                   /**< (PWM_ETRG1) Recoverable Fault Enable Mask */
#define PWM_ETRG1_RFEN                      PWM_ETRG1_RFEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ETRG1_RFEN_Msk instead */
#define PWM_ETRG1_MASK                      (0xF3FFFFFFU)                                  /**< \deprecated (PWM_ETRG1) Register MASK  (Use PWM_ETRG1_Msk instead)  */
#define PWM_ETRG1_Msk                       (0xF3FFFFFFU)                                  /**< (PWM_ETRG1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_LEBR1 : (PWM Offset: 0x430) (R/W 32) PWM Leading-Edge Blanking Register (trg_num = 1) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t LEBDELAY:7;                /**< bit:   0..6  Leading-Edge Blanking Delay for TRGINx   */
    uint32_t :9;                        /**< bit:  7..15  Reserved */
    uint32_t PWMLFEN:1;                 /**< bit:     16  PWML Falling Edge Enable                 */
    uint32_t PWMLREN:1;                 /**< bit:     17  PWML Rising Edge Enable                  */
    uint32_t PWMHFEN:1;                 /**< bit:     18  PWMH Falling Edge Enable                 */
    uint32_t PWMHREN:1;                 /**< bit:     19  PWMH Rising Edge Enable                  */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_LEBR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_LEBR1_OFFSET                    (0x430)                                       /**<  (PWM_LEBR1) PWM Leading-Edge Blanking Register (trg_num = 1)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_LEBR1_LEBDELAY_Pos              0                                              /**< (PWM_LEBR1) Leading-Edge Blanking Delay for TRGINx Position */
#define PWM_LEBR1_LEBDELAY_Msk              (0x7FU << PWM_LEBR1_LEBDELAY_Pos)              /**< (PWM_LEBR1) Leading-Edge Blanking Delay for TRGINx Mask */
#define PWM_LEBR1_LEBDELAY(value)           (PWM_LEBR1_LEBDELAY_Msk & ((value) << PWM_LEBR1_LEBDELAY_Pos))
#define PWM_LEBR1_PWMLFEN_Pos               16                                             /**< (PWM_LEBR1) PWML Falling Edge Enable Position */
#define PWM_LEBR1_PWMLFEN_Msk               (0x1U << PWM_LEBR1_PWMLFEN_Pos)                /**< (PWM_LEBR1) PWML Falling Edge Enable Mask */
#define PWM_LEBR1_PWMLFEN                   PWM_LEBR1_PWMLFEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_LEBR1_PWMLFEN_Msk instead */
#define PWM_LEBR1_PWMLREN_Pos               17                                             /**< (PWM_LEBR1) PWML Rising Edge Enable Position */
#define PWM_LEBR1_PWMLREN_Msk               (0x1U << PWM_LEBR1_PWMLREN_Pos)                /**< (PWM_LEBR1) PWML Rising Edge Enable Mask */
#define PWM_LEBR1_PWMLREN                   PWM_LEBR1_PWMLREN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_LEBR1_PWMLREN_Msk instead */
#define PWM_LEBR1_PWMHFEN_Pos               18                                             /**< (PWM_LEBR1) PWMH Falling Edge Enable Position */
#define PWM_LEBR1_PWMHFEN_Msk               (0x1U << PWM_LEBR1_PWMHFEN_Pos)                /**< (PWM_LEBR1) PWMH Falling Edge Enable Mask */
#define PWM_LEBR1_PWMHFEN                   PWM_LEBR1_PWMHFEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_LEBR1_PWMHFEN_Msk instead */
#define PWM_LEBR1_PWMHREN_Pos               19                                             /**< (PWM_LEBR1) PWMH Rising Edge Enable Position */
#define PWM_LEBR1_PWMHREN_Msk               (0x1U << PWM_LEBR1_PWMHREN_Pos)                /**< (PWM_LEBR1) PWMH Rising Edge Enable Mask */
#define PWM_LEBR1_PWMHREN                   PWM_LEBR1_PWMHREN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_LEBR1_PWMHREN_Msk instead */
#define PWM_LEBR1_MASK                      (0xF007FU)                                     /**< \deprecated (PWM_LEBR1) Register MASK  (Use PWM_LEBR1_Msk instead)  */
#define PWM_LEBR1_Msk                       (0xF007FU)                                     /**< (PWM_LEBR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CMUPD2 : (PWM Offset: 0x440) (/W 32) PWM Channel Mode Update Register (ch_num = 2) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :9;                        /**< bit:   0..8  Reserved */
    uint32_t CPOLUP:1;                  /**< bit:      9  Channel Polarity Update                  */
    uint32_t :3;                        /**< bit: 10..12  Reserved */
    uint32_t CPOLINVUP:1;               /**< bit:     13  Channel Polarity Inversion Update        */
    uint32_t :18;                       /**< bit: 14..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMUPD2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMUPD2_OFFSET                   (0x440)                                       /**<  (PWM_CMUPD2) PWM Channel Mode Update Register (ch_num = 2)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMUPD2_CPOLUP_Pos               9                                              /**< (PWM_CMUPD2) Channel Polarity Update Position */
#define PWM_CMUPD2_CPOLUP_Msk               (0x1U << PWM_CMUPD2_CPOLUP_Pos)                /**< (PWM_CMUPD2) Channel Polarity Update Mask */
#define PWM_CMUPD2_CPOLUP                   PWM_CMUPD2_CPOLUP_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMUPD2_CPOLUP_Msk instead */
#define PWM_CMUPD2_CPOLINVUP_Pos            13                                             /**< (PWM_CMUPD2) Channel Polarity Inversion Update Position */
#define PWM_CMUPD2_CPOLINVUP_Msk            (0x1U << PWM_CMUPD2_CPOLINVUP_Pos)             /**< (PWM_CMUPD2) Channel Polarity Inversion Update Mask */
#define PWM_CMUPD2_CPOLINVUP                PWM_CMUPD2_CPOLINVUP_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMUPD2_CPOLINVUP_Msk instead */
#define PWM_CMUPD2_MASK                     (0x2200U)                                      /**< \deprecated (PWM_CMUPD2) Register MASK  (Use PWM_CMUPD2_Msk instead)  */
#define PWM_CMUPD2_Msk                      (0x2200U)                                      /**< (PWM_CMUPD2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_ETRG2 : (PWM Offset: 0x44c) (R/W 32) PWM External Trigger Register (trg_num = 2) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MAXCNT:24;                 /**< bit:  0..23  Maximum Counter value                    */
    uint32_t TRGMODE:2;                 /**< bit: 24..25  External Trigger Mode                    */
    uint32_t :2;                        /**< bit: 26..27  Reserved */
    uint32_t TRGEDGE:1;                 /**< bit:     28  Edge Selection                           */
    uint32_t TRGFILT:1;                 /**< bit:     29  Filtered input                           */
    uint32_t TRGSRC:1;                  /**< bit:     30  Trigger Source                           */
    uint32_t RFEN:1;                    /**< bit:     31  Recoverable Fault Enable                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_ETRG2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_ETRG2_OFFSET                    (0x44C)                                       /**<  (PWM_ETRG2) PWM External Trigger Register (trg_num = 2)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_ETRG2_MAXCNT_Pos                0                                              /**< (PWM_ETRG2) Maximum Counter value Position */
#define PWM_ETRG2_MAXCNT_Msk                (0xFFFFFFU << PWM_ETRG2_MAXCNT_Pos)            /**< (PWM_ETRG2) Maximum Counter value Mask */
#define PWM_ETRG2_MAXCNT(value)             (PWM_ETRG2_MAXCNT_Msk & ((value) << PWM_ETRG2_MAXCNT_Pos))
#define PWM_ETRG2_TRGMODE_Pos               24                                             /**< (PWM_ETRG2) External Trigger Mode Position */
#define PWM_ETRG2_TRGMODE_Msk               (0x3U << PWM_ETRG2_TRGMODE_Pos)                /**< (PWM_ETRG2) External Trigger Mode Mask */
#define PWM_ETRG2_TRGMODE(value)            (PWM_ETRG2_TRGMODE_Msk & ((value) << PWM_ETRG2_TRGMODE_Pos))
#define   PWM_ETRG2_TRGMODE_OFF_Val         (0x0U)                                         /**< (PWM_ETRG2) External trigger is not enabled.  */
#define   PWM_ETRG2_TRGMODE_MODE1_Val       (0x1U)                                         /**< (PWM_ETRG2) External PWM Reset Mode  */
#define   PWM_ETRG2_TRGMODE_MODE2_Val       (0x2U)                                         /**< (PWM_ETRG2) External PWM Start Mode  */
#define   PWM_ETRG2_TRGMODE_MODE3_Val       (0x3U)                                         /**< (PWM_ETRG2) Cycle-by-cycle Duty Mode  */
#define PWM_ETRG2_TRGMODE_OFF               (PWM_ETRG2_TRGMODE_OFF_Val << PWM_ETRG2_TRGMODE_Pos)  /**< (PWM_ETRG2) External trigger is not enabled. Position  */
#define PWM_ETRG2_TRGMODE_MODE1             (PWM_ETRG2_TRGMODE_MODE1_Val << PWM_ETRG2_TRGMODE_Pos)  /**< (PWM_ETRG2) External PWM Reset Mode Position  */
#define PWM_ETRG2_TRGMODE_MODE2             (PWM_ETRG2_TRGMODE_MODE2_Val << PWM_ETRG2_TRGMODE_Pos)  /**< (PWM_ETRG2) External PWM Start Mode Position  */
#define PWM_ETRG2_TRGMODE_MODE3             (PWM_ETRG2_TRGMODE_MODE3_Val << PWM_ETRG2_TRGMODE_Pos)  /**< (PWM_ETRG2) Cycle-by-cycle Duty Mode Position  */
#define PWM_ETRG2_TRGEDGE_Pos               28                                             /**< (PWM_ETRG2) Edge Selection Position */
#define PWM_ETRG2_TRGEDGE_Msk               (0x1U << PWM_ETRG2_TRGEDGE_Pos)                /**< (PWM_ETRG2) Edge Selection Mask */
#define PWM_ETRG2_TRGEDGE                   PWM_ETRG2_TRGEDGE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ETRG2_TRGEDGE_Msk instead */
#define   PWM_ETRG2_TRGEDGE_FALLING_ZERO_Val (0x0U)                                         /**< (PWM_ETRG2) TRGMODE = 1: TRGINx event detection on falling edge.TRGMODE = 2, 3: TRGINx active level is 0  */
#define   PWM_ETRG2_TRGEDGE_RISING_ONE_Val  (0x1U)                                         /**< (PWM_ETRG2) TRGMODE = 1: TRGINx event detection on rising edge.TRGMODE = 2, 3: TRGINx active level is 1  */
#define PWM_ETRG2_TRGEDGE_FALLING_ZERO      (PWM_ETRG2_TRGEDGE_FALLING_ZERO_Val << PWM_ETRG2_TRGEDGE_Pos)  /**< (PWM_ETRG2) TRGMODE = 1: TRGINx event detection on falling edge.TRGMODE = 2, 3: TRGINx active level is 0 Position  */
#define PWM_ETRG2_TRGEDGE_RISING_ONE        (PWM_ETRG2_TRGEDGE_RISING_ONE_Val << PWM_ETRG2_TRGEDGE_Pos)  /**< (PWM_ETRG2) TRGMODE = 1: TRGINx event detection on rising edge.TRGMODE = 2, 3: TRGINx active level is 1 Position  */
#define PWM_ETRG2_TRGFILT_Pos               29                                             /**< (PWM_ETRG2) Filtered input Position */
#define PWM_ETRG2_TRGFILT_Msk               (0x1U << PWM_ETRG2_TRGFILT_Pos)                /**< (PWM_ETRG2) Filtered input Mask */
#define PWM_ETRG2_TRGFILT                   PWM_ETRG2_TRGFILT_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ETRG2_TRGFILT_Msk instead */
#define PWM_ETRG2_TRGSRC_Pos                30                                             /**< (PWM_ETRG2) Trigger Source Position */
#define PWM_ETRG2_TRGSRC_Msk                (0x1U << PWM_ETRG2_TRGSRC_Pos)                 /**< (PWM_ETRG2) Trigger Source Mask */
#define PWM_ETRG2_TRGSRC                    PWM_ETRG2_TRGSRC_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ETRG2_TRGSRC_Msk instead */
#define PWM_ETRG2_RFEN_Pos                  31                                             /**< (PWM_ETRG2) Recoverable Fault Enable Position */
#define PWM_ETRG2_RFEN_Msk                  (0x1U << PWM_ETRG2_RFEN_Pos)                   /**< (PWM_ETRG2) Recoverable Fault Enable Mask */
#define PWM_ETRG2_RFEN                      PWM_ETRG2_RFEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_ETRG2_RFEN_Msk instead */
#define PWM_ETRG2_MASK                      (0xF3FFFFFFU)                                  /**< \deprecated (PWM_ETRG2) Register MASK  (Use PWM_ETRG2_Msk instead)  */
#define PWM_ETRG2_Msk                       (0xF3FFFFFFU)                                  /**< (PWM_ETRG2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_LEBR2 : (PWM Offset: 0x450) (R/W 32) PWM Leading-Edge Blanking Register (trg_num = 2) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t LEBDELAY:7;                /**< bit:   0..6  Leading-Edge Blanking Delay for TRGINx   */
    uint32_t :9;                        /**< bit:  7..15  Reserved */
    uint32_t PWMLFEN:1;                 /**< bit:     16  PWML Falling Edge Enable                 */
    uint32_t PWMLREN:1;                 /**< bit:     17  PWML Rising Edge Enable                  */
    uint32_t PWMHFEN:1;                 /**< bit:     18  PWMH Falling Edge Enable                 */
    uint32_t PWMHREN:1;                 /**< bit:     19  PWMH Rising Edge Enable                  */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_LEBR2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_LEBR2_OFFSET                    (0x450)                                       /**<  (PWM_LEBR2) PWM Leading-Edge Blanking Register (trg_num = 2)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_LEBR2_LEBDELAY_Pos              0                                              /**< (PWM_LEBR2) Leading-Edge Blanking Delay for TRGINx Position */
#define PWM_LEBR2_LEBDELAY_Msk              (0x7FU << PWM_LEBR2_LEBDELAY_Pos)              /**< (PWM_LEBR2) Leading-Edge Blanking Delay for TRGINx Mask */
#define PWM_LEBR2_LEBDELAY(value)           (PWM_LEBR2_LEBDELAY_Msk & ((value) << PWM_LEBR2_LEBDELAY_Pos))
#define PWM_LEBR2_PWMLFEN_Pos               16                                             /**< (PWM_LEBR2) PWML Falling Edge Enable Position */
#define PWM_LEBR2_PWMLFEN_Msk               (0x1U << PWM_LEBR2_PWMLFEN_Pos)                /**< (PWM_LEBR2) PWML Falling Edge Enable Mask */
#define PWM_LEBR2_PWMLFEN                   PWM_LEBR2_PWMLFEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_LEBR2_PWMLFEN_Msk instead */
#define PWM_LEBR2_PWMLREN_Pos               17                                             /**< (PWM_LEBR2) PWML Rising Edge Enable Position */
#define PWM_LEBR2_PWMLREN_Msk               (0x1U << PWM_LEBR2_PWMLREN_Pos)                /**< (PWM_LEBR2) PWML Rising Edge Enable Mask */
#define PWM_LEBR2_PWMLREN                   PWM_LEBR2_PWMLREN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_LEBR2_PWMLREN_Msk instead */
#define PWM_LEBR2_PWMHFEN_Pos               18                                             /**< (PWM_LEBR2) PWMH Falling Edge Enable Position */
#define PWM_LEBR2_PWMHFEN_Msk               (0x1U << PWM_LEBR2_PWMHFEN_Pos)                /**< (PWM_LEBR2) PWMH Falling Edge Enable Mask */
#define PWM_LEBR2_PWMHFEN                   PWM_LEBR2_PWMHFEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_LEBR2_PWMHFEN_Msk instead */
#define PWM_LEBR2_PWMHREN_Pos               19                                             /**< (PWM_LEBR2) PWMH Rising Edge Enable Position */
#define PWM_LEBR2_PWMHREN_Msk               (0x1U << PWM_LEBR2_PWMHREN_Pos)                /**< (PWM_LEBR2) PWMH Rising Edge Enable Mask */
#define PWM_LEBR2_PWMHREN                   PWM_LEBR2_PWMHREN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_LEBR2_PWMHREN_Msk instead */
#define PWM_LEBR2_MASK                      (0xF007FU)                                     /**< \deprecated (PWM_LEBR2) Register MASK  (Use PWM_LEBR2_Msk instead)  */
#define PWM_LEBR2_Msk                       (0xF007FU)                                     /**< (PWM_LEBR2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PWM_CMUPD3 : (PWM Offset: 0x460) (/W 32) PWM Channel Mode Update Register (ch_num = 3) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :9;                        /**< bit:   0..8  Reserved */
    uint32_t CPOLUP:1;                  /**< bit:      9  Channel Polarity Update                  */
    uint32_t :3;                        /**< bit: 10..12  Reserved */
    uint32_t CPOLINVUP:1;               /**< bit:     13  Channel Polarity Inversion Update        */
    uint32_t :18;                       /**< bit: 14..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PWM_CMUPD3_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PWM_CMUPD3_OFFSET                   (0x460)                                       /**<  (PWM_CMUPD3) PWM Channel Mode Update Register (ch_num = 3)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PWM_CMUPD3_CPOLUP_Pos               9                                              /**< (PWM_CMUPD3) Channel Polarity Update Position */
#define PWM_CMUPD3_CPOLUP_Msk               (0x1U << PWM_CMUPD3_CPOLUP_Pos)                /**< (PWM_CMUPD3) Channel Polarity Update Mask */
#define PWM_CMUPD3_CPOLUP                   PWM_CMUPD3_CPOLUP_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMUPD3_CPOLUP_Msk instead */
#define PWM_CMUPD3_CPOLINVUP_Pos            13                                             /**< (PWM_CMUPD3) Channel Polarity Inversion Update Position */
#define PWM_CMUPD3_CPOLINVUP_Msk            (0x1U << PWM_CMUPD3_CPOLINVUP_Pos)             /**< (PWM_CMUPD3) Channel Polarity Inversion Update Mask */
#define PWM_CMUPD3_CPOLINVUP                PWM_CMUPD3_CPOLINVUP_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PWM_CMUPD3_CPOLINVUP_Msk instead */
#define PWM_CMUPD3_MASK                     (0x2200U)                                      /**< \deprecated (PWM_CMUPD3) Register MASK  (Use PWM_CMUPD3_Msk instead)  */
#define PWM_CMUPD3_Msk                      (0x2200U)                                      /**< (PWM_CMUPD3) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
#define PWMCMP_NUMBER 8
#define PWMCHNUM_NUMBER 4
/** \brief PWM_CH_NUM hardware registers */
typedef struct {  
  __IO uint32_t PWM_CMR;        /**< (PWM_CH_NUM Offset: 0x00) PWM Channel Mode Register (ch_num = 0) */
  __IO uint32_t PWM_CDTY;       /**< (PWM_CH_NUM Offset: 0x04) PWM Channel Duty Cycle Register (ch_num = 0) */
  __O  uint32_t PWM_CDTYUPD;    /**< (PWM_CH_NUM Offset: 0x08) PWM Channel Duty Cycle Update Register (ch_num = 0) */
  __IO uint32_t PWM_CPRD;       /**< (PWM_CH_NUM Offset: 0x0C) PWM Channel Period Register (ch_num = 0) */
  __O  uint32_t PWM_CPRDUPD;    /**< (PWM_CH_NUM Offset: 0x10) PWM Channel Period Update Register (ch_num = 0) */
  __I  uint32_t PWM_CCNT;       /**< (PWM_CH_NUM Offset: 0x14) PWM Channel Counter Register (ch_num = 0) */
  __IO uint32_t PWM_DT;         /**< (PWM_CH_NUM Offset: 0x18) PWM Channel Dead Time Register (ch_num = 0) */
  __O  uint32_t PWM_DTUPD;      /**< (PWM_CH_NUM Offset: 0x1C) PWM Channel Dead Time Update Register (ch_num = 0) */
} PwmChNum;

/** \brief PWM_CMP hardware registers */
typedef struct {  
  __IO uint32_t PWM_CMPV;       /**< (PWM_CMP Offset: 0x00) PWM Comparison 0 Value Register */
  __O  uint32_t PWM_CMPVUPD;    /**< (PWM_CMP Offset: 0x04) PWM Comparison 0 Value Update Register */
  __IO uint32_t PWM_CMPM;       /**< (PWM_CMP Offset: 0x08) PWM Comparison 0 Mode Register */
  __O  uint32_t PWM_CMPMUPD;    /**< (PWM_CMP Offset: 0x0C) PWM Comparison 0 Mode Update Register */
} PwmCmp;

/** \brief PWM hardware registers */
typedef struct {  
  __IO uint32_t PWM_CLK;        /**< (PWM Offset: 0x00) PWM Clock Register */
  __O  uint32_t PWM_ENA;        /**< (PWM Offset: 0x04) PWM Enable Register */
  __O  uint32_t PWM_DIS;        /**< (PWM Offset: 0x08) PWM Disable Register */
  __I  uint32_t PWM_SR;         /**< (PWM Offset: 0x0C) PWM Status Register */
  __O  uint32_t PWM_IER1;       /**< (PWM Offset: 0x10) PWM Interrupt Enable Register 1 */
  __O  uint32_t PWM_IDR1;       /**< (PWM Offset: 0x14) PWM Interrupt Disable Register 1 */
  __I  uint32_t PWM_IMR1;       /**< (PWM Offset: 0x18) PWM Interrupt Mask Register 1 */
  __I  uint32_t PWM_ISR1;       /**< (PWM Offset: 0x1C) PWM Interrupt Status Register 1 */
  __IO uint32_t PWM_SCM;        /**< (PWM Offset: 0x20) PWM Sync Channels Mode Register */
  __O  uint32_t PWM_DMAR;       /**< (PWM Offset: 0x24) PWM DMA Register */
  __IO uint32_t PWM_SCUC;       /**< (PWM Offset: 0x28) PWM Sync Channels Update Control Register */
  __IO uint32_t PWM_SCUP;       /**< (PWM Offset: 0x2C) PWM Sync Channels Update Period Register */
  __O  uint32_t PWM_SCUPUPD;    /**< (PWM Offset: 0x30) PWM Sync Channels Update Period Update Register */
  __O  uint32_t PWM_IER2;       /**< (PWM Offset: 0x34) PWM Interrupt Enable Register 2 */
  __O  uint32_t PWM_IDR2;       /**< (PWM Offset: 0x38) PWM Interrupt Disable Register 2 */
  __I  uint32_t PWM_IMR2;       /**< (PWM Offset: 0x3C) PWM Interrupt Mask Register 2 */
  __I  uint32_t PWM_ISR2;       /**< (PWM Offset: 0x40) PWM Interrupt Status Register 2 */
  __IO uint32_t PWM_OOV;        /**< (PWM Offset: 0x44) PWM Output Override Value Register */
  __IO uint32_t PWM_OS;         /**< (PWM Offset: 0x48) PWM Output Selection Register */
  __O  uint32_t PWM_OSS;        /**< (PWM Offset: 0x4C) PWM Output Selection Set Register */
  __O  uint32_t PWM_OSC;        /**< (PWM Offset: 0x50) PWM Output Selection Clear Register */
  __O  uint32_t PWM_OSSUPD;     /**< (PWM Offset: 0x54) PWM Output Selection Set Update Register */
  __O  uint32_t PWM_OSCUPD;     /**< (PWM Offset: 0x58) PWM Output Selection Clear Update Register */
  __IO uint32_t PWM_FMR;        /**< (PWM Offset: 0x5C) PWM Fault Mode Register */
  __I  uint32_t PWM_FSR;        /**< (PWM Offset: 0x60) PWM Fault Status Register */
  __O  uint32_t PWM_FCR;        /**< (PWM Offset: 0x64) PWM Fault Clear Register */
  __IO uint32_t PWM_FPV1;       /**< (PWM Offset: 0x68) PWM Fault Protection Value Register 1 */
  __IO uint32_t PWM_FPE;        /**< (PWM Offset: 0x6C) PWM Fault Protection Enable Register */
  __I  uint32_t Reserved1[3];
  __IO uint32_t PWM_ELMR[2];    /**< (PWM Offset: 0x7C) PWM Event Line 0 Mode Register 0 */
  __I  uint32_t Reserved2[7];
  __IO uint32_t PWM_SSPR;       /**< (PWM Offset: 0xA0) PWM Spread Spectrum Register */
  __O  uint32_t PWM_SSPUP;      /**< (PWM Offset: 0xA4) PWM Spread Spectrum Update Register */
  __I  uint32_t Reserved3[2];
  __IO uint32_t PWM_SMMR;       /**< (PWM Offset: 0xB0) PWM Stepper Motor Mode Register */
  __I  uint32_t Reserved4[3];
  __IO uint32_t PWM_FPV2;       /**< (PWM Offset: 0xC0) PWM Fault Protection Value 2 Register */
  __I  uint32_t Reserved5[8];
  __O  uint32_t PWM_WPCR;       /**< (PWM Offset: 0xE4) PWM Write Protection Control Register */
  __I  uint32_t PWM_WPSR;       /**< (PWM Offset: 0xE8) PWM Write Protection Status Register */
  __I  uint32_t Reserved6[17];
       PwmCmp   PWM_CMP[PWMCMP_NUMBER]; /**< Offset: 0x130 PWM Comparison 0 Value Register */
  __I  uint32_t Reserved7[20];
       PwmChNum PWM_CH_NUM[PWMCHNUM_NUMBER]; /**< Offset: 0x200 PWM Channel Mode Register (ch_num = 0) */
  __I  uint32_t Reserved8[96];
  __O  uint32_t PWM_CMUPD0;     /**< (PWM Offset: 0x400) PWM Channel Mode Update Register (ch_num = 0) */
  __I  uint32_t Reserved9[7];
  __O  uint32_t PWM_CMUPD1;     /**< (PWM Offset: 0x420) PWM Channel Mode Update Register (ch_num = 1) */
  __I  uint32_t Reserved10[2];
  __IO uint32_t PWM_ETRG1;      /**< (PWM Offset: 0x42C) PWM External Trigger Register (trg_num = 1) */
  __IO uint32_t PWM_LEBR1;      /**< (PWM Offset: 0x430) PWM Leading-Edge Blanking Register (trg_num = 1) */
  __I  uint32_t Reserved11[3];
  __O  uint32_t PWM_CMUPD2;     /**< (PWM Offset: 0x440) PWM Channel Mode Update Register (ch_num = 2) */
  __I  uint32_t Reserved12[2];
  __IO uint32_t PWM_ETRG2;      /**< (PWM Offset: 0x44C) PWM External Trigger Register (trg_num = 2) */
  __IO uint32_t PWM_LEBR2;      /**< (PWM Offset: 0x450) PWM Leading-Edge Blanking Register (trg_num = 2) */
  __I  uint32_t Reserved13[3];
  __O  uint32_t PWM_CMUPD3;     /**< (PWM Offset: 0x460) PWM Channel Mode Update Register (ch_num = 3) */
} Pwm;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief PWM_CH_NUM hardware registers */
typedef struct {  
  __IO PWM_CMR_Type                   PWM_CMR;        /**< Offset: 0x00 (R/W  32) PWM Channel Mode Register (ch_num = 0) */
  __IO PWM_CDTY_Type                  PWM_CDTY;       /**< Offset: 0x04 (R/W  32) PWM Channel Duty Cycle Register (ch_num = 0) */
  __O  PWM_CDTYUPD_Type               PWM_CDTYUPD;    /**< Offset: 0x08 ( /W  32) PWM Channel Duty Cycle Update Register (ch_num = 0) */
  __IO PWM_CPRD_Type                  PWM_CPRD;       /**< Offset: 0x0C (R/W  32) PWM Channel Period Register (ch_num = 0) */
  __O  PWM_CPRDUPD_Type               PWM_CPRDUPD;    /**< Offset: 0x10 ( /W  32) PWM Channel Period Update Register (ch_num = 0) */
  __I  PWM_CCNT_Type                  PWM_CCNT;       /**< Offset: 0x14 (R/   32) PWM Channel Counter Register (ch_num = 0) */
  __IO PWM_DT_Type                    PWM_DT;         /**< Offset: 0x18 (R/W  32) PWM Channel Dead Time Register (ch_num = 0) */
  __O  PWM_DTUPD_Type                 PWM_DTUPD;      /**< Offset: 0x1C ( /W  32) PWM Channel Dead Time Update Register (ch_num = 0) */
} PwmChNum;

/** \brief PWM_CMP hardware registers */
typedef struct {  
  __IO PWM_CMPV_Type                  PWM_CMPV;       /**< Offset: 0x00 (R/W  32) PWM Comparison 0 Value Register */
  __O  PWM_CMPVUPD_Type               PWM_CMPVUPD;    /**< Offset: 0x04 ( /W  32) PWM Comparison 0 Value Update Register */
  __IO PWM_CMPM_Type                  PWM_CMPM;       /**< Offset: 0x08 (R/W  32) PWM Comparison 0 Mode Register */
  __O  PWM_CMPMUPD_Type               PWM_CMPMUPD;    /**< Offset: 0x0C ( /W  32) PWM Comparison 0 Mode Update Register */
} PwmCmp;

/** \brief PWM hardware registers */
typedef struct {  
  __IO PWM_CLK_Type                   PWM_CLK;        /**< Offset: 0x00 (R/W  32) PWM Clock Register */
  __O  PWM_ENA_Type                   PWM_ENA;        /**< Offset: 0x04 ( /W  32) PWM Enable Register */
  __O  PWM_DIS_Type                   PWM_DIS;        /**< Offset: 0x08 ( /W  32) PWM Disable Register */
  __I  PWM_SR_Type                    PWM_SR;         /**< Offset: 0x0C (R/   32) PWM Status Register */
  __O  PWM_IER1_Type                  PWM_IER1;       /**< Offset: 0x10 ( /W  32) PWM Interrupt Enable Register 1 */
  __O  PWM_IDR1_Type                  PWM_IDR1;       /**< Offset: 0x14 ( /W  32) PWM Interrupt Disable Register 1 */
  __I  PWM_IMR1_Type                  PWM_IMR1;       /**< Offset: 0x18 (R/   32) PWM Interrupt Mask Register 1 */
  __I  PWM_ISR1_Type                  PWM_ISR1;       /**< Offset: 0x1C (R/   32) PWM Interrupt Status Register 1 */
  __IO PWM_SCM_Type                   PWM_SCM;        /**< Offset: 0x20 (R/W  32) PWM Sync Channels Mode Register */
  __O  PWM_DMAR_Type                  PWM_DMAR;       /**< Offset: 0x24 ( /W  32) PWM DMA Register */
  __IO PWM_SCUC_Type                  PWM_SCUC;       /**< Offset: 0x28 (R/W  32) PWM Sync Channels Update Control Register */
  __IO PWM_SCUP_Type                  PWM_SCUP;       /**< Offset: 0x2C (R/W  32) PWM Sync Channels Update Period Register */
  __O  PWM_SCUPUPD_Type               PWM_SCUPUPD;    /**< Offset: 0x30 ( /W  32) PWM Sync Channels Update Period Update Register */
  __O  PWM_IER2_Type                  PWM_IER2;       /**< Offset: 0x34 ( /W  32) PWM Interrupt Enable Register 2 */
  __O  PWM_IDR2_Type                  PWM_IDR2;       /**< Offset: 0x38 ( /W  32) PWM Interrupt Disable Register 2 */
  __I  PWM_IMR2_Type                  PWM_IMR2;       /**< Offset: 0x3C (R/   32) PWM Interrupt Mask Register 2 */
  __I  PWM_ISR2_Type                  PWM_ISR2;       /**< Offset: 0x40 (R/   32) PWM Interrupt Status Register 2 */
  __IO PWM_OOV_Type                   PWM_OOV;        /**< Offset: 0x44 (R/W  32) PWM Output Override Value Register */
  __IO PWM_OS_Type                    PWM_OS;         /**< Offset: 0x48 (R/W  32) PWM Output Selection Register */
  __O  PWM_OSS_Type                   PWM_OSS;        /**< Offset: 0x4C ( /W  32) PWM Output Selection Set Register */
  __O  PWM_OSC_Type                   PWM_OSC;        /**< Offset: 0x50 ( /W  32) PWM Output Selection Clear Register */
  __O  PWM_OSSUPD_Type                PWM_OSSUPD;     /**< Offset: 0x54 ( /W  32) PWM Output Selection Set Update Register */
  __O  PWM_OSCUPD_Type                PWM_OSCUPD;     /**< Offset: 0x58 ( /W  32) PWM Output Selection Clear Update Register */
  __IO PWM_FMR_Type                   PWM_FMR;        /**< Offset: 0x5C (R/W  32) PWM Fault Mode Register */
  __I  PWM_FSR_Type                   PWM_FSR;        /**< Offset: 0x60 (R/   32) PWM Fault Status Register */
  __O  PWM_FCR_Type                   PWM_FCR;        /**< Offset: 0x64 ( /W  32) PWM Fault Clear Register */
  __IO PWM_FPV1_Type                  PWM_FPV1;       /**< Offset: 0x68 (R/W  32) PWM Fault Protection Value Register 1 */
  __IO PWM_FPE_Type                   PWM_FPE;        /**< Offset: 0x6C (R/W  32) PWM Fault Protection Enable Register */
       RoReg8                         Reserved1[0xC];
  __IO PWM_ELMR_Type                  PWM_ELMR[2];    /**< Offset: 0x7C (R/W  32) PWM Event Line 0 Mode Register 0 */
       RoReg8                         Reserved2[0x1C];
  __IO PWM_SSPR_Type                  PWM_SSPR;       /**< Offset: 0xA0 (R/W  32) PWM Spread Spectrum Register */
  __O  PWM_SSPUP_Type                 PWM_SSPUP;      /**< Offset: 0xA4 ( /W  32) PWM Spread Spectrum Update Register */
       RoReg8                         Reserved3[0x8];
  __IO PWM_SMMR_Type                  PWM_SMMR;       /**< Offset: 0xB0 (R/W  32) PWM Stepper Motor Mode Register */
       RoReg8                         Reserved4[0xC];
  __IO PWM_FPV2_Type                  PWM_FPV2;       /**< Offset: 0xC0 (R/W  32) PWM Fault Protection Value 2 Register */
       RoReg8                         Reserved5[0x20];
  __O  PWM_WPCR_Type                  PWM_WPCR;       /**< Offset: 0xE4 ( /W  32) PWM Write Protection Control Register */
  __I  PWM_WPSR_Type                  PWM_WPSR;       /**< Offset: 0xE8 (R/   32) PWM Write Protection Status Register */
       RoReg8                         Reserved6[0x44];
       PwmCmp                         PWM_CMP[8];     /**< Offset: 0x130 PWM Comparison 0 Value Register */
       RoReg8                         Reserved7[0x50];
       PwmChNum                       PWM_CH_NUM[4];  /**< Offset: 0x200 PWM Channel Mode Register (ch_num = 0) */
       RoReg8                         Reserved8[0x180];
  __O  PWM_CMUPD0_Type                PWM_CMUPD0;     /**< Offset: 0x400 ( /W  32) PWM Channel Mode Update Register (ch_num = 0) */
       RoReg8                         Reserved9[0x1C];
  __O  PWM_CMUPD1_Type                PWM_CMUPD1;     /**< Offset: 0x420 ( /W  32) PWM Channel Mode Update Register (ch_num = 1) */
       RoReg8                         Reserved10[0x8];
  __IO PWM_ETRG1_Type                 PWM_ETRG1;      /**< Offset: 0x42C (R/W  32) PWM External Trigger Register (trg_num = 1) */
  __IO PWM_LEBR1_Type                 PWM_LEBR1;      /**< Offset: 0x430 (R/W  32) PWM Leading-Edge Blanking Register (trg_num = 1) */
       RoReg8                         Reserved11[0xC];
  __O  PWM_CMUPD2_Type                PWM_CMUPD2;     /**< Offset: 0x440 ( /W  32) PWM Channel Mode Update Register (ch_num = 2) */
       RoReg8                         Reserved12[0x8];
  __IO PWM_ETRG2_Type                 PWM_ETRG2;      /**< Offset: 0x44C (R/W  32) PWM External Trigger Register (trg_num = 2) */
  __IO PWM_LEBR2_Type                 PWM_LEBR2;      /**< Offset: 0x450 (R/W  32) PWM Leading-Edge Blanking Register (trg_num = 2) */
       RoReg8                         Reserved13[0xC];
  __O  PWM_CMUPD3_Type                PWM_CMUPD3;     /**< Offset: 0x460 ( /W  32) PWM Channel Mode Update Register (ch_num = 3) */
} Pwm;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Pulse Width Modulation Controller */

#endif /* _SAME70_PWM_COMPONENT_H_ */
