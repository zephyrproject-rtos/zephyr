/* ---------------------------------------------------------------------------- */
/*                  Atmel Microcontroller Software Support                      */
/*                       SAM Software Package License                           */
/* ---------------------------------------------------------------------------- */
/* Copyright (c) %copyright_year%, Atmel Corporation                                        */
/*                                                                              */
/* All rights reserved.                                                         */
/*                                                                              */
/* Redistribution and use in source and binary forms, with or without           */
/* modification, are permitted provided that the following condition is met:    */
/*                                                                              */
/* - Redistributions of source code must retain the above copyright notice,     */
/* this list of conditions and the disclaimer below.                            */
/*                                                                              */
/* Atmel's name may not be used to endorse or promote products derived from     */
/* this software without specific prior written permission.                     */
/*                                                                              */
/* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR   */
/* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE   */
/* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,      */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,  */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/* ---------------------------------------------------------------------------- */

#ifndef _SAM3XA_SMC_COMPONENT_
#define _SAM3XA_SMC_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Static Memory Controller */
/* ============================================================================= */
/** \addtogroup SAM3XA_SMC Static Memory Controller */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief SmcCs_number hardware registers */
typedef struct {
  __IO uint32_t SMC_SETUP;   /**< \brief (SmcCs_number Offset: 0x0) SMC Setup Register */
  __IO uint32_t SMC_PULSE;   /**< \brief (SmcCs_number Offset: 0x4) SMC Pulse Register */
  __IO uint32_t SMC_CYCLE;   /**< \brief (SmcCs_number Offset: 0x8) SMC Cycle Register */
  __IO uint32_t SMC_TIMINGS; /**< \brief (SmcCs_number Offset: 0xC) SMC Timings Register */
  __IO uint32_t SMC_MODE;    /**< \brief (SmcCs_number Offset: 0x10) SMC Mode Register */
} SmcCs_number;
/** \brief Smc hardware registers */
#define SMCCS_NUMBER_NUMBER 8
typedef struct {
  __IO uint32_t     SMC_CFG;                            /**< \brief (Smc Offset: 0x000) SMC NFC Configuration Register */
  __O  uint32_t     SMC_CTRL;                           /**< \brief (Smc Offset: 0x004) SMC NFC Control Register */
  __I  uint32_t     SMC_SR;                             /**< \brief (Smc Offset: 0x008) SMC NFC Status Register */
  __O  uint32_t     SMC_IER;                            /**< \brief (Smc Offset: 0x00C) SMC NFC Interrupt Enable Register */
  __O  uint32_t     SMC_IDR;                            /**< \brief (Smc Offset: 0x010) SMC NFC Interrupt Disable Register */
  __I  uint32_t     SMC_IMR;                            /**< \brief (Smc Offset: 0x014) SMC NFC Interrupt Mask Register */
  __IO uint32_t     SMC_ADDR;                           /**< \brief (Smc Offset: 0x018) SMC NFC Address Cycle Zero Register */
  __IO uint32_t     SMC_BANK;                           /**< \brief (Smc Offset: 0x01C) SMC Bank Address Register */
  __O  uint32_t     SMC_ECC_CTRL;                       /**< \brief (Smc Offset: 0x020) SMC ECC Control Register */
  __IO uint32_t     SMC_ECC_MD;                         /**< \brief (Smc Offset: 0x024) SMC ECC Mode Register */
  __I  uint32_t     SMC_ECC_SR1;                        /**< \brief (Smc Offset: 0x028) SMC ECC Status 1 Register */
  __I  uint32_t     SMC_ECC_PR0;                        /**< \brief (Smc Offset: 0x02C) SMC ECC Parity 0 Register */
  __I  uint32_t     SMC_ECC_PR1;                        /**< \brief (Smc Offset: 0x030) SMC ECC parity 1 Register */
  __I  uint32_t     SMC_ECC_SR2;                        /**< \brief (Smc Offset: 0x034) SMC ECC status 2 Register */
  __I  uint32_t     SMC_ECC_PR2;                        /**< \brief (Smc Offset: 0x038) SMC ECC parity 2 Register */
  __I  uint32_t     SMC_ECC_PR3;                        /**< \brief (Smc Offset: 0x03C) SMC ECC parity 3 Register */
  __I  uint32_t     SMC_ECC_PR4;                        /**< \brief (Smc Offset: 0x040) SMC ECC parity 4 Register */
  __I  uint32_t     SMC_ECC_PR5;                        /**< \brief (Smc Offset: 0x044) SMC ECC parity 5 Register */
  __I  uint32_t     SMC_ECC_PR6;                        /**< \brief (Smc Offset: 0x048) SMC ECC parity 6 Register */
  __I  uint32_t     SMC_ECC_PR7;                        /**< \brief (Smc Offset: 0x04C) SMC ECC parity 7 Register */
  __I  uint32_t     SMC_ECC_PR8;                        /**< \brief (Smc Offset: 0x050) SMC ECC parity 8 Register */
  __I  uint32_t     SMC_ECC_PR9;                        /**< \brief (Smc Offset: 0x054) SMC ECC parity 9 Register */
  __I  uint32_t     SMC_ECC_PR10;                       /**< \brief (Smc Offset: 0x058) SMC ECC parity 10 Register */
  __I  uint32_t     SMC_ECC_PR11;                       /**< \brief (Smc Offset: 0x05C) SMC ECC parity 11 Register */
  __I  uint32_t     SMC_ECC_PR12;                       /**< \brief (Smc Offset: 0x060) SMC ECC parity 12 Register */
  __I  uint32_t     SMC_ECC_PR13;                       /**< \brief (Smc Offset: 0x064) SMC ECC parity 13 Register */
  __I  uint32_t     SMC_ECC_PR14;                       /**< \brief (Smc Offset: 0x068) SMC ECC parity 14 Register */
  __I  uint32_t     SMC_ECC_PR15;                       /**< \brief (Smc Offset: 0x06C) SMC ECC parity 15 Register */
       SmcCs_number SMC_CS_NUMBER[SMCCS_NUMBER_NUMBER]; /**< \brief (Smc Offset: 0x70) CS_number = 0 .. 7 */
  __IO uint32_t     SMC_OCMS;                           /**< \brief (Smc Offset: 0x110) SMC OCMS Register */
  __O  uint32_t     SMC_KEY1;                           /**< \brief (Smc Offset: 0x114) SMC OCMS KEY1 Register */
  __O  uint32_t     SMC_KEY2;                           /**< \brief (Smc Offset: 0x118) SMC OCMS KEY2 Register */
  __I  uint32_t     Reserved1[50];
  __O  uint32_t     SMC_WPCR;                           /**< \brief (Smc Offset: 0x1E4) Write Protection Control Register */
  __I  uint32_t     SMC_WPSR;                           /**< \brief (Smc Offset: 0x1E8) Write Protection Status Register */
} Smc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- SMC_CFG : (SMC Offset: 0x000) SMC NFC Configuration Register -------- */
#define SMC_CFG_PAGESIZE_Pos 0
#define SMC_CFG_PAGESIZE_Msk (0x3u << SMC_CFG_PAGESIZE_Pos) /**< \brief (SMC_CFG) Page Size of the NAND Flash Device */
#define   SMC_CFG_PAGESIZE_PS512 (0x0u << 0) /**< \brief (SMC_CFG) Main area 512 Bytes */
#define   SMC_CFG_PAGESIZE_PS1024 (0x1u << 0) /**< \brief (SMC_CFG) Main area 1024 Bytes */
#define   SMC_CFG_PAGESIZE_PS2048 (0x2u << 0) /**< \brief (SMC_CFG) Main area 2048 Bytes */
#define   SMC_CFG_PAGESIZE_PS4096 (0x3u << 0) /**< \brief (SMC_CFG) Main area 4096 Bytes */
#define SMC_CFG_WSPARE (0x1u << 8) /**< \brief (SMC_CFG) Write Spare Area */
#define SMC_CFG_RSPARE (0x1u << 9) /**< \brief (SMC_CFG) Read Spare Area */
#define SMC_CFG_EDGECTRL (0x1u << 12) /**< \brief (SMC_CFG) Rising/Falling Edge Detection Control */
#define SMC_CFG_RBEDGE (0x1u << 13) /**< \brief (SMC_CFG) Ready/Busy Signal Edge Detection */
#define SMC_CFG_DTOCYC_Pos 16
#define SMC_CFG_DTOCYC_Msk (0xfu << SMC_CFG_DTOCYC_Pos) /**< \brief (SMC_CFG) Data Timeout Cycle Number */
#define SMC_CFG_DTOCYC(value) ((SMC_CFG_DTOCYC_Msk & ((value) << SMC_CFG_DTOCYC_Pos)))
#define SMC_CFG_DTOMUL_Pos 20
#define SMC_CFG_DTOMUL_Msk (0x7u << SMC_CFG_DTOMUL_Pos) /**< \brief (SMC_CFG) Data Timeout Multiplier */
#define   SMC_CFG_DTOMUL_X1 (0x0u << 20) /**< \brief (SMC_CFG) DTOCYC */
#define   SMC_CFG_DTOMUL_X16 (0x1u << 20) /**< \brief (SMC_CFG) DTOCYC x 16 */
#define   SMC_CFG_DTOMUL_X128 (0x2u << 20) /**< \brief (SMC_CFG) DTOCYC x 128 */
#define   SMC_CFG_DTOMUL_X256 (0x3u << 20) /**< \brief (SMC_CFG) DTOCYC x 256 */
#define   SMC_CFG_DTOMUL_X1024 (0x4u << 20) /**< \brief (SMC_CFG) DTOCYC x 1024 */
#define   SMC_CFG_DTOMUL_X4096 (0x5u << 20) /**< \brief (SMC_CFG) DTOCYC x 4096 */
#define   SMC_CFG_DTOMUL_X65536 (0x6u << 20) /**< \brief (SMC_CFG) DTOCYC x 65536 */
#define   SMC_CFG_DTOMUL_X1048576 (0x7u << 20) /**< \brief (SMC_CFG) DTOCYC x 1048576 */
/* -------- SMC_CTRL : (SMC Offset: 0x004) SMC NFC Control Register -------- */
#define SMC_CTRL_NFCEN (0x1u << 0) /**< \brief (SMC_CTRL) NAND Flash Controller Enable */
#define SMC_CTRL_NFCDIS (0x1u << 1) /**< \brief (SMC_CTRL) NAND Flash Controller Disable */
/* -------- SMC_SR : (SMC Offset: 0x008) SMC NFC Status Register -------- */
#define SMC_SR_SMCSTS (0x1u << 0) /**< \brief (SMC_SR) NAND Flash Controller status (this field cannot be reset) */
#define SMC_SR_RB_RISE (0x1u << 4) /**< \brief (SMC_SR) Selected Ready Busy Rising Edge Detected */
#define SMC_SR_RB_FALL (0x1u << 5) /**< \brief (SMC_SR) Selected Ready Busy Falling Edge Detected */
#define SMC_SR_NFCBUSY (0x1u << 8) /**< \brief (SMC_SR) NFC Busy (this field cannot be reset) */
#define SMC_SR_NFCWR (0x1u << 11) /**< \brief (SMC_SR) NFC Write/Read Operation (this field cannot be reset) */
#define SMC_SR_NFCSID_Pos 12
#define SMC_SR_NFCSID_Msk (0x7u << SMC_SR_NFCSID_Pos) /**< \brief (SMC_SR) NFC Chip Select ID (this field cannot be reset) */
#define SMC_SR_XFRDONE (0x1u << 16) /**< \brief (SMC_SR) NFC Data Transfer Terminated */
#define SMC_SR_CMDDONE (0x1u << 17) /**< \brief (SMC_SR) Command Done */
#define SMC_SR_DTOE (0x1u << 20) /**< \brief (SMC_SR) Data Timeout Error */
#define SMC_SR_UNDEF (0x1u << 21) /**< \brief (SMC_SR) Undefined Area Error */
#define SMC_SR_AWB (0x1u << 22) /**< \brief (SMC_SR) Accessing While Busy */
#define SMC_SR_NFCASE (0x1u << 23) /**< \brief (SMC_SR) NFC Access Size Error */
#define SMC_SR_RB_EDGE0 (0x1u << 24) /**< \brief (SMC_SR) Ready/Busy Line 0 Edge Detected */
/* -------- SMC_IER : (SMC Offset: 0x00C) SMC NFC Interrupt Enable Register -------- */
#define SMC_IER_RB_RISE (0x1u << 4) /**< \brief (SMC_IER) Ready Busy Rising Edge Detection Interrupt Enable */
#define SMC_IER_RB_FALL (0x1u << 5) /**< \brief (SMC_IER) Ready Busy Falling Edge Detection Interrupt Enable */
#define SMC_IER_XFRDONE (0x1u << 16) /**< \brief (SMC_IER) Transfer Done Interrupt Enable */
#define SMC_IER_CMDDONE (0x1u << 17) /**< \brief (SMC_IER) Command Done Interrupt Enable */
#define SMC_IER_DTOE (0x1u << 20) /**< \brief (SMC_IER) Data Timeout Error Interrupt Enable */
#define SMC_IER_UNDEF (0x1u << 21) /**< \brief (SMC_IER) Undefined Area Access Interrupt Enable */
#define SMC_IER_AWB (0x1u << 22) /**< \brief (SMC_IER) Accessing While Busy Interrupt Enable */
#define SMC_IER_NFCASE (0x1u << 23) /**< \brief (SMC_IER) NFC Access Size Error Interrupt Enable */
#define SMC_IER_RB_EDGE0 (0x1u << 24) /**< \brief (SMC_IER) Ready/Busy Line 0 Interrupt Enable */
/* -------- SMC_IDR : (SMC Offset: 0x010) SMC NFC Interrupt Disable Register -------- */
#define SMC_IDR_RB_RISE (0x1u << 4) /**< \brief (SMC_IDR) Ready Busy Rising Edge Detection Interrupt Disable */
#define SMC_IDR_RB_FALL (0x1u << 5) /**< \brief (SMC_IDR) Ready Busy Falling Edge Detection Interrupt Disable */
#define SMC_IDR_XFRDONE (0x1u << 16) /**< \brief (SMC_IDR) Transfer Done Interrupt Disable */
#define SMC_IDR_CMDDONE (0x1u << 17) /**< \brief (SMC_IDR) Command Done Interrupt Disable */
#define SMC_IDR_DTOE (0x1u << 20) /**< \brief (SMC_IDR) Data Timeout Error Interrupt Disable */
#define SMC_IDR_UNDEF (0x1u << 21) /**< \brief (SMC_IDR) Undefined Area Access Interrupt Disable */
#define SMC_IDR_AWB (0x1u << 22) /**< \brief (SMC_IDR) Accessing While Busy Interrupt Disable */
#define SMC_IDR_NFCASE (0x1u << 23) /**< \brief (SMC_IDR) NFC Access Size Error Interrupt Disable */
#define SMC_IDR_RB_EDGE0 (0x1u << 24) /**< \brief (SMC_IDR) Ready/Busy Line 0 Interrupt Disable */
/* -------- SMC_IMR : (SMC Offset: 0x014) SMC NFC Interrupt Mask Register -------- */
#define SMC_IMR_RB_RISE (0x1u << 4) /**< \brief (SMC_IMR) Ready Busy Rising Edge Detection Interrupt Mask */
#define SMC_IMR_RB_FALL (0x1u << 5) /**< \brief (SMC_IMR) Ready Busy Falling Edge Detection Interrupt Mask */
#define SMC_IMR_XFRDONE (0x1u << 16) /**< \brief (SMC_IMR) Transfer Done Interrupt Mask */
#define SMC_IMR_CMDDONE (0x1u << 17) /**< \brief (SMC_IMR) Command Done Interrupt Mask */
#define SMC_IMR_DTOE (0x1u << 20) /**< \brief (SMC_IMR) Data Timeout Error Interrupt Mask */
#define SMC_IMR_UNDEF (0x1u << 21) /**< \brief (SMC_IMR) Undefined Area Access Interrupt Mask5 */
#define SMC_IMR_AWB (0x1u << 22) /**< \brief (SMC_IMR) Accessing While Busy Interrupt Mask */
#define SMC_IMR_NFCASE (0x1u << 23) /**< \brief (SMC_IMR) NFC Access Size Error Interrupt Mask */
#define SMC_IMR_RB_EDGE0 (0x1u << 24) /**< \brief (SMC_IMR) Ready/Busy Line 0 Interrupt Mask */
/* -------- SMC_ADDR : (SMC Offset: 0x018) SMC NFC Address Cycle Zero Register -------- */
#define SMC_ADDR_ADDR_CYCLE0_Pos 0
#define SMC_ADDR_ADDR_CYCLE0_Msk (0xffu << SMC_ADDR_ADDR_CYCLE0_Pos) /**< \brief (SMC_ADDR) NAND Flash Array Address cycle 0 */
#define SMC_ADDR_ADDR_CYCLE0(value) ((SMC_ADDR_ADDR_CYCLE0_Msk & ((value) << SMC_ADDR_ADDR_CYCLE0_Pos)))
/* -------- SMC_BANK : (SMC Offset: 0x01C) SMC Bank Address Register -------- */
#define SMC_BANK_BANK_Pos 0
#define SMC_BANK_BANK_Msk (0x7u << SMC_BANK_BANK_Pos) /**< \brief (SMC_BANK) Bank Identifier */
#define SMC_BANK_BANK(value) ((SMC_BANK_BANK_Msk & ((value) << SMC_BANK_BANK_Pos)))
/* -------- SMC_ECC_CTRL : (SMC Offset: 0x020) SMC ECC Control Register -------- */
#define SMC_ECC_CTRL_RST (0x1u << 0) /**< \brief (SMC_ECC_CTRL) Reset ECC */
#define SMC_ECC_CTRL_SWRST (0x1u << 1) /**< \brief (SMC_ECC_CTRL) Software Reset */
/* -------- SMC_ECC_MD : (SMC Offset: 0x024) SMC ECC Mode Register -------- */
#define SMC_ECC_MD_ECC_PAGESIZE_Pos 0
#define SMC_ECC_MD_ECC_PAGESIZE_Msk (0x3u << SMC_ECC_MD_ECC_PAGESIZE_Pos) /**< \brief (SMC_ECC_MD) ECC Page Size */
#define   SMC_ECC_MD_ECC_PAGESIZE_PS512 (0x0u << 0) /**< \brief (SMC_ECC_MD) Main area 512 Words */
#define   SMC_ECC_MD_ECC_PAGESIZE_PS1024 (0x1u << 0) /**< \brief (SMC_ECC_MD) Main area 1024 Words */
#define   SMC_ECC_MD_ECC_PAGESIZE_PS2048 (0x2u << 0) /**< \brief (SMC_ECC_MD) Main area 2048 Words */
#define   SMC_ECC_MD_ECC_PAGESIZE_PS4096 (0x3u << 0) /**< \brief (SMC_ECC_MD) Main area 4096 Words */
#define SMC_ECC_MD_TYPCORREC_Pos 4
#define SMC_ECC_MD_TYPCORREC_Msk (0x3u << SMC_ECC_MD_TYPCORREC_Pos) /**< \brief (SMC_ECC_MD) Type of Correction */
#define   SMC_ECC_MD_TYPCORREC_CPAGE (0x0u << 4) /**< \brief (SMC_ECC_MD) 1 bit correction for a page of 512/1024/2048/4096 Bytes  (for 8 or 16-bit NAND Flash) */
#define   SMC_ECC_MD_TYPCORREC_C256B (0x1u << 4) /**< \brief (SMC_ECC_MD) 1 bit correction for 256 Bytes of data for a page of 512/2048/4096 bytes (for 8-bit NAND Flash only) */
#define   SMC_ECC_MD_TYPCORREC_C512B (0x2u << 4) /**< \brief (SMC_ECC_MD) 1 bit correction for 512 Bytes of data for a page of 512/2048/4096 bytes (for 8-bit NAND Flash only) */
/* -------- SMC_ECC_SR1 : (SMC Offset: 0x028) SMC ECC Status 1 Register -------- */
#define SMC_ECC_SR1_RECERR0 (0x1u << 0) /**< \brief (SMC_ECC_SR1) Recoverable Error */
#define SMC_ECC_SR1_ECCERR0 (0x1u << 1) /**< \brief (SMC_ECC_SR1) ECC Error */
#define SMC_ECC_SR1_MULERR0 (0x1u << 2) /**< \brief (SMC_ECC_SR1) Multiple Error */
#define SMC_ECC_SR1_RECERR1 (0x1u << 4) /**< \brief (SMC_ECC_SR1) Recoverable Error in the page between the 256th and the 511th bytes or the 512nd and the 1023rd bytes */
#define SMC_ECC_SR1_ECCERR1 (0x1u << 5) /**< \brief (SMC_ECC_SR1) ECC Error in the page between the 256th and the 511th bytes or between the 512nd and the 1023rd bytes */
#define SMC_ECC_SR1_MULERR1 (0x1u << 6) /**< \brief (SMC_ECC_SR1) Multiple Error in the page between the 256th and the 511th bytes or between the 512nd and the 1023rd bytes */
#define SMC_ECC_SR1_RECERR2 (0x1u << 8) /**< \brief (SMC_ECC_SR1) Recoverable Error in the page between the 512nd and the 767th bytes or between the 1024th and the 1535th bytes */
#define SMC_ECC_SR1_ECCERR2 (0x1u << 9) /**< \brief (SMC_ECC_SR1) ECC Error in the page between the 512nd and the 767th bytes or between the 1024th and the 1535th bytes */
#define SMC_ECC_SR1_MULERR2 (0x1u << 10) /**< \brief (SMC_ECC_SR1) Multiple Error in the page between the 512nd and the 767th bytes or between the 1024th and the 1535th bytes */
#define SMC_ECC_SR1_RECERR3 (0x1u << 12) /**< \brief (SMC_ECC_SR1) Recoverable Error in the page between the 768th and the 1023rd bytes or between the 1536th and the 2047th bytes */
#define SMC_ECC_SR1_ECCERR3 (0x1u << 13) /**< \brief (SMC_ECC_SR1) ECC Error in the page between the 768th and the 1023rd bytes or between the 1536th and the 2047th bytes */
#define SMC_ECC_SR1_MULERR3 (0x1u << 14) /**< \brief (SMC_ECC_SR1) Multiple Error in the page between the 768th and the 1023rd bytes or between the 1536th and the 2047th bytes */
#define SMC_ECC_SR1_RECERR4 (0x1u << 16) /**< \brief (SMC_ECC_SR1) Recoverable Error in the page between the 1024th and the 1279th bytes or between the 2048th and the 2559th bytes */
#define SMC_ECC_SR1_ECCERR4 (0x1u << 17) /**< \brief (SMC_ECC_SR1) ECC Error in the page between the 1024th and the 1279th bytes or between the 2048th and the 2559th bytes */
#define SMC_ECC_SR1_MULERR4 (0x1u << 18) /**< \brief (SMC_ECC_SR1) Multiple Error in the page between the 1024th and the 1279th bytes or between the 2048th and the 2559th bytes */
#define SMC_ECC_SR1_RECERR5 (0x1u << 20) /**< \brief (SMC_ECC_SR1) Recoverable Error in the page between the 1280th and the 1535th bytes or between the 2560th and the 3071st bytes */
#define SMC_ECC_SR1_ECCERR5 (0x1u << 21) /**< \brief (SMC_ECC_SR1) ECC Error in the page between the 1280th and the 1535th bytes or between the 2560th and the 3071st bytes */
#define SMC_ECC_SR1_MULERR5 (0x1u << 22) /**< \brief (SMC_ECC_SR1) Multiple Error in the page between the 1280th and the 1535th bytes or between the 2560th and the 3071st bytes */
#define SMC_ECC_SR1_RECERR6 (0x1u << 24) /**< \brief (SMC_ECC_SR1) Recoverable Error in the page between the 1536th and the 1791st bytes or between the 3072nd and the 3583rd bytes */
#define SMC_ECC_SR1_ECCERR6 (0x1u << 25) /**< \brief (SMC_ECC_SR1) ECC Error in the page between the 1536th and the 1791st bytes or between the 3072nd and the 3583rd bytes */
#define SMC_ECC_SR1_MULERR6 (0x1u << 26) /**< \brief (SMC_ECC_SR1) Multiple Error in the page between the 1536th and the 1791st bytes or between the 3072nd and the 3583rd bytes */
#define SMC_ECC_SR1_RECERR7 (0x1u << 28) /**< \brief (SMC_ECC_SR1) Recoverable Error in the page between the 1792nd and the 2047th bytes or between the 3584th and the 4095th bytes */
#define SMC_ECC_SR1_ECCERR7 (0x1u << 29) /**< \brief (SMC_ECC_SR1) ECC Error in the page between the 1792nd and the 2047th bytes or between the 3584th and the 4095th bytes */
#define SMC_ECC_SR1_MULERR7 (0x1u << 30) /**< \brief (SMC_ECC_SR1) Multiple Error in the page between the 1792nd and the 2047th bytes or between the 3584th and the 4095th bytes */
/* -------- SMC_ECC_PR0 : (SMC Offset: 0x02C) SMC ECC Parity 0 Register -------- */
#define SMC_ECC_PR0_BITADDR_Pos 0
#define SMC_ECC_PR0_BITADDR_Msk (0xfu << SMC_ECC_PR0_BITADDR_Pos) /**< \brief (SMC_ECC_PR0) Bit Address */
#define SMC_ECC_PR0_WORDADDR_Pos 4
#define SMC_ECC_PR0_WORDADDR_Msk (0xfffu << SMC_ECC_PR0_WORDADDR_Pos) /**< \brief (SMC_ECC_PR0) Word Address */
#define SMC_ECC_PR0_BITADDR_W9BIT_Pos 0
#define SMC_ECC_PR0_BITADDR_W9BIT_Msk (0x7u << SMC_ECC_PR0_BITADDR_W9BIT_Pos) /**< \brief (SMC_ECC_PR0) Corrupted Bit Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR0_WORDADDR_W9BIT_Pos 3
#define SMC_ECC_PR0_WORDADDR_W9BIT_Msk (0x1ffu << SMC_ECC_PR0_WORDADDR_W9BIT_Pos) /**< \brief (SMC_ECC_PR0) Corrupted Word Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR0_NPARITY_Pos 12
#define SMC_ECC_PR0_NPARITY_Msk (0xfffu << SMC_ECC_PR0_NPARITY_Pos) /**< \brief (SMC_ECC_PR0) Parity N */
#define SMC_ECC_PR0_BITADDR_W8BIT_Pos 0
#define SMC_ECC_PR0_BITADDR_W8BIT_Msk (0x7u << SMC_ECC_PR0_BITADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR0) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR0_WORDADDR_W8BIT_Pos 3
#define SMC_ECC_PR0_WORDADDR_W8BIT_Msk (0xffu << SMC_ECC_PR0_WORDADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR0) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR0_NPARITY_W8BIT_Pos 12
#define SMC_ECC_PR0_NPARITY_W8BIT_Msk (0x7ffu << SMC_ECC_PR0_NPARITY_W8BIT_Pos) /**< \brief (SMC_ECC_PR0) Parity N */
/* -------- SMC_ECC_PR1 : (SMC Offset: 0x030) SMC ECC parity 1 Register -------- */
#define SMC_ECC_PR1_NPARITY_Pos 0
#define SMC_ECC_PR1_NPARITY_Msk (0xffffu << SMC_ECC_PR1_NPARITY_Pos) /**< \brief (SMC_ECC_PR1) Parity N */
#define SMC_ECC_PR1_BITADDR_Pos 0
#define SMC_ECC_PR1_BITADDR_Msk (0x7u << SMC_ECC_PR1_BITADDR_Pos) /**< \brief (SMC_ECC_PR1) Corrupted Bit Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR1_WORDADDR_Pos 3
#define SMC_ECC_PR1_WORDADDR_Msk (0x1ffu << SMC_ECC_PR1_WORDADDR_Pos) /**< \brief (SMC_ECC_PR1) Corrupted Word Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR1_NPARITY_W9BIT_Pos 12
#define SMC_ECC_PR1_NPARITY_W9BIT_Msk (0xfffu << SMC_ECC_PR1_NPARITY_W9BIT_Pos) /**< \brief (SMC_ECC_PR1) Parity N */
#define SMC_ECC_PR1_WORDADDR_W8BIT_Pos 3
#define SMC_ECC_PR1_WORDADDR_W8BIT_Msk (0xffu << SMC_ECC_PR1_WORDADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR1) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR1_NPARITY_W8BIT_Pos 12
#define SMC_ECC_PR1_NPARITY_W8BIT_Msk (0x7ffu << SMC_ECC_PR1_NPARITY_W8BIT_Pos) /**< \brief (SMC_ECC_PR1) Parity N */
/* -------- SMC_ECC_SR2 : (SMC Offset: 0x034) SMC ECC status 2 Register -------- */
#define SMC_ECC_SR2_RECERR8 (0x1u << 0) /**< \brief (SMC_ECC_SR2) Recoverable Error in the page between the 2048th and the 2303rd bytes */
#define SMC_ECC_SR2_ECCERR8 (0x1u << 1) /**< \brief (SMC_ECC_SR2) ECC Error in the page between the 2048th and the 2303rd bytes */
#define SMC_ECC_SR2_MULERR8 (0x1u << 2) /**< \brief (SMC_ECC_SR2) Multiple Error in the page between the 2048th and the 2303rd bytes */
#define SMC_ECC_SR2_RECERR9 (0x1u << 4) /**< \brief (SMC_ECC_SR2) Recoverable Error in the page between the 2304th and the 2559th bytes */
#define SMC_ECC_SR2_ECCERR9 (0x1u << 5) /**< \brief (SMC_ECC_SR2) ECC Error in the page between the 2304th and the 2559th bytes */
#define SMC_ECC_SR2_MULERR9 (0x1u << 6) /**< \brief (SMC_ECC_SR2) Multiple Error in the page between the 2304th and the 2559th bytes */
#define SMC_ECC_SR2_RECERR10 (0x1u << 8) /**< \brief (SMC_ECC_SR2) Recoverable Error in the page between the 2560th and the 2815th bytes */
#define SMC_ECC_SR2_ECCERR10 (0x1u << 9) /**< \brief (SMC_ECC_SR2) ECC Error in the page between the 2560th and the 2815th bytes */
#define SMC_ECC_SR2_MULERR10 (0x1u << 10) /**< \brief (SMC_ECC_SR2) Multiple Error in the page between the 2560th and the 2815th bytes */
#define SMC_ECC_SR2_RECERR11 (0x1u << 12) /**< \brief (SMC_ECC_SR2) Recoverable Error in the page between the 2816th and the 3071st bytes */
#define SMC_ECC_SR2_ECCERR11 (0x1u << 13) /**< \brief (SMC_ECC_SR2) ECC Error in the page between the 2816th and the 3071st bytes */
#define SMC_ECC_SR2_MULERR11 (0x1u << 14) /**< \brief (SMC_ECC_SR2) Multiple Error in the page between the 2816th and the 3071st bytes */
#define SMC_ECC_SR2_RECERR12 (0x1u << 16) /**< \brief (SMC_ECC_SR2) Recoverable Error in the page between the 3072nd and the 3327th bytes */
#define SMC_ECC_SR2_ECCERR12 (0x1u << 17) /**< \brief (SMC_ECC_SR2) ECC Error in the page between the 3072nd and the 3327th bytes */
#define SMC_ECC_SR2_MULERR12 (0x1u << 18) /**< \brief (SMC_ECC_SR2) Multiple Error in the page between the 3072nd and the 3327th bytes */
#define SMC_ECC_SR2_RECERR13 (0x1u << 20) /**< \brief (SMC_ECC_SR2) Recoverable Error in the page between the 3328th and the 3583rd bytes */
#define SMC_ECC_SR2_ECCERR13 (0x1u << 21) /**< \brief (SMC_ECC_SR2) ECC Error in the page between the 3328th and the 3583rd bytes */
#define SMC_ECC_SR2_MULERR13 (0x1u << 22) /**< \brief (SMC_ECC_SR2) Multiple Error in the page between the 3328th and the 3583rd bytes */
#define SMC_ECC_SR2_RECERR14 (0x1u << 24) /**< \brief (SMC_ECC_SR2) Recoverable Error in the page between the 3584th and the 3839th bytes */
#define SMC_ECC_SR2_ECCERR14 (0x1u << 25) /**< \brief (SMC_ECC_SR2) ECC Error in the page between the 3584th and the 3839th bytes */
#define SMC_ECC_SR2_MULERR14 (0x1u << 26) /**< \brief (SMC_ECC_SR2) Multiple Error in the page between the 3584th and the 3839th bytes */
#define SMC_ECC_SR2_RECERR15 (0x1u << 28) /**< \brief (SMC_ECC_SR2) Recoverable Error in the page between the 3840th and the 4095th bytes */
#define SMC_ECC_SR2_ECCERR15 (0x1u << 29) /**< \brief (SMC_ECC_SR2) ECC Error in the page between the 3840th and the 4095th bytes */
#define SMC_ECC_SR2_MULERR15 (0x1u << 30) /**< \brief (SMC_ECC_SR2) Multiple Error in the page between the 3840th and the 4095th bytes */
/* -------- SMC_ECC_PR2 : (SMC Offset: 0x038) SMC ECC parity 2 Register -------- */
#define SMC_ECC_PR2_BITADDR_Pos 0
#define SMC_ECC_PR2_BITADDR_Msk (0x7u << SMC_ECC_PR2_BITADDR_Pos) /**< \brief (SMC_ECC_PR2) Corrupted Bit Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR2_WORDADDR_Pos 3
#define SMC_ECC_PR2_WORDADDR_Msk (0x1ffu << SMC_ECC_PR2_WORDADDR_Pos) /**< \brief (SMC_ECC_PR2) Corrupted Word Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR2_NPARITY_Pos 12
#define SMC_ECC_PR2_NPARITY_Msk (0xfffu << SMC_ECC_PR2_NPARITY_Pos) /**< \brief (SMC_ECC_PR2) Parity N */
#define SMC_ECC_PR2_WORDADDR_W8BIT_Pos 3
#define SMC_ECC_PR2_WORDADDR_W8BIT_Msk (0xffu << SMC_ECC_PR2_WORDADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR2) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR2_NPARITY_W8BIT_Pos 12
#define SMC_ECC_PR2_NPARITY_W8BIT_Msk (0x7ffu << SMC_ECC_PR2_NPARITY_W8BIT_Pos) /**< \brief (SMC_ECC_PR2) Parity N */
/* -------- SMC_ECC_PR3 : (SMC Offset: 0x03C) SMC ECC parity 3 Register -------- */
#define SMC_ECC_PR3_BITADDR_Pos 0
#define SMC_ECC_PR3_BITADDR_Msk (0x7u << SMC_ECC_PR3_BITADDR_Pos) /**< \brief (SMC_ECC_PR3) Corrupted Bit Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR3_WORDADDR_Pos 3
#define SMC_ECC_PR3_WORDADDR_Msk (0x1ffu << SMC_ECC_PR3_WORDADDR_Pos) /**< \brief (SMC_ECC_PR3) Corrupted Word Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR3_NPARITY_Pos 12
#define SMC_ECC_PR3_NPARITY_Msk (0xfffu << SMC_ECC_PR3_NPARITY_Pos) /**< \brief (SMC_ECC_PR3) Parity N */
#define SMC_ECC_PR3_WORDADDR_W8BIT_Pos 3
#define SMC_ECC_PR3_WORDADDR_W8BIT_Msk (0xffu << SMC_ECC_PR3_WORDADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR3) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR3_NPARITY_W8BIT_Pos 12
#define SMC_ECC_PR3_NPARITY_W8BIT_Msk (0x7ffu << SMC_ECC_PR3_NPARITY_W8BIT_Pos) /**< \brief (SMC_ECC_PR3) Parity N */
/* -------- SMC_ECC_PR4 : (SMC Offset: 0x040) SMC ECC parity 4 Register -------- */
#define SMC_ECC_PR4_BITADDR_Pos 0
#define SMC_ECC_PR4_BITADDR_Msk (0x7u << SMC_ECC_PR4_BITADDR_Pos) /**< \brief (SMC_ECC_PR4) Corrupted Bit Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR4_WORDADDR_Pos 3
#define SMC_ECC_PR4_WORDADDR_Msk (0x1ffu << SMC_ECC_PR4_WORDADDR_Pos) /**< \brief (SMC_ECC_PR4) Corrupted Word Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR4_NPARITY_Pos 12
#define SMC_ECC_PR4_NPARITY_Msk (0xfffu << SMC_ECC_PR4_NPARITY_Pos) /**< \brief (SMC_ECC_PR4) Parity N */
#define SMC_ECC_PR4_WORDADDR_W8BIT_Pos 3
#define SMC_ECC_PR4_WORDADDR_W8BIT_Msk (0xffu << SMC_ECC_PR4_WORDADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR4) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR4_NPARITY_W8BIT_Pos 12
#define SMC_ECC_PR4_NPARITY_W8BIT_Msk (0x7ffu << SMC_ECC_PR4_NPARITY_W8BIT_Pos) /**< \brief (SMC_ECC_PR4) Parity N */
/* -------- SMC_ECC_PR5 : (SMC Offset: 0x044) SMC ECC parity 5 Register -------- */
#define SMC_ECC_PR5_BITADDR_Pos 0
#define SMC_ECC_PR5_BITADDR_Msk (0x7u << SMC_ECC_PR5_BITADDR_Pos) /**< \brief (SMC_ECC_PR5) Corrupted Bit Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR5_WORDADDR_Pos 3
#define SMC_ECC_PR5_WORDADDR_Msk (0x1ffu << SMC_ECC_PR5_WORDADDR_Pos) /**< \brief (SMC_ECC_PR5) Corrupted Word Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR5_NPARITY_Pos 12
#define SMC_ECC_PR5_NPARITY_Msk (0xfffu << SMC_ECC_PR5_NPARITY_Pos) /**< \brief (SMC_ECC_PR5) Parity N */
#define SMC_ECC_PR5_WORDADDR_W8BIT_Pos 3
#define SMC_ECC_PR5_WORDADDR_W8BIT_Msk (0xffu << SMC_ECC_PR5_WORDADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR5) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR5_NPARITY_W8BIT_Pos 12
#define SMC_ECC_PR5_NPARITY_W8BIT_Msk (0x7ffu << SMC_ECC_PR5_NPARITY_W8BIT_Pos) /**< \brief (SMC_ECC_PR5) Parity N */
/* -------- SMC_ECC_PR6 : (SMC Offset: 0x048) SMC ECC parity 6 Register -------- */
#define SMC_ECC_PR6_BITADDR_Pos 0
#define SMC_ECC_PR6_BITADDR_Msk (0x7u << SMC_ECC_PR6_BITADDR_Pos) /**< \brief (SMC_ECC_PR6) Corrupted Bit Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR6_WORDADDR_Pos 3
#define SMC_ECC_PR6_WORDADDR_Msk (0x1ffu << SMC_ECC_PR6_WORDADDR_Pos) /**< \brief (SMC_ECC_PR6) Corrupted Word Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR6_NPARITY_Pos 12
#define SMC_ECC_PR6_NPARITY_Msk (0xfffu << SMC_ECC_PR6_NPARITY_Pos) /**< \brief (SMC_ECC_PR6) Parity N */
#define SMC_ECC_PR6_WORDADDR_W8BIT_Pos 3
#define SMC_ECC_PR6_WORDADDR_W8BIT_Msk (0xffu << SMC_ECC_PR6_WORDADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR6) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR6_NPARITY_W8BIT_Pos 12
#define SMC_ECC_PR6_NPARITY_W8BIT_Msk (0x7ffu << SMC_ECC_PR6_NPARITY_W8BIT_Pos) /**< \brief (SMC_ECC_PR6) Parity N */
/* -------- SMC_ECC_PR7 : (SMC Offset: 0x04C) SMC ECC parity 7 Register -------- */
#define SMC_ECC_PR7_BITADDR_Pos 0
#define SMC_ECC_PR7_BITADDR_Msk (0x7u << SMC_ECC_PR7_BITADDR_Pos) /**< \brief (SMC_ECC_PR7) Corrupted Bit Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR7_WORDADDR_Pos 3
#define SMC_ECC_PR7_WORDADDR_Msk (0x1ffu << SMC_ECC_PR7_WORDADDR_Pos) /**< \brief (SMC_ECC_PR7) Corrupted Word Address in the Page between (i x 512) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR7_NPARITY_Pos 12
#define SMC_ECC_PR7_NPARITY_Msk (0xfffu << SMC_ECC_PR7_NPARITY_Pos) /**< \brief (SMC_ECC_PR7) Parity N */
#define SMC_ECC_PR7_WORDADDR_W8BIT_Pos 3
#define SMC_ECC_PR7_WORDADDR_W8BIT_Msk (0xffu << SMC_ECC_PR7_WORDADDR_W8BIT_Pos) /**< \brief (SMC_ECC_PR7) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR7_NPARITY_W8BIT_Pos 12
#define SMC_ECC_PR7_NPARITY_W8BIT_Msk (0x7ffu << SMC_ECC_PR7_NPARITY_W8BIT_Pos) /**< \brief (SMC_ECC_PR7) Parity N */
/* -------- SMC_ECC_PR8 : (SMC Offset: 0x050) SMC ECC parity 8 Register -------- */
#define SMC_ECC_PR8_BITADDR_Pos 0
#define SMC_ECC_PR8_BITADDR_Msk (0x7u << SMC_ECC_PR8_BITADDR_Pos) /**< \brief (SMC_ECC_PR8) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR8_WORDADDR_Pos 3
#define SMC_ECC_PR8_WORDADDR_Msk (0xffu << SMC_ECC_PR8_WORDADDR_Pos) /**< \brief (SMC_ECC_PR8) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR8_NPARITY_Pos 12
#define SMC_ECC_PR8_NPARITY_Msk (0x7ffu << SMC_ECC_PR8_NPARITY_Pos) /**< \brief (SMC_ECC_PR8) Parity N */
/* -------- SMC_ECC_PR9 : (SMC Offset: 0x054) SMC ECC parity 9 Register -------- */
#define SMC_ECC_PR9_BITADDR_Pos 0
#define SMC_ECC_PR9_BITADDR_Msk (0x7u << SMC_ECC_PR9_BITADDR_Pos) /**< \brief (SMC_ECC_PR9) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR9_WORDADDR_Pos 3
#define SMC_ECC_PR9_WORDADDR_Msk (0xffu << SMC_ECC_PR9_WORDADDR_Pos) /**< \brief (SMC_ECC_PR9) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR9_NPARITY_Pos 12
#define SMC_ECC_PR9_NPARITY_Msk (0x7ffu << SMC_ECC_PR9_NPARITY_Pos) /**< \brief (SMC_ECC_PR9) Parity N */
/* -------- SMC_ECC_PR10 : (SMC Offset: 0x058) SMC ECC parity 10 Register -------- */
#define SMC_ECC_PR10_BITADDR_Pos 0
#define SMC_ECC_PR10_BITADDR_Msk (0x7u << SMC_ECC_PR10_BITADDR_Pos) /**< \brief (SMC_ECC_PR10) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR10_WORDADDR_Pos 3
#define SMC_ECC_PR10_WORDADDR_Msk (0xffu << SMC_ECC_PR10_WORDADDR_Pos) /**< \brief (SMC_ECC_PR10) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR10_NPARITY_Pos 12
#define SMC_ECC_PR10_NPARITY_Msk (0x7ffu << SMC_ECC_PR10_NPARITY_Pos) /**< \brief (SMC_ECC_PR10) Parity N */
/* -------- SMC_ECC_PR11 : (SMC Offset: 0x05C) SMC ECC parity 11 Register -------- */
#define SMC_ECC_PR11_BITADDR_Pos 0
#define SMC_ECC_PR11_BITADDR_Msk (0x7u << SMC_ECC_PR11_BITADDR_Pos) /**< \brief (SMC_ECC_PR11) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR11_WORDADDR_Pos 3
#define SMC_ECC_PR11_WORDADDR_Msk (0xffu << SMC_ECC_PR11_WORDADDR_Pos) /**< \brief (SMC_ECC_PR11) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR11_NPARITY_Pos 12
#define SMC_ECC_PR11_NPARITY_Msk (0x7ffu << SMC_ECC_PR11_NPARITY_Pos) /**< \brief (SMC_ECC_PR11) Parity N */
/* -------- SMC_ECC_PR12 : (SMC Offset: 0x060) SMC ECC parity 12 Register -------- */
#define SMC_ECC_PR12_BITADDR_Pos 0
#define SMC_ECC_PR12_BITADDR_Msk (0x7u << SMC_ECC_PR12_BITADDR_Pos) /**< \brief (SMC_ECC_PR12) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR12_WORDADDR_Pos 3
#define SMC_ECC_PR12_WORDADDR_Msk (0xffu << SMC_ECC_PR12_WORDADDR_Pos) /**< \brief (SMC_ECC_PR12) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR12_NPARITY_Pos 12
#define SMC_ECC_PR12_NPARITY_Msk (0x7ffu << SMC_ECC_PR12_NPARITY_Pos) /**< \brief (SMC_ECC_PR12) Parity N */
/* -------- SMC_ECC_PR13 : (SMC Offset: 0x064) SMC ECC parity 13 Register -------- */
#define SMC_ECC_PR13_BITADDR_Pos 0
#define SMC_ECC_PR13_BITADDR_Msk (0x7u << SMC_ECC_PR13_BITADDR_Pos) /**< \brief (SMC_ECC_PR13) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR13_WORDADDR_Pos 3
#define SMC_ECC_PR13_WORDADDR_Msk (0xffu << SMC_ECC_PR13_WORDADDR_Pos) /**< \brief (SMC_ECC_PR13) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR13_NPARITY_Pos 12
#define SMC_ECC_PR13_NPARITY_Msk (0x7ffu << SMC_ECC_PR13_NPARITY_Pos) /**< \brief (SMC_ECC_PR13) Parity N */
/* -------- SMC_ECC_PR14 : (SMC Offset: 0x068) SMC ECC parity 14 Register -------- */
#define SMC_ECC_PR14_BITADDR_Pos 0
#define SMC_ECC_PR14_BITADDR_Msk (0x7u << SMC_ECC_PR14_BITADDR_Pos) /**< \brief (SMC_ECC_PR14) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR14_WORDADDR_Pos 3
#define SMC_ECC_PR14_WORDADDR_Msk (0xffu << SMC_ECC_PR14_WORDADDR_Pos) /**< \brief (SMC_ECC_PR14) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR14_NPARITY_Pos 12
#define SMC_ECC_PR14_NPARITY_Msk (0x7ffu << SMC_ECC_PR14_NPARITY_Pos) /**< \brief (SMC_ECC_PR14) Parity N */
/* -------- SMC_ECC_PR15 : (SMC Offset: 0x06C) SMC ECC parity 15 Register -------- */
#define SMC_ECC_PR15_BITADDR_Pos 0
#define SMC_ECC_PR15_BITADDR_Msk (0x7u << SMC_ECC_PR15_BITADDR_Pos) /**< \brief (SMC_ECC_PR15) Corrupted Bit Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR15_WORDADDR_Pos 3
#define SMC_ECC_PR15_WORDADDR_Msk (0xffu << SMC_ECC_PR15_WORDADDR_Pos) /**< \brief (SMC_ECC_PR15) Corrupted Word Address in the Page between (i x 256) and ((i + 1) x 512) - 1) Bytes */
#define SMC_ECC_PR15_NPARITY_Pos 12
#define SMC_ECC_PR15_NPARITY_Msk (0x7ffu << SMC_ECC_PR15_NPARITY_Pos) /**< \brief (SMC_ECC_PR15) Parity N */
/* -------- SMC_SETUP : (SMC Offset: N/A) SMC Setup Register -------- */
#define SMC_SETUP_NWE_SETUP_Pos 0
#define SMC_SETUP_NWE_SETUP_Msk (0x3fu << SMC_SETUP_NWE_SETUP_Pos) /**< \brief (SMC_SETUP) NWE Setup Length */
#define SMC_SETUP_NWE_SETUP(value) ((SMC_SETUP_NWE_SETUP_Msk & ((value) << SMC_SETUP_NWE_SETUP_Pos)))
#define SMC_SETUP_NCS_WR_SETUP_Pos 8
#define SMC_SETUP_NCS_WR_SETUP_Msk (0x3fu << SMC_SETUP_NCS_WR_SETUP_Pos) /**< \brief (SMC_SETUP) NCS Setup Length in Write Access */
#define SMC_SETUP_NCS_WR_SETUP(value) ((SMC_SETUP_NCS_WR_SETUP_Msk & ((value) << SMC_SETUP_NCS_WR_SETUP_Pos)))
#define SMC_SETUP_NRD_SETUP_Pos 16
#define SMC_SETUP_NRD_SETUP_Msk (0x3fu << SMC_SETUP_NRD_SETUP_Pos) /**< \brief (SMC_SETUP) NRD Setup Length */
#define SMC_SETUP_NRD_SETUP(value) ((SMC_SETUP_NRD_SETUP_Msk & ((value) << SMC_SETUP_NRD_SETUP_Pos)))
#define SMC_SETUP_NCS_RD_SETUP_Pos 24
#define SMC_SETUP_NCS_RD_SETUP_Msk (0x3fu << SMC_SETUP_NCS_RD_SETUP_Pos) /**< \brief (SMC_SETUP) NCS Setup Length in Read Access */
#define SMC_SETUP_NCS_RD_SETUP(value) ((SMC_SETUP_NCS_RD_SETUP_Msk & ((value) << SMC_SETUP_NCS_RD_SETUP_Pos)))
/* -------- SMC_PULSE : (SMC Offset: N/A) SMC Pulse Register -------- */
#define SMC_PULSE_NWE_PULSE_Pos 0
#define SMC_PULSE_NWE_PULSE_Msk (0x7fu << SMC_PULSE_NWE_PULSE_Pos) /**< \brief (SMC_PULSE) NWE Pulse Length */
#define SMC_PULSE_NWE_PULSE(value) ((SMC_PULSE_NWE_PULSE_Msk & ((value) << SMC_PULSE_NWE_PULSE_Pos)))
#define SMC_PULSE_NCS_WR_PULSE_Pos 8
#define SMC_PULSE_NCS_WR_PULSE_Msk (0x7fu << SMC_PULSE_NCS_WR_PULSE_Pos) /**< \brief (SMC_PULSE) NCS Pulse Length in WRITE Access */
#define SMC_PULSE_NCS_WR_PULSE(value) ((SMC_PULSE_NCS_WR_PULSE_Msk & ((value) << SMC_PULSE_NCS_WR_PULSE_Pos)))
#define SMC_PULSE_NRD_PULSE_Pos 16
#define SMC_PULSE_NRD_PULSE_Msk (0x7fu << SMC_PULSE_NRD_PULSE_Pos) /**< \brief (SMC_PULSE) NRD Pulse Length */
#define SMC_PULSE_NRD_PULSE(value) ((SMC_PULSE_NRD_PULSE_Msk & ((value) << SMC_PULSE_NRD_PULSE_Pos)))
#define SMC_PULSE_NCS_RD_PULSE_Pos 24
#define SMC_PULSE_NCS_RD_PULSE_Msk (0x7fu << SMC_PULSE_NCS_RD_PULSE_Pos) /**< \brief (SMC_PULSE) NCS Pulse Length in READ Access */
#define SMC_PULSE_NCS_RD_PULSE(value) ((SMC_PULSE_NCS_RD_PULSE_Msk & ((value) << SMC_PULSE_NCS_RD_PULSE_Pos)))
/* -------- SMC_CYCLE : (SMC Offset: N/A) SMC Cycle Register -------- */
#define SMC_CYCLE_NWE_CYCLE_Pos 0
#define SMC_CYCLE_NWE_CYCLE_Msk (0x1ffu << SMC_CYCLE_NWE_CYCLE_Pos) /**< \brief (SMC_CYCLE) Total Write Cycle Length */
#define SMC_CYCLE_NWE_CYCLE(value) ((SMC_CYCLE_NWE_CYCLE_Msk & ((value) << SMC_CYCLE_NWE_CYCLE_Pos)))
#define SMC_CYCLE_NRD_CYCLE_Pos 16
#define SMC_CYCLE_NRD_CYCLE_Msk (0x1ffu << SMC_CYCLE_NRD_CYCLE_Pos) /**< \brief (SMC_CYCLE) Total Read Cycle Length */
#define SMC_CYCLE_NRD_CYCLE(value) ((SMC_CYCLE_NRD_CYCLE_Msk & ((value) << SMC_CYCLE_NRD_CYCLE_Pos)))
/* -------- SMC_TIMINGS : (SMC Offset: N/A) SMC Timings Register -------- */
#define SMC_TIMINGS_TCLR_Pos 0
#define SMC_TIMINGS_TCLR_Msk (0xfu << SMC_TIMINGS_TCLR_Pos) /**< \brief (SMC_TIMINGS) CLE to REN Low Delay */
#define SMC_TIMINGS_TCLR(value) ((SMC_TIMINGS_TCLR_Msk & ((value) << SMC_TIMINGS_TCLR_Pos)))
#define SMC_TIMINGS_TADL_Pos 4
#define SMC_TIMINGS_TADL_Msk (0xfu << SMC_TIMINGS_TADL_Pos) /**< \brief (SMC_TIMINGS) ALE to Data Start */
#define SMC_TIMINGS_TADL(value) ((SMC_TIMINGS_TADL_Msk & ((value) << SMC_TIMINGS_TADL_Pos)))
#define SMC_TIMINGS_TAR_Pos 8
#define SMC_TIMINGS_TAR_Msk (0xfu << SMC_TIMINGS_TAR_Pos) /**< \brief (SMC_TIMINGS) ALE to REN Low Delay */
#define SMC_TIMINGS_TAR(value) ((SMC_TIMINGS_TAR_Msk & ((value) << SMC_TIMINGS_TAR_Pos)))
#define SMC_TIMINGS_OCMS (0x1u << 12) /**< \brief (SMC_TIMINGS) Off Chip Memory Scrambling Enable */
#define SMC_TIMINGS_TRR_Pos 16
#define SMC_TIMINGS_TRR_Msk (0xfu << SMC_TIMINGS_TRR_Pos) /**< \brief (SMC_TIMINGS) Ready to REN Low Delay */
#define SMC_TIMINGS_TRR(value) ((SMC_TIMINGS_TRR_Msk & ((value) << SMC_TIMINGS_TRR_Pos)))
#define SMC_TIMINGS_TWB_Pos 24
#define SMC_TIMINGS_TWB_Msk (0xfu << SMC_TIMINGS_TWB_Pos) /**< \brief (SMC_TIMINGS) WEN High to REN to Busy */
#define SMC_TIMINGS_TWB(value) ((SMC_TIMINGS_TWB_Msk & ((value) << SMC_TIMINGS_TWB_Pos)))
#define SMC_TIMINGS_RBNSEL_Pos 28
#define SMC_TIMINGS_RBNSEL_Msk (0x7u << SMC_TIMINGS_RBNSEL_Pos) /**< \brief (SMC_TIMINGS) Ready/Busy Line Selection */
#define SMC_TIMINGS_RBNSEL(value) ((SMC_TIMINGS_RBNSEL_Msk & ((value) << SMC_TIMINGS_RBNSEL_Pos)))
#define SMC_TIMINGS_NFSEL (0x1u << 31) /**< \brief (SMC_TIMINGS) NAND Flash Selection */
/* -------- SMC_MODE : (SMC Offset: N/A) SMC Mode Register -------- */
#define SMC_MODE_READ_MODE (0x1u << 0) /**< \brief (SMC_MODE) Selection of the Control Signal for Read Operation */
#define   SMC_MODE_READ_MODE_NCS_CTRL (0x0u << 0) /**< \brief (SMC_MODE) The Read operation is controlled by the NCS signal. */
#define   SMC_MODE_READ_MODE_NRD_CTRL (0x1u << 0) /**< \brief (SMC_MODE) The Read operation is controlled by the NRD signal. */
#define SMC_MODE_WRITE_MODE (0x1u << 1) /**< \brief (SMC_MODE) Selection of the Control Signal for Write Operation */
#define   SMC_MODE_WRITE_MODE_NCS_CTRL (0x0u << 1) /**< \brief (SMC_MODE) The Write operation is controller by the NCS signal. */
#define   SMC_MODE_WRITE_MODE_NWE_CTRL (0x1u << 1) /**< \brief (SMC_MODE) The Write operation is controlled by the NWE signal. */
#define SMC_MODE_EXNW_MODE_Pos 4
#define SMC_MODE_EXNW_MODE_Msk (0x3u << SMC_MODE_EXNW_MODE_Pos) /**< \brief (SMC_MODE) NWAIT Mode */
#define   SMC_MODE_EXNW_MODE_DISABLED (0x0u << 4) /**< \brief (SMC_MODE) Disabled */
#define   SMC_MODE_EXNW_MODE_FROZEN (0x2u << 4) /**< \brief (SMC_MODE) Frozen Mode */
#define   SMC_MODE_EXNW_MODE_READY (0x3u << 4) /**< \brief (SMC_MODE) Ready Mode */
#define SMC_MODE_BAT (0x1u << 8) /**< \brief (SMC_MODE) Byte Access Type */
#define SMC_MODE_DBW (0x1u << 12) /**< \brief (SMC_MODE) Data Bus Width */
#define   SMC_MODE_DBW_BIT_8 (0x0u << 12) /**< \brief (SMC_MODE) 8-bit bus */
#define   SMC_MODE_DBW_BIT_16 (0x1u << 12) /**< \brief (SMC_MODE) 16-bit bus */
#define SMC_MODE_TDF_CYCLES_Pos 16
#define SMC_MODE_TDF_CYCLES_Msk (0xfu << SMC_MODE_TDF_CYCLES_Pos) /**< \brief (SMC_MODE) Data Float Time */
#define SMC_MODE_TDF_CYCLES(value) ((SMC_MODE_TDF_CYCLES_Msk & ((value) << SMC_MODE_TDF_CYCLES_Pos)))
#define SMC_MODE_TDF_MODE (0x1u << 20) /**< \brief (SMC_MODE) TDF Optimization */
/* -------- SMC_OCMS : (SMC Offset: 0x110) SMC OCMS Register -------- */
#define SMC_OCMS_SMSE (0x1u << 0) /**< \brief (SMC_OCMS) Static Memory Controller Scrambling Enable */
#define SMC_OCMS_SRSE (0x1u << 1) /**< \brief (SMC_OCMS) SRAM Scrambling Enable */
/* -------- SMC_KEY1 : (SMC Offset: 0x114) SMC OCMS KEY1 Register -------- */
#define SMC_KEY1_KEY1_Pos 0
#define SMC_KEY1_KEY1_Msk (0xffffffffu << SMC_KEY1_KEY1_Pos) /**< \brief (SMC_KEY1) Off Chip Memory Scrambling (OCMS) Key Part 1 */
#define SMC_KEY1_KEY1(value) ((SMC_KEY1_KEY1_Msk & ((value) << SMC_KEY1_KEY1_Pos)))
/* -------- SMC_KEY2 : (SMC Offset: 0x118) SMC OCMS KEY2 Register -------- */
#define SMC_KEY2_KEY2_Pos 0
#define SMC_KEY2_KEY2_Msk (0xffffffffu << SMC_KEY2_KEY2_Pos) /**< \brief (SMC_KEY2) Off Chip Memory Scrambling (OCMS) Key Part 2 */
#define SMC_KEY2_KEY2(value) ((SMC_KEY2_KEY2_Msk & ((value) << SMC_KEY2_KEY2_Pos)))
/* -------- SMC_WPCR : (SMC Offset: 0x1E4) Write Protection Control Register -------- */
#define SMC_WPCR_WP_EN (0x1u << 0) /**< \brief (SMC_WPCR) Write Protection Enable */
#define SMC_WPCR_WP_KEY_Pos 8
#define SMC_WPCR_WP_KEY_Msk (0xffffffu << SMC_WPCR_WP_KEY_Pos) /**< \brief (SMC_WPCR) Write Protection KEY Password */
#define   SMC_WPCR_WP_KEY_PASSWD (0x534D43u << 8) /**< \brief (SMC_WPCR) Writing any other value in this field aborts the write operation of the WP_EN bit. Always reads as 0. */
/* -------- SMC_WPSR : (SMC Offset: 0x1E8) Write Protection Status Register -------- */
#define SMC_WPSR_WP_VS_Pos 0
#define SMC_WPSR_WP_VS_Msk (0xfu << SMC_WPSR_WP_VS_Pos) /**< \brief (SMC_WPSR) Write Protection Violation Status */
#define SMC_WPSR_WP_VSRC_Pos 8
#define SMC_WPSR_WP_VSRC_Msk (0xffffu << SMC_WPSR_WP_VSRC_Pos) /**< \brief (SMC_WPSR) Write Protection Violation Source */

/*@}*/


#endif /* _SAM3XA_SMC_COMPONENT_ */
