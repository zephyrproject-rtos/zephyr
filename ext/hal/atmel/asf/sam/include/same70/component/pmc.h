/**
 * \file
 *
 * \brief Component description for PMC
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

#ifndef _SAME70_PMC_COMPONENT_H_
#define _SAME70_PMC_COMPONENT_H_
#define _SAME70_PMC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_PMC Power Management Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR PMC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define PMC_44006                      /**< (PMC) Module ID */
#define REV_PMC G                      /**< (PMC) Module revision */

/* -------- PMC_SCER : (PMC Offset: 0x00) (/W 32) System Clock Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :5;                        /**< bit:   0..4  Reserved */
    uint32_t USBCLK:1;                  /**< bit:      5  Enable USB FS Clock                      */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t PCK0:1;                    /**< bit:      8  Programmable Clock 0 Output Enable       */
    uint32_t PCK1:1;                    /**< bit:      9  Programmable Clock 1 Output Enable       */
    uint32_t PCK2:1;                    /**< bit:     10  Programmable Clock 2 Output Enable       */
    uint32_t PCK3:1;                    /**< bit:     11  Programmable Clock 3 Output Enable       */
    uint32_t PCK4:1;                    /**< bit:     12  Programmable Clock 4 Output Enable       */
    uint32_t PCK5:1;                    /**< bit:     13  Programmable Clock 5 Output Enable       */
    uint32_t PCK6:1;                    /**< bit:     14  Programmable Clock 6 Output Enable       */
    uint32_t :17;                       /**< bit: 15..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :8;                        /**< bit:   0..7  Reserved */
    uint32_t PCK:7;                     /**< bit:  8..14  Programmable Clock 6 Output Enable       */
    uint32_t :17;                       /**< bit: 15..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SCER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SCER_OFFSET                     (0x00)                                        /**<  (PMC_SCER) System Clock Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SCER_USBCLK_Pos                 5                                              /**< (PMC_SCER) Enable USB FS Clock Position */
#define PMC_SCER_USBCLK_Msk                 (0x1U << PMC_SCER_USBCLK_Pos)                  /**< (PMC_SCER) Enable USB FS Clock Mask */
#define PMC_SCER_USBCLK                     PMC_SCER_USBCLK_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCER_USBCLK_Msk instead */
#define PMC_SCER_PCK0_Pos                   8                                              /**< (PMC_SCER) Programmable Clock 0 Output Enable Position */
#define PMC_SCER_PCK0_Msk                   (0x1U << PMC_SCER_PCK0_Pos)                    /**< (PMC_SCER) Programmable Clock 0 Output Enable Mask */
#define PMC_SCER_PCK0                       PMC_SCER_PCK0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCER_PCK0_Msk instead */
#define PMC_SCER_PCK1_Pos                   9                                              /**< (PMC_SCER) Programmable Clock 1 Output Enable Position */
#define PMC_SCER_PCK1_Msk                   (0x1U << PMC_SCER_PCK1_Pos)                    /**< (PMC_SCER) Programmable Clock 1 Output Enable Mask */
#define PMC_SCER_PCK1                       PMC_SCER_PCK1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCER_PCK1_Msk instead */
#define PMC_SCER_PCK2_Pos                   10                                             /**< (PMC_SCER) Programmable Clock 2 Output Enable Position */
#define PMC_SCER_PCK2_Msk                   (0x1U << PMC_SCER_PCK2_Pos)                    /**< (PMC_SCER) Programmable Clock 2 Output Enable Mask */
#define PMC_SCER_PCK2                       PMC_SCER_PCK2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCER_PCK2_Msk instead */
#define PMC_SCER_PCK3_Pos                   11                                             /**< (PMC_SCER) Programmable Clock 3 Output Enable Position */
#define PMC_SCER_PCK3_Msk                   (0x1U << PMC_SCER_PCK3_Pos)                    /**< (PMC_SCER) Programmable Clock 3 Output Enable Mask */
#define PMC_SCER_PCK3                       PMC_SCER_PCK3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCER_PCK3_Msk instead */
#define PMC_SCER_PCK4_Pos                   12                                             /**< (PMC_SCER) Programmable Clock 4 Output Enable Position */
#define PMC_SCER_PCK4_Msk                   (0x1U << PMC_SCER_PCK4_Pos)                    /**< (PMC_SCER) Programmable Clock 4 Output Enable Mask */
#define PMC_SCER_PCK4                       PMC_SCER_PCK4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCER_PCK4_Msk instead */
#define PMC_SCER_PCK5_Pos                   13                                             /**< (PMC_SCER) Programmable Clock 5 Output Enable Position */
#define PMC_SCER_PCK5_Msk                   (0x1U << PMC_SCER_PCK5_Pos)                    /**< (PMC_SCER) Programmable Clock 5 Output Enable Mask */
#define PMC_SCER_PCK5                       PMC_SCER_PCK5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCER_PCK5_Msk instead */
#define PMC_SCER_PCK6_Pos                   14                                             /**< (PMC_SCER) Programmable Clock 6 Output Enable Position */
#define PMC_SCER_PCK6_Msk                   (0x1U << PMC_SCER_PCK6_Pos)                    /**< (PMC_SCER) Programmable Clock 6 Output Enable Mask */
#define PMC_SCER_PCK6                       PMC_SCER_PCK6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCER_PCK6_Msk instead */
#define PMC_SCER_PCK_Pos                    8                                              /**< (PMC_SCER Position) Programmable Clock 6 Output Enable */
#define PMC_SCER_PCK_Msk                    (0x7FU << PMC_SCER_PCK_Pos)                    /**< (PMC_SCER Mask) PCK */
#define PMC_SCER_PCK(value)                 (PMC_SCER_PCK_Msk & ((value) << PMC_SCER_PCK_Pos))  
#define PMC_SCER_MASK                       (0x7F20U)                                      /**< \deprecated (PMC_SCER) Register MASK  (Use PMC_SCER_Msk instead)  */
#define PMC_SCER_Msk                        (0x7F20U)                                      /**< (PMC_SCER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SCDR : (PMC Offset: 0x04) (/W 32) System Clock Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :5;                        /**< bit:   0..4  Reserved */
    uint32_t USBCLK:1;                  /**< bit:      5  Disable USB FS Clock                     */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t PCK0:1;                    /**< bit:      8  Programmable Clock 0 Output Disable      */
    uint32_t PCK1:1;                    /**< bit:      9  Programmable Clock 1 Output Disable      */
    uint32_t PCK2:1;                    /**< bit:     10  Programmable Clock 2 Output Disable      */
    uint32_t PCK3:1;                    /**< bit:     11  Programmable Clock 3 Output Disable      */
    uint32_t PCK4:1;                    /**< bit:     12  Programmable Clock 4 Output Disable      */
    uint32_t PCK5:1;                    /**< bit:     13  Programmable Clock 5 Output Disable      */
    uint32_t PCK6:1;                    /**< bit:     14  Programmable Clock 6 Output Disable      */
    uint32_t :17;                       /**< bit: 15..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :8;                        /**< bit:   0..7  Reserved */
    uint32_t PCK:7;                     /**< bit:  8..14  Programmable Clock 6 Output Disable      */
    uint32_t :17;                       /**< bit: 15..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SCDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SCDR_OFFSET                     (0x04)                                        /**<  (PMC_SCDR) System Clock Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SCDR_USBCLK_Pos                 5                                              /**< (PMC_SCDR) Disable USB FS Clock Position */
#define PMC_SCDR_USBCLK_Msk                 (0x1U << PMC_SCDR_USBCLK_Pos)                  /**< (PMC_SCDR) Disable USB FS Clock Mask */
#define PMC_SCDR_USBCLK                     PMC_SCDR_USBCLK_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCDR_USBCLK_Msk instead */
#define PMC_SCDR_PCK0_Pos                   8                                              /**< (PMC_SCDR) Programmable Clock 0 Output Disable Position */
#define PMC_SCDR_PCK0_Msk                   (0x1U << PMC_SCDR_PCK0_Pos)                    /**< (PMC_SCDR) Programmable Clock 0 Output Disable Mask */
#define PMC_SCDR_PCK0                       PMC_SCDR_PCK0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCDR_PCK0_Msk instead */
#define PMC_SCDR_PCK1_Pos                   9                                              /**< (PMC_SCDR) Programmable Clock 1 Output Disable Position */
#define PMC_SCDR_PCK1_Msk                   (0x1U << PMC_SCDR_PCK1_Pos)                    /**< (PMC_SCDR) Programmable Clock 1 Output Disable Mask */
#define PMC_SCDR_PCK1                       PMC_SCDR_PCK1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCDR_PCK1_Msk instead */
#define PMC_SCDR_PCK2_Pos                   10                                             /**< (PMC_SCDR) Programmable Clock 2 Output Disable Position */
#define PMC_SCDR_PCK2_Msk                   (0x1U << PMC_SCDR_PCK2_Pos)                    /**< (PMC_SCDR) Programmable Clock 2 Output Disable Mask */
#define PMC_SCDR_PCK2                       PMC_SCDR_PCK2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCDR_PCK2_Msk instead */
#define PMC_SCDR_PCK3_Pos                   11                                             /**< (PMC_SCDR) Programmable Clock 3 Output Disable Position */
#define PMC_SCDR_PCK3_Msk                   (0x1U << PMC_SCDR_PCK3_Pos)                    /**< (PMC_SCDR) Programmable Clock 3 Output Disable Mask */
#define PMC_SCDR_PCK3                       PMC_SCDR_PCK3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCDR_PCK3_Msk instead */
#define PMC_SCDR_PCK4_Pos                   12                                             /**< (PMC_SCDR) Programmable Clock 4 Output Disable Position */
#define PMC_SCDR_PCK4_Msk                   (0x1U << PMC_SCDR_PCK4_Pos)                    /**< (PMC_SCDR) Programmable Clock 4 Output Disable Mask */
#define PMC_SCDR_PCK4                       PMC_SCDR_PCK4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCDR_PCK4_Msk instead */
#define PMC_SCDR_PCK5_Pos                   13                                             /**< (PMC_SCDR) Programmable Clock 5 Output Disable Position */
#define PMC_SCDR_PCK5_Msk                   (0x1U << PMC_SCDR_PCK5_Pos)                    /**< (PMC_SCDR) Programmable Clock 5 Output Disable Mask */
#define PMC_SCDR_PCK5                       PMC_SCDR_PCK5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCDR_PCK5_Msk instead */
#define PMC_SCDR_PCK6_Pos                   14                                             /**< (PMC_SCDR) Programmable Clock 6 Output Disable Position */
#define PMC_SCDR_PCK6_Msk                   (0x1U << PMC_SCDR_PCK6_Pos)                    /**< (PMC_SCDR) Programmable Clock 6 Output Disable Mask */
#define PMC_SCDR_PCK6                       PMC_SCDR_PCK6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCDR_PCK6_Msk instead */
#define PMC_SCDR_PCK_Pos                    8                                              /**< (PMC_SCDR Position) Programmable Clock 6 Output Disable */
#define PMC_SCDR_PCK_Msk                    (0x7FU << PMC_SCDR_PCK_Pos)                    /**< (PMC_SCDR Mask) PCK */
#define PMC_SCDR_PCK(value)                 (PMC_SCDR_PCK_Msk & ((value) << PMC_SCDR_PCK_Pos))  
#define PMC_SCDR_MASK                       (0x7F20U)                                      /**< \deprecated (PMC_SCDR) Register MASK  (Use PMC_SCDR_Msk instead)  */
#define PMC_SCDR_Msk                        (0x7F20U)                                      /**< (PMC_SCDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SCSR : (PMC Offset: 0x08) (R/ 32) System Clock Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t HCLKS:1;                   /**< bit:      0  Processor Clock Status                   */
    uint32_t :4;                        /**< bit:   1..4  Reserved */
    uint32_t USBCLK:1;                  /**< bit:      5  USB FS Clock Status                      */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t PCK0:1;                    /**< bit:      8  Programmable Clock 0 Output Status       */
    uint32_t PCK1:1;                    /**< bit:      9  Programmable Clock 1 Output Status       */
    uint32_t PCK2:1;                    /**< bit:     10  Programmable Clock 2 Output Status       */
    uint32_t PCK3:1;                    /**< bit:     11  Programmable Clock 3 Output Status       */
    uint32_t PCK4:1;                    /**< bit:     12  Programmable Clock 4 Output Status       */
    uint32_t PCK5:1;                    /**< bit:     13  Programmable Clock 5 Output Status       */
    uint32_t PCK6:1;                    /**< bit:     14  Programmable Clock 6 Output Status       */
    uint32_t :17;                       /**< bit: 15..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :8;                        /**< bit:   0..7  Reserved */
    uint32_t PCK:7;                     /**< bit:  8..14  Programmable Clock 6 Output Status       */
    uint32_t :17;                       /**< bit: 15..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SCSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SCSR_OFFSET                     (0x08)                                        /**<  (PMC_SCSR) System Clock Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SCSR_HCLKS_Pos                  0                                              /**< (PMC_SCSR) Processor Clock Status Position */
#define PMC_SCSR_HCLKS_Msk                  (0x1U << PMC_SCSR_HCLKS_Pos)                   /**< (PMC_SCSR) Processor Clock Status Mask */
#define PMC_SCSR_HCLKS                      PMC_SCSR_HCLKS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_HCLKS_Msk instead */
#define PMC_SCSR_USBCLK_Pos                 5                                              /**< (PMC_SCSR) USB FS Clock Status Position */
#define PMC_SCSR_USBCLK_Msk                 (0x1U << PMC_SCSR_USBCLK_Pos)                  /**< (PMC_SCSR) USB FS Clock Status Mask */
#define PMC_SCSR_USBCLK                     PMC_SCSR_USBCLK_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_USBCLK_Msk instead */
#define PMC_SCSR_PCK0_Pos                   8                                              /**< (PMC_SCSR) Programmable Clock 0 Output Status Position */
#define PMC_SCSR_PCK0_Msk                   (0x1U << PMC_SCSR_PCK0_Pos)                    /**< (PMC_SCSR) Programmable Clock 0 Output Status Mask */
#define PMC_SCSR_PCK0                       PMC_SCSR_PCK0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_PCK0_Msk instead */
#define PMC_SCSR_PCK1_Pos                   9                                              /**< (PMC_SCSR) Programmable Clock 1 Output Status Position */
#define PMC_SCSR_PCK1_Msk                   (0x1U << PMC_SCSR_PCK1_Pos)                    /**< (PMC_SCSR) Programmable Clock 1 Output Status Mask */
#define PMC_SCSR_PCK1                       PMC_SCSR_PCK1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_PCK1_Msk instead */
#define PMC_SCSR_PCK2_Pos                   10                                             /**< (PMC_SCSR) Programmable Clock 2 Output Status Position */
#define PMC_SCSR_PCK2_Msk                   (0x1U << PMC_SCSR_PCK2_Pos)                    /**< (PMC_SCSR) Programmable Clock 2 Output Status Mask */
#define PMC_SCSR_PCK2                       PMC_SCSR_PCK2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_PCK2_Msk instead */
#define PMC_SCSR_PCK3_Pos                   11                                             /**< (PMC_SCSR) Programmable Clock 3 Output Status Position */
#define PMC_SCSR_PCK3_Msk                   (0x1U << PMC_SCSR_PCK3_Pos)                    /**< (PMC_SCSR) Programmable Clock 3 Output Status Mask */
#define PMC_SCSR_PCK3                       PMC_SCSR_PCK3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_PCK3_Msk instead */
#define PMC_SCSR_PCK4_Pos                   12                                             /**< (PMC_SCSR) Programmable Clock 4 Output Status Position */
#define PMC_SCSR_PCK4_Msk                   (0x1U << PMC_SCSR_PCK4_Pos)                    /**< (PMC_SCSR) Programmable Clock 4 Output Status Mask */
#define PMC_SCSR_PCK4                       PMC_SCSR_PCK4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_PCK4_Msk instead */
#define PMC_SCSR_PCK5_Pos                   13                                             /**< (PMC_SCSR) Programmable Clock 5 Output Status Position */
#define PMC_SCSR_PCK5_Msk                   (0x1U << PMC_SCSR_PCK5_Pos)                    /**< (PMC_SCSR) Programmable Clock 5 Output Status Mask */
#define PMC_SCSR_PCK5                       PMC_SCSR_PCK5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_PCK5_Msk instead */
#define PMC_SCSR_PCK6_Pos                   14                                             /**< (PMC_SCSR) Programmable Clock 6 Output Status Position */
#define PMC_SCSR_PCK6_Msk                   (0x1U << PMC_SCSR_PCK6_Pos)                    /**< (PMC_SCSR) Programmable Clock 6 Output Status Mask */
#define PMC_SCSR_PCK6                       PMC_SCSR_PCK6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SCSR_PCK6_Msk instead */
#define PMC_SCSR_PCK_Pos                    8                                              /**< (PMC_SCSR Position) Programmable Clock 6 Output Status */
#define PMC_SCSR_PCK_Msk                    (0x7FU << PMC_SCSR_PCK_Pos)                    /**< (PMC_SCSR Mask) PCK */
#define PMC_SCSR_PCK(value)                 (PMC_SCSR_PCK_Msk & ((value) << PMC_SCSR_PCK_Pos))  
#define PMC_SCSR_MASK                       (0x7F21U)                                      /**< \deprecated (PMC_SCSR) Register MASK  (Use PMC_SCSR_Msk instead)  */
#define PMC_SCSR_Msk                        (0x7F21U)                                      /**< (PMC_SCSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PCER0 : (PMC Offset: 0x10) (/W 32) Peripheral Clock Enable Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID7:1;                    /**< bit:      7  Peripheral Clock 7 Enable                */
    uint32_t PID8:1;                    /**< bit:      8  Peripheral Clock 8 Enable                */
    uint32_t :7;                        /**< bit:  9..15  Reserved */
    uint32_t PID16:1;                   /**< bit:     16  Peripheral Clock 16 Enable               */
    uint32_t PID17:1;                   /**< bit:     17  Peripheral Clock 17 Enable               */
    uint32_t PID18:1;                   /**< bit:     18  Peripheral Clock 18 Enable               */
    uint32_t PID19:1;                   /**< bit:     19  Peripheral Clock 19 Enable               */
    uint32_t PID20:1;                   /**< bit:     20  Peripheral Clock 20 Enable               */
    uint32_t PID21:1;                   /**< bit:     21  Peripheral Clock 21 Enable               */
    uint32_t PID22:1;                   /**< bit:     22  Peripheral Clock 22 Enable               */
    uint32_t PID23:1;                   /**< bit:     23  Peripheral Clock 23 Enable               */
    uint32_t PID24:1;                   /**< bit:     24  Peripheral Clock 24 Enable               */
    uint32_t PID25:1;                   /**< bit:     25  Peripheral Clock 25 Enable               */
    uint32_t PID26:1;                   /**< bit:     26  Peripheral Clock 26 Enable               */
    uint32_t PID27:1;                   /**< bit:     27  Peripheral Clock 27 Enable               */
    uint32_t PID28:1;                   /**< bit:     28  Peripheral Clock 28 Enable               */
    uint32_t PID29:1;                   /**< bit:     29  Peripheral Clock 29 Enable               */
    uint32_t PID30:1;                   /**< bit:     30  Peripheral Clock 30 Enable               */
    uint32_t PID31:1;                   /**< bit:     31  Peripheral Clock 31 Enable               */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID:18;                    /**< bit:  7..24  Peripheral Clock 3x Enable               */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PCER0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PCER0_OFFSET                    (0x10)                                        /**<  (PMC_PCER0) Peripheral Clock Enable Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PCER0_PID7_Pos                  7                                              /**< (PMC_PCER0) Peripheral Clock 7 Enable Position */
#define PMC_PCER0_PID7_Msk                  (0x1U << PMC_PCER0_PID7_Pos)                   /**< (PMC_PCER0) Peripheral Clock 7 Enable Mask */
#define PMC_PCER0_PID7                      PMC_PCER0_PID7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID7_Msk instead */
#define PMC_PCER0_PID8_Pos                  8                                              /**< (PMC_PCER0) Peripheral Clock 8 Enable Position */
#define PMC_PCER0_PID8_Msk                  (0x1U << PMC_PCER0_PID8_Pos)                   /**< (PMC_PCER0) Peripheral Clock 8 Enable Mask */
#define PMC_PCER0_PID8                      PMC_PCER0_PID8_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID8_Msk instead */
#define PMC_PCER0_PID16_Pos                 16                                             /**< (PMC_PCER0) Peripheral Clock 16 Enable Position */
#define PMC_PCER0_PID16_Msk                 (0x1U << PMC_PCER0_PID16_Pos)                  /**< (PMC_PCER0) Peripheral Clock 16 Enable Mask */
#define PMC_PCER0_PID16                     PMC_PCER0_PID16_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID16_Msk instead */
#define PMC_PCER0_PID17_Pos                 17                                             /**< (PMC_PCER0) Peripheral Clock 17 Enable Position */
#define PMC_PCER0_PID17_Msk                 (0x1U << PMC_PCER0_PID17_Pos)                  /**< (PMC_PCER0) Peripheral Clock 17 Enable Mask */
#define PMC_PCER0_PID17                     PMC_PCER0_PID17_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID17_Msk instead */
#define PMC_PCER0_PID18_Pos                 18                                             /**< (PMC_PCER0) Peripheral Clock 18 Enable Position */
#define PMC_PCER0_PID18_Msk                 (0x1U << PMC_PCER0_PID18_Pos)                  /**< (PMC_PCER0) Peripheral Clock 18 Enable Mask */
#define PMC_PCER0_PID18                     PMC_PCER0_PID18_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID18_Msk instead */
#define PMC_PCER0_PID19_Pos                 19                                             /**< (PMC_PCER0) Peripheral Clock 19 Enable Position */
#define PMC_PCER0_PID19_Msk                 (0x1U << PMC_PCER0_PID19_Pos)                  /**< (PMC_PCER0) Peripheral Clock 19 Enable Mask */
#define PMC_PCER0_PID19                     PMC_PCER0_PID19_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID19_Msk instead */
#define PMC_PCER0_PID20_Pos                 20                                             /**< (PMC_PCER0) Peripheral Clock 20 Enable Position */
#define PMC_PCER0_PID20_Msk                 (0x1U << PMC_PCER0_PID20_Pos)                  /**< (PMC_PCER0) Peripheral Clock 20 Enable Mask */
#define PMC_PCER0_PID20                     PMC_PCER0_PID20_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID20_Msk instead */
#define PMC_PCER0_PID21_Pos                 21                                             /**< (PMC_PCER0) Peripheral Clock 21 Enable Position */
#define PMC_PCER0_PID21_Msk                 (0x1U << PMC_PCER0_PID21_Pos)                  /**< (PMC_PCER0) Peripheral Clock 21 Enable Mask */
#define PMC_PCER0_PID21                     PMC_PCER0_PID21_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID21_Msk instead */
#define PMC_PCER0_PID22_Pos                 22                                             /**< (PMC_PCER0) Peripheral Clock 22 Enable Position */
#define PMC_PCER0_PID22_Msk                 (0x1U << PMC_PCER0_PID22_Pos)                  /**< (PMC_PCER0) Peripheral Clock 22 Enable Mask */
#define PMC_PCER0_PID22                     PMC_PCER0_PID22_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID22_Msk instead */
#define PMC_PCER0_PID23_Pos                 23                                             /**< (PMC_PCER0) Peripheral Clock 23 Enable Position */
#define PMC_PCER0_PID23_Msk                 (0x1U << PMC_PCER0_PID23_Pos)                  /**< (PMC_PCER0) Peripheral Clock 23 Enable Mask */
#define PMC_PCER0_PID23                     PMC_PCER0_PID23_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID23_Msk instead */
#define PMC_PCER0_PID24_Pos                 24                                             /**< (PMC_PCER0) Peripheral Clock 24 Enable Position */
#define PMC_PCER0_PID24_Msk                 (0x1U << PMC_PCER0_PID24_Pos)                  /**< (PMC_PCER0) Peripheral Clock 24 Enable Mask */
#define PMC_PCER0_PID24                     PMC_PCER0_PID24_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID24_Msk instead */
#define PMC_PCER0_PID25_Pos                 25                                             /**< (PMC_PCER0) Peripheral Clock 25 Enable Position */
#define PMC_PCER0_PID25_Msk                 (0x1U << PMC_PCER0_PID25_Pos)                  /**< (PMC_PCER0) Peripheral Clock 25 Enable Mask */
#define PMC_PCER0_PID25                     PMC_PCER0_PID25_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID25_Msk instead */
#define PMC_PCER0_PID26_Pos                 26                                             /**< (PMC_PCER0) Peripheral Clock 26 Enable Position */
#define PMC_PCER0_PID26_Msk                 (0x1U << PMC_PCER0_PID26_Pos)                  /**< (PMC_PCER0) Peripheral Clock 26 Enable Mask */
#define PMC_PCER0_PID26                     PMC_PCER0_PID26_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID26_Msk instead */
#define PMC_PCER0_PID27_Pos                 27                                             /**< (PMC_PCER0) Peripheral Clock 27 Enable Position */
#define PMC_PCER0_PID27_Msk                 (0x1U << PMC_PCER0_PID27_Pos)                  /**< (PMC_PCER0) Peripheral Clock 27 Enable Mask */
#define PMC_PCER0_PID27                     PMC_PCER0_PID27_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID27_Msk instead */
#define PMC_PCER0_PID28_Pos                 28                                             /**< (PMC_PCER0) Peripheral Clock 28 Enable Position */
#define PMC_PCER0_PID28_Msk                 (0x1U << PMC_PCER0_PID28_Pos)                  /**< (PMC_PCER0) Peripheral Clock 28 Enable Mask */
#define PMC_PCER0_PID28                     PMC_PCER0_PID28_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID28_Msk instead */
#define PMC_PCER0_PID29_Pos                 29                                             /**< (PMC_PCER0) Peripheral Clock 29 Enable Position */
#define PMC_PCER0_PID29_Msk                 (0x1U << PMC_PCER0_PID29_Pos)                  /**< (PMC_PCER0) Peripheral Clock 29 Enable Mask */
#define PMC_PCER0_PID29                     PMC_PCER0_PID29_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID29_Msk instead */
#define PMC_PCER0_PID30_Pos                 30                                             /**< (PMC_PCER0) Peripheral Clock 30 Enable Position */
#define PMC_PCER0_PID30_Msk                 (0x1U << PMC_PCER0_PID30_Pos)                  /**< (PMC_PCER0) Peripheral Clock 30 Enable Mask */
#define PMC_PCER0_PID30                     PMC_PCER0_PID30_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID30_Msk instead */
#define PMC_PCER0_PID31_Pos                 31                                             /**< (PMC_PCER0) Peripheral Clock 31 Enable Position */
#define PMC_PCER0_PID31_Msk                 (0x1U << PMC_PCER0_PID31_Pos)                  /**< (PMC_PCER0) Peripheral Clock 31 Enable Mask */
#define PMC_PCER0_PID31                     PMC_PCER0_PID31_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER0_PID31_Msk instead */
#define PMC_PCER0_PID_Pos                   7                                              /**< (PMC_PCER0 Position) Peripheral Clock 3x Enable */
#define PMC_PCER0_PID_Msk                   (0x3FFFFU << PMC_PCER0_PID_Pos)                /**< (PMC_PCER0 Mask) PID */
#define PMC_PCER0_PID(value)                (PMC_PCER0_PID_Msk & ((value) << PMC_PCER0_PID_Pos))  
#define PMC_PCER0_MASK                      (0xFFFF0180U)                                  /**< \deprecated (PMC_PCER0) Register MASK  (Use PMC_PCER0_Msk instead)  */
#define PMC_PCER0_Msk                       (0xFFFF0180U)                                  /**< (PMC_PCER0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PCDR0 : (PMC Offset: 0x14) (/W 32) Peripheral Clock Disable Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID7:1;                    /**< bit:      7  Peripheral Clock 7 Disable               */
    uint32_t PID8:1;                    /**< bit:      8  Peripheral Clock 8 Disable               */
    uint32_t :7;                        /**< bit:  9..15  Reserved */
    uint32_t PID16:1;                   /**< bit:     16  Peripheral Clock 16 Disable              */
    uint32_t PID17:1;                   /**< bit:     17  Peripheral Clock 17 Disable              */
    uint32_t PID18:1;                   /**< bit:     18  Peripheral Clock 18 Disable              */
    uint32_t PID19:1;                   /**< bit:     19  Peripheral Clock 19 Disable              */
    uint32_t PID20:1;                   /**< bit:     20  Peripheral Clock 20 Disable              */
    uint32_t PID21:1;                   /**< bit:     21  Peripheral Clock 21 Disable              */
    uint32_t PID22:1;                   /**< bit:     22  Peripheral Clock 22 Disable              */
    uint32_t PID23:1;                   /**< bit:     23  Peripheral Clock 23 Disable              */
    uint32_t PID24:1;                   /**< bit:     24  Peripheral Clock 24 Disable              */
    uint32_t PID25:1;                   /**< bit:     25  Peripheral Clock 25 Disable              */
    uint32_t PID26:1;                   /**< bit:     26  Peripheral Clock 26 Disable              */
    uint32_t PID27:1;                   /**< bit:     27  Peripheral Clock 27 Disable              */
    uint32_t PID28:1;                   /**< bit:     28  Peripheral Clock 28 Disable              */
    uint32_t PID29:1;                   /**< bit:     29  Peripheral Clock 29 Disable              */
    uint32_t PID30:1;                   /**< bit:     30  Peripheral Clock 30 Disable              */
    uint32_t PID31:1;                   /**< bit:     31  Peripheral Clock 31 Disable              */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID:18;                    /**< bit:  7..24  Peripheral Clock 3x Disable              */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PCDR0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PCDR0_OFFSET                    (0x14)                                        /**<  (PMC_PCDR0) Peripheral Clock Disable Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PCDR0_PID7_Pos                  7                                              /**< (PMC_PCDR0) Peripheral Clock 7 Disable Position */
#define PMC_PCDR0_PID7_Msk                  (0x1U << PMC_PCDR0_PID7_Pos)                   /**< (PMC_PCDR0) Peripheral Clock 7 Disable Mask */
#define PMC_PCDR0_PID7                      PMC_PCDR0_PID7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID7_Msk instead */
#define PMC_PCDR0_PID8_Pos                  8                                              /**< (PMC_PCDR0) Peripheral Clock 8 Disable Position */
#define PMC_PCDR0_PID8_Msk                  (0x1U << PMC_PCDR0_PID8_Pos)                   /**< (PMC_PCDR0) Peripheral Clock 8 Disable Mask */
#define PMC_PCDR0_PID8                      PMC_PCDR0_PID8_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID8_Msk instead */
#define PMC_PCDR0_PID16_Pos                 16                                             /**< (PMC_PCDR0) Peripheral Clock 16 Disable Position */
#define PMC_PCDR0_PID16_Msk                 (0x1U << PMC_PCDR0_PID16_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 16 Disable Mask */
#define PMC_PCDR0_PID16                     PMC_PCDR0_PID16_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID16_Msk instead */
#define PMC_PCDR0_PID17_Pos                 17                                             /**< (PMC_PCDR0) Peripheral Clock 17 Disable Position */
#define PMC_PCDR0_PID17_Msk                 (0x1U << PMC_PCDR0_PID17_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 17 Disable Mask */
#define PMC_PCDR0_PID17                     PMC_PCDR0_PID17_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID17_Msk instead */
#define PMC_PCDR0_PID18_Pos                 18                                             /**< (PMC_PCDR0) Peripheral Clock 18 Disable Position */
#define PMC_PCDR0_PID18_Msk                 (0x1U << PMC_PCDR0_PID18_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 18 Disable Mask */
#define PMC_PCDR0_PID18                     PMC_PCDR0_PID18_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID18_Msk instead */
#define PMC_PCDR0_PID19_Pos                 19                                             /**< (PMC_PCDR0) Peripheral Clock 19 Disable Position */
#define PMC_PCDR0_PID19_Msk                 (0x1U << PMC_PCDR0_PID19_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 19 Disable Mask */
#define PMC_PCDR0_PID19                     PMC_PCDR0_PID19_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID19_Msk instead */
#define PMC_PCDR0_PID20_Pos                 20                                             /**< (PMC_PCDR0) Peripheral Clock 20 Disable Position */
#define PMC_PCDR0_PID20_Msk                 (0x1U << PMC_PCDR0_PID20_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 20 Disable Mask */
#define PMC_PCDR0_PID20                     PMC_PCDR0_PID20_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID20_Msk instead */
#define PMC_PCDR0_PID21_Pos                 21                                             /**< (PMC_PCDR0) Peripheral Clock 21 Disable Position */
#define PMC_PCDR0_PID21_Msk                 (0x1U << PMC_PCDR0_PID21_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 21 Disable Mask */
#define PMC_PCDR0_PID21                     PMC_PCDR0_PID21_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID21_Msk instead */
#define PMC_PCDR0_PID22_Pos                 22                                             /**< (PMC_PCDR0) Peripheral Clock 22 Disable Position */
#define PMC_PCDR0_PID22_Msk                 (0x1U << PMC_PCDR0_PID22_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 22 Disable Mask */
#define PMC_PCDR0_PID22                     PMC_PCDR0_PID22_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID22_Msk instead */
#define PMC_PCDR0_PID23_Pos                 23                                             /**< (PMC_PCDR0) Peripheral Clock 23 Disable Position */
#define PMC_PCDR0_PID23_Msk                 (0x1U << PMC_PCDR0_PID23_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 23 Disable Mask */
#define PMC_PCDR0_PID23                     PMC_PCDR0_PID23_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID23_Msk instead */
#define PMC_PCDR0_PID24_Pos                 24                                             /**< (PMC_PCDR0) Peripheral Clock 24 Disable Position */
#define PMC_PCDR0_PID24_Msk                 (0x1U << PMC_PCDR0_PID24_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 24 Disable Mask */
#define PMC_PCDR0_PID24                     PMC_PCDR0_PID24_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID24_Msk instead */
#define PMC_PCDR0_PID25_Pos                 25                                             /**< (PMC_PCDR0) Peripheral Clock 25 Disable Position */
#define PMC_PCDR0_PID25_Msk                 (0x1U << PMC_PCDR0_PID25_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 25 Disable Mask */
#define PMC_PCDR0_PID25                     PMC_PCDR0_PID25_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID25_Msk instead */
#define PMC_PCDR0_PID26_Pos                 26                                             /**< (PMC_PCDR0) Peripheral Clock 26 Disable Position */
#define PMC_PCDR0_PID26_Msk                 (0x1U << PMC_PCDR0_PID26_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 26 Disable Mask */
#define PMC_PCDR0_PID26                     PMC_PCDR0_PID26_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID26_Msk instead */
#define PMC_PCDR0_PID27_Pos                 27                                             /**< (PMC_PCDR0) Peripheral Clock 27 Disable Position */
#define PMC_PCDR0_PID27_Msk                 (0x1U << PMC_PCDR0_PID27_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 27 Disable Mask */
#define PMC_PCDR0_PID27                     PMC_PCDR0_PID27_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID27_Msk instead */
#define PMC_PCDR0_PID28_Pos                 28                                             /**< (PMC_PCDR0) Peripheral Clock 28 Disable Position */
#define PMC_PCDR0_PID28_Msk                 (0x1U << PMC_PCDR0_PID28_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 28 Disable Mask */
#define PMC_PCDR0_PID28                     PMC_PCDR0_PID28_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID28_Msk instead */
#define PMC_PCDR0_PID29_Pos                 29                                             /**< (PMC_PCDR0) Peripheral Clock 29 Disable Position */
#define PMC_PCDR0_PID29_Msk                 (0x1U << PMC_PCDR0_PID29_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 29 Disable Mask */
#define PMC_PCDR0_PID29                     PMC_PCDR0_PID29_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID29_Msk instead */
#define PMC_PCDR0_PID30_Pos                 30                                             /**< (PMC_PCDR0) Peripheral Clock 30 Disable Position */
#define PMC_PCDR0_PID30_Msk                 (0x1U << PMC_PCDR0_PID30_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 30 Disable Mask */
#define PMC_PCDR0_PID30                     PMC_PCDR0_PID30_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID30_Msk instead */
#define PMC_PCDR0_PID31_Pos                 31                                             /**< (PMC_PCDR0) Peripheral Clock 31 Disable Position */
#define PMC_PCDR0_PID31_Msk                 (0x1U << PMC_PCDR0_PID31_Pos)                  /**< (PMC_PCDR0) Peripheral Clock 31 Disable Mask */
#define PMC_PCDR0_PID31                     PMC_PCDR0_PID31_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR0_PID31_Msk instead */
#define PMC_PCDR0_PID_Pos                   7                                              /**< (PMC_PCDR0 Position) Peripheral Clock 3x Disable */
#define PMC_PCDR0_PID_Msk                   (0x3FFFFU << PMC_PCDR0_PID_Pos)                /**< (PMC_PCDR0 Mask) PID */
#define PMC_PCDR0_PID(value)                (PMC_PCDR0_PID_Msk & ((value) << PMC_PCDR0_PID_Pos))  
#define PMC_PCDR0_MASK                      (0xFFFF0180U)                                  /**< \deprecated (PMC_PCDR0) Register MASK  (Use PMC_PCDR0_Msk instead)  */
#define PMC_PCDR0_Msk                       (0xFFFF0180U)                                  /**< (PMC_PCDR0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PCSR0 : (PMC Offset: 0x18) (R/ 32) Peripheral Clock Status Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID7:1;                    /**< bit:      7  Peripheral Clock 7 Status                */
    uint32_t PID8:1;                    /**< bit:      8  Peripheral Clock 8 Status                */
    uint32_t :7;                        /**< bit:  9..15  Reserved */
    uint32_t PID16:1;                   /**< bit:     16  Peripheral Clock 16 Status               */
    uint32_t PID17:1;                   /**< bit:     17  Peripheral Clock 17 Status               */
    uint32_t PID18:1;                   /**< bit:     18  Peripheral Clock 18 Status               */
    uint32_t PID19:1;                   /**< bit:     19  Peripheral Clock 19 Status               */
    uint32_t PID20:1;                   /**< bit:     20  Peripheral Clock 20 Status               */
    uint32_t PID21:1;                   /**< bit:     21  Peripheral Clock 21 Status               */
    uint32_t PID22:1;                   /**< bit:     22  Peripheral Clock 22 Status               */
    uint32_t PID23:1;                   /**< bit:     23  Peripheral Clock 23 Status               */
    uint32_t PID24:1;                   /**< bit:     24  Peripheral Clock 24 Status               */
    uint32_t PID25:1;                   /**< bit:     25  Peripheral Clock 25 Status               */
    uint32_t PID26:1;                   /**< bit:     26  Peripheral Clock 26 Status               */
    uint32_t PID27:1;                   /**< bit:     27  Peripheral Clock 27 Status               */
    uint32_t PID28:1;                   /**< bit:     28  Peripheral Clock 28 Status               */
    uint32_t PID29:1;                   /**< bit:     29  Peripheral Clock 29 Status               */
    uint32_t PID30:1;                   /**< bit:     30  Peripheral Clock 30 Status               */
    uint32_t PID31:1;                   /**< bit:     31  Peripheral Clock 31 Status               */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID:18;                    /**< bit:  7..24  Peripheral Clock 3x Status               */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PCSR0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PCSR0_OFFSET                    (0x18)                                        /**<  (PMC_PCSR0) Peripheral Clock Status Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PCSR0_PID7_Pos                  7                                              /**< (PMC_PCSR0) Peripheral Clock 7 Status Position */
