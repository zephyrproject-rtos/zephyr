/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_CCL_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_CCL_COMPONENT_FIXUP_H_

/* -------- CCL_CTRL : (CCL Offset: 0x00) (R/W 8) Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint8_t  ENABLE:1;         /*!< bit:      1  Enable                             */
    uint8_t  :4;               /*!< bit:  2.. 5  Reserved                           */
    uint8_t  RUNSTDBY:1;       /*!< bit:      6  Run in Standby                     */
    uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} CCL_CTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CCL_SEQCTRL : (CCL Offset: 0x04) (R/W 8) SEQ Control x -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SEQSEL:4;         /*!< bit:  0.. 3  Sequential Selection               */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} CCL_SEQCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CCL_LUTCTRL : (CCL Offset: 0x08) (R/W 32) LUT Control x -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t ENABLE:1;         /*!< bit:      1  LUT Enable                         */
    uint32_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint32_t FILTSEL:2;        /*!< bit:  4.. 5  Filter Selection                   */
    uint32_t :1;               /*!< bit:      6  Reserved                           */
    uint32_t EDGESEL:1;        /*!< bit:      7  Edge Selection                     */
    uint32_t INSEL0:4;         /*!< bit:  8..11  Input Selection 0                  */
    uint32_t INSEL1:4;         /*!< bit: 12..15  Input Selection 1                  */
    uint32_t INSEL2:4;         /*!< bit: 16..19  Input Selection 2                  */
    uint32_t INVEI:1;          /*!< bit:     20  Inverted Event Input Enable        */
    uint32_t LUTEI:1;          /*!< bit:     21  LUT Event Input Enable             */
    uint32_t LUTEO:1;          /*!< bit:     22  LUT Event Output Enable            */
    uint32_t :1;               /*!< bit:     23  Reserved                           */
    uint32_t TRUTH:8;          /*!< bit: 24..31  Truth Value                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CCL_LUTCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CCL hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO CCL_CTRL_Type             CTRL;        /**< \brief Offset: 0x0 (R/W  8) Control */
       RoReg8                    Reserved1[0x3];
  __IO CCL_SEQCTRL_Type          SEQCTRL[2];  /**< \brief Offset: 0x4 (R/W  8) SEQ Control x */
       RoReg8                    Reserved2[0x2];
  __IO CCL_LUTCTRL_Type          LUTCTRL[4];  /**< \brief Offset: 0x8 (R/W 32) LUT Control x */
} Ccl;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_CCL_COMPONENT_FIXUP_H_ */
