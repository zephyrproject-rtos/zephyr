/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_GCLK_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_GCLK_COMPONENT_FIXUP_H_

/* -------- GCLK_CTRLA : (GCLK Offset: 0x00) (R/W 8) Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} GCLK_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GCLK_SYNCBUSY : (GCLK Offset: 0x04) ( R/ 32) Synchronization Busy -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset Synchroniation Busy bit */
    uint32_t :1;               /*!< bit:      1  Reserved                           */
    uint32_t GENCTRL0:1;       /*!< bit:      2  Generic Clock Generator Control 0 Synchronization Busy bits */
    uint32_t GENCTRL1:1;       /*!< bit:      3  Generic Clock Generator Control 1 Synchronization Busy bits */
    uint32_t GENCTRL2:1;       /*!< bit:      4  Generic Clock Generator Control 2 Synchronization Busy bits */
    uint32_t GENCTRL3:1;       /*!< bit:      5  Generic Clock Generator Control 3 Synchronization Busy bits */
    uint32_t GENCTRL4:1;       /*!< bit:      6  Generic Clock Generator Control 4 Synchronization Busy bits */
    uint32_t GENCTRL5:1;       /*!< bit:      7  Generic Clock Generator Control 5 Synchronization Busy bits */
    uint32_t GENCTRL6:1;       /*!< bit:      8  Generic Clock Generator Control 6 Synchronization Busy bits */
    uint32_t GENCTRL7:1;       /*!< bit:      9  Generic Clock Generator Control 7 Synchronization Busy bits */
    uint32_t GENCTRL8:1;       /*!< bit:     10  Generic Clock Generator Control 8 Synchronization Busy bits */
    uint32_t GENCTRL9:1;       /*!< bit:     11  Generic Clock Generator Control 9 Synchronization Busy bits */
    uint32_t GENCTRL10:1;      /*!< bit:     12  Generic Clock Generator Control 10 Synchronization Busy bits */
    uint32_t GENCTRL11:1;      /*!< bit:     13  Generic Clock Generator Control 11 Synchronization Busy bits */
    uint32_t :18;              /*!< bit: 14..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint32_t GENCTRL:12;       /*!< bit:  2..13  Generic Clock Generator Control x Synchronization Busy bits */
    uint32_t :18;              /*!< bit: 14..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GCLK_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GCLK_SYNCBUSY_OFFSET        0x04         /**< \brief (GCLK_SYNCBUSY offset) Synchronization Busy */

/* -------- GCLK_GENCTRL : (GCLK Offset: 0x20) (R/W 32) Generic Clock Generator Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SRC:4;            /*!< bit:  0.. 3  Source Select                      */
    uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint32_t GENEN:1;          /*!< bit:      8  Generic Clock Generator Enable     */
    uint32_t IDC:1;            /*!< bit:      9  Improve Duty Cycle                 */
    uint32_t OOV:1;            /*!< bit:     10  Output Off Value                   */
    uint32_t OE:1;             /*!< bit:     11  Output Enable                      */
    uint32_t DIVSEL:1;         /*!< bit:     12  Divide Selection                   */
    uint32_t RUNSTDBY:1;       /*!< bit:     13  Run in Standby                     */
    uint32_t :2;               /*!< bit: 14..15  Reserved                           */
    uint32_t DIV:16;           /*!< bit: 16..31  Division Factor                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GCLK_GENCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GCLK_PCHCTRL : (GCLK Offset: 0x80) (R/W 32) Peripheral Clock Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t GEN:4;            /*!< bit:  0.. 3  Generic Clock Generator            */
    uint32_t :2;               /*!< bit:  4.. 5  Reserved                           */
    uint32_t CHEN:1;           /*!< bit:      6  Channel Enable                     */
    uint32_t WRTLOCK:1;        /*!< bit:      7  Write Lock                         */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GCLK_PCHCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GCLK_PCHCTRL_CHEN_BIT_MASK (_UINT32_(0x1) << GCLK_PCHCTRL_CHEN_Pos) 

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO GCLK_CTRLA_Type           CTRLA;       /**< \brief Offset: 0x00 (R/W  8) Control */
	   RoReg8                    Reserved1[0x03];
  __I  GCLK_SYNCBUSY_Type        SYNCBUSY;    /**< \brief Offset: 0x04 (R/  32) Synchronization Busy */
	   RoReg8                    Reserved2[0x18];
  __IO GCLK_GENCTRL_Type         GENCTRL[12]; /**< \brief Offset: 0x20 (R/W 32) Generic Clock Generator Control */
	   RoReg8                    Reserved3[0x30];
  __IO GCLK_PCHCTRL_Type         PCHCTRL[48]; /**< \brief Offset: 0x80 (R/W 32) Peripheral Clock Control */
} Gclk;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_GCLK_COMPONENT_FIXUP_H_ */
