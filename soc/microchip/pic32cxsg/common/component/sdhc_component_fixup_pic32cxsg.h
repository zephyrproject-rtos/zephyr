/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_SDHC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_SDHC_COMPONENT_FIXUP_H_

/* -------- SDHC_SSAR : (SDHC Offset: 0x00) (R/W 32) SDMA System Address / Argument 2 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // CMD23 mode
    uint32_t ARG2:32;          /*!< bit:  0..31  Argument 2                         */
  } CMD23;                     /*!< Structure used for CMD23                        */
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  SDMA System Address                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_SSAR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_BSR : (SDHC Offset: 0x04) (R/W 16) Block Size -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t BLOCKSIZE:10;     /*!< bit:  0.. 9  Transfer Block Size                */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t BOUNDARY:3;       /*!< bit: 12..14  SDMA Buffer Boundary               */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_BSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_BCR : (SDHC Offset: 0x06) (R/W 16) Block Count -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t BCNT:16;          /*!< bit:  0..15  Blocks Count for Current Transfer  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_BCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_ARG1R : (SDHC Offset: 0x08) (R/W 32) Argument 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ARG:32;           /*!< bit:  0..31  Argument 1                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_ARG1R_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_TMR : (SDHC Offset: 0x0C) (R/W 16) Transfer Mode -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DMAEN:1;          /*!< bit:      0  DMA Enable                         */
    uint16_t BCEN:1;           /*!< bit:      1  Block Count Enable                 */
    uint16_t ACMDEN:2;         /*!< bit:  2.. 3  Auto Command Enable                */
    uint16_t DTDSEL:1;         /*!< bit:      4  Data Transfer Direction Selection  */
    uint16_t MSBSEL:1;         /*!< bit:      5  Multi/Single Block Selection       */
    uint16_t :10;              /*!< bit:  6..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_TMR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_CR : (SDHC Offset: 0x0E) (R/W 16) Command -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t RESPTYP:2;        /*!< bit:  0.. 1  Response Type                      */
    uint16_t :1;               /*!< bit:      2  Reserved                           */
    uint16_t CMDCCEN:1;        /*!< bit:      3  Command CRC Check Enable           */
    uint16_t CMDICEN:1;        /*!< bit:      4  Command Index Check Enable         */
    uint16_t DPSEL:1;          /*!< bit:      5  Data Present Select                */
    uint16_t CMDTYP:2;         /*!< bit:  6.. 7  Command Type                       */
    uint16_t CMDIDX:6;         /*!< bit:  8..13  Command Index                      */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_CR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_RR : (SDHC Offset: 0x10) ( R/ 32) Response -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CMDRESP:32;       /*!< bit:  0..31  Command Response                   */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_RR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_BDPR : (SDHC Offset: 0x20) (R/W 32) Buffer Data Port -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BUFDATA:32;       /*!< bit:  0..31  Buffer Data                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_BDPR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_PSR : (SDHC Offset: 0x24) ( R/ 32) Present State -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CMDINHC:1;        /*!< bit:      0  Command Inhibit (CMD)              */
    uint32_t CMDINHD:1;        /*!< bit:      1  Command Inhibit (DAT)              */
    uint32_t DLACT:1;          /*!< bit:      2  DAT Line Active                    */
    uint32_t RTREQ:1;          /*!< bit:      3  Re-Tuning Request                  */
    uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint32_t WTACT:1;          /*!< bit:      8  Write Transfer Active              */
    uint32_t RTACT:1;          /*!< bit:      9  Read Transfer Active               */
    uint32_t BUFWREN:1;        /*!< bit:     10  Buffer Write Enable                */
    uint32_t BUFRDEN:1;        /*!< bit:     11  Buffer Read Enable                 */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t CARDINS:1;        /*!< bit:     16  Card Inserted                      */
    uint32_t CARDSS:1;         /*!< bit:     17  Card State Stable                  */
    uint32_t CARDDPL:1;        /*!< bit:     18  Card Detect Pin Level              */
    uint32_t WRPPL:1;          /*!< bit:     19  Write Protect Pin Level            */
    uint32_t DATLL:4;          /*!< bit: 20..23  DAT[3:0] Line Level                */
    uint32_t CMDLL:1;          /*!< bit:     24  CMD Line Level                     */
    uint32_t :7;               /*!< bit: 25..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_PSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_HC1R : (SDHC Offset: 0x28) (R/W 8) Host Control 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  LEDCTRL:1;        /*!< bit:      0  LED Control                        */
    uint8_t  DW:1;             /*!< bit:      1  Data Width                         */
    uint8_t  HSEN:1;           /*!< bit:      2  High Speed Enable                  */
    uint8_t  DMASEL:2;         /*!< bit:  3.. 4  DMA Select                         */
    uint8_t  :1;               /*!< bit:      5  Reserved                           */
    uint8_t  CARDDTL:1;        /*!< bit:      6  Card Detect Test Level             */
    uint8_t  CARDDSEL:1;       /*!< bit:      7  Card Detect Signal Selection       */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint8_t  :1;               /*!< bit:      0  Reserved                           */
    uint8_t  DW:1;             /*!< bit:      1  Data Width                         */
    uint8_t  HSEN:1;           /*!< bit:      2  High Speed Enable                  */
    uint8_t  DMASEL:2;         /*!< bit:  3.. 4  DMA Select                         */
    uint8_t  :3;               /*!< bit:  5.. 7  Reserved                           */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_HC1R_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_PCR : (SDHC Offset: 0x29) (R/W 8) Power Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SDBPWR:1;         /*!< bit:      0  SD Bus Power                       */
    uint8_t  SDBVSEL:3;        /*!< bit:  1.. 3  SD Bus Voltage Select              */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_PCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_BGCR : (SDHC Offset: 0x2A) (R/W 8) Block Gap Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  STPBGR:1;         /*!< bit:      0  Stop at Block Gap Request          */
    uint8_t  CONTR:1;          /*!< bit:      1  Continue Request                   */
    uint8_t  RWCTRL:1;         /*!< bit:      2  Read Wait Control                  */
    uint8_t  INTBG:1;          /*!< bit:      3  Interrupt at Block Gap             */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint8_t  STPBGR:1;         /*!< bit:      0  Stop at Block Gap Request          */
    uint8_t  CONTR:1;          /*!< bit:      1  Continue Request                   */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_BGCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_WCR : (SDHC Offset: 0x2B) (R/W 8) Wakeup Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  WKENCINT:1;       /*!< bit:      0  Wakeup Event Enable on Card Interrupt */
    uint8_t  WKENCINS:1;       /*!< bit:      1  Wakeup Event Enable on Card Insertion */
    uint8_t  WKENCREM:1;       /*!< bit:      2  Wakeup Event Enable on Card Removal */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_WCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_CCR : (SDHC Offset: 0x2C) (R/W 16) Clock Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t INTCLKEN:1;       /*!< bit:      0  Internal Clock Enable              */
    uint16_t INTCLKS:1;        /*!< bit:      1  Internal Clock Stable              */
    uint16_t SDCLKEN:1;        /*!< bit:      2  SD Clock Enable                    */
    uint16_t :2;               /*!< bit:  3.. 4  Reserved                           */
    uint16_t CLKGSEL:1;        /*!< bit:      5  Clock Generator Select             */
    uint16_t USDCLKFSEL:2;     /*!< bit:  6.. 7  Upper Bits of SDCLK Frequency Select */
    uint16_t SDCLKFSEL:8;      /*!< bit:  8..15  SDCLK Frequency Select             */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_CCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_TCR : (SDHC Offset: 0x2E) (R/W 8) Timeout Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DTCVAL:4;         /*!< bit:  0.. 3  Data Timeout Counter Value         */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_TCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_SRR : (SDHC Offset: 0x2F) (R/W 8) Software Reset -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRSTALL:1;       /*!< bit:      0  Software Reset For All             */
    uint8_t  SWRSTCMD:1;       /*!< bit:      1  Software Reset For CMD Line        */
    uint8_t  SWRSTDAT:1;       /*!< bit:      2  Software Reset For DAT Line        */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_SRR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_NISTR : (SDHC Offset: 0x30) (R/W 16) Normal Interrupt Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CMDC:1;           /*!< bit:      0  Command Complete                   */
    uint16_t TRFC:1;           /*!< bit:      1  Transfer Complete                  */
    uint16_t BLKGE:1;          /*!< bit:      2  Block Gap Event                    */
    uint16_t DMAINT:1;         /*!< bit:      3  DMA Interrupt                      */
    uint16_t BWRRDY:1;         /*!< bit:      4  Buffer Write Ready                 */
    uint16_t BRDRDY:1;         /*!< bit:      5  Buffer Read Ready                  */
    uint16_t CINS:1;           /*!< bit:      6  Card Insertion                     */
    uint16_t CREM:1;           /*!< bit:      7  Card Removal                       */
    uint16_t CINT:1;           /*!< bit:      8  Card Interrupt                     */
    uint16_t :6;               /*!< bit:  9..14  Reserved                           */
    uint16_t ERRINT:1;         /*!< bit:     15  Error Interrupt                    */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint16_t CMDC:1;           /*!< bit:      0  Command Complete                   */
    uint16_t TRFC:1;           /*!< bit:      1  Transfer Complete                  */
    uint16_t BLKGE:1;          /*!< bit:      2  Block Gap Event                    */
    uint16_t DMAINT:1;         /*!< bit:      3  DMA Interrupt                      */
    uint16_t BWRRDY:1;         /*!< bit:      4  Buffer Write Ready                 */
    uint16_t BRDRDY:1;         /*!< bit:      5  Buffer Read Ready                  */
    uint16_t :8;               /*!< bit:  6..13  Reserved                           */
    uint16_t BOOTAR:1;         /*!< bit:     14  Boot Acknowledge Received          */
    uint16_t ERRINT:1;         /*!< bit:     15  Error Interrupt                    */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_NISTR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_EISTR : (SDHC Offset: 0x32) (R/W 16) Error Interrupt Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CMDTEO:1;         /*!< bit:      0  Command Timeout Error              */
    uint16_t CMDCRC:1;         /*!< bit:      1  Command CRC Error                  */
    uint16_t CMDEND:1;         /*!< bit:      2  Command End Bit Error              */
    uint16_t CMDIDX:1;         /*!< bit:      3  Command Index Error                */
    uint16_t DATTEO:1;         /*!< bit:      4  Data Timeout Error                 */
    uint16_t DATCRC:1;         /*!< bit:      5  Data CRC Error                     */
    uint16_t DATEND:1;         /*!< bit:      6  Data End Bit Error                 */
    uint16_t CURLIM:1;         /*!< bit:      7  Current Limit Error                */
    uint16_t ACMD:1;           /*!< bit:      8  Auto CMD Error                     */
    uint16_t ADMA:1;           /*!< bit:      9  ADMA Error                         */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint16_t CMDTEO:1;         /*!< bit:      0  Command Timeout Error              */
    uint16_t CMDCRC:1;         /*!< bit:      1  Command CRC Error                  */
    uint16_t CMDEND:1;         /*!< bit:      2  Command End Bit Error              */
    uint16_t CMDIDX:1;         /*!< bit:      3  Command Index Error                */
    uint16_t DATTEO:1;         /*!< bit:      4  Data Timeout Error                 */
    uint16_t DATCRC:1;         /*!< bit:      5  Data CRC Error                     */
    uint16_t DATEND:1;         /*!< bit:      6  Data End Bit Error                 */
    uint16_t CURLIM:1;         /*!< bit:      7  Current Limit Error                */
    uint16_t ACMD:1;           /*!< bit:      8  Auto CMD Error                     */
    uint16_t ADMA:1;           /*!< bit:      9  ADMA Error                         */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t BOOTAE:1;         /*!< bit:     12  Boot Acknowledge Error             */
    uint16_t :3;               /*!< bit: 13..15  Reserved                           */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_EISTR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_NISTER : (SDHC Offset: 0x34) (R/W 16) Normal Interrupt Status Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CMDC:1;           /*!< bit:      0  Command Complete Status Enable     */
    uint16_t TRFC:1;           /*!< bit:      1  Transfer Complete Status Enable    */
    uint16_t BLKGE:1;          /*!< bit:      2  Block Gap Event Status Enable      */
    uint16_t DMAINT:1;         /*!< bit:      3  DMA Interrupt Status Enable        */
    uint16_t BWRRDY:1;         /*!< bit:      4  Buffer Write Ready Status Enable   */
    uint16_t BRDRDY:1;         /*!< bit:      5  Buffer Read Ready Status Enable    */
    uint16_t CINS:1;           /*!< bit:      6  Card Insertion Status Enable       */
    uint16_t CREM:1;           /*!< bit:      7  Card Removal Status Enable         */
    uint16_t CINT:1;           /*!< bit:      8  Card Interrupt Status Enable       */
    uint16_t :7;               /*!< bit:  9..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint16_t CMDC:1;           /*!< bit:      0  Command Complete Status Enable     */
    uint16_t TRFC:1;           /*!< bit:      1  Transfer Complete Status Enable    */
    uint16_t BLKGE:1;          /*!< bit:      2  Block Gap Event Status Enable      */
    uint16_t DMAINT:1;         /*!< bit:      3  DMA Interrupt Status Enable        */
    uint16_t BWRRDY:1;         /*!< bit:      4  Buffer Write Ready Status Enable   */
    uint16_t BRDRDY:1;         /*!< bit:      5  Buffer Read Ready Status Enable    */
    uint16_t :8;               /*!< bit:  6..13  Reserved                           */
    uint16_t BOOTAR:1;         /*!< bit:     14  Boot Acknowledge Received Status Enable */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_NISTER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_EISTER : (SDHC Offset: 0x36) (R/W 16) Error Interrupt Status Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CMDTEO:1;         /*!< bit:      0  Command Timeout Error Status Enable */
    uint16_t CMDCRC:1;         /*!< bit:      1  Command CRC Error Status Enable    */
    uint16_t CMDEND:1;         /*!< bit:      2  Command End Bit Error Status Enable */
    uint16_t CMDIDX:1;         /*!< bit:      3  Command Index Error Status Enable  */
    uint16_t DATTEO:1;         /*!< bit:      4  Data Timeout Error Status Enable   */
    uint16_t DATCRC:1;         /*!< bit:      5  Data CRC Error Status Enable       */
    uint16_t DATEND:1;         /*!< bit:      6  Data End Bit Error Status Enable   */
    uint16_t CURLIM:1;         /*!< bit:      7  Current Limit Error Status Enable  */
    uint16_t ACMD:1;           /*!< bit:      8  Auto CMD Error Status Enable       */
    uint16_t ADMA:1;           /*!< bit:      9  ADMA Error Status Enable           */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint16_t CMDTEO:1;         /*!< bit:      0  Command Timeout Error Status Enable */
    uint16_t CMDCRC:1;         /*!< bit:      1  Command CRC Error Status Enable    */
    uint16_t CMDEND:1;         /*!< bit:      2  Command End Bit Error Status Enable */
    uint16_t CMDIDX:1;         /*!< bit:      3  Command Index Error Status Enable  */
    uint16_t DATTEO:1;         /*!< bit:      4  Data Timeout Error Status Enable   */
    uint16_t DATCRC:1;         /*!< bit:      5  Data CRC Error Status Enable       */
    uint16_t DATEND:1;         /*!< bit:      6  Data End Bit Error Status Enable   */
    uint16_t CURLIM:1;         /*!< bit:      7  Current Limit Error Status Enable  */
    uint16_t ACMD:1;           /*!< bit:      8  Auto CMD Error Status Enable       */
    uint16_t ADMA:1;           /*!< bit:      9  ADMA Error Status Enable           */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t BOOTAE:1;         /*!< bit:     12  Boot Acknowledge Error Status Enable */
    uint16_t :3;               /*!< bit: 13..15  Reserved                           */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_EISTER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_NISIER : (SDHC Offset: 0x38) (R/W 16) Normal Interrupt Signal Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CMDC:1;           /*!< bit:      0  Command Complete Signal Enable     */
    uint16_t TRFC:1;           /*!< bit:      1  Transfer Complete Signal Enable    */
    uint16_t BLKGE:1;          /*!< bit:      2  Block Gap Event Signal Enable      */
    uint16_t DMAINT:1;         /*!< bit:      3  DMA Interrupt Signal Enable        */
    uint16_t BWRRDY:1;         /*!< bit:      4  Buffer Write Ready Signal Enable   */
    uint16_t BRDRDY:1;         /*!< bit:      5  Buffer Read Ready Signal Enable    */
    uint16_t CINS:1;           /*!< bit:      6  Card Insertion Signal Enable       */
    uint16_t CREM:1;           /*!< bit:      7  Card Removal Signal Enable         */
    uint16_t CINT:1;           /*!< bit:      8  Card Interrupt Signal Enable       */
    uint16_t :7;               /*!< bit:  9..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint16_t CMDC:1;           /*!< bit:      0  Command Complete Signal Enable     */
    uint16_t TRFC:1;           /*!< bit:      1  Transfer Complete Signal Enable    */
    uint16_t BLKGE:1;          /*!< bit:      2  Block Gap Event Signal Enable      */
    uint16_t DMAINT:1;         /*!< bit:      3  DMA Interrupt Signal Enable        */
    uint16_t BWRRDY:1;         /*!< bit:      4  Buffer Write Ready Signal Enable   */
    uint16_t BRDRDY:1;         /*!< bit:      5  Buffer Read Ready Signal Enable    */
    uint16_t :8;               /*!< bit:  6..13  Reserved                           */
    uint16_t BOOTAR:1;         /*!< bit:     14  Boot Acknowledge Received Signal Enable */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_NISIER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_EISIER : (SDHC Offset: 0x3A) (R/W 16) Error Interrupt Signal Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CMDTEO:1;         /*!< bit:      0  Command Timeout Error Signal Enable */
    uint16_t CMDCRC:1;         /*!< bit:      1  Command CRC Error Signal Enable    */
    uint16_t CMDEND:1;         /*!< bit:      2  Command End Bit Error Signal Enable */
    uint16_t CMDIDX:1;         /*!< bit:      3  Command Index Error Signal Enable  */
    uint16_t DATTEO:1;         /*!< bit:      4  Data Timeout Error Signal Enable   */
    uint16_t DATCRC:1;         /*!< bit:      5  Data CRC Error Signal Enable       */
    uint16_t DATEND:1;         /*!< bit:      6  Data End Bit Error Signal Enable   */
    uint16_t CURLIM:1;         /*!< bit:      7  Current Limit Error Signal Enable  */
    uint16_t ACMD:1;           /*!< bit:      8  Auto CMD Error Signal Enable       */
    uint16_t ADMA:1;           /*!< bit:      9  ADMA Error Signal Enable           */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint16_t CMDTEO:1;         /*!< bit:      0  Command Timeout Error Signal Enable */
    uint16_t CMDCRC:1;         /*!< bit:      1  Command CRC Error Signal Enable    */
    uint16_t CMDEND:1;         /*!< bit:      2  Command End Bit Error Signal Enable */
    uint16_t CMDIDX:1;         /*!< bit:      3  Command Index Error Signal Enable  */
    uint16_t DATTEO:1;         /*!< bit:      4  Data Timeout Error Signal Enable   */
    uint16_t DATCRC:1;         /*!< bit:      5  Data CRC Error Signal Enable       */
    uint16_t DATEND:1;         /*!< bit:      6  Data End Bit Error Signal Enable   */
    uint16_t CURLIM:1;         /*!< bit:      7  Current Limit Error Signal Enable  */
    uint16_t ACMD:1;           /*!< bit:      8  Auto CMD Error Signal Enable       */
    uint16_t ADMA:1;           /*!< bit:      9  ADMA Error Signal Enable           */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t BOOTAE:1;         /*!< bit:     12  Boot Acknowledge Error Signal Enable */
    uint16_t :3;               /*!< bit: 13..15  Reserved                           */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_EISIER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_ACESR : (SDHC Offset: 0x3C) ( R/ 16) Auto CMD Error Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t ACMD12NE:1;       /*!< bit:      0  Auto CMD12 Not Executed            */
    uint16_t ACMDTEO:1;        /*!< bit:      1  Auto CMD Timeout Error             */
    uint16_t ACMDCRC:1;        /*!< bit:      2  Auto CMD CRC Error                 */
    uint16_t ACMDEND:1;        /*!< bit:      3  Auto CMD End Bit Error             */
    uint16_t ACMDIDX:1;        /*!< bit:      4  Auto CMD Index Error               */
    uint16_t :2;               /*!< bit:  5.. 6  Reserved                           */
    uint16_t CMDNI:1;          /*!< bit:      7  Command not Issued By Auto CMD12 Error */
    uint16_t :8;               /*!< bit:  8..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_ACESR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_HC2R : (SDHC Offset: 0x3E) (R/W 16) Host Control 2 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t UHSMS:3;          /*!< bit:  0.. 2  UHS Mode Select                    */
    uint16_t VS18EN:1;         /*!< bit:      3  1.8V Signaling Enable              */
    uint16_t DRVSEL:2;         /*!< bit:  4.. 5  Driver Strength Select             */
    uint16_t EXTUN:1;          /*!< bit:      6  Execute Tuning                     */
    uint16_t SLCKSEL:1;        /*!< bit:      7  Sampling Clock Select              */
    uint16_t :6;               /*!< bit:  8..13  Reserved                           */
    uint16_t ASINTEN:1;        /*!< bit:     14  Asynchronous Interrupt Enable      */
    uint16_t PVALEN:1;         /*!< bit:     15  Preset Value Enable                */
  } bit;                       /*!< Structure used for bit  access                  */
  struct { // EMMC mode
    uint16_t HS200EN:4;        /*!< bit:  0.. 3  HS200 Mode Enable                  */
    uint16_t DRVSEL:2;         /*!< bit:  4.. 5  Driver Strength Select             */
    uint16_t EXTUN:1;          /*!< bit:      6  Execute Tuning                     */
    uint16_t SLCKSEL:1;        /*!< bit:      7  Sampling Clock Select              */
    uint16_t :7;               /*!< bit:  8..14  Reserved                           */
    uint16_t PVALEN:1;         /*!< bit:     15  Preset Value Enable                */
  } EMMC;                      /*!< Structure used for EMMC                         */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_HC2R_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- SDHC_CA0R : (SDHC Offset: 0x40) ( R/ 32) Capabilities 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TEOCLKF:6;        /*!< bit:  0.. 5  Timeout Clock Frequency            */
    uint32_t :1;               /*!< bit:      6  Reserved                           */
    uint32_t TEOCLKU:1;        /*!< bit:      7  Timeout Clock Unit                 */
    uint32_t BASECLKF:8;       /*!< bit:  8..15  Base Clock Frequency               */
    uint32_t MAXBLKL:2;        /*!< bit: 16..17  Max Block Length                   */
    uint32_t ED8SUP:1;         /*!< bit:     18  8-bit Support for Embedded Device  */
    uint32_t ADMA2SUP:1;       /*!< bit:     19  ADMA2 Support                      */
    uint32_t :1;               /*!< bit:     20  Reserved                           */
    uint32_t HSSUP:1;          /*!< bit:     21  High Speed Support                 */
    uint32_t SDMASUP:1;        /*!< bit:     22  SDMA Support                       */
    uint32_t SRSUP:1;          /*!< bit:     23  Suspend/Resume Support             */
    uint32_t V33VSUP:1;        /*!< bit:     24  Voltage Support 3.3V               */
    uint32_t V30VSUP:1;        /*!< bit:     25  Voltage Support 3.0V               */
    uint32_t V18VSUP:1;        /*!< bit:     26  Voltage Support 1.8V               */
    uint32_t :1;               /*!< bit:     27  Reserved                           */
    uint32_t SB64SUP:1;        /*!< bit:     28  64-Bit System Bus Support          */
    uint32_t ASINTSUP:1;       /*!< bit:     29  Asynchronous Interrupt Support     */
    uint32_t SLTYPE:2;         /*!< bit: 30..31  Slot Type                          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_CA0R_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_CA1R : (SDHC Offset: 0x44) ( R/ 32) Capabilities 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SDR50SUP:1;       /*!< bit:      0  SDR50 Support                      */
    uint32_t SDR104SUP:1;      /*!< bit:      1  SDR104 Support                     */
    uint32_t DDR50SUP:1;       /*!< bit:      2  DDR50 Support                      */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t DRVASUP:1;        /*!< bit:      4  Driver Type A Support              */
    uint32_t DRVCSUP:1;        /*!< bit:      5  Driver Type C Support              */
    uint32_t DRVDSUP:1;        /*!< bit:      6  Driver Type D Support              */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t TCNTRT:4;         /*!< bit:  8..11  Timer Count for Re-Tuning          */
    uint32_t :1;               /*!< bit:     12  Reserved                           */
    uint32_t TSDR50:1;         /*!< bit:     13  Use Tuning for SDR50               */
    uint32_t :2;               /*!< bit: 14..15  Reserved                           */
    uint32_t CLKMULT:8;        /*!< bit: 16..23  Clock Multiplier                   */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_CA1R_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_MCCAR : (SDHC Offset: 0x48) ( R/ 32) Maximum Current Capabilities -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MAXCUR33V:8;      /*!< bit:  0.. 7  Maximum Current for 3.3V           */
    uint32_t MAXCUR30V:8;      /*!< bit:  8..15  Maximum Current for 3.0V           */
    uint32_t MAXCUR18V:8;      /*!< bit: 16..23  Maximum Current for 1.8V           */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_MCCAR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- SDHC_FERACES : (SDHC Offset: 0x50) ( /W 16) Force Event for Auto CMD Error Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t ACMD12NE:1;       /*!< bit:      0  Force Event for Auto CMD12 Not Executed */
    uint16_t ACMDTEO:1;        /*!< bit:      1  Force Event for Auto CMD Timeout Error */
    uint16_t ACMDCRC:1;        /*!< bit:      2  Force Event for Auto CMD CRC Error */
    uint16_t ACMDEND:1;        /*!< bit:      3  Force Event for Auto CMD End Bit Error */
    uint16_t ACMDIDX:1;        /*!< bit:      4  Force Event for Auto CMD Index Error */
    uint16_t :2;               /*!< bit:  5.. 6  Reserved                           */
    uint16_t CMDNI:1;          /*!< bit:      7  Force Event for Command Not Issued By Auto CMD12 Error */
    uint16_t :8;               /*!< bit:  8..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_FERACES_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_FEREIS : (SDHC Offset: 0x52) ( /W 16) Force Event for Error Interrupt Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CMDTEO:1;         /*!< bit:      0  Force Event for Command Timeout Error */
    uint16_t CMDCRC:1;         /*!< bit:      1  Force Event for Command CRC Error  */
    uint16_t CMDEND:1;         /*!< bit:      2  Force Event for Command End Bit Error */
    uint16_t CMDIDX:1;         /*!< bit:      3  Force Event for Command Index Error */
    uint16_t DATTEO:1;         /*!< bit:      4  Force Event for Data Timeout Error */
    uint16_t DATCRC:1;         /*!< bit:      5  Force Event for Data CRC Error     */
    uint16_t DATEND:1;         /*!< bit:      6  Force Event for Data End Bit Error */
    uint16_t CURLIM:1;         /*!< bit:      7  Force Event for Current Limit Error */
    uint16_t ACMD:1;           /*!< bit:      8  Force Event for Auto CMD Error     */
    uint16_t ADMA:1;           /*!< bit:      9  Force Event for ADMA Error         */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t BOOTAE:1;         /*!< bit:     12  Force Event for Boot Acknowledge Error */
    uint16_t :3;               /*!< bit: 13..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_FEREIS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_AESR : (SDHC Offset: 0x54) ( R/ 8) ADMA Error Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ERRST:2;          /*!< bit:  0.. 1  ADMA Error State                   */
    uint8_t  LMIS:1;           /*!< bit:      2  ADMA Length Mismatch Error         */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_AESR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_ASAR : (SDHC Offset: 0x58) (R/W 32) ADMA System Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADMASA:32;        /*!< bit:  0..31  ADMA System Address                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_ASAR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_PVR : (SDHC Offset: 0x60) (R/W 16) Preset Value n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SDCLKFSEL:10;     /*!< bit:  0.. 9  SDCLK Frequency Select Value for Initialization */
    uint16_t CLKGSEL:1;        /*!< bit:     10  Clock Generator Select Value for Initialization */
    uint16_t :3;               /*!< bit: 11..13  Reserved                           */
    uint16_t DRVSEL:2;         /*!< bit: 14..15  Driver Strength Select Value for Initialization */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_PVR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_SISR : (SDHC Offset: 0xFC) ( R/ 16) Slot Interrupt Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t INTSSL:1;         /*!< bit:      0  Interrupt Signal for Each Slot     */
    uint16_t :15;              /*!< bit:  1..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_SISR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_HCVR : (SDHC Offset: 0xFE) ( R/ 16) Host Controller Version -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SVER:8;           /*!< bit:  0.. 7  Spec Version                       */
    uint16_t VVER:8;           /*!< bit:  8..15  Vendor Version                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} SDHC_HCVR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_APSR : (SDHC Offset: 0x200) ( R/ 32) Additional Present State Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t HDATLL:4;         /*!< bit:  0.. 3  High Line Level                       */
    uint32_t :28;              /*!< bit:  4..32  reserved                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_APSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_MC1R : (SDHC Offset: 0x204) (R/W 8) MMC Control 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CMDTYP:2;         /*!< bit:  0.. 1  e.MMC Command Type                 */
    uint8_t  :1;               /*!< bit:      2  Reserved                           */
    uint8_t  DDR:1;            /*!< bit:      3  e.MMC HSDDR Mode                   */
    uint8_t  OPD:1;            /*!< bit:      4  e.MMC Open Drain Mode              */
    uint8_t  BOOTA:1;          /*!< bit:      5  e.MMC Boot Acknowledge Enable      */
    uint8_t  RSTN:1;           /*!< bit:      6  e.MMC Reset Signal                 */
    uint8_t  FCD:1;            /*!< bit:      7  e.MMC Force Card Detect            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_MC1R_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_MC2R : (SDHC Offset: 0x205) ( /W 8) MMC Control 2 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SRESP:1;          /*!< bit:      0  e.MMC Abort Wait IRQ               */
    uint8_t  ABOOT:1;          /*!< bit:      1  e.MMC Abort Boot                   */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_MC2R_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_ACR : (SDHC Offset: 0x208) (R/W 32) AHB Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BMAX:2;           /*!< bit:  0.. 1  AHB Maximum Burst                  */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_ACR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_CC2R : (SDHC Offset: 0x20C) (R/W 32) Clock Control 2 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FSDCLKD:1;        /*!< bit:      0  Force SDCK Disabled                */
    uint32_t :31;              /*!< bit:  1..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_CC2R_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_CACR : (SDHC Offset: 0x230) (R/W 32) Capabilities Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CAPWREN:1;        /*!< bit:      0  Capabilities Registers Write Enable (Required to write the correct frequencies in the Capabilities Registers) */
    uint32_t :7;               /*!< bit:  1.. 7  Reserved                           */
    uint32_t KEY:8;            /*!< bit:  8..15  Key (0x46)                         */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SDHC_CACR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SDHC_DBGR : (SDHC Offset: 0x234) (R/W 8) Debug -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  NIDBG:1;          /*!< bit:      0  Non-intrusive debug enable         */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} SDHC_DBGR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief SDHC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO SDHC_SSAR_Type            SSAR;        /**< \brief Offset: 0x000 (R/W 32) SDMA System Address / Argument 2 */
  __IO SDHC_BSR_Type             BSR;         /**< \brief Offset: 0x004 (R/W 16) Block Size */
  __IO SDHC_BCR_Type             BCR;         /**< \brief Offset: 0x006 (R/W 16) Block Count */
  __IO SDHC_ARG1R_Type           ARG1R;       /**< \brief Offset: 0x008 (R/W 32) Argument 1 */
  __IO SDHC_TMR_Type             TMR;         /**< \brief Offset: 0x00C (R/W 16) Transfer Mode */
  __IO SDHC_CR_Type              CR;          /**< \brief Offset: 0x00E (R/W 16) Command */
  __I  SDHC_RR_Type              RR[4];       /**< \brief Offset: 0x010 (R/  32) Response */
  __IO SDHC_BDPR_Type            BDPR;        /**< \brief Offset: 0x020 (R/W 32) Buffer Data Port */
  __I  SDHC_PSR_Type             PSR;         /**< \brief Offset: 0x024 (R/  32) Present State */
  __IO SDHC_HC1R_Type            HC1R;        /**< \brief Offset: 0x028 (R/W  8) Host Control 1 */
  __IO SDHC_PCR_Type             PCR;         /**< \brief Offset: 0x029 (R/W  8) Power Control */
  __IO SDHC_BGCR_Type            BGCR;        /**< \brief Offset: 0x02A (R/W  8) Block Gap Control */
  __IO SDHC_WCR_Type             WCR;         /**< \brief Offset: 0x02B (R/W  8) Wakeup Control */
  __IO SDHC_CCR_Type             CCR;         /**< \brief Offset: 0x02C (R/W 16) Clock Control */
  __IO SDHC_TCR_Type             TCR;         /**< \brief Offset: 0x02E (R/W  8) Timeout Control */
  __IO SDHC_SRR_Type             SRR;         /**< \brief Offset: 0x02F (R/W  8) Software Reset */
  __IO SDHC_NISTR_Type           NISTR;       /**< \brief Offset: 0x030 (R/W 16) Normal Interrupt Status */
  __IO SDHC_EISTR_Type           EISTR;       /**< \brief Offset: 0x032 (R/W 16) Error Interrupt Status */
  __IO SDHC_NISTER_Type          NISTER;      /**< \brief Offset: 0x034 (R/W 16) Normal Interrupt Status Enable */
  __IO SDHC_EISTER_Type          EISTER;      /**< \brief Offset: 0x036 (R/W 16) Error Interrupt Status Enable */
  __IO SDHC_NISIER_Type          NISIER;      /**< \brief Offset: 0x038 (R/W 16) Normal Interrupt Signal Enable */
  __IO SDHC_EISIER_Type          EISIER;      /**< \brief Offset: 0x03A (R/W 16) Error Interrupt Signal Enable */
  __I  SDHC_ACESR_Type           ACESR;       /**< \brief Offset: 0x03C (R/  16) Auto CMD Error Status */
  __IO SDHC_HC2R_Type            HC2R;        /**< \brief Offset: 0x03E (R/W 16) Host Control 2 */
  __I  SDHC_CA0R_Type            CA0R;        /**< \brief Offset: 0x040 (R/  32) Capabilities 0 */
  __I  SDHC_CA1R_Type            CA1R;        /**< \brief Offset: 0x044 (R/  32) Capabilities 1 */
  __I  SDHC_MCCAR_Type           MCCAR;       /**< \brief Offset: 0x048 (R/  32) Maximum Current Capabilities */
       RoReg8                    Reserved1[0x4];
  __O  SDHC_FERACES_Type         FERACES;     /**< \brief Offset: 0x050 ( /W 16) Force Event for Auto CMD Error Status */
  __O  SDHC_FEREIS_Type          FEREIS;      /**< \brief Offset: 0x052 ( /W 16) Force Event for Error Interrupt Status */
  __I  SDHC_AESR_Type            AESR;        /**< \brief Offset: 0x054 (R/   8) ADMA Error Status */
       RoReg8                    Reserved2[0x3];
  __IO SDHC_ASAR_Type            ASAR[1];     /**< \brief Offset: 0x058 (R/W 32) ADMA System Address n */
       RoReg8                    Reserved3[0x4];
  __IO SDHC_PVR_Type             PVR[8];      /**< \brief Offset: 0x060 (R/W 16) Preset Value n */
       RoReg8                    Reserved4[0x8C];
  __I  SDHC_SISR_Type            SISR;        /**< \brief Offset: 0x0FC (R/  16) Slot Interrupt Status */
  __I  SDHC_HCVR_Type            HCVR;        /**< \brief Offset: 0x0FE (R/  16) Host Controller Version */
       RoReg8                    Reserved5[0x100];
  __I  SDHC_APSR_Type            APSR;        /**< Offset: 0x200 (R/   32) Additional Present State Register */
  __IO SDHC_MC1R_Type            MC1R;        /**< \brief Offset: 0x204 (R/W  8) MMC Control 1 */
  __O  SDHC_MC2R_Type            MC2R;        /**< \brief Offset: 0x205 ( /W  8) MMC Control 2 */
       RoReg8                    Reserved6[0x2];
  __IO SDHC_ACR_Type             ACR;         /**< \brief Offset: 0x208 (R/W 32) AHB Control */
  __IO SDHC_CC2R_Type            CC2R;        /**< \brief Offset: 0x20C (R/W 32) Clock Control 2 */
       RoReg8                    Reserved7[0x20];
  __IO SDHC_CACR_Type            CACR;        /**< \brief Offset: 0x230 (R/W 32) Capabilities Control */
  __IO SDHC_DBGR_Type            DBGR;        /**< \brief Offset: 0x234 (R/W  8) Debug */
} Sdhc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_SDHC_COMPONENT_FIXUP_H_ */
