/**
 * \file
 *
 * \brief Component description for RSWDT
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

#ifndef _SAME70_RSWDT_COMPONENT_H_
#define _SAME70_RSWDT_COMPONENT_H_
#define _SAME70_RSWDT_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_RSWDT Reinforced Safety Watchdog Timer
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR RSWDT */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define RSWDT_11110                      /**< (RSWDT) Module ID */
#define REV_RSWDT D                      /**< (RSWDT) Module revision */

/* -------- RSWDT_CR : (RSWDT Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WDRSTT:1;                  /**< bit:      0  Watchdog Restart                         */
    uint32_t :23;                       /**< bit:  1..23  Reserved */
    uint32_t KEY:8;                     /**< bit: 24..31  Password                                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RSWDT_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RSWDT_CR_OFFSET                     (0x00)                                        /**<  (RSWDT_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RSWDT_CR_WDRSTT_Pos                 0                                              /**< (RSWDT_CR) Watchdog Restart Position */
#define RSWDT_CR_WDRSTT_Msk                 (0x1U << RSWDT_CR_WDRSTT_Pos)                  /**< (RSWDT_CR) Watchdog Restart Mask */
#define RSWDT_CR_WDRSTT                     RSWDT_CR_WDRSTT_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSWDT_CR_WDRSTT_Msk instead */
#define RSWDT_CR_KEY_Pos                    24                                             /**< (RSWDT_CR) Password Position */
#define RSWDT_CR_KEY_Msk                    (0xFFU << RSWDT_CR_KEY_Pos)                    /**< (RSWDT_CR) Password Mask */
#define RSWDT_CR_KEY(value)                 (RSWDT_CR_KEY_Msk & ((value) << RSWDT_CR_KEY_Pos))
#define   RSWDT_CR_KEY_PASSWD_Val           (0xC4U)                                        /**< (RSWDT_CR) Writing any other value in this field aborts the write operation.  */
#define RSWDT_CR_KEY_PASSWD                 (RSWDT_CR_KEY_PASSWD_Val << RSWDT_CR_KEY_Pos)  /**< (RSWDT_CR) Writing any other value in this field aborts the write operation. Position  */
#define RSWDT_CR_MASK                       (0xFF000001U)                                  /**< \deprecated (RSWDT_CR) Register MASK  (Use RSWDT_CR_Msk instead)  */
#define RSWDT_CR_Msk                        (0xFF000001U)                                  /**< (RSWDT_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RSWDT_MR : (RSWDT Offset: 0x04) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WDV:12;                    /**< bit:  0..11  Watchdog Counter Value                   */
    uint32_t WDFIEN:1;                  /**< bit:     12  Watchdog Fault Interrupt Enable          */
    uint32_t WDRSTEN:1;                 /**< bit:     13  Watchdog Reset Enable                    */
    uint32_t :1;                        /**< bit:     14  Reserved */
    uint32_t WDDIS:1;                   /**< bit:     15  Watchdog Disable                         */
    uint32_t ALLONES:12;                /**< bit: 16..27  Must Always Be Written with 0xFFF        */
    uint32_t WDDBGHLT:1;                /**< bit:     28  Watchdog Debug Halt                      */
    uint32_t WDIDLEHLT:1;               /**< bit:     29  Watchdog Idle Halt                       */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RSWDT_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RSWDT_MR_OFFSET                     (0x04)                                        /**<  (RSWDT_MR) Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RSWDT_MR_WDV_Pos                    0                                              /**< (RSWDT_MR) Watchdog Counter Value Position */