#define PMC_PCSR0_PID7_Msk                  (0x1U << PMC_PCSR0_PID7_Pos)                   /**< (PMC_PCSR0) Peripheral Clock 7 Status Mask */
#define PMC_PCSR0_PID7                      PMC_PCSR0_PID7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID7_Msk instead */
#define PMC_PCSR0_PID8_Pos                  8                                              /**< (PMC_PCSR0) Peripheral Clock 8 Status Position */
#define PMC_PCSR0_PID8_Msk                  (0x1U << PMC_PCSR0_PID8_Pos)                   /**< (PMC_PCSR0) Peripheral Clock 8 Status Mask */
#define PMC_PCSR0_PID8                      PMC_PCSR0_PID8_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID8_Msk instead */
#define PMC_PCSR0_PID16_Pos                 16                                             /**< (PMC_PCSR0) Peripheral Clock 16 Status Position */
#define PMC_PCSR0_PID16_Msk                 (0x1U << PMC_PCSR0_PID16_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 16 Status Mask */
#define PMC_PCSR0_PID16                     PMC_PCSR0_PID16_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID16_Msk instead */
#define PMC_PCSR0_PID17_Pos                 17                                             /**< (PMC_PCSR0) Peripheral Clock 17 Status Position */
#define PMC_PCSR0_PID17_Msk                 (0x1U << PMC_PCSR0_PID17_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 17 Status Mask */
#define PMC_PCSR0_PID17                     PMC_PCSR0_PID17_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID17_Msk instead */
#define PMC_PCSR0_PID18_Pos                 18                                             /**< (PMC_PCSR0) Peripheral Clock 18 Status Position */
#define PMC_PCSR0_PID18_Msk                 (0x1U << PMC_PCSR0_PID18_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 18 Status Mask */
#define PMC_PCSR0_PID18                     PMC_PCSR0_PID18_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID18_Msk instead */
#define PMC_PCSR0_PID19_Pos                 19                                             /**< (PMC_PCSR0) Peripheral Clock 19 Status Position */
#define PMC_PCSR0_PID19_Msk                 (0x1U << PMC_PCSR0_PID19_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 19 Status Mask */
#define PMC_PCSR0_PID19                     PMC_PCSR0_PID19_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID19_Msk instead */
#define PMC_PCSR0_PID20_Pos                 20                                             /**< (PMC_PCSR0) Peripheral Clock 20 Status Position */
#define PMC_PCSR0_PID20_Msk                 (0x1U << PMC_PCSR0_PID20_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 20 Status Mask */
#define PMC_PCSR0_PID20                     PMC_PCSR0_PID20_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID20_Msk instead */
#define PMC_PCSR0_PID21_Pos                 21                                             /**< (PMC_PCSR0) Peripheral Clock 21 Status Position */
#define PMC_PCSR0_PID21_Msk                 (0x1U << PMC_PCSR0_PID21_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 21 Status Mask */
#define PMC_PCSR0_PID21                     PMC_PCSR0_PID21_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID21_Msk instead */
#define PMC_PCSR0_PID22_Pos                 22                                             /**< (PMC_PCSR0) Peripheral Clock 22 Status Position */
#define PMC_PCSR0_PID22_Msk                 (0x1U << PMC_PCSR0_PID22_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 22 Status Mask */
#define PMC_PCSR0_PID22                     PMC_PCSR0_PID22_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID22_Msk instead */
#define PMC_PCSR0_PID23_Pos                 23                                             /**< (PMC_PCSR0) Peripheral Clock 23 Status Position */
#define PMC_PCSR0_PID23_Msk                 (0x1U << PMC_PCSR0_PID23_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 23 Status Mask */
#define PMC_PCSR0_PID23                     PMC_PCSR0_PID23_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID23_Msk instead */
#define PMC_PCSR0_PID24_Pos                 24                                             /**< (PMC_PCSR0) Peripheral Clock 24 Status Position */
#define PMC_PCSR0_PID24_Msk                 (0x1U << PMC_PCSR0_PID24_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 24 Status Mask */
#define PMC_PCSR0_PID24                     PMC_PCSR0_PID24_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID24_Msk instead */
#define PMC_PCSR0_PID25_Pos                 25                                             /**< (PMC_PCSR0) Peripheral Clock 25 Status Position */
#define PMC_PCSR0_PID25_Msk                 (0x1U << PMC_PCSR0_PID25_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 25 Status Mask */
#define PMC_PCSR0_PID25                     PMC_PCSR0_PID25_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID25_Msk instead */
#define PMC_PCSR0_PID26_Pos                 26                                             /**< (PMC_PCSR0) Peripheral Clock 26 Status Position */
#define PMC_PCSR0_PID26_Msk                 (0x1U << PMC_PCSR0_PID26_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 26 Status Mask */
#define PMC_PCSR0_PID26                     PMC_PCSR0_PID26_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID26_Msk instead */
#define PMC_PCSR0_PID27_Pos                 27                                             /**< (PMC_PCSR0) Peripheral Clock 27 Status Position */
#define PMC_PCSR0_PID27_Msk                 (0x1U << PMC_PCSR0_PID27_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 27 Status Mask */
#define PMC_PCSR0_PID27                     PMC_PCSR0_PID27_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID27_Msk instead */
#define PMC_PCSR0_PID28_Pos                 28                                             /**< (PMC_PCSR0) Peripheral Clock 28 Status Position */
#define PMC_PCSR0_PID28_Msk                 (0x1U << PMC_PCSR0_PID28_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 28 Status Mask */
#define PMC_PCSR0_PID28                     PMC_PCSR0_PID28_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID28_Msk instead */
#define PMC_PCSR0_PID29_Pos                 29                                             /**< (PMC_PCSR0) Peripheral Clock 29 Status Position */
#define PMC_PCSR0_PID29_Msk                 (0x1U << PMC_PCSR0_PID29_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 29 Status Mask */
#define PMC_PCSR0_PID29                     PMC_PCSR0_PID29_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID29_Msk instead */
#define PMC_PCSR0_PID30_Pos                 30                                             /**< (PMC_PCSR0) Peripheral Clock 30 Status Position */
#define PMC_PCSR0_PID30_Msk                 (0x1U << PMC_PCSR0_PID30_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 30 Status Mask */
#define PMC_PCSR0_PID30                     PMC_PCSR0_PID30_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID30_Msk instead */
#define PMC_PCSR0_PID31_Pos                 31                                             /**< (PMC_PCSR0) Peripheral Clock 31 Status Position */
#define PMC_PCSR0_PID31_Msk                 (0x1U << PMC_PCSR0_PID31_Pos)                  /**< (PMC_PCSR0) Peripheral Clock 31 Status Mask */
#define PMC_PCSR0_PID31                     PMC_PCSR0_PID31_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR0_PID31_Msk instead */
#define PMC_PCSR0_PID_Pos                   7                                              /**< (PMC_PCSR0 Position) Peripheral Clock 3x Status */
#define PMC_PCSR0_PID_Msk                   (0x3FFFFU << PMC_PCSR0_PID_Pos)                /**< (PMC_PCSR0 Mask) PID */
#define PMC_PCSR0_PID(value)                (PMC_PCSR0_PID_Msk & ((value) << PMC_PCSR0_PID_Pos))  
#define PMC_PCSR0_MASK                      (0xFFFF0180U)                                  /**< \deprecated (PMC_PCSR0) Register MASK  (Use PMC_PCSR0_Msk instead)  */
#define PMC_PCSR0_Msk                       (0xFFFF0180U)                                  /**< (PMC_PCSR0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- CKGR_UCKR : (PMC Offset: 0x1c) (R/W 32) UTMI Clock Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :16;                       /**< bit:  0..15  Reserved */
    uint32_t UPLLEN:1;                  /**< bit:     16  UTMI PLL Enable                          */
    uint32_t :3;                        /**< bit: 17..19  Reserved */
    uint32_t UPLLCOUNT:4;               /**< bit: 20..23  UTMI PLL Start-up Time                   */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} CKGR_UCKR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define CKGR_UCKR_OFFSET                    (0x1C)                                        /**<  (CKGR_UCKR) UTMI Clock Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define CKGR_UCKR_UPLLEN_Pos                16                                             /**< (CKGR_UCKR) UTMI PLL Enable Position */
#define CKGR_UCKR_UPLLEN_Msk                (0x1U << CKGR_UCKR_UPLLEN_Pos)                 /**< (CKGR_UCKR) UTMI PLL Enable Mask */
#define CKGR_UCKR_UPLLEN                    CKGR_UCKR_UPLLEN_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_UCKR_UPLLEN_Msk instead */
#define CKGR_UCKR_UPLLCOUNT_Pos             20                                             /**< (CKGR_UCKR) UTMI PLL Start-up Time Position */
#define CKGR_UCKR_UPLLCOUNT_Msk             (0xFU << CKGR_UCKR_UPLLCOUNT_Pos)              /**< (CKGR_UCKR) UTMI PLL Start-up Time Mask */
#define CKGR_UCKR_UPLLCOUNT(value)          (CKGR_UCKR_UPLLCOUNT_Msk & ((value) << CKGR_UCKR_UPLLCOUNT_Pos))
#define CKGR_UCKR_MASK                      (0xF10000U)                                    /**< \deprecated (CKGR_UCKR) Register MASK  (Use CKGR_UCKR_Msk instead)  */
#define CKGR_UCKR_Msk                       (0xF10000U)                                    /**< (CKGR_UCKR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- CKGR_MOR : (PMC Offset: 0x20) (R/W 32) Main Oscillator Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MOSCXTEN:1;                /**< bit:      0  3 to 20 MHz Crystal Oscillator Enable    */
    uint32_t MOSCXTBY:1;                /**< bit:      1  3 to 20 MHz Crystal Oscillator Bypass    */
    uint32_t WAITMODE:1;                /**< bit:      2  Wait Mode Command (Write-only)           */
    uint32_t MOSCRCEN:1;                /**< bit:      3  4/8/12 MHz On-Chip RC Oscillator Enable  */
    uint32_t MOSCRCF:3;                 /**< bit:   4..6  4/8/12 MHz RC Oscillator Frequency Selection */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t MOSCXTST:8;                /**< bit:  8..15  3 to 20 MHz Crystal Oscillator Start-up Time */
    uint32_t KEY:8;                     /**< bit: 16..23  Write Access Password                    */
    uint32_t MOSCSEL:1;                 /**< bit:     24  Main Clock Oscillator Selection          */
    uint32_t CFDEN:1;                   /**< bit:     25  Clock Failure Detector Enable            */
    uint32_t XT32KFME:1;                /**< bit:     26  32.768 kHz Crystal Oscillator Frequency Monitoring Enable */
    uint32_t :5;                        /**< bit: 27..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} CKGR_MOR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define CKGR_MOR_OFFSET                     (0x20)                                        /**<  (CKGR_MOR) Main Oscillator Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define CKGR_MOR_MOSCXTEN_Pos               0                                              /**< (CKGR_MOR) 3 to 20 MHz Crystal Oscillator Enable Position */
#define CKGR_MOR_MOSCXTEN_Msk               (0x1U << CKGR_MOR_MOSCXTEN_Pos)                /**< (CKGR_MOR) 3 to 20 MHz Crystal Oscillator Enable Mask */
#define CKGR_MOR_MOSCXTEN                   CKGR_MOR_MOSCXTEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MOR_MOSCXTEN_Msk instead */
#define CKGR_MOR_MOSCXTBY_Pos               1                                              /**< (CKGR_MOR) 3 to 20 MHz Crystal Oscillator Bypass Position */
#define CKGR_MOR_MOSCXTBY_Msk               (0x1U << CKGR_MOR_MOSCXTBY_Pos)                /**< (CKGR_MOR) 3 to 20 MHz Crystal Oscillator Bypass Mask */
#define CKGR_MOR_MOSCXTBY                   CKGR_MOR_MOSCXTBY_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MOR_MOSCXTBY_Msk instead */
#define CKGR_MOR_WAITMODE_Pos               2                                              /**< (CKGR_MOR) Wait Mode Command (Write-only) Position */
#define CKGR_MOR_WAITMODE_Msk               (0x1U << CKGR_MOR_WAITMODE_Pos)                /**< (CKGR_MOR) Wait Mode Command (Write-only) Mask */
#define CKGR_MOR_WAITMODE                   CKGR_MOR_WAITMODE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MOR_WAITMODE_Msk instead */
#define CKGR_MOR_MOSCRCEN_Pos               3                                              /**< (CKGR_MOR) 4/8/12 MHz On-Chip RC Oscillator Enable Position */
#define CKGR_MOR_MOSCRCEN_Msk               (0x1U << CKGR_MOR_MOSCRCEN_Pos)                /**< (CKGR_MOR) 4/8/12 MHz On-Chip RC Oscillator Enable Mask */
#define CKGR_MOR_MOSCRCEN                   CKGR_MOR_MOSCRCEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MOR_MOSCRCEN_Msk instead */
#define CKGR_MOR_MOSCRCF_Pos                4                                              /**< (CKGR_MOR) 4/8/12 MHz RC Oscillator Frequency Selection Position */
#define CKGR_MOR_MOSCRCF_Msk                (0x7U << CKGR_MOR_MOSCRCF_Pos)                 /**< (CKGR_MOR) 4/8/12 MHz RC Oscillator Frequency Selection Mask */
#define CKGR_MOR_MOSCRCF(value)             (CKGR_MOR_MOSCRCF_Msk & ((value) << CKGR_MOR_MOSCRCF_Pos))
#define   CKGR_MOR_MOSCRCF_4_MHz_Val        (0x0U)                                         /**< (CKGR_MOR) The RC oscillator frequency is at 4 MHz (default)  */
#define   CKGR_MOR_MOSCRCF_8_MHz_Val        (0x1U)                                         /**< (CKGR_MOR) The RC oscillator frequency is at 8 MHz  */
#define   CKGR_MOR_MOSCRCF_12_MHz_Val       (0x2U)                                         /**< (CKGR_MOR) The RC oscillator frequency is at 12 MHz  */
#define CKGR_MOR_MOSCRCF_4_MHz              (CKGR_MOR_MOSCRCF_4_MHz_Val << CKGR_MOR_MOSCRCF_Pos)  /**< (CKGR_MOR) The RC oscillator frequency is at 4 MHz (default) Position  */
#define CKGR_MOR_MOSCRCF_8_MHz              (CKGR_MOR_MOSCRCF_8_MHz_Val << CKGR_MOR_MOSCRCF_Pos)  /**< (CKGR_MOR) The RC oscillator frequency is at 8 MHz Position  */
#define CKGR_MOR_MOSCRCF_12_MHz             (CKGR_MOR_MOSCRCF_12_MHz_Val << CKGR_MOR_MOSCRCF_Pos)  /**< (CKGR_MOR) The RC oscillator frequency is at 12 MHz Position  */
#define CKGR_MOR_MOSCXTST_Pos               8                                              /**< (CKGR_MOR) 3 to 20 MHz Crystal Oscillator Start-up Time Position */
#define CKGR_MOR_MOSCXTST_Msk               (0xFFU << CKGR_MOR_MOSCXTST_Pos)               /**< (CKGR_MOR) 3 to 20 MHz Crystal Oscillator Start-up Time Mask */
#define CKGR_MOR_MOSCXTST(value)            (CKGR_MOR_MOSCXTST_Msk & ((value) << CKGR_MOR_MOSCXTST_Pos))
#define CKGR_MOR_KEY_Pos                    16                                             /**< (CKGR_MOR) Write Access Password Position */
#define CKGR_MOR_KEY_Msk                    (0xFFU << CKGR_MOR_KEY_Pos)                    /**< (CKGR_MOR) Write Access Password Mask */
#define CKGR_MOR_KEY(value)                 (CKGR_MOR_KEY_Msk & ((value) << CKGR_MOR_KEY_Pos))
#define   CKGR_MOR_KEY_PASSWD_Val           (0x37U)                                        /**< (CKGR_MOR) Writing any other value in this field aborts the write operation.Always reads as 0.  */
#define CKGR_MOR_KEY_PASSWD                 (CKGR_MOR_KEY_PASSWD_Val << CKGR_MOR_KEY_Pos)  /**< (CKGR_MOR) Writing any other value in this field aborts the write operation.Always reads as 0. Position  */
#define CKGR_MOR_MOSCSEL_Pos                24                                             /**< (CKGR_MOR) Main Clock Oscillator Selection Position */
#define CKGR_MOR_MOSCSEL_Msk                (0x1U << CKGR_MOR_MOSCSEL_Pos)                 /**< (CKGR_MOR) Main Clock Oscillator Selection Mask */
#define CKGR_MOR_MOSCSEL                    CKGR_MOR_MOSCSEL_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MOR_MOSCSEL_Msk instead */
#define CKGR_MOR_CFDEN_Pos                  25                                             /**< (CKGR_MOR) Clock Failure Detector Enable Position */
#define CKGR_MOR_CFDEN_Msk                  (0x1U << CKGR_MOR_CFDEN_Pos)                   /**< (CKGR_MOR) Clock Failure Detector Enable Mask */
#define CKGR_MOR_CFDEN                      CKGR_MOR_CFDEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MOR_CFDEN_Msk instead */
#define CKGR_MOR_XT32KFME_Pos               26                                             /**< (CKGR_MOR) 32.768 kHz Crystal Oscillator Frequency Monitoring Enable Position */
#define CKGR_MOR_XT32KFME_Msk               (0x1U << CKGR_MOR_XT32KFME_Pos)                /**< (CKGR_MOR) 32.768 kHz Crystal Oscillator Frequency Monitoring Enable Mask */
#define CKGR_MOR_XT32KFME                   CKGR_MOR_XT32KFME_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MOR_XT32KFME_Msk instead */
#define CKGR_MOR_MASK                       (0x7FFFF7FU)                                   /**< \deprecated (CKGR_MOR) Register MASK  (Use CKGR_MOR_Msk instead)  */
#define CKGR_MOR_Msk                        (0x7FFFF7FU)                                   /**< (CKGR_MOR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- CKGR_MCFR : (PMC Offset: 0x24) (R/W 32) Main Clock Frequency Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MAINF:16;                  /**< bit:  0..15  Main Clock Frequency                     */
    uint32_t MAINFRDY:1;                /**< bit:     16  Main Clock Frequency Measure Ready       */
    uint32_t :3;                        /**< bit: 17..19  Reserved */
    uint32_t RCMEAS:1;                  /**< bit:     20  RC Oscillator Frequency Measure (write-only) */
    uint32_t :3;                        /**< bit: 21..23  Reserved */
    uint32_t CCSS:1;                    /**< bit:     24  Counter Clock Source Selection           */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} CKGR_MCFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define CKGR_MCFR_OFFSET                    (0x24)                                        /**<  (CKGR_MCFR) Main Clock Frequency Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define CKGR_MCFR_MAINF_Pos                 0                                              /**< (CKGR_MCFR) Main Clock Frequency Position */
#define CKGR_MCFR_MAINF_Msk                 (0xFFFFU << CKGR_MCFR_MAINF_Pos)               /**< (CKGR_MCFR) Main Clock Frequency Mask */
#define CKGR_MCFR_MAINF(value)              (CKGR_MCFR_MAINF_Msk & ((value) << CKGR_MCFR_MAINF_Pos))
#define CKGR_MCFR_MAINFRDY_Pos              16                                             /**< (CKGR_MCFR) Main Clock Frequency Measure Ready Position */
#define CKGR_MCFR_MAINFRDY_Msk              (0x1U << CKGR_MCFR_MAINFRDY_Pos)               /**< (CKGR_MCFR) Main Clock Frequency Measure Ready Mask */
#define CKGR_MCFR_MAINFRDY                  CKGR_MCFR_MAINFRDY_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MCFR_MAINFRDY_Msk instead */
#define CKGR_MCFR_RCMEAS_Pos                20                                             /**< (CKGR_MCFR) RC Oscillator Frequency Measure (write-only) Position */
#define CKGR_MCFR_RCMEAS_Msk                (0x1U << CKGR_MCFR_RCMEAS_Pos)                 /**< (CKGR_MCFR) RC Oscillator Frequency Measure (write-only) Mask */
#define CKGR_MCFR_RCMEAS                    CKGR_MCFR_RCMEAS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MCFR_RCMEAS_Msk instead */
#define CKGR_MCFR_CCSS_Pos                  24                                             /**< (CKGR_MCFR) Counter Clock Source Selection Position */
#define CKGR_MCFR_CCSS_Msk                  (0x1U << CKGR_MCFR_CCSS_Pos)                   /**< (CKGR_MCFR) Counter Clock Source Selection Mask */
#define CKGR_MCFR_CCSS                      CKGR_MCFR_CCSS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_MCFR_CCSS_Msk instead */
#define CKGR_MCFR_MASK                      (0x111FFFFU)                                   /**< \deprecated (CKGR_MCFR) Register MASK  (Use CKGR_MCFR_Msk instead)  */
#define CKGR_MCFR_Msk                       (0x111FFFFU)                                   /**< (CKGR_MCFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- CKGR_PLLAR : (PMC Offset: 0x28) (R/W 32) PLLA Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DIVA:8;                    /**< bit:   0..7  PLLA Front End Divider                   */
    uint32_t PLLACOUNT:6;               /**< bit:  8..13  PLLA Counter                             */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t MULA:11;                   /**< bit: 16..26  PLLA Multiplier                          */
    uint32_t :2;                        /**< bit: 27..28  Reserved */
    uint32_t ONE:1;                     /**< bit:     29  Must Be Set to 1                         */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} CKGR_PLLAR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define CKGR_PLLAR_OFFSET                   (0x28)                                        /**<  (CKGR_PLLAR) PLLA Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define CKGR_PLLAR_DIVA_Pos                 0                                              /**< (CKGR_PLLAR) PLLA Front End Divider Position */
#define CKGR_PLLAR_DIVA_Msk                 (0xFFU << CKGR_PLLAR_DIVA_Pos)                 /**< (CKGR_PLLAR) PLLA Front End Divider Mask */
#define CKGR_PLLAR_DIVA(value)              (CKGR_PLLAR_DIVA_Msk & ((value) << CKGR_PLLAR_DIVA_Pos))
#define   CKGR_PLLAR_DIVA_0_Val             (0x0U)                                         /**< (CKGR_PLLAR) Divider output is 0 and PLLA is disabled.  */
#define   CKGR_PLLAR_DIVA_BYPASS_Val        (0x1U)                                         /**< (CKGR_PLLAR) Divider is bypassed (divide by 1) and PLLA is enabled.  */
#define CKGR_PLLAR_DIVA_0                   (CKGR_PLLAR_DIVA_0_Val << CKGR_PLLAR_DIVA_Pos)  /**< (CKGR_PLLAR) Divider output is 0 and PLLA is disabled. Position  */
#define CKGR_PLLAR_DIVA_BYPASS              (CKGR_PLLAR_DIVA_BYPASS_Val << CKGR_PLLAR_DIVA_Pos)  /**< (CKGR_PLLAR) Divider is bypassed (divide by 1) and PLLA is enabled. Position  */
#define CKGR_PLLAR_PLLACOUNT_Pos            8                                              /**< (CKGR_PLLAR) PLLA Counter Position */
#define CKGR_PLLAR_PLLACOUNT_Msk            (0x3FU << CKGR_PLLAR_PLLACOUNT_Pos)            /**< (CKGR_PLLAR) PLLA Counter Mask */
#define CKGR_PLLAR_PLLACOUNT(value)         (CKGR_PLLAR_PLLACOUNT_Msk & ((value) << CKGR_PLLAR_PLLACOUNT_Pos))
#define CKGR_PLLAR_MULA_Pos                 16                                             /**< (CKGR_PLLAR) PLLA Multiplier Position */
#define CKGR_PLLAR_MULA_Msk                 (0x7FFU << CKGR_PLLAR_MULA_Pos)                /**< (CKGR_PLLAR) PLLA Multiplier Mask */
#define CKGR_PLLAR_MULA(value)              (CKGR_PLLAR_MULA_Msk & ((value) << CKGR_PLLAR_MULA_Pos))
#define CKGR_PLLAR_ONE_Pos                  29                                             /**< (CKGR_PLLAR) Must Be Set to 1 Position */
#define CKGR_PLLAR_ONE_Msk                  (0x1U << CKGR_PLLAR_ONE_Pos)                   /**< (CKGR_PLLAR) Must Be Set to 1 Mask */
#define CKGR_PLLAR_ONE                      CKGR_PLLAR_ONE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use CKGR_PLLAR_ONE_Msk instead */
#define CKGR_PLLAR_MASK                     (0x27FF3FFFU)                                  /**< \deprecated (CKGR_PLLAR) Register MASK  (Use CKGR_PLLAR_Msk instead)  */
#define CKGR_PLLAR_Msk                      (0x27FF3FFFU)                                  /**< (CKGR_PLLAR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_MCKR : (PMC Offset: 0x30) (R/W 32) Master Clock Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CSS:2;                     /**< bit:   0..1  Master Clock Source Selection            */
    uint32_t :2;                        /**< bit:   2..3  Reserved */
    uint32_t PRES:3;                    /**< bit:   4..6  Processor Clock Prescaler                */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t MDIV:2;                    /**< bit:   8..9  Master Clock Division                    */
    uint32_t :3;                        /**< bit: 10..12  Reserved */
    uint32_t UPLLDIV2:1;                /**< bit:     13  UPLL Divisor by 2                        */
    uint32_t :18;                       /**< bit: 14..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_MCKR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_MCKR_OFFSET                     (0x30)                                        /**<  (PMC_MCKR) Master Clock Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_MCKR_CSS_Pos                    0                                              /**< (PMC_MCKR) Master Clock Source Selection Position */
#define PMC_MCKR_CSS_Msk                    (0x3U << PMC_MCKR_CSS_Pos)                     /**< (PMC_MCKR) Master Clock Source Selection Mask */
#define PMC_MCKR_CSS(value)                 (PMC_MCKR_CSS_Msk & ((value) << PMC_MCKR_CSS_Pos))
#define   PMC_MCKR_CSS_SLOW_CLK_Val         (0x0U)                                         /**< (PMC_MCKR) Slow Clock is selected  */
#define   PMC_MCKR_CSS_MAIN_CLK_Val         (0x1U)                                         /**< (PMC_MCKR) Main Clock is selected  */
#define   PMC_MCKR_CSS_PLLA_CLK_Val         (0x2U)                                         /**< (PMC_MCKR) PLLA Clock is selected  */
#define   PMC_MCKR_CSS_UPLL_CLK_Val         (0x3U)                                         /**< (PMC_MCKR) Divided UPLL Clock is selected  */
#define PMC_MCKR_CSS_SLOW_CLK               (PMC_MCKR_CSS_SLOW_CLK_Val << PMC_MCKR_CSS_Pos)  /**< (PMC_MCKR) Slow Clock is selected Position  */
#define PMC_MCKR_CSS_MAIN_CLK               (PMC_MCKR_CSS_MAIN_CLK_Val << PMC_MCKR_CSS_Pos)  /**< (PMC_MCKR) Main Clock is selected Position  */
#define PMC_MCKR_CSS_PLLA_CLK               (PMC_MCKR_CSS_PLLA_CLK_Val << PMC_MCKR_CSS_Pos)  /**< (PMC_MCKR) PLLA Clock is selected Position  */
#define PMC_MCKR_CSS_UPLL_CLK               (PMC_MCKR_CSS_UPLL_CLK_Val << PMC_MCKR_CSS_Pos)  /**< (PMC_MCKR) Divided UPLL Clock is selected Position  */
#define PMC_MCKR_PRES_Pos                   4                                              /**< (PMC_MCKR) Processor Clock Prescaler Position */
#define PMC_MCKR_PRES_Msk                   (0x7U << PMC_MCKR_PRES_Pos)                    /**< (PMC_MCKR) Processor Clock Prescaler Mask */
#define PMC_MCKR_PRES(value)                (PMC_MCKR_PRES_Msk & ((value) << PMC_MCKR_PRES_Pos))
#define   PMC_MCKR_PRES_CLK_1_Val           (0x0U)                                         /**< (PMC_MCKR) Selected clock  */
#define   PMC_MCKR_PRES_CLK_2_Val           (0x1U)                                         /**< (PMC_MCKR) Selected clock divided by 2  */
#define   PMC_MCKR_PRES_CLK_4_Val           (0x2U)                                         /**< (PMC_MCKR) Selected clock divided by 4  */
#define   PMC_MCKR_PRES_CLK_8_Val           (0x3U)                                         /**< (PMC_MCKR) Selected clock divided by 8  */
#define   PMC_MCKR_PRES_CLK_16_Val          (0x4U)                                         /**< (PMC_MCKR) Selected clock divided by 16  */
#define   PMC_MCKR_PRES_CLK_32_Val          (0x5U)                                         /**< (PMC_MCKR) Selected clock divided by 32  */
#define   PMC_MCKR_PRES_CLK_64_Val          (0x6U)                                         /**< (PMC_MCKR) Selected clock divided by 64  */
#define   PMC_MCKR_PRES_CLK_3_Val           (0x7U)                                         /**< (PMC_MCKR) Selected clock divided by 3  */
#define PMC_MCKR_PRES_CLK_1                 (PMC_MCKR_PRES_CLK_1_Val << PMC_MCKR_PRES_Pos)  /**< (PMC_MCKR) Selected clock Position  */
#define PMC_MCKR_PRES_CLK_2                 (PMC_MCKR_PRES_CLK_2_Val << PMC_MCKR_PRES_Pos)  /**< (PMC_MCKR) Selected clock divided by 2 Position  */
#define PMC_MCKR_PRES_CLK_4                 (PMC_MCKR_PRES_CLK_4_Val << PMC_MCKR_PRES_Pos)  /**< (PMC_MCKR) Selected clock divided by 4 Position  */
#define PMC_MCKR_PRES_CLK_8                 (PMC_MCKR_PRES_CLK_8_Val << PMC_MCKR_PRES_Pos)  /**< (PMC_MCKR) Selected clock divided by 8 Position  */
#define PMC_MCKR_PRES_CLK_16                (PMC_MCKR_PRES_CLK_16_Val << PMC_MCKR_PRES_Pos)  /**< (PMC_MCKR) Selected clock divided by 16 Position  */
#define PMC_MCKR_PRES_CLK_32                (PMC_MCKR_PRES_CLK_32_Val << PMC_MCKR_PRES_Pos)  /**< (PMC_MCKR) Selected clock divided by 32 Position  */
#define PMC_MCKR_PRES_CLK_64                (PMC_MCKR_PRES_CLK_64_Val << PMC_MCKR_PRES_Pos)  /**< (PMC_MCKR) Selected clock divided by 64 Position  */
#define PMC_MCKR_PRES_CLK_3                 (PMC_MCKR_PRES_CLK_3_Val << PMC_MCKR_PRES_Pos)  /**< (PMC_MCKR) Selected clock divided by 3 Position  */
#define PMC_MCKR_MDIV_Pos                   8                                              /**< (PMC_MCKR) Master Clock Division Position */
#define PMC_MCKR_MDIV_Msk                   (0x3U << PMC_MCKR_MDIV_Pos)                    /**< (PMC_MCKR) Master Clock Division Mask */
#define PMC_MCKR_MDIV(value)                (PMC_MCKR_MDIV_Msk & ((value) << PMC_MCKR_MDIV_Pos))
#define   PMC_MCKR_MDIV_EQ_PCK_Val          (0x0U)                                         /**< (PMC_MCKR) Master Clock is Prescaler Output Clock divided by 1.  */
#define   PMC_MCKR_MDIV_PCK_DIV2_Val        (0x1U)                                         /**< (PMC_MCKR) Master Clock is Prescaler Output Clock divided by 2.  */
#define   PMC_MCKR_MDIV_PCK_DIV4_Val        (0x2U)                                         /**< (PMC_MCKR) Master Clock is Prescaler Output Clock divided by 4.  */
#define   PMC_MCKR_MDIV_PCK_DIV3_Val        (0x3U)                                         /**< (PMC_MCKR) Master Clock is Prescaler Output Clock divided by 3.  */
#define PMC_MCKR_MDIV_EQ_PCK                (PMC_MCKR_MDIV_EQ_PCK_Val << PMC_MCKR_MDIV_Pos)  /**< (PMC_MCKR) Master Clock is Prescaler Output Clock divided by 1. Position  */
#define PMC_MCKR_MDIV_PCK_DIV2              (PMC_MCKR_MDIV_PCK_DIV2_Val << PMC_MCKR_MDIV_Pos)  /**< (PMC_MCKR) Master Clock is Prescaler Output Clock divided by 2. Position  */
#define PMC_MCKR_MDIV_PCK_DIV4              (PMC_MCKR_MDIV_PCK_DIV4_Val << PMC_MCKR_MDIV_Pos)  /**< (PMC_MCKR) Master Clock is Prescaler Output Clock divided by 4. Position  */
#define PMC_MCKR_MDIV_PCK_DIV3              (PMC_MCKR_MDIV_PCK_DIV3_Val << PMC_MCKR_MDIV_Pos)  /**< (PMC_MCKR) Master Clock is Prescaler Output Clock divided by 3. Position  */
#define PMC_MCKR_UPLLDIV2_Pos               13                                             /**< (PMC_MCKR) UPLL Divisor by 2 Position */
#define PMC_MCKR_UPLLDIV2_Msk               (0x1U << PMC_MCKR_UPLLDIV2_Pos)                /**< (PMC_MCKR) UPLL Divisor by 2 Mask */
#define PMC_MCKR_UPLLDIV2                   PMC_MCKR_UPLLDIV2_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_MCKR_UPLLDIV2_Msk instead */
#define PMC_MCKR_MASK                       (0x2373U)                                      /**< \deprecated (PMC_MCKR) Register MASK  (Use PMC_MCKR_Msk instead)  */
#define PMC_MCKR_Msk                        (0x2373U)                                      /**< (PMC_MCKR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_USB : (PMC Offset: 0x38) (R/W 32) USB Clock Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t USBS:1;                    /**< bit:      0  USB Input Clock Selection                */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t USBDIV:4;                  /**< bit:  8..11  Divider for USB Clock                    */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_USB_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_USB_OFFSET                      (0x38)                                        /**<  (PMC_USB) USB Clock Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_USB_USBS_Pos                    0                                              /**< (PMC_USB) USB Input Clock Selection Position */
#define PMC_USB_USBS_Msk                    (0x1U << PMC_USB_USBS_Pos)                     /**< (PMC_USB) USB Input Clock Selection Mask */
#define PMC_USB_USBS                        PMC_USB_USBS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_USB_USBS_Msk instead */
#define PMC_USB_USBDIV_Pos                  8                                              /**< (PMC_USB) Divider for USB Clock Position */
#define PMC_USB_USBDIV_Msk                  (0xFU << PMC_USB_USBDIV_Pos)                   /**< (PMC_USB) Divider for USB Clock Mask */
#define PMC_USB_USBDIV(value)               (PMC_USB_USBDIV_Msk & ((value) << PMC_USB_USBDIV_Pos))
#define PMC_USB_MASK                        (0xF01U)                                       /**< \deprecated (PMC_USB) Register MASK  (Use PMC_USB_Msk instead)  */
#define PMC_USB_Msk                         (0xF01U)                                       /**< (PMC_USB) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PCK : (PMC Offset: 0x40) (R/W 32) Programmable Clock 0 Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CSS:3;                     /**< bit:   0..2  Master Clock Source Selection            */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t PRES:8;                    /**< bit:  4..11  Programmable Clock Prescaler             */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PCK_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PCK_OFFSET                      (0x40)                                        /**<  (PMC_PCK) Programmable Clock 0 Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PCK_CSS_Pos                     0                                              /**< (PMC_PCK) Master Clock Source Selection Position */
#define PMC_PCK_CSS_Msk                     (0x7U << PMC_PCK_CSS_Pos)                      /**< (PMC_PCK) Master Clock Source Selection Mask */
#define PMC_PCK_CSS(value)                  (PMC_PCK_CSS_Msk & ((value) << PMC_PCK_CSS_Pos))
#define   PMC_PCK_CSS_SLOW_CLK_Val          (0x0U)                                         /**< (PMC_PCK) Slow Clock is selected  */
#define   PMC_PCK_CSS_MAIN_CLK_Val          (0x1U)                                         /**< (PMC_PCK) Main Clock is selected  */
#define   PMC_PCK_CSS_PLLA_CLK_Val          (0x2U)                                         /**< (PMC_PCK) PLLA Clock is selected  */
#define   PMC_PCK_CSS_UPLL_CLK_Val          (0x3U)                                         /**< (PMC_PCK) Divided UPLL Clock is selected  */
#define   PMC_PCK_CSS_MCK_Val               (0x4U)                                         /**< (PMC_PCK) Master Clock is selected  */
#define PMC_PCK_CSS_SLOW_CLK                (PMC_PCK_CSS_SLOW_CLK_Val << PMC_PCK_CSS_Pos)  /**< (PMC_PCK) Slow Clock is selected Position  */
#define PMC_PCK_CSS_MAIN_CLK                (PMC_PCK_CSS_MAIN_CLK_Val << PMC_PCK_CSS_Pos)  /**< (PMC_PCK) Main Clock is selected Position  */
#define PMC_PCK_CSS_PLLA_CLK                (PMC_PCK_CSS_PLLA_CLK_Val << PMC_PCK_CSS_Pos)  /**< (PMC_PCK) PLLA Clock is selected Position  */
#define PMC_PCK_CSS_UPLL_CLK                (PMC_PCK_CSS_UPLL_CLK_Val << PMC_PCK_CSS_Pos)  /**< (PMC_PCK) Divided UPLL Clock is selected Position  */
#define PMC_PCK_CSS_MCK                     (PMC_PCK_CSS_MCK_Val << PMC_PCK_CSS_Pos)       /**< (PMC_PCK) Master Clock is selected Position  */
#define PMC_PCK_PRES_Pos                    4                                              /**< (PMC_PCK) Programmable Clock Prescaler Position */
#define PMC_PCK_PRES_Msk                    (0xFFU << PMC_PCK_PRES_Pos)                    /**< (PMC_PCK) Programmable Clock Prescaler Mask */
#define PMC_PCK_PRES(value)                 (PMC_PCK_PRES_Msk & ((value) << PMC_PCK_PRES_Pos))
#define PMC_PCK_MASK                        (0xFF7U)                                       /**< \deprecated (PMC_PCK) Register MASK  (Use PMC_PCK_Msk instead)  */
#define PMC_PCK_Msk                         (0xFF7U)                                       /**< (PMC_PCK) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_IER : (PMC Offset: 0x60) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MOSCXTS:1;                 /**< bit:      0  3 to 20 MHz Crystal Oscillator Status Interrupt Enable */
    uint32_t LOCKA:1;                   /**< bit:      1  PLLA Lock Interrupt Enable               */
    uint32_t :1;                        /**< bit:      2  Reserved */
    uint32_t MCKRDY:1;                  /**< bit:      3  Master Clock Ready Interrupt Enable      */
    uint32_t :2;                        /**< bit:   4..5  Reserved */
    uint32_t LOCKU:1;                   /**< bit:      6  UTMI PLL Lock Interrupt Enable           */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t PCKRDY0:1;                 /**< bit:      8  Programmable Clock Ready 0 Interrupt Enable */
    uint32_t PCKRDY1:1;                 /**< bit:      9  Programmable Clock Ready 1 Interrupt Enable */
    uint32_t PCKRDY2:1;                 /**< bit:     10  Programmable Clock Ready 2 Interrupt Enable */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t MOSCSELS:1;                /**< bit:     16  Main Clock Source Oscillator Selection Status Interrupt Enable */
    uint32_t MOSCRCS:1;                 /**< bit:     17  4/8/12 MHz RC Oscillator Status Interrupt Enable */
    uint32_t CFDEV:1;                   /**< bit:     18  Clock Failure Detector Event Interrupt Enable */
    uint32_t :2;                        /**< bit: 19..20  Reserved */
    uint32_t XT32KERR:1;                /**< bit:     21  32.768 kHz Crystal Oscillator Error Interrupt Enable */
    uint32_t :10;                       /**< bit: 22..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_IER_OFFSET                      (0x60)                                        /**<  (PMC_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_IER_MOSCXTS_Pos                 0                                              /**< (PMC_IER) 3 to 20 MHz Crystal Oscillator Status Interrupt Enable Position */
