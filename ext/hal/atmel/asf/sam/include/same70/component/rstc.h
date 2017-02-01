/**
 * \file
 *
 * \brief Component description for RSTC
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

#ifndef _SAME70_RSTC_COMPONENT_H_
#define _SAME70_RSTC_COMPONENT_H_
#define _SAME70_RSTC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_RSTC Reset Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR RSTC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define RSTC_11009                      /**< (RSTC) Module ID */
#define REV_RSTC J                      /**< (RSTC) Module revision */

/* -------- RSTC_CR : (RSTC Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PROCRST:1;                 /**< bit:      0  Processor Reset                          */
    uint32_t :2;                        /**< bit:   1..2  Reserved */
    uint32_t EXTRST:1;                  /**< bit:      3  External Reset                           */
    uint32_t :20;                       /**< bit:  4..23  Reserved */
    uint32_t KEY:8;                     /**< bit: 24..31  System Reset Key                         */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RSTC_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RSTC_CR_OFFSET                      (0x00)                                        /**<  (RSTC_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RSTC_CR_PROCRST_Pos                 0                                              /**< (RSTC_CR) Processor Reset Position */
#define RSTC_CR_PROCRST_Msk                 (0x1U << RSTC_CR_PROCRST_Pos)                  /**< (RSTC_CR) Processor Reset Mask */
#define RSTC_CR_PROCRST                     RSTC_CR_PROCRST_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSTC_CR_PROCRST_Msk instead */
#define RSTC_CR_EXTRST_Pos                  3                                              /**< (RSTC_CR) External Reset Position */
#define RSTC_CR_EXTRST_Msk                  (0x1U << RSTC_CR_EXTRST_Pos)                   /**< (RSTC_CR) External Reset Mask */
#define RSTC_CR_EXTRST                      RSTC_CR_EXTRST_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSTC_CR_EXTRST_Msk instead */
#define RSTC_CR_KEY_Pos                     24                                             /**< (RSTC_CR) System Reset Key Position */
#define RSTC_CR_KEY_Msk                     (0xFFU << RSTC_CR_KEY_Pos)                     /**< (RSTC_CR) System Reset Key Mask */
#define RSTC_CR_KEY(value)                  (RSTC_CR_KEY_Msk & ((value) << RSTC_CR_KEY_Pos))
#define   RSTC_CR_KEY_PASSWD_Val            (0xA5U)                                        /**< (RSTC_CR) Writing any other value in this field aborts the write operation.  */
#define RSTC_CR_KEY_PASSWD                  (RSTC_CR_KEY_PASSWD_Val << RSTC_CR_KEY_Pos)    /**< (RSTC_CR) Writing any other value in this field aborts the write operation. Position  */
#define RSTC_CR_MASK                        (0xFF000009U)                                  /**< \deprecated (RSTC_CR) Register MASK  (Use RSTC_CR_Msk instead)  */
#define RSTC_CR_Msk                         (0xFF000009U)                                  /**< (RSTC_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RSTC_SR : (RSTC Offset: 0x04) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t URSTS:1;                   /**< bit:      0  User Reset Status                        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t RSTTYP:3;                  /**< bit:  8..10  Reset Type                               */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t NRSTL:1;                   /**< bit:     16  NRST Pin Level                           */
    uint32_t SRCMP:1;                   /**< bit:     17  Software Reset Command in Progress       */
    uint32_t :14;                       /**< bit: 18..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RSTC_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RSTC_SR_OFFSET                      (0x04)                                        /**<  (RSTC_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RSTC_SR_URSTS_Pos                   0                                              /**< (RSTC_SR) User Reset Status Position */
