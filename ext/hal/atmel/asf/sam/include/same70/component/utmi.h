/**
 * \file
 *
 * \brief Component description for UTMI
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

#ifndef _SAME70_UTMI_COMPONENT_H_
#define _SAME70_UTMI_COMPONENT_H_
#define _SAME70_UTMI_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_UTMI USB Transmitter Interface Macrocell
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR UTMI */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define UTMI_11300                      /**< (UTMI) Module ID */
#define REV_UTMI A                      /**< (UTMI) Module revision */

/* -------- UTMI_OHCIICR : (UTMI Offset: 0x10) (R/W 32) OHCI Interrupt Configuration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RES0:1;                    /**< bit:      0  USB PORTx Reset                          */
    uint32_t :3;                        /**< bit:   1..3  Reserved */
    uint32_t ARIE:1;                    /**< bit:      4  OHCI Asynchronous Resume Interrupt Enable */
    uint32_t APPSTART:1;                /**< bit:      5  Reserved                                 */
    uint32_t :17;                       /**< bit:  6..22  Reserved */
    uint32_t UDPPUDIS:1;                /**< bit:     23  USB Device Pull-up Disable               */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UTMI_OHCIICR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UTMI_OHCIICR_OFFSET                 (0x10)                                        /**<  (UTMI_OHCIICR) OHCI Interrupt Configuration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UTMI_OHCIICR_RES0_Pos               0                                              /**< (UTMI_OHCIICR) USB PORTx Reset Position */
#define UTMI_OHCIICR_RES0_Msk               (0x1U << UTMI_OHCIICR_RES0_Pos)                /**< (UTMI_OHCIICR) USB PORTx Reset Mask */
#define UTMI_OHCIICR_RES0                   UTMI_OHCIICR_RES0_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use UTMI_OHCIICR_RES0_Msk instead */
#define UTMI_OHCIICR_ARIE_Pos               4                                              /**< (UTMI_OHCIICR) OHCI Asynchronous Resume Interrupt Enable Position */
#define UTMI_OHCIICR_ARIE_Msk               (0x1U << UTMI_OHCIICR_ARIE_Pos)                /**< (UTMI_OHCIICR) OHCI Asynchronous Resume Interrupt Enable Mask */
#define UTMI_OHCIICR_ARIE                   UTMI_OHCIICR_ARIE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use UTMI_OHCIICR_ARIE_Msk instead */
#define UTMI_OHCIICR_APPSTART_Pos           5                                              /**< (UTMI_OHCIICR) Reserved Position */
#define UTMI_OHCIICR_APPSTART_Msk           (0x1U << UTMI_OHCIICR_APPSTART_Pos)            /**< (UTMI_OHCIICR) Reserved Mask */
#define UTMI_OHCIICR_APPSTART               UTMI_OHCIICR_APPSTART_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use UTMI_OHCIICR_APPSTART_Msk instead */
#define UTMI_OHCIICR_UDPPUDIS_Pos           23                                             /**< (UTMI_OHCIICR) USB Device Pull-up Disable Position */
#define UTMI_OHCIICR_UDPPUDIS_Msk           (0x1U << UTMI_OHCIICR_UDPPUDIS_Pos)            /**< (UTMI_OHCIICR) USB Device Pull-up Disable Mask */
#define UTMI_OHCIICR_UDPPUDIS               UTMI_OHCIICR_UDPPUDIS_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use UTMI_OHCIICR_UDPPUDIS_Msk instead */
#define UTMI_OHCIICR_MASK                   (0x800031U)                                    /**< \deprecated (UTMI_OHCIICR) Register MASK  (Use UTMI_OHCIICR_Msk instead)  */
#define UTMI_OHCIICR_Msk                    (0x800031U)                                    /**< (UTMI_OHCIICR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- UTMI_CKTRIM : (UTMI Offset: 0x30) (R/W 32) UTMI Clock Trimming Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FREQ:2;                    /**< bit:   0..1  UTMI Reference Clock Frequency           */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} UTMI_CKTRIM_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define UTMI_CKTRIM_OFFSET                  (0x30)                                        /**<  (UTMI_CKTRIM) UTMI Clock Trimming Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define UTMI_CKTRIM_FREQ_Pos                0                                              /**< (UTMI_CKTRIM) UTMI Reference Clock Frequency Position */
#define UTMI_CKTRIM_FREQ_Msk                (0x3U << UTMI_CKTRIM_FREQ_Pos)                 /**< (UTMI_CKTRIM) UTMI Reference Clock Frequency Mask */
#define UTMI_CKTRIM_FREQ(value)             (UTMI_CKTRIM_FREQ_Msk & ((value) << UTMI_CKTRIM_FREQ_Pos))
#define   UTMI_CKTRIM_FREQ_XTAL12_Val       (0x0U)                                         /**< (UTMI_CKTRIM) 12 MHz reference clock  */
#define   UTMI_CKTRIM_FREQ_XTAL16_Val       (0x1U)                                         /**< (UTMI_CKTRIM) 16 MHz reference clock  */
#define UTMI_CKTRIM_FREQ_XTAL12             (UTMI_CKTRIM_FREQ_XTAL12_Val << UTMI_CKTRIM_FREQ_Pos)  /**< (UTMI_CKTRIM) 12 MHz reference clock Position  */
#define UTMI_CKTRIM_FREQ_XTAL16             (UTMI_CKTRIM_FREQ_XTAL16_Val << UTMI_CKTRIM_FREQ_Pos)  /**< (UTMI_CKTRIM) 16 MHz reference clock Position  */
#define UTMI_CKTRIM_MASK                    (0x03U)                                        /**< \deprecated (UTMI_CKTRIM) Register MASK  (Use UTMI_CKTRIM_Msk instead)  */
#define UTMI_CKTRIM_Msk                     (0x03U)                                        /**< (UTMI_CKTRIM) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief UTMI hardware registers */
typedef struct {  
  __I  uint32_t Reserved1[4];
  __IO uint32_t UTMI_OHCIICR;   /**< (UTMI Offset: 0x10) OHCI Interrupt Configuration Register */
  __I  uint32_t Reserved2[7];
  __IO uint32_t UTMI_CKTRIM;    /**< (UTMI Offset: 0x30) UTMI Clock Trimming Register */
} Utmi;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief UTMI hardware registers */
typedef struct {  
       RoReg8                         Reserved1[0x10];
  __IO UTMI_OHCIICR_Type              UTMI_OHCIICR;   /**< Offset: 0x10 (R/W  32) OHCI Interrupt Configuration Register */
       RoReg8                         Reserved2[0x1C];
  __IO UTMI_CKTRIM_Type               UTMI_CKTRIM;    /**< Offset: 0x30 (R/W  32) UTMI Clock Trimming Register */
} Utmi;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of USB Transmitter Interface Macrocell */

#endif /* _SAME70_UTMI_COMPONENT_H_ */
