/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_NVMCTRL_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_NVMCTRL_COMPONENT_FIXUP_H_

/* -------- NVMCTRL_CTRLA : (NVMCTRL Offset: 0x00) (R/W 16) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint16_t AUTOWS:1;         /*!< bit:      2  Auto Wait State Enable             */
    uint16_t SUSPEN:1;         /*!< bit:      3  Suspend Enable                     */
    uint16_t WMODE:2;          /*!< bit:  4.. 5  Write Mode                         */
    uint16_t PRM:2;            /*!< bit:  6.. 7  Power Reduction Mode during Sleep  */
    uint16_t RWS:4;            /*!< bit:  8..11  NVM Read Wait States               */
    uint16_t AHBNS0:1;         /*!< bit:     12  Force AHB0 access to NONSEQ, burst transfers are continuously rearbitrated */
    uint16_t AHBNS1:1;         /*!< bit:     13  Force AHB1 access to NONSEQ, burst transfers are continuously rearbitrated */
    uint16_t CACHEDIS0:1;      /*!< bit:     14  AHB0 Cache Disable                 */
    uint16_t CACHEDIS1:1;      /*!< bit:     15  AHB1 Cache Disable                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} NVMCTRL_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_CTRLB : (NVMCTRL Offset: 0x04) ( /W 16) Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CMD:7;            /*!< bit:  0.. 6  Command                            */
    uint16_t :1;               /*!< bit:      7  Reserved                           */
    uint16_t CMDEX:8;          /*!< bit:  8..15  Command Execution                  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} NVMCTRL_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_PARAM : (NVMCTRL Offset: 0x08) ( R/ 32) NVM Parameter -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NVMP:16;          /*!< bit:  0..15  NVM Pages                          */
    uint32_t PSZ:3;            /*!< bit: 16..18  Page Size                          */
    uint32_t :12;              /*!< bit: 19..30  Reserved                           */
    uint32_t SEE:1;            /*!< bit:     31  SmartEEPROM Supported              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} NVMCTRL_PARAM_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_INTENCLR : (NVMCTRL Offset: 0x0C) (R/W 16) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DONE:1;           /*!< bit:      0  Command Done Interrupt Clear       */
    uint16_t ADDRE:1;          /*!< bit:      1  Address Error                      */
    uint16_t PROGE:1;          /*!< bit:      2  Programming Error Interrupt Clear  */
    uint16_t LOCKE:1;          /*!< bit:      3  Lock Error Interrupt Clear         */
    uint16_t ECCSE:1;          /*!< bit:      4  ECC Single Error Interrupt Clear   */
    uint16_t ECCDE:1;          /*!< bit:      5  ECC Dual Error Interrupt Clear     */
    uint16_t NVME:1;           /*!< bit:      6  NVM Error Interrupt Clear          */
    uint16_t SUSP:1;           /*!< bit:      7  Suspended Write Or Erase Interrupt Clear */
    uint16_t SEESFULL:1;       /*!< bit:      8  Active SEES Full Interrupt Clear   */
    uint16_t SEESOVF:1;        /*!< bit:      9  Active SEES Overflow Interrupt Clear */
    uint16_t SEEWRC:1;         /*!< bit:     10  SEE Write Completed Interrupt Clear */
    uint16_t :5;               /*!< bit: 11..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} NVMCTRL_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_INTENSET : (NVMCTRL Offset: 0x0E) (R/W 16) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DONE:1;           /*!< bit:      0  Command Done Interrupt Enable      */
    uint16_t ADDRE:1;          /*!< bit:      1  Address Error Interrupt Enable     */
    uint16_t PROGE:1;          /*!< bit:      2  Programming Error Interrupt Enable */
    uint16_t LOCKE:1;          /*!< bit:      3  Lock Error Interrupt Enable        */
    uint16_t ECCSE:1;          /*!< bit:      4  ECC Single Error Interrupt Enable  */
    uint16_t ECCDE:1;          /*!< bit:      5  ECC Dual Error Interrupt Enable    */
    uint16_t NVME:1;           /*!< bit:      6  NVM Error Interrupt Enable         */
    uint16_t SUSP:1;           /*!< bit:      7  Suspended Write Or Erase  Interrupt Enable */
    uint16_t SEESFULL:1;       /*!< bit:      8  Active SEES Full Interrupt Enable  */
    uint16_t SEESOVF:1;        /*!< bit:      9  Active SEES Overflow Interrupt Enable */
    uint16_t SEEWRC:1;         /*!< bit:     10  SEE Write Completed Interrupt Enable */
    uint16_t :5;               /*!< bit: 11..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} NVMCTRL_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_INTFLAG : (NVMCTRL Offset: 0x10) (R/W 16) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint16_t DONE:1;           /*!< bit:      0  Command Done                       */
    __I uint16_t ADDRE:1;          /*!< bit:      1  Address Error                      */
    __I uint16_t PROGE:1;          /*!< bit:      2  Programming Error                  */
    __I uint16_t LOCKE:1;          /*!< bit:      3  Lock Error                         */
    __I uint16_t ECCSE:1;          /*!< bit:      4  ECC Single Error                   */
    __I uint16_t ECCDE:1;          /*!< bit:      5  ECC Dual Error                     */
    __I uint16_t NVME:1;           /*!< bit:      6  NVM Error                          */
    __I uint16_t SUSP:1;           /*!< bit:      7  Suspended Write Or Erase Operation */
    __I uint16_t SEESFULL:1;       /*!< bit:      8  Active SEES Full                   */
    __I uint16_t SEESOVF:1;        /*!< bit:      9  Active SEES Overflow               */
    __I uint16_t SEEWRC:1;         /*!< bit:     10  SEE Write Completed                */
    __I uint16_t :5;               /*!< bit: 11..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} NVMCTRL_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_STATUS : (NVMCTRL Offset: 0x12) ( R/ 16) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t READY:1;          /*!< bit:      0  Ready to accept a command          */
    uint16_t PRM:1;            /*!< bit:      1  Power Reduction Mode               */
    uint16_t LOAD:1;           /*!< bit:      2  NVM Page Buffer Active Loading     */
    uint16_t SUSP:1;           /*!< bit:      3  NVM Write Or Erase Operation Is Suspended */
    uint16_t AFIRST:1;         /*!< bit:      4  BANKA First                        */
    uint16_t BPDIS:1;          /*!< bit:      5  Boot Loader Protection Disable     */
    uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint16_t BOOTPROT:4;       /*!< bit:  8..11  Boot Loader Protection Size        */
    uint16_t DPBE:1;           /*!< bit:     12  Dual Boot Protection Enable        */
    uint16_t BPHL:1;           /*!< bit:     13  Boot Protect Hard Lock             */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} NVMCTRL_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_ADDR : (NVMCTRL Offset: 0x14) (R/W 32) Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:24;          /*!< bit:  0..23  NVM Address                        */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} NVMCTRL_ADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_RUNLOCK : (NVMCTRL Offset: 0x18) ( R/ 32) Lock Section -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUNLOCK:32;       /*!< bit:  0..31  Region Un-Lock Bits                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} NVMCTRL_RUNLOCK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_PBLDATA : (NVMCTRL Offset: 0x1C) ( R/ 32) Page Buffer Load Data x -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DATA:32;          /*!< bit:  0..31  Page Buffer Data                   */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} NVMCTRL_PBLDATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_ECCERR : (NVMCTRL Offset: 0x24) ( R/ 32) ECC Error Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:24;          /*!< bit:  0..23  Error Address                      */
    uint32_t :4;               /*!< bit: 24..27  Reserved                           */
    uint32_t TYPEL:2;          /*!< bit: 28..29  Low Double-Word Error Type         */
    uint32_t TYPEH:2;          /*!< bit: 30..31  High Double-Word Error Type        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} NVMCTRL_ECCERR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_DBGCTRL : (NVMCTRL Offset: 0x28) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ECCDIS:1;         /*!< bit:      0  Debugger ECC Read Disable          */
    uint8_t  ECCELOG:1;        /*!< bit:      1  Debugger ECC Error Tracking Mode   */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} NVMCTRL_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_BCTRL : (NVMCTRL Offset: 0x29) (R/W 8) Boot Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  BRPE:1;           /*!< bit:      0  Boot Read Protection Enable                         */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} NVMCTRL_BCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_SEECFG : (NVMCTRL Offset: 0x2A) (R/W 8) SmartEEPROM Configuration Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  WMODE:1;          /*!< bit:      0  Write Mode                         */
    uint8_t  APRDIS:1;         /*!< bit:      1  Automatic Page Reallocation Disable */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} NVMCTRL_SEECFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- NVMCTRL_SEESTAT : (NVMCTRL Offset: 0x2C) ( R/ 32) SmartEEPROM Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ASEES:1;          /*!< bit:      0  Active SmartEEPROM Sector          */
    uint32_t LOAD:1;           /*!< bit:      1  Page Buffer Loaded                 */
    uint32_t BUSY:1;           /*!< bit:      2  Busy                               */
    uint32_t LOCK:1;           /*!< bit:      3  SmartEEPROM Write Access Is Locked */
    uint32_t RLOCK:1;          /*!< bit:      4  SmartEEPROM Write Access To Register Address Space Is Locked */
    uint32_t :3;               /*!< bit:  5.. 7  Reserved                           */
    uint32_t SBLK:4;           /*!< bit:  8..11  Blocks Number In a Sector          */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t PSZ:3;            /*!< bit: 16..18  SmartEEPROM Page Size              */
    uint32_t :13;              /*!< bit: 19..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} NVMCTRL_SEESTAT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief NVMCTRL APB hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO NVMCTRL_CTRLA_Type        CTRLA;       /**< \brief Offset: 0x00 (R/W 16) Control A */
       RoReg8                    Reserved1[0x2];
  __O  NVMCTRL_CTRLB_Type        CTRLB;       /**< \brief Offset: 0x04 ( /W 16) Control B */
       RoReg8                    Reserved2[0x2];
  __I  NVMCTRL_PARAM_Type        PARAM;       /**< \brief Offset: 0x08 (R/  32) NVM Parameter */
  __IO NVMCTRL_INTENCLR_Type     INTENCLR;    /**< \brief Offset: 0x0C (R/W 16) Interrupt Enable Clear */
  __IO NVMCTRL_INTENSET_Type     INTENSET;    /**< \brief Offset: 0x0E (R/W 16) Interrupt Enable Set */
  __IO NVMCTRL_INTFLAG_Type      INTFLAG;     /**< \brief Offset: 0x10 (R/W 16) Interrupt Flag Status and Clear */
  __I  NVMCTRL_STATUS_Type       STATUS;      /**< \brief Offset: 0x12 (R/  16) Status */
  __IO NVMCTRL_ADDR_Type         ADDR;        /**< \brief Offset: 0x14 (R/W 32) Address */
  __I  NVMCTRL_RUNLOCK_Type      RUNLOCK;     /**< \brief Offset: 0x18 (R/  32) Lock Section */
  __I  NVMCTRL_PBLDATA_Type      PBLDATA[2];  /**< \brief Offset: 0x1C (R/  32) Page Buffer Load Data x */
  __I  NVMCTRL_ECCERR_Type       ECCERR;      /**< \brief Offset: 0x24 (R/  32) ECC Error Status Register */
  __IO NVMCTRL_DBGCTRL_Type      DBGCTRL;     /**< \brief Offset: 0x28 (R/W  8) Debug Control */
  __IO NVMCTRL_BCTRL_Type        BCTRL;		  /**< \brief Offset: 0x29 (R/W  8) Boot Control */
  __IO NVMCTRL_SEECFG_Type       SEECFG;      /**< \brief Offset: 0x2A (R/W  8) SmartEEPROM Configuration Register */
       RoReg8                    Reserved4[0x1];
  __I  NVMCTRL_SEESTAT_Type      SEESTAT;     /**< \brief Offset: 0x2C (R/  32) SmartEEPROM Status Register */
} Nvmctrl;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_NVMCTRL_COMPONENT_FIXUP_H_ */
