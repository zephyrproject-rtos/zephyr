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

#ifndef _SAM3XA_PDC_COMPONENT_
#define _SAM3XA_PDC_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Peripheral DMA Controller */
/* ============================================================================= */
/** \addtogroup SAM3XA_PDC Peripheral DMA Controller */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Pdc hardware registers */
typedef struct {
  __IO uint32_t PERIPH_RPR;  /**< \brief (Pdc Offset: 0x00) Receive Pointer Register */
  __IO uint32_t PERIPH_RCR;  /**< \brief (Pdc Offset: 0x04) Receive Counter Register */
  __IO uint32_t PERIPH_TPR;  /**< \brief (Pdc Offset: 0x08) Transmit Pointer Register */
  __IO uint32_t PERIPH_TCR;  /**< \brief (Pdc Offset: 0x0C) Transmit Counter Register */
  __IO uint32_t PERIPH_RNPR; /**< \brief (Pdc Offset: 0x10) Receive Next Pointer Register */
  __IO uint32_t PERIPH_RNCR; /**< \brief (Pdc Offset: 0x14) Receive Next Counter Register */
  __IO uint32_t PERIPH_TNPR; /**< \brief (Pdc Offset: 0x18) Transmit Next Pointer Register */
  __IO uint32_t PERIPH_TNCR; /**< \brief (Pdc Offset: 0x1C) Transmit Next Counter Register */
  __O  uint32_t PERIPH_PTCR; /**< \brief (Pdc Offset: 0x20) Transfer Control Register */
  __I  uint32_t PERIPH_PTSR; /**< \brief (Pdc Offset: 0x24) Transfer Status Register */
} Pdc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- PERIPH_RPR : (PDC Offset: 0x00) Receive Pointer Register -------- */
#define PERIPH_RPR_RXPTR_Pos 0
#define PERIPH_RPR_RXPTR_Msk (0xffffffffu << PERIPH_RPR_RXPTR_Pos) /**< \brief (PERIPH_RPR) Receive Pointer Register */
#define PERIPH_RPR_RXPTR(value) ((PERIPH_RPR_RXPTR_Msk & ((value) << PERIPH_RPR_RXPTR_Pos)))
/* -------- PERIPH_RCR : (PDC Offset: 0x04) Receive Counter Register -------- */
#define PERIPH_RCR_RXCTR_Pos 0
#define PERIPH_RCR_RXCTR_Msk (0xffffu << PERIPH_RCR_RXCTR_Pos) /**< \brief (PERIPH_RCR) Receive Counter Register */
#define PERIPH_RCR_RXCTR(value) ((PERIPH_RCR_RXCTR_Msk & ((value) << PERIPH_RCR_RXCTR_Pos)))
/* -------- PERIPH_TPR : (PDC Offset: 0x08) Transmit Pointer Register -------- */
#define PERIPH_TPR_TXPTR_Pos 0
#define PERIPH_TPR_TXPTR_Msk (0xffffffffu << PERIPH_TPR_TXPTR_Pos) /**< \brief (PERIPH_TPR) Transmit Counter Register */
#define PERIPH_TPR_TXPTR(value) ((PERIPH_TPR_TXPTR_Msk & ((value) << PERIPH_TPR_TXPTR_Pos)))
/* -------- PERIPH_TCR : (PDC Offset: 0x0C) Transmit Counter Register -------- */
#define PERIPH_TCR_TXCTR_Pos 0
#define PERIPH_TCR_TXCTR_Msk (0xffffu << PERIPH_TCR_TXCTR_Pos) /**< \brief (PERIPH_TCR) Transmit Counter Register */
#define PERIPH_TCR_TXCTR(value) ((PERIPH_TCR_TXCTR_Msk & ((value) << PERIPH_TCR_TXCTR_Pos)))
/* -------- PERIPH_RNPR : (PDC Offset: 0x10) Receive Next Pointer Register -------- */
#define PERIPH_RNPR_RXNPTR_Pos 0
#define PERIPH_RNPR_RXNPTR_Msk (0xffffffffu << PERIPH_RNPR_RXNPTR_Pos) /**< \brief (PERIPH_RNPR) Receive Next Pointer */
#define PERIPH_RNPR_RXNPTR(value) ((PERIPH_RNPR_RXNPTR_Msk & ((value) << PERIPH_RNPR_RXNPTR_Pos)))
/* -------- PERIPH_RNCR : (PDC Offset: 0x14) Receive Next Counter Register -------- */
#define PERIPH_RNCR_RXNCTR_Pos 0
#define PERIPH_RNCR_RXNCTR_Msk (0xffffu << PERIPH_RNCR_RXNCTR_Pos) /**< \brief (PERIPH_RNCR) Receive Next Counter */
#define PERIPH_RNCR_RXNCTR(value) ((PERIPH_RNCR_RXNCTR_Msk & ((value) << PERIPH_RNCR_RXNCTR_Pos)))
/* -------- PERIPH_TNPR : (PDC Offset: 0x18) Transmit Next Pointer Register -------- */
#define PERIPH_TNPR_TXNPTR_Pos 0
#define PERIPH_TNPR_TXNPTR_Msk (0xffffffffu << PERIPH_TNPR_TXNPTR_Pos) /**< \brief (PERIPH_TNPR) Transmit Next Pointer */
#define PERIPH_TNPR_TXNPTR(value) ((PERIPH_TNPR_TXNPTR_Msk & ((value) << PERIPH_TNPR_TXNPTR_Pos)))
/* -------- PERIPH_TNCR : (PDC Offset: 0x1C) Transmit Next Counter Register -------- */
#define PERIPH_TNCR_TXNCTR_Pos 0
#define PERIPH_TNCR_TXNCTR_Msk (0xffffu << PERIPH_TNCR_TXNCTR_Pos) /**< \brief (PERIPH_TNCR) Transmit Counter Next */
#define PERIPH_TNCR_TXNCTR(value) ((PERIPH_TNCR_TXNCTR_Msk & ((value) << PERIPH_TNCR_TXNCTR_Pos)))
/* -------- PERIPH_PTCR : (PDC Offset: 0x20) Transfer Control Register -------- */
#define PERIPH_PTCR_RXTEN (0x1u << 0) /**< \brief (PERIPH_PTCR) Receiver Transfer Enable */
#define PERIPH_PTCR_RXTDIS (0x1u << 1) /**< \brief (PERIPH_PTCR) Receiver Transfer Disable */
#define PERIPH_PTCR_TXTEN (0x1u << 8) /**< \brief (PERIPH_PTCR) Transmitter Transfer Enable */
#define PERIPH_PTCR_TXTDIS (0x1u << 9) /**< \brief (PERIPH_PTCR) Transmitter Transfer Disable */
/* -------- PERIPH_PTSR : (PDC Offset: 0x24) Transfer Status Register -------- */
#define PERIPH_PTSR_RXTEN (0x1u << 0) /**< \brief (PERIPH_PTSR) Receiver Transfer Enable */
#define PERIPH_PTSR_TXTEN (0x1u << 8) /**< \brief (PERIPH_PTSR) Transmitter Transfer Enable */

/*@}*/


#endif /* _SAM3XA_PDC_COMPONENT_ */