#define PMC_IER_MOSCXTS_Msk                 (0x1U << PMC_IER_MOSCXTS_Pos)                  /**< (PMC_IER) 3 to 20 MHz Crystal Oscillator Status Interrupt Enable Mask */
#define PMC_IER_MOSCXTS                     PMC_IER_MOSCXTS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_MOSCXTS_Msk instead */
#define PMC_IER_LOCKA_Pos                   1                                              /**< (PMC_IER) PLLA Lock Interrupt Enable Position */
#define PMC_IER_LOCKA_Msk                   (0x1U << PMC_IER_LOCKA_Pos)                    /**< (PMC_IER) PLLA Lock Interrupt Enable Mask */
#define PMC_IER_LOCKA                       PMC_IER_LOCKA_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_LOCKA_Msk instead */
#define PMC_IER_MCKRDY_Pos                  3                                              /**< (PMC_IER) Master Clock Ready Interrupt Enable Position */
#define PMC_IER_MCKRDY_Msk                  (0x1U << PMC_IER_MCKRDY_Pos)                   /**< (PMC_IER) Master Clock Ready Interrupt Enable Mask */
#define PMC_IER_MCKRDY                      PMC_IER_MCKRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_MCKRDY_Msk instead */
#define PMC_IER_LOCKU_Pos                   6                                              /**< (PMC_IER) UTMI PLL Lock Interrupt Enable Position */
#define PMC_IER_LOCKU_Msk                   (0x1U << PMC_IER_LOCKU_Pos)                    /**< (PMC_IER) UTMI PLL Lock Interrupt Enable Mask */
#define PMC_IER_LOCKU                       PMC_IER_LOCKU_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_LOCKU_Msk instead */
#define PMC_IER_PCKRDY0_Pos                 8                                              /**< (PMC_IER) Programmable Clock Ready 0 Interrupt Enable Position */
#define PMC_IER_PCKRDY0_Msk                 (0x1U << PMC_IER_PCKRDY0_Pos)                  /**< (PMC_IER) Programmable Clock Ready 0 Interrupt Enable Mask */
#define PMC_IER_PCKRDY0                     PMC_IER_PCKRDY0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_PCKRDY0_Msk instead */
#define PMC_IER_PCKRDY1_Pos                 9                                              /**< (PMC_IER) Programmable Clock Ready 1 Interrupt Enable Position */
#define PMC_IER_PCKRDY1_Msk                 (0x1U << PMC_IER_PCKRDY1_Pos)                  /**< (PMC_IER) Programmable Clock Ready 1 Interrupt Enable Mask */
#define PMC_IER_PCKRDY1                     PMC_IER_PCKRDY1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_PCKRDY1_Msk instead */
#define PMC_IER_PCKRDY2_Pos                 10                                             /**< (PMC_IER) Programmable Clock Ready 2 Interrupt Enable Position */
#define PMC_IER_PCKRDY2_Msk                 (0x1U << PMC_IER_PCKRDY2_Pos)                  /**< (PMC_IER) Programmable Clock Ready 2 Interrupt Enable Mask */
#define PMC_IER_PCKRDY2                     PMC_IER_PCKRDY2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_PCKRDY2_Msk instead */
#define PMC_IER_MOSCSELS_Pos                16                                             /**< (PMC_IER) Main Clock Source Oscillator Selection Status Interrupt Enable Position */
#define PMC_IER_MOSCSELS_Msk                (0x1U << PMC_IER_MOSCSELS_Pos)                 /**< (PMC_IER) Main Clock Source Oscillator Selection Status Interrupt Enable Mask */
#define PMC_IER_MOSCSELS                    PMC_IER_MOSCSELS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_MOSCSELS_Msk instead */
#define PMC_IER_MOSCRCS_Pos                 17                                             /**< (PMC_IER) 4/8/12 MHz RC Oscillator Status Interrupt Enable Position */
#define PMC_IER_MOSCRCS_Msk                 (0x1U << PMC_IER_MOSCRCS_Pos)                  /**< (PMC_IER) 4/8/12 MHz RC Oscillator Status Interrupt Enable Mask */
#define PMC_IER_MOSCRCS                     PMC_IER_MOSCRCS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_MOSCRCS_Msk instead */
#define PMC_IER_CFDEV_Pos                   18                                             /**< (PMC_IER) Clock Failure Detector Event Interrupt Enable Position */
#define PMC_IER_CFDEV_Msk                   (0x1U << PMC_IER_CFDEV_Pos)                    /**< (PMC_IER) Clock Failure Detector Event Interrupt Enable Mask */
#define PMC_IER_CFDEV                       PMC_IER_CFDEV_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_CFDEV_Msk instead */
#define PMC_IER_XT32KERR_Pos                21                                             /**< (PMC_IER) 32.768 kHz Crystal Oscillator Error Interrupt Enable Position */
#define PMC_IER_XT32KERR_Msk                (0x1U << PMC_IER_XT32KERR_Pos)                 /**< (PMC_IER) 32.768 kHz Crystal Oscillator Error Interrupt Enable Mask */
#define PMC_IER_XT32KERR                    PMC_IER_XT32KERR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IER_XT32KERR_Msk instead */
#define PMC_IER_MASK                        (0x27074BU)                                    /**< \deprecated (PMC_IER) Register MASK  (Use PMC_IER_Msk instead)  */
#define PMC_IER_Msk                         (0x27074BU)                                    /**< (PMC_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_IDR : (PMC Offset: 0x64) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MOSCXTS:1;                 /**< bit:      0  3 to 20 MHz Crystal Oscillator Status Interrupt Disable */
    uint32_t LOCKA:1;                   /**< bit:      1  PLLA Lock Interrupt Disable              */
    uint32_t :1;                        /**< bit:      2  Reserved */
    uint32_t MCKRDY:1;                  /**< bit:      3  Master Clock Ready Interrupt Disable     */
    uint32_t :2;                        /**< bit:   4..5  Reserved */
    uint32_t LOCKU:1;                   /**< bit:      6  UTMI PLL Lock Interrupt Disable          */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t PCKRDY0:1;                 /**< bit:      8  Programmable Clock Ready 0 Interrupt Disable */
    uint32_t PCKRDY1:1;                 /**< bit:      9  Programmable Clock Ready 1 Interrupt Disable */
    uint32_t PCKRDY2:1;                 /**< bit:     10  Programmable Clock Ready 2 Interrupt Disable */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t MOSCSELS:1;                /**< bit:     16  Main Clock Source Oscillator Selection Status Interrupt Disable */
    uint32_t MOSCRCS:1;                 /**< bit:     17  4/8/12 MHz RC Status Interrupt Disable   */
    uint32_t CFDEV:1;                   /**< bit:     18  Clock Failure Detector Event Interrupt Disable */
    uint32_t :2;                        /**< bit: 19..20  Reserved */
    uint32_t XT32KERR:1;                /**< bit:     21  32.768 kHz Crystal Oscillator Error Interrupt Disable */
    uint32_t :10;                       /**< bit: 22..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_IDR_OFFSET                      (0x64)                                        /**<  (PMC_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_IDR_MOSCXTS_Pos                 0                                              /**< (PMC_IDR) 3 to 20 MHz Crystal Oscillator Status Interrupt Disable Position */
#define PMC_IDR_MOSCXTS_Msk                 (0x1U << PMC_IDR_MOSCXTS_Pos)                  /**< (PMC_IDR) 3 to 20 MHz Crystal Oscillator Status Interrupt Disable Mask */
#define PMC_IDR_MOSCXTS                     PMC_IDR_MOSCXTS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_MOSCXTS_Msk instead */
#define PMC_IDR_LOCKA_Pos                   1                                              /**< (PMC_IDR) PLLA Lock Interrupt Disable Position */
#define PMC_IDR_LOCKA_Msk                   (0x1U << PMC_IDR_LOCKA_Pos)                    /**< (PMC_IDR) PLLA Lock Interrupt Disable Mask */
#define PMC_IDR_LOCKA                       PMC_IDR_LOCKA_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_LOCKA_Msk instead */
#define PMC_IDR_MCKRDY_Pos                  3                                              /**< (PMC_IDR) Master Clock Ready Interrupt Disable Position */
#define PMC_IDR_MCKRDY_Msk                  (0x1U << PMC_IDR_MCKRDY_Pos)                   /**< (PMC_IDR) Master Clock Ready Interrupt Disable Mask */
#define PMC_IDR_MCKRDY                      PMC_IDR_MCKRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_MCKRDY_Msk instead */
#define PMC_IDR_LOCKU_Pos                   6                                              /**< (PMC_IDR) UTMI PLL Lock Interrupt Disable Position */
#define PMC_IDR_LOCKU_Msk                   (0x1U << PMC_IDR_LOCKU_Pos)                    /**< (PMC_IDR) UTMI PLL Lock Interrupt Disable Mask */
#define PMC_IDR_LOCKU                       PMC_IDR_LOCKU_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_LOCKU_Msk instead */
#define PMC_IDR_PCKRDY0_Pos                 8                                              /**< (PMC_IDR) Programmable Clock Ready 0 Interrupt Disable Position */
#define PMC_IDR_PCKRDY0_Msk                 (0x1U << PMC_IDR_PCKRDY0_Pos)                  /**< (PMC_IDR) Programmable Clock Ready 0 Interrupt Disable Mask */
#define PMC_IDR_PCKRDY0                     PMC_IDR_PCKRDY0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_PCKRDY0_Msk instead */
#define PMC_IDR_PCKRDY1_Pos                 9                                              /**< (PMC_IDR) Programmable Clock Ready 1 Interrupt Disable Position */
#define PMC_IDR_PCKRDY1_Msk                 (0x1U << PMC_IDR_PCKRDY1_Pos)                  /**< (PMC_IDR) Programmable Clock Ready 1 Interrupt Disable Mask */
#define PMC_IDR_PCKRDY1                     PMC_IDR_PCKRDY1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_PCKRDY1_Msk instead */
#define PMC_IDR_PCKRDY2_Pos                 10                                             /**< (PMC_IDR) Programmable Clock Ready 2 Interrupt Disable Position */
#define PMC_IDR_PCKRDY2_Msk                 (0x1U << PMC_IDR_PCKRDY2_Pos)                  /**< (PMC_IDR) Programmable Clock Ready 2 Interrupt Disable Mask */
#define PMC_IDR_PCKRDY2                     PMC_IDR_PCKRDY2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_PCKRDY2_Msk instead */
#define PMC_IDR_MOSCSELS_Pos                16                                             /**< (PMC_IDR) Main Clock Source Oscillator Selection Status Interrupt Disable Position */
#define PMC_IDR_MOSCSELS_Msk                (0x1U << PMC_IDR_MOSCSELS_Pos)                 /**< (PMC_IDR) Main Clock Source Oscillator Selection Status Interrupt Disable Mask */
#define PMC_IDR_MOSCSELS                    PMC_IDR_MOSCSELS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_MOSCSELS_Msk instead */
#define PMC_IDR_MOSCRCS_Pos                 17                                             /**< (PMC_IDR) 4/8/12 MHz RC Status Interrupt Disable Position */
#define PMC_IDR_MOSCRCS_Msk                 (0x1U << PMC_IDR_MOSCRCS_Pos)                  /**< (PMC_IDR) 4/8/12 MHz RC Status Interrupt Disable Mask */
#define PMC_IDR_MOSCRCS                     PMC_IDR_MOSCRCS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_MOSCRCS_Msk instead */
#define PMC_IDR_CFDEV_Pos                   18                                             /**< (PMC_IDR) Clock Failure Detector Event Interrupt Disable Position */
#define PMC_IDR_CFDEV_Msk                   (0x1U << PMC_IDR_CFDEV_Pos)                    /**< (PMC_IDR) Clock Failure Detector Event Interrupt Disable Mask */
#define PMC_IDR_CFDEV                       PMC_IDR_CFDEV_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_CFDEV_Msk instead */
#define PMC_IDR_XT32KERR_Pos                21                                             /**< (PMC_IDR) 32.768 kHz Crystal Oscillator Error Interrupt Disable Position */
#define PMC_IDR_XT32KERR_Msk                (0x1U << PMC_IDR_XT32KERR_Pos)                 /**< (PMC_IDR) 32.768 kHz Crystal Oscillator Error Interrupt Disable Mask */
#define PMC_IDR_XT32KERR                    PMC_IDR_XT32KERR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IDR_XT32KERR_Msk instead */
#define PMC_IDR_MASK                        (0x27074BU)                                    /**< \deprecated (PMC_IDR) Register MASK  (Use PMC_IDR_Msk instead)  */
#define PMC_IDR_Msk                         (0x27074BU)                                    /**< (PMC_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SR : (PMC Offset: 0x68) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MOSCXTS:1;                 /**< bit:      0  3 to 20 MHz Crystal Oscillator Status    */
    uint32_t LOCKA:1;                   /**< bit:      1  PLLA Lock Status                         */
    uint32_t :1;                        /**< bit:      2  Reserved */
    uint32_t MCKRDY:1;                  /**< bit:      3  Master Clock Status                      */
    uint32_t :2;                        /**< bit:   4..5  Reserved */
    uint32_t LOCKU:1;                   /**< bit:      6  UTMI PLL Lock Status                     */
    uint32_t OSCSELS:1;                 /**< bit:      7  Slow Clock Source Oscillator Selection   */
    uint32_t PCKRDY0:1;                 /**< bit:      8  Programmable Clock Ready Status          */
    uint32_t PCKRDY1:1;                 /**< bit:      9  Programmable Clock Ready Status          */
    uint32_t PCKRDY2:1;                 /**< bit:     10  Programmable Clock Ready Status          */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t MOSCSELS:1;                /**< bit:     16  Main Clock Source Oscillator Selection Status */
    uint32_t MOSCRCS:1;                 /**< bit:     17  4/8/12 MHz RC Oscillator Status          */
    uint32_t CFDEV:1;                   /**< bit:     18  Clock Failure Detector Event             */
    uint32_t CFDS:1;                    /**< bit:     19  Clock Failure Detector Status            */
    uint32_t FOS:1;                     /**< bit:     20  Clock Failure Detector Fault Output Status */
    uint32_t XT32KERR:1;                /**< bit:     21  Slow Crystal Oscillator Error            */
    uint32_t :10;                       /**< bit: 22..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SR_OFFSET                       (0x68)                                        /**<  (PMC_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SR_MOSCXTS_Pos                  0                                              /**< (PMC_SR) 3 to 20 MHz Crystal Oscillator Status Position */
#define PMC_SR_MOSCXTS_Msk                  (0x1U << PMC_SR_MOSCXTS_Pos)                   /**< (PMC_SR) 3 to 20 MHz Crystal Oscillator Status Mask */
#define PMC_SR_MOSCXTS                      PMC_SR_MOSCXTS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_MOSCXTS_Msk instead */
#define PMC_SR_LOCKA_Pos                    1                                              /**< (PMC_SR) PLLA Lock Status Position */
#define PMC_SR_LOCKA_Msk                    (0x1U << PMC_SR_LOCKA_Pos)                     /**< (PMC_SR) PLLA Lock Status Mask */
#define PMC_SR_LOCKA                        PMC_SR_LOCKA_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_LOCKA_Msk instead */
#define PMC_SR_MCKRDY_Pos                   3                                              /**< (PMC_SR) Master Clock Status Position */
#define PMC_SR_MCKRDY_Msk                   (0x1U << PMC_SR_MCKRDY_Pos)                    /**< (PMC_SR) Master Clock Status Mask */
#define PMC_SR_MCKRDY                       PMC_SR_MCKRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_MCKRDY_Msk instead */
#define PMC_SR_LOCKU_Pos                    6                                              /**< (PMC_SR) UTMI PLL Lock Status Position */
#define PMC_SR_LOCKU_Msk                    (0x1U << PMC_SR_LOCKU_Pos)                     /**< (PMC_SR) UTMI PLL Lock Status Mask */
#define PMC_SR_LOCKU                        PMC_SR_LOCKU_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_LOCKU_Msk instead */
#define PMC_SR_OSCSELS_Pos                  7                                              /**< (PMC_SR) Slow Clock Source Oscillator Selection Position */
#define PMC_SR_OSCSELS_Msk                  (0x1U << PMC_SR_OSCSELS_Pos)                   /**< (PMC_SR) Slow Clock Source Oscillator Selection Mask */
#define PMC_SR_OSCSELS                      PMC_SR_OSCSELS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_OSCSELS_Msk instead */
#define PMC_SR_PCKRDY0_Pos                  8                                              /**< (PMC_SR) Programmable Clock Ready Status Position */
#define PMC_SR_PCKRDY0_Msk                  (0x1U << PMC_SR_PCKRDY0_Pos)                   /**< (PMC_SR) Programmable Clock Ready Status Mask */
#define PMC_SR_PCKRDY0                      PMC_SR_PCKRDY0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_PCKRDY0_Msk instead */
#define PMC_SR_PCKRDY1_Pos                  9                                              /**< (PMC_SR) Programmable Clock Ready Status Position */
#define PMC_SR_PCKRDY1_Msk                  (0x1U << PMC_SR_PCKRDY1_Pos)                   /**< (PMC_SR) Programmable Clock Ready Status Mask */
#define PMC_SR_PCKRDY1                      PMC_SR_PCKRDY1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_PCKRDY1_Msk instead */
#define PMC_SR_PCKRDY2_Pos                  10                                             /**< (PMC_SR) Programmable Clock Ready Status Position */
#define PMC_SR_PCKRDY2_Msk                  (0x1U << PMC_SR_PCKRDY2_Pos)                   /**< (PMC_SR) Programmable Clock Ready Status Mask */
#define PMC_SR_PCKRDY2                      PMC_SR_PCKRDY2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_PCKRDY2_Msk instead */
#define PMC_SR_MOSCSELS_Pos                 16                                             /**< (PMC_SR) Main Clock Source Oscillator Selection Status Position */
#define PMC_SR_MOSCSELS_Msk                 (0x1U << PMC_SR_MOSCSELS_Pos)                  /**< (PMC_SR) Main Clock Source Oscillator Selection Status Mask */
#define PMC_SR_MOSCSELS                     PMC_SR_MOSCSELS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_MOSCSELS_Msk instead */
#define PMC_SR_MOSCRCS_Pos                  17                                             /**< (PMC_SR) 4/8/12 MHz RC Oscillator Status Position */
#define PMC_SR_MOSCRCS_Msk                  (0x1U << PMC_SR_MOSCRCS_Pos)                   /**< (PMC_SR) 4/8/12 MHz RC Oscillator Status Mask */
#define PMC_SR_MOSCRCS                      PMC_SR_MOSCRCS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_MOSCRCS_Msk instead */
#define PMC_SR_CFDEV_Pos                    18                                             /**< (PMC_SR) Clock Failure Detector Event Position */
#define PMC_SR_CFDEV_Msk                    (0x1U << PMC_SR_CFDEV_Pos)                     /**< (PMC_SR) Clock Failure Detector Event Mask */
#define PMC_SR_CFDEV                        PMC_SR_CFDEV_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_CFDEV_Msk instead */
#define PMC_SR_CFDS_Pos                     19                                             /**< (PMC_SR) Clock Failure Detector Status Position */
#define PMC_SR_CFDS_Msk                     (0x1U << PMC_SR_CFDS_Pos)                      /**< (PMC_SR) Clock Failure Detector Status Mask */
#define PMC_SR_CFDS                         PMC_SR_CFDS_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_CFDS_Msk instead */
#define PMC_SR_FOS_Pos                      20                                             /**< (PMC_SR) Clock Failure Detector Fault Output Status Position */
#define PMC_SR_FOS_Msk                      (0x1U << PMC_SR_FOS_Pos)                       /**< (PMC_SR) Clock Failure Detector Fault Output Status Mask */
#define PMC_SR_FOS                          PMC_SR_FOS_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_FOS_Msk instead */
#define PMC_SR_XT32KERR_Pos                 21                                             /**< (PMC_SR) Slow Crystal Oscillator Error Position */
#define PMC_SR_XT32KERR_Msk                 (0x1U << PMC_SR_XT32KERR_Pos)                  /**< (PMC_SR) Slow Crystal Oscillator Error Mask */
#define PMC_SR_XT32KERR                     PMC_SR_XT32KERR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SR_XT32KERR_Msk instead */
#define PMC_SR_MASK                         (0x3F07CBU)                                    /**< \deprecated (PMC_SR) Register MASK  (Use PMC_SR_Msk instead)  */
#define PMC_SR_Msk                          (0x3F07CBU)                                    /**< (PMC_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_IMR : (PMC Offset: 0x6c) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MOSCXTS:1;                 /**< bit:      0  3 to 20 MHz Crystal Oscillator Status Interrupt Mask */
    uint32_t LOCKA:1;                   /**< bit:      1  PLLA Lock Interrupt Mask                 */
    uint32_t :1;                        /**< bit:      2  Reserved */
    uint32_t MCKRDY:1;                  /**< bit:      3  Master Clock Ready Interrupt Mask        */
    uint32_t :2;                        /**< bit:   4..5  Reserved */
    uint32_t LOCKU:1;                   /**< bit:      6  UTMI PLL Lock Interrupt Mask             */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t PCKRDY0:1;                 /**< bit:      8  Programmable Clock Ready 0 Interrupt Mask */
    uint32_t PCKRDY1:1;                 /**< bit:      9  Programmable Clock Ready 1 Interrupt Mask */
    uint32_t PCKRDY2:1;                 /**< bit:     10  Programmable Clock Ready 2 Interrupt Mask */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t MOSCSELS:1;                /**< bit:     16  Main Clock Source Oscillator Selection Status Interrupt Mask */
    uint32_t MOSCRCS:1;                 /**< bit:     17  4/8/12 MHz RC Status Interrupt Mask      */
    uint32_t CFDEV:1;                   /**< bit:     18  Clock Failure Detector Event Interrupt Mask */
    uint32_t :2;                        /**< bit: 19..20  Reserved */
    uint32_t XT32KERR:1;                /**< bit:     21  32.768 kHz Crystal Oscillator Error Interrupt Mask */
    uint32_t :10;                       /**< bit: 22..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_IMR_OFFSET                      (0x6C)                                        /**<  (PMC_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_IMR_MOSCXTS_Pos                 0                                              /**< (PMC_IMR) 3 to 20 MHz Crystal Oscillator Status Interrupt Mask Position */
#define PMC_IMR_MOSCXTS_Msk                 (0x1U << PMC_IMR_MOSCXTS_Pos)                  /**< (PMC_IMR) 3 to 20 MHz Crystal Oscillator Status Interrupt Mask Mask */
#define PMC_IMR_MOSCXTS                     PMC_IMR_MOSCXTS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_MOSCXTS_Msk instead */
#define PMC_IMR_LOCKA_Pos                   1                                              /**< (PMC_IMR) PLLA Lock Interrupt Mask Position */
#define PMC_IMR_LOCKA_Msk                   (0x1U << PMC_IMR_LOCKA_Pos)                    /**< (PMC_IMR) PLLA Lock Interrupt Mask Mask */
#define PMC_IMR_LOCKA                       PMC_IMR_LOCKA_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_LOCKA_Msk instead */
#define PMC_IMR_MCKRDY_Pos                  3                                              /**< (PMC_IMR) Master Clock Ready Interrupt Mask Position */
#define PMC_IMR_MCKRDY_Msk                  (0x1U << PMC_IMR_MCKRDY_Pos)                   /**< (PMC_IMR) Master Clock Ready Interrupt Mask Mask */
#define PMC_IMR_MCKRDY                      PMC_IMR_MCKRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_MCKRDY_Msk instead */
#define PMC_IMR_LOCKU_Pos                   6                                              /**< (PMC_IMR) UTMI PLL Lock Interrupt Mask Position */
#define PMC_IMR_LOCKU_Msk                   (0x1U << PMC_IMR_LOCKU_Pos)                    /**< (PMC_IMR) UTMI PLL Lock Interrupt Mask Mask */
#define PMC_IMR_LOCKU                       PMC_IMR_LOCKU_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_LOCKU_Msk instead */
#define PMC_IMR_PCKRDY0_Pos                 8                                              /**< (PMC_IMR) Programmable Clock Ready 0 Interrupt Mask Position */
#define PMC_IMR_PCKRDY0_Msk                 (0x1U << PMC_IMR_PCKRDY0_Pos)                  /**< (PMC_IMR) Programmable Clock Ready 0 Interrupt Mask Mask */
#define PMC_IMR_PCKRDY0                     PMC_IMR_PCKRDY0_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_PCKRDY0_Msk instead */
#define PMC_IMR_PCKRDY1_Pos                 9                                              /**< (PMC_IMR) Programmable Clock Ready 1 Interrupt Mask Position */
#define PMC_IMR_PCKRDY1_Msk                 (0x1U << PMC_IMR_PCKRDY1_Pos)                  /**< (PMC_IMR) Programmable Clock Ready 1 Interrupt Mask Mask */
#define PMC_IMR_PCKRDY1                     PMC_IMR_PCKRDY1_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_PCKRDY1_Msk instead */
#define PMC_IMR_PCKRDY2_Pos                 10                                             /**< (PMC_IMR) Programmable Clock Ready 2 Interrupt Mask Position */
#define PMC_IMR_PCKRDY2_Msk                 (0x1U << PMC_IMR_PCKRDY2_Pos)                  /**< (PMC_IMR) Programmable Clock Ready 2 Interrupt Mask Mask */
#define PMC_IMR_PCKRDY2                     PMC_IMR_PCKRDY2_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_PCKRDY2_Msk instead */
#define PMC_IMR_MOSCSELS_Pos                16                                             /**< (PMC_IMR) Main Clock Source Oscillator Selection Status Interrupt Mask Position */
#define PMC_IMR_MOSCSELS_Msk                (0x1U << PMC_IMR_MOSCSELS_Pos)                 /**< (PMC_IMR) Main Clock Source Oscillator Selection Status Interrupt Mask Mask */
#define PMC_IMR_MOSCSELS                    PMC_IMR_MOSCSELS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_MOSCSELS_Msk instead */
#define PMC_IMR_MOSCRCS_Pos                 17                                             /**< (PMC_IMR) 4/8/12 MHz RC Status Interrupt Mask Position */
#define PMC_IMR_MOSCRCS_Msk                 (0x1U << PMC_IMR_MOSCRCS_Pos)                  /**< (PMC_IMR) 4/8/12 MHz RC Status Interrupt Mask Mask */
#define PMC_IMR_MOSCRCS                     PMC_IMR_MOSCRCS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_MOSCRCS_Msk instead */
#define PMC_IMR_CFDEV_Pos                   18                                             /**< (PMC_IMR) Clock Failure Detector Event Interrupt Mask Position */
#define PMC_IMR_CFDEV_Msk                   (0x1U << PMC_IMR_CFDEV_Pos)                    /**< (PMC_IMR) Clock Failure Detector Event Interrupt Mask Mask */
#define PMC_IMR_CFDEV                       PMC_IMR_CFDEV_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_CFDEV_Msk instead */
#define PMC_IMR_XT32KERR_Pos                21                                             /**< (PMC_IMR) 32.768 kHz Crystal Oscillator Error Interrupt Mask Position */
#define PMC_IMR_XT32KERR_Msk                (0x1U << PMC_IMR_XT32KERR_Pos)                 /**< (PMC_IMR) 32.768 kHz Crystal Oscillator Error Interrupt Mask Mask */
#define PMC_IMR_XT32KERR                    PMC_IMR_XT32KERR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_IMR_XT32KERR_Msk instead */
#define PMC_IMR_MASK                        (0x27074BU)                                    /**< \deprecated (PMC_IMR) Register MASK  (Use PMC_IMR_Msk instead)  */
#define PMC_IMR_Msk                         (0x27074BU)                                    /**< (PMC_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_FSMR : (PMC Offset: 0x70) (R/W 32) Fast Startup Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FSTT0:1;                   /**< bit:      0  Fast Startup Input Enable 0              */
    uint32_t FSTT1:1;                   /**< bit:      1  Fast Startup Input Enable 1              */
    uint32_t FSTT2:1;                   /**< bit:      2  Fast Startup Input Enable 2              */
    uint32_t FSTT3:1;                   /**< bit:      3  Fast Startup Input Enable 3              */
    uint32_t FSTT4:1;                   /**< bit:      4  Fast Startup Input Enable 4              */
    uint32_t FSTT5:1;                   /**< bit:      5  Fast Startup Input Enable 5              */
    uint32_t FSTT6:1;                   /**< bit:      6  Fast Startup Input Enable 6              */
    uint32_t FSTT7:1;                   /**< bit:      7  Fast Startup Input Enable 7              */
    uint32_t FSTT8:1;                   /**< bit:      8  Fast Startup Input Enable 8              */
    uint32_t FSTT9:1;                   /**< bit:      9  Fast Startup Input Enable 9              */
    uint32_t FSTT10:1;                  /**< bit:     10  Fast Startup Input Enable 10             */
    uint32_t FSTT11:1;                  /**< bit:     11  Fast Startup Input Enable 11             */
    uint32_t FSTT12:1;                  /**< bit:     12  Fast Startup Input Enable 12             */
    uint32_t FSTT13:1;                  /**< bit:     13  Fast Startup Input Enable 13             */
    uint32_t FSTT14:1;                  /**< bit:     14  Fast Startup Input Enable 14             */
    uint32_t FSTT15:1;                  /**< bit:     15  Fast Startup Input Enable 15             */
    uint32_t RTTAL:1;                   /**< bit:     16  RTT Alarm Enable                         */
    uint32_t RTCAL:1;                   /**< bit:     17  RTC Alarm Enable                         */
    uint32_t USBAL:1;                   /**< bit:     18  USB Alarm Enable                         */
    uint32_t :1;                        /**< bit:     19  Reserved */
    uint32_t LPM:1;                     /**< bit:     20  Low-power Mode                           */
    uint32_t FLPM:2;                    /**< bit: 21..22  Flash Low-power Mode                     */
    uint32_t FFLPM:1;                   /**< bit:     23  Force Flash Low-power Mode               */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_FSMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_FSMR_OFFSET                     (0x70)                                        /**<  (PMC_FSMR) Fast Startup Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_FSMR_FSTT0_Pos                  0                                              /**< (PMC_FSMR) Fast Startup Input Enable 0 Position */