#define RSTC_SR_URSTS_Msk                   (0x1U << RSTC_SR_URSTS_Pos)                    /**< (RSTC_SR) User Reset Status Mask */
#define RSTC_SR_URSTS                       RSTC_SR_URSTS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSTC_SR_URSTS_Msk instead */
#define RSTC_SR_RSTTYP_Pos                  8                                              /**< (RSTC_SR) Reset Type Position */
#define RSTC_SR_RSTTYP_Msk                  (0x7U << RSTC_SR_RSTTYP_Pos)                   /**< (RSTC_SR) Reset Type Mask */
#define RSTC_SR_RSTTYP(value)               (RSTC_SR_RSTTYP_Msk & ((value) << RSTC_SR_RSTTYP_Pos))
#define   RSTC_SR_RSTTYP_GENERAL_RST_Val    (0x0U)                                         /**< (RSTC_SR) First power-up reset  */
#define   RSTC_SR_RSTTYP_BACKUP_RST_Val     (0x1U)                                         /**< (RSTC_SR) Return from Backup Mode  */
#define   RSTC_SR_RSTTYP_WDT_RST_Val        (0x2U)                                         /**< (RSTC_SR) Watchdog fault occurred  */
#define   RSTC_SR_RSTTYP_SOFT_RST_Val       (0x3U)                                         /**< (RSTC_SR) Processor reset required by the software  */
#define   RSTC_SR_RSTTYP_USER_RST_Val       (0x4U)                                         /**< (RSTC_SR) NRST pin detected low  */
#define RSTC_SR_RSTTYP_GENERAL_RST          (RSTC_SR_RSTTYP_GENERAL_RST_Val << RSTC_SR_RSTTYP_Pos)  /**< (RSTC_SR) First power-up reset Position  */
#define RSTC_SR_RSTTYP_BACKUP_RST           (RSTC_SR_RSTTYP_BACKUP_RST_Val << RSTC_SR_RSTTYP_Pos)  /**< (RSTC_SR) Return from Backup Mode Position  */
#define RSTC_SR_RSTTYP_WDT_RST              (RSTC_SR_RSTTYP_WDT_RST_Val << RSTC_SR_RSTTYP_Pos)  /**< (RSTC_SR) Watchdog fault occurred Position  */
#define RSTC_SR_RSTTYP_SOFT_RST             (RSTC_SR_RSTTYP_SOFT_RST_Val << RSTC_SR_RSTTYP_Pos)  /**< (RSTC_SR) Processor reset required by the software Position  */
#define RSTC_SR_RSTTYP_USER_RST             (RSTC_SR_RSTTYP_USER_RST_Val << RSTC_SR_RSTTYP_Pos)  /**< (RSTC_SR) NRST pin detected low Position  */
#define RSTC_SR_NRSTL_Pos                   16                                             /**< (RSTC_SR) NRST Pin Level Position */
#define RSTC_SR_NRSTL_Msk                   (0x1U << RSTC_SR_NRSTL_Pos)                    /**< (RSTC_SR) NRST Pin Level Mask */
#define RSTC_SR_NRSTL                       RSTC_SR_NRSTL_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSTC_SR_NRSTL_Msk instead */
#define RSTC_SR_SRCMP_Pos                   17                                             /**< (RSTC_SR) Software Reset Command in Progress Position */
#define RSTC_SR_SRCMP_Msk                   (0x1U << RSTC_SR_SRCMP_Pos)                    /**< (RSTC_SR) Software Reset Command in Progress Mask */
#define RSTC_SR_SRCMP                       RSTC_SR_SRCMP_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSTC_SR_SRCMP_Msk instead */
#define RSTC_SR_MASK                        (0x30701U)                                     /**< \deprecated (RSTC_SR) Register MASK  (Use RSTC_SR_Msk instead)  */
#define RSTC_SR_Msk                         (0x30701U)                                     /**< (RSTC_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- RSTC_MR : (RSTC Offset: 0x08) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t URSTEN:1;                  /**< bit:      0  User Reset Enable                        */
    uint32_t :3;                        /**< bit:   1..3  Reserved */
    uint32_t URSTIEN:1;                 /**< bit:      4  User Reset Interrupt Enable              */
    uint32_t :3;                        /**< bit:   5..7  Reserved */
    uint32_t ERSTL:4;                   /**< bit:  8..11  External Reset Length                    */
    uint32_t :12;                       /**< bit: 12..23  Reserved */
    uint32_t KEY:8;                     /**< bit: 24..31  Write Access Password                    */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} RSTC_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RSTC_MR_OFFSET                      (0x08)                                        /**<  (RSTC_MR) Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define RSTC_MR_URSTEN_Pos                  0                                              /**< (RSTC_MR) User Reset Enable Position */
