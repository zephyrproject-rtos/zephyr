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

#ifndef _SAM4S_HSMCI_COMPONENT_
#define _SAM4S_HSMCI_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR High Speed MultiMedia Card Interface */
/* ============================================================================= */
/** \addtogroup SAM4S_HSMCI High Speed MultiMedia Card Interface */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Hsmci hardware registers */
typedef struct {
  __O  uint32_t HSMCI_CR;        /**< \brief (Hsmci Offset: 0x00) Control Register */
  __IO uint32_t HSMCI_MR;        /**< \brief (Hsmci Offset: 0x04) Mode Register */
  __IO uint32_t HSMCI_DTOR;      /**< \brief (Hsmci Offset: 0x08) Data Timeout Register */
  __IO uint32_t HSMCI_SDCR;      /**< \brief (Hsmci Offset: 0x0C) SD/SDIO Card Register */
  __IO uint32_t HSMCI_ARGR;      /**< \brief (Hsmci Offset: 0x10) Argument Register */
  __O  uint32_t HSMCI_CMDR;      /**< \brief (Hsmci Offset: 0x14) Command Register */
  __IO uint32_t HSMCI_BLKR;      /**< \brief (Hsmci Offset: 0x18) Block Register */
  __IO uint32_t HSMCI_CSTOR;     /**< \brief (Hsmci Offset: 0x1C) Completion Signal Timeout Register */
  __I  uint32_t HSMCI_RSPR[4];   /**< \brief (Hsmci Offset: 0x20) Response Register */
  __I  uint32_t HSMCI_RDR;       /**< \brief (Hsmci Offset: 0x30) Receive Data Register */
  __O  uint32_t HSMCI_TDR;       /**< \brief (Hsmci Offset: 0x34) Transmit Data Register */
  __I  uint32_t Reserved1[2];
  __I  uint32_t HSMCI_SR;        /**< \brief (Hsmci Offset: 0x40) Status Register */
  __O  uint32_t HSMCI_IER;       /**< \brief (Hsmci Offset: 0x44) Interrupt Enable Register */
  __O  uint32_t HSMCI_IDR;       /**< \brief (Hsmci Offset: 0x48) Interrupt Disable Register */
  __I  uint32_t HSMCI_IMR;       /**< \brief (Hsmci Offset: 0x4C) Interrupt Mask Register */
  __I  uint32_t Reserved2[1];
  __IO uint32_t HSMCI_CFG;       /**< \brief (Hsmci Offset: 0x54) Configuration Register */
  __I  uint32_t Reserved3[35];
  __IO uint32_t HSMCI_WPMR;      /**< \brief (Hsmci Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t HSMCI_WPSR;      /**< \brief (Hsmci Offset: 0xE8) Write Protection Status Register */
  __I  uint32_t Reserved4[5];
  __IO uint32_t HSMCI_RPR;       /**< \brief (Hsmci Offset: 0x100) Receive Pointer Register */
  __IO uint32_t HSMCI_RCR;       /**< \brief (Hsmci Offset: 0x104) Receive Counter Register */
  __IO uint32_t HSMCI_TPR;       /**< \brief (Hsmci Offset: 0x108) Transmit Pointer Register */
  __IO uint32_t HSMCI_TCR;       /**< \brief (Hsmci Offset: 0x10C) Transmit Counter Register */
  __IO uint32_t HSMCI_RNPR;      /**< \brief (Hsmci Offset: 0x110) Receive Next Pointer Register */
  __IO uint32_t HSMCI_RNCR;      /**< \brief (Hsmci Offset: 0x114) Receive Next Counter Register */
  __IO uint32_t HSMCI_TNPR;      /**< \brief (Hsmci Offset: 0x118) Transmit Next Pointer Register */
  __IO uint32_t HSMCI_TNCR;      /**< \brief (Hsmci Offset: 0x11C) Transmit Next Counter Register */
  __O  uint32_t HSMCI_PTCR;      /**< \brief (Hsmci Offset: 0x120) Transfer Control Register */
  __I  uint32_t HSMCI_PTSR;      /**< \brief (Hsmci Offset: 0x124) Transfer Status Register */
  __I  uint32_t Reserved5[54];
  __IO uint32_t HSMCI_FIFO[256]; /**< \brief (Hsmci Offset: 0x200) FIFO Memory Aperture0 */
} Hsmci;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- HSMCI_CR : (HSMCI Offset: 0x00) Control Register -------- */
#define HSMCI_CR_MCIEN (0x1u << 0) /**< \brief (HSMCI_CR) Multi-Media Interface Enable */
#define HSMCI_CR_MCIDIS (0x1u << 1) /**< \brief (HSMCI_CR) Multi-Media Interface Disable */
#define HSMCI_CR_PWSEN (0x1u << 2) /**< \brief (HSMCI_CR) Power Save Mode Enable */
#define HSMCI_CR_PWSDIS (0x1u << 3) /**< \brief (HSMCI_CR) Power Save Mode Disable */
#define HSMCI_CR_SWRST (0x1u << 7) /**< \brief (HSMCI_CR) Software Reset */
/* -------- HSMCI_MR : (HSMCI Offset: 0x04) Mode Register -------- */
#define HSMCI_MR_CLKDIV_Pos 0
#define HSMCI_MR_CLKDIV_Msk (0xffu << HSMCI_MR_CLKDIV_Pos) /**< \brief (HSMCI_MR) Clock Divider */
#define HSMCI_MR_CLKDIV(value) ((HSMCI_MR_CLKDIV_Msk & ((value) << HSMCI_MR_CLKDIV_Pos)))
#define HSMCI_MR_PWSDIV_Pos 8
#define HSMCI_MR_PWSDIV_Msk (0x7u << HSMCI_MR_PWSDIV_Pos) /**< \brief (HSMCI_MR) Power Saving Divider */
#define HSMCI_MR_PWSDIV(value) ((HSMCI_MR_PWSDIV_Msk & ((value) << HSMCI_MR_PWSDIV_Pos)))
#define HSMCI_MR_RDPROOF (0x1u << 11) /**< \brief (HSMCI_MR) Read Proof Enable */
#define HSMCI_MR_WRPROOF (0x1u << 12) /**< \brief (HSMCI_MR) Write Proof Enable */
#define HSMCI_MR_FBYTE (0x1u << 13) /**< \brief (HSMCI_MR) Force Byte Transfer */
#define HSMCI_MR_PADV (0x1u << 14) /**< \brief (HSMCI_MR) Padding Value */
#define HSMCI_MR_PDCMODE (0x1u << 15) /**< \brief (HSMCI_MR) PDC-oriented Mode */
/* -------- HSMCI_DTOR : (HSMCI Offset: 0x08) Data Timeout Register -------- */
#define HSMCI_DTOR_DTOCYC_Pos 0
#define HSMCI_DTOR_DTOCYC_Msk (0xfu << HSMCI_DTOR_DTOCYC_Pos) /**< \brief (HSMCI_DTOR) Data Timeout Cycle Number */
#define HSMCI_DTOR_DTOCYC(value) ((HSMCI_DTOR_DTOCYC_Msk & ((value) << HSMCI_DTOR_DTOCYC_Pos)))
#define HSMCI_DTOR_DTOMUL_Pos 4
#define HSMCI_DTOR_DTOMUL_Msk (0x7u << HSMCI_DTOR_DTOMUL_Pos) /**< \brief (HSMCI_DTOR) Data Timeout Multiplier */
#define HSMCI_DTOR_DTOMUL(value) ((HSMCI_DTOR_DTOMUL_Msk & ((value) << HSMCI_DTOR_DTOMUL_Pos)))
#define   HSMCI_DTOR_DTOMUL_1 (0x0u << 4) /**< \brief (HSMCI_DTOR) DTOCYC */
#define   HSMCI_DTOR_DTOMUL_16 (0x1u << 4) /**< \brief (HSMCI_DTOR) DTOCYC x 16 */
#define   HSMCI_DTOR_DTOMUL_128 (0x2u << 4) /**< \brief (HSMCI_DTOR) DTOCYC x 128 */
#define   HSMCI_DTOR_DTOMUL_256 (0x3u << 4) /**< \brief (HSMCI_DTOR) DTOCYC x 256 */
#define   HSMCI_DTOR_DTOMUL_1024 (0x4u << 4) /**< \brief (HSMCI_DTOR) DTOCYC x 1024 */
#define   HSMCI_DTOR_DTOMUL_4096 (0x5u << 4) /**< \brief (HSMCI_DTOR) DTOCYC x 4096 */
#define   HSMCI_DTOR_DTOMUL_65536 (0x6u << 4) /**< \brief (HSMCI_DTOR) DTOCYC x 65536 */
#define   HSMCI_DTOR_DTOMUL_1048576 (0x7u << 4) /**< \brief (HSMCI_DTOR) DTOCYC x 1048576 */
/* -------- HSMCI_SDCR : (HSMCI Offset: 0x0C) SD/SDIO Card Register -------- */
#define HSMCI_SDCR_SDCSEL_Pos 0
#define HSMCI_SDCR_SDCSEL_Msk (0x3u << HSMCI_SDCR_SDCSEL_Pos) /**< \brief (HSMCI_SDCR) SDCard/SDIO Slot */
#define HSMCI_SDCR_SDCSEL(value) ((HSMCI_SDCR_SDCSEL_Msk & ((value) << HSMCI_SDCR_SDCSEL_Pos)))
#define   HSMCI_SDCR_SDCSEL_SLOTA (0x0u << 0) /**< \brief (HSMCI_SDCR) Slot A is selected. */
#define   HSMCI_SDCR_SDCSEL_SLOTB (0x1u << 0) /**< \brief (HSMCI_SDCR) - */
#define   HSMCI_SDCR_SDCSEL_SLOTC (0x2u << 0) /**< \brief (HSMCI_SDCR) - */
#define   HSMCI_SDCR_SDCSEL_SLOTD (0x3u << 0) /**< \brief (HSMCI_SDCR) - */
#define HSMCI_SDCR_SDCBUS_Pos 6
#define HSMCI_SDCR_SDCBUS_Msk (0x3u << HSMCI_SDCR_SDCBUS_Pos) /**< \brief (HSMCI_SDCR) SDCard/SDIO Bus Width */
#define HSMCI_SDCR_SDCBUS(value) ((HSMCI_SDCR_SDCBUS_Msk & ((value) << HSMCI_SDCR_SDCBUS_Pos)))
#define   HSMCI_SDCR_SDCBUS_1 (0x0u << 6) /**< \brief (HSMCI_SDCR) 1 bit */
#define   HSMCI_SDCR_SDCBUS_4 (0x2u << 6) /**< \brief (HSMCI_SDCR) 4 bits */
#define   HSMCI_SDCR_SDCBUS_8 (0x3u << 6) /**< \brief (HSMCI_SDCR) 8 bits */
/* -------- HSMCI_ARGR : (HSMCI Offset: 0x10) Argument Register -------- */
#define HSMCI_ARGR_ARG_Pos 0
#define HSMCI_ARGR_ARG_Msk (0xffffffffu << HSMCI_ARGR_ARG_Pos) /**< \brief (HSMCI_ARGR) Command Argument */
#define HSMCI_ARGR_ARG(value) ((HSMCI_ARGR_ARG_Msk & ((value) << HSMCI_ARGR_ARG_Pos)))
/* -------- HSMCI_CMDR : (HSMCI Offset: 0x14) Command Register -------- */
#define HSMCI_CMDR_CMDNB_Pos 0
#define HSMCI_CMDR_CMDNB_Msk (0x3fu << HSMCI_CMDR_CMDNB_Pos) /**< \brief (HSMCI_CMDR) Command Number */
#define HSMCI_CMDR_CMDNB(value) ((HSMCI_CMDR_CMDNB_Msk & ((value) << HSMCI_CMDR_CMDNB_Pos)))
#define HSMCI_CMDR_RSPTYP_Pos 6
#define HSMCI_CMDR_RSPTYP_Msk (0x3u << HSMCI_CMDR_RSPTYP_Pos) /**< \brief (HSMCI_CMDR) Response Type */
#define HSMCI_CMDR_RSPTYP(value) ((HSMCI_CMDR_RSPTYP_Msk & ((value) << HSMCI_CMDR_RSPTYP_Pos)))
#define   HSMCI_CMDR_RSPTYP_NORESP (0x0u << 6) /**< \brief (HSMCI_CMDR) No response */
#define   HSMCI_CMDR_RSPTYP_48_BIT (0x1u << 6) /**< \brief (HSMCI_CMDR) 48-bit response */
#define   HSMCI_CMDR_RSPTYP_136_BIT (0x2u << 6) /**< \brief (HSMCI_CMDR) 136-bit response */
#define   HSMCI_CMDR_RSPTYP_R1B (0x3u << 6) /**< \brief (HSMCI_CMDR) R1b response type */
#define HSMCI_CMDR_SPCMD_Pos 8
#define HSMCI_CMDR_SPCMD_Msk (0x7u << HSMCI_CMDR_SPCMD_Pos) /**< \brief (HSMCI_CMDR) Special Command */
#define HSMCI_CMDR_SPCMD(value) ((HSMCI_CMDR_SPCMD_Msk & ((value) << HSMCI_CMDR_SPCMD_Pos)))
#define   HSMCI_CMDR_SPCMD_STD (0x0u << 8) /**< \brief (HSMCI_CMDR) Not a special CMD. */
#define   HSMCI_CMDR_SPCMD_INIT (0x1u << 8) /**< \brief (HSMCI_CMDR) Initialization CMD: 74 clock cycles for initialization sequence. */
#define   HSMCI_CMDR_SPCMD_SYNC (0x2u << 8) /**< \brief (HSMCI_CMDR) Synchronized CMD: Wait for the end of the current data block transfer before sending the pending command. */
#define   HSMCI_CMDR_SPCMD_CE_ATA (0x3u << 8) /**< \brief (HSMCI_CMDR) CE-ATA Completion Signal disable Command. The host cancels the ability for the device to return a command completion signal on the command line. */
#define   HSMCI_CMDR_SPCMD_IT_CMD (0x4u << 8) /**< \brief (HSMCI_CMDR) Interrupt command: Corresponds to the Interrupt Mode (CMD40). */
#define   HSMCI_CMDR_SPCMD_IT_RESP (0x5u << 8) /**< \brief (HSMCI_CMDR) Interrupt response: Corresponds to the Interrupt Mode (CMD40). */
#define   HSMCI_CMDR_SPCMD_BOR (0x6u << 8) /**< \brief (HSMCI_CMDR) Boot Operation Request. Start a boot operation mode, the host processor can read boot data from the MMC device directly. */
#define   HSMCI_CMDR_SPCMD_EBO (0x7u << 8) /**< \brief (HSMCI_CMDR) End Boot Operation. This command allows the host processor to terminate the boot operation mode. */
#define HSMCI_CMDR_OPDCMD (0x1u << 11) /**< \brief (HSMCI_CMDR) Open Drain Command */
#define   HSMCI_CMDR_OPDCMD_PUSHPULL (0x0u << 11) /**< \brief (HSMCI_CMDR) Push pull command. */
#define   HSMCI_CMDR_OPDCMD_OPENDRAIN (0x1u << 11) /**< \brief (HSMCI_CMDR) Open drain command. */
#define HSMCI_CMDR_MAXLAT (0x1u << 12) /**< \brief (HSMCI_CMDR) Max Latency for Command to Response */
#define   HSMCI_CMDR_MAXLAT_5 (0x0u << 12) /**< \brief (HSMCI_CMDR) 5-cycle max latency. */
#define   HSMCI_CMDR_MAXLAT_64 (0x1u << 12) /**< \brief (HSMCI_CMDR) 64-cycle max latency. */
#define HSMCI_CMDR_TRCMD_Pos 16
#define HSMCI_CMDR_TRCMD_Msk (0x3u << HSMCI_CMDR_TRCMD_Pos) /**< \brief (HSMCI_CMDR) Transfer Command */
#define HSMCI_CMDR_TRCMD(value) ((HSMCI_CMDR_TRCMD_Msk & ((value) << HSMCI_CMDR_TRCMD_Pos)))
#define   HSMCI_CMDR_TRCMD_NO_DATA (0x0u << 16) /**< \brief (HSMCI_CMDR) No data transfer */
#define   HSMCI_CMDR_TRCMD_START_DATA (0x1u << 16) /**< \brief (HSMCI_CMDR) Start data transfer */
#define   HSMCI_CMDR_TRCMD_STOP_DATA (0x2u << 16) /**< \brief (HSMCI_CMDR) Stop data transfer */
#define HSMCI_CMDR_TRDIR (0x1u << 18) /**< \brief (HSMCI_CMDR) Transfer Direction */
#define   HSMCI_CMDR_TRDIR_WRITE (0x0u << 18) /**< \brief (HSMCI_CMDR) Write. */
#define   HSMCI_CMDR_TRDIR_READ (0x1u << 18) /**< \brief (HSMCI_CMDR) Read. */
#define HSMCI_CMDR_TRTYP_Pos 19
#define HSMCI_CMDR_TRTYP_Msk (0x7u << HSMCI_CMDR_TRTYP_Pos) /**< \brief (HSMCI_CMDR) Transfer Type */
#define HSMCI_CMDR_TRTYP(value) ((HSMCI_CMDR_TRTYP_Msk & ((value) << HSMCI_CMDR_TRTYP_Pos)))
#define   HSMCI_CMDR_TRTYP_SINGLE (0x0u << 19) /**< \brief (HSMCI_CMDR) MMC/SD Card Single Block */
#define   HSMCI_CMDR_TRTYP_MULTIPLE (0x1u << 19) /**< \brief (HSMCI_CMDR) MMC/SD Card Multiple Block */
#define   HSMCI_CMDR_TRTYP_STREAM (0x2u << 19) /**< \brief (HSMCI_CMDR) MMC Stream */
#define   HSMCI_CMDR_TRTYP_BYTE (0x4u << 19) /**< \brief (HSMCI_CMDR) SDIO Byte */
#define   HSMCI_CMDR_TRTYP_BLOCK (0x5u << 19) /**< \brief (HSMCI_CMDR) SDIO Block */
#define HSMCI_CMDR_IOSPCMD_Pos 24
#define HSMCI_CMDR_IOSPCMD_Msk (0x3u << HSMCI_CMDR_IOSPCMD_Pos) /**< \brief (HSMCI_CMDR) SDIO Special Command */
#define HSMCI_CMDR_IOSPCMD(value) ((HSMCI_CMDR_IOSPCMD_Msk & ((value) << HSMCI_CMDR_IOSPCMD_Pos)))
#define   HSMCI_CMDR_IOSPCMD_STD (0x0u << 24) /**< \brief (HSMCI_CMDR) Not an SDIO Special Command */
#define   HSMCI_CMDR_IOSPCMD_SUSPEND (0x1u << 24) /**< \brief (HSMCI_CMDR) SDIO Suspend Command */
#define   HSMCI_CMDR_IOSPCMD_RESUME (0x2u << 24) /**< \brief (HSMCI_CMDR) SDIO Resume Command */
#define HSMCI_CMDR_ATACS (0x1u << 26) /**< \brief (HSMCI_CMDR) ATA with Command Completion Signal */
#define   HSMCI_CMDR_ATACS_NORMAL (0x0u << 26) /**< \brief (HSMCI_CMDR) Normal operation mode. */
#define   HSMCI_CMDR_ATACS_COMPLETION (0x1u << 26) /**< \brief (HSMCI_CMDR) This bit indicates that a completion signal is expected within a programmed amount of time (HSMCI_CSTOR). */
#define HSMCI_CMDR_BOOT_ACK (0x1u << 27) /**< \brief (HSMCI_CMDR) Boot Operation Acknowledge */
/* -------- HSMCI_BLKR : (HSMCI Offset: 0x18) Block Register -------- */
#define HSMCI_BLKR_BCNT_Pos 0
#define HSMCI_BLKR_BCNT_Msk (0xffffu << HSMCI_BLKR_BCNT_Pos) /**< \brief (HSMCI_BLKR) MMC/SDIO Block Count - SDIO Byte Count */
#define HSMCI_BLKR_BCNT(value) ((HSMCI_BLKR_BCNT_Msk & ((value) << HSMCI_BLKR_BCNT_Pos)))
#define HSMCI_BLKR_BLKLEN_Pos 16
#define HSMCI_BLKR_BLKLEN_Msk (0xffffu << HSMCI_BLKR_BLKLEN_Pos) /**< \brief (HSMCI_BLKR) Data Block Length */
#define HSMCI_BLKR_BLKLEN(value) ((HSMCI_BLKR_BLKLEN_Msk & ((value) << HSMCI_BLKR_BLKLEN_Pos)))
/* -------- HSMCI_CSTOR : (HSMCI Offset: 0x1C) Completion Signal Timeout Register -------- */
#define HSMCI_CSTOR_CSTOCYC_Pos 0
#define HSMCI_CSTOR_CSTOCYC_Msk (0xfu << HSMCI_CSTOR_CSTOCYC_Pos) /**< \brief (HSMCI_CSTOR) Completion Signal Timeout Cycle Number */
#define HSMCI_CSTOR_CSTOCYC(value) ((HSMCI_CSTOR_CSTOCYC_Msk & ((value) << HSMCI_CSTOR_CSTOCYC_Pos)))
#define HSMCI_CSTOR_CSTOMUL_Pos 4
#define HSMCI_CSTOR_CSTOMUL_Msk (0x7u << HSMCI_CSTOR_CSTOMUL_Pos) /**< \brief (HSMCI_CSTOR) Completion Signal Timeout Multiplier */
#define HSMCI_CSTOR_CSTOMUL(value) ((HSMCI_CSTOR_CSTOMUL_Msk & ((value) << HSMCI_CSTOR_CSTOMUL_Pos)))
#define   HSMCI_CSTOR_CSTOMUL_1 (0x0u << 4) /**< \brief (HSMCI_CSTOR) CSTOCYC x 1 */
#define   HSMCI_CSTOR_CSTOMUL_16 (0x1u << 4) /**< \brief (HSMCI_CSTOR) CSTOCYC x 16 */
#define   HSMCI_CSTOR_CSTOMUL_128 (0x2u << 4) /**< \brief (HSMCI_CSTOR) CSTOCYC x 128 */
#define   HSMCI_CSTOR_CSTOMUL_256 (0x3u << 4) /**< \brief (HSMCI_CSTOR) CSTOCYC x 256 */
#define   HSMCI_CSTOR_CSTOMUL_1024 (0x4u << 4) /**< \brief (HSMCI_CSTOR) CSTOCYC x 1024 */
#define   HSMCI_CSTOR_CSTOMUL_4096 (0x5u << 4) /**< \brief (HSMCI_CSTOR) CSTOCYC x 4096 */
#define   HSMCI_CSTOR_CSTOMUL_65536 (0x6u << 4) /**< \brief (HSMCI_CSTOR) CSTOCYC x 65536 */
#define   HSMCI_CSTOR_CSTOMUL_1048576 (0x7u << 4) /**< \brief (HSMCI_CSTOR) CSTOCYC x 1048576 */
/* -------- HSMCI_RSPR[4] : (HSMCI Offset: 0x20) Response Register -------- */
#define HSMCI_RSPR_RSP_Pos 0
#define HSMCI_RSPR_RSP_Msk (0xffffffffu << HSMCI_RSPR_RSP_Pos) /**< \brief (HSMCI_RSPR[4]) Response */
/* -------- HSMCI_RDR : (HSMCI Offset: 0x30) Receive Data Register -------- */
#define HSMCI_RDR_DATA_Pos 0
#define HSMCI_RDR_DATA_Msk (0xffffffffu << HSMCI_RDR_DATA_Pos) /**< \brief (HSMCI_RDR) Data to Read */
/* -------- HSMCI_TDR : (HSMCI Offset: 0x34) Transmit Data Register -------- */
#define HSMCI_TDR_DATA_Pos 0
#define HSMCI_TDR_DATA_Msk (0xffffffffu << HSMCI_TDR_DATA_Pos) /**< \brief (HSMCI_TDR) Data to Write */
#define HSMCI_TDR_DATA(value) ((HSMCI_TDR_DATA_Msk & ((value) << HSMCI_TDR_DATA_Pos)))
/* -------- HSMCI_SR : (HSMCI Offset: 0x40) Status Register -------- */
#define HSMCI_SR_CMDRDY (0x1u << 0) /**< \brief (HSMCI_SR) Command Ready (cleared by writing in HSMCI_CMDR) */
#define HSMCI_SR_RXRDY (0x1u << 1) /**< \brief (HSMCI_SR) Receiver Ready (cleared by reading HSMCI_RDR) */
#define HSMCI_SR_TXRDY (0x1u << 2) /**< \brief (HSMCI_SR) Transmit Ready (cleared by writing in HSMCI_TDR) */
#define HSMCI_SR_BLKE (0x1u << 3) /**< \brief (HSMCI_SR) Data Block Ended (cleared on read) */
#define HSMCI_SR_DTIP (0x1u << 4) /**< \brief (HSMCI_SR) Data Transfer in Progress (cleared at the end of CRC16 calculation) */
#define HSMCI_SR_NOTBUSY (0x1u << 5) /**< \brief (HSMCI_SR) HSMCI Not Busy */
#define HSMCI_SR_ENDRX (0x1u << 6) /**< \brief (HSMCI_SR) End of RX Buffer (cleared by writing HSMCI_RCR or HSMCI_RNCR(1)) */
#define HSMCI_SR_ENDTX (0x1u << 7) /**< \brief (HSMCI_SR) End of TX Buffer (cleared by writing HSMCI_TCR or HSMCI_TNCR(1)) */
#define HSMCI_SR_SDIOIRQA (0x1u << 8) /**< \brief (HSMCI_SR) SDIO Interrupt for Slot A (cleared on read) */
#define HSMCI_SR_SDIOWAIT (0x1u << 12) /**< \brief (HSMCI_SR) SDIO Read Wait Operation Status */
#define HSMCI_SR_CSRCV (0x1u << 13) /**< \brief (HSMCI_SR) CE-ATA Completion Signal Received (cleared on read) */
#define HSMCI_SR_RXBUFF (0x1u << 14) /**< \brief (HSMCI_SR) RX Buffer Full (cleared by writing HSMCI_RCR or HSMCI_RNCR(1)) */
#define HSMCI_SR_TXBUFE (0x1u << 15) /**< \brief (HSMCI_SR) TX Buffer Empty (cleared by writing HSMCI_TCR or HSMCI_TNCR(1)) */
#define HSMCI_SR_RINDE (0x1u << 16) /**< \brief (HSMCI_SR) Response Index Error (cleared by writing in HSMCI_CMDR) */
#define HSMCI_SR_RDIRE (0x1u << 17) /**< \brief (HSMCI_SR) Response Direction Error (cleared by writing in HSMCI_CMDR) */
#define HSMCI_SR_RCRCE (0x1u << 18) /**< \brief (HSMCI_SR) Response CRC Error (cleared by writing in HSMCI_CMDR) */
#define HSMCI_SR_RENDE (0x1u << 19) /**< \brief (HSMCI_SR) Response End Bit Error (cleared by writing in HSMCI_CMDR) */
#define HSMCI_SR_RTOE (0x1u << 20) /**< \brief (HSMCI_SR) Response Time-out Error (cleared by writing in HSMCI_CMDR) */
#define HSMCI_SR_DCRCE (0x1u << 21) /**< \brief (HSMCI_SR) Data CRC Error (cleared on read) */
#define HSMCI_SR_DTOE (0x1u << 22) /**< \brief (HSMCI_SR) Data Time-out Error (cleared on read) */
#define HSMCI_SR_CSTOE (0x1u << 23) /**< \brief (HSMCI_SR) Completion Signal Time-out Error (cleared on read) */
#define HSMCI_SR_FIFOEMPTY (0x1u << 26) /**< \brief (HSMCI_SR) FIFO empty flag */
#define HSMCI_SR_XFRDONE (0x1u << 27) /**< \brief (HSMCI_SR) Transfer Done flag */
#define HSMCI_SR_ACKRCV (0x1u << 28) /**< \brief (HSMCI_SR) Boot Operation Acknowledge Received  (cleared on read) */
#define HSMCI_SR_ACKRCVE (0x1u << 29) /**< \brief (HSMCI_SR) Boot Operation Acknowledge Error (cleared on read) */
#define HSMCI_SR_OVRE (0x1u << 30) /**< \brief (HSMCI_SR) Overrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) */
#define HSMCI_SR_UNRE (0x1u << 31) /**< \brief (HSMCI_SR) Underrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) */
/* -------- HSMCI_IER : (HSMCI Offset: 0x44) Interrupt Enable Register -------- */
#define HSMCI_IER_CMDRDY (0x1u << 0) /**< \brief (HSMCI_IER) Command Ready Interrupt Enable */
#define HSMCI_IER_RXRDY (0x1u << 1) /**< \brief (HSMCI_IER) Receiver Ready Interrupt Enable */
#define HSMCI_IER_TXRDY (0x1u << 2) /**< \brief (HSMCI_IER) Transmit Ready Interrupt Enable */
#define HSMCI_IER_BLKE (0x1u << 3) /**< \brief (HSMCI_IER) Data Block Ended Interrupt Enable */
#define HSMCI_IER_DTIP (0x1u << 4) /**< \brief (HSMCI_IER) Data Transfer in Progress Interrupt Enable */
#define HSMCI_IER_NOTBUSY (0x1u << 5) /**< \brief (HSMCI_IER) Data Not Busy Interrupt Enable */
#define HSMCI_IER_ENDRX (0x1u << 6) /**< \brief (HSMCI_IER) End of Receive Buffer Interrupt Enable */
#define HSMCI_IER_ENDTX (0x1u << 7) /**< \brief (HSMCI_IER) End of Transmit Buffer Interrupt Enable */
#define HSMCI_IER_SDIOIRQA (0x1u << 8) /**< \brief (HSMCI_IER) SDIO Interrupt for Slot A Interrupt Enable */
#define HSMCI_IER_SDIOWAIT (0x1u << 12) /**< \brief (HSMCI_IER) SDIO Read Wait Operation Status Interrupt Enable */
#define HSMCI_IER_CSRCV (0x1u << 13) /**< \brief (HSMCI_IER) Completion Signal Received Interrupt Enable */
#define HSMCI_IER_RXBUFF (0x1u << 14) /**< \brief (HSMCI_IER) Receive Buffer Full Interrupt Enable */
#define HSMCI_IER_TXBUFE (0x1u << 15) /**< \brief (HSMCI_IER) Transmit Buffer Empty Interrupt Enable */
#define HSMCI_IER_RINDE (0x1u << 16) /**< \brief (HSMCI_IER) Response Index Error Interrupt Enable */
#define HSMCI_IER_RDIRE (0x1u << 17) /**< \brief (HSMCI_IER) Response Direction Error Interrupt Enable */
#define HSMCI_IER_RCRCE (0x1u << 18) /**< \brief (HSMCI_IER) Response CRC Error Interrupt Enable */
#define HSMCI_IER_RENDE (0x1u << 19) /**< \brief (HSMCI_IER) Response End Bit Error Interrupt Enable */
#define HSMCI_IER_RTOE (0x1u << 20) /**< \brief (HSMCI_IER) Response Time-out Error Interrupt Enable */
#define HSMCI_IER_DCRCE (0x1u << 21) /**< \brief (HSMCI_IER) Data CRC Error Interrupt Enable */
#define HSMCI_IER_DTOE (0x1u << 22) /**< \brief (HSMCI_IER) Data Time-out Error Interrupt Enable */
#define HSMCI_IER_CSTOE (0x1u << 23) /**< \brief (HSMCI_IER) Completion Signal Timeout Error Interrupt Enable */
#define HSMCI_IER_FIFOEMPTY (0x1u << 26) /**< \brief (HSMCI_IER) FIFO empty Interrupt enable */
#define HSMCI_IER_XFRDONE (0x1u << 27) /**< \brief (HSMCI_IER) Transfer Done Interrupt enable */
#define HSMCI_IER_ACKRCV (0x1u << 28) /**< \brief (HSMCI_IER) Boot Acknowledge Interrupt Enable */
#define HSMCI_IER_ACKRCVE (0x1u << 29) /**< \brief (HSMCI_IER) Boot Acknowledge Error Interrupt Enable */
#define HSMCI_IER_OVRE (0x1u << 30) /**< \brief (HSMCI_IER) Overrun Interrupt Enable */
#define HSMCI_IER_UNRE (0x1u << 31) /**< \brief (HSMCI_IER) Underrun Interrupt Enable */
/* -------- HSMCI_IDR : (HSMCI Offset: 0x48) Interrupt Disable Register -------- */
#define HSMCI_IDR_CMDRDY (0x1u << 0) /**< \brief (HSMCI_IDR) Command Ready Interrupt Disable */
#define HSMCI_IDR_RXRDY (0x1u << 1) /**< \brief (HSMCI_IDR) Receiver Ready Interrupt Disable */
#define HSMCI_IDR_TXRDY (0x1u << 2) /**< \brief (HSMCI_IDR) Transmit Ready Interrupt Disable */
#define HSMCI_IDR_BLKE (0x1u << 3) /**< \brief (HSMCI_IDR) Data Block Ended Interrupt Disable */
#define HSMCI_IDR_DTIP (0x1u << 4) /**< \brief (HSMCI_IDR) Data Transfer in Progress Interrupt Disable */
#define HSMCI_IDR_NOTBUSY (0x1u << 5) /**< \brief (HSMCI_IDR) Data Not Busy Interrupt Disable */
#define HSMCI_IDR_ENDRX (0x1u << 6) /**< \brief (HSMCI_IDR) End of Receive Buffer Interrupt Disable */
#define HSMCI_IDR_ENDTX (0x1u << 7) /**< \brief (HSMCI_IDR) End of Transmit Buffer Interrupt Disable */
#define HSMCI_IDR_SDIOIRQA (0x1u << 8) /**< \brief (HSMCI_IDR) SDIO Interrupt for Slot A Interrupt Disable */
#define HSMCI_IDR_SDIOWAIT (0x1u << 12) /**< \brief (HSMCI_IDR) SDIO Read Wait Operation Status Interrupt Disable */
#define HSMCI_IDR_CSRCV (0x1u << 13) /**< \brief (HSMCI_IDR) Completion Signal received interrupt Disable */
#define HSMCI_IDR_RXBUFF (0x1u << 14) /**< \brief (HSMCI_IDR) Receive Buffer Full Interrupt Disable */
#define HSMCI_IDR_TXBUFE (0x1u << 15) /**< \brief (HSMCI_IDR) Transmit Buffer Empty Interrupt Disable */
#define HSMCI_IDR_RINDE (0x1u << 16) /**< \brief (HSMCI_IDR) Response Index Error Interrupt Disable */
#define HSMCI_IDR_RDIRE (0x1u << 17) /**< \brief (HSMCI_IDR) Response Direction Error Interrupt Disable */
#define HSMCI_IDR_RCRCE (0x1u << 18) /**< \brief (HSMCI_IDR) Response CRC Error Interrupt Disable */
#define HSMCI_IDR_RENDE (0x1u << 19) /**< \brief (HSMCI_IDR) Response End Bit Error Interrupt Disable */
#define HSMCI_IDR_RTOE (0x1u << 20) /**< \brief (HSMCI_IDR) Response Time-out Error Interrupt Disable */
#define HSMCI_IDR_DCRCE (0x1u << 21) /**< \brief (HSMCI_IDR) Data CRC Error Interrupt Disable */
#define HSMCI_IDR_DTOE (0x1u << 22) /**< \brief (HSMCI_IDR) Data Time-out Error Interrupt Disable */
#define HSMCI_IDR_CSTOE (0x1u << 23) /**< \brief (HSMCI_IDR) Completion Signal Time out Error Interrupt Disable */
#define HSMCI_IDR_FIFOEMPTY (0x1u << 26) /**< \brief (HSMCI_IDR) FIFO empty Interrupt Disable */
#define HSMCI_IDR_XFRDONE (0x1u << 27) /**< \brief (HSMCI_IDR) Transfer Done Interrupt Disable */
#define HSMCI_IDR_ACKRCV (0x1u << 28) /**< \brief (HSMCI_IDR) Boot Acknowledge Interrupt Disable */
#define HSMCI_IDR_ACKRCVE (0x1u << 29) /**< \brief (HSMCI_IDR) Boot Acknowledge Error Interrupt Disable */
#define HSMCI_IDR_OVRE (0x1u << 30) /**< \brief (HSMCI_IDR) Overrun Interrupt Disable */
#define HSMCI_IDR_UNRE (0x1u << 31) /**< \brief (HSMCI_IDR) Underrun Interrupt Disable */
/* -------- HSMCI_IMR : (HSMCI Offset: 0x4C) Interrupt Mask Register -------- */
#define HSMCI_IMR_CMDRDY (0x1u << 0) /**< \brief (HSMCI_IMR) Command Ready Interrupt Mask */
#define HSMCI_IMR_RXRDY (0x1u << 1) /**< \brief (HSMCI_IMR) Receiver Ready Interrupt Mask */
#define HSMCI_IMR_TXRDY (0x1u << 2) /**< \brief (HSMCI_IMR) Transmit Ready Interrupt Mask */
#define HSMCI_IMR_BLKE (0x1u << 3) /**< \brief (HSMCI_IMR) Data Block Ended Interrupt Mask */
#define HSMCI_IMR_DTIP (0x1u << 4) /**< \brief (HSMCI_IMR) Data Transfer in Progress Interrupt Mask */
#define HSMCI_IMR_NOTBUSY (0x1u << 5) /**< \brief (HSMCI_IMR) Data Not Busy Interrupt Mask */
#define HSMCI_IMR_ENDRX (0x1u << 6) /**< \brief (HSMCI_IMR) End of Receive Buffer Interrupt Mask */
#define HSMCI_IMR_ENDTX (0x1u << 7) /**< \brief (HSMCI_IMR) End of Transmit Buffer Interrupt Mask */
#define HSMCI_IMR_SDIOIRQA (0x1u << 8) /**< \brief (HSMCI_IMR) SDIO Interrupt for Slot A Interrupt Mask */
#define HSMCI_IMR_SDIOWAIT (0x1u << 12) /**< \brief (HSMCI_IMR) SDIO Read Wait Operation Status Interrupt Mask */
#define HSMCI_IMR_CSRCV (0x1u << 13) /**< \brief (HSMCI_IMR) Completion Signal Received Interrupt Mask */
#define HSMCI_IMR_RXBUFF (0x1u << 14) /**< \brief (HSMCI_IMR) Receive Buffer Full Interrupt Mask */
#define HSMCI_IMR_TXBUFE (0x1u << 15) /**< \brief (HSMCI_IMR) Transmit Buffer Empty Interrupt Mask */
#define HSMCI_IMR_RINDE (0x1u << 16) /**< \brief (HSMCI_IMR) Response Index Error Interrupt Mask */
#define HSMCI_IMR_RDIRE (0x1u << 17) /**< \brief (HSMCI_IMR) Response Direction Error Interrupt Mask */
#define HSMCI_IMR_RCRCE (0x1u << 18) /**< \brief (HSMCI_IMR) Response CRC Error Interrupt Mask */
#define HSMCI_IMR_RENDE (0x1u << 19) /**< \brief (HSMCI_IMR) Response End Bit Error Interrupt Mask */
#define HSMCI_IMR_RTOE (0x1u << 20) /**< \brief (HSMCI_IMR) Response Time-out Error Interrupt Mask */
#define HSMCI_IMR_DCRCE (0x1u << 21) /**< \brief (HSMCI_IMR) Data CRC Error Interrupt Mask */
#define HSMCI_IMR_DTOE (0x1u << 22) /**< \brief (HSMCI_IMR) Data Time-out Error Interrupt Mask */
#define HSMCI_IMR_CSTOE (0x1u << 23) /**< \brief (HSMCI_IMR) Completion Signal Time-out Error Interrupt Mask */
#define HSMCI_IMR_FIFOEMPTY (0x1u << 26) /**< \brief (HSMCI_IMR) FIFO Empty Interrupt Mask */
#define HSMCI_IMR_XFRDONE (0x1u << 27) /**< \brief (HSMCI_IMR) Transfer Done Interrupt Mask */
#define HSMCI_IMR_ACKRCV (0x1u << 28) /**< \brief (HSMCI_IMR) Boot Operation Acknowledge Received Interrupt Mask */
#define HSMCI_IMR_ACKRCVE (0x1u << 29) /**< \brief (HSMCI_IMR) Boot Operation Acknowledge Error Interrupt Mask */
#define HSMCI_IMR_OVRE (0x1u << 30) /**< \brief (HSMCI_IMR) Overrun Interrupt Mask */
#define HSMCI_IMR_UNRE (0x1u << 31) /**< \brief (HSMCI_IMR) Underrun Interrupt Mask */
/* -------- HSMCI_CFG : (HSMCI Offset: 0x54) Configuration Register -------- */
#define HSMCI_CFG_FIFOMODE (0x1u << 0) /**< \brief (HSMCI_CFG) HSMCI Internal FIFO control mode */
#define HSMCI_CFG_FERRCTRL (0x1u << 4) /**< \brief (HSMCI_CFG) Flow Error flag reset control mode */
#define HSMCI_CFG_HSMODE (0x1u << 8) /**< \brief (HSMCI_CFG) High Speed Mode */
#define HSMCI_CFG_LSYNC (0x1u << 12) /**< \brief (HSMCI_CFG) Synchronize on the last block */
/* -------- HSMCI_WPMR : (HSMCI Offset: 0xE4) Write Protection Mode Register -------- */
#define HSMCI_WPMR_WPEN (0x1u << 0) /**< \brief (HSMCI_WPMR) Write Protect Enable */
#define HSMCI_WPMR_WPKEY_Pos 8
#define HSMCI_WPMR_WPKEY_Msk (0xffffffu << HSMCI_WPMR_WPKEY_Pos) /**< \brief (HSMCI_WPMR) Write Protect Key */
#define HSMCI_WPMR_WPKEY(value) ((HSMCI_WPMR_WPKEY_Msk & ((value) << HSMCI_WPMR_WPKEY_Pos)))
#define   HSMCI_WPMR_WPKEY_PASSWD (0x4D4349u << 8) /**< \brief (HSMCI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. */
/* -------- HSMCI_WPSR : (HSMCI Offset: 0xE8) Write Protection Status Register -------- */
#define HSMCI_WPSR_WPVS (0x1u << 0) /**< \brief (HSMCI_WPSR) Write Protection Violation Status */
#define HSMCI_WPSR_WPVSRC_Pos 8
#define HSMCI_WPSR_WPVSRC_Msk (0xffffu << HSMCI_WPSR_WPVSRC_Pos) /**< \brief (HSMCI_WPSR) Write Protection Violation Source */
/* -------- HSMCI_RPR : (HSMCI Offset: 0x100) Receive Pointer Register -------- */
#define HSMCI_RPR_RXPTR_Pos 0
#define HSMCI_RPR_RXPTR_Msk (0xffffffffu << HSMCI_RPR_RXPTR_Pos) /**< \brief (HSMCI_RPR) Receive Pointer Register */
#define HSMCI_RPR_RXPTR(value) ((HSMCI_RPR_RXPTR_Msk & ((value) << HSMCI_RPR_RXPTR_Pos)))
/* -------- HSMCI_RCR : (HSMCI Offset: 0x104) Receive Counter Register -------- */
#define HSMCI_RCR_RXCTR_Pos 0
#define HSMCI_RCR_RXCTR_Msk (0xffffu << HSMCI_RCR_RXCTR_Pos) /**< \brief (HSMCI_RCR) Receive Counter Register */
#define HSMCI_RCR_RXCTR(value) ((HSMCI_RCR_RXCTR_Msk & ((value) << HSMCI_RCR_RXCTR_Pos)))
/* -------- HSMCI_TPR : (HSMCI Offset: 0x108) Transmit Pointer Register -------- */
#define HSMCI_TPR_TXPTR_Pos 0
#define HSMCI_TPR_TXPTR_Msk (0xffffffffu << HSMCI_TPR_TXPTR_Pos) /**< \brief (HSMCI_TPR) Transmit Counter Register */
#define HSMCI_TPR_TXPTR(value) ((HSMCI_TPR_TXPTR_Msk & ((value) << HSMCI_TPR_TXPTR_Pos)))
/* -------- HSMCI_TCR : (HSMCI Offset: 0x10C) Transmit Counter Register -------- */
#define HSMCI_TCR_TXCTR_Pos 0
#define HSMCI_TCR_TXCTR_Msk (0xffffu << HSMCI_TCR_TXCTR_Pos) /**< \brief (HSMCI_TCR) Transmit Counter Register */
#define HSMCI_TCR_TXCTR(value) ((HSMCI_TCR_TXCTR_Msk & ((value) << HSMCI_TCR_TXCTR_Pos)))
/* -------- HSMCI_RNPR : (HSMCI Offset: 0x110) Receive Next Pointer Register -------- */
#define HSMCI_RNPR_RXNPTR_Pos 0
#define HSMCI_RNPR_RXNPTR_Msk (0xffffffffu << HSMCI_RNPR_RXNPTR_Pos) /**< \brief (HSMCI_RNPR) Receive Next Pointer */
#define HSMCI_RNPR_RXNPTR(value) ((HSMCI_RNPR_RXNPTR_Msk & ((value) << HSMCI_RNPR_RXNPTR_Pos)))
/* -------- HSMCI_RNCR : (HSMCI Offset: 0x114) Receive Next Counter Register -------- */
#define HSMCI_RNCR_RXNCTR_Pos 0
#define HSMCI_RNCR_RXNCTR_Msk (0xffffu << HSMCI_RNCR_RXNCTR_Pos) /**< \brief (HSMCI_RNCR) Receive Next Counter */
#define HSMCI_RNCR_RXNCTR(value) ((HSMCI_RNCR_RXNCTR_Msk & ((value) << HSMCI_RNCR_RXNCTR_Pos)))
/* -------- HSMCI_TNPR : (HSMCI Offset: 0x118) Transmit Next Pointer Register -------- */
#define HSMCI_TNPR_TXNPTR_Pos 0
#define HSMCI_TNPR_TXNPTR_Msk (0xffffffffu << HSMCI_TNPR_TXNPTR_Pos) /**< \brief (HSMCI_TNPR) Transmit Next Pointer */
#define HSMCI_TNPR_TXNPTR(value) ((HSMCI_TNPR_TXNPTR_Msk & ((value) << HSMCI_TNPR_TXNPTR_Pos)))
/* -------- HSMCI_TNCR : (HSMCI Offset: 0x11C) Transmit Next Counter Register -------- */
#define HSMCI_TNCR_TXNCTR_Pos 0
#define HSMCI_TNCR_TXNCTR_Msk (0xffffu << HSMCI_TNCR_TXNCTR_Pos) /**< \brief (HSMCI_TNCR) Transmit Counter Next */
#define HSMCI_TNCR_TXNCTR(value) ((HSMCI_TNCR_TXNCTR_Msk & ((value) << HSMCI_TNCR_TXNCTR_Pos)))
/* -------- HSMCI_PTCR : (HSMCI Offset: 0x120) Transfer Control Register -------- */
#define HSMCI_PTCR_RXTEN (0x1u << 0) /**< \brief (HSMCI_PTCR) Receiver Transfer Enable */
#define HSMCI_PTCR_RXTDIS (0x1u << 1) /**< \brief (HSMCI_PTCR) Receiver Transfer Disable */
#define HSMCI_PTCR_TXTEN (0x1u << 8) /**< \brief (HSMCI_PTCR) Transmitter Transfer Enable */
#define HSMCI_PTCR_TXTDIS (0x1u << 9) /**< \brief (HSMCI_PTCR) Transmitter Transfer Disable */
/* -------- HSMCI_PTSR : (HSMCI Offset: 0x124) Transfer Status Register -------- */
#define HSMCI_PTSR_RXTEN (0x1u << 0) /**< \brief (HSMCI_PTSR) Receiver Transfer Enable */
#define HSMCI_PTSR_TXTEN (0x1u << 8) /**< \brief (HSMCI_PTSR) Transmitter Transfer Enable */
/* -------- HSMCI_FIFO[256] : (HSMCI Offset: 0x200) FIFO Memory Aperture0 -------- */
#define HSMCI_FIFO_DATA_Pos 0
#define HSMCI_FIFO_DATA_Msk (0xffffffffu << HSMCI_FIFO_DATA_Pos) /**< \brief (HSMCI_FIFO[256]) Data to Read or Data to Write */
#define HSMCI_FIFO_DATA(value) ((HSMCI_FIFO_DATA_Msk & ((value) << HSMCI_FIFO_DATA_Pos)))

/*@}*/


#endif /* _SAM4S_HSMCI_COMPONENT_ */