#define PMC_FSMR_FSTT0_Msk                  (0x1U << PMC_FSMR_FSTT0_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 0 Mask */
#define PMC_FSMR_FSTT0                      PMC_FSMR_FSTT0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT0_Msk instead */
#define PMC_FSMR_FSTT1_Pos                  1                                              /**< (PMC_FSMR) Fast Startup Input Enable 1 Position */
#define PMC_FSMR_FSTT1_Msk                  (0x1U << PMC_FSMR_FSTT1_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 1 Mask */
#define PMC_FSMR_FSTT1                      PMC_FSMR_FSTT1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT1_Msk instead */
#define PMC_FSMR_FSTT2_Pos                  2                                              /**< (PMC_FSMR) Fast Startup Input Enable 2 Position */
#define PMC_FSMR_FSTT2_Msk                  (0x1U << PMC_FSMR_FSTT2_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 2 Mask */
#define PMC_FSMR_FSTT2                      PMC_FSMR_FSTT2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT2_Msk instead */
#define PMC_FSMR_FSTT3_Pos                  3                                              /**< (PMC_FSMR) Fast Startup Input Enable 3 Position */
#define PMC_FSMR_FSTT3_Msk                  (0x1U << PMC_FSMR_FSTT3_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 3 Mask */
#define PMC_FSMR_FSTT3                      PMC_FSMR_FSTT3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT3_Msk instead */
#define PMC_FSMR_FSTT4_Pos                  4                                              /**< (PMC_FSMR) Fast Startup Input Enable 4 Position */
#define PMC_FSMR_FSTT4_Msk                  (0x1U << PMC_FSMR_FSTT4_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 4 Mask */
#define PMC_FSMR_FSTT4                      PMC_FSMR_FSTT4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT4_Msk instead */
#define PMC_FSMR_FSTT5_Pos                  5                                              /**< (PMC_FSMR) Fast Startup Input Enable 5 Position */
#define PMC_FSMR_FSTT5_Msk                  (0x1U << PMC_FSMR_FSTT5_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 5 Mask */
#define PMC_FSMR_FSTT5                      PMC_FSMR_FSTT5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT5_Msk instead */
#define PMC_FSMR_FSTT6_Pos                  6                                              /**< (PMC_FSMR) Fast Startup Input Enable 6 Position */
#define PMC_FSMR_FSTT6_Msk                  (0x1U << PMC_FSMR_FSTT6_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 6 Mask */
#define PMC_FSMR_FSTT6                      PMC_FSMR_FSTT6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT6_Msk instead */
#define PMC_FSMR_FSTT7_Pos                  7                                              /**< (PMC_FSMR) Fast Startup Input Enable 7 Position */
#define PMC_FSMR_FSTT7_Msk                  (0x1U << PMC_FSMR_FSTT7_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 7 Mask */
#define PMC_FSMR_FSTT7                      PMC_FSMR_FSTT7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT7_Msk instead */
#define PMC_FSMR_FSTT8_Pos                  8                                              /**< (PMC_FSMR) Fast Startup Input Enable 8 Position */
#define PMC_FSMR_FSTT8_Msk                  (0x1U << PMC_FSMR_FSTT8_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 8 Mask */
#define PMC_FSMR_FSTT8                      PMC_FSMR_FSTT8_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT8_Msk instead */
#define PMC_FSMR_FSTT9_Pos                  9                                              /**< (PMC_FSMR) Fast Startup Input Enable 9 Position */
#define PMC_FSMR_FSTT9_Msk                  (0x1U << PMC_FSMR_FSTT9_Pos)                   /**< (PMC_FSMR) Fast Startup Input Enable 9 Mask */
#define PMC_FSMR_FSTT9                      PMC_FSMR_FSTT9_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT9_Msk instead */
#define PMC_FSMR_FSTT10_Pos                 10                                             /**< (PMC_FSMR) Fast Startup Input Enable 10 Position */
#define PMC_FSMR_FSTT10_Msk                 (0x1U << PMC_FSMR_FSTT10_Pos)                  /**< (PMC_FSMR) Fast Startup Input Enable 10 Mask */
#define PMC_FSMR_FSTT10                     PMC_FSMR_FSTT10_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT10_Msk instead */
#define PMC_FSMR_FSTT11_Pos                 11                                             /**< (PMC_FSMR) Fast Startup Input Enable 11 Position */
#define PMC_FSMR_FSTT11_Msk                 (0x1U << PMC_FSMR_FSTT11_Pos)                  /**< (PMC_FSMR) Fast Startup Input Enable 11 Mask */
#define PMC_FSMR_FSTT11                     PMC_FSMR_FSTT11_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT11_Msk instead */
#define PMC_FSMR_FSTT12_Pos                 12                                             /**< (PMC_FSMR) Fast Startup Input Enable 12 Position */
#define PMC_FSMR_FSTT12_Msk                 (0x1U << PMC_FSMR_FSTT12_Pos)                  /**< (PMC_FSMR) Fast Startup Input Enable 12 Mask */
#define PMC_FSMR_FSTT12                     PMC_FSMR_FSTT12_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT12_Msk instead */
#define PMC_FSMR_FSTT13_Pos                 13                                             /**< (PMC_FSMR) Fast Startup Input Enable 13 Position */
#define PMC_FSMR_FSTT13_Msk                 (0x1U << PMC_FSMR_FSTT13_Pos)                  /**< (PMC_FSMR) Fast Startup Input Enable 13 Mask */
#define PMC_FSMR_FSTT13                     PMC_FSMR_FSTT13_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT13_Msk instead */
#define PMC_FSMR_FSTT14_Pos                 14                                             /**< (PMC_FSMR) Fast Startup Input Enable 14 Position */
#define PMC_FSMR_FSTT14_Msk                 (0x1U << PMC_FSMR_FSTT14_Pos)                  /**< (PMC_FSMR) Fast Startup Input Enable 14 Mask */
#define PMC_FSMR_FSTT14                     PMC_FSMR_FSTT14_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT14_Msk instead */
#define PMC_FSMR_FSTT15_Pos                 15                                             /**< (PMC_FSMR) Fast Startup Input Enable 15 Position */
#define PMC_FSMR_FSTT15_Msk                 (0x1U << PMC_FSMR_FSTT15_Pos)                  /**< (PMC_FSMR) Fast Startup Input Enable 15 Mask */
#define PMC_FSMR_FSTT15                     PMC_FSMR_FSTT15_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FSTT15_Msk instead */
#define PMC_FSMR_RTTAL_Pos                  16                                             /**< (PMC_FSMR) RTT Alarm Enable Position */
#define PMC_FSMR_RTTAL_Msk                  (0x1U << PMC_FSMR_RTTAL_Pos)                   /**< (PMC_FSMR) RTT Alarm Enable Mask */
#define PMC_FSMR_RTTAL                      PMC_FSMR_RTTAL_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_RTTAL_Msk instead */
#define PMC_FSMR_RTCAL_Pos                  17                                             /**< (PMC_FSMR) RTC Alarm Enable Position */
#define PMC_FSMR_RTCAL_Msk                  (0x1U << PMC_FSMR_RTCAL_Pos)                   /**< (PMC_FSMR) RTC Alarm Enable Mask */
#define PMC_FSMR_RTCAL                      PMC_FSMR_RTCAL_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_RTCAL_Msk instead */
#define PMC_FSMR_USBAL_Pos                  18                                             /**< (PMC_FSMR) USB Alarm Enable Position */
#define PMC_FSMR_USBAL_Msk                  (0x1U << PMC_FSMR_USBAL_Pos)                   /**< (PMC_FSMR) USB Alarm Enable Mask */
#define PMC_FSMR_USBAL                      PMC_FSMR_USBAL_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_USBAL_Msk instead */
#define PMC_FSMR_LPM_Pos                    20                                             /**< (PMC_FSMR) Low-power Mode Position */
#define PMC_FSMR_LPM_Msk                    (0x1U << PMC_FSMR_LPM_Pos)                     /**< (PMC_FSMR) Low-power Mode Mask */
#define PMC_FSMR_LPM                        PMC_FSMR_LPM_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_LPM_Msk instead */
#define PMC_FSMR_FLPM_Pos                   21                                             /**< (PMC_FSMR) Flash Low-power Mode Position */
#define PMC_FSMR_FLPM_Msk                   (0x3U << PMC_FSMR_FLPM_Pos)                    /**< (PMC_FSMR) Flash Low-power Mode Mask */
#define PMC_FSMR_FLPM(value)                (PMC_FSMR_FLPM_Msk & ((value) << PMC_FSMR_FLPM_Pos))
#define   PMC_FSMR_FLPM_FLASH_STANDBY_Val   (0x0U)                                         /**< (PMC_FSMR) Flash is in Standby Mode when system enters Wait Mode  */
#define   PMC_FSMR_FLPM_FLASH_DEEP_POWERDOWN_Val (0x1U)                                         /**< (PMC_FSMR) Flash is in Deep-power-down mode when system enters Wait Mode  */
#define   PMC_FSMR_FLPM_FLASH_IDLE_Val      (0x2U)                                         /**< (PMC_FSMR) Idle mode  */
#define PMC_FSMR_FLPM_FLASH_STANDBY         (PMC_FSMR_FLPM_FLASH_STANDBY_Val << PMC_FSMR_FLPM_Pos)  /**< (PMC_FSMR) Flash is in Standby Mode when system enters Wait Mode Position  */
#define PMC_FSMR_FLPM_FLASH_DEEP_POWERDOWN  (PMC_FSMR_FLPM_FLASH_DEEP_POWERDOWN_Val << PMC_FSMR_FLPM_Pos)  /**< (PMC_FSMR) Flash is in Deep-power-down mode when system enters Wait Mode Position  */
#define PMC_FSMR_FLPM_FLASH_IDLE            (PMC_FSMR_FLPM_FLASH_IDLE_Val << PMC_FSMR_FLPM_Pos)  /**< (PMC_FSMR) Idle mode Position  */
#define PMC_FSMR_FFLPM_Pos                  23                                             /**< (PMC_FSMR) Force Flash Low-power Mode Position */
#define PMC_FSMR_FFLPM_Msk                  (0x1U << PMC_FSMR_FFLPM_Pos)                   /**< (PMC_FSMR) Force Flash Low-power Mode Mask */
#define PMC_FSMR_FFLPM                      PMC_FSMR_FFLPM_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSMR_FFLPM_Msk instead */
#define PMC_FSMR_MASK                       (0xF7FFFFU)                                    /**< \deprecated (PMC_FSMR) Register MASK  (Use PMC_FSMR_Msk instead)  */
#define PMC_FSMR_Msk                        (0xF7FFFFU)                                    /**< (PMC_FSMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_FSPR : (PMC Offset: 0x74) (R/W 32) Fast Startup Polarity Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FSTP0:1;                   /**< bit:      0  Fast Startup Input Polarity 0            */
    uint32_t FSTP1:1;                   /**< bit:      1  Fast Startup Input Polarity 1            */
    uint32_t FSTP2:1;                   /**< bit:      2  Fast Startup Input Polarity 2            */
    uint32_t FSTP3:1;                   /**< bit:      3  Fast Startup Input Polarity 3            */
    uint32_t FSTP4:1;                   /**< bit:      4  Fast Startup Input Polarity 4            */
    uint32_t FSTP5:1;                   /**< bit:      5  Fast Startup Input Polarity 5            */
    uint32_t FSTP6:1;                   /**< bit:      6  Fast Startup Input Polarity 6            */
    uint32_t FSTP7:1;                   /**< bit:      7  Fast Startup Input Polarity 7            */
    uint32_t FSTP8:1;                   /**< bit:      8  Fast Startup Input Polarity 8            */
    uint32_t FSTP9:1;                   /**< bit:      9  Fast Startup Input Polarity 9            */
    uint32_t FSTP10:1;                  /**< bit:     10  Fast Startup Input Polarity 10           */
    uint32_t FSTP11:1;                  /**< bit:     11  Fast Startup Input Polarity 11           */
    uint32_t FSTP12:1;                  /**< bit:     12  Fast Startup Input Polarity 12           */
    uint32_t FSTP13:1;                  /**< bit:     13  Fast Startup Input Polarity 13           */
    uint32_t FSTP14:1;                  /**< bit:     14  Fast Startup Input Polarity 14           */
    uint32_t FSTP15:1;                  /**< bit:     15  Fast Startup Input Polarity 15           */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t FSTP:16;                   /**< bit:  0..15  Fast Startup Input Polarity x5           */
    uint32_t :16;                       /**< bit: 16..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_FSPR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_FSPR_OFFSET                     (0x74)                                        /**<  (PMC_FSPR) Fast Startup Polarity Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_FSPR_FSTP0_Pos                  0                                              /**< (PMC_FSPR) Fast Startup Input Polarity 0 Position */
#define PMC_FSPR_FSTP0_Msk                  (0x1U << PMC_FSPR_FSTP0_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 0 Mask */
#define PMC_FSPR_FSTP0                      PMC_FSPR_FSTP0_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP0_Msk instead */
#define PMC_FSPR_FSTP1_Pos                  1                                              /**< (PMC_FSPR) Fast Startup Input Polarity 1 Position */
#define PMC_FSPR_FSTP1_Msk                  (0x1U << PMC_FSPR_FSTP1_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 1 Mask */
#define PMC_FSPR_FSTP1                      PMC_FSPR_FSTP1_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP1_Msk instead */
#define PMC_FSPR_FSTP2_Pos                  2                                              /**< (PMC_FSPR) Fast Startup Input Polarity 2 Position */
#define PMC_FSPR_FSTP2_Msk                  (0x1U << PMC_FSPR_FSTP2_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 2 Mask */
#define PMC_FSPR_FSTP2                      PMC_FSPR_FSTP2_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP2_Msk instead */
#define PMC_FSPR_FSTP3_Pos                  3                                              /**< (PMC_FSPR) Fast Startup Input Polarity 3 Position */
#define PMC_FSPR_FSTP3_Msk                  (0x1U << PMC_FSPR_FSTP3_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 3 Mask */
#define PMC_FSPR_FSTP3                      PMC_FSPR_FSTP3_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP3_Msk instead */
#define PMC_FSPR_FSTP4_Pos                  4                                              /**< (PMC_FSPR) Fast Startup Input Polarity 4 Position */
#define PMC_FSPR_FSTP4_Msk                  (0x1U << PMC_FSPR_FSTP4_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 4 Mask */
#define PMC_FSPR_FSTP4                      PMC_FSPR_FSTP4_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP4_Msk instead */
#define PMC_FSPR_FSTP5_Pos                  5                                              /**< (PMC_FSPR) Fast Startup Input Polarity 5 Position */
#define PMC_FSPR_FSTP5_Msk                  (0x1U << PMC_FSPR_FSTP5_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 5 Mask */
#define PMC_FSPR_FSTP5                      PMC_FSPR_FSTP5_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP5_Msk instead */
#define PMC_FSPR_FSTP6_Pos                  6                                              /**< (PMC_FSPR) Fast Startup Input Polarity 6 Position */
#define PMC_FSPR_FSTP6_Msk                  (0x1U << PMC_FSPR_FSTP6_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 6 Mask */
#define PMC_FSPR_FSTP6                      PMC_FSPR_FSTP6_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP6_Msk instead */
#define PMC_FSPR_FSTP7_Pos                  7                                              /**< (PMC_FSPR) Fast Startup Input Polarity 7 Position */
#define PMC_FSPR_FSTP7_Msk                  (0x1U << PMC_FSPR_FSTP7_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 7 Mask */
#define PMC_FSPR_FSTP7                      PMC_FSPR_FSTP7_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP7_Msk instead */
#define PMC_FSPR_FSTP8_Pos                  8                                              /**< (PMC_FSPR) Fast Startup Input Polarity 8 Position */
#define PMC_FSPR_FSTP8_Msk                  (0x1U << PMC_FSPR_FSTP8_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 8 Mask */
#define PMC_FSPR_FSTP8                      PMC_FSPR_FSTP8_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP8_Msk instead */
#define PMC_FSPR_FSTP9_Pos                  9                                              /**< (PMC_FSPR) Fast Startup Input Polarity 9 Position */
#define PMC_FSPR_FSTP9_Msk                  (0x1U << PMC_FSPR_FSTP9_Pos)                   /**< (PMC_FSPR) Fast Startup Input Polarity 9 Mask */
#define PMC_FSPR_FSTP9                      PMC_FSPR_FSTP9_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP9_Msk instead */
#define PMC_FSPR_FSTP10_Pos                 10                                             /**< (PMC_FSPR) Fast Startup Input Polarity 10 Position */
#define PMC_FSPR_FSTP10_Msk                 (0x1U << PMC_FSPR_FSTP10_Pos)                  /**< (PMC_FSPR) Fast Startup Input Polarity 10 Mask */
#define PMC_FSPR_FSTP10                     PMC_FSPR_FSTP10_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP10_Msk instead */
#define PMC_FSPR_FSTP11_Pos                 11                                             /**< (PMC_FSPR) Fast Startup Input Polarity 11 Position */
#define PMC_FSPR_FSTP11_Msk                 (0x1U << PMC_FSPR_FSTP11_Pos)                  /**< (PMC_FSPR) Fast Startup Input Polarity 11 Mask */
#define PMC_FSPR_FSTP11                     PMC_FSPR_FSTP11_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP11_Msk instead */
#define PMC_FSPR_FSTP12_Pos                 12                                             /**< (PMC_FSPR) Fast Startup Input Polarity 12 Position */
#define PMC_FSPR_FSTP12_Msk                 (0x1U << PMC_FSPR_FSTP12_Pos)                  /**< (PMC_FSPR) Fast Startup Input Polarity 12 Mask */
#define PMC_FSPR_FSTP12                     PMC_FSPR_FSTP12_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP12_Msk instead */
#define PMC_FSPR_FSTP13_Pos                 13                                             /**< (PMC_FSPR) Fast Startup Input Polarity 13 Position */
#define PMC_FSPR_FSTP13_Msk                 (0x1U << PMC_FSPR_FSTP13_Pos)                  /**< (PMC_FSPR) Fast Startup Input Polarity 13 Mask */
#define PMC_FSPR_FSTP13                     PMC_FSPR_FSTP13_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP13_Msk instead */
#define PMC_FSPR_FSTP14_Pos                 14                                             /**< (PMC_FSPR) Fast Startup Input Polarity 14 Position */
#define PMC_FSPR_FSTP14_Msk                 (0x1U << PMC_FSPR_FSTP14_Pos)                  /**< (PMC_FSPR) Fast Startup Input Polarity 14 Mask */
#define PMC_FSPR_FSTP14                     PMC_FSPR_FSTP14_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP14_Msk instead */
#define PMC_FSPR_FSTP15_Pos                 15                                             /**< (PMC_FSPR) Fast Startup Input Polarity 15 Position */
#define PMC_FSPR_FSTP15_Msk                 (0x1U << PMC_FSPR_FSTP15_Pos)                  /**< (PMC_FSPR) Fast Startup Input Polarity 15 Mask */
#define PMC_FSPR_FSTP15                     PMC_FSPR_FSTP15_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FSPR_FSTP15_Msk instead */
#define PMC_FSPR_FSTP_Pos                   0                                              /**< (PMC_FSPR Position) Fast Startup Input Polarity x5 */
#define PMC_FSPR_FSTP_Msk                   (0xFFFFU << PMC_FSPR_FSTP_Pos)                 /**< (PMC_FSPR Mask) FSTP */
#define PMC_FSPR_FSTP(value)                (PMC_FSPR_FSTP_Msk & ((value) << PMC_FSPR_FSTP_Pos))  
#define PMC_FSPR_MASK                       (0xFFFFU)                                      /**< \deprecated (PMC_FSPR) Register MASK  (Use PMC_FSPR_Msk instead)  */
#define PMC_FSPR_Msk                        (0xFFFFU)                                      /**< (PMC_FSPR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_FOCR : (PMC Offset: 0x78) (/W 32) Fault Output Clear Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FOCLR:1;                   /**< bit:      0  Fault Output Clear                       */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_FOCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_FOCR_OFFSET                     (0x78)                                        /**<  (PMC_FOCR) Fault Output Clear Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_FOCR_FOCLR_Pos                  0                                              /**< (PMC_FOCR) Fault Output Clear Position */
#define PMC_FOCR_FOCLR_Msk                  (0x1U << PMC_FOCR_FOCLR_Pos)                   /**< (PMC_FOCR) Fault Output Clear Mask */
#define PMC_FOCR_FOCLR                      PMC_FOCR_FOCLR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_FOCR_FOCLR_Msk instead */
#define PMC_FOCR_MASK                       (0x01U)                                        /**< \deprecated (PMC_FOCR) Register MASK  (Use PMC_FOCR_Msk instead)  */
#define PMC_FOCR_Msk                        (0x01U)                                        /**< (PMC_FOCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_WPMR : (PMC Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_WPMR_OFFSET                     (0xE4)                                        /**<  (PMC_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_WPMR_WPEN_Pos                   0                                              /**< (PMC_WPMR) Write Protection Enable Position */
#define PMC_WPMR_WPEN_Msk                   (0x1U << PMC_WPMR_WPEN_Pos)                    /**< (PMC_WPMR) Write Protection Enable Mask */
#define PMC_WPMR_WPEN                       PMC_WPMR_WPEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_WPMR_WPEN_Msk instead */
#define PMC_WPMR_WPKEY_Pos                  8                                              /**< (PMC_WPMR) Write Protection Key Position */
#define PMC_WPMR_WPKEY_Msk                  (0xFFFFFFU << PMC_WPMR_WPKEY_Pos)              /**< (PMC_WPMR) Write Protection Key Mask */
#define PMC_WPMR_WPKEY(value)               (PMC_WPMR_WPKEY_Msk & ((value) << PMC_WPMR_WPKEY_Pos))
#define   PMC_WPMR_WPKEY_PASSWD_Val         (0x504D43U)                                    /**< (PMC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0.  */
#define PMC_WPMR_WPKEY_PASSWD               (PMC_WPMR_WPKEY_PASSWD_Val << PMC_WPMR_WPKEY_Pos)  /**< (PMC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0. Position  */
#define PMC_WPMR_MASK                       (0xFFFFFF01U)                                  /**< \deprecated (PMC_WPMR) Register MASK  (Use PMC_WPMR_Msk instead)  */
#define PMC_WPMR_Msk                        (0xFFFFFF01U)                                  /**< (PMC_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_WPSR : (PMC Offset: 0xe8) (R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:16;                 /**< bit:  8..23  Write Protection Violation Source        */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_WPSR_OFFSET                     (0xE8)                                        /**<  (PMC_WPSR) Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_WPSR_WPVS_Pos                   0                                              /**< (PMC_WPSR) Write Protection Violation Status Position */
#define PMC_WPSR_WPVS_Msk                   (0x1U << PMC_WPSR_WPVS_Pos)                    /**< (PMC_WPSR) Write Protection Violation Status Mask */
#define PMC_WPSR_WPVS                       PMC_WPSR_WPVS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_WPSR_WPVS_Msk instead */
#define PMC_WPSR_WPVSRC_Pos                 8                                              /**< (PMC_WPSR) Write Protection Violation Source Position */
#define PMC_WPSR_WPVSRC_Msk                 (0xFFFFU << PMC_WPSR_WPVSRC_Pos)               /**< (PMC_WPSR) Write Protection Violation Source Mask */
#define PMC_WPSR_WPVSRC(value)              (PMC_WPSR_WPVSRC_Msk & ((value) << PMC_WPSR_WPVSRC_Pos))
#define PMC_WPSR_MASK                       (0xFFFF01U)                                    /**< \deprecated (PMC_WPSR) Register MASK  (Use PMC_WPSR_Msk instead)  */
#define PMC_WPSR_Msk                        (0xFFFF01U)                                    /**< (PMC_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PCER1 : (PMC Offset: 0x100) (/W 32) Peripheral Clock Enable Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PID32:1;                   /**< bit:      0  Peripheral Clock 32 Enable               */
    uint32_t PID33:1;                   /**< bit:      1  Peripheral Clock 33 Enable               */
    uint32_t PID34:1;                   /**< bit:      2  Peripheral Clock 34 Enable               */
    uint32_t PID35:1;                   /**< bit:      3  Peripheral Clock 35 Enable               */
    uint32_t :1;                        /**< bit:      4  Reserved */
    uint32_t PID37:1;                   /**< bit:      5  Peripheral Clock 37 Enable               */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t PID39:1;                   /**< bit:      7  Peripheral Clock 39 Enable               */
    uint32_t PID40:1;                   /**< bit:      8  Peripheral Clock 40 Enable               */
    uint32_t PID41:1;                   /**< bit:      9  Peripheral Clock 41 Enable               */
    uint32_t PID42:1;                   /**< bit:     10  Peripheral Clock 42 Enable               */
    uint32_t PID43:1;                   /**< bit:     11  Peripheral Clock 43 Enable               */
    uint32_t PID44:1;                   /**< bit:     12  Peripheral Clock 44 Enable               */
    uint32_t PID45:1;                   /**< bit:     13  Peripheral Clock 45 Enable               */
    uint32_t PID46:1;                   /**< bit:     14  Peripheral Clock 46 Enable               */
    uint32_t PID47:1;                   /**< bit:     15  Peripheral Clock 47 Enable               */
    uint32_t PID48:1;                   /**< bit:     16  Peripheral Clock 48 Enable               */
    uint32_t PID49:1;                   /**< bit:     17  Peripheral Clock 49 Enable               */
    uint32_t PID50:1;                   /**< bit:     18  Peripheral Clock 50 Enable               */
    uint32_t PID51:1;                   /**< bit:     19  Peripheral Clock 51 Enable               */
    uint32_t PID52:1;                   /**< bit:     20  Peripheral Clock 52 Enable               */
    uint32_t PID53:1;                   /**< bit:     21  Peripheral Clock 53 Enable               */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t PID56:1;                   /**< bit:     24  Peripheral Clock 56 Enable               */
    uint32_t PID57:1;                   /**< bit:     25  Peripheral Clock 57 Enable               */
    uint32_t PID58:1;                   /**< bit:     26  Peripheral Clock 58 Enable               */
    uint32_t PID59:1;                   /**< bit:     27  Peripheral Clock 59 Enable               */
    uint32_t PID60:1;                   /**< bit:     28  Peripheral Clock 60 Enable               */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t PID:25;                    /**< bit:  0..24  Peripheral Clock 6x Enable               */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PCER1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PCER1_OFFSET                    (0x100)                                       /**<  (PMC_PCER1) Peripheral Clock Enable Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PCER1_PID32_Pos                 0                                              /**< (PMC_PCER1) Peripheral Clock 32 Enable Position */
#define PMC_PCER1_PID32_Msk                 (0x1U << PMC_PCER1_PID32_Pos)                  /**< (PMC_PCER1) Peripheral Clock 32 Enable Mask */
#define PMC_PCER1_PID32                     PMC_PCER1_PID32_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID32_Msk instead */
#define PMC_PCER1_PID33_Pos                 1                                              /**< (PMC_PCER1) Peripheral Clock 33 Enable Position */
#define PMC_PCER1_PID33_Msk                 (0x1U << PMC_PCER1_PID33_Pos)                  /**< (PMC_PCER1) Peripheral Clock 33 Enable Mask */
#define PMC_PCER1_PID33                     PMC_PCER1_PID33_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID33_Msk instead */
#define PMC_PCER1_PID34_Pos                 2                                              /**< (PMC_PCER1) Peripheral Clock 34 Enable Position */
#define PMC_PCER1_PID34_Msk                 (0x1U << PMC_PCER1_PID34_Pos)                  /**< (PMC_PCER1) Peripheral Clock 34 Enable Mask */
#define PMC_PCER1_PID34                     PMC_PCER1_PID34_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID34_Msk instead */
#define PMC_PCER1_PID35_Pos                 3                                              /**< (PMC_PCER1) Peripheral Clock 35 Enable Position */
#define PMC_PCER1_PID35_Msk                 (0x1U << PMC_PCER1_PID35_Pos)                  /**< (PMC_PCER1) Peripheral Clock 35 Enable Mask */
#define PMC_PCER1_PID35                     PMC_PCER1_PID35_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID35_Msk instead */
#define PMC_PCER1_PID37_Pos                 5                                              /**< (PMC_PCER1) Peripheral Clock 37 Enable Position */
#define PMC_PCER1_PID37_Msk                 (0x1U << PMC_PCER1_PID37_Pos)                  /**< (PMC_PCER1) Peripheral Clock 37 Enable Mask */
#define PMC_PCER1_PID37                     PMC_PCER1_PID37_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID37_Msk instead */
#define PMC_PCER1_PID39_Pos                 7                                              /**< (PMC_PCER1) Peripheral Clock 39 Enable Position */
#define PMC_PCER1_PID39_Msk                 (0x1U << PMC_PCER1_PID39_Pos)                  /**< (PMC_PCER1) Peripheral Clock 39 Enable Mask */
#define PMC_PCER1_PID39                     PMC_PCER1_PID39_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID39_Msk instead */
#define PMC_PCER1_PID40_Pos                 8                                              /**< (PMC_PCER1) Peripheral Clock 40 Enable Position */
#define PMC_PCER1_PID40_Msk                 (0x1U << PMC_PCER1_PID40_Pos)                  /**< (PMC_PCER1) Peripheral Clock 40 Enable Mask */
#define PMC_PCER1_PID40                     PMC_PCER1_PID40_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID40_Msk instead */
#define PMC_PCER1_PID41_Pos                 9                                              /**< (PMC_PCER1) Peripheral Clock 41 Enable Position */
#define PMC_PCER1_PID41_Msk                 (0x1U << PMC_PCER1_PID41_Pos)                  /**< (PMC_PCER1) Peripheral Clock 41 Enable Mask */
#define PMC_PCER1_PID41                     PMC_PCER1_PID41_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID41_Msk instead */
#define PMC_PCER1_PID42_Pos                 10                                             /**< (PMC_PCER1) Peripheral Clock 42 Enable Position */
#define PMC_PCER1_PID42_Msk                 (0x1U << PMC_PCER1_PID42_Pos)                  /**< (PMC_PCER1) Peripheral Clock 42 Enable Mask */
#define PMC_PCER1_PID42                     PMC_PCER1_PID42_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID42_Msk instead */
#define PMC_PCER1_PID43_Pos                 11                                             /**< (PMC_PCER1) Peripheral Clock 43 Enable Position */
#define PMC_PCER1_PID43_Msk                 (0x1U << PMC_PCER1_PID43_Pos)                  /**< (PMC_PCER1) Peripheral Clock 43 Enable Mask */
#define PMC_PCER1_PID43                     PMC_PCER1_PID43_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID43_Msk instead */
#define PMC_PCER1_PID44_Pos                 12                                             /**< (PMC_PCER1) Peripheral Clock 44 Enable Position */
#define PMC_PCER1_PID44_Msk                 (0x1U << PMC_PCER1_PID44_Pos)                  /**< (PMC_PCER1) Peripheral Clock 44 Enable Mask */
#define PMC_PCER1_PID44                     PMC_PCER1_PID44_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID44_Msk instead */
#define PMC_PCER1_PID45_Pos                 13                                             /**< (PMC_PCER1) Peripheral Clock 45 Enable Position */
#define PMC_PCER1_PID45_Msk                 (0x1U << PMC_PCER1_PID45_Pos)                  /**< (PMC_PCER1) Peripheral Clock 45 Enable Mask */
#define PMC_PCER1_PID45                     PMC_PCER1_PID45_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID45_Msk instead */
#define PMC_PCER1_PID46_Pos                 14                                             /**< (PMC_PCER1) Peripheral Clock 46 Enable Position */
#define PMC_PCER1_PID46_Msk                 (0x1U << PMC_PCER1_PID46_Pos)                  /**< (PMC_PCER1) Peripheral Clock 46 Enable Mask */
#define PMC_PCER1_PID46                     PMC_PCER1_PID46_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID46_Msk instead */
#define PMC_PCER1_PID47_Pos                 15                                             /**< (PMC_PCER1) Peripheral Clock 47 Enable Position */
#define PMC_PCER1_PID47_Msk                 (0x1U << PMC_PCER1_PID47_Pos)                  /**< (PMC_PCER1) Peripheral Clock 47 Enable Mask */
#define PMC_PCER1_PID47                     PMC_PCER1_PID47_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID47_Msk instead */
#define PMC_PCER1_PID48_Pos                 16                                             /**< (PMC_PCER1) Peripheral Clock 48 Enable Position */
#define PMC_PCER1_PID48_Msk                 (0x1U << PMC_PCER1_PID48_Pos)                  /**< (PMC_PCER1) Peripheral Clock 48 Enable Mask */
#define PMC_PCER1_PID48                     PMC_PCER1_PID48_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID48_Msk instead */
#define PMC_PCER1_PID49_Pos                 17                                             /**< (PMC_PCER1) Peripheral Clock 49 Enable Position */
#define PMC_PCER1_PID49_Msk                 (0x1U << PMC_PCER1_PID49_Pos)                  /**< (PMC_PCER1) Peripheral Clock 49 Enable Mask */
#define PMC_PCER1_PID49                     PMC_PCER1_PID49_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID49_Msk instead */
#define PMC_PCER1_PID50_Pos                 18                                             /**< (PMC_PCER1) Peripheral Clock 50 Enable Position */
#define PMC_PCER1_PID50_Msk                 (0x1U << PMC_PCER1_PID50_Pos)                  /**< (PMC_PCER1) Peripheral Clock 50 Enable Mask */
#define PMC_PCER1_PID50                     PMC_PCER1_PID50_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID50_Msk instead */
#define PMC_PCER1_PID51_Pos                 19                                             /**< (PMC_PCER1) Peripheral Clock 51 Enable Position */
#define PMC_PCER1_PID51_Msk                 (0x1U << PMC_PCER1_PID51_Pos)                  /**< (PMC_PCER1) Peripheral Clock 51 Enable Mask */
#define PMC_PCER1_PID51                     PMC_PCER1_PID51_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID51_Msk instead */
#define PMC_PCER1_PID52_Pos                 20                                             /**< (PMC_PCER1) Peripheral Clock 52 Enable Position */
#define PMC_PCER1_PID52_Msk                 (0x1U << PMC_PCER1_PID52_Pos)                  /**< (PMC_PCER1) Peripheral Clock 52 Enable Mask */
#define PMC_PCER1_PID52                     PMC_PCER1_PID52_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID52_Msk instead */
#define PMC_PCER1_PID53_Pos                 21                                             /**< (PMC_PCER1) Peripheral Clock 53 Enable Position */
#define PMC_PCER1_PID53_Msk                 (0x1U << PMC_PCER1_PID53_Pos)                  /**< (PMC_PCER1) Peripheral Clock 53 Enable Mask */
#define PMC_PCER1_PID53                     PMC_PCER1_PID53_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID53_Msk instead */
#define PMC_PCER1_PID56_Pos                 24                                             /**< (PMC_PCER1) Peripheral Clock 56 Enable Position */
#define PMC_PCER1_PID56_Msk                 (0x1U << PMC_PCER1_PID56_Pos)                  /**< (PMC_PCER1) Peripheral Clock 56 Enable Mask */
#define PMC_PCER1_PID56                     PMC_PCER1_PID56_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID56_Msk instead */
#define PMC_PCER1_PID57_Pos                 25                                             /**< (PMC_PCER1) Peripheral Clock 57 Enable Position */
#define PMC_PCER1_PID57_Msk                 (0x1U << PMC_PCER1_PID57_Pos)                  /**< (PMC_PCER1) Peripheral Clock 57 Enable Mask */
#define PMC_PCER1_PID57                     PMC_PCER1_PID57_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID57_Msk instead */
#define PMC_PCER1_PID58_Pos                 26                                             /**< (PMC_PCER1) Peripheral Clock 58 Enable Position */
#define PMC_PCER1_PID58_Msk                 (0x1U << PMC_PCER1_PID58_Pos)                  /**< (PMC_PCER1) Peripheral Clock 58 Enable Mask */
#define PMC_PCER1_PID58                     PMC_PCER1_PID58_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID58_Msk instead */
#define PMC_PCER1_PID59_Pos                 27                                             /**< (PMC_PCER1) Peripheral Clock 59 Enable Position */
#define PMC_PCER1_PID59_Msk                 (0x1U << PMC_PCER1_PID59_Pos)                  /**< (PMC_PCER1) Peripheral Clock 59 Enable Mask */
#define PMC_PCER1_PID59                     PMC_PCER1_PID59_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID59_Msk instead */
#define PMC_PCER1_PID60_Pos                 28                                             /**< (PMC_PCER1) Peripheral Clock 60 Enable Position */
#define PMC_PCER1_PID60_Msk                 (0x1U << PMC_PCER1_PID60_Pos)                  /**< (PMC_PCER1) Peripheral Clock 60 Enable Mask */
#define PMC_PCER1_PID60                     PMC_PCER1_PID60_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCER1_PID60_Msk instead */
#define PMC_PCER1_PID_Pos                   0                                              /**< (PMC_PCER1 Position) Peripheral Clock 6x Enable */
#define PMC_PCER1_PID_Msk                   (0x1FFFFFFU << PMC_PCER1_PID_Pos)              /**< (PMC_PCER1 Mask) PID */
#define PMC_PCER1_PID(value)                (PMC_PCER1_PID_Msk & ((value) << PMC_PCER1_PID_Pos))  
#define PMC_PCER1_MASK                      (0x1F3FFFAFU)                                  /**< \deprecated (PMC_PCER1) Register MASK  (Use PMC_PCER1_Msk instead)  */
#define PMC_PCER1_Msk                       (0x1F3FFFAFU)                                  /**< (PMC_PCER1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PCDR1 : (PMC Offset: 0x104) (/W 32) Peripheral Clock Disable Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PID32:1;                   /**< bit:      0  Peripheral Clock 32 Disable              */
    uint32_t PID33:1;                   /**< bit:      1  Peripheral Clock 33 Disable              */
    uint32_t PID34:1;                   /**< bit:      2  Peripheral Clock 34 Disable              */
    uint32_t PID35:1;                   /**< bit:      3  Peripheral Clock 35 Disable              */
    uint32_t :1;                        /**< bit:      4  Reserved */
    uint32_t PID37:1;                   /**< bit:      5  Peripheral Clock 37 Disable              */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t PID39:1;                   /**< bit:      7  Peripheral Clock 39 Disable              */
    uint32_t PID40:1;                   /**< bit:      8  Peripheral Clock 40 Disable              */
    uint32_t PID41:1;                   /**< bit:      9  Peripheral Clock 41 Disable              */
    uint32_t PID42:1;                   /**< bit:     10  Peripheral Clock 42 Disable              */
    uint32_t PID43:1;                   /**< bit:     11  Peripheral Clock 43 Disable              */
    uint32_t PID44:1;                   /**< bit:     12  Peripheral Clock 44 Disable              */
    uint32_t PID45:1;                   /**< bit:     13  Peripheral Clock 45 Disable              */
    uint32_t PID46:1;                   /**< bit:     14  Peripheral Clock 46 Disable              */
    uint32_t PID47:1;                   /**< bit:     15  Peripheral Clock 47 Disable              */
    uint32_t PID48:1;                   /**< bit:     16  Peripheral Clock 48 Disable              */
    uint32_t PID49:1;                   /**< bit:     17  Peripheral Clock 49 Disable              */
    uint32_t PID50:1;                   /**< bit:     18  Peripheral Clock 50 Disable              */
    uint32_t PID51:1;                   /**< bit:     19  Peripheral Clock 51 Disable              */
    uint32_t PID52:1;                   /**< bit:     20  Peripheral Clock 52 Disable              */
    uint32_t PID53:1;                   /**< bit:     21  Peripheral Clock 53 Disable              */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t PID56:1;                   /**< bit:     24  Peripheral Clock 56 Disable              */
    uint32_t PID57:1;                   /**< bit:     25  Peripheral Clock 57 Disable              */
    uint32_t PID58:1;                   /**< bit:     26  Peripheral Clock 58 Disable              */
    uint32_t PID59:1;                   /**< bit:     27  Peripheral Clock 59 Disable              */
    uint32_t PID60:1;                   /**< bit:     28  Peripheral Clock 60 Disable              */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t PID:25;                    /**< bit:  0..24  Peripheral Clock 6x Disable              */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PCDR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PCDR1_OFFSET                    (0x104)                                       /**<  (PMC_PCDR1) Peripheral Clock Disable Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PCDR1_PID32_Pos                 0                                              /**< (PMC_PCDR1) Peripheral Clock 32 Disable Position */
