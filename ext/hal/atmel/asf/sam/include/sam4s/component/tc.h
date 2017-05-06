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

#ifndef _SAM4S_TC_COMPONENT_
#define _SAM4S_TC_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Timer Counter */
/* ============================================================================= */
/** \addtogroup SAM4S_TC Timer Counter */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief TcChannel hardware registers */
typedef struct {
  __O  uint32_t TC_CCR;       /**< \brief (TcChannel Offset: 0x0) Channel Control Register */
  __IO uint32_t TC_CMR;       /**< \brief (TcChannel Offset: 0x4) Channel Mode Register */
  __IO uint32_t TC_SMMR;      /**< \brief (TcChannel Offset: 0x8) Stepper Motor Mode Register */
  __I  uint32_t Reserved1[1];
  __I  uint32_t TC_CV;        /**< \brief (TcChannel Offset: 0x10) Counter Value */
  __IO uint32_t TC_RA;        /**< \brief (TcChannel Offset: 0x14) Register A */
  __IO uint32_t TC_RB;        /**< \brief (TcChannel Offset: 0x18) Register B */
  __IO uint32_t TC_RC;        /**< \brief (TcChannel Offset: 0x1C) Register C */
  __I  uint32_t TC_SR;        /**< \brief (TcChannel Offset: 0x20) Status Register */
  __O  uint32_t TC_IER;       /**< \brief (TcChannel Offset: 0x24) Interrupt Enable Register */
  __O  uint32_t TC_IDR;       /**< \brief (TcChannel Offset: 0x28) Interrupt Disable Register */
  __I  uint32_t TC_IMR;       /**< \brief (TcChannel Offset: 0x2C) Interrupt Mask Register */
  __I  uint32_t Reserved2[4];
} TcChannel;
/** \brief Tc hardware registers */
#define TCCHANNEL_NUMBER 3
typedef struct {
       TcChannel TC_CHANNEL[TCCHANNEL_NUMBER]; /**< \brief (Tc Offset: 0x0) channel = 0 .. 2 */
  __O  uint32_t  TC_BCR;                       /**< \brief (Tc Offset: 0xC0) Block Control Register */
  __IO uint32_t  TC_BMR;                       /**< \brief (Tc Offset: 0xC4) Block Mode Register */
  __O  uint32_t  TC_QIER;                      /**< \brief (Tc Offset: 0xC8) QDEC Interrupt Enable Register */
  __O  uint32_t  TC_QIDR;                      /**< \brief (Tc Offset: 0xCC) QDEC Interrupt Disable Register */
  __I  uint32_t  TC_QIMR;                      /**< \brief (Tc Offset: 0xD0) QDEC Interrupt Mask Register */
  __I  uint32_t  TC_QISR;                      /**< \brief (Tc Offset: 0xD4) QDEC Interrupt Status Register */
  __IO uint32_t  TC_FMR;                       /**< \brief (Tc Offset: 0xD8) Fault Mode Register */
  __I  uint32_t  Reserved1[2];
  __IO uint32_t  TC_WPMR;                      /**< \brief (Tc Offset: 0xE4) Write Protection Mode Register */
} Tc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- TC_CCR : (TC Offset: N/A) Channel Control Register -------- */
#define TC_CCR_CLKEN (0x1u << 0) /**< \brief (TC_CCR) Counter Clock Enable Command */
#define TC_CCR_CLKDIS (0x1u << 1) /**< \brief (TC_CCR) Counter Clock Disable Command */
#define TC_CCR_SWTRG (0x1u << 2) /**< \brief (TC_CCR) Software Trigger Command */
/* -------- TC_CMR : (TC Offset: N/A) Channel Mode Register -------- */
#define TC_CMR_TCCLKS_Pos 0
#define TC_CMR_TCCLKS_Msk (0x7u << TC_CMR_TCCLKS_Pos) /**< \brief (TC_CMR) Clock Selection */
#define TC_CMR_TCCLKS(value) ((TC_CMR_TCCLKS_Msk & ((value) << TC_CMR_TCCLKS_Pos)))
#define   TC_CMR_TCCLKS_TIMER_CLOCK1 (0x0u << 0) /**< \brief (TC_CMR) Clock selected: internal MCK/2 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_TIMER_CLOCK2 (0x1u << 0) /**< \brief (TC_CMR) Clock selected: internal MCK/8 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_TIMER_CLOCK3 (0x2u << 0) /**< \brief (TC_CMR) Clock selected: internal MCK/32 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_TIMER_CLOCK4 (0x3u << 0) /**< \brief (TC_CMR) Clock selected: internal MCK/128 clock signal (from PMC) */
#define   TC_CMR_TCCLKS_TIMER_CLOCK5 (0x4u << 0) /**< \brief (TC_CMR) Clock selected: internal SLCK clock signal (from PMC) */
#define   TC_CMR_TCCLKS_XC0 (0x5u << 0) /**< \brief (TC_CMR) Clock selected: XC0 */
#define   TC_CMR_TCCLKS_XC1 (0x6u << 0) /**< \brief (TC_CMR) Clock selected: XC1 */
#define   TC_CMR_TCCLKS_XC2 (0x7u << 0) /**< \brief (TC_CMR) Clock selected: XC2 */
#define TC_CMR_CLKI (0x1u << 3) /**< \brief (TC_CMR) Clock Invert */
#define TC_CMR_BURST_Pos 4
#define TC_CMR_BURST_Msk (0x3u << TC_CMR_BURST_Pos) /**< \brief (TC_CMR) Burst Signal Selection */
#define TC_CMR_BURST(value) ((TC_CMR_BURST_Msk & ((value) << TC_CMR_BURST_Pos)))
#define   TC_CMR_BURST_NONE (0x0u << 4) /**< \brief (TC_CMR) The clock is not gated by an external signal. */
#define   TC_CMR_BURST_XC0 (0x1u << 4) /**< \brief (TC_CMR) XC0 is ANDed with the selected clock. */
#define   TC_CMR_BURST_XC1 (0x2u << 4) /**< \brief (TC_CMR) XC1 is ANDed with the selected clock. */
#define   TC_CMR_BURST_XC2 (0x3u << 4) /**< \brief (TC_CMR) XC2 is ANDed with the selected clock. */
#define TC_CMR_LDBSTOP (0x1u << 6) /**< \brief (TC_CMR) Counter Clock Stopped with RB Loading */
#define TC_CMR_LDBDIS (0x1u << 7) /**< \brief (TC_CMR) Counter Clock Disable with RB Loading */
#define TC_CMR_ETRGEDG_Pos 8
#define TC_CMR_ETRGEDG_Msk (0x3u << TC_CMR_ETRGEDG_Pos) /**< \brief (TC_CMR) External Trigger Edge Selection */
#define TC_CMR_ETRGEDG(value) ((TC_CMR_ETRGEDG_Msk & ((value) << TC_CMR_ETRGEDG_Pos)))
#define   TC_CMR_ETRGEDG_NONE (0x0u << 8) /**< \brief (TC_CMR) The clock is not gated by an external signal. */
#define   TC_CMR_ETRGEDG_RISING (0x1u << 8) /**< \brief (TC_CMR) Rising edge */
#define   TC_CMR_ETRGEDG_FALLING (0x2u << 8) /**< \brief (TC_CMR) Falling edge */
#define   TC_CMR_ETRGEDG_EDGE (0x3u << 8) /**< \brief (TC_CMR) Each edge */
#define TC_CMR_ABETRG (0x1u << 10) /**< \brief (TC_CMR) TIOA or TIOB External Trigger Selection */
#define TC_CMR_CPCTRG (0x1u << 14) /**< \brief (TC_CMR) RC Compare Trigger Enable */
#define TC_CMR_WAVE (0x1u << 15) /**< \brief (TC_CMR) Waveform Mode */
#define TC_CMR_LDRA_Pos 16
#define TC_CMR_LDRA_Msk (0x3u << TC_CMR_LDRA_Pos) /**< \brief (TC_CMR) RA Loading Edge Selection */
#define TC_CMR_LDRA(value) ((TC_CMR_LDRA_Msk & ((value) << TC_CMR_LDRA_Pos)))
#define   TC_CMR_LDRA_NONE (0x0u << 16) /**< \brief (TC_CMR) None */
#define   TC_CMR_LDRA_RISING (0x1u << 16) /**< \brief (TC_CMR) Rising edge of TIOA */
#define   TC_CMR_LDRA_FALLING (0x2u << 16) /**< \brief (TC_CMR) Falling edge of TIOA */
#define   TC_CMR_LDRA_EDGE (0x3u << 16) /**< \brief (TC_CMR) Each edge of TIOA */
#define TC_CMR_LDRB_Pos 18
#define TC_CMR_LDRB_Msk (0x3u << TC_CMR_LDRB_Pos) /**< \brief (TC_CMR) RB Loading Edge Selection */
#define TC_CMR_LDRB(value) ((TC_CMR_LDRB_Msk & ((value) << TC_CMR_LDRB_Pos)))
#define   TC_CMR_LDRB_NONE (0x0u << 18) /**< \brief (TC_CMR) None */
#define   TC_CMR_LDRB_RISING (0x1u << 18) /**< \brief (TC_CMR) Rising edge of TIOA */
#define   TC_CMR_LDRB_FALLING (0x2u << 18) /**< \brief (TC_CMR) Falling edge of TIOA */
#define   TC_CMR_LDRB_EDGE (0x3u << 18) /**< \brief (TC_CMR) Each edge of TIOA */
#define TC_CMR_CPCSTOP (0x1u << 6) /**< \brief (TC_CMR) Counter Clock Stopped with RC Compare */
#define TC_CMR_CPCDIS (0x1u << 7) /**< \brief (TC_CMR) Counter Clock Disable with RC Compare */
#define TC_CMR_EEVTEDG_Pos 8
#define TC_CMR_EEVTEDG_Msk (0x3u << TC_CMR_EEVTEDG_Pos) /**< \brief (TC_CMR) External Event Edge Selection */
#define TC_CMR_EEVTEDG(value) ((TC_CMR_EEVTEDG_Msk & ((value) << TC_CMR_EEVTEDG_Pos)))
#define   TC_CMR_EEVTEDG_NONE (0x0u << 8) /**< \brief (TC_CMR) None */
#define   TC_CMR_EEVTEDG_RISING (0x1u << 8) /**< \brief (TC_CMR) Rising edge */
#define   TC_CMR_EEVTEDG_FALLING (0x2u << 8) /**< \brief (TC_CMR) Falling edge */
#define   TC_CMR_EEVTEDG_EDGE (0x3u << 8) /**< \brief (TC_CMR) Each edge */
#define TC_CMR_EEVT_Pos 10
#define TC_CMR_EEVT_Msk (0x3u << TC_CMR_EEVT_Pos) /**< \brief (TC_CMR) External Event Selection */
#define TC_CMR_EEVT(value) ((TC_CMR_EEVT_Msk & ((value) << TC_CMR_EEVT_Pos)))
#define   TC_CMR_EEVT_TIOB (0x0u << 10) /**< \brief (TC_CMR) TIOB */
#define   TC_CMR_EEVT_XC0 (0x1u << 10) /**< \brief (TC_CMR) XC0 */
#define   TC_CMR_EEVT_XC1 (0x2u << 10) /**< \brief (TC_CMR) XC1 */
#define   TC_CMR_EEVT_XC2 (0x3u << 10) /**< \brief (TC_CMR) XC2 */
#define TC_CMR_ENETRG (0x1u << 12) /**< \brief (TC_CMR) External Event Trigger Enable */
#define TC_CMR_WAVSEL_Pos 13
#define TC_CMR_WAVSEL_Msk (0x3u << TC_CMR_WAVSEL_Pos) /**< \brief (TC_CMR) Waveform Selection */
#define TC_CMR_WAVSEL(value) ((TC_CMR_WAVSEL_Msk & ((value) << TC_CMR_WAVSEL_Pos)))
#define   TC_CMR_WAVSEL_UP (0x0u << 13) /**< \brief (TC_CMR) UP mode without automatic trigger on RC Compare */
#define   TC_CMR_WAVSEL_UPDOWN (0x1u << 13) /**< \brief (TC_CMR) UPDOWN mode without automatic trigger on RC Compare */
#define   TC_CMR_WAVSEL_UP_RC (0x2u << 13) /**< \brief (TC_CMR) UP mode with automatic trigger on RC Compare */
#define   TC_CMR_WAVSEL_UPDOWN_RC (0x3u << 13) /**< \brief (TC_CMR) UPDOWN mode with automatic trigger on RC Compare */
#define TC_CMR_ACPA_Pos 16
#define TC_CMR_ACPA_Msk (0x3u << TC_CMR_ACPA_Pos) /**< \brief (TC_CMR) RA Compare Effect on TIOA */
#define TC_CMR_ACPA(value) ((TC_CMR_ACPA_Msk & ((value) << TC_CMR_ACPA_Pos)))
#define   TC_CMR_ACPA_NONE (0x0u << 16) /**< \brief (TC_CMR) None */
#define   TC_CMR_ACPA_SET (0x1u << 16) /**< \brief (TC_CMR) Set */
#define   TC_CMR_ACPA_CLEAR (0x2u << 16) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_ACPA_TOGGLE (0x3u << 16) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_ACPC_Pos 18
#define TC_CMR_ACPC_Msk (0x3u << TC_CMR_ACPC_Pos) /**< \brief (TC_CMR) RC Compare Effect on TIOA */
#define TC_CMR_ACPC(value) ((TC_CMR_ACPC_Msk & ((value) << TC_CMR_ACPC_Pos)))
#define   TC_CMR_ACPC_NONE (0x0u << 18) /**< \brief (TC_CMR) None */
#define   TC_CMR_ACPC_SET (0x1u << 18) /**< \brief (TC_CMR) Set */
#define   TC_CMR_ACPC_CLEAR (0x2u << 18) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_ACPC_TOGGLE (0x3u << 18) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_AEEVT_Pos 20
#define TC_CMR_AEEVT_Msk (0x3u << TC_CMR_AEEVT_Pos) /**< \brief (TC_CMR) External Event Effect on TIOA */
#define TC_CMR_AEEVT(value) ((TC_CMR_AEEVT_Msk & ((value) << TC_CMR_AEEVT_Pos)))
#define   TC_CMR_AEEVT_NONE (0x0u << 20) /**< \brief (TC_CMR) None */
#define   TC_CMR_AEEVT_SET (0x1u << 20) /**< \brief (TC_CMR) Set */
#define   TC_CMR_AEEVT_CLEAR (0x2u << 20) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_AEEVT_TOGGLE (0x3u << 20) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_ASWTRG_Pos 22
#define TC_CMR_ASWTRG_Msk (0x3u << TC_CMR_ASWTRG_Pos) /**< \brief (TC_CMR) Software Trigger Effect on TIOA */
#define TC_CMR_ASWTRG(value) ((TC_CMR_ASWTRG_Msk & ((value) << TC_CMR_ASWTRG_Pos)))
#define   TC_CMR_ASWTRG_NONE (0x0u << 22) /**< \brief (TC_CMR) None */
#define   TC_CMR_ASWTRG_SET (0x1u << 22) /**< \brief (TC_CMR) Set */
#define   TC_CMR_ASWTRG_CLEAR (0x2u << 22) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_ASWTRG_TOGGLE (0x3u << 22) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_BCPB_Pos 24
#define TC_CMR_BCPB_Msk (0x3u << TC_CMR_BCPB_Pos) /**< \brief (TC_CMR) RB Compare Effect on TIOB */
#define TC_CMR_BCPB(value) ((TC_CMR_BCPB_Msk & ((value) << TC_CMR_BCPB_Pos)))
#define   TC_CMR_BCPB_NONE (0x0u << 24) /**< \brief (TC_CMR) None */
#define   TC_CMR_BCPB_SET (0x1u << 24) /**< \brief (TC_CMR) Set */
#define   TC_CMR_BCPB_CLEAR (0x2u << 24) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_BCPB_TOGGLE (0x3u << 24) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_BCPC_Pos 26
#define TC_CMR_BCPC_Msk (0x3u << TC_CMR_BCPC_Pos) /**< \brief (TC_CMR) RC Compare Effect on TIOB */
#define TC_CMR_BCPC(value) ((TC_CMR_BCPC_Msk & ((value) << TC_CMR_BCPC_Pos)))
#define   TC_CMR_BCPC_NONE (0x0u << 26) /**< \brief (TC_CMR) None */
#define   TC_CMR_BCPC_SET (0x1u << 26) /**< \brief (TC_CMR) Set */
#define   TC_CMR_BCPC_CLEAR (0x2u << 26) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_BCPC_TOGGLE (0x3u << 26) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_BEEVT_Pos 28
#define TC_CMR_BEEVT_Msk (0x3u << TC_CMR_BEEVT_Pos) /**< \brief (TC_CMR) External Event Effect on TIOB */
#define TC_CMR_BEEVT(value) ((TC_CMR_BEEVT_Msk & ((value) << TC_CMR_BEEVT_Pos)))
#define   TC_CMR_BEEVT_NONE (0x0u << 28) /**< \brief (TC_CMR) None */
#define   TC_CMR_BEEVT_SET (0x1u << 28) /**< \brief (TC_CMR) Set */
#define   TC_CMR_BEEVT_CLEAR (0x2u << 28) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_BEEVT_TOGGLE (0x3u << 28) /**< \brief (TC_CMR) Toggle */
#define TC_CMR_BSWTRG_Pos 30
#define TC_CMR_BSWTRG_Msk (0x3u << TC_CMR_BSWTRG_Pos) /**< \brief (TC_CMR) Software Trigger Effect on TIOB */
#define TC_CMR_BSWTRG(value) ((TC_CMR_BSWTRG_Msk & ((value) << TC_CMR_BSWTRG_Pos)))
#define   TC_CMR_BSWTRG_NONE (0x0u << 30) /**< \brief (TC_CMR) None */
#define   TC_CMR_BSWTRG_SET (0x1u << 30) /**< \brief (TC_CMR) Set */
#define   TC_CMR_BSWTRG_CLEAR (0x2u << 30) /**< \brief (TC_CMR) Clear */
#define   TC_CMR_BSWTRG_TOGGLE (0x3u << 30) /**< \brief (TC_CMR) Toggle */
/* -------- TC_SMMR : (TC Offset: N/A) Stepper Motor Mode Register -------- */
#define TC_SMMR_GCEN (0x1u << 0) /**< \brief (TC_SMMR) Gray Count Enable */
#define TC_SMMR_DOWN (0x1u << 1) /**< \brief (TC_SMMR) Down Count */
/* -------- TC_CV : (TC Offset: N/A) Counter Value -------- */
#define TC_CV_CV_Pos 0
#define TC_CV_CV_Msk (0xffffffffu << TC_CV_CV_Pos) /**< \brief (TC_CV) Counter Value */
/* -------- TC_RA : (TC Offset: N/A) Register A -------- */
#define TC_RA_RA_Pos 0
#define TC_RA_RA_Msk (0xffffffffu << TC_RA_RA_Pos) /**< \brief (TC_RA) Register A */
#define TC_RA_RA(value) ((TC_RA_RA_Msk & ((value) << TC_RA_RA_Pos)))
/* -------- TC_RB : (TC Offset: N/A) Register B -------- */
#define TC_RB_RB_Pos 0
#define TC_RB_RB_Msk (0xffffffffu << TC_RB_RB_Pos) /**< \brief (TC_RB) Register B */
#define TC_RB_RB(value) ((TC_RB_RB_Msk & ((value) << TC_RB_RB_Pos)))
/* -------- TC_RC : (TC Offset: N/A) Register C -------- */
#define TC_RC_RC_Pos 0
#define TC_RC_RC_Msk (0xffffffffu << TC_RC_RC_Pos) /**< \brief (TC_RC) Register C */
#define TC_RC_RC(value) ((TC_RC_RC_Msk & ((value) << TC_RC_RC_Pos)))
/* -------- TC_SR : (TC Offset: N/A) Status Register -------- */
#define TC_SR_COVFS (0x1u << 0) /**< \brief (TC_SR) Counter Overflow Status (cleared on read) */
#define TC_SR_LOVRS (0x1u << 1) /**< \brief (TC_SR) Load Overrun Status (cleared on read) */
#define TC_SR_CPAS (0x1u << 2) /**< \brief (TC_SR) RA Compare Status (cleared on read) */
#define TC_SR_CPBS (0x1u << 3) /**< \brief (TC_SR) RB Compare Status (cleared on read) */
#define TC_SR_CPCS (0x1u << 4) /**< \brief (TC_SR) RC Compare Status (cleared on read) */
#define TC_SR_LDRAS (0x1u << 5) /**< \brief (TC_SR) RA Loading Status (cleared on read) */
#define TC_SR_LDRBS (0x1u << 6) /**< \brief (TC_SR) RB Loading Status (cleared on read) */
#define TC_SR_ETRGS (0x1u << 7) /**< \brief (TC_SR) External Trigger Status (cleared on read) */
#define TC_SR_CLKSTA (0x1u << 16) /**< \brief (TC_SR) Clock Enabling Status */
#define TC_SR_MTIOA (0x1u << 17) /**< \brief (TC_SR) TIOA Mirror */
#define TC_SR_MTIOB (0x1u << 18) /**< \brief (TC_SR) TIOB Mirror */
/* -------- TC_IER : (TC Offset: N/A) Interrupt Enable Register -------- */
#define TC_IER_COVFS (0x1u << 0) /**< \brief (TC_IER) Counter Overflow */
#define TC_IER_LOVRS (0x1u << 1) /**< \brief (TC_IER) Load Overrun */
#define TC_IER_CPAS (0x1u << 2) /**< \brief (TC_IER) RA Compare */
#define TC_IER_CPBS (0x1u << 3) /**< \brief (TC_IER) RB Compare */
#define TC_IER_CPCS (0x1u << 4) /**< \brief (TC_IER) RC Compare */
#define TC_IER_LDRAS (0x1u << 5) /**< \brief (TC_IER) RA Loading */
#define TC_IER_LDRBS (0x1u << 6) /**< \brief (TC_IER) RB Loading */
#define TC_IER_ETRGS (0x1u << 7) /**< \brief (TC_IER) External Trigger */
/* -------- TC_IDR : (TC Offset: N/A) Interrupt Disable Register -------- */
#define TC_IDR_COVFS (0x1u << 0) /**< \brief (TC_IDR) Counter Overflow */
#define TC_IDR_LOVRS (0x1u << 1) /**< \brief (TC_IDR) Load Overrun */
#define TC_IDR_CPAS (0x1u << 2) /**< \brief (TC_IDR) RA Compare */
#define TC_IDR_CPBS (0x1u << 3) /**< \brief (TC_IDR) RB Compare */
#define TC_IDR_CPCS (0x1u << 4) /**< \brief (TC_IDR) RC Compare */
#define TC_IDR_LDRAS (0x1u << 5) /**< \brief (TC_IDR) RA Loading */
#define TC_IDR_LDRBS (0x1u << 6) /**< \brief (TC_IDR) RB Loading */
#define TC_IDR_ETRGS (0x1u << 7) /**< \brief (TC_IDR) External Trigger */
/* -------- TC_IMR : (TC Offset: N/A) Interrupt Mask Register -------- */
#define TC_IMR_COVFS (0x1u << 0) /**< \brief (TC_IMR) Counter Overflow */
#define TC_IMR_LOVRS (0x1u << 1) /**< \brief (TC_IMR) Load Overrun */
#define TC_IMR_CPAS (0x1u << 2) /**< \brief (TC_IMR) RA Compare */
#define TC_IMR_CPBS (0x1u << 3) /**< \brief (TC_IMR) RB Compare */
#define TC_IMR_CPCS (0x1u << 4) /**< \brief (TC_IMR) RC Compare */
#define TC_IMR_LDRAS (0x1u << 5) /**< \brief (TC_IMR) RA Loading */
#define TC_IMR_LDRBS (0x1u << 6) /**< \brief (TC_IMR) RB Loading */
#define TC_IMR_ETRGS (0x1u << 7) /**< \brief (TC_IMR) External Trigger */
/* -------- TC_BCR : (TC Offset: 0xC0) Block Control Register -------- */
#define TC_BCR_SYNC (0x1u << 0) /**< \brief (TC_BCR) Synchro Command */
/* -------- TC_BMR : (TC Offset: 0xC4) Block Mode Register -------- */
#define TC_BMR_TC0XC0S_Pos 0
#define TC_BMR_TC0XC0S_Msk (0x3u << TC_BMR_TC0XC0S_Pos) /**< \brief (TC_BMR) External Clock Signal 0 Selection */
#define TC_BMR_TC0XC0S(value) ((TC_BMR_TC0XC0S_Msk & ((value) << TC_BMR_TC0XC0S_Pos)))
#define   TC_BMR_TC0XC0S_TCLK0 (0x0u << 0) /**< \brief (TC_BMR) Signal connected to XC0: TCLK0 */
#define   TC_BMR_TC0XC0S_TIOA1 (0x2u << 0) /**< \brief (TC_BMR) Signal connected to XC0: TIOA1 */
#define   TC_BMR_TC0XC0S_TIOA2 (0x3u << 0) /**< \brief (TC_BMR) Signal connected to XC0: TIOA2 */
#define TC_BMR_TC1XC1S_Pos 2
#define TC_BMR_TC1XC1S_Msk (0x3u << TC_BMR_TC1XC1S_Pos) /**< \brief (TC_BMR) External Clock Signal 1 Selection */
#define TC_BMR_TC1XC1S(value) ((TC_BMR_TC1XC1S_Msk & ((value) << TC_BMR_TC1XC1S_Pos)))
#define   TC_BMR_TC1XC1S_TCLK1 (0x0u << 2) /**< \brief (TC_BMR) Signal connected to XC1: TCLK1 */
#define   TC_BMR_TC1XC1S_TIOA0 (0x2u << 2) /**< \brief (TC_BMR) Signal connected to XC1: TIOA0 */
#define   TC_BMR_TC1XC1S_TIOA2 (0x3u << 2) /**< \brief (TC_BMR) Signal connected to XC1: TIOA2 */
#define TC_BMR_TC2XC2S_Pos 4
#define TC_BMR_TC2XC2S_Msk (0x3u << TC_BMR_TC2XC2S_Pos) /**< \brief (TC_BMR) External Clock Signal 2 Selection */
#define TC_BMR_TC2XC2S(value) ((TC_BMR_TC2XC2S_Msk & ((value) << TC_BMR_TC2XC2S_Pos)))
#define   TC_BMR_TC2XC2S_TCLK2 (0x0u << 4) /**< \brief (TC_BMR) Signal connected to XC2: TCLK2 */
#define   TC_BMR_TC2XC2S_TIOA0 (0x2u << 4) /**< \brief (TC_BMR) Signal connected to XC2: TIOA0 */
#define   TC_BMR_TC2XC2S_TIOA1 (0x3u << 4) /**< \brief (TC_BMR) Signal connected to XC2: TIOA1 */
#define TC_BMR_QDEN (0x1u << 8) /**< \brief (TC_BMR) Quadrature Decoder Enabled */
#define TC_BMR_POSEN (0x1u << 9) /**< \brief (TC_BMR) Position Enabled */
#define TC_BMR_SPEEDEN (0x1u << 10) /**< \brief (TC_BMR) Speed Enabled */
#define TC_BMR_QDTRANS (0x1u << 11) /**< \brief (TC_BMR) Quadrature Decoding Transparent */
#define TC_BMR_EDGPHA (0x1u << 12) /**< \brief (TC_BMR) Edge on PHA Count Mode */
#define TC_BMR_INVA (0x1u << 13) /**< \brief (TC_BMR) Inverted PHA */
#define TC_BMR_INVB (0x1u << 14) /**< \brief (TC_BMR) Inverted PHB */
#define TC_BMR_INVIDX (0x1u << 15) /**< \brief (TC_BMR) Inverted Index */
#define TC_BMR_SWAP (0x1u << 16) /**< \brief (TC_BMR) Swap PHA and PHB */
#define TC_BMR_IDXPHB (0x1u << 17) /**< \brief (TC_BMR) Index Pin is PHB Pin */
#define TC_BMR_MAXFILT_Pos 20
#define TC_BMR_MAXFILT_Msk (0x3fu << TC_BMR_MAXFILT_Pos) /**< \brief (TC_BMR) Maximum Filter */
#define TC_BMR_MAXFILT(value) ((TC_BMR_MAXFILT_Msk & ((value) << TC_BMR_MAXFILT_Pos)))
/* -------- TC_QIER : (TC Offset: 0xC8) QDEC Interrupt Enable Register -------- */
#define TC_QIER_IDX (0x1u << 0) /**< \brief (TC_QIER) Index */
#define TC_QIER_DIRCHG (0x1u << 1) /**< \brief (TC_QIER) Direction Change */
#define TC_QIER_QERR (0x1u << 2) /**< \brief (TC_QIER) Quadrature Error */
/* -------- TC_QIDR : (TC Offset: 0xCC) QDEC Interrupt Disable Register -------- */
#define TC_QIDR_IDX (0x1u << 0) /**< \brief (TC_QIDR) Index */
#define TC_QIDR_DIRCHG (0x1u << 1) /**< \brief (TC_QIDR) Direction Change */
#define TC_QIDR_QERR (0x1u << 2) /**< \brief (TC_QIDR) Quadrature Error */
/* -------- TC_QIMR : (TC Offset: 0xD0) QDEC Interrupt Mask Register -------- */
#define TC_QIMR_IDX (0x1u << 0) /**< \brief (TC_QIMR) Index */
#define TC_QIMR_DIRCHG (0x1u << 1) /**< \brief (TC_QIMR) Direction Change */
#define TC_QIMR_QERR (0x1u << 2) /**< \brief (TC_QIMR) Quadrature Error */
/* -------- TC_QISR : (TC Offset: 0xD4) QDEC Interrupt Status Register -------- */
#define TC_QISR_IDX (0x1u << 0) /**< \brief (TC_QISR) Index */
#define TC_QISR_DIRCHG (0x1u << 1) /**< \brief (TC_QISR) Direction Change */
#define TC_QISR_QERR (0x1u << 2) /**< \brief (TC_QISR) Quadrature Error */
#define TC_QISR_DIR (0x1u << 8) /**< \brief (TC_QISR) Direction */
/* -------- TC_FMR : (TC Offset: 0xD8) Fault Mode Register -------- */
#define TC_FMR_ENCF0 (0x1u << 0) /**< \brief (TC_FMR) Enable Compare Fault Channel 0 */
#define TC_FMR_ENCF1 (0x1u << 1) /**< \brief (TC_FMR) Enable Compare Fault Channel 1 */
/* -------- TC_WPMR : (TC Offset: 0xE4) Write Protection Mode Register -------- */
#define TC_WPMR_WPEN (0x1u << 0) /**< \brief (TC_WPMR) Write Protection Enable */
#define TC_WPMR_WPKEY_Pos 8
#define TC_WPMR_WPKEY_Msk (0xffffffu << TC_WPMR_WPKEY_Pos) /**< \brief (TC_WPMR) Write Protection Key */
#define TC_WPMR_WPKEY(value) ((TC_WPMR_WPKEY_Msk & ((value) << TC_WPMR_WPKEY_Pos)))
#define   TC_WPMR_WPKEY_PASSWD (0x54494Du << 8) /**< \brief (TC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. */

/*@}*/


#endif /* _SAM4S_TC_COMPONENT_ */
