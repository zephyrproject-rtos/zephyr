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

#ifndef _SAM4S_SSC_COMPONENT_
#define _SAM4S_SSC_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Synchronous Serial Controller */
/* ============================================================================= */
/** \addtogroup SAM4S_SSC Synchronous Serial Controller */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Ssc hardware registers */
typedef struct {
  __O  uint32_t SSC_CR;        /**< \brief (Ssc Offset: 0x0) Control Register */
  __IO uint32_t SSC_CMR;       /**< \brief (Ssc Offset: 0x4) Clock Mode Register */
  __I  uint32_t Reserved1[2];
  __IO uint32_t SSC_RCMR;      /**< \brief (Ssc Offset: 0x10) Receive Clock Mode Register */
  __IO uint32_t SSC_RFMR;      /**< \brief (Ssc Offset: 0x14) Receive Frame Mode Register */
  __IO uint32_t SSC_TCMR;      /**< \brief (Ssc Offset: 0x18) Transmit Clock Mode Register */
  __IO uint32_t SSC_TFMR;      /**< \brief (Ssc Offset: 0x1C) Transmit Frame Mode Register */
  __I  uint32_t SSC_RHR;       /**< \brief (Ssc Offset: 0x20) Receive Holding Register */
  __O  uint32_t SSC_THR;       /**< \brief (Ssc Offset: 0x24) Transmit Holding Register */
  __I  uint32_t Reserved2[2];
  __I  uint32_t SSC_RSHR;      /**< \brief (Ssc Offset: 0x30) Receive Sync. Holding Register */
  __IO uint32_t SSC_TSHR;      /**< \brief (Ssc Offset: 0x34) Transmit Sync. Holding Register */
  __IO uint32_t SSC_RC0R;      /**< \brief (Ssc Offset: 0x38) Receive Compare 0 Register */
  __IO uint32_t SSC_RC1R;      /**< \brief (Ssc Offset: 0x3C) Receive Compare 1 Register */
  __I  uint32_t SSC_SR;        /**< \brief (Ssc Offset: 0x40) Status Register */
  __O  uint32_t SSC_IER;       /**< \brief (Ssc Offset: 0x44) Interrupt Enable Register */
  __O  uint32_t SSC_IDR;       /**< \brief (Ssc Offset: 0x48) Interrupt Disable Register */
  __I  uint32_t SSC_IMR;       /**< \brief (Ssc Offset: 0x4C) Interrupt Mask Register */
  __I  uint32_t Reserved3[37];
  __IO uint32_t SSC_WPMR;      /**< \brief (Ssc Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t SSC_WPSR;      /**< \brief (Ssc Offset: 0xE8) Write Protection Status Register */
  __I  uint32_t Reserved4[5];
  __IO uint32_t SSC_RPR;       /**< \brief (Ssc Offset: 0x100) Receive Pointer Register */
  __IO uint32_t SSC_RCR;       /**< \brief (Ssc Offset: 0x104) Receive Counter Register */
  __IO uint32_t SSC_TPR;       /**< \brief (Ssc Offset: 0x108) Transmit Pointer Register */
  __IO uint32_t SSC_TCR;       /**< \brief (Ssc Offset: 0x10C) Transmit Counter Register */
  __IO uint32_t SSC_RNPR;      /**< \brief (Ssc Offset: 0x110) Receive Next Pointer Register */
  __IO uint32_t SSC_RNCR;      /**< \brief (Ssc Offset: 0x114) Receive Next Counter Register */
  __IO uint32_t SSC_TNPR;      /**< \brief (Ssc Offset: 0x118) Transmit Next Pointer Register */
  __IO uint32_t SSC_TNCR;      /**< \brief (Ssc Offset: 0x11C) Transmit Next Counter Register */
  __O  uint32_t SSC_PTCR;      /**< \brief (Ssc Offset: 0x120) Transfer Control Register */
  __I  uint32_t SSC_PTSR;      /**< \brief (Ssc Offset: 0x124) Transfer Status Register */
} Ssc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- SSC_CR : (SSC Offset: 0x0) Control Register -------- */
#define SSC_CR_RXEN (0x1u << 0) /**< \brief (SSC_CR) Receive Enable */
#define SSC_CR_RXDIS (0x1u << 1) /**< \brief (SSC_CR) Receive Disable */
#define SSC_CR_TXEN (0x1u << 8) /**< \brief (SSC_CR) Transmit Enable */
#define SSC_CR_TXDIS (0x1u << 9) /**< \brief (SSC_CR) Transmit Disable */
#define SSC_CR_SWRST (0x1u << 15) /**< \brief (SSC_CR) Software Reset */
/* -------- SSC_CMR : (SSC Offset: 0x4) Clock Mode Register -------- */
#define SSC_CMR_DIV_Pos 0
#define SSC_CMR_DIV_Msk (0xfffu << SSC_CMR_DIV_Pos) /**< \brief (SSC_CMR) Clock Divider */
#define SSC_CMR_DIV(value) ((SSC_CMR_DIV_Msk & ((value) << SSC_CMR_DIV_Pos)))
/* -------- SSC_RCMR : (SSC Offset: 0x10) Receive Clock Mode Register -------- */
#define SSC_RCMR_CKS_Pos 0
#define SSC_RCMR_CKS_Msk (0x3u << SSC_RCMR_CKS_Pos) /**< \brief (SSC_RCMR) Receive Clock Selection */
#define SSC_RCMR_CKS(value) ((SSC_RCMR_CKS_Msk & ((value) << SSC_RCMR_CKS_Pos)))
#define   SSC_RCMR_CKS_MCK (0x0u << 0) /**< \brief (SSC_RCMR) Divided Clock */
#define   SSC_RCMR_CKS_TK (0x1u << 0) /**< \brief (SSC_RCMR) TK Clock signal */
#define   SSC_RCMR_CKS_RK (0x2u << 0) /**< \brief (SSC_RCMR) RK pin */
#define SSC_RCMR_CKO_Pos 2
#define SSC_RCMR_CKO_Msk (0x7u << SSC_RCMR_CKO_Pos) /**< \brief (SSC_RCMR) Receive Clock Output Mode Selection */
#define SSC_RCMR_CKO(value) ((SSC_RCMR_CKO_Msk & ((value) << SSC_RCMR_CKO_Pos)))
#define   SSC_RCMR_CKO_NONE (0x0u << 2) /**< \brief (SSC_RCMR) None, RK pin is an input */
#define   SSC_RCMR_CKO_CONTINUOUS (0x1u << 2) /**< \brief (SSC_RCMR) Continuous Receive Clock, RK pin is an output */
#define   SSC_RCMR_CKO_TRANSFER (0x2u << 2) /**< \brief (SSC_RCMR) Receive Clock only during data transfers, RK pin is an output */
#define SSC_RCMR_CKI (0x1u << 5) /**< \brief (SSC_RCMR) Receive Clock Inversion */
#define SSC_RCMR_CKG_Pos 6
#define SSC_RCMR_CKG_Msk (0x3u << SSC_RCMR_CKG_Pos) /**< \brief (SSC_RCMR) Receive Clock Gating Selection */
#define SSC_RCMR_CKG(value) ((SSC_RCMR_CKG_Msk & ((value) << SSC_RCMR_CKG_Pos)))
#define   SSC_RCMR_CKG_CONTINUOUS (0x0u << 6) /**< \brief (SSC_RCMR) None */
#define   SSC_RCMR_CKG_EN_RF_LOW (0x1u << 6) /**< \brief (SSC_RCMR) Receive Clock enabled only if RF Low */
#define   SSC_RCMR_CKG_EN_RF_HIGH (0x2u << 6) /**< \brief (SSC_RCMR) Receive Clock enabled only if RF High */
#define SSC_RCMR_START_Pos 8
#define SSC_RCMR_START_Msk (0xfu << SSC_RCMR_START_Pos) /**< \brief (SSC_RCMR) Receive Start Selection */
#define SSC_RCMR_START(value) ((SSC_RCMR_START_Msk & ((value) << SSC_RCMR_START_Pos)))
#define   SSC_RCMR_START_CONTINUOUS (0x0u << 8) /**< \brief (SSC_RCMR) Continuous, as soon as the receiver is enabled, and immediately after the end of transfer of the previous data. */
#define   SSC_RCMR_START_TRANSMIT (0x1u << 8) /**< \brief (SSC_RCMR) Transmit start */
#define   SSC_RCMR_START_RF_LOW (0x2u << 8) /**< \brief (SSC_RCMR) Detection of a low level on RF signal */
#define   SSC_RCMR_START_RF_HIGH (0x3u << 8) /**< \brief (SSC_RCMR) Detection of a high level on RF signal */
#define   SSC_RCMR_START_RF_FALLING (0x4u << 8) /**< \brief (SSC_RCMR) Detection of a falling edge on RF signal */
#define   SSC_RCMR_START_RF_RISING (0x5u << 8) /**< \brief (SSC_RCMR) Detection of a rising edge on RF signal */
#define   SSC_RCMR_START_RF_LEVEL (0x6u << 8) /**< \brief (SSC_RCMR) Detection of any level change on RF signal */
#define   SSC_RCMR_START_RF_EDGE (0x7u << 8) /**< \brief (SSC_RCMR) Detection of any edge on RF signal */
#define   SSC_RCMR_START_CMP_0 (0x8u << 8) /**< \brief (SSC_RCMR) Compare 0 */
#define SSC_RCMR_STOP (0x1u << 12) /**< \brief (SSC_RCMR) Receive Stop Selection */
#define SSC_RCMR_STTDLY_Pos 16
#define SSC_RCMR_STTDLY_Msk (0xffu << SSC_RCMR_STTDLY_Pos) /**< \brief (SSC_RCMR) Receive Start Delay */
#define SSC_RCMR_STTDLY(value) ((SSC_RCMR_STTDLY_Msk & ((value) << SSC_RCMR_STTDLY_Pos)))
#define SSC_RCMR_PERIOD_Pos 24
#define SSC_RCMR_PERIOD_Msk (0xffu << SSC_RCMR_PERIOD_Pos) /**< \brief (SSC_RCMR) Receive Period Divider Selection */
#define SSC_RCMR_PERIOD(value) ((SSC_RCMR_PERIOD_Msk & ((value) << SSC_RCMR_PERIOD_Pos)))
/* -------- SSC_RFMR : (SSC Offset: 0x14) Receive Frame Mode Register -------- */
#define SSC_RFMR_DATLEN_Pos 0
#define SSC_RFMR_DATLEN_Msk (0x1fu << SSC_RFMR_DATLEN_Pos) /**< \brief (SSC_RFMR) Data Length */
#define SSC_RFMR_DATLEN(value) ((SSC_RFMR_DATLEN_Msk & ((value) << SSC_RFMR_DATLEN_Pos)))
#define SSC_RFMR_LOOP (0x1u << 5) /**< \brief (SSC_RFMR) Loop Mode */
#define SSC_RFMR_MSBF (0x1u << 7) /**< \brief (SSC_RFMR) Most Significant Bit First */
#define SSC_RFMR_DATNB_Pos 8
#define SSC_RFMR_DATNB_Msk (0xfu << SSC_RFMR_DATNB_Pos) /**< \brief (SSC_RFMR) Data Number per Frame */
#define SSC_RFMR_DATNB(value) ((SSC_RFMR_DATNB_Msk & ((value) << SSC_RFMR_DATNB_Pos)))
#define SSC_RFMR_FSLEN_Pos 16
#define SSC_RFMR_FSLEN_Msk (0xfu << SSC_RFMR_FSLEN_Pos) /**< \brief (SSC_RFMR) Receive Frame Sync Length */
#define SSC_RFMR_FSLEN(value) ((SSC_RFMR_FSLEN_Msk & ((value) << SSC_RFMR_FSLEN_Pos)))
#define SSC_RFMR_FSOS_Pos 20
#define SSC_RFMR_FSOS_Msk (0x7u << SSC_RFMR_FSOS_Pos) /**< \brief (SSC_RFMR) Receive Frame Sync Output Selection */
#define SSC_RFMR_FSOS(value) ((SSC_RFMR_FSOS_Msk & ((value) << SSC_RFMR_FSOS_Pos)))
#define   SSC_RFMR_FSOS_NONE (0x0u << 20) /**< \brief (SSC_RFMR) None, RF pin is an input */
#define   SSC_RFMR_FSOS_NEGATIVE (0x1u << 20) /**< \brief (SSC_RFMR) Negative Pulse, RF pin is an output */
#define   SSC_RFMR_FSOS_POSITIVE (0x2u << 20) /**< \brief (SSC_RFMR) Positive Pulse, RF pin is an output */
#define   SSC_RFMR_FSOS_LOW (0x3u << 20) /**< \brief (SSC_RFMR) Driven Low during data transfer, RF pin is an output */
#define   SSC_RFMR_FSOS_HIGH (0x4u << 20) /**< \brief (SSC_RFMR) Driven High during data transfer, RF pin is an output */
#define   SSC_RFMR_FSOS_TOGGLING (0x5u << 20) /**< \brief (SSC_RFMR) Toggling at each start of data transfer, RF pin is an output */
#define SSC_RFMR_FSEDGE (0x1u << 24) /**< \brief (SSC_RFMR) Frame Sync Edge Detection */
#define   SSC_RFMR_FSEDGE_POSITIVE (0x0u << 24) /**< \brief (SSC_RFMR) Positive Edge Detection */
#define   SSC_RFMR_FSEDGE_NEGATIVE (0x1u << 24) /**< \brief (SSC_RFMR) Negative Edge Detection */
#define SSC_RFMR_FSLEN_EXT_Pos 28
#define SSC_RFMR_FSLEN_EXT_Msk (0xfu << SSC_RFMR_FSLEN_EXT_Pos) /**< \brief (SSC_RFMR) FSLEN Field Extension */
#define SSC_RFMR_FSLEN_EXT(value) ((SSC_RFMR_FSLEN_EXT_Msk & ((value) << SSC_RFMR_FSLEN_EXT_Pos)))
/* -------- SSC_TCMR : (SSC Offset: 0x18) Transmit Clock Mode Register -------- */
#define SSC_TCMR_CKS_Pos 0
#define SSC_TCMR_CKS_Msk (0x3u << SSC_TCMR_CKS_Pos) /**< \brief (SSC_TCMR) Transmit Clock Selection */
#define SSC_TCMR_CKS(value) ((SSC_TCMR_CKS_Msk & ((value) << SSC_TCMR_CKS_Pos)))
#define   SSC_TCMR_CKS_MCK (0x0u << 0) /**< \brief (SSC_TCMR) Divided Clock */
#define   SSC_TCMR_CKS_RK (0x1u << 0) /**< \brief (SSC_TCMR) RK Clock signal */
#define   SSC_TCMR_CKS_TK (0x2u << 0) /**< \brief (SSC_TCMR) TK pin */
#define SSC_TCMR_CKO_Pos 2
#define SSC_TCMR_CKO_Msk (0x7u << SSC_TCMR_CKO_Pos) /**< \brief (SSC_TCMR) Transmit Clock Output Mode Selection */
#define SSC_TCMR_CKO(value) ((SSC_TCMR_CKO_Msk & ((value) << SSC_TCMR_CKO_Pos)))
#define   SSC_TCMR_CKO_NONE (0x0u << 2) /**< \brief (SSC_TCMR) None, TK pin is an input */
#define   SSC_TCMR_CKO_CONTINUOUS (0x1u << 2) /**< \brief (SSC_TCMR) Continuous Transmit Clock, TK pin is an output */
#define   SSC_TCMR_CKO_TRANSFER (0x2u << 2) /**< \brief (SSC_TCMR) Transmit Clock only during data transfers, TK pin is an output */
#define SSC_TCMR_CKI (0x1u << 5) /**< \brief (SSC_TCMR) Transmit Clock Inversion */
#define SSC_TCMR_CKG_Pos 6
#define SSC_TCMR_CKG_Msk (0x3u << SSC_TCMR_CKG_Pos) /**< \brief (SSC_TCMR) Transmit Clock Gating Selection */
#define SSC_TCMR_CKG(value) ((SSC_TCMR_CKG_Msk & ((value) << SSC_TCMR_CKG_Pos)))
#define   SSC_TCMR_CKG_CONTINUOUS (0x0u << 6) /**< \brief (SSC_TCMR) None */
#define   SSC_TCMR_CKG_EN_TF_LOW (0x1u << 6) /**< \brief (SSC_TCMR) Transmit Clock enabled only if TF Low */
#define   SSC_TCMR_CKG_EN_TF_HIGH (0x2u << 6) /**< \brief (SSC_TCMR) Transmit Clock enabled only if TF High */
#define SSC_TCMR_START_Pos 8
#define SSC_TCMR_START_Msk (0xfu << SSC_TCMR_START_Pos) /**< \brief (SSC_TCMR) Transmit Start Selection */
#define SSC_TCMR_START(value) ((SSC_TCMR_START_Msk & ((value) << SSC_TCMR_START_Pos)))
#define   SSC_TCMR_START_CONTINUOUS (0x0u << 8) /**< \brief (SSC_TCMR) Continuous, as soon as a word is written in the SSC_THR (if Transmit is enabled), and immediately after the end of transfer of the previous data */
#define   SSC_TCMR_START_RECEIVE (0x1u << 8) /**< \brief (SSC_TCMR) Receive start */
#define   SSC_TCMR_START_TF_LOW (0x2u << 8) /**< \brief (SSC_TCMR) Detection of a low level on TF signal */
#define   SSC_TCMR_START_TF_HIGH (0x3u << 8) /**< \brief (SSC_TCMR) Detection of a high level on TF signal */
#define   SSC_TCMR_START_TF_FALLING (0x4u << 8) /**< \brief (SSC_TCMR) Detection of a falling edge on TF signal */
#define   SSC_TCMR_START_TF_RISING (0x5u << 8) /**< \brief (SSC_TCMR) Detection of a rising edge on TF signal */
#define   SSC_TCMR_START_TF_LEVEL (0x6u << 8) /**< \brief (SSC_TCMR) Detection of any level change on TF signal */
#define   SSC_TCMR_START_TF_EDGE (0x7u << 8) /**< \brief (SSC_TCMR) Detection of any edge on TF signal */
#define SSC_TCMR_STTDLY_Pos 16
#define SSC_TCMR_STTDLY_Msk (0xffu << SSC_TCMR_STTDLY_Pos) /**< \brief (SSC_TCMR) Transmit Start Delay */
#define SSC_TCMR_STTDLY(value) ((SSC_TCMR_STTDLY_Msk & ((value) << SSC_TCMR_STTDLY_Pos)))
#define SSC_TCMR_PERIOD_Pos 24
#define SSC_TCMR_PERIOD_Msk (0xffu << SSC_TCMR_PERIOD_Pos) /**< \brief (SSC_TCMR) Transmit Period Divider Selection */
#define SSC_TCMR_PERIOD(value) ((SSC_TCMR_PERIOD_Msk & ((value) << SSC_TCMR_PERIOD_Pos)))
/* -------- SSC_TFMR : (SSC Offset: 0x1C) Transmit Frame Mode Register -------- */
#define SSC_TFMR_DATLEN_Pos 0
#define SSC_TFMR_DATLEN_Msk (0x1fu << SSC_TFMR_DATLEN_Pos) /**< \brief (SSC_TFMR) Data Length */
#define SSC_TFMR_DATLEN(value) ((SSC_TFMR_DATLEN_Msk & ((value) << SSC_TFMR_DATLEN_Pos)))
#define SSC_TFMR_DATDEF (0x1u << 5) /**< \brief (SSC_TFMR) Data Default Value */
#define SSC_TFMR_MSBF (0x1u << 7) /**< \brief (SSC_TFMR) Most Significant Bit First */
#define SSC_TFMR_DATNB_Pos 8
#define SSC_TFMR_DATNB_Msk (0xfu << SSC_TFMR_DATNB_Pos) /**< \brief (SSC_TFMR) Data Number per Frame */
#define SSC_TFMR_DATNB(value) ((SSC_TFMR_DATNB_Msk & ((value) << SSC_TFMR_DATNB_Pos)))
#define SSC_TFMR_FSLEN_Pos 16
#define SSC_TFMR_FSLEN_Msk (0xfu << SSC_TFMR_FSLEN_Pos) /**< \brief (SSC_TFMR) Transmit Frame Sync Length */
#define SSC_TFMR_FSLEN(value) ((SSC_TFMR_FSLEN_Msk & ((value) << SSC_TFMR_FSLEN_Pos)))
#define SSC_TFMR_FSOS_Pos 20
#define SSC_TFMR_FSOS_Msk (0x7u << SSC_TFMR_FSOS_Pos) /**< \brief (SSC_TFMR) Transmit Frame Sync Output Selection */
#define SSC_TFMR_FSOS(value) ((SSC_TFMR_FSOS_Msk & ((value) << SSC_TFMR_FSOS_Pos)))
#define   SSC_TFMR_FSOS_NONE (0x0u << 20) /**< \brief (SSC_TFMR) None, TF pin is an input */
#define   SSC_TFMR_FSOS_NEGATIVE (0x1u << 20) /**< \brief (SSC_TFMR) Negative Pulse, TF pin is an output */
#define   SSC_TFMR_FSOS_POSITIVE (0x2u << 20) /**< \brief (SSC_TFMR) Positive Pulse, TF pin is an output */
#define   SSC_TFMR_FSOS_LOW (0x3u << 20) /**< \brief (SSC_TFMR) Driven Low during data transfer */
#define   SSC_TFMR_FSOS_HIGH (0x4u << 20) /**< \brief (SSC_TFMR) Driven High during data transfer */
#define   SSC_TFMR_FSOS_TOGGLING (0x5u << 20) /**< \brief (SSC_TFMR) Toggling at each start of data transfer */
#define SSC_TFMR_FSDEN (0x1u << 23) /**< \brief (SSC_TFMR) Frame Sync Data Enable */
#define SSC_TFMR_FSEDGE (0x1u << 24) /**< \brief (SSC_TFMR) Frame Sync Edge Detection */
#define   SSC_TFMR_FSEDGE_POSITIVE (0x0u << 24) /**< \brief (SSC_TFMR) Positive Edge Detection */
#define   SSC_TFMR_FSEDGE_NEGATIVE (0x1u << 24) /**< \brief (SSC_TFMR) Negative Edge Detection */
#define SSC_TFMR_FSLEN_EXT_Pos 28
#define SSC_TFMR_FSLEN_EXT_Msk (0xfu << SSC_TFMR_FSLEN_EXT_Pos) /**< \brief (SSC_TFMR) FSLEN Field Extension */
#define SSC_TFMR_FSLEN_EXT(value) ((SSC_TFMR_FSLEN_EXT_Msk & ((value) << SSC_TFMR_FSLEN_EXT_Pos)))
/* -------- SSC_RHR : (SSC Offset: 0x20) Receive Holding Register -------- */
#define SSC_RHR_RDAT_Pos 0
#define SSC_RHR_RDAT_Msk (0xffffffffu << SSC_RHR_RDAT_Pos) /**< \brief (SSC_RHR) Receive Data */
/* -------- SSC_THR : (SSC Offset: 0x24) Transmit Holding Register -------- */
#define SSC_THR_TDAT_Pos 0
#define SSC_THR_TDAT_Msk (0xffffffffu << SSC_THR_TDAT_Pos) /**< \brief (SSC_THR) Transmit Data */
#define SSC_THR_TDAT(value) ((SSC_THR_TDAT_Msk & ((value) << SSC_THR_TDAT_Pos)))
/* -------- SSC_RSHR : (SSC Offset: 0x30) Receive Sync. Holding Register -------- */
#define SSC_RSHR_RSDAT_Pos 0
#define SSC_RSHR_RSDAT_Msk (0xffffu << SSC_RSHR_RSDAT_Pos) /**< \brief (SSC_RSHR) Receive Synchronization Data */
/* -------- SSC_TSHR : (SSC Offset: 0x34) Transmit Sync. Holding Register -------- */
#define SSC_TSHR_TSDAT_Pos 0
#define SSC_TSHR_TSDAT_Msk (0xffffu << SSC_TSHR_TSDAT_Pos) /**< \brief (SSC_TSHR) Transmit Synchronization Data */
#define SSC_TSHR_TSDAT(value) ((SSC_TSHR_TSDAT_Msk & ((value) << SSC_TSHR_TSDAT_Pos)))
/* -------- SSC_RC0R : (SSC Offset: 0x38) Receive Compare 0 Register -------- */
#define SSC_RC0R_CP0_Pos 0
#define SSC_RC0R_CP0_Msk (0xffffu << SSC_RC0R_CP0_Pos) /**< \brief (SSC_RC0R) Receive Compare Data 0 */
#define SSC_RC0R_CP0(value) ((SSC_RC0R_CP0_Msk & ((value) << SSC_RC0R_CP0_Pos)))
/* -------- SSC_RC1R : (SSC Offset: 0x3C) Receive Compare 1 Register -------- */
#define SSC_RC1R_CP1_Pos 0
#define SSC_RC1R_CP1_Msk (0xffffu << SSC_RC1R_CP1_Pos) /**< \brief (SSC_RC1R) Receive Compare Data 1 */
#define SSC_RC1R_CP1(value) ((SSC_RC1R_CP1_Msk & ((value) << SSC_RC1R_CP1_Pos)))
/* -------- SSC_SR : (SSC Offset: 0x40) Status Register -------- */
#define SSC_SR_TXRDY (0x1u << 0) /**< \brief (SSC_SR) Transmit Ready */
#define SSC_SR_TXEMPTY (0x1u << 1) /**< \brief (SSC_SR) Transmit Empty */
#define SSC_SR_ENDTX (0x1u << 2) /**< \brief (SSC_SR) End of Transmission */
#define SSC_SR_TXBUFE (0x1u << 3) /**< \brief (SSC_SR) Transmit Buffer Empty */
#define SSC_SR_RXRDY (0x1u << 4) /**< \brief (SSC_SR) Receive Ready */
#define SSC_SR_OVRUN (0x1u << 5) /**< \brief (SSC_SR) Receive Overrun */
#define SSC_SR_ENDRX (0x1u << 6) /**< \brief (SSC_SR) End of Reception */
#define SSC_SR_RXBUFF (0x1u << 7) /**< \brief (SSC_SR) Receive Buffer Full */
#define SSC_SR_CP0 (0x1u << 8) /**< \brief (SSC_SR) Compare 0 */
#define SSC_SR_CP1 (0x1u << 9) /**< \brief (SSC_SR) Compare 1 */
#define SSC_SR_TXSYN (0x1u << 10) /**< \brief (SSC_SR) Transmit Sync */
#define SSC_SR_RXSYN (0x1u << 11) /**< \brief (SSC_SR) Receive Sync */
#define SSC_SR_TXEN (0x1u << 16) /**< \brief (SSC_SR) Transmit Enable */
#define SSC_SR_RXEN (0x1u << 17) /**< \brief (SSC_SR) Receive Enable */
/* -------- SSC_IER : (SSC Offset: 0x44) Interrupt Enable Register -------- */
#define SSC_IER_TXRDY (0x1u << 0) /**< \brief (SSC_IER) Transmit Ready Interrupt Enable */
#define SSC_IER_TXEMPTY (0x1u << 1) /**< \brief (SSC_IER) Transmit Empty Interrupt Enable */
#define SSC_IER_ENDTX (0x1u << 2) /**< \brief (SSC_IER) End of Transmission Interrupt Enable */
#define SSC_IER_TXBUFE (0x1u << 3) /**< \brief (SSC_IER) Transmit Buffer Empty Interrupt Enable */
#define SSC_IER_RXRDY (0x1u << 4) /**< \brief (SSC_IER) Receive Ready Interrupt Enable */
#define SSC_IER_OVRUN (0x1u << 5) /**< \brief (SSC_IER) Receive Overrun Interrupt Enable */
#define SSC_IER_ENDRX (0x1u << 6) /**< \brief (SSC_IER) End of Reception Interrupt Enable */
#define SSC_IER_RXBUFF (0x1u << 7) /**< \brief (SSC_IER) Receive Buffer Full Interrupt Enable */
#define SSC_IER_CP0 (0x1u << 8) /**< \brief (SSC_IER) Compare 0 Interrupt Enable */
#define SSC_IER_CP1 (0x1u << 9) /**< \brief (SSC_IER) Compare 1 Interrupt Enable */
#define SSC_IER_TXSYN (0x1u << 10) /**< \brief (SSC_IER) Tx Sync Interrupt Enable */
#define SSC_IER_RXSYN (0x1u << 11) /**< \brief (SSC_IER) Rx Sync Interrupt Enable */
/* -------- SSC_IDR : (SSC Offset: 0x48) Interrupt Disable Register -------- */
#define SSC_IDR_TXRDY (0x1u << 0) /**< \brief (SSC_IDR) Transmit Ready Interrupt Disable */
#define SSC_IDR_TXEMPTY (0x1u << 1) /**< \brief (SSC_IDR) Transmit Empty Interrupt Disable */
#define SSC_IDR_ENDTX (0x1u << 2) /**< \brief (SSC_IDR) End of Transmission Interrupt Disable */
#define SSC_IDR_TXBUFE (0x1u << 3) /**< \brief (SSC_IDR) Transmit Buffer Empty Interrupt Disable */
#define SSC_IDR_RXRDY (0x1u << 4) /**< \brief (SSC_IDR) Receive Ready Interrupt Disable */
#define SSC_IDR_OVRUN (0x1u << 5) /**< \brief (SSC_IDR) Receive Overrun Interrupt Disable */
#define SSC_IDR_ENDRX (0x1u << 6) /**< \brief (SSC_IDR) End of Reception Interrupt Disable */
#define SSC_IDR_RXBUFF (0x1u << 7) /**< \brief (SSC_IDR) Receive Buffer Full Interrupt Disable */
#define SSC_IDR_CP0 (0x1u << 8) /**< \brief (SSC_IDR) Compare 0 Interrupt Disable */
#define SSC_IDR_CP1 (0x1u << 9) /**< \brief (SSC_IDR) Compare 1 Interrupt Disable */
#define SSC_IDR_TXSYN (0x1u << 10) /**< \brief (SSC_IDR) Tx Sync Interrupt Enable */
#define SSC_IDR_RXSYN (0x1u << 11) /**< \brief (SSC_IDR) Rx Sync Interrupt Enable */
/* -------- SSC_IMR : (SSC Offset: 0x4C) Interrupt Mask Register -------- */
#define SSC_IMR_TXRDY (0x1u << 0) /**< \brief (SSC_IMR) Transmit Ready Interrupt Mask */
#define SSC_IMR_TXEMPTY (0x1u << 1) /**< \brief (SSC_IMR) Transmit Empty Interrupt Mask */
#define SSC_IMR_ENDTX (0x1u << 2) /**< \brief (SSC_IMR) End of Transmission Interrupt Mask */
#define SSC_IMR_TXBUFE (0x1u << 3) /**< \brief (SSC_IMR) Transmit Buffer Empty Interrupt Mask */
#define SSC_IMR_RXRDY (0x1u << 4) /**< \brief (SSC_IMR) Receive Ready Interrupt Mask */
#define SSC_IMR_OVRUN (0x1u << 5) /**< \brief (SSC_IMR) Receive Overrun Interrupt Mask */
#define SSC_IMR_ENDRX (0x1u << 6) /**< \brief (SSC_IMR) End of Reception Interrupt Mask */
#define SSC_IMR_RXBUFF (0x1u << 7) /**< \brief (SSC_IMR) Receive Buffer Full Interrupt Mask */
#define SSC_IMR_CP0 (0x1u << 8) /**< \brief (SSC_IMR) Compare 0 Interrupt Mask */
#define SSC_IMR_CP1 (0x1u << 9) /**< \brief (SSC_IMR) Compare 1 Interrupt Mask */
#define SSC_IMR_TXSYN (0x1u << 10) /**< \brief (SSC_IMR) Tx Sync Interrupt Mask */
#define SSC_IMR_RXSYN (0x1u << 11) /**< \brief (SSC_IMR) Rx Sync Interrupt Mask */
/* -------- SSC_WPMR : (SSC Offset: 0xE4) Write Protection Mode Register -------- */
#define SSC_WPMR_WPEN (0x1u << 0) /**< \brief (SSC_WPMR) Write Protection Enable */
#define SSC_WPMR_WPKEY_Pos 8
#define SSC_WPMR_WPKEY_Msk (0xffffffu << SSC_WPMR_WPKEY_Pos) /**< \brief (SSC_WPMR) Write Protection Key */
#define SSC_WPMR_WPKEY(value) ((SSC_WPMR_WPKEY_Msk & ((value) << SSC_WPMR_WPKEY_Pos)))
#define   SSC_WPMR_WPKEY_PASSWD (0x535343u << 8) /**< \brief (SSC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. */
/* -------- SSC_WPSR : (SSC Offset: 0xE8) Write Protection Status Register -------- */
#define SSC_WPSR_WPVS (0x1u << 0) /**< \brief (SSC_WPSR) Write Protection Violation Status */
#define SSC_WPSR_WPVSRC_Pos 8
#define SSC_WPSR_WPVSRC_Msk (0xffffu << SSC_WPSR_WPVSRC_Pos) /**< \brief (SSC_WPSR) Write Protect Violation Source */
/* -------- SSC_RPR : (SSC Offset: 0x100) Receive Pointer Register -------- */
#define SSC_RPR_RXPTR_Pos 0
#define SSC_RPR_RXPTR_Msk (0xffffffffu << SSC_RPR_RXPTR_Pos) /**< \brief (SSC_RPR) Receive Pointer Register */
#define SSC_RPR_RXPTR(value) ((SSC_RPR_RXPTR_Msk & ((value) << SSC_RPR_RXPTR_Pos)))
/* -------- SSC_RCR : (SSC Offset: 0x104) Receive Counter Register -------- */
#define SSC_RCR_RXCTR_Pos 0
#define SSC_RCR_RXCTR_Msk (0xffffu << SSC_RCR_RXCTR_Pos) /**< \brief (SSC_RCR) Receive Counter Register */
#define SSC_RCR_RXCTR(value) ((SSC_RCR_RXCTR_Msk & ((value) << SSC_RCR_RXCTR_Pos)))
/* -------- SSC_TPR : (SSC Offset: 0x108) Transmit Pointer Register -------- */
#define SSC_TPR_TXPTR_Pos 0
#define SSC_TPR_TXPTR_Msk (0xffffffffu << SSC_TPR_TXPTR_Pos) /**< \brief (SSC_TPR) Transmit Counter Register */
#define SSC_TPR_TXPTR(value) ((SSC_TPR_TXPTR_Msk & ((value) << SSC_TPR_TXPTR_Pos)))
/* -------- SSC_TCR : (SSC Offset: 0x10C) Transmit Counter Register -------- */
#define SSC_TCR_TXCTR_Pos 0
#define SSC_TCR_TXCTR_Msk (0xffffu << SSC_TCR_TXCTR_Pos) /**< \brief (SSC_TCR) Transmit Counter Register */
#define SSC_TCR_TXCTR(value) ((SSC_TCR_TXCTR_Msk & ((value) << SSC_TCR_TXCTR_Pos)))
/* -------- SSC_RNPR : (SSC Offset: 0x110) Receive Next Pointer Register -------- */
#define SSC_RNPR_RXNPTR_Pos 0
#define SSC_RNPR_RXNPTR_Msk (0xffffffffu << SSC_RNPR_RXNPTR_Pos) /**< \brief (SSC_RNPR) Receive Next Pointer */
#define SSC_RNPR_RXNPTR(value) ((SSC_RNPR_RXNPTR_Msk & ((value) << SSC_RNPR_RXNPTR_Pos)))
/* -------- SSC_RNCR : (SSC Offset: 0x114) Receive Next Counter Register -------- */
#define SSC_RNCR_RXNCTR_Pos 0
#define SSC_RNCR_RXNCTR_Msk (0xffffu << SSC_RNCR_RXNCTR_Pos) /**< \brief (SSC_RNCR) Receive Next Counter */
#define SSC_RNCR_RXNCTR(value) ((SSC_RNCR_RXNCTR_Msk & ((value) << SSC_RNCR_RXNCTR_Pos)))
/* -------- SSC_TNPR : (SSC Offset: 0x118) Transmit Next Pointer Register -------- */
#define SSC_TNPR_TXNPTR_Pos 0
#define SSC_TNPR_TXNPTR_Msk (0xffffffffu << SSC_TNPR_TXNPTR_Pos) /**< \brief (SSC_TNPR) Transmit Next Pointer */
#define SSC_TNPR_TXNPTR(value) ((SSC_TNPR_TXNPTR_Msk & ((value) << SSC_TNPR_TXNPTR_Pos)))
/* -------- SSC_TNCR : (SSC Offset: 0x11C) Transmit Next Counter Register -------- */
#define SSC_TNCR_TXNCTR_Pos 0
#define SSC_TNCR_TXNCTR_Msk (0xffffu << SSC_TNCR_TXNCTR_Pos) /**< \brief (SSC_TNCR) Transmit Counter Next */
#define SSC_TNCR_TXNCTR(value) ((SSC_TNCR_TXNCTR_Msk & ((value) << SSC_TNCR_TXNCTR_Pos)))
/* -------- SSC_PTCR : (SSC Offset: 0x120) Transfer Control Register -------- */
#define SSC_PTCR_RXTEN (0x1u << 0) /**< \brief (SSC_PTCR) Receiver Transfer Enable */
#define SSC_PTCR_RXTDIS (0x1u << 1) /**< \brief (SSC_PTCR) Receiver Transfer Disable */
#define SSC_PTCR_TXTEN (0x1u << 8) /**< \brief (SSC_PTCR) Transmitter Transfer Enable */
#define SSC_PTCR_TXTDIS (0x1u << 9) /**< \brief (SSC_PTCR) Transmitter Transfer Disable */
/* -------- SSC_PTSR : (SSC Offset: 0x124) Transfer Status Register -------- */
#define SSC_PTSR_RXTEN (0x1u << 0) /**< \brief (SSC_PTSR) Receiver Transfer Enable */
#define SSC_PTSR_TXTEN (0x1u << 8) /**< \brief (SSC_PTSR) Transmitter Transfer Enable */

/*@}*/


#endif /* _SAM4S_SSC_COMPONENT_ */