#define PMC_PCDR1_PID32_Msk                 (0x1U << PMC_PCDR1_PID32_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 32 Disable Mask */
#define PMC_PCDR1_PID32                     PMC_PCDR1_PID32_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID32_Msk instead */
#define PMC_PCDR1_PID33_Pos                 1                                              /**< (PMC_PCDR1) Peripheral Clock 33 Disable Position */
#define PMC_PCDR1_PID33_Msk                 (0x1U << PMC_PCDR1_PID33_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 33 Disable Mask */
#define PMC_PCDR1_PID33                     PMC_PCDR1_PID33_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID33_Msk instead */
#define PMC_PCDR1_PID34_Pos                 2                                              /**< (PMC_PCDR1) Peripheral Clock 34 Disable Position */
#define PMC_PCDR1_PID34_Msk                 (0x1U << PMC_PCDR1_PID34_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 34 Disable Mask */
#define PMC_PCDR1_PID34                     PMC_PCDR1_PID34_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID34_Msk instead */
#define PMC_PCDR1_PID35_Pos                 3                                              /**< (PMC_PCDR1) Peripheral Clock 35 Disable Position */
#define PMC_PCDR1_PID35_Msk                 (0x1U << PMC_PCDR1_PID35_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 35 Disable Mask */
#define PMC_PCDR1_PID35                     PMC_PCDR1_PID35_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID35_Msk instead */
#define PMC_PCDR1_PID37_Pos                 5                                              /**< (PMC_PCDR1) Peripheral Clock 37 Disable Position */
#define PMC_PCDR1_PID37_Msk                 (0x1U << PMC_PCDR1_PID37_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 37 Disable Mask */
#define PMC_PCDR1_PID37                     PMC_PCDR1_PID37_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID37_Msk instead */
#define PMC_PCDR1_PID39_Pos                 7                                              /**< (PMC_PCDR1) Peripheral Clock 39 Disable Position */
#define PMC_PCDR1_PID39_Msk                 (0x1U << PMC_PCDR1_PID39_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 39 Disable Mask */
#define PMC_PCDR1_PID39                     PMC_PCDR1_PID39_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID39_Msk instead */
#define PMC_PCDR1_PID40_Pos                 8                                              /**< (PMC_PCDR1) Peripheral Clock 40 Disable Position */
#define PMC_PCDR1_PID40_Msk                 (0x1U << PMC_PCDR1_PID40_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 40 Disable Mask */
#define PMC_PCDR1_PID40                     PMC_PCDR1_PID40_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID40_Msk instead */
#define PMC_PCDR1_PID41_Pos                 9                                              /**< (PMC_PCDR1) Peripheral Clock 41 Disable Position */
#define PMC_PCDR1_PID41_Msk                 (0x1U << PMC_PCDR1_PID41_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 41 Disable Mask */
#define PMC_PCDR1_PID41                     PMC_PCDR1_PID41_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID41_Msk instead */
#define PMC_PCDR1_PID42_Pos                 10                                             /**< (PMC_PCDR1) Peripheral Clock 42 Disable Position */
#define PMC_PCDR1_PID42_Msk                 (0x1U << PMC_PCDR1_PID42_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 42 Disable Mask */
#define PMC_PCDR1_PID42                     PMC_PCDR1_PID42_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID42_Msk instead */
#define PMC_PCDR1_PID43_Pos                 11                                             /**< (PMC_PCDR1) Peripheral Clock 43 Disable Position */
#define PMC_PCDR1_PID43_Msk                 (0x1U << PMC_PCDR1_PID43_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 43 Disable Mask */
#define PMC_PCDR1_PID43                     PMC_PCDR1_PID43_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID43_Msk instead */
#define PMC_PCDR1_PID44_Pos                 12                                             /**< (PMC_PCDR1) Peripheral Clock 44 Disable Position */
#define PMC_PCDR1_PID44_Msk                 (0x1U << PMC_PCDR1_PID44_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 44 Disable Mask */
#define PMC_PCDR1_PID44                     PMC_PCDR1_PID44_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID44_Msk instead */
#define PMC_PCDR1_PID45_Pos                 13                                             /**< (PMC_PCDR1) Peripheral Clock 45 Disable Position */
#define PMC_PCDR1_PID45_Msk                 (0x1U << PMC_PCDR1_PID45_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 45 Disable Mask */
#define PMC_PCDR1_PID45                     PMC_PCDR1_PID45_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID45_Msk instead */
#define PMC_PCDR1_PID46_Pos                 14                                             /**< (PMC_PCDR1) Peripheral Clock 46 Disable Position */
#define PMC_PCDR1_PID46_Msk                 (0x1U << PMC_PCDR1_PID46_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 46 Disable Mask */
#define PMC_PCDR1_PID46                     PMC_PCDR1_PID46_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID46_Msk instead */
#define PMC_PCDR1_PID47_Pos                 15                                             /**< (PMC_PCDR1) Peripheral Clock 47 Disable Position */
#define PMC_PCDR1_PID47_Msk                 (0x1U << PMC_PCDR1_PID47_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 47 Disable Mask */
#define PMC_PCDR1_PID47                     PMC_PCDR1_PID47_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID47_Msk instead */
#define PMC_PCDR1_PID48_Pos                 16                                             /**< (PMC_PCDR1) Peripheral Clock 48 Disable Position */
#define PMC_PCDR1_PID48_Msk                 (0x1U << PMC_PCDR1_PID48_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 48 Disable Mask */
#define PMC_PCDR1_PID48                     PMC_PCDR1_PID48_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID48_Msk instead */
#define PMC_PCDR1_PID49_Pos                 17                                             /**< (PMC_PCDR1) Peripheral Clock 49 Disable Position */
#define PMC_PCDR1_PID49_Msk                 (0x1U << PMC_PCDR1_PID49_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 49 Disable Mask */
#define PMC_PCDR1_PID49                     PMC_PCDR1_PID49_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID49_Msk instead */
#define PMC_PCDR1_PID50_Pos                 18                                             /**< (PMC_PCDR1) Peripheral Clock 50 Disable Position */
#define PMC_PCDR1_PID50_Msk                 (0x1U << PMC_PCDR1_PID50_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 50 Disable Mask */
#define PMC_PCDR1_PID50                     PMC_PCDR1_PID50_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID50_Msk instead */
#define PMC_PCDR1_PID51_Pos                 19                                             /**< (PMC_PCDR1) Peripheral Clock 51 Disable Position */
#define PMC_PCDR1_PID51_Msk                 (0x1U << PMC_PCDR1_PID51_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 51 Disable Mask */
#define PMC_PCDR1_PID51                     PMC_PCDR1_PID51_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID51_Msk instead */
#define PMC_PCDR1_PID52_Pos                 20                                             /**< (PMC_PCDR1) Peripheral Clock 52 Disable Position */
#define PMC_PCDR1_PID52_Msk                 (0x1U << PMC_PCDR1_PID52_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 52 Disable Mask */
#define PMC_PCDR1_PID52                     PMC_PCDR1_PID52_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID52_Msk instead */
#define PMC_PCDR1_PID53_Pos                 21                                             /**< (PMC_PCDR1) Peripheral Clock 53 Disable Position */
#define PMC_PCDR1_PID53_Msk                 (0x1U << PMC_PCDR1_PID53_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 53 Disable Mask */
#define PMC_PCDR1_PID53                     PMC_PCDR1_PID53_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID53_Msk instead */
#define PMC_PCDR1_PID56_Pos                 24                                             /**< (PMC_PCDR1) Peripheral Clock 56 Disable Position */
#define PMC_PCDR1_PID56_Msk                 (0x1U << PMC_PCDR1_PID56_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 56 Disable Mask */
#define PMC_PCDR1_PID56                     PMC_PCDR1_PID56_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID56_Msk instead */
#define PMC_PCDR1_PID57_Pos                 25                                             /**< (PMC_PCDR1) Peripheral Clock 57 Disable Position */
#define PMC_PCDR1_PID57_Msk                 (0x1U << PMC_PCDR1_PID57_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 57 Disable Mask */
#define PMC_PCDR1_PID57                     PMC_PCDR1_PID57_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID57_Msk instead */
#define PMC_PCDR1_PID58_Pos                 26                                             /**< (PMC_PCDR1) Peripheral Clock 58 Disable Position */
#define PMC_PCDR1_PID58_Msk                 (0x1U << PMC_PCDR1_PID58_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 58 Disable Mask */
#define PMC_PCDR1_PID58                     PMC_PCDR1_PID58_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID58_Msk instead */
#define PMC_PCDR1_PID59_Pos                 27                                             /**< (PMC_PCDR1) Peripheral Clock 59 Disable Position */
#define PMC_PCDR1_PID59_Msk                 (0x1U << PMC_PCDR1_PID59_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 59 Disable Mask */
#define PMC_PCDR1_PID59                     PMC_PCDR1_PID59_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID59_Msk instead */
#define PMC_PCDR1_PID60_Pos                 28                                             /**< (PMC_PCDR1) Peripheral Clock 60 Disable Position */
#define PMC_PCDR1_PID60_Msk                 (0x1U << PMC_PCDR1_PID60_Pos)                  /**< (PMC_PCDR1) Peripheral Clock 60 Disable Mask */
#define PMC_PCDR1_PID60                     PMC_PCDR1_PID60_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCDR1_PID60_Msk instead */
#define PMC_PCDR1_PID_Pos                   0                                              /**< (PMC_PCDR1 Position) Peripheral Clock 6x Disable */
#define PMC_PCDR1_PID_Msk                   (0x1FFFFFFU << PMC_PCDR1_PID_Pos)              /**< (PMC_PCDR1 Mask) PID */
#define PMC_PCDR1_PID(value)                (PMC_PCDR1_PID_Msk & ((value) << PMC_PCDR1_PID_Pos))  
#define PMC_PCDR1_MASK                      (0x1F3FFFAFU)                                  /**< \deprecated (PMC_PCDR1) Register MASK  (Use PMC_PCDR1_Msk instead)  */
#define PMC_PCDR1_Msk                       (0x1F3FFFAFU)                                  /**< (PMC_PCDR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PCSR1 : (PMC Offset: 0x108) (R/ 32) Peripheral Clock Status Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PID32:1;                   /**< bit:      0  Peripheral Clock 32 Status               */
    uint32_t PID33:1;                   /**< bit:      1  Peripheral Clock 33 Status               */
    uint32_t PID34:1;                   /**< bit:      2  Peripheral Clock 34 Status               */
    uint32_t PID35:1;                   /**< bit:      3  Peripheral Clock 35 Status               */
    uint32_t :1;                        /**< bit:      4  Reserved */
    uint32_t PID37:1;                   /**< bit:      5  Peripheral Clock 37 Status               */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t PID39:1;                   /**< bit:      7  Peripheral Clock 39 Status               */
    uint32_t PID40:1;                   /**< bit:      8  Peripheral Clock 40 Status               */
    uint32_t PID41:1;                   /**< bit:      9  Peripheral Clock 41 Status               */
    uint32_t PID42:1;                   /**< bit:     10  Peripheral Clock 42 Status               */
    uint32_t PID43:1;                   /**< bit:     11  Peripheral Clock 43 Status               */
    uint32_t PID44:1;                   /**< bit:     12  Peripheral Clock 44 Status               */
    uint32_t PID45:1;                   /**< bit:     13  Peripheral Clock 45 Status               */
    uint32_t PID46:1;                   /**< bit:     14  Peripheral Clock 46 Status               */
    uint32_t PID47:1;                   /**< bit:     15  Peripheral Clock 47 Status               */
    uint32_t PID48:1;                   /**< bit:     16  Peripheral Clock 48 Status               */
    uint32_t PID49:1;                   /**< bit:     17  Peripheral Clock 49 Status               */
    uint32_t PID50:1;                   /**< bit:     18  Peripheral Clock 50 Status               */
    uint32_t PID51:1;                   /**< bit:     19  Peripheral Clock 51 Status               */
    uint32_t PID52:1;                   /**< bit:     20  Peripheral Clock 52 Status               */
    uint32_t PID53:1;                   /**< bit:     21  Peripheral Clock 53 Status               */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t PID56:1;                   /**< bit:     24  Peripheral Clock 56 Status               */
    uint32_t PID57:1;                   /**< bit:     25  Peripheral Clock 57 Status               */
    uint32_t PID58:1;                   /**< bit:     26  Peripheral Clock 58 Status               */
    uint32_t PID59:1;                   /**< bit:     27  Peripheral Clock 59 Status               */
    uint32_t PID60:1;                   /**< bit:     28  Peripheral Clock 60 Status               */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t PID:25;                    /**< bit:  0..24  Peripheral Clock 6x Status               */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PCSR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PCSR1_OFFSET                    (0x108)                                       /**<  (PMC_PCSR1) Peripheral Clock Status Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PCSR1_PID32_Pos                 0                                              /**< (PMC_PCSR1) Peripheral Clock 32 Status Position */
#define PMC_PCSR1_PID32_Msk                 (0x1U << PMC_PCSR1_PID32_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 32 Status Mask */
#define PMC_PCSR1_PID32                     PMC_PCSR1_PID32_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID32_Msk instead */
#define PMC_PCSR1_PID33_Pos                 1                                              /**< (PMC_PCSR1) Peripheral Clock 33 Status Position */
#define PMC_PCSR1_PID33_Msk                 (0x1U << PMC_PCSR1_PID33_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 33 Status Mask */
#define PMC_PCSR1_PID33                     PMC_PCSR1_PID33_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID33_Msk instead */
#define PMC_PCSR1_PID34_Pos                 2                                              /**< (PMC_PCSR1) Peripheral Clock 34 Status Position */
#define PMC_PCSR1_PID34_Msk                 (0x1U << PMC_PCSR1_PID34_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 34 Status Mask */
#define PMC_PCSR1_PID34                     PMC_PCSR1_PID34_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID34_Msk instead */
#define PMC_PCSR1_PID35_Pos                 3                                              /**< (PMC_PCSR1) Peripheral Clock 35 Status Position */
#define PMC_PCSR1_PID35_Msk                 (0x1U << PMC_PCSR1_PID35_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 35 Status Mask */
#define PMC_PCSR1_PID35                     PMC_PCSR1_PID35_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID35_Msk instead */
#define PMC_PCSR1_PID37_Pos                 5                                              /**< (PMC_PCSR1) Peripheral Clock 37 Status Position */
#define PMC_PCSR1_PID37_Msk                 (0x1U << PMC_PCSR1_PID37_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 37 Status Mask */
#define PMC_PCSR1_PID37                     PMC_PCSR1_PID37_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID37_Msk instead */
#define PMC_PCSR1_PID39_Pos                 7                                              /**< (PMC_PCSR1) Peripheral Clock 39 Status Position */
#define PMC_PCSR1_PID39_Msk                 (0x1U << PMC_PCSR1_PID39_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 39 Status Mask */
#define PMC_PCSR1_PID39                     PMC_PCSR1_PID39_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID39_Msk instead */
#define PMC_PCSR1_PID40_Pos                 8                                              /**< (PMC_PCSR1) Peripheral Clock 40 Status Position */
#define PMC_PCSR1_PID40_Msk                 (0x1U << PMC_PCSR1_PID40_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 40 Status Mask */
#define PMC_PCSR1_PID40                     PMC_PCSR1_PID40_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID40_Msk instead */
#define PMC_PCSR1_PID41_Pos                 9                                              /**< (PMC_PCSR1) Peripheral Clock 41 Status Position */
#define PMC_PCSR1_PID41_Msk                 (0x1U << PMC_PCSR1_PID41_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 41 Status Mask */
#define PMC_PCSR1_PID41                     PMC_PCSR1_PID41_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID41_Msk instead */
#define PMC_PCSR1_PID42_Pos                 10                                             /**< (PMC_PCSR1) Peripheral Clock 42 Status Position */
#define PMC_PCSR1_PID42_Msk                 (0x1U << PMC_PCSR1_PID42_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 42 Status Mask */
#define PMC_PCSR1_PID42                     PMC_PCSR1_PID42_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID42_Msk instead */
#define PMC_PCSR1_PID43_Pos                 11                                             /**< (PMC_PCSR1) Peripheral Clock 43 Status Position */
#define PMC_PCSR1_PID43_Msk                 (0x1U << PMC_PCSR1_PID43_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 43 Status Mask */
#define PMC_PCSR1_PID43                     PMC_PCSR1_PID43_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID43_Msk instead */
#define PMC_PCSR1_PID44_Pos                 12                                             /**< (PMC_PCSR1) Peripheral Clock 44 Status Position */
#define PMC_PCSR1_PID44_Msk                 (0x1U << PMC_PCSR1_PID44_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 44 Status Mask */
#define PMC_PCSR1_PID44                     PMC_PCSR1_PID44_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID44_Msk instead */
#define PMC_PCSR1_PID45_Pos                 13                                             /**< (PMC_PCSR1) Peripheral Clock 45 Status Position */
#define PMC_PCSR1_PID45_Msk                 (0x1U << PMC_PCSR1_PID45_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 45 Status Mask */
#define PMC_PCSR1_PID45                     PMC_PCSR1_PID45_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID45_Msk instead */
#define PMC_PCSR1_PID46_Pos                 14                                             /**< (PMC_PCSR1) Peripheral Clock 46 Status Position */
#define PMC_PCSR1_PID46_Msk                 (0x1U << PMC_PCSR1_PID46_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 46 Status Mask */
#define PMC_PCSR1_PID46                     PMC_PCSR1_PID46_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID46_Msk instead */
#define PMC_PCSR1_PID47_Pos                 15                                             /**< (PMC_PCSR1) Peripheral Clock 47 Status Position */
#define PMC_PCSR1_PID47_Msk                 (0x1U << PMC_PCSR1_PID47_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 47 Status Mask */
#define PMC_PCSR1_PID47                     PMC_PCSR1_PID47_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID47_Msk instead */
#define PMC_PCSR1_PID48_Pos                 16                                             /**< (PMC_PCSR1) Peripheral Clock 48 Status Position */
#define PMC_PCSR1_PID48_Msk                 (0x1U << PMC_PCSR1_PID48_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 48 Status Mask */
#define PMC_PCSR1_PID48                     PMC_PCSR1_PID48_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID48_Msk instead */
#define PMC_PCSR1_PID49_Pos                 17                                             /**< (PMC_PCSR1) Peripheral Clock 49 Status Position */
#define PMC_PCSR1_PID49_Msk                 (0x1U << PMC_PCSR1_PID49_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 49 Status Mask */
#define PMC_PCSR1_PID49                     PMC_PCSR1_PID49_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID49_Msk instead */
#define PMC_PCSR1_PID50_Pos                 18                                             /**< (PMC_PCSR1) Peripheral Clock 50 Status Position */
#define PMC_PCSR1_PID50_Msk                 (0x1U << PMC_PCSR1_PID50_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 50 Status Mask */
#define PMC_PCSR1_PID50                     PMC_PCSR1_PID50_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID50_Msk instead */
#define PMC_PCSR1_PID51_Pos                 19                                             /**< (PMC_PCSR1) Peripheral Clock 51 Status Position */
#define PMC_PCSR1_PID51_Msk                 (0x1U << PMC_PCSR1_PID51_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 51 Status Mask */
#define PMC_PCSR1_PID51                     PMC_PCSR1_PID51_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID51_Msk instead */
#define PMC_PCSR1_PID52_Pos                 20                                             /**< (PMC_PCSR1) Peripheral Clock 52 Status Position */
#define PMC_PCSR1_PID52_Msk                 (0x1U << PMC_PCSR1_PID52_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 52 Status Mask */
#define PMC_PCSR1_PID52                     PMC_PCSR1_PID52_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID52_Msk instead */
#define PMC_PCSR1_PID53_Pos                 21                                             /**< (PMC_PCSR1) Peripheral Clock 53 Status Position */
#define PMC_PCSR1_PID53_Msk                 (0x1U << PMC_PCSR1_PID53_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 53 Status Mask */
#define PMC_PCSR1_PID53                     PMC_PCSR1_PID53_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID53_Msk instead */
#define PMC_PCSR1_PID56_Pos                 24                                             /**< (PMC_PCSR1) Peripheral Clock 56 Status Position */
#define PMC_PCSR1_PID56_Msk                 (0x1U << PMC_PCSR1_PID56_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 56 Status Mask */
#define PMC_PCSR1_PID56                     PMC_PCSR1_PID56_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID56_Msk instead */
#define PMC_PCSR1_PID57_Pos                 25                                             /**< (PMC_PCSR1) Peripheral Clock 57 Status Position */
#define PMC_PCSR1_PID57_Msk                 (0x1U << PMC_PCSR1_PID57_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 57 Status Mask */
#define PMC_PCSR1_PID57                     PMC_PCSR1_PID57_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID57_Msk instead */
#define PMC_PCSR1_PID58_Pos                 26                                             /**< (PMC_PCSR1) Peripheral Clock 58 Status Position */
#define PMC_PCSR1_PID58_Msk                 (0x1U << PMC_PCSR1_PID58_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 58 Status Mask */
#define PMC_PCSR1_PID58                     PMC_PCSR1_PID58_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID58_Msk instead */
#define PMC_PCSR1_PID59_Pos                 27                                             /**< (PMC_PCSR1) Peripheral Clock 59 Status Position */
#define PMC_PCSR1_PID59_Msk                 (0x1U << PMC_PCSR1_PID59_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 59 Status Mask */
#define PMC_PCSR1_PID59                     PMC_PCSR1_PID59_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID59_Msk instead */
#define PMC_PCSR1_PID60_Pos                 28                                             /**< (PMC_PCSR1) Peripheral Clock 60 Status Position */
#define PMC_PCSR1_PID60_Msk                 (0x1U << PMC_PCSR1_PID60_Pos)                  /**< (PMC_PCSR1) Peripheral Clock 60 Status Mask */
#define PMC_PCSR1_PID60                     PMC_PCSR1_PID60_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCSR1_PID60_Msk instead */
#define PMC_PCSR1_PID_Pos                   0                                              /**< (PMC_PCSR1 Position) Peripheral Clock 6x Status */
#define PMC_PCSR1_PID_Msk                   (0x1FFFFFFU << PMC_PCSR1_PID_Pos)              /**< (PMC_PCSR1 Mask) PID */
#define PMC_PCSR1_PID(value)                (PMC_PCSR1_PID_Msk & ((value) << PMC_PCSR1_PID_Pos))  
#define PMC_PCSR1_MASK                      (0x1F3FFFAFU)                                  /**< \deprecated (PMC_PCSR1) Register MASK  (Use PMC_PCSR1_Msk instead)  */
#define PMC_PCSR1_Msk                       (0x1F3FFFAFU)                                  /**< (PMC_PCSR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PCR : (PMC Offset: 0x10c) (R/W 32) Peripheral Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PID:7;                     /**< bit:   0..6  Peripheral ID                            */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t GCLKCSS:3;                 /**< bit:  8..10  Generic Clock Source Selection           */
    uint32_t :1;                        /**< bit:     11  Reserved */
    uint32_t CMD:1;                     /**< bit:     12  Command                                  */
    uint32_t :7;                        /**< bit: 13..19  Reserved */
    uint32_t GCLKDIV:8;                 /**< bit: 20..27  Generic Clock Division Ratio             */
    uint32_t EN:1;                      /**< bit:     28  Enable                                   */
    uint32_t GCLKEN:1;                  /**< bit:     29  Generic Clock Enable                     */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PCR_OFFSET                      (0x10C)                                       /**<  (PMC_PCR) Peripheral Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PCR_PID_Pos                     0                                              /**< (PMC_PCR) Peripheral ID Position */
#define PMC_PCR_PID_Msk                     (0x7FU << PMC_PCR_PID_Pos)                     /**< (PMC_PCR) Peripheral ID Mask */
#define PMC_PCR_PID(value)                  (PMC_PCR_PID_Msk & ((value) << PMC_PCR_PID_Pos))
#define PMC_PCR_GCLKCSS_Pos                 8                                              /**< (PMC_PCR) Generic Clock Source Selection Position */
#define PMC_PCR_GCLKCSS_Msk                 (0x7U << PMC_PCR_GCLKCSS_Pos)                  /**< (PMC_PCR) Generic Clock Source Selection Mask */
#define PMC_PCR_GCLKCSS(value)              (PMC_PCR_GCLKCSS_Msk & ((value) << PMC_PCR_GCLKCSS_Pos))
#define   PMC_PCR_GCLKCSS_SLOW_CLK_Val      (0x0U)                                         /**< (PMC_PCR) Slow clock is selected  */
#define   PMC_PCR_GCLKCSS_MAIN_CLK_Val      (0x1U)                                         /**< (PMC_PCR) Main clock is selected  */
#define   PMC_PCR_GCLKCSS_PLLA_CLK_Val      (0x2U)                                         /**< (PMC_PCR) PLLACK is selected  */
#define   PMC_PCR_GCLKCSS_UPLL_CLK_Val      (0x3U)                                         /**< (PMC_PCR) UPLL Clock is selected  */
#define   PMC_PCR_GCLKCSS_MCK_CLK_Val       (0x4U)                                         /**< (PMC_PCR) Master Clock is selected  */
#define PMC_PCR_GCLKCSS_SLOW_CLK            (PMC_PCR_GCLKCSS_SLOW_CLK_Val << PMC_PCR_GCLKCSS_Pos)  /**< (PMC_PCR) Slow clock is selected Position  */
#define PMC_PCR_GCLKCSS_MAIN_CLK            (PMC_PCR_GCLKCSS_MAIN_CLK_Val << PMC_PCR_GCLKCSS_Pos)  /**< (PMC_PCR) Main clock is selected Position  */
#define PMC_PCR_GCLKCSS_PLLA_CLK            (PMC_PCR_GCLKCSS_PLLA_CLK_Val << PMC_PCR_GCLKCSS_Pos)  /**< (PMC_PCR) PLLACK is selected Position  */
#define PMC_PCR_GCLKCSS_UPLL_CLK            (PMC_PCR_GCLKCSS_UPLL_CLK_Val << PMC_PCR_GCLKCSS_Pos)  /**< (PMC_PCR) UPLL Clock is selected Position  */
#define PMC_PCR_GCLKCSS_MCK_CLK             (PMC_PCR_GCLKCSS_MCK_CLK_Val << PMC_PCR_GCLKCSS_Pos)  /**< (PMC_PCR) Master Clock is selected Position  */
#define PMC_PCR_CMD_Pos                     12                                             /**< (PMC_PCR) Command Position */
#define PMC_PCR_CMD_Msk                     (0x1U << PMC_PCR_CMD_Pos)                      /**< (PMC_PCR) Command Mask */
#define PMC_PCR_CMD                         PMC_PCR_CMD_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCR_CMD_Msk instead */
#define PMC_PCR_GCLKDIV_Pos                 20                                             /**< (PMC_PCR) Generic Clock Division Ratio Position */
#define PMC_PCR_GCLKDIV_Msk                 (0xFFU << PMC_PCR_GCLKDIV_Pos)                 /**< (PMC_PCR) Generic Clock Division Ratio Mask */
#define PMC_PCR_GCLKDIV(value)              (PMC_PCR_GCLKDIV_Msk & ((value) << PMC_PCR_GCLKDIV_Pos))
#define PMC_PCR_EN_Pos                      28                                             /**< (PMC_PCR) Enable Position */
#define PMC_PCR_EN_Msk                      (0x1U << PMC_PCR_EN_Pos)                       /**< (PMC_PCR) Enable Mask */
#define PMC_PCR_EN                          PMC_PCR_EN_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCR_EN_Msk instead */
#define PMC_PCR_GCLKEN_Pos                  29                                             /**< (PMC_PCR) Generic Clock Enable Position */
#define PMC_PCR_GCLKEN_Msk                  (0x1U << PMC_PCR_GCLKEN_Pos)                   /**< (PMC_PCR) Generic Clock Enable Mask */
#define PMC_PCR_GCLKEN                      PMC_PCR_GCLKEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_PCR_GCLKEN_Msk instead */
#define PMC_PCR_MASK                        (0x3FF0177FU)                                  /**< \deprecated (PMC_PCR) Register MASK  (Use PMC_PCR_Msk instead)  */
#define PMC_PCR_Msk                         (0x3FF0177FU)                                  /**< (PMC_PCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_OCR : (PMC Offset: 0x110) (R/W 32) Oscillator Calibration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CAL4:7;                    /**< bit:   0..6  RC Oscillator Calibration bits for 4 MHz */
    uint32_t SEL4:1;                    /**< bit:      7  Selection of RC Oscillator Calibration bits for 4 MHz */
    uint32_t CAL8:7;                    /**< bit:  8..14  RC Oscillator Calibration bits for 8 MHz */
    uint32_t SEL8:1;                    /**< bit:     15  Selection of RC Oscillator Calibration bits for 8 MHz */
    uint32_t CAL12:7;                   /**< bit: 16..22  RC Oscillator Calibration bits for 12 MHz */
    uint32_t SEL12:1;                   /**< bit:     23  Selection of RC Oscillator Calibration bits for 12 MHz */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_OCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_OCR_OFFSET                      (0x110)                                       /**<  (PMC_OCR) Oscillator Calibration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_OCR_CAL4_Pos                    0                                              /**< (PMC_OCR) RC Oscillator Calibration bits for 4 MHz Position */
#define PMC_OCR_CAL4_Msk                    (0x7FU << PMC_OCR_CAL4_Pos)                    /**< (PMC_OCR) RC Oscillator Calibration bits for 4 MHz Mask */
#define PMC_OCR_CAL4(value)                 (PMC_OCR_CAL4_Msk & ((value) << PMC_OCR_CAL4_Pos))
#define PMC_OCR_SEL4_Pos                    7                                              /**< (PMC_OCR) Selection of RC Oscillator Calibration bits for 4 MHz Position */
#define PMC_OCR_SEL4_Msk                    (0x1U << PMC_OCR_SEL4_Pos)                     /**< (PMC_OCR) Selection of RC Oscillator Calibration bits for 4 MHz Mask */
#define PMC_OCR_SEL4                        PMC_OCR_SEL4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_OCR_SEL4_Msk instead */
#define PMC_OCR_CAL8_Pos                    8                                              /**< (PMC_OCR) RC Oscillator Calibration bits for 8 MHz Position */
#define PMC_OCR_CAL8_Msk                    (0x7FU << PMC_OCR_CAL8_Pos)                    /**< (PMC_OCR) RC Oscillator Calibration bits for 8 MHz Mask */
#define PMC_OCR_CAL8(value)                 (PMC_OCR_CAL8_Msk & ((value) << PMC_OCR_CAL8_Pos))
#define PMC_OCR_SEL8_Pos                    15                                             /**< (PMC_OCR) Selection of RC Oscillator Calibration bits for 8 MHz Position */
#define PMC_OCR_SEL8_Msk                    (0x1U << PMC_OCR_SEL8_Pos)                     /**< (PMC_OCR) Selection of RC Oscillator Calibration bits for 8 MHz Mask */
#define PMC_OCR_SEL8                        PMC_OCR_SEL8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_OCR_SEL8_Msk instead */
#define PMC_OCR_CAL12_Pos                   16                                             /**< (PMC_OCR) RC Oscillator Calibration bits for 12 MHz Position */
#define PMC_OCR_CAL12_Msk                   (0x7FU << PMC_OCR_CAL12_Pos)                   /**< (PMC_OCR) RC Oscillator Calibration bits for 12 MHz Mask */
#define PMC_OCR_CAL12(value)                (PMC_OCR_CAL12_Msk & ((value) << PMC_OCR_CAL12_Pos))
#define PMC_OCR_SEL12_Pos                   23                                             /**< (PMC_OCR) Selection of RC Oscillator Calibration bits for 12 MHz Position */
#define PMC_OCR_SEL12_Msk                   (0x1U << PMC_OCR_SEL12_Pos)                    /**< (PMC_OCR) Selection of RC Oscillator Calibration bits for 12 MHz Mask */
#define PMC_OCR_SEL12                       PMC_OCR_SEL12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_OCR_SEL12_Msk instead */
#define PMC_OCR_MASK                        (0xFFFFFFU)                                    /**< \deprecated (PMC_OCR) Register MASK  (Use PMC_OCR_Msk instead)  */
#define PMC_OCR_Msk                         (0xFFFFFFU)                                    /**< (PMC_OCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_ER0 : (PMC Offset: 0x114) (/W 32) SleepWalking Enable Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID7:1;                    /**< bit:      7  Peripheral 7 SleepWalking Enable         */
    uint32_t PID8:1;                    /**< bit:      8  Peripheral 8 SleepWalking Enable         */
    uint32_t PID9:1;                    /**< bit:      9  Peripheral 9 SleepWalking Enable         */
    uint32_t PID10:1;                   /**< bit:     10  Peripheral 10 SleepWalking Enable        */
    uint32_t PID11:1;                   /**< bit:     11  Peripheral 11 SleepWalking Enable        */
    uint32_t PID12:1;                   /**< bit:     12  Peripheral 12 SleepWalking Enable        */
    uint32_t PID13:1;                   /**< bit:     13  Peripheral 13 SleepWalking Enable        */
    uint32_t PID14:1;                   /**< bit:     14  Peripheral 14 SleepWalking Enable        */
    uint32_t PID15:1;                   /**< bit:     15  Peripheral 15 SleepWalking Enable        */
    uint32_t PID16:1;                   /**< bit:     16  Peripheral 16 SleepWalking Enable        */
    uint32_t PID17:1;                   /**< bit:     17  Peripheral 17 SleepWalking Enable        */
    uint32_t PID18:1;                   /**< bit:     18  Peripheral 18 SleepWalking Enable        */
    uint32_t PID19:1;                   /**< bit:     19  Peripheral 19 SleepWalking Enable        */
    uint32_t PID20:1;                   /**< bit:     20  Peripheral 20 SleepWalking Enable        */
    uint32_t PID21:1;                   /**< bit:     21  Peripheral 21 SleepWalking Enable        */
    uint32_t PID22:1;                   /**< bit:     22  Peripheral 22 SleepWalking Enable        */
    uint32_t PID23:1;                   /**< bit:     23  Peripheral 23 SleepWalking Enable        */
    uint32_t PID24:1;                   /**< bit:     24  Peripheral 24 SleepWalking Enable        */
    uint32_t PID25:1;                   /**< bit:     25  Peripheral 25 SleepWalking Enable        */
    uint32_t PID26:1;                   /**< bit:     26  Peripheral 26 SleepWalking Enable        */
    uint32_t PID27:1;                   /**< bit:     27  Peripheral 27 SleepWalking Enable        */
    uint32_t PID28:1;                   /**< bit:     28  Peripheral 28 SleepWalking Enable        */
    uint32_t PID29:1;                   /**< bit:     29  Peripheral 29 SleepWalking Enable        */
    uint32_t PID30:1;                   /**< bit:     30  Peripheral 30 SleepWalking Enable        */
    uint32_t PID31:1;                   /**< bit:     31  Peripheral 31 SleepWalking Enable        */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID:25;                    /**< bit:  7..31  Peripheral 3x SleepWalking Enable        */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_ER0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_ER0_OFFSET                (0x114)                                       /**<  (PMC_SLPWK_ER0) SleepWalking Enable Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_ER0_PID7_Pos              7                                              /**< (PMC_SLPWK_ER0) Peripheral 7 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID7_Msk              (0x1U << PMC_SLPWK_ER0_PID7_Pos)               /**< (PMC_SLPWK_ER0) Peripheral 7 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID7                  PMC_SLPWK_ER0_PID7_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID7_Msk instead */