#define RSWDT_MR_WDV_Msk                    (0xFFFU << RSWDT_MR_WDV_Pos)                   /**< (RSWDT_MR) Watchdog Counter Value Mask */
#define RSWDT_MR_WDV(value)                 (RSWDT_MR_WDV_Msk & ((value) << RSWDT_MR_WDV_Pos))
#define RSWDT_MR_WDFIEN_Pos                 12                                             /**< (RSWDT_MR) Watchdog Fault Interrupt Enable Position */
#define RSWDT_MR_WDFIEN_Msk                 (0x1U << RSWDT_MR_WDFIEN_Pos)                  /**< (RSWDT_MR) Watchdog Fault Interrupt Enable Mask */
#define RSWDT_MR_WDFIEN                     RSWDT_MR_WDFIEN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSWDT_MR_WDFIEN_Msk instead */
#define RSWDT_MR_WDRSTEN_Pos                13                                             /**< (RSWDT_MR) Watchdog Reset Enable Position */
#define RSWDT_MR_WDRSTEN_Msk                (0x1U << RSWDT_MR_WDRSTEN_Pos)                 /**< (RSWDT_MR) Watchdog Reset Enable Mask */
#define RSWDT_MR_WDRSTEN                    RSWDT_MR_WDRSTEN_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSWDT_MR_WDRSTEN_Msk instead */
#define RSWDT_MR_WDDIS_Pos                  15                                             /**< (RSWDT_MR) Watchdog Disable Position */
#define RSWDT_MR_WDDIS_Msk                  (0x1U << RSWDT_MR_WDDIS_Pos)                   /**< (RSWDT_MR) Watchdog Disable Mask */
#define RSWDT_MR_WDDIS                      RSWDT_MR_WDDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSWDT_MR_WDDIS_Msk instead */
#define RSWDT_MR_ALLONES_Pos                16                                             /**< (RSWDT_MR) Must Always Be Written with 0xFFF Position */
#define RSWDT_MR_ALLONES_Msk                (0xFFFU << RSWDT_MR_ALLONES_Pos)               /**< (RSWDT_MR) Must Always Be Written with 0xFFF Mask */
#define RSWDT_MR_ALLONES(value)             (RSWDT_MR_ALLONES_Msk & ((value) << RSWDT_MR_ALLONES_Pos))
#define RSWDT_MR_WDDBGHLT_Pos               28                                             /**< (RSWDT_MR) Watchdog Debug Halt Position */
#define RSWDT_MR_WDDBGHLT_Msk               (0x1U << RSWDT_MR_WDDBGHLT_Pos)                /**< (RSWDT_MR) Watchdog Debug Halt Mask */
#define RSWDT_MR_WDDBGHLT                   RSWDT_MR_WDDBGHLT_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSWDT_MR_WDDBGHLT_Msk instead */
#define RSWDT_MR_WDIDLEHLT_Pos              29                                             /**< (RSWDT_MR) Watchdog Idle Halt Position */
#define RSWDT_MR_WDIDLEHLT_Msk              (0x1U << RSWDT_MR_WDIDLEHLT_Pos)               /**< (RSWDT_MR) Watchdog Idle Halt Mask */
#define RSWDT_MR_WDIDLEHLT                  RSWDT_MR_WDIDLEHLT_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSWDT_MR_WDIDLEHLT_Msk instead */
#define RSWDT_MR_MASK                       (0x3FFFBFFFU)                                  /**< \deprecated (RSWDT_MR) Register MASK  (Use RSWDT_MR_Msk instead)  */
#define RSWDT_MR_Msk                        (0x3FFFBFFFU)                                  /**< (RSWDT_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RSWDT_SR : (RSWDT Offset: 0x08) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WDUNF:1;                   /**< bit:      0  Watchdog Underflow                       */
    uint32_t :31;                       /**< bit:  1..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RSWDT_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RSWDT_SR_OFFSET                     (0x08)                                        /**<  (RSWDT_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RSWDT_SR_WDUNF_Pos                  0                                              /**< (RSWDT_SR) Watchdog Underflow Position */
#define RSWDT_SR_WDUNF_Msk                  (0x1U << RSWDT_SR_WDUNF_Pos)                   /**< (RSWDT_SR) Watchdog Underflow Mask */
#define RSWDT_SR_WDUNF                      RSWDT_SR_WDUNF_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSWDT_SR_WDUNF_Msk instead */
#define RSWDT_SR_MASK                       (0x01U)                                        /**< \deprecated (RSWDT_SR) Register MASK  (Use RSWDT_SR_Msk instead)  */
#define RSWDT_SR_Msk                        (0x01U)                                        /**< (RSWDT_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief RSWDT hardware registers */
typedef struct {  
  __O  uint32_t RSWDT_CR;       /**< (RSWDT Offset: 0x00) Control Register */
  __IO uint32_t RSWDT_MR;       /**< (RSWDT Offset: 0x04) Mode Register */
  __I  uint32_t RSWDT_SR;       /**< (RSWDT Offset: 0x08) Status Register */
} Rswdt;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief RSWDT hardware registers */
typedef struct {  
  __O  RSWDT_CR_Type                  RSWDT_CR;       /**< Offset: 0x00 ( /W  32) Control Register */
  __IO RSWDT_MR_Type                  RSWDT_MR;       /**< Offset: 0x04 (R/W  32) Mode Register */
  __I  RSWDT_SR_Type                  RSWDT_SR;       /**< Offset: 0x08 (R/   32) Status Register */
} Rswdt;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Reinforced Safety Watchdog Timer */

#endif /* _SAME70_RSWDT_COMPONENT_H_ */
