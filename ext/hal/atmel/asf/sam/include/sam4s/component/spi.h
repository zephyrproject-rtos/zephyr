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

#ifndef _SAM4S_SPI_COMPONENT_
#define _SAM4S_SPI_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Serial Peripheral Interface */
/* ============================================================================= */
/** \addtogroup SAM4S_SPI Serial Peripheral Interface */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Spi hardware registers */
typedef struct {
  __O  uint32_t SPI_CR;        /**< \brief (Spi Offset: 0x00) Control Register */
  __IO uint32_t SPI_MR;        /**< \brief (Spi Offset: 0x04) Mode Register */
  __I  uint32_t SPI_RDR;       /**< \brief (Spi Offset: 0x08) Receive Data Register */
  __O  uint32_t SPI_TDR;       /**< \brief (Spi Offset: 0x0C) Transmit Data Register */
  __I  uint32_t SPI_SR;        /**< \brief (Spi Offset: 0x10) Status Register */
  __O  uint32_t SPI_IER;       /**< \brief (Spi Offset: 0x14) Interrupt Enable Register */
  __O  uint32_t SPI_IDR;       /**< \brief (Spi Offset: 0x18) Interrupt Disable Register */
  __I  uint32_t SPI_IMR;       /**< \brief (Spi Offset: 0x1C) Interrupt Mask Register */
  __I  uint32_t Reserved1[4];
  __IO uint32_t SPI_CSR[4];    /**< \brief (Spi Offset: 0x30) Chip Select Register */
  __I  uint32_t Reserved2[41];
  __IO uint32_t SPI_WPMR;      /**< \brief (Spi Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t SPI_WPSR;      /**< \brief (Spi Offset: 0xE8) Write Protection Status Register */
  __I  uint32_t Reserved3[5];
  __IO uint32_t SPI_RPR;       /**< \brief (Spi Offset: 0x100) Receive Pointer Register */
  __IO uint32_t SPI_RCR;       /**< \brief (Spi Offset: 0x104) Receive Counter Register */
  __IO uint32_t SPI_TPR;       /**< \brief (Spi Offset: 0x108) Transmit Pointer Register */
  __IO uint32_t SPI_TCR;       /**< \brief (Spi Offset: 0x10C) Transmit Counter Register */
  __IO uint32_t SPI_RNPR;      /**< \brief (Spi Offset: 0x110) Receive Next Pointer Register */
  __IO uint32_t SPI_RNCR;      /**< \brief (Spi Offset: 0x114) Receive Next Counter Register */
  __IO uint32_t SPI_TNPR;      /**< \brief (Spi Offset: 0x118) Transmit Next Pointer Register */
  __IO uint32_t SPI_TNCR;      /**< \brief (Spi Offset: 0x11C) Transmit Next Counter Register */
  __O  uint32_t SPI_PTCR;      /**< \brief (Spi Offset: 0x120) Transfer Control Register */
  __I  uint32_t SPI_PTSR;      /**< \brief (Spi Offset: 0x124) Transfer Status Register */
} Spi;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- SPI_CR : (SPI Offset: 0x00) Control Register -------- */
#define SPI_CR_SPIEN (0x1u << 0) /**< \brief (SPI_CR) SPI Enable */
#define SPI_CR_SPIDIS (0x1u << 1) /**< \brief (SPI_CR) SPI Disable */
#define SPI_CR_SWRST (0x1u << 7) /**< \brief (SPI_CR) SPI Software Reset */
#define SPI_CR_LASTXFER (0x1u << 24) /**< \brief (SPI_CR) Last Transfer */
/* -------- SPI_MR : (SPI Offset: 0x04) Mode Register -------- */
#define SPI_MR_MSTR (0x1u << 0) /**< \brief (SPI_MR) Master/Slave Mode */
#define SPI_MR_PS (0x1u << 1) /**< \brief (SPI_MR) Peripheral Select */
#define SPI_MR_PCSDEC (0x1u << 2) /**< \brief (SPI_MR) Chip Select Decode */
#define SPI_MR_MODFDIS (0x1u << 4) /**< \brief (SPI_MR) Mode Fault Detection */
#define SPI_MR_WDRBT (0x1u << 5) /**< \brief (SPI_MR) Wait Data Read Before Transfer */
#define SPI_MR_LLB (0x1u << 7) /**< \brief (SPI_MR) Local Loopback Enable */
#define SPI_MR_PCS_Pos 16
#define SPI_MR_PCS_Msk (0xfu << SPI_MR_PCS_Pos) /**< \brief (SPI_MR) Peripheral Chip Select */
#define SPI_MR_PCS(value) ((SPI_MR_PCS_Msk & ((value) << SPI_MR_PCS_Pos)))
#define SPI_MR_DLYBCS_Pos 24
#define SPI_MR_DLYBCS_Msk (0xffu << SPI_MR_DLYBCS_Pos) /**< \brief (SPI_MR) Delay Between Chip Selects */
#define SPI_MR_DLYBCS(value) ((SPI_MR_DLYBCS_Msk & ((value) << SPI_MR_DLYBCS_Pos)))
/* -------- SPI_RDR : (SPI Offset: 0x08) Receive Data Register -------- */
#define SPI_RDR_RD_Pos 0
#define SPI_RDR_RD_Msk (0xffffu << SPI_RDR_RD_Pos) /**< \brief (SPI_RDR) Receive Data */
#define SPI_RDR_PCS_Pos 16
#define SPI_RDR_PCS_Msk (0xfu << SPI_RDR_PCS_Pos) /**< \brief (SPI_RDR) Peripheral Chip Select */
/* -------- SPI_TDR : (SPI Offset: 0x0C) Transmit Data Register -------- */
#define SPI_TDR_TD_Pos 0
#define SPI_TDR_TD_Msk (0xffffu << SPI_TDR_TD_Pos) /**< \brief (SPI_TDR) Transmit Data */
#define SPI_TDR_TD(value) ((SPI_TDR_TD_Msk & ((value) << SPI_TDR_TD_Pos)))
#define SPI_TDR_PCS_Pos 16
#define SPI_TDR_PCS_Msk (0xfu << SPI_TDR_PCS_Pos) /**< \brief (SPI_TDR) Peripheral Chip Select */
#define SPI_TDR_PCS(value) ((SPI_TDR_PCS_Msk & ((value) << SPI_TDR_PCS_Pos)))
#define SPI_TDR_LASTXFER (0x1u << 24) /**< \brief (SPI_TDR) Last Transfer */
/* -------- SPI_SR : (SPI Offset: 0x10) Status Register -------- */
#define SPI_SR_RDRF (0x1u << 0) /**< \brief (SPI_SR) Receive Data Register Full (cleared by reading SPI_RDR) */
#define SPI_SR_TDRE (0x1u << 1) /**< \brief (SPI_SR) Transmit Data Register Empty (cleared by writing SPI_TDR) */
#define SPI_SR_MODF (0x1u << 2) /**< \brief (SPI_SR) Mode Fault Error (cleared on read) */
#define SPI_SR_OVRES (0x1u << 3) /**< \brief (SPI_SR) Overrun Error Status (cleared on read) */
#define SPI_SR_ENDRX (0x1u << 4) /**< \brief (SPI_SR) End of RX Buffer (cleared by writing SPI_RCR or SPI_RNCR) */
#define SPI_SR_ENDTX (0x1u << 5) /**< \brief (SPI_SR) End of TX Buffer (cleared by writing SPI_TCR or SPI_TNCR) */
#define SPI_SR_RXBUFF (0x1u << 6) /**< \brief (SPI_SR) RX Buffer Full (cleared by writing SPI_RCR or SPI_RNCR) */
#define SPI_SR_TXBUFE (0x1u << 7) /**< \brief (SPI_SR) TX Buffer Empty (cleared by writing SPI_TCR or SPI_TNCR) */
#define SPI_SR_NSSR (0x1u << 8) /**< \brief (SPI_SR) NSS Rising (cleared on read) */
#define SPI_SR_TXEMPTY (0x1u << 9) /**< \brief (SPI_SR) Transmission Registers Empty (cleared by writing SPI_TDR) */
#define SPI_SR_UNDES (0x1u << 10) /**< \brief (SPI_SR) Underrun Error Status (Slave mode only) (cleared on read) */
#define SPI_SR_SPIENS (0x1u << 16) /**< \brief (SPI_SR) SPI Enable Status */
/* -------- SPI_IER : (SPI Offset: 0x14) Interrupt Enable Register -------- */
#define SPI_IER_RDRF (0x1u << 0) /**< \brief (SPI_IER) Receive Data Register Full Interrupt Enable */
#define SPI_IER_TDRE (0x1u << 1) /**< \brief (SPI_IER) SPI Transmit Data Register Empty Interrupt Enable */
#define SPI_IER_MODF (0x1u << 2) /**< \brief (SPI_IER) Mode Fault Error Interrupt Enable */
#define SPI_IER_OVRES (0x1u << 3) /**< \brief (SPI_IER) Overrun Error Interrupt Enable */
#define SPI_IER_ENDRX (0x1u << 4) /**< \brief (SPI_IER) End of Receive Buffer Interrupt Enable */
#define SPI_IER_ENDTX (0x1u << 5) /**< \brief (SPI_IER) End of Transmit Buffer Interrupt Enable */
#define SPI_IER_RXBUFF (0x1u << 6) /**< \brief (SPI_IER) Receive Buffer Full Interrupt Enable */
#define SPI_IER_TXBUFE (0x1u << 7) /**< \brief (SPI_IER) Transmit Buffer Empty Interrupt Enable */
#define SPI_IER_NSSR (0x1u << 8) /**< \brief (SPI_IER) NSS Rising Interrupt Enable */
#define SPI_IER_TXEMPTY (0x1u << 9) /**< \brief (SPI_IER) Transmission Registers Empty Enable */
#define SPI_IER_UNDES (0x1u << 10) /**< \brief (SPI_IER) Underrun Error Interrupt Enable */
/* -------- SPI_IDR : (SPI Offset: 0x18) Interrupt Disable Register -------- */
#define SPI_IDR_RDRF (0x1u << 0) /**< \brief (SPI_IDR) Receive Data Register Full Interrupt Disable */
#define SPI_IDR_TDRE (0x1u << 1) /**< \brief (SPI_IDR) SPI Transmit Data Register Empty Interrupt Disable */
#define SPI_IDR_MODF (0x1u << 2) /**< \brief (SPI_IDR) Mode Fault Error Interrupt Disable */
#define SPI_IDR_OVRES (0x1u << 3) /**< \brief (SPI_IDR) Overrun Error Interrupt Disable */
#define SPI_IDR_ENDRX (0x1u << 4) /**< \brief (SPI_IDR) End of Receive Buffer Interrupt Disable */
#define SPI_IDR_ENDTX (0x1u << 5) /**< \brief (SPI_IDR) End of Transmit Buffer Interrupt Disable */
#define SPI_IDR_RXBUFF (0x1u << 6) /**< \brief (SPI_IDR) Receive Buffer Full Interrupt Disable */
#define SPI_IDR_TXBUFE (0x1u << 7) /**< \brief (SPI_IDR) Transmit Buffer Empty Interrupt Disable */
#define SPI_IDR_NSSR (0x1u << 8) /**< \brief (SPI_IDR) NSS Rising Interrupt Disable */
#define SPI_IDR_TXEMPTY (0x1u << 9) /**< \brief (SPI_IDR) Transmission Registers Empty Disable */
#define SPI_IDR_UNDES (0x1u << 10) /**< \brief (SPI_IDR) Underrun Error Interrupt Disable */
/* -------- SPI_IMR : (SPI Offset: 0x1C) Interrupt Mask Register -------- */
#define SPI_IMR_RDRF (0x1u << 0) /**< \brief (SPI_IMR) Receive Data Register Full Interrupt Mask */
#define SPI_IMR_TDRE (0x1u << 1) /**< \brief (SPI_IMR) SPI Transmit Data Register Empty Interrupt Mask */
#define SPI_IMR_MODF (0x1u << 2) /**< \brief (SPI_IMR) Mode Fault Error Interrupt Mask */
#define SPI_IMR_OVRES (0x1u << 3) /**< \brief (SPI_IMR) Overrun Error Interrupt Mask */
#define SPI_IMR_ENDRX (0x1u << 4) /**< \brief (SPI_IMR) End of Receive Buffer Interrupt Mask */
#define SPI_IMR_ENDTX (0x1u << 5) /**< \brief (SPI_IMR) End of Transmit Buffer Interrupt Mask */
#define SPI_IMR_RXBUFF (0x1u << 6) /**< \brief (SPI_IMR) Receive Buffer Full Interrupt Mask */
#define SPI_IMR_TXBUFE (0x1u << 7) /**< \brief (SPI_IMR) Transmit Buffer Empty Interrupt Mask */
#define SPI_IMR_NSSR (0x1u << 8) /**< \brief (SPI_IMR) NSS Rising Interrupt Mask */
#define SPI_IMR_TXEMPTY (0x1u << 9) /**< \brief (SPI_IMR) Transmission Registers Empty Mask */
#define SPI_IMR_UNDES (0x1u << 10) /**< \brief (SPI_IMR) Underrun Error Interrupt Mask */
/* -------- SPI_CSR[4] : (SPI Offset: 0x30) Chip Select Register -------- */
#define SPI_CSR_CPOL (0x1u << 0) /**< \brief (SPI_CSR[4]) Clock Polarity */
#define SPI_CSR_NCPHA (0x1u << 1) /**< \brief (SPI_CSR[4]) Clock Phase */
#define SPI_CSR_CSNAAT (0x1u << 2) /**< \brief (SPI_CSR[4]) Chip Select Not Active After Transfer (Ignored if CSAAT = 1) */
#define SPI_CSR_CSAAT (0x1u << 3) /**< \brief (SPI_CSR[4]) Chip Select Active After Transfer */
#define SPI_CSR_BITS_Pos 4
#define SPI_CSR_BITS_Msk (0xfu << SPI_CSR_BITS_Pos) /**< \brief (SPI_CSR[4]) Bits Per Transfer */
#define SPI_CSR_BITS(value) ((SPI_CSR_BITS_Msk & ((value) << SPI_CSR_BITS_Pos)))
#define   SPI_CSR_BITS_8_BIT (0x0u << 4) /**< \brief (SPI_CSR[4]) 8 bits for transfer */
#define   SPI_CSR_BITS_9_BIT (0x1u << 4) /**< \brief (SPI_CSR[4]) 9 bits for transfer */
#define   SPI_CSR_BITS_10_BIT (0x2u << 4) /**< \brief (SPI_CSR[4]) 10 bits for transfer */
#define   SPI_CSR_BITS_11_BIT (0x3u << 4) /**< \brief (SPI_CSR[4]) 11 bits for transfer */
#define   SPI_CSR_BITS_12_BIT (0x4u << 4) /**< \brief (SPI_CSR[4]) 12 bits for transfer */
#define   SPI_CSR_BITS_13_BIT (0x5u << 4) /**< \brief (SPI_CSR[4]) 13 bits for transfer */
#define   SPI_CSR_BITS_14_BIT (0x6u << 4) /**< \brief (SPI_CSR[4]) 14 bits for transfer */
#define   SPI_CSR_BITS_15_BIT (0x7u << 4) /**< \brief (SPI_CSR[4]) 15 bits for transfer */
#define   SPI_CSR_BITS_16_BIT (0x8u << 4) /**< \brief (SPI_CSR[4]) 16 bits for transfer */
#define SPI_CSR_SCBR_Pos 8
#define SPI_CSR_SCBR_Msk (0xffu << SPI_CSR_SCBR_Pos) /**< \brief (SPI_CSR[4]) Serial Clock Bit Rate */
#define SPI_CSR_SCBR(value) ((SPI_CSR_SCBR_Msk & ((value) << SPI_CSR_SCBR_Pos)))
#define SPI_CSR_DLYBS_Pos 16
#define SPI_CSR_DLYBS_Msk (0xffu << SPI_CSR_DLYBS_Pos) /**< \brief (SPI_CSR[4]) Delay Before SPCK */
#define SPI_CSR_DLYBS(value) ((SPI_CSR_DLYBS_Msk & ((value) << SPI_CSR_DLYBS_Pos)))
#define SPI_CSR_DLYBCT_Pos 24
#define SPI_CSR_DLYBCT_Msk (0xffu << SPI_CSR_DLYBCT_Pos) /**< \brief (SPI_CSR[4]) Delay Between Consecutive Transfers */
#define SPI_CSR_DLYBCT(value) ((SPI_CSR_DLYBCT_Msk & ((value) << SPI_CSR_DLYBCT_Pos)))
/* -------- SPI_WPMR : (SPI Offset: 0xE4) Write Protection Mode Register -------- */
#define SPI_WPMR_WPEN (0x1u << 0) /**< \brief (SPI_WPMR) Write Protection Enable */
#define SPI_WPMR_WPKEY_Pos 8
#define SPI_WPMR_WPKEY_Msk (0xffffffu << SPI_WPMR_WPKEY_Pos) /**< \brief (SPI_WPMR) Write Protection Key */
#define SPI_WPMR_WPKEY(value) ((SPI_WPMR_WPKEY_Msk & ((value) << SPI_WPMR_WPKEY_Pos)))
#define   SPI_WPMR_WPKEY_PASSWD (0x535049u << 8) /**< \brief (SPI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. */
/* -------- SPI_WPSR : (SPI Offset: 0xE8) Write Protection Status Register -------- */
#define SPI_WPSR_WPVS (0x1u << 0) /**< \brief (SPI_WPSR) Write Protection Violation Status */
#define SPI_WPSR_WPVSRC_Pos 8
#define SPI_WPSR_WPVSRC_Msk (0xffu << SPI_WPSR_WPVSRC_Pos) /**< \brief (SPI_WPSR) Write Protection Violation Source */
/* -------- SPI_RPR : (SPI Offset: 0x100) Receive Pointer Register -------- */
#define SPI_RPR_RXPTR_Pos 0
#define SPI_RPR_RXPTR_Msk (0xffffffffu << SPI_RPR_RXPTR_Pos) /**< \brief (SPI_RPR) Receive Pointer Register */
#define SPI_RPR_RXPTR(value) ((SPI_RPR_RXPTR_Msk & ((value) << SPI_RPR_RXPTR_Pos)))
/* -------- SPI_RCR : (SPI Offset: 0x104) Receive Counter Register -------- */
#define SPI_RCR_RXCTR_Pos 0
#define SPI_RCR_RXCTR_Msk (0xffffu << SPI_RCR_RXCTR_Pos) /**< \brief (SPI_RCR) Receive Counter Register */
#define SPI_RCR_RXCTR(value) ((SPI_RCR_RXCTR_Msk & ((value) << SPI_RCR_RXCTR_Pos)))
/* -------- SPI_TPR : (SPI Offset: 0x108) Transmit Pointer Register -------- */
#define SPI_TPR_TXPTR_Pos 0
#define SPI_TPR_TXPTR_Msk (0xffffffffu << SPI_TPR_TXPTR_Pos) /**< \brief (SPI_TPR) Transmit Counter Register */
#define SPI_TPR_TXPTR(value) ((SPI_TPR_TXPTR_Msk & ((value) << SPI_TPR_TXPTR_Pos)))
/* -------- SPI_TCR : (SPI Offset: 0x10C) Transmit Counter Register -------- */
#define SPI_TCR_TXCTR_Pos 0
#define SPI_TCR_TXCTR_Msk (0xffffu << SPI_TCR_TXCTR_Pos) /**< \brief (SPI_TCR) Transmit Counter Register */
#define SPI_TCR_TXCTR(value) ((SPI_TCR_TXCTR_Msk & ((value) << SPI_TCR_TXCTR_Pos)))
/* -------- SPI_RNPR : (SPI Offset: 0x110) Receive Next Pointer Register -------- */
#define SPI_RNPR_RXNPTR_Pos 0
#define SPI_RNPR_RXNPTR_Msk (0xffffffffu << SPI_RNPR_RXNPTR_Pos) /**< \brief (SPI_RNPR) Receive Next Pointer */
#define SPI_RNPR_RXNPTR(value) ((SPI_RNPR_RXNPTR_Msk & ((value) << SPI_RNPR_RXNPTR_Pos)))
/* -------- SPI_RNCR : (SPI Offset: 0x114) Receive Next Counter Register -------- */
#define SPI_RNCR_RXNCTR_Pos 0
#define SPI_RNCR_RXNCTR_Msk (0xffffu << SPI_RNCR_RXNCTR_Pos) /**< \brief (SPI_RNCR) Receive Next Counter */
#define SPI_RNCR_RXNCTR(value) ((SPI_RNCR_RXNCTR_Msk & ((value) << SPI_RNCR_RXNCTR_Pos)))
/* -------- SPI_TNPR : (SPI Offset: 0x118) Transmit Next Pointer Register -------- */
#define SPI_TNPR_TXNPTR_Pos 0
#define SPI_TNPR_TXNPTR_Msk (0xffffffffu << SPI_TNPR_TXNPTR_Pos) /**< \brief (SPI_TNPR) Transmit Next Pointer */
#define SPI_TNPR_TXNPTR(value) ((SPI_TNPR_TXNPTR_Msk & ((value) << SPI_TNPR_TXNPTR_Pos)))
/* -------- SPI_TNCR : (SPI Offset: 0x11C) Transmit Next Counter Register -------- */
#define SPI_TNCR_TXNCTR_Pos 0
#define SPI_TNCR_TXNCTR_Msk (0xffffu << SPI_TNCR_TXNCTR_Pos) /**< \brief (SPI_TNCR) Transmit Counter Next */
#define SPI_TNCR_TXNCTR(value) ((SPI_TNCR_TXNCTR_Msk & ((value) << SPI_TNCR_TXNCTR_Pos)))
/* -------- SPI_PTCR : (SPI Offset: 0x120) Transfer Control Register -------- */
#define SPI_PTCR_RXTEN (0x1u << 0) /**< \brief (SPI_PTCR) Receiver Transfer Enable */
#define SPI_PTCR_RXTDIS (0x1u << 1) /**< \brief (SPI_PTCR) Receiver Transfer Disable */
#define SPI_PTCR_TXTEN (0x1u << 8) /**< \brief (SPI_PTCR) Transmitter Transfer Enable */
#define SPI_PTCR_TXTDIS (0x1u << 9) /**< \brief (SPI_PTCR) Transmitter Transfer Disable */
/* -------- SPI_PTSR : (SPI Offset: 0x124) Transfer Status Register -------- */
#define SPI_PTSR_RXTEN (0x1u << 0) /**< \brief (SPI_PTSR) Receiver Transfer Enable */
#define SPI_PTSR_TXTEN (0x1u << 8) /**< \brief (SPI_PTSR) Transmitter Transfer Enable */

/*@}*/


#endif /* _SAM4S_SPI_COMPONENT_ */
