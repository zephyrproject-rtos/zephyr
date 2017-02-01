/**
 * \file
 *
 * \brief Component description for TRNG
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

#ifndef _SAME70_TRNG_COMPONENT_H_
#define _SAME70_TRNG_COMPONENT_H_
#define _SAME70_TRNG_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_TRNG True Random Number Generator
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR TRNG */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define TRNG_6334                       /**< (TRNG) Module ID */
#define REV_TRNG G                      /**< (TRNG) Module revision */

/* -------- TRNG_CR : (TRNG Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ENABLE:1;                  /**< bit:      0  Enables the TRNG to Provide Random Values */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t KEY:24;                    /**< bit:  8..31  Security Key                             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TRNG_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TRNG_CR_OFFSET                      (0x00)                                        /**<  (TRNG_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TRNG_CR_ENABLE_Pos                  0                                              /**< (TRNG_CR) Enables the TRNG to Provide Random Values Position */
#define TRNG_CR_ENABLE_Msk                  (0x1U << TRNG_CR_ENABLE_Pos)                   /**< (TRNG_CR) Enables the TRNG to Provide Random Values Mask */
#define TRNG_CR_ENABLE                      TRNG_CR_ENABLE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TRNG_CR_ENABLE_Msk instead */
#define TRNG_CR_KEY_Pos                     8                                              /**< (TRNG_CR) Security Key Position */
#define TRNG_CR_KEY_Msk                     (0xFFFFFFU << TRNG_CR_KEY_Pos)                 /**< (TRNG_CR) Security Key Mask */
#define TRNG_CR_KEY(value)                  (TRNG_CR_KEY_Msk & ((value) << TRNG_CR_KEY_Pos))
#define   TRNG_CR_KEY_PASSWD_Val            (0x524E47U)                                    /**< (TRNG_CR) Writing any other value in this field aborts the write operation.  */
#define TRNG_CR_KEY_PASSWD                  (TRNG_CR_KEY_PASSWD_Val << TRNG_CR_KEY_Pos)    /**< (TRNG_CR) Writing any other value in this field aborts the write operation. Position  */
#define TRNG_CR_MASK                        (0xFFFFFF01U)                                  /**< \deprecated (TRNG_CR) Register MASK  (Use TRNG_CR_Msk instead)  */
#define TRNG_CR_Msk                         (0xFFFFFF01U)                                  /**< (TRNG_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TRNG_IER : (TRNG Offset: 0x10) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATRDY:1;                  /**< bit:      0  Data Ready Interrupt Enable              */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TRNG_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TRNG_IER_OFFSET                     (0x10)                                        /**<  (TRNG_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TRNG_IER_DATRDY_Pos                 0                                              /**< (TRNG_IER) Data Ready Interrupt Enable Position */
#define TRNG_IER_DATRDY_Msk                 (0x1U << TRNG_IER_DATRDY_Pos)                  /**< (TRNG_IER) Data Ready Interrupt Enable Mask */
#define TRNG_IER_DATRDY                     TRNG_IER_DATRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TRNG_IER_DATRDY_Msk instead */
#define TRNG_IER_MASK                       (0x01U)                                        /**< \deprecated (TRNG_IER) Register MASK  (Use TRNG_IER_Msk instead)  */
#define TRNG_IER_Msk                        (0x01U)                                        /**< (TRNG_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TRNG_IDR : (TRNG Offset: 0x14) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATRDY:1;                  /**< bit:      0  Data Ready Interrupt Disable             */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TRNG_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TRNG_IDR_OFFSET                     (0x14)                                        /**<  (TRNG_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TRNG_IDR_DATRDY_Pos                 0                                              /**< (TRNG_IDR) Data Ready Interrupt Disable Position */
#define TRNG_IDR_DATRDY_Msk                 (0x1U << TRNG_IDR_DATRDY_Pos)                  /**< (TRNG_IDR) Data Ready Interrupt Disable Mask */
#define TRNG_IDR_DATRDY                     TRNG_IDR_DATRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TRNG_IDR_DATRDY_Msk instead */
#define TRNG_IDR_MASK                       (0x01U)                                        /**< \deprecated (TRNG_IDR) Register MASK  (Use TRNG_IDR_Msk instead)  */
#define TRNG_IDR_Msk                        (0x01U)                                        /**< (TRNG_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TRNG_IMR : (TRNG Offset: 0x18) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATRDY:1;                  /**< bit:      0  Data Ready Interrupt Mask                */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TRNG_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TRNG_IMR_OFFSET                     (0x18)                                        /**<  (TRNG_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TRNG_IMR_DATRDY_Pos                 0                                              /**< (TRNG_IMR) Data Ready Interrupt Mask Position */
#define TRNG_IMR_DATRDY_Msk                 (0x1U << TRNG_IMR_DATRDY_Pos)                  /**< (TRNG_IMR) Data Ready Interrupt Mask Mask */
#define TRNG_IMR_DATRDY                     TRNG_IMR_DATRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TRNG_IMR_DATRDY_Msk instead */
#define TRNG_IMR_MASK                       (0x01U)                                        /**< \deprecated (TRNG_IMR) Register MASK  (Use TRNG_IMR_Msk instead)  */
#define TRNG_IMR_Msk                        (0x01U)                                        /**< (TRNG_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TRNG_ISR : (TRNG Offset: 0x1c) (R/ 32) Interrupt Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATRDY:1;                  /**< bit:      0  Data Ready                               */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TRNG_ISR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TRNG_ISR_OFFSET                     (0x1C)                                        /**<  (TRNG_ISR) Interrupt Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TRNG_ISR_DATRDY_Pos                 0                                              /**< (TRNG_ISR) Data Ready Position */
#define TRNG_ISR_DATRDY_Msk                 (0x1U << TRNG_ISR_DATRDY_Pos)                  /**< (TRNG_ISR) Data Ready Mask */
#define TRNG_ISR_DATRDY                     TRNG_ISR_DATRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TRNG_ISR_DATRDY_Msk instead */
#define TRNG_ISR_MASK                       (0x01U)                                        /**< \deprecated (TRNG_ISR) Register MASK  (Use TRNG_ISR_Msk instead)  */
#define TRNG_ISR_Msk                        (0x01U)                                        /**< (TRNG_ISR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TRNG_ODATA : (TRNG Offset: 0x50) (R/ 32) Output Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ODATA:32;                  /**< bit:  0..31  Output Data                              */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TRNG_ODATA_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TRNG_ODATA_OFFSET                   (0x50)                                        /**<  (TRNG_ODATA) Output Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TRNG_ODATA_ODATA_Pos                0                                              /**< (TRNG_ODATA) Output Data Position */
#define TRNG_ODATA_ODATA_Msk                (0xFFFFFFFFU << TRNG_ODATA_ODATA_Pos)          /**< (TRNG_ODATA) Output Data Mask */
#define TRNG_ODATA_ODATA(value)             (TRNG_ODATA_ODATA_Msk & ((value) << TRNG_ODATA_ODATA_Pos))
#define TRNG_ODATA_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (TRNG_ODATA) Register MASK  (Use TRNG_ODATA_Msk instead)  */
#define TRNG_ODATA_Msk                      (0xFFFFFFFFU)                                  /**< (TRNG_ODATA) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief TRNG hardware registers */
typedef struct {  
  __O  uint32_t TRNG_CR;        /**< (TRNG Offset: 0x00) Control Register */
  __I  uint32_t Reserved1[3];
  __O  uint32_t TRNG_IER;       /**< (TRNG Offset: 0x10) Interrupt Enable Register */
  __O  uint32_t TRNG_IDR;       /**< (TRNG Offset: 0x14) Interrupt Disable Register */
  __I  uint32_t TRNG_IMR;       /**< (TRNG Offset: 0x18) Interrupt Mask Register */
  __I  uint32_t TRNG_ISR;       /**< (TRNG Offset: 0x1C) Interrupt Status Register */
  __I  uint32_t Reserved2[12];
  __I  uint32_t TRNG_ODATA;     /**< (TRNG Offset: 0x50) Output Data Register */
} Trng;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief TRNG hardware registers */
typedef struct {  
  __O  TRNG_CR_Type                   TRNG_CR;        /**< Offset: 0x00 ( /W  32) Control Register */
       RoReg8                         Reserved1[0xC];
  __O  TRNG_IER_Type                  TRNG_IER;       /**< Offset: 0x10 ( /W  32) Interrupt Enable Register */
  __O  TRNG_IDR_Type                  TRNG_IDR;       /**< Offset: 0x14 ( /W  32) Interrupt Disable Register */
  __I  TRNG_IMR_Type                  TRNG_IMR;       /**< Offset: 0x18 (R/   32) Interrupt Mask Register */
  __I  TRNG_ISR_Type                  TRNG_ISR;       /**< Offset: 0x1C (R/   32) Interrupt Status Register */
       RoReg8                         Reserved2[0x30];
  __I  TRNG_ODATA_Type                TRNG_ODATA;     /**< Offset: 0x50 (R/   32) Output Data Register */
} Trng;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of True Random Number Generator */

#endif /* _SAME70_TRNG_COMPONENT_H_ */