#define RSTC_MR_URSTEN_Msk                  (0x1U << RSTC_MR_URSTEN_Pos)                   /**< (RSTC_MR) User Reset Enable Mask */
#define RSTC_MR_URSTEN                      RSTC_MR_URSTEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSTC_MR_URSTEN_Msk instead */
#define RSTC_MR_URSTIEN_Pos                 4                                              /**< (RSTC_MR) User Reset Interrupt Enable Position */
#define RSTC_MR_URSTIEN_Msk                 (0x1U << RSTC_MR_URSTIEN_Pos)                  /**< (RSTC_MR) User Reset Interrupt Enable Mask */
#define RSTC_MR_URSTIEN                     RSTC_MR_URSTIEN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use RSTC_MR_URSTIEN_Msk instead */
#define RSTC_MR_ERSTL_Pos                   8                                              /**< (RSTC_MR) External Reset Length Position */
#define RSTC_MR_ERSTL_Msk                   (0xFU << RSTC_MR_ERSTL_Pos)                    /**< (RSTC_MR) External Reset Length Mask */
#define RSTC_MR_ERSTL(value)                (RSTC_MR_ERSTL_Msk & ((value) << RSTC_MR_ERSTL_Pos))
#define RSTC_MR_KEY_Pos                     24                                             /**< (RSTC_MR) Write Access Password Position */
#define RSTC_MR_KEY_Msk                     (0xFFU << RSTC_MR_KEY_Pos)                     /**< (RSTC_MR) Write Access Password Mask */
#define RSTC_MR_KEY(value)                  (RSTC_MR_KEY_Msk & ((value) << RSTC_MR_KEY_Pos))
#define   RSTC_MR_KEY_PASSWD_Val            (0xA5U)                                        /**< (RSTC_MR) Writing any other value in this field aborts the write operation.Always reads as 0.  */
#define RSTC_MR_KEY_PASSWD                  (RSTC_MR_KEY_PASSWD_Val << RSTC_MR_KEY_Pos)    /**< (RSTC_MR) Writing any other value in this field aborts the write operation.Always reads as 0. Position  */
#define RSTC_MR_MASK                        (0xFF000F11U)                                  /**< \deprecated (RSTC_MR) Register MASK  (Use RSTC_MR_Msk instead)  */
#define RSTC_MR_Msk                         (0xFF000F11U)                                  /**< (RSTC_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief RSTC hardware registers */
typedef struct {  
  __O  uint32_t RSTC_CR;        /**< (RSTC Offset: 0x00) Control Register */
  __I  uint32_t RSTC_SR;        /**< (RSTC Offset: 0x04) Status Register */
  __IO uint32_t RSTC_MR;        /**< (RSTC Offset: 0x08) Mode Register */
} Rstc;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief RSTC hardware registers */
typedef struct {  
  __O  RSTC_CR_Type                   RSTC_CR;        /**< Offset: 0x00 ( /W  32) Control Register */
  __I  RSTC_SR_Type                   RSTC_SR;        /**< Offset: 0x04 (R/   32) Status Register */
  __IO RSTC_MR_Type                   RSTC_MR;        /**< Offset: 0x08 (R/W  32) Mode Register */
} Rstc;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Reset Controller */

#endif /* _SAME70_RSTC_COMPONENT_H_ */