#define PMC_SLPWK_ER0_PID8_Pos              8                                              /**< (PMC_SLPWK_ER0) Peripheral 8 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID8_Msk              (0x1U << PMC_SLPWK_ER0_PID8_Pos)               /**< (PMC_SLPWK_ER0) Peripheral 8 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID8                  PMC_SLPWK_ER0_PID8_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID8_Msk instead */
#define PMC_SLPWK_ER0_PID9_Pos              9                                              /**< (PMC_SLPWK_ER0) Peripheral 9 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID9_Msk              (0x1U << PMC_SLPWK_ER0_PID9_Pos)               /**< (PMC_SLPWK_ER0) Peripheral 9 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID9                  PMC_SLPWK_ER0_PID9_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID9_Msk instead */
#define PMC_SLPWK_ER0_PID10_Pos             10                                             /**< (PMC_SLPWK_ER0) Peripheral 10 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID10_Msk             (0x1U << PMC_SLPWK_ER0_PID10_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 10 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID10                 PMC_SLPWK_ER0_PID10_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID10_Msk instead */
#define PMC_SLPWK_ER0_PID11_Pos             11                                             /**< (PMC_SLPWK_ER0) Peripheral 11 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID11_Msk             (0x1U << PMC_SLPWK_ER0_PID11_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 11 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID11                 PMC_SLPWK_ER0_PID11_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID11_Msk instead */
#define PMC_SLPWK_ER0_PID12_Pos             12                                             /**< (PMC_SLPWK_ER0) Peripheral 12 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID12_Msk             (0x1U << PMC_SLPWK_ER0_PID12_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 12 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID12                 PMC_SLPWK_ER0_PID12_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID12_Msk instead */
#define PMC_SLPWK_ER0_PID13_Pos             13                                             /**< (PMC_SLPWK_ER0) Peripheral 13 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID13_Msk             (0x1U << PMC_SLPWK_ER0_PID13_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 13 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID13                 PMC_SLPWK_ER0_PID13_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID13_Msk instead */
#define PMC_SLPWK_ER0_PID14_Pos             14                                             /**< (PMC_SLPWK_ER0) Peripheral 14 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID14_Msk             (0x1U << PMC_SLPWK_ER0_PID14_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 14 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID14                 PMC_SLPWK_ER0_PID14_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID14_Msk instead */
#define PMC_SLPWK_ER0_PID15_Pos             15                                             /**< (PMC_SLPWK_ER0) Peripheral 15 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID15_Msk             (0x1U << PMC_SLPWK_ER0_PID15_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 15 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID15                 PMC_SLPWK_ER0_PID15_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID15_Msk instead */
#define PMC_SLPWK_ER0_PID16_Pos             16                                             /**< (PMC_SLPWK_ER0) Peripheral 16 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID16_Msk             (0x1U << PMC_SLPWK_ER0_PID16_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 16 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID16                 PMC_SLPWK_ER0_PID16_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID16_Msk instead */
#define PMC_SLPWK_ER0_PID17_Pos             17                                             /**< (PMC_SLPWK_ER0) Peripheral 17 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID17_Msk             (0x1U << PMC_SLPWK_ER0_PID17_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 17 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID17                 PMC_SLPWK_ER0_PID17_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID17_Msk instead */
#define PMC_SLPWK_ER0_PID18_Pos             18                                             /**< (PMC_SLPWK_ER0) Peripheral 18 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID18_Msk             (0x1U << PMC_SLPWK_ER0_PID18_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 18 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID18                 PMC_SLPWK_ER0_PID18_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID18_Msk instead */
#define PMC_SLPWK_ER0_PID19_Pos             19                                             /**< (PMC_SLPWK_ER0) Peripheral 19 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID19_Msk             (0x1U << PMC_SLPWK_ER0_PID19_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 19 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID19                 PMC_SLPWK_ER0_PID19_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID19_Msk instead */
#define PMC_SLPWK_ER0_PID20_Pos             20                                             /**< (PMC_SLPWK_ER0) Peripheral 20 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID20_Msk             (0x1U << PMC_SLPWK_ER0_PID20_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 20 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID20                 PMC_SLPWK_ER0_PID20_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID20_Msk instead */
#define PMC_SLPWK_ER0_PID21_Pos             21                                             /**< (PMC_SLPWK_ER0) Peripheral 21 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID21_Msk             (0x1U << PMC_SLPWK_ER0_PID21_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 21 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID21                 PMC_SLPWK_ER0_PID21_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID21_Msk instead */
#define PMC_SLPWK_ER0_PID22_Pos             22                                             /**< (PMC_SLPWK_ER0) Peripheral 22 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID22_Msk             (0x1U << PMC_SLPWK_ER0_PID22_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 22 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID22                 PMC_SLPWK_ER0_PID22_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID22_Msk instead */
#define PMC_SLPWK_ER0_PID23_Pos             23                                             /**< (PMC_SLPWK_ER0) Peripheral 23 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID23_Msk             (0x1U << PMC_SLPWK_ER0_PID23_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 23 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID23                 PMC_SLPWK_ER0_PID23_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID23_Msk instead */
#define PMC_SLPWK_ER0_PID24_Pos             24                                             /**< (PMC_SLPWK_ER0) Peripheral 24 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID24_Msk             (0x1U << PMC_SLPWK_ER0_PID24_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 24 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID24                 PMC_SLPWK_ER0_PID24_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID24_Msk instead */
#define PMC_SLPWK_ER0_PID25_Pos             25                                             /**< (PMC_SLPWK_ER0) Peripheral 25 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID25_Msk             (0x1U << PMC_SLPWK_ER0_PID25_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 25 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID25                 PMC_SLPWK_ER0_PID25_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID25_Msk instead */
#define PMC_SLPWK_ER0_PID26_Pos             26                                             /**< (PMC_SLPWK_ER0) Peripheral 26 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID26_Msk             (0x1U << PMC_SLPWK_ER0_PID26_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 26 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID26                 PMC_SLPWK_ER0_PID26_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID26_Msk instead */
#define PMC_SLPWK_ER0_PID27_Pos             27                                             /**< (PMC_SLPWK_ER0) Peripheral 27 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID27_Msk             (0x1U << PMC_SLPWK_ER0_PID27_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 27 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID27                 PMC_SLPWK_ER0_PID27_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID27_Msk instead */
#define PMC_SLPWK_ER0_PID28_Pos             28                                             /**< (PMC_SLPWK_ER0) Peripheral 28 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID28_Msk             (0x1U << PMC_SLPWK_ER0_PID28_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 28 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID28                 PMC_SLPWK_ER0_PID28_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID28_Msk instead */
#define PMC_SLPWK_ER0_PID29_Pos             29                                             /**< (PMC_SLPWK_ER0) Peripheral 29 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID29_Msk             (0x1U << PMC_SLPWK_ER0_PID29_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 29 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID29                 PMC_SLPWK_ER0_PID29_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID29_Msk instead */
#define PMC_SLPWK_ER0_PID30_Pos             30                                             /**< (PMC_SLPWK_ER0) Peripheral 30 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID30_Msk             (0x1U << PMC_SLPWK_ER0_PID30_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 30 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID30                 PMC_SLPWK_ER0_PID30_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID30_Msk instead */
#define PMC_SLPWK_ER0_PID31_Pos             31                                             /**< (PMC_SLPWK_ER0) Peripheral 31 SleepWalking Enable Position */
#define PMC_SLPWK_ER0_PID31_Msk             (0x1U << PMC_SLPWK_ER0_PID31_Pos)              /**< (PMC_SLPWK_ER0) Peripheral 31 SleepWalking Enable Mask */
#define PMC_SLPWK_ER0_PID31                 PMC_SLPWK_ER0_PID31_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER0_PID31_Msk instead */
#define PMC_SLPWK_ER0_PID_Pos               7                                              /**< (PMC_SLPWK_ER0 Position) Peripheral 3x SleepWalking Enable */
#define PMC_SLPWK_ER0_PID_Msk               (0x1FFFFFFU << PMC_SLPWK_ER0_PID_Pos)          /**< (PMC_SLPWK_ER0 Mask) PID */
#define PMC_SLPWK_ER0_PID(value)            (PMC_SLPWK_ER0_PID_Msk & ((value) << PMC_SLPWK_ER0_PID_Pos))  
#define PMC_SLPWK_ER0_MASK                  (0xFFFFFF80U)                                  /**< \deprecated (PMC_SLPWK_ER0) Register MASK  (Use PMC_SLPWK_ER0_Msk instead)  */
#define PMC_SLPWK_ER0_Msk                   (0xFFFFFF80U)                                  /**< (PMC_SLPWK_ER0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_DR0 : (PMC Offset: 0x118) (/W 32) SleepWalking Disable Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID7:1;                    /**< bit:      7  Peripheral 7 SleepWalking Disable        */
    uint32_t PID8:1;                    /**< bit:      8  Peripheral 8 SleepWalking Disable        */
    uint32_t PID9:1;                    /**< bit:      9  Peripheral 9 SleepWalking Disable        */
    uint32_t PID10:1;                   /**< bit:     10  Peripheral 10 SleepWalking Disable       */
    uint32_t PID11:1;                   /**< bit:     11  Peripheral 11 SleepWalking Disable       */
    uint32_t PID12:1;                   /**< bit:     12  Peripheral 12 SleepWalking Disable       */
    uint32_t PID13:1;                   /**< bit:     13  Peripheral 13 SleepWalking Disable       */
    uint32_t PID14:1;                   /**< bit:     14  Peripheral 14 SleepWalking Disable       */
    uint32_t PID15:1;                   /**< bit:     15  Peripheral 15 SleepWalking Disable       */
    uint32_t PID16:1;                   /**< bit:     16  Peripheral 16 SleepWalking Disable       */
    uint32_t PID17:1;                   /**< bit:     17  Peripheral 17 SleepWalking Disable       */
    uint32_t PID18:1;                   /**< bit:     18  Peripheral 18 SleepWalking Disable       */
    uint32_t PID19:1;                   /**< bit:     19  Peripheral 19 SleepWalking Disable       */
    uint32_t PID20:1;                   /**< bit:     20  Peripheral 20 SleepWalking Disable       */
    uint32_t PID21:1;                   /**< bit:     21  Peripheral 21 SleepWalking Disable       */
    uint32_t PID22:1;                   /**< bit:     22  Peripheral 22 SleepWalking Disable       */
    uint32_t PID23:1;                   /**< bit:     23  Peripheral 23 SleepWalking Disable       */
    uint32_t PID24:1;                   /**< bit:     24  Peripheral 24 SleepWalking Disable       */
    uint32_t PID25:1;                   /**< bit:     25  Peripheral 25 SleepWalking Disable       */
    uint32_t PID26:1;                   /**< bit:     26  Peripheral 26 SleepWalking Disable       */
    uint32_t PID27:1;                   /**< bit:     27  Peripheral 27 SleepWalking Disable       */
    uint32_t PID28:1;                   /**< bit:     28  Peripheral 28 SleepWalking Disable       */
    uint32_t PID29:1;                   /**< bit:     29  Peripheral 29 SleepWalking Disable       */
    uint32_t PID30:1;                   /**< bit:     30  Peripheral 30 SleepWalking Disable       */
    uint32_t PID31:1;                   /**< bit:     31  Peripheral 31 SleepWalking Disable       */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID:25;                    /**< bit:  7..31  Peripheral 3x SleepWalking Disable       */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_DR0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_DR0_OFFSET                (0x118)                                       /**<  (PMC_SLPWK_DR0) SleepWalking Disable Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_DR0_PID7_Pos              7                                              /**< (PMC_SLPWK_DR0) Peripheral 7 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID7_Msk              (0x1U << PMC_SLPWK_DR0_PID7_Pos)               /**< (PMC_SLPWK_DR0) Peripheral 7 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID7                  PMC_SLPWK_DR0_PID7_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID7_Msk instead */
#define PMC_SLPWK_DR0_PID8_Pos              8                                              /**< (PMC_SLPWK_DR0) Peripheral 8 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID8_Msk              (0x1U << PMC_SLPWK_DR0_PID8_Pos)               /**< (PMC_SLPWK_DR0) Peripheral 8 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID8                  PMC_SLPWK_DR0_PID8_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID8_Msk instead */
#define PMC_SLPWK_DR0_PID9_Pos              9                                              /**< (PMC_SLPWK_DR0) Peripheral 9 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID9_Msk              (0x1U << PMC_SLPWK_DR0_PID9_Pos)               /**< (PMC_SLPWK_DR0) Peripheral 9 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID9                  PMC_SLPWK_DR0_PID9_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID9_Msk instead */
#define PMC_SLPWK_DR0_PID10_Pos             10                                             /**< (PMC_SLPWK_DR0) Peripheral 10 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID10_Msk             (0x1U << PMC_SLPWK_DR0_PID10_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 10 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID10                 PMC_SLPWK_DR0_PID10_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID10_Msk instead */
#define PMC_SLPWK_DR0_PID11_Pos             11                                             /**< (PMC_SLPWK_DR0) Peripheral 11 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID11_Msk             (0x1U << PMC_SLPWK_DR0_PID11_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 11 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID11                 PMC_SLPWK_DR0_PID11_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID11_Msk instead */
#define PMC_SLPWK_DR0_PID12_Pos             12                                             /**< (PMC_SLPWK_DR0) Peripheral 12 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID12_Msk             (0x1U << PMC_SLPWK_DR0_PID12_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 12 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID12                 PMC_SLPWK_DR0_PID12_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID12_Msk instead */
#define PMC_SLPWK_DR0_PID13_Pos             13                                             /**< (PMC_SLPWK_DR0) Peripheral 13 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID13_Msk             (0x1U << PMC_SLPWK_DR0_PID13_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 13 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID13                 PMC_SLPWK_DR0_PID13_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID13_Msk instead */
#define PMC_SLPWK_DR0_PID14_Pos             14                                             /**< (PMC_SLPWK_DR0) Peripheral 14 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID14_Msk             (0x1U << PMC_SLPWK_DR0_PID14_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 14 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID14                 PMC_SLPWK_DR0_PID14_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID14_Msk instead */
#define PMC_SLPWK_DR0_PID15_Pos             15                                             /**< (PMC_SLPWK_DR0) Peripheral 15 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID15_Msk             (0x1U << PMC_SLPWK_DR0_PID15_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 15 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID15                 PMC_SLPWK_DR0_PID15_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID15_Msk instead */
#define PMC_SLPWK_DR0_PID16_Pos             16                                             /**< (PMC_SLPWK_DR0) Peripheral 16 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID16_Msk             (0x1U << PMC_SLPWK_DR0_PID16_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 16 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID16                 PMC_SLPWK_DR0_PID16_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID16_Msk instead */
#define PMC_SLPWK_DR0_PID17_Pos             17                                             /**< (PMC_SLPWK_DR0) Peripheral 17 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID17_Msk             (0x1U << PMC_SLPWK_DR0_PID17_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 17 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID17                 PMC_SLPWK_DR0_PID17_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID17_Msk instead */
#define PMC_SLPWK_DR0_PID18_Pos             18                                             /**< (PMC_SLPWK_DR0) Peripheral 18 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID18_Msk             (0x1U << PMC_SLPWK_DR0_PID18_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 18 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID18                 PMC_SLPWK_DR0_PID18_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID18_Msk instead */
#define PMC_SLPWK_DR0_PID19_Pos             19                                             /**< (PMC_SLPWK_DR0) Peripheral 19 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID19_Msk             (0x1U << PMC_SLPWK_DR0_PID19_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 19 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID19                 PMC_SLPWK_DR0_PID19_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID19_Msk instead */
#define PMC_SLPWK_DR0_PID20_Pos             20                                             /**< (PMC_SLPWK_DR0) Peripheral 20 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID20_Msk             (0x1U << PMC_SLPWK_DR0_PID20_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 20 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID20                 PMC_SLPWK_DR0_PID20_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID20_Msk instead */
#define PMC_SLPWK_DR0_PID21_Pos             21                                             /**< (PMC_SLPWK_DR0) Peripheral 21 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID21_Msk             (0x1U << PMC_SLPWK_DR0_PID21_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 21 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID21                 PMC_SLPWK_DR0_PID21_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID21_Msk instead */
#define PMC_SLPWK_DR0_PID22_Pos             22                                             /**< (PMC_SLPWK_DR0) Peripheral 22 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID22_Msk             (0x1U << PMC_SLPWK_DR0_PID22_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 22 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID22                 PMC_SLPWK_DR0_PID22_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID22_Msk instead */
#define PMC_SLPWK_DR0_PID23_Pos             23                                             /**< (PMC_SLPWK_DR0) Peripheral 23 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID23_Msk             (0x1U << PMC_SLPWK_DR0_PID23_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 23 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID23                 PMC_SLPWK_DR0_PID23_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID23_Msk instead */
#define PMC_SLPWK_DR0_PID24_Pos             24                                             /**< (PMC_SLPWK_DR0) Peripheral 24 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID24_Msk             (0x1U << PMC_SLPWK_DR0_PID24_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 24 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID24                 PMC_SLPWK_DR0_PID24_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID24_Msk instead */
#define PMC_SLPWK_DR0_PID25_Pos             25                                             /**< (PMC_SLPWK_DR0) Peripheral 25 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID25_Msk             (0x1U << PMC_SLPWK_DR0_PID25_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 25 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID25                 PMC_SLPWK_DR0_PID25_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID25_Msk instead */
#define PMC_SLPWK_DR0_PID26_Pos             26                                             /**< (PMC_SLPWK_DR0) Peripheral 26 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID26_Msk             (0x1U << PMC_SLPWK_DR0_PID26_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 26 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID26                 PMC_SLPWK_DR0_PID26_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID26_Msk instead */
#define PMC_SLPWK_DR0_PID27_Pos             27                                             /**< (PMC_SLPWK_DR0) Peripheral 27 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID27_Msk             (0x1U << PMC_SLPWK_DR0_PID27_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 27 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID27                 PMC_SLPWK_DR0_PID27_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID27_Msk instead */
#define PMC_SLPWK_DR0_PID28_Pos             28                                             /**< (PMC_SLPWK_DR0) Peripheral 28 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID28_Msk             (0x1U << PMC_SLPWK_DR0_PID28_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 28 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID28                 PMC_SLPWK_DR0_PID28_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID28_Msk instead */
#define PMC_SLPWK_DR0_PID29_Pos             29                                             /**< (PMC_SLPWK_DR0) Peripheral 29 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID29_Msk             (0x1U << PMC_SLPWK_DR0_PID29_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 29 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID29                 PMC_SLPWK_DR0_PID29_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID29_Msk instead */
#define PMC_SLPWK_DR0_PID30_Pos             30                                             /**< (PMC_SLPWK_DR0) Peripheral 30 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID30_Msk             (0x1U << PMC_SLPWK_DR0_PID30_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 30 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID30                 PMC_SLPWK_DR0_PID30_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID30_Msk instead */
#define PMC_SLPWK_DR0_PID31_Pos             31                                             /**< (PMC_SLPWK_DR0) Peripheral 31 SleepWalking Disable Position */
#define PMC_SLPWK_DR0_PID31_Msk             (0x1U << PMC_SLPWK_DR0_PID31_Pos)              /**< (PMC_SLPWK_DR0) Peripheral 31 SleepWalking Disable Mask */
#define PMC_SLPWK_DR0_PID31                 PMC_SLPWK_DR0_PID31_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR0_PID31_Msk instead */
#define PMC_SLPWK_DR0_PID_Pos               7                                              /**< (PMC_SLPWK_DR0 Position) Peripheral 3x SleepWalking Disable */
#define PMC_SLPWK_DR0_PID_Msk               (0x1FFFFFFU << PMC_SLPWK_DR0_PID_Pos)          /**< (PMC_SLPWK_DR0 Mask) PID */
#define PMC_SLPWK_DR0_PID(value)            (PMC_SLPWK_DR0_PID_Msk & ((value) << PMC_SLPWK_DR0_PID_Pos))  
#define PMC_SLPWK_DR0_MASK                  (0xFFFFFF80U)                                  /**< \deprecated (PMC_SLPWK_DR0) Register MASK  (Use PMC_SLPWK_DR0_Msk instead)  */
#define PMC_SLPWK_DR0_Msk                   (0xFFFFFF80U)                                  /**< (PMC_SLPWK_DR0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_SR0 : (PMC Offset: 0x11c) (R/ 32) SleepWalking Status Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID7:1;                    /**< bit:      7  Peripheral 7 SleepWalking Status         */
    uint32_t PID8:1;                    /**< bit:      8  Peripheral 8 SleepWalking Status         */
    uint32_t PID9:1;                    /**< bit:      9  Peripheral 9 SleepWalking Status         */
    uint32_t PID10:1;                   /**< bit:     10  Peripheral 10 SleepWalking Status        */
    uint32_t PID11:1;                   /**< bit:     11  Peripheral 11 SleepWalking Status        */
    uint32_t PID12:1;                   /**< bit:     12  Peripheral 12 SleepWalking Status        */
    uint32_t PID13:1;                   /**< bit:     13  Peripheral 13 SleepWalking Status        */
    uint32_t PID14:1;                   /**< bit:     14  Peripheral 14 SleepWalking Status        */
    uint32_t PID15:1;                   /**< bit:     15  Peripheral 15 SleepWalking Status        */
    uint32_t PID16:1;                   /**< bit:     16  Peripheral 16 SleepWalking Status        */
    uint32_t PID17:1;                   /**< bit:     17  Peripheral 17 SleepWalking Status        */
    uint32_t PID18:1;                   /**< bit:     18  Peripheral 18 SleepWalking Status        */
    uint32_t PID19:1;                   /**< bit:     19  Peripheral 19 SleepWalking Status        */
    uint32_t PID20:1;                   /**< bit:     20  Peripheral 20 SleepWalking Status        */
    uint32_t PID21:1;                   /**< bit:     21  Peripheral 21 SleepWalking Status        */
    uint32_t PID22:1;                   /**< bit:     22  Peripheral 22 SleepWalking Status        */
    uint32_t PID23:1;                   /**< bit:     23  Peripheral 23 SleepWalking Status        */
    uint32_t PID24:1;                   /**< bit:     24  Peripheral 24 SleepWalking Status        */
    uint32_t PID25:1;                   /**< bit:     25  Peripheral 25 SleepWalking Status        */
    uint32_t PID26:1;                   /**< bit:     26  Peripheral 26 SleepWalking Status        */
    uint32_t PID27:1;                   /**< bit:     27  Peripheral 27 SleepWalking Status        */
    uint32_t PID28:1;                   /**< bit:     28  Peripheral 28 SleepWalking Status        */
    uint32_t PID29:1;                   /**< bit:     29  Peripheral 29 SleepWalking Status        */
    uint32_t PID30:1;                   /**< bit:     30  Peripheral 30 SleepWalking Status        */
    uint32_t PID31:1;                   /**< bit:     31  Peripheral 31 SleepWalking Status        */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID:25;                    /**< bit:  7..31  Peripheral 3x SleepWalking Status        */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_SR0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_SR0_OFFSET                (0x11C)                                       /**<  (PMC_SLPWK_SR0) SleepWalking Status Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_SR0_PID7_Pos              7                                              /**< (PMC_SLPWK_SR0) Peripheral 7 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID7_Msk              (0x1U << PMC_SLPWK_SR0_PID7_Pos)               /**< (PMC_SLPWK_SR0) Peripheral 7 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID7                  PMC_SLPWK_SR0_PID7_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID7_Msk instead */
#define PMC_SLPWK_SR0_PID8_Pos              8                                              /**< (PMC_SLPWK_SR0) Peripheral 8 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID8_Msk              (0x1U << PMC_SLPWK_SR0_PID8_Pos)               /**< (PMC_SLPWK_SR0) Peripheral 8 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID8                  PMC_SLPWK_SR0_PID8_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID8_Msk instead */
#define PMC_SLPWK_SR0_PID9_Pos              9                                              /**< (PMC_SLPWK_SR0) Peripheral 9 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID9_Msk              (0x1U << PMC_SLPWK_SR0_PID9_Pos)               /**< (PMC_SLPWK_SR0) Peripheral 9 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID9                  PMC_SLPWK_SR0_PID9_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID9_Msk instead */
#define PMC_SLPWK_SR0_PID10_Pos             10                                             /**< (PMC_SLPWK_SR0) Peripheral 10 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID10_Msk             (0x1U << PMC_SLPWK_SR0_PID10_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 10 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID10                 PMC_SLPWK_SR0_PID10_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID10_Msk instead */
#define PMC_SLPWK_SR0_PID11_Pos             11                                             /**< (PMC_SLPWK_SR0) Peripheral 11 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID11_Msk             (0x1U << PMC_SLPWK_SR0_PID11_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 11 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID11                 PMC_SLPWK_SR0_PID11_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID11_Msk instead */
#define PMC_SLPWK_SR0_PID12_Pos             12                                             /**< (PMC_SLPWK_SR0) Peripheral 12 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID12_Msk             (0x1U << PMC_SLPWK_SR0_PID12_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 12 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID12                 PMC_SLPWK_SR0_PID12_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID12_Msk instead */
#define PMC_SLPWK_SR0_PID13_Pos             13                                             /**< (PMC_SLPWK_SR0) Peripheral 13 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID13_Msk             (0x1U << PMC_SLPWK_SR0_PID13_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 13 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID13                 PMC_SLPWK_SR0_PID13_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID13_Msk instead */
#define PMC_SLPWK_SR0_PID14_Pos             14                                             /**< (PMC_SLPWK_SR0) Peripheral 14 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID14_Msk             (0x1U << PMC_SLPWK_SR0_PID14_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 14 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID14                 PMC_SLPWK_SR0_PID14_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID14_Msk instead */
#define PMC_SLPWK_SR0_PID15_Pos             15                                             /**< (PMC_SLPWK_SR0) Peripheral 15 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID15_Msk             (0x1U << PMC_SLPWK_SR0_PID15_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 15 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID15                 PMC_SLPWK_SR0_PID15_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID15_Msk instead */
#define PMC_SLPWK_SR0_PID16_Pos             16                                             /**< (PMC_SLPWK_SR0) Peripheral 16 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID16_Msk             (0x1U << PMC_SLPWK_SR0_PID16_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 16 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID16                 PMC_SLPWK_SR0_PID16_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID16_Msk instead */
#define PMC_SLPWK_SR0_PID17_Pos             17                                             /**< (PMC_SLPWK_SR0) Peripheral 17 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID17_Msk             (0x1U << PMC_SLPWK_SR0_PID17_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 17 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID17                 PMC_SLPWK_SR0_PID17_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID17_Msk instead */
#define PMC_SLPWK_SR0_PID18_Pos             18                                             /**< (PMC_SLPWK_SR0) Peripheral 18 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID18_Msk             (0x1U << PMC_SLPWK_SR0_PID18_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 18 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID18                 PMC_SLPWK_SR0_PID18_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID18_Msk instead */
#define PMC_SLPWK_SR0_PID19_Pos             19                                             /**< (PMC_SLPWK_SR0) Peripheral 19 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID19_Msk             (0x1U << PMC_SLPWK_SR0_PID19_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 19 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID19                 PMC_SLPWK_SR0_PID19_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID19_Msk instead */
#define PMC_SLPWK_SR0_PID20_Pos             20                                             /**< (PMC_SLPWK_SR0) Peripheral 20 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID20_Msk             (0x1U << PMC_SLPWK_SR0_PID20_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 20 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID20                 PMC_SLPWK_SR0_PID20_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID20_Msk instead */
#define PMC_SLPWK_SR0_PID21_Pos             21                                             /**< (PMC_SLPWK_SR0) Peripheral 21 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID21_Msk             (0x1U << PMC_SLPWK_SR0_PID21_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 21 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID21                 PMC_SLPWK_SR0_PID21_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID21_Msk instead */
#define PMC_SLPWK_SR0_PID22_Pos             22                                             /**< (PMC_SLPWK_SR0) Peripheral 22 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID22_Msk             (0x1U << PMC_SLPWK_SR0_PID22_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 22 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID22                 PMC_SLPWK_SR0_PID22_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID22_Msk instead */
#define PMC_SLPWK_SR0_PID23_Pos             23                                             /**< (PMC_SLPWK_SR0) Peripheral 23 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID23_Msk             (0x1U << PMC_SLPWK_SR0_PID23_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 23 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID23                 PMC_SLPWK_SR0_PID23_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID23_Msk instead */
#define PMC_SLPWK_SR0_PID24_Pos             24                                             /**< (PMC_SLPWK_SR0) Peripheral 24 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID24_Msk             (0x1U << PMC_SLPWK_SR0_PID24_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 24 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID24                 PMC_SLPWK_SR0_PID24_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID24_Msk instead */
#define PMC_SLPWK_SR0_PID25_Pos             25                                             /**< (PMC_SLPWK_SR0) Peripheral 25 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID25_Msk             (0x1U << PMC_SLPWK_SR0_PID25_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 25 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID25                 PMC_SLPWK_SR0_PID25_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID25_Msk instead */
#define PMC_SLPWK_SR0_PID26_Pos             26                                             /**< (PMC_SLPWK_SR0) Peripheral 26 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID26_Msk             (0x1U << PMC_SLPWK_SR0_PID26_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 26 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID26                 PMC_SLPWK_SR0_PID26_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID26_Msk instead */
#define PMC_SLPWK_SR0_PID27_Pos             27                                             /**< (PMC_SLPWK_SR0) Peripheral 27 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID27_Msk             (0x1U << PMC_SLPWK_SR0_PID27_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 27 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID27                 PMC_SLPWK_SR0_PID27_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID27_Msk instead */
#define PMC_SLPWK_SR0_PID28_Pos             28                                             /**< (PMC_SLPWK_SR0) Peripheral 28 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID28_Msk             (0x1U << PMC_SLPWK_SR0_PID28_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 28 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID28                 PMC_SLPWK_SR0_PID28_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID28_Msk instead */
#define PMC_SLPWK_SR0_PID29_Pos             29                                             /**< (PMC_SLPWK_SR0) Peripheral 29 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID29_Msk             (0x1U << PMC_SLPWK_SR0_PID29_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 29 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID29                 PMC_SLPWK_SR0_PID29_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID29_Msk instead */
#define PMC_SLPWK_SR0_PID30_Pos             30                                             /**< (PMC_SLPWK_SR0) Peripheral 30 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID30_Msk             (0x1U << PMC_SLPWK_SR0_PID30_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 30 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID30                 PMC_SLPWK_SR0_PID30_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID30_Msk instead */
#define PMC_SLPWK_SR0_PID31_Pos             31                                             /**< (PMC_SLPWK_SR0) Peripheral 31 SleepWalking Status Position */
#define PMC_SLPWK_SR0_PID31_Msk             (0x1U << PMC_SLPWK_SR0_PID31_Pos)              /**< (PMC_SLPWK_SR0) Peripheral 31 SleepWalking Status Mask */
#define PMC_SLPWK_SR0_PID31                 PMC_SLPWK_SR0_PID31_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR0_PID31_Msk instead */
#define PMC_SLPWK_SR0_PID_Pos               7                                              /**< (PMC_SLPWK_SR0 Position) Peripheral 3x SleepWalking Status */
#define PMC_SLPWK_SR0_PID_Msk               (0x1FFFFFFU << PMC_SLPWK_SR0_PID_Pos)          /**< (PMC_SLPWK_SR0 Mask) PID */
#define PMC_SLPWK_SR0_PID(value)            (PMC_SLPWK_SR0_PID_Msk & ((value) << PMC_SLPWK_SR0_PID_Pos))  
#define PMC_SLPWK_SR0_MASK                  (0xFFFFFF80U)                                  /**< \deprecated (PMC_SLPWK_SR0) Register MASK  (Use PMC_SLPWK_SR0_Msk instead)  */
#define PMC_SLPWK_SR0_Msk                   (0xFFFFFF80U)                                  /**< (PMC_SLPWK_SR0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_ASR0 : (PMC Offset: 0x120) (R/ 32) SleepWalking Activity Status Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID7:1;                    /**< bit:      7  Peripheral 7 Activity Status             */
    uint32_t PID8:1;                    /**< bit:      8  Peripheral 8 Activity Status             */
    uint32_t PID9:1;                    /**< bit:      9  Peripheral 9 Activity Status             */
    uint32_t PID10:1;                   /**< bit:     10  Peripheral 10 Activity Status            */
    uint32_t PID11:1;                   /**< bit:     11  Peripheral 11 Activity Status            */
    uint32_t PID12:1;                   /**< bit:     12  Peripheral 12 Activity Status            */
    uint32_t PID13:1;                   /**< bit:     13  Peripheral 13 Activity Status            */
    uint32_t PID14:1;                   /**< bit:     14  Peripheral 14 Activity Status            */
    uint32_t PID15:1;                   /**< bit:     15  Peripheral 15 Activity Status            */
    uint32_t PID16:1;                   /**< bit:     16  Peripheral 16 Activity Status            */
    uint32_t PID17:1;                   /**< bit:     17  Peripheral 17 Activity Status            */
    uint32_t PID18:1;                   /**< bit:     18  Peripheral 18 Activity Status            */
    uint32_t PID19:1;                   /**< bit:     19  Peripheral 19 Activity Status            */
    uint32_t PID20:1;                   /**< bit:     20  Peripheral 20 Activity Status            */
    uint32_t PID21:1;                   /**< bit:     21  Peripheral 21 Activity Status            */
    uint32_t PID22:1;                   /**< bit:     22  Peripheral 22 Activity Status            */
    uint32_t PID23:1;                   /**< bit:     23  Peripheral 23 Activity Status            */
    uint32_t PID24:1;                   /**< bit:     24  Peripheral 24 Activity Status            */
    uint32_t PID25:1;                   /**< bit:     25  Peripheral 25 Activity Status            */
    uint32_t PID26:1;                   /**< bit:     26  Peripheral 26 Activity Status            */
    uint32_t PID27:1;                   /**< bit:     27  Peripheral 27 Activity Status            */
    uint32_t PID28:1;                   /**< bit:     28  Peripheral 28 Activity Status            */
    uint32_t PID29:1;                   /**< bit:     29  Peripheral 29 Activity Status            */
    uint32_t PID30:1;                   /**< bit:     30  Peripheral 30 Activity Status            */
    uint32_t PID31:1;                   /**< bit:     31  Peripheral 31 Activity Status            */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t PID:25;                    /**< bit:  7..31  Peripheral 3x Activity Status            */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_ASR0_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_ASR0_OFFSET               (0x120)                                       /**<  (PMC_SLPWK_ASR0) SleepWalking Activity Status Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_ASR0_PID7_Pos             7                                              /**< (PMC_SLPWK_ASR0) Peripheral 7 Activity Status Position */
#define PMC_SLPWK_ASR0_PID7_Msk             (0x1U << PMC_SLPWK_ASR0_PID7_Pos)              /**< (PMC_SLPWK_ASR0) Peripheral 7 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID7                 PMC_SLPWK_ASR0_PID7_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID7_Msk instead */
#define PMC_SLPWK_ASR0_PID8_Pos             8                                              /**< (PMC_SLPWK_ASR0) Peripheral 8 Activity Status Position */
#define PMC_SLPWK_ASR0_PID8_Msk             (0x1U << PMC_SLPWK_ASR0_PID8_Pos)              /**< (PMC_SLPWK_ASR0) Peripheral 8 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID8                 PMC_SLPWK_ASR0_PID8_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID8_Msk instead */
#define PMC_SLPWK_ASR0_PID9_Pos             9                                              /**< (PMC_SLPWK_ASR0) Peripheral 9 Activity Status Position */
#define PMC_SLPWK_ASR0_PID9_Msk             (0x1U << PMC_SLPWK_ASR0_PID9_Pos)              /**< (PMC_SLPWK_ASR0) Peripheral 9 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID9                 PMC_SLPWK_ASR0_PID9_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID9_Msk instead */
#define PMC_SLPWK_ASR0_PID10_Pos            10                                             /**< (PMC_SLPWK_ASR0) Peripheral 10 Activity Status Position */
#define PMC_SLPWK_ASR0_PID10_Msk            (0x1U << PMC_SLPWK_ASR0_PID10_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 10 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID10                PMC_SLPWK_ASR0_PID10_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID10_Msk instead */
#define PMC_SLPWK_ASR0_PID11_Pos            11                                             /**< (PMC_SLPWK_ASR0) Peripheral 11 Activity Status Position */
#define PMC_SLPWK_ASR0_PID11_Msk            (0x1U << PMC_SLPWK_ASR0_PID11_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 11 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID11                PMC_SLPWK_ASR0_PID11_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID11_Msk instead */
#define PMC_SLPWK_ASR0_PID12_Pos            12                                             /**< (PMC_SLPWK_ASR0) Peripheral 12 Activity Status Position */
#define PMC_SLPWK_ASR0_PID12_Msk            (0x1U << PMC_SLPWK_ASR0_PID12_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 12 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID12                PMC_SLPWK_ASR0_PID12_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID12_Msk instead */
#define PMC_SLPWK_ASR0_PID13_Pos            13                                             /**< (PMC_SLPWK_ASR0) Peripheral 13 Activity Status Position */
#define PMC_SLPWK_ASR0_PID13_Msk            (0x1U << PMC_SLPWK_ASR0_PID13_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 13 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID13                PMC_SLPWK_ASR0_PID13_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID13_Msk instead */
#define PMC_SLPWK_ASR0_PID14_Pos            14                                             /**< (PMC_SLPWK_ASR0) Peripheral 14 Activity Status Position */
#define PMC_SLPWK_ASR0_PID14_Msk            (0x1U << PMC_SLPWK_ASR0_PID14_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 14 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID14                PMC_SLPWK_ASR0_PID14_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID14_Msk instead */
#define PMC_SLPWK_ASR0_PID15_Pos            15                                             /**< (PMC_SLPWK_ASR0) Peripheral 15 Activity Status Position */
#define PMC_SLPWK_ASR0_PID15_Msk            (0x1U << PMC_SLPWK_ASR0_PID15_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 15 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID15                PMC_SLPWK_ASR0_PID15_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID15_Msk instead */
#define PMC_SLPWK_ASR0_PID16_Pos            16                                             /**< (PMC_SLPWK_ASR0) Peripheral 16 Activity Status Position */
#define PMC_SLPWK_ASR0_PID16_Msk            (0x1U << PMC_SLPWK_ASR0_PID16_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 16 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID16                PMC_SLPWK_ASR0_PID16_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID16_Msk instead */
#define PMC_SLPWK_ASR0_PID17_Pos            17                                             /**< (PMC_SLPWK_ASR0) Peripheral 17 Activity Status Position */
#define PMC_SLPWK_ASR0_PID17_Msk            (0x1U << PMC_SLPWK_ASR0_PID17_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 17 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID17                PMC_SLPWK_ASR0_PID17_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID17_Msk instead */
#define PMC_SLPWK_ASR0_PID18_Pos            18                                             /**< (PMC_SLPWK_ASR0) Peripheral 18 Activity Status Position */
#define PMC_SLPWK_ASR0_PID18_Msk            (0x1U << PMC_SLPWK_ASR0_PID18_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 18 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID18                PMC_SLPWK_ASR0_PID18_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID18_Msk instead */
#define PMC_SLPWK_ASR0_PID19_Pos            19                                             /**< (PMC_SLPWK_ASR0) Peripheral 19 Activity Status Position */
#define PMC_SLPWK_ASR0_PID19_Msk            (0x1U << PMC_SLPWK_ASR0_PID19_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 19 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID19                PMC_SLPWK_ASR0_PID19_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID19_Msk instead */
#define PMC_SLPWK_ASR0_PID20_Pos            20                                             /**< (PMC_SLPWK_ASR0) Peripheral 20 Activity Status Position */
#define PMC_SLPWK_ASR0_PID20_Msk            (0x1U << PMC_SLPWK_ASR0_PID20_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 20 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID20                PMC_SLPWK_ASR0_PID20_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID20_Msk instead */
#define PMC_SLPWK_ASR0_PID21_Pos            21                                             /**< (PMC_SLPWK_ASR0) Peripheral 21 Activity Status Position */
#define PMC_SLPWK_ASR0_PID21_Msk            (0x1U << PMC_SLPWK_ASR0_PID21_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 21 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID21                PMC_SLPWK_ASR0_PID21_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID21_Msk instead */
#define PMC_SLPWK_ASR0_PID22_Pos            22                                             /**< (PMC_SLPWK_ASR0) Peripheral 22 Activity Status Position */
#define PMC_SLPWK_ASR0_PID22_Msk            (0x1U << PMC_SLPWK_ASR0_PID22_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 22 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID22                PMC_SLPWK_ASR0_PID22_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID22_Msk instead */
#define PMC_SLPWK_ASR0_PID23_Pos            23                                             /**< (PMC_SLPWK_ASR0) Peripheral 23 Activity Status Position */
#define PMC_SLPWK_ASR0_PID23_Msk            (0x1U << PMC_SLPWK_ASR0_PID23_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 23 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID23                PMC_SLPWK_ASR0_PID23_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID23_Msk instead */
#define PMC_SLPWK_ASR0_PID24_Pos            24                                             /**< (PMC_SLPWK_ASR0) Peripheral 24 Activity Status Position */
#define PMC_SLPWK_ASR0_PID24_Msk            (0x1U << PMC_SLPWK_ASR0_PID24_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 24 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID24                PMC_SLPWK_ASR0_PID24_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID24_Msk instead */
#define PMC_SLPWK_ASR0_PID25_Pos            25                                             /**< (PMC_SLPWK_ASR0) Peripheral 25 Activity Status Position */
#define PMC_SLPWK_ASR0_PID25_Msk            (0x1U << PMC_SLPWK_ASR0_PID25_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 25 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID25                PMC_SLPWK_ASR0_PID25_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID25_Msk instead */
#define PMC_SLPWK_ASR0_PID26_Pos            26                                             /**< (PMC_SLPWK_ASR0) Peripheral 26 Activity Status Position */
#define PMC_SLPWK_ASR0_PID26_Msk            (0x1U << PMC_SLPWK_ASR0_PID26_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 26 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID26                PMC_SLPWK_ASR0_PID26_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID26_Msk instead */
#define PMC_SLPWK_ASR0_PID27_Pos            27                                             /**< (PMC_SLPWK_ASR0) Peripheral 27 Activity Status Position */
#define PMC_SLPWK_ASR0_PID27_Msk            (0x1U << PMC_SLPWK_ASR0_PID27_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 27 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID27                PMC_SLPWK_ASR0_PID27_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID27_Msk instead */
#define PMC_SLPWK_ASR0_PID28_Pos            28                                             /**< (PMC_SLPWK_ASR0) Peripheral 28 Activity Status Position */
#define PMC_SLPWK_ASR0_PID28_Msk            (0x1U << PMC_SLPWK_ASR0_PID28_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 28 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID28                PMC_SLPWK_ASR0_PID28_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID28_Msk instead */
#define PMC_SLPWK_ASR0_PID29_Pos            29                                             /**< (PMC_SLPWK_ASR0) Peripheral 29 Activity Status Position */
#define PMC_SLPWK_ASR0_PID29_Msk            (0x1U << PMC_SLPWK_ASR0_PID29_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 29 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID29                PMC_SLPWK_ASR0_PID29_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID29_Msk instead */
#define PMC_SLPWK_ASR0_PID30_Pos            30                                             /**< (PMC_SLPWK_ASR0) Peripheral 30 Activity Status Position */
#define PMC_SLPWK_ASR0_PID30_Msk            (0x1U << PMC_SLPWK_ASR0_PID30_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 30 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID30                PMC_SLPWK_ASR0_PID30_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID30_Msk instead */
#define PMC_SLPWK_ASR0_PID31_Pos            31                                             /**< (PMC_SLPWK_ASR0) Peripheral 31 Activity Status Position */
#define PMC_SLPWK_ASR0_PID31_Msk            (0x1U << PMC_SLPWK_ASR0_PID31_Pos)             /**< (PMC_SLPWK_ASR0) Peripheral 31 Activity Status Mask */
#define PMC_SLPWK_ASR0_PID31                PMC_SLPWK_ASR0_PID31_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR0_PID31_Msk instead */
#define PMC_SLPWK_ASR0_PID_Pos              7                                              /**< (PMC_SLPWK_ASR0 Position) Peripheral 3x Activity Status */
#define PMC_SLPWK_ASR0_PID_Msk              (0x1FFFFFFU << PMC_SLPWK_ASR0_PID_Pos)         /**< (PMC_SLPWK_ASR0 Mask) PID */
#define PMC_SLPWK_ASR0_PID(value)           (PMC_SLPWK_ASR0_PID_Msk & ((value) << PMC_SLPWK_ASR0_PID_Pos))  
#define PMC_SLPWK_ASR0_MASK                 (0xFFFFFF80U)                                  /**< \deprecated (PMC_SLPWK_ASR0) Register MASK  (Use PMC_SLPWK_ASR0_Msk instead)  */
#define PMC_SLPWK_ASR0_Msk                  (0xFFFFFF80U)                                  /**< (PMC_SLPWK_ASR0) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_PMMR : (PMC Offset: 0x130) (R/W 32) PLL Maximum Multiplier Value Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PLLA_MMAX:11;              /**< bit:  0..10  PLLA Maximum Allowed Multiplier Value    */
    uint32_t :21;                       /**< bit: 11..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_PMMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_PMMR_OFFSET                     (0x130)                                       /**<  (PMC_PMMR) PLL Maximum Multiplier Value Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_PMMR_PLLA_MMAX_Pos              0                                              /**< (PMC_PMMR) PLLA Maximum Allowed Multiplier Value Position */
#define PMC_PMMR_PLLA_MMAX_Msk              (0x7FFU << PMC_PMMR_PLLA_MMAX_Pos)             /**< (PMC_PMMR) PLLA Maximum Allowed Multiplier Value Mask */
#define PMC_PMMR_PLLA_MMAX(value)           (PMC_PMMR_PLLA_MMAX_Msk & ((value) << PMC_PMMR_PLLA_MMAX_Pos))
#define PMC_PMMR_MASK                       (0x7FFU)                                       /**< \deprecated (PMC_PMMR) Register MASK  (Use PMC_PMMR_Msk instead)  */
#define PMC_PMMR_Msk                        (0x7FFU)                                       /**< (PMC_PMMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_ER1 : (PMC Offset: 0x134) (/W 32) SleepWalking Enable Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PID32:1;                   /**< bit:      0  Peripheral 32 SleepWalking Enable        */
    uint32_t PID33:1;                   /**< bit:      1  Peripheral 33 SleepWalking Enable        */
    uint32_t PID34:1;                   /**< bit:      2  Peripheral 34 SleepWalking Enable        */
    uint32_t PID35:1;                   /**< bit:      3  Peripheral 35 SleepWalking Enable        */
    uint32_t :1;                        /**< bit:      4  Reserved */
    uint32_t PID37:1;                   /**< bit:      5  Peripheral 37 SleepWalking Enable        */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t PID39:1;                   /**< bit:      7  Peripheral 39 SleepWalking Enable        */
    uint32_t PID40:1;                   /**< bit:      8  Peripheral 40 SleepWalking Enable        */
    uint32_t PID41:1;                   /**< bit:      9  Peripheral 41 SleepWalking Enable        */
    uint32_t PID42:1;                   /**< bit:     10  Peripheral 42 SleepWalking Enable        */
    uint32_t PID43:1;                   /**< bit:     11  Peripheral 43 SleepWalking Enable        */
    uint32_t PID44:1;                   /**< bit:     12  Peripheral 44 SleepWalking Enable        */
    uint32_t PID45:1;                   /**< bit:     13  Peripheral 45 SleepWalking Enable        */
    uint32_t PID46:1;                   /**< bit:     14  Peripheral 46 SleepWalking Enable        */
    uint32_t PID47:1;                   /**< bit:     15  Peripheral 47 SleepWalking Enable        */
    uint32_t PID48:1;                   /**< bit:     16  Peripheral 48 SleepWalking Enable        */
    uint32_t PID49:1;                   /**< bit:     17  Peripheral 49 SleepWalking Enable        */
    uint32_t PID50:1;                   /**< bit:     18  Peripheral 50 SleepWalking Enable        */
    uint32_t PID51:1;                   /**< bit:     19  Peripheral 51 SleepWalking Enable        */
    uint32_t PID52:1;                   /**< bit:     20  Peripheral 52 SleepWalking Enable        */
    uint32_t PID53:1;                   /**< bit:     21  Peripheral 53 SleepWalking Enable        */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t PID56:1;                   /**< bit:     24  Peripheral 56 SleepWalking Enable        */
    uint32_t PID57:1;                   /**< bit:     25  Peripheral 57 SleepWalking Enable        */
    uint32_t PID58:1;                   /**< bit:     26  Peripheral 58 SleepWalking Enable        */
    uint32_t PID59:1;                   /**< bit:     27  Peripheral 59 SleepWalking Enable        */
    uint32_t PID60:1;                   /**< bit:     28  Peripheral 60 SleepWalking Enable        */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t PID:25;                    /**< bit:  0..24  Peripheral 6x SleepWalking Enable        */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_ER1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_ER1_OFFSET                (0x134)                                       /**<  (PMC_SLPWK_ER1) SleepWalking Enable Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_ER1_PID32_Pos             0                                              /**< (PMC_SLPWK_ER1) Peripheral 32 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID32_Msk             (0x1U << PMC_SLPWK_ER1_PID32_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 32 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID32                 PMC_SLPWK_ER1_PID32_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID32_Msk instead */
#define PMC_SLPWK_ER1_PID33_Pos             1                                              /**< (PMC_SLPWK_ER1) Peripheral 33 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID33_Msk             (0x1U << PMC_SLPWK_ER1_PID33_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 33 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID33                 PMC_SLPWK_ER1_PID33_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID33_Msk instead */
#define PMC_SLPWK_ER1_PID34_Pos             2                                              /**< (PMC_SLPWK_ER1) Peripheral 34 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID34_Msk             (0x1U << PMC_SLPWK_ER1_PID34_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 34 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID34                 PMC_SLPWK_ER1_PID34_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID34_Msk instead */
#define PMC_SLPWK_ER1_PID35_Pos             3                                              /**< (PMC_SLPWK_ER1) Peripheral 35 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID35_Msk             (0x1U << PMC_SLPWK_ER1_PID35_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 35 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID35                 PMC_SLPWK_ER1_PID35_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID35_Msk instead */
#define PMC_SLPWK_ER1_PID37_Pos             5                                              /**< (PMC_SLPWK_ER1) Peripheral 37 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID37_Msk             (0x1U << PMC_SLPWK_ER1_PID37_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 37 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID37                 PMC_SLPWK_ER1_PID37_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID37_Msk instead */
#define PMC_SLPWK_ER1_PID39_Pos             7                                              /**< (PMC_SLPWK_ER1) Peripheral 39 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID39_Msk             (0x1U << PMC_SLPWK_ER1_PID39_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 39 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID39                 PMC_SLPWK_ER1_PID39_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID39_Msk instead */
#define PMC_SLPWK_ER1_PID40_Pos             8                                              /**< (PMC_SLPWK_ER1) Peripheral 40 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID40_Msk             (0x1U << PMC_SLPWK_ER1_PID40_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 40 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID40                 PMC_SLPWK_ER1_PID40_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID40_Msk instead */
#define PMC_SLPWK_ER1_PID41_Pos             9                                              /**< (PMC_SLPWK_ER1) Peripheral 41 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID41_Msk             (0x1U << PMC_SLPWK_ER1_PID41_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 41 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID41                 PMC_SLPWK_ER1_PID41_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID41_Msk instead */
#define PMC_SLPWK_ER1_PID42_Pos             10                                             /**< (PMC_SLPWK_ER1) Peripheral 42 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID42_Msk             (0x1U << PMC_SLPWK_ER1_PID42_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 42 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID42                 PMC_SLPWK_ER1_PID42_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID42_Msk instead */
#define PMC_SLPWK_ER1_PID43_Pos             11                                             /**< (PMC_SLPWK_ER1) Peripheral 43 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID43_Msk             (0x1U << PMC_SLPWK_ER1_PID43_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 43 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID43                 PMC_SLPWK_ER1_PID43_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID43_Msk instead */
#define PMC_SLPWK_ER1_PID44_Pos             12                                             /**< (PMC_SLPWK_ER1) Peripheral 44 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID44_Msk             (0x1U << PMC_SLPWK_ER1_PID44_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 44 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID44                 PMC_SLPWK_ER1_PID44_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID44_Msk instead */
#define PMC_SLPWK_ER1_PID45_Pos             13                                             /**< (PMC_SLPWK_ER1) Peripheral 45 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID45_Msk             (0x1U << PMC_SLPWK_ER1_PID45_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 45 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID45                 PMC_SLPWK_ER1_PID45_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID45_Msk instead */
#define PMC_SLPWK_ER1_PID46_Pos             14                                             /**< (PMC_SLPWK_ER1) Peripheral 46 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID46_Msk             (0x1U << PMC_SLPWK_ER1_PID46_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 46 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID46                 PMC_SLPWK_ER1_PID46_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID46_Msk instead */
#define PMC_SLPWK_ER1_PID47_Pos             15                                             /**< (PMC_SLPWK_ER1) Peripheral 47 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID47_Msk             (0x1U << PMC_SLPWK_ER1_PID47_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 47 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID47                 PMC_SLPWK_ER1_PID47_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID47_Msk instead */
#define PMC_SLPWK_ER1_PID48_Pos             16                                             /**< (PMC_SLPWK_ER1) Peripheral 48 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID48_Msk             (0x1U << PMC_SLPWK_ER1_PID48_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 48 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID48                 PMC_SLPWK_ER1_PID48_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID48_Msk instead */
#define PMC_SLPWK_ER1_PID49_Pos             17                                             /**< (PMC_SLPWK_ER1) Peripheral 49 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID49_Msk             (0x1U << PMC_SLPWK_ER1_PID49_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 49 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID49                 PMC_SLPWK_ER1_PID49_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID49_Msk instead */
#define PMC_SLPWK_ER1_PID50_Pos             18                                             /**< (PMC_SLPWK_ER1) Peripheral 50 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID50_Msk             (0x1U << PMC_SLPWK_ER1_PID50_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 50 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID50                 PMC_SLPWK_ER1_PID50_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID50_Msk instead */
#define PMC_SLPWK_ER1_PID51_Pos             19                                             /**< (PMC_SLPWK_ER1) Peripheral 51 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID51_Msk             (0x1U << PMC_SLPWK_ER1_PID51_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 51 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID51                 PMC_SLPWK_ER1_PID51_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID51_Msk instead */
#define PMC_SLPWK_ER1_PID52_Pos             20                                             /**< (PMC_SLPWK_ER1) Peripheral 52 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID52_Msk             (0x1U << PMC_SLPWK_ER1_PID52_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 52 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID52                 PMC_SLPWK_ER1_PID52_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID52_Msk instead */
#define PMC_SLPWK_ER1_PID53_Pos             21                                             /**< (PMC_SLPWK_ER1) Peripheral 53 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID53_Msk             (0x1U << PMC_SLPWK_ER1_PID53_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 53 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID53                 PMC_SLPWK_ER1_PID53_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID53_Msk instead */
#define PMC_SLPWK_ER1_PID56_Pos             24                                             /**< (PMC_SLPWK_ER1) Peripheral 56 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID56_Msk             (0x1U << PMC_SLPWK_ER1_PID56_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 56 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID56                 PMC_SLPWK_ER1_PID56_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID56_Msk instead */
#define PMC_SLPWK_ER1_PID57_Pos             25                                             /**< (PMC_SLPWK_ER1) Peripheral 57 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID57_Msk             (0x1U << PMC_SLPWK_ER1_PID57_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 57 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID57                 PMC_SLPWK_ER1_PID57_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID57_Msk instead */
#define PMC_SLPWK_ER1_PID58_Pos             26                                             /**< (PMC_SLPWK_ER1) Peripheral 58 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID58_Msk             (0x1U << PMC_SLPWK_ER1_PID58_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 58 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID58                 PMC_SLPWK_ER1_PID58_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID58_Msk instead */
#define PMC_SLPWK_ER1_PID59_Pos             27                                             /**< (PMC_SLPWK_ER1) Peripheral 59 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID59_Msk             (0x1U << PMC_SLPWK_ER1_PID59_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 59 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID59                 PMC_SLPWK_ER1_PID59_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID59_Msk instead */
#define PMC_SLPWK_ER1_PID60_Pos             28                                             /**< (PMC_SLPWK_ER1) Peripheral 60 SleepWalking Enable Position */
#define PMC_SLPWK_ER1_PID60_Msk             (0x1U << PMC_SLPWK_ER1_PID60_Pos)              /**< (PMC_SLPWK_ER1) Peripheral 60 SleepWalking Enable Mask */
#define PMC_SLPWK_ER1_PID60                 PMC_SLPWK_ER1_PID60_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ER1_PID60_Msk instead */
#define PMC_SLPWK_ER1_PID_Pos               0                                              /**< (PMC_SLPWK_ER1 Position) Peripheral 6x SleepWalking Enable */
#define PMC_SLPWK_ER1_PID_Msk               (0x1FFFFFFU << PMC_SLPWK_ER1_PID_Pos)          /**< (PMC_SLPWK_ER1 Mask) PID */
#define PMC_SLPWK_ER1_PID(value)            (PMC_SLPWK_ER1_PID_Msk & ((value) << PMC_SLPWK_ER1_PID_Pos))  
#define PMC_SLPWK_ER1_MASK                  (0x1F3FFFAFU)                                  /**< \deprecated (PMC_SLPWK_ER1) Register MASK  (Use PMC_SLPWK_ER1_Msk instead)  */
#define PMC_SLPWK_ER1_Msk                   (0x1F3FFFAFU)                                  /**< (PMC_SLPWK_ER1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_DR1 : (PMC Offset: 0x138) (/W 32) SleepWalking Disable Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PID32:1;                   /**< bit:      0  Peripheral 32 SleepWalking Disable       */
    uint32_t PID33:1;                   /**< bit:      1  Peripheral 33 SleepWalking Disable       */
    uint32_t PID34:1;                   /**< bit:      2  Peripheral 34 SleepWalking Disable       */
    uint32_t PID35:1;                   /**< bit:      3  Peripheral 35 SleepWalking Disable       */
    uint32_t :1;                        /**< bit:      4  Reserved */
    uint32_t PID37:1;                   /**< bit:      5  Peripheral 37 SleepWalking Disable       */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t PID39:1;                   /**< bit:      7  Peripheral 39 SleepWalking Disable       */
    uint32_t PID40:1;                   /**< bit:      8  Peripheral 40 SleepWalking Disable       */
    uint32_t PID41:1;                   /**< bit:      9  Peripheral 41 SleepWalking Disable       */
    uint32_t PID42:1;                   /**< bit:     10  Peripheral 42 SleepWalking Disable       */
    uint32_t PID43:1;                   /**< bit:     11  Peripheral 43 SleepWalking Disable       */
    uint32_t PID44:1;                   /**< bit:     12  Peripheral 44 SleepWalking Disable       */
    uint32_t PID45:1;                   /**< bit:     13  Peripheral 45 SleepWalking Disable       */
    uint32_t PID46:1;                   /**< bit:     14  Peripheral 46 SleepWalking Disable       */
    uint32_t PID47:1;                   /**< bit:     15  Peripheral 47 SleepWalking Disable       */
    uint32_t PID48:1;                   /**< bit:     16  Peripheral 48 SleepWalking Disable       */
    uint32_t PID49:1;                   /**< bit:     17  Peripheral 49 SleepWalking Disable       */
    uint32_t PID50:1;                   /**< bit:     18  Peripheral 50 SleepWalking Disable       */
    uint32_t PID51:1;                   /**< bit:     19  Peripheral 51 SleepWalking Disable       */
    uint32_t PID52:1;                   /**< bit:     20  Peripheral 52 SleepWalking Disable       */
    uint32_t PID53:1;                   /**< bit:     21  Peripheral 53 SleepWalking Disable       */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t PID56:1;                   /**< bit:     24  Peripheral 56 SleepWalking Disable       */
    uint32_t PID57:1;                   /**< bit:     25  Peripheral 57 SleepWalking Disable       */
    uint32_t PID58:1;                   /**< bit:     26  Peripheral 58 SleepWalking Disable       */
    uint32_t PID59:1;                   /**< bit:     27  Peripheral 59 SleepWalking Disable       */
    uint32_t PID60:1;                   /**< bit:     28  Peripheral 60 SleepWalking Disable       */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t PID:25;                    /**< bit:  0..24  Peripheral 6x SleepWalking Disable       */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_DR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_DR1_OFFSET                (0x138)                                       /**<  (PMC_SLPWK_DR1) SleepWalking Disable Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_DR1_PID32_Pos             0                                              /**< (PMC_SLPWK_DR1) Peripheral 32 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID32_Msk             (0x1U << PMC_SLPWK_DR1_PID32_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 32 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID32                 PMC_SLPWK_DR1_PID32_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID32_Msk instead */
#define PMC_SLPWK_DR1_PID33_Pos             1                                              /**< (PMC_SLPWK_DR1) Peripheral 33 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID33_Msk             (0x1U << PMC_SLPWK_DR1_PID33_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 33 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID33                 PMC_SLPWK_DR1_PID33_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID33_Msk instead */
#define PMC_SLPWK_DR1_PID34_Pos             2                                              /**< (PMC_SLPWK_DR1) Peripheral 34 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID34_Msk             (0x1U << PMC_SLPWK_DR1_PID34_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 34 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID34                 PMC_SLPWK_DR1_PID34_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID34_Msk instead */
#define PMC_SLPWK_DR1_PID35_Pos             3                                              /**< (PMC_SLPWK_DR1) Peripheral 35 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID35_Msk             (0x1U << PMC_SLPWK_DR1_PID35_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 35 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID35                 PMC_SLPWK_DR1_PID35_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID35_Msk instead */
#define PMC_SLPWK_DR1_PID37_Pos             5                                              /**< (PMC_SLPWK_DR1) Peripheral 37 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID37_Msk             (0x1U << PMC_SLPWK_DR1_PID37_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 37 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID37                 PMC_SLPWK_DR1_PID37_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID37_Msk instead */
#define PMC_SLPWK_DR1_PID39_Pos             7                                              /**< (PMC_SLPWK_DR1) Peripheral 39 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID39_Msk             (0x1U << PMC_SLPWK_DR1_PID39_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 39 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID39                 PMC_SLPWK_DR1_PID39_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID39_Msk instead */
#define PMC_SLPWK_DR1_PID40_Pos             8                                              /**< (PMC_SLPWK_DR1) Peripheral 40 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID40_Msk             (0x1U << PMC_SLPWK_DR1_PID40_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 40 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID40                 PMC_SLPWK_DR1_PID40_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID40_Msk instead */
#define PMC_SLPWK_DR1_PID41_Pos             9                                              /**< (PMC_SLPWK_DR1) Peripheral 41 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID41_Msk             (0x1U << PMC_SLPWK_DR1_PID41_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 41 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID41                 PMC_SLPWK_DR1_PID41_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID41_Msk instead */
#define PMC_SLPWK_DR1_PID42_Pos             10                                             /**< (PMC_SLPWK_DR1) Peripheral 42 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID42_Msk             (0x1U << PMC_SLPWK_DR1_PID42_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 42 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID42                 PMC_SLPWK_DR1_PID42_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID42_Msk instead */
#define PMC_SLPWK_DR1_PID43_Pos             11                                             /**< (PMC_SLPWK_DR1) Peripheral 43 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID43_Msk             (0x1U << PMC_SLPWK_DR1_PID43_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 43 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID43                 PMC_SLPWK_DR1_PID43_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID43_Msk instead */
#define PMC_SLPWK_DR1_PID44_Pos             12                                             /**< (PMC_SLPWK_DR1) Peripheral 44 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID44_Msk             (0x1U << PMC_SLPWK_DR1_PID44_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 44 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID44                 PMC_SLPWK_DR1_PID44_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID44_Msk instead */
#define PMC_SLPWK_DR1_PID45_Pos             13                                             /**< (PMC_SLPWK_DR1) Peripheral 45 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID45_Msk             (0x1U << PMC_SLPWK_DR1_PID45_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 45 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID45                 PMC_SLPWK_DR1_PID45_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID45_Msk instead */
#define PMC_SLPWK_DR1_PID46_Pos             14                                             /**< (PMC_SLPWK_DR1) Peripheral 46 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID46_Msk             (0x1U << PMC_SLPWK_DR1_PID46_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 46 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID46                 PMC_SLPWK_DR1_PID46_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID46_Msk instead */
#define PMC_SLPWK_DR1_PID47_Pos             15                                             /**< (PMC_SLPWK_DR1) Peripheral 47 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID47_Msk             (0x1U << PMC_SLPWK_DR1_PID47_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 47 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID47                 PMC_SLPWK_DR1_PID47_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID47_Msk instead */
#define PMC_SLPWK_DR1_PID48_Pos             16                                             /**< (PMC_SLPWK_DR1) Peripheral 48 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID48_Msk             (0x1U << PMC_SLPWK_DR1_PID48_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 48 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID48                 PMC_SLPWK_DR1_PID48_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID48_Msk instead */
#define PMC_SLPWK_DR1_PID49_Pos             17                                             /**< (PMC_SLPWK_DR1) Peripheral 49 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID49_Msk             (0x1U << PMC_SLPWK_DR1_PID49_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 49 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID49                 PMC_SLPWK_DR1_PID49_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID49_Msk instead */
#define PMC_SLPWK_DR1_PID50_Pos             18                                             /**< (PMC_SLPWK_DR1) Peripheral 50 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID50_Msk             (0x1U << PMC_SLPWK_DR1_PID50_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 50 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID50                 PMC_SLPWK_DR1_PID50_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID50_Msk instead */
#define PMC_SLPWK_DR1_PID51_Pos             19                                             /**< (PMC_SLPWK_DR1) Peripheral 51 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID51_Msk             (0x1U << PMC_SLPWK_DR1_PID51_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 51 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID51                 PMC_SLPWK_DR1_PID51_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID51_Msk instead */
#define PMC_SLPWK_DR1_PID52_Pos             20                                             /**< (PMC_SLPWK_DR1) Peripheral 52 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID52_Msk             (0x1U << PMC_SLPWK_DR1_PID52_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 52 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID52                 PMC_SLPWK_DR1_PID52_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID52_Msk instead */
#define PMC_SLPWK_DR1_PID53_Pos             21                                             /**< (PMC_SLPWK_DR1) Peripheral 53 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID53_Msk             (0x1U << PMC_SLPWK_DR1_PID53_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 53 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID53                 PMC_SLPWK_DR1_PID53_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID53_Msk instead */
#define PMC_SLPWK_DR1_PID56_Pos             24                                             /**< (PMC_SLPWK_DR1) Peripheral 56 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID56_Msk             (0x1U << PMC_SLPWK_DR1_PID56_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 56 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID56                 PMC_SLPWK_DR1_PID56_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID56_Msk instead */
#define PMC_SLPWK_DR1_PID57_Pos             25                                             /**< (PMC_SLPWK_DR1) Peripheral 57 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID57_Msk             (0x1U << PMC_SLPWK_DR1_PID57_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 57 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID57                 PMC_SLPWK_DR1_PID57_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID57_Msk instead */
#define PMC_SLPWK_DR1_PID58_Pos             26                                             /**< (PMC_SLPWK_DR1) Peripheral 58 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID58_Msk             (0x1U << PMC_SLPWK_DR1_PID58_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 58 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID58                 PMC_SLPWK_DR1_PID58_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID58_Msk instead */
#define PMC_SLPWK_DR1_PID59_Pos             27                                             /**< (PMC_SLPWK_DR1) Peripheral 59 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID59_Msk             (0x1U << PMC_SLPWK_DR1_PID59_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 59 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID59                 PMC_SLPWK_DR1_PID59_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID59_Msk instead */
#define PMC_SLPWK_DR1_PID60_Pos             28                                             /**< (PMC_SLPWK_DR1) Peripheral 60 SleepWalking Disable Position */
#define PMC_SLPWK_DR1_PID60_Msk             (0x1U << PMC_SLPWK_DR1_PID60_Pos)              /**< (PMC_SLPWK_DR1) Peripheral 60 SleepWalking Disable Mask */
#define PMC_SLPWK_DR1_PID60                 PMC_SLPWK_DR1_PID60_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_DR1_PID60_Msk instead */
#define PMC_SLPWK_DR1_PID_Pos               0                                              /**< (PMC_SLPWK_DR1 Position) Peripheral 6x SleepWalking Disable */
#define PMC_SLPWK_DR1_PID_Msk               (0x1FFFFFFU << PMC_SLPWK_DR1_PID_Pos)          /**< (PMC_SLPWK_DR1 Mask) PID */
#define PMC_SLPWK_DR1_PID(value)            (PMC_SLPWK_DR1_PID_Msk & ((value) << PMC_SLPWK_DR1_PID_Pos))  
#define PMC_SLPWK_DR1_MASK                  (0x1F3FFFAFU)                                  /**< \deprecated (PMC_SLPWK_DR1) Register MASK  (Use PMC_SLPWK_DR1_Msk instead)  */
#define PMC_SLPWK_DR1_Msk                   (0x1F3FFFAFU)                                  /**< (PMC_SLPWK_DR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_SR1 : (PMC Offset: 0x13c) (R/ 32) SleepWalking Status Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PID32:1;                   /**< bit:      0  Peripheral 32 SleepWalking Status        */
    uint32_t PID33:1;                   /**< bit:      1  Peripheral 33 SleepWalking Status        */
    uint32_t PID34:1;                   /**< bit:      2  Peripheral 34 SleepWalking Status        */
    uint32_t PID35:1;                   /**< bit:      3  Peripheral 35 SleepWalking Status        */
    uint32_t :1;                        /**< bit:      4  Reserved */
    uint32_t PID37:1;                   /**< bit:      5  Peripheral 37 SleepWalking Status        */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t PID39:1;                   /**< bit:      7  Peripheral 39 SleepWalking Status        */
    uint32_t PID40:1;                   /**< bit:      8  Peripheral 40 SleepWalking Status        */
    uint32_t PID41:1;                   /**< bit:      9  Peripheral 41 SleepWalking Status        */
    uint32_t PID42:1;                   /**< bit:     10  Peripheral 42 SleepWalking Status        */
    uint32_t PID43:1;                   /**< bit:     11  Peripheral 43 SleepWalking Status        */
    uint32_t PID44:1;                   /**< bit:     12  Peripheral 44 SleepWalking Status        */
    uint32_t PID45:1;                   /**< bit:     13  Peripheral 45 SleepWalking Status        */
    uint32_t PID46:1;                   /**< bit:     14  Peripheral 46 SleepWalking Status        */
    uint32_t PID47:1;                   /**< bit:     15  Peripheral 47 SleepWalking Status        */
    uint32_t PID48:1;                   /**< bit:     16  Peripheral 48 SleepWalking Status        */
    uint32_t PID49:1;                   /**< bit:     17  Peripheral 49 SleepWalking Status        */
    uint32_t PID50:1;                   /**< bit:     18  Peripheral 50 SleepWalking Status        */
    uint32_t PID51:1;                   /**< bit:     19  Peripheral 51 SleepWalking Status        */
    uint32_t PID52:1;                   /**< bit:     20  Peripheral 52 SleepWalking Status        */
    uint32_t PID53:1;                   /**< bit:     21  Peripheral 53 SleepWalking Status        */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t PID56:1;                   /**< bit:     24  Peripheral 56 SleepWalking Status        */
    uint32_t PID57:1;                   /**< bit:     25  Peripheral 57 SleepWalking Status        */
    uint32_t PID58:1;                   /**< bit:     26  Peripheral 58 SleepWalking Status        */
    uint32_t PID59:1;                   /**< bit:     27  Peripheral 59 SleepWalking Status        */
    uint32_t PID60:1;                   /**< bit:     28  Peripheral 60 SleepWalking Status        */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t PID:25;                    /**< bit:  0..24  Peripheral 6x SleepWalking Status        */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_SR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_SR1_OFFSET                (0x13C)                                       /**<  (PMC_SLPWK_SR1) SleepWalking Status Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_SR1_PID32_Pos             0                                              /**< (PMC_SLPWK_SR1) Peripheral 32 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID32_Msk             (0x1U << PMC_SLPWK_SR1_PID32_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 32 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID32                 PMC_SLPWK_SR1_PID32_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID32_Msk instead */
#define PMC_SLPWK_SR1_PID33_Pos             1                                              /**< (PMC_SLPWK_SR1) Peripheral 33 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID33_Msk             (0x1U << PMC_SLPWK_SR1_PID33_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 33 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID33                 PMC_SLPWK_SR1_PID33_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID33_Msk instead */
#define PMC_SLPWK_SR1_PID34_Pos             2                                              /**< (PMC_SLPWK_SR1) Peripheral 34 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID34_Msk             (0x1U << PMC_SLPWK_SR1_PID34_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 34 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID34                 PMC_SLPWK_SR1_PID34_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID34_Msk instead */
#define PMC_SLPWK_SR1_PID35_Pos             3                                              /**< (PMC_SLPWK_SR1) Peripheral 35 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID35_Msk             (0x1U << PMC_SLPWK_SR1_PID35_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 35 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID35                 PMC_SLPWK_SR1_PID35_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID35_Msk instead */
#define PMC_SLPWK_SR1_PID37_Pos             5                                              /**< (PMC_SLPWK_SR1) Peripheral 37 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID37_Msk             (0x1U << PMC_SLPWK_SR1_PID37_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 37 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID37                 PMC_SLPWK_SR1_PID37_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID37_Msk instead */
#define PMC_SLPWK_SR1_PID39_Pos             7                                              /**< (PMC_SLPWK_SR1) Peripheral 39 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID39_Msk             (0x1U << PMC_SLPWK_SR1_PID39_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 39 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID39                 PMC_SLPWK_SR1_PID39_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID39_Msk instead */
#define PMC_SLPWK_SR1_PID40_Pos             8                                              /**< (PMC_SLPWK_SR1) Peripheral 40 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID40_Msk             (0x1U << PMC_SLPWK_SR1_PID40_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 40 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID40                 PMC_SLPWK_SR1_PID40_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID40_Msk instead */
#define PMC_SLPWK_SR1_PID41_Pos             9                                              /**< (PMC_SLPWK_SR1) Peripheral 41 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID41_Msk             (0x1U << PMC_SLPWK_SR1_PID41_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 41 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID41                 PMC_SLPWK_SR1_PID41_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID41_Msk instead */
#define PMC_SLPWK_SR1_PID42_Pos             10                                             /**< (PMC_SLPWK_SR1) Peripheral 42 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID42_Msk             (0x1U << PMC_SLPWK_SR1_PID42_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 42 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID42                 PMC_SLPWK_SR1_PID42_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID42_Msk instead */
#define PMC_SLPWK_SR1_PID43_Pos             11                                             /**< (PMC_SLPWK_SR1) Peripheral 43 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID43_Msk             (0x1U << PMC_SLPWK_SR1_PID43_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 43 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID43                 PMC_SLPWK_SR1_PID43_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID43_Msk instead */
#define PMC_SLPWK_SR1_PID44_Pos             12                                             /**< (PMC_SLPWK_SR1) Peripheral 44 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID44_Msk             (0x1U << PMC_SLPWK_SR1_PID44_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 44 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID44                 PMC_SLPWK_SR1_PID44_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID44_Msk instead */
#define PMC_SLPWK_SR1_PID45_Pos             13                                             /**< (PMC_SLPWK_SR1) Peripheral 45 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID45_Msk             (0x1U << PMC_SLPWK_SR1_PID45_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 45 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID45                 PMC_SLPWK_SR1_PID45_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID45_Msk instead */
#define PMC_SLPWK_SR1_PID46_Pos             14                                             /**< (PMC_SLPWK_SR1) Peripheral 46 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID46_Msk             (0x1U << PMC_SLPWK_SR1_PID46_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 46 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID46                 PMC_SLPWK_SR1_PID46_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID46_Msk instead */
#define PMC_SLPWK_SR1_PID47_Pos             15                                             /**< (PMC_SLPWK_SR1) Peripheral 47 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID47_Msk             (0x1U << PMC_SLPWK_SR1_PID47_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 47 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID47                 PMC_SLPWK_SR1_PID47_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID47_Msk instead */
#define PMC_SLPWK_SR1_PID48_Pos             16                                             /**< (PMC_SLPWK_SR1) Peripheral 48 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID48_Msk             (0x1U << PMC_SLPWK_SR1_PID48_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 48 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID48                 PMC_SLPWK_SR1_PID48_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID48_Msk instead */
#define PMC_SLPWK_SR1_PID49_Pos             17                                             /**< (PMC_SLPWK_SR1) Peripheral 49 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID49_Msk             (0x1U << PMC_SLPWK_SR1_PID49_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 49 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID49                 PMC_SLPWK_SR1_PID49_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID49_Msk instead */
#define PMC_SLPWK_SR1_PID50_Pos             18                                             /**< (PMC_SLPWK_SR1) Peripheral 50 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID50_Msk             (0x1U << PMC_SLPWK_SR1_PID50_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 50 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID50                 PMC_SLPWK_SR1_PID50_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID50_Msk instead */
#define PMC_SLPWK_SR1_PID51_Pos             19                                             /**< (PMC_SLPWK_SR1) Peripheral 51 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID51_Msk             (0x1U << PMC_SLPWK_SR1_PID51_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 51 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID51                 PMC_SLPWK_SR1_PID51_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID51_Msk instead */
#define PMC_SLPWK_SR1_PID52_Pos             20                                             /**< (PMC_SLPWK_SR1) Peripheral 52 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID52_Msk             (0x1U << PMC_SLPWK_SR1_PID52_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 52 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID52                 PMC_SLPWK_SR1_PID52_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID52_Msk instead */
#define PMC_SLPWK_SR1_PID53_Pos             21                                             /**< (PMC_SLPWK_SR1) Peripheral 53 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID53_Msk             (0x1U << PMC_SLPWK_SR1_PID53_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 53 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID53                 PMC_SLPWK_SR1_PID53_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID53_Msk instead */
#define PMC_SLPWK_SR1_PID56_Pos             24                                             /**< (PMC_SLPWK_SR1) Peripheral 56 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID56_Msk             (0x1U << PMC_SLPWK_SR1_PID56_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 56 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID56                 PMC_SLPWK_SR1_PID56_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID56_Msk instead */
#define PMC_SLPWK_SR1_PID57_Pos             25                                             /**< (PMC_SLPWK_SR1) Peripheral 57 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID57_Msk             (0x1U << PMC_SLPWK_SR1_PID57_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 57 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID57                 PMC_SLPWK_SR1_PID57_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID57_Msk instead */
#define PMC_SLPWK_SR1_PID58_Pos             26                                             /**< (PMC_SLPWK_SR1) Peripheral 58 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID58_Msk             (0x1U << PMC_SLPWK_SR1_PID58_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 58 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID58                 PMC_SLPWK_SR1_PID58_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID58_Msk instead */
#define PMC_SLPWK_SR1_PID59_Pos             27                                             /**< (PMC_SLPWK_SR1) Peripheral 59 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID59_Msk             (0x1U << PMC_SLPWK_SR1_PID59_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 59 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID59                 PMC_SLPWK_SR1_PID59_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID59_Msk instead */
#define PMC_SLPWK_SR1_PID60_Pos             28                                             /**< (PMC_SLPWK_SR1) Peripheral 60 SleepWalking Status Position */
#define PMC_SLPWK_SR1_PID60_Msk             (0x1U << PMC_SLPWK_SR1_PID60_Pos)              /**< (PMC_SLPWK_SR1) Peripheral 60 SleepWalking Status Mask */
#define PMC_SLPWK_SR1_PID60                 PMC_SLPWK_SR1_PID60_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_SR1_PID60_Msk instead */
#define PMC_SLPWK_SR1_PID_Pos               0                                              /**< (PMC_SLPWK_SR1 Position) Peripheral 6x SleepWalking Status */
#define PMC_SLPWK_SR1_PID_Msk               (0x1FFFFFFU << PMC_SLPWK_SR1_PID_Pos)          /**< (PMC_SLPWK_SR1 Mask) PID */
#define PMC_SLPWK_SR1_PID(value)            (PMC_SLPWK_SR1_PID_Msk & ((value) << PMC_SLPWK_SR1_PID_Pos))  
#define PMC_SLPWK_SR1_MASK                  (0x1F3FFFAFU)                                  /**< \deprecated (PMC_SLPWK_SR1) Register MASK  (Use PMC_SLPWK_SR1_Msk instead)  */
#define PMC_SLPWK_SR1_Msk                   (0x1F3FFFAFU)                                  /**< (PMC_SLPWK_SR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_ASR1 : (PMC Offset: 0x140) (R/ 32) SleepWalking Activity Status Register 1 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PID32:1;                   /**< bit:      0  Peripheral 32 Activity Status            */
    uint32_t PID33:1;                   /**< bit:      1  Peripheral 33 Activity Status            */
    uint32_t PID34:1;                   /**< bit:      2  Peripheral 34 Activity Status            */
    uint32_t PID35:1;                   /**< bit:      3  Peripheral 35 Activity Status            */
    uint32_t :1;                        /**< bit:      4  Reserved */
    uint32_t PID37:1;                   /**< bit:      5  Peripheral 37 Activity Status            */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t PID39:1;                   /**< bit:      7  Peripheral 39 Activity Status            */
    uint32_t PID40:1;                   /**< bit:      8  Peripheral 40 Activity Status            */
    uint32_t PID41:1;                   /**< bit:      9  Peripheral 41 Activity Status            */
    uint32_t PID42:1;                   /**< bit:     10  Peripheral 42 Activity Status            */
    uint32_t PID43:1;                   /**< bit:     11  Peripheral 43 Activity Status            */
    uint32_t PID44:1;                   /**< bit:     12  Peripheral 44 Activity Status            */
    uint32_t PID45:1;                   /**< bit:     13  Peripheral 45 Activity Status            */
    uint32_t PID46:1;                   /**< bit:     14  Peripheral 46 Activity Status            */
    uint32_t PID47:1;                   /**< bit:     15  Peripheral 47 Activity Status            */
    uint32_t PID48:1;                   /**< bit:     16  Peripheral 48 Activity Status            */
    uint32_t PID49:1;                   /**< bit:     17  Peripheral 49 Activity Status            */
    uint32_t PID50:1;                   /**< bit:     18  Peripheral 50 Activity Status            */
    uint32_t PID51:1;                   /**< bit:     19  Peripheral 51 Activity Status            */
    uint32_t PID52:1;                   /**< bit:     20  Peripheral 52 Activity Status            */
    uint32_t PID53:1;                   /**< bit:     21  Peripheral 53 Activity Status            */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t PID56:1;                   /**< bit:     24  Peripheral 56 Activity Status            */
    uint32_t PID57:1;                   /**< bit:     25  Peripheral 57 Activity Status            */
    uint32_t PID58:1;                   /**< bit:     26  Peripheral 58 Activity Status            */
    uint32_t PID59:1;                   /**< bit:     27  Peripheral 59 Activity Status            */
    uint32_t PID60:1;                   /**< bit:     28  Peripheral 60 Activity Status            */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t PID:25;                    /**< bit:  0..24  Peripheral 6x Activity Status            */
    uint32_t :7;                        /**< bit: 25..31 Reserved */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_ASR1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_ASR1_OFFSET               (0x140)                                       /**<  (PMC_SLPWK_ASR1) SleepWalking Activity Status Register 1  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_ASR1_PID32_Pos            0                                              /**< (PMC_SLPWK_ASR1) Peripheral 32 Activity Status Position */
#define PMC_SLPWK_ASR1_PID32_Msk            (0x1U << PMC_SLPWK_ASR1_PID32_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 32 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID32                PMC_SLPWK_ASR1_PID32_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID32_Msk instead */
#define PMC_SLPWK_ASR1_PID33_Pos            1                                              /**< (PMC_SLPWK_ASR1) Peripheral 33 Activity Status Position */
#define PMC_SLPWK_ASR1_PID33_Msk            (0x1U << PMC_SLPWK_ASR1_PID33_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 33 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID33                PMC_SLPWK_ASR1_PID33_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID33_Msk instead */
#define PMC_SLPWK_ASR1_PID34_Pos            2                                              /**< (PMC_SLPWK_ASR1) Peripheral 34 Activity Status Position */
#define PMC_SLPWK_ASR1_PID34_Msk            (0x1U << PMC_SLPWK_ASR1_PID34_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 34 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID34                PMC_SLPWK_ASR1_PID34_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID34_Msk instead */
#define PMC_SLPWK_ASR1_PID35_Pos            3                                              /**< (PMC_SLPWK_ASR1) Peripheral 35 Activity Status Position */
#define PMC_SLPWK_ASR1_PID35_Msk            (0x1U << PMC_SLPWK_ASR1_PID35_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 35 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID35                PMC_SLPWK_ASR1_PID35_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID35_Msk instead */
#define PMC_SLPWK_ASR1_PID37_Pos            5                                              /**< (PMC_SLPWK_ASR1) Peripheral 37 Activity Status Position */
#define PMC_SLPWK_ASR1_PID37_Msk            (0x1U << PMC_SLPWK_ASR1_PID37_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 37 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID37                PMC_SLPWK_ASR1_PID37_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID37_Msk instead */
#define PMC_SLPWK_ASR1_PID39_Pos            7                                              /**< (PMC_SLPWK_ASR1) Peripheral 39 Activity Status Position */
#define PMC_SLPWK_ASR1_PID39_Msk            (0x1U << PMC_SLPWK_ASR1_PID39_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 39 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID39                PMC_SLPWK_ASR1_PID39_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID39_Msk instead */
#define PMC_SLPWK_ASR1_PID40_Pos            8                                              /**< (PMC_SLPWK_ASR1) Peripheral 40 Activity Status Position */
#define PMC_SLPWK_ASR1_PID40_Msk            (0x1U << PMC_SLPWK_ASR1_PID40_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 40 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID40                PMC_SLPWK_ASR1_PID40_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID40_Msk instead */
#define PMC_SLPWK_ASR1_PID41_Pos            9                                              /**< (PMC_SLPWK_ASR1) Peripheral 41 Activity Status Position */
#define PMC_SLPWK_ASR1_PID41_Msk            (0x1U << PMC_SLPWK_ASR1_PID41_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 41 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID41                PMC_SLPWK_ASR1_PID41_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID41_Msk instead */
#define PMC_SLPWK_ASR1_PID42_Pos            10                                             /**< (PMC_SLPWK_ASR1) Peripheral 42 Activity Status Position */
#define PMC_SLPWK_ASR1_PID42_Msk            (0x1U << PMC_SLPWK_ASR1_PID42_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 42 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID42                PMC_SLPWK_ASR1_PID42_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID42_Msk instead */
#define PMC_SLPWK_ASR1_PID43_Pos            11                                             /**< (PMC_SLPWK_ASR1) Peripheral 43 Activity Status Position */
#define PMC_SLPWK_ASR1_PID43_Msk            (0x1U << PMC_SLPWK_ASR1_PID43_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 43 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID43                PMC_SLPWK_ASR1_PID43_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID43_Msk instead */
#define PMC_SLPWK_ASR1_PID44_Pos            12                                             /**< (PMC_SLPWK_ASR1) Peripheral 44 Activity Status Position */
#define PMC_SLPWK_ASR1_PID44_Msk            (0x1U << PMC_SLPWK_ASR1_PID44_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 44 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID44                PMC_SLPWK_ASR1_PID44_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID44_Msk instead */
#define PMC_SLPWK_ASR1_PID45_Pos            13                                             /**< (PMC_SLPWK_ASR1) Peripheral 45 Activity Status Position */
#define PMC_SLPWK_ASR1_PID45_Msk            (0x1U << PMC_SLPWK_ASR1_PID45_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 45 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID45                PMC_SLPWK_ASR1_PID45_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID45_Msk instead */
#define PMC_SLPWK_ASR1_PID46_Pos            14                                             /**< (PMC_SLPWK_ASR1) Peripheral 46 Activity Status Position */
#define PMC_SLPWK_ASR1_PID46_Msk            (0x1U << PMC_SLPWK_ASR1_PID46_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 46 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID46                PMC_SLPWK_ASR1_PID46_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID46_Msk instead */
#define PMC_SLPWK_ASR1_PID47_Pos            15                                             /**< (PMC_SLPWK_ASR1) Peripheral 47 Activity Status Position */
#define PMC_SLPWK_ASR1_PID47_Msk            (0x1U << PMC_SLPWK_ASR1_PID47_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 47 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID47                PMC_SLPWK_ASR1_PID47_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID47_Msk instead */
#define PMC_SLPWK_ASR1_PID48_Pos            16                                             /**< (PMC_SLPWK_ASR1) Peripheral 48 Activity Status Position */
#define PMC_SLPWK_ASR1_PID48_Msk            (0x1U << PMC_SLPWK_ASR1_PID48_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 48 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID48                PMC_SLPWK_ASR1_PID48_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID48_Msk instead */
#define PMC_SLPWK_ASR1_PID49_Pos            17                                             /**< (PMC_SLPWK_ASR1) Peripheral 49 Activity Status Position */
#define PMC_SLPWK_ASR1_PID49_Msk            (0x1U << PMC_SLPWK_ASR1_PID49_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 49 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID49                PMC_SLPWK_ASR1_PID49_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID49_Msk instead */
#define PMC_SLPWK_ASR1_PID50_Pos            18                                             /**< (PMC_SLPWK_ASR1) Peripheral 50 Activity Status Position */
#define PMC_SLPWK_ASR1_PID50_Msk            (0x1U << PMC_SLPWK_ASR1_PID50_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 50 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID50                PMC_SLPWK_ASR1_PID50_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID50_Msk instead */
#define PMC_SLPWK_ASR1_PID51_Pos            19                                             /**< (PMC_SLPWK_ASR1) Peripheral 51 Activity Status Position */
#define PMC_SLPWK_ASR1_PID51_Msk            (0x1U << PMC_SLPWK_ASR1_PID51_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 51 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID51                PMC_SLPWK_ASR1_PID51_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID51_Msk instead */
#define PMC_SLPWK_ASR1_PID52_Pos            20                                             /**< (PMC_SLPWK_ASR1) Peripheral 52 Activity Status Position */
#define PMC_SLPWK_ASR1_PID52_Msk            (0x1U << PMC_SLPWK_ASR1_PID52_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 52 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID52                PMC_SLPWK_ASR1_PID52_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID52_Msk instead */
#define PMC_SLPWK_ASR1_PID53_Pos            21                                             /**< (PMC_SLPWK_ASR1) Peripheral 53 Activity Status Position */
#define PMC_SLPWK_ASR1_PID53_Msk            (0x1U << PMC_SLPWK_ASR1_PID53_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 53 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID53                PMC_SLPWK_ASR1_PID53_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID53_Msk instead */
#define PMC_SLPWK_ASR1_PID56_Pos            24                                             /**< (PMC_SLPWK_ASR1) Peripheral 56 Activity Status Position */
#define PMC_SLPWK_ASR1_PID56_Msk            (0x1U << PMC_SLPWK_ASR1_PID56_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 56 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID56                PMC_SLPWK_ASR1_PID56_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID56_Msk instead */
#define PMC_SLPWK_ASR1_PID57_Pos            25                                             /**< (PMC_SLPWK_ASR1) Peripheral 57 Activity Status Position */
#define PMC_SLPWK_ASR1_PID57_Msk            (0x1U << PMC_SLPWK_ASR1_PID57_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 57 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID57                PMC_SLPWK_ASR1_PID57_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID57_Msk instead */
#define PMC_SLPWK_ASR1_PID58_Pos            26                                             /**< (PMC_SLPWK_ASR1) Peripheral 58 Activity Status Position */
#define PMC_SLPWK_ASR1_PID58_Msk            (0x1U << PMC_SLPWK_ASR1_PID58_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 58 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID58                PMC_SLPWK_ASR1_PID58_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID58_Msk instead */
#define PMC_SLPWK_ASR1_PID59_Pos            27                                             /**< (PMC_SLPWK_ASR1) Peripheral 59 Activity Status Position */
#define PMC_SLPWK_ASR1_PID59_Msk            (0x1U << PMC_SLPWK_ASR1_PID59_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 59 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID59                PMC_SLPWK_ASR1_PID59_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID59_Msk instead */
#define PMC_SLPWK_ASR1_PID60_Pos            28                                             /**< (PMC_SLPWK_ASR1) Peripheral 60 Activity Status Position */
#define PMC_SLPWK_ASR1_PID60_Msk            (0x1U << PMC_SLPWK_ASR1_PID60_Pos)             /**< (PMC_SLPWK_ASR1) Peripheral 60 Activity Status Mask */
#define PMC_SLPWK_ASR1_PID60                PMC_SLPWK_ASR1_PID60_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_ASR1_PID60_Msk instead */
#define PMC_SLPWK_ASR1_PID_Pos              0                                              /**< (PMC_SLPWK_ASR1 Position) Peripheral 6x Activity Status */
#define PMC_SLPWK_ASR1_PID_Msk              (0x1FFFFFFU << PMC_SLPWK_ASR1_PID_Pos)         /**< (PMC_SLPWK_ASR1 Mask) PID */
#define PMC_SLPWK_ASR1_PID(value)           (PMC_SLPWK_ASR1_PID_Msk & ((value) << PMC_SLPWK_ASR1_PID_Pos))  
#define PMC_SLPWK_ASR1_MASK                 (0x1F3FFFAFU)                                  /**< \deprecated (PMC_SLPWK_ASR1) Register MASK  (Use PMC_SLPWK_ASR1_Msk instead)  */
#define PMC_SLPWK_ASR1_Msk                  (0x1F3FFFAFU)                                  /**< (PMC_SLPWK_ASR1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PMC_SLPWK_AIPR : (PMC Offset: 0x144) (R/ 32) SleepWalking Activity In Progress Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t AIP:1;                     /**< bit:      0  Activity In Progress                     */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PMC_SLPWK_AIPR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PMC_SLPWK_AIPR_OFFSET               (0x144)                                       /**<  (PMC_SLPWK_AIPR) SleepWalking Activity In Progress Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PMC_SLPWK_AIPR_AIP_Pos              0                                              /**< (PMC_SLPWK_AIPR) Activity In Progress Position */
#define PMC_SLPWK_AIPR_AIP_Msk              (0x1U << PMC_SLPWK_AIPR_AIP_Pos)               /**< (PMC_SLPWK_AIPR) Activity In Progress Mask */
#define PMC_SLPWK_AIPR_AIP                  PMC_SLPWK_AIPR_AIP_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use PMC_SLPWK_AIPR_AIP_Msk instead */
#define PMC_SLPWK_AIPR_MASK                 (0x01U)                                        /**< \deprecated (PMC_SLPWK_AIPR) Register MASK  (Use PMC_SLPWK_AIPR_Msk instead)  */
#define PMC_SLPWK_AIPR_Msk                  (0x01U)                                        /**< (PMC_SLPWK_AIPR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief PMC hardware registers */
typedef struct {  
  __O  uint32_t PMC_SCER;       /**< (PMC Offset: 0x00) System Clock Enable Register */
  __O  uint32_t PMC_SCDR;       /**< (PMC Offset: 0x04) System Clock Disable Register */
  __I  uint32_t PMC_SCSR;       /**< (PMC Offset: 0x08) System Clock Status Register */
  __I  uint32_t Reserved1[1];
  __O  uint32_t PMC_PCER0;      /**< (PMC Offset: 0x10) Peripheral Clock Enable Register 0 */
  __O  uint32_t PMC_PCDR0;      /**< (PMC Offset: 0x14) Peripheral Clock Disable Register 0 */
  __I  uint32_t PMC_PCSR0;      /**< (PMC Offset: 0x18) Peripheral Clock Status Register 0 */
  __IO uint32_t CKGR_UCKR;      /**< (PMC Offset: 0x1C) UTMI Clock Register */
  __IO uint32_t CKGR_MOR;       /**< (PMC Offset: 0x20) Main Oscillator Register */
  __IO uint32_t CKGR_MCFR;      /**< (PMC Offset: 0x24) Main Clock Frequency Register */
  __IO uint32_t CKGR_PLLAR;     /**< (PMC Offset: 0x28) PLLA Register */
  __I  uint32_t Reserved2[1];
  __IO uint32_t PMC_MCKR;       /**< (PMC Offset: 0x30) Master Clock Register */
  __I  uint32_t Reserved3[1];
  __IO uint32_t PMC_USB;        /**< (PMC Offset: 0x38) USB Clock Register */
  __I  uint32_t Reserved4[1];
  __IO uint32_t PMC_PCK[8];     /**< (PMC Offset: 0x40) Programmable Clock 0 Register 0 */
  __O  uint32_t PMC_IER;        /**< (PMC Offset: 0x60) Interrupt Enable Register */
  __O  uint32_t PMC_IDR;        /**< (PMC Offset: 0x64) Interrupt Disable Register */
  __I  uint32_t PMC_SR;         /**< (PMC Offset: 0x68) Status Register */
  __I  uint32_t PMC_IMR;        /**< (PMC Offset: 0x6C) Interrupt Mask Register */
  __IO uint32_t PMC_FSMR;       /**< (PMC Offset: 0x70) Fast Startup Mode Register */
  __IO uint32_t PMC_FSPR;       /**< (PMC Offset: 0x74) Fast Startup Polarity Register */
  __O  uint32_t PMC_FOCR;       /**< (PMC Offset: 0x78) Fault Output Clear Register */
  __I  uint32_t Reserved5[26];
  __IO uint32_t PMC_WPMR;       /**< (PMC Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t PMC_WPSR;       /**< (PMC Offset: 0xE8) Write Protection Status Register */
  __I  uint32_t Reserved6[5];
  __O  uint32_t PMC_PCER1;      /**< (PMC Offset: 0x100) Peripheral Clock Enable Register 1 */
  __O  uint32_t PMC_PCDR1;      /**< (PMC Offset: 0x104) Peripheral Clock Disable Register 1 */
  __I  uint32_t PMC_PCSR1;      /**< (PMC Offset: 0x108) Peripheral Clock Status Register 1 */
  __IO uint32_t PMC_PCR;        /**< (PMC Offset: 0x10C) Peripheral Control Register */
  __IO uint32_t PMC_OCR;        /**< (PMC Offset: 0x110) Oscillator Calibration Register */
  __O  uint32_t PMC_SLPWK_ER0;  /**< (PMC Offset: 0x114) SleepWalking Enable Register 0 */
  __O  uint32_t PMC_SLPWK_DR0;  /**< (PMC Offset: 0x118) SleepWalking Disable Register 0 */
  __I  uint32_t PMC_SLPWK_SR0;  /**< (PMC Offset: 0x11C) SleepWalking Status Register 0 */
  __I  uint32_t PMC_SLPWK_ASR0; /**< (PMC Offset: 0x120) SleepWalking Activity Status Register 0 */
  __I  uint32_t Reserved7[3];
  __IO uint32_t PMC_PMMR;       /**< (PMC Offset: 0x130) PLL Maximum Multiplier Value Register */
  __O  uint32_t PMC_SLPWK_ER1;  /**< (PMC Offset: 0x134) SleepWalking Enable Register 1 */
  __O  uint32_t PMC_SLPWK_DR1;  /**< (PMC Offset: 0x138) SleepWalking Disable Register 1 */
  __I  uint32_t PMC_SLPWK_SR1;  /**< (PMC Offset: 0x13C) SleepWalking Status Register 1 */
  __I  uint32_t PMC_SLPWK_ASR1; /**< (PMC Offset: 0x140) SleepWalking Activity Status Register 1 */
  __I  uint32_t PMC_SLPWK_AIPR; /**< (PMC Offset: 0x144) SleepWalking Activity In Progress Register */
} Pmc;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief PMC hardware registers */
typedef struct {  
  __O  PMC_SCER_Type                  PMC_SCER;       /**< Offset: 0x00 ( /W  32) System Clock Enable Register */
  __O  PMC_SCDR_Type                  PMC_SCDR;       /**< Offset: 0x04 ( /W  32) System Clock Disable Register */
  __I  PMC_SCSR_Type                  PMC_SCSR;       /**< Offset: 0x08 (R/   32) System Clock Status Register */
       RoReg8                         Reserved1[0x4];
  __O  PMC_PCER0_Type                 PMC_PCER0;      /**< Offset: 0x10 ( /W  32) Peripheral Clock Enable Register 0 */
  __O  PMC_PCDR0_Type                 PMC_PCDR0;      /**< Offset: 0x14 ( /W  32) Peripheral Clock Disable Register 0 */
  __I  PMC_PCSR0_Type                 PMC_PCSR0;      /**< Offset: 0x18 (R/   32) Peripheral Clock Status Register 0 */
  __IO CKGR_UCKR_Type                 CKGR_UCKR;      /**< Offset: 0x1C (R/W  32) UTMI Clock Register */
  __IO CKGR_MOR_Type                  CKGR_MOR;       /**< Offset: 0x20 (R/W  32) Main Oscillator Register */
  __IO CKGR_MCFR_Type                 CKGR_MCFR;      /**< Offset: 0x24 (R/W  32) Main Clock Frequency Register */
  __IO CKGR_PLLAR_Type                CKGR_PLLAR;     /**< Offset: 0x28 (R/W  32) PLLA Register */
       RoReg8                         Reserved2[0x4];
  __IO PMC_MCKR_Type                  PMC_MCKR;       /**< Offset: 0x30 (R/W  32) Master Clock Register */
       RoReg8                         Reserved3[0x4];
  __IO PMC_USB_Type                   PMC_USB;        /**< Offset: 0x38 (R/W  32) USB Clock Register */
       RoReg8                         Reserved4[0x4];
  __IO PMC_PCK_Type                   PMC_PCK[8];     /**< Offset: 0x40 (R/W  32) Programmable Clock 0 Register 0 */
  __O  PMC_IER_Type                   PMC_IER;        /**< Offset: 0x60 ( /W  32) Interrupt Enable Register */
  __O  PMC_IDR_Type                   PMC_IDR;        /**< Offset: 0x64 ( /W  32) Interrupt Disable Register */
  __I  PMC_SR_Type                    PMC_SR;         /**< Offset: 0x68 (R/   32) Status Register */
  __I  PMC_IMR_Type                   PMC_IMR;        /**< Offset: 0x6C (R/   32) Interrupt Mask Register */
  __IO PMC_FSMR_Type                  PMC_FSMR;       /**< Offset: 0x70 (R/W  32) Fast Startup Mode Register */
  __IO PMC_FSPR_Type                  PMC_FSPR;       /**< Offset: 0x74 (R/W  32) Fast Startup Polarity Register */
  __O  PMC_FOCR_Type                  PMC_FOCR;       /**< Offset: 0x78 ( /W  32) Fault Output Clear Register */
       RoReg8                         Reserved5[0x68];
  __IO PMC_WPMR_Type                  PMC_WPMR;       /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
  __I  PMC_WPSR_Type                  PMC_WPSR;       /**< Offset: 0xE8 (R/   32) Write Protection Status Register */
       RoReg8                         Reserved6[0x14];
  __O  PMC_PCER1_Type                 PMC_PCER1;      /**< Offset: 0x100 ( /W  32) Peripheral Clock Enable Register 1 */
  __O  PMC_PCDR1_Type                 PMC_PCDR1;      /**< Offset: 0x104 ( /W  32) Peripheral Clock Disable Register 1 */
  __I  PMC_PCSR1_Type                 PMC_PCSR1;      /**< Offset: 0x108 (R/   32) Peripheral Clock Status Register 1 */
  __IO PMC_PCR_Type                   PMC_PCR;        /**< Offset: 0x10C (R/W  32) Peripheral Control Register */
  __IO PMC_OCR_Type                   PMC_OCR;        /**< Offset: 0x110 (R/W  32) Oscillator Calibration Register */
  __O  PMC_SLPWK_ER0_Type             PMC_SLPWK_ER0;  /**< Offset: 0x114 ( /W  32) SleepWalking Enable Register 0 */
  __O  PMC_SLPWK_DR0_Type             PMC_SLPWK_DR0;  /**< Offset: 0x118 ( /W  32) SleepWalking Disable Register 0 */
  __I  PMC_SLPWK_SR0_Type             PMC_SLPWK_SR0;  /**< Offset: 0x11C (R/   32) SleepWalking Status Register 0 */
  __I  PMC_SLPWK_ASR0_Type            PMC_SLPWK_ASR0; /**< Offset: 0x120 (R/   32) SleepWalking Activity Status Register 0 */
       RoReg8                         Reserved7[0xC];
  __IO PMC_PMMR_Type                  PMC_PMMR;       /**< Offset: 0x130 (R/W  32) PLL Maximum Multiplier Value Register */
  __O  PMC_SLPWK_ER1_Type             PMC_SLPWK_ER1;  /**< Offset: 0x134 ( /W  32) SleepWalking Enable Register 1 */
  __O  PMC_SLPWK_DR1_Type             PMC_SLPWK_DR1;  /**< Offset: 0x138 ( /W  32) SleepWalking Disable Register 1 */
  __I  PMC_SLPWK_SR1_Type             PMC_SLPWK_SR1;  /**< Offset: 0x13C (R/   32) SleepWalking Status Register 1 */
  __I  PMC_SLPWK_ASR1_Type            PMC_SLPWK_ASR1; /**< Offset: 0x140 (R/   32) SleepWalking Activity Status Register 1 */
  __I  PMC_SLPWK_AIPR_Type            PMC_SLPWK_AIPR; /**< Offset: 0x144 (R/   32) SleepWalking Activity In Progress Register */
} Pmc;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Power Management Controller */

#endif /* _SAME70_PMC_COMPONENT_H_ */
