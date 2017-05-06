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

#ifndef _SAM4S_PWM_COMPONENT_
#define _SAM4S_PWM_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Pulse Width Modulation Controller */
/* ============================================================================= */
/** \addtogroup SAM4S_PWM Pulse Width Modulation Controller */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief PwmCh_num hardware registers */
typedef struct {
  __IO uint32_t PWM_CMR;     /**< \brief (PwmCh_num Offset: 0x0) PWM Channel Mode Register */
  __IO uint32_t PWM_CDTY;    /**< \brief (PwmCh_num Offset: 0x4) PWM Channel Duty Cycle Register */
  __O  uint32_t PWM_CDTYUPD; /**< \brief (PwmCh_num Offset: 0x8) PWM Channel Duty Cycle Update Register */
  __IO uint32_t PWM_CPRD;    /**< \brief (PwmCh_num Offset: 0xC) PWM Channel Period Register */
  __O  uint32_t PWM_CPRDUPD; /**< \brief (PwmCh_num Offset: 0x10) PWM Channel Period Update Register */
  __I  uint32_t PWM_CCNT;    /**< \brief (PwmCh_num Offset: 0x14) PWM Channel Counter Register */
  __IO uint32_t PWM_DT;      /**< \brief (PwmCh_num Offset: 0x18) PWM Channel Dead Time Register */
  __O  uint32_t PWM_DTUPD;   /**< \brief (PwmCh_num Offset: 0x1C) PWM Channel Dead Time Update Register */
} PwmCh_num;
/** \brief PwmCmp hardware registers */
typedef struct {
  __IO uint32_t PWM_CMPV;    /**< \brief (PwmCmp Offset: 0x0) PWM Comparison 0 Value Register */
  __O  uint32_t PWM_CMPVUPD; /**< \brief (PwmCmp Offset: 0x4) PWM Comparison 0 Value Update Register */
  __IO uint32_t PWM_CMPM;    /**< \brief (PwmCmp Offset: 0x8) PWM Comparison 0 Mode Register */
  __O  uint32_t PWM_CMPMUPD; /**< \brief (PwmCmp Offset: 0xC) PWM Comparison 0 Mode Update Register */
} PwmCmp;
/** \brief Pwm hardware registers */
#define PWMCMP_NUMBER 8
#define PWMCH_NUM_NUMBER 4
typedef struct {
  __IO uint32_t  PWM_CLK;                      /**< \brief (Pwm Offset: 0x00) PWM Clock Register */
  __O  uint32_t  PWM_ENA;                      /**< \brief (Pwm Offset: 0x04) PWM Enable Register */
  __O  uint32_t  PWM_DIS;                      /**< \brief (Pwm Offset: 0x08) PWM Disable Register */
  __I  uint32_t  PWM_SR;                       /**< \brief (Pwm Offset: 0x0C) PWM Status Register */
  __O  uint32_t  PWM_IER1;                     /**< \brief (Pwm Offset: 0x10) PWM Interrupt Enable Register 1 */
  __O  uint32_t  PWM_IDR1;                     /**< \brief (Pwm Offset: 0x14) PWM Interrupt Disable Register 1 */
  __I  uint32_t  PWM_IMR1;                     /**< \brief (Pwm Offset: 0x18) PWM Interrupt Mask Register 1 */
  __I  uint32_t  PWM_ISR1;                     /**< \brief (Pwm Offset: 0x1C) PWM Interrupt Status Register 1 */
  __IO uint32_t  PWM_SCM;                      /**< \brief (Pwm Offset: 0x20) PWM Sync Channels Mode Register */
  __I  uint32_t  Reserved1[1];
  __IO uint32_t  PWM_SCUC;                     /**< \brief (Pwm Offset: 0x28) PWM Sync Channels Update Control Register */
  __IO uint32_t  PWM_SCUP;                     /**< \brief (Pwm Offset: 0x2C) PWM Sync Channels Update Period Register */
  __O  uint32_t  PWM_SCUPUPD;                  /**< \brief (Pwm Offset: 0x30) PWM Sync Channels Update Period Update Register */
  __O  uint32_t  PWM_IER2;                     /**< \brief (Pwm Offset: 0x34) PWM Interrupt Enable Register 2 */
  __O  uint32_t  PWM_IDR2;                     /**< \brief (Pwm Offset: 0x38) PWM Interrupt Disable Register 2 */
  __I  uint32_t  PWM_IMR2;                     /**< \brief (Pwm Offset: 0x3C) PWM Interrupt Mask Register 2 */
  __I  uint32_t  PWM_ISR2;                     /**< \brief (Pwm Offset: 0x40) PWM Interrupt Status Register 2 */
  __IO uint32_t  PWM_OOV;                      /**< \brief (Pwm Offset: 0x44) PWM Output Override Value Register */
  __IO uint32_t  PWM_OS;                       /**< \brief (Pwm Offset: 0x48) PWM Output Selection Register */
  __O  uint32_t  PWM_OSS;                      /**< \brief (Pwm Offset: 0x4C) PWM Output Selection Set Register */
  __O  uint32_t  PWM_OSC;                      /**< \brief (Pwm Offset: 0x50) PWM Output Selection Clear Register */
  __O  uint32_t  PWM_OSSUPD;                   /**< \brief (Pwm Offset: 0x54) PWM Output Selection Set Update Register */
  __O  uint32_t  PWM_OSCUPD;                   /**< \brief (Pwm Offset: 0x58) PWM Output Selection Clear Update Register */
  __IO uint32_t  PWM_FMR;                      /**< \brief (Pwm Offset: 0x5C) PWM Fault Mode Register */
  __I  uint32_t  PWM_FSR;                      /**< \brief (Pwm Offset: 0x60) PWM Fault Status Register */
  __O  uint32_t  PWM_FCR;                      /**< \brief (Pwm Offset: 0x64) PWM Fault Clear Register */
  __IO uint32_t  PWM_FPV;                      /**< \brief (Pwm Offset: 0x68) PWM Fault Protection Value Register */
  __IO uint32_t  PWM_FPE;                      /**< \brief (Pwm Offset: 0x6C) PWM Fault Protection Enable Register */
  __I  uint32_t  Reserved2[3];
  __IO uint32_t  PWM_ELMR[2];                  /**< \brief (Pwm Offset: 0x7C) PWM Event Line 0 Mode Register */
  __I  uint32_t  Reserved3[11];
  __IO uint32_t  PWM_SMMR;                     /**< \brief (Pwm Offset: 0xB0) PWM Stepper Motor Mode Register */
  __I  uint32_t  Reserved4[12];
  __O  uint32_t  PWM_WPCR;                     /**< \brief (Pwm Offset: 0xE4) PWM Write Protection Control Register */
  __I  uint32_t  PWM_WPSR;                     /**< \brief (Pwm Offset: 0xE8) PWM Write Protection Status Register */
  __I  uint32_t  Reserved5[7];
  __IO uint32_t  PWM_TPR;                      /**< \brief (Pwm Offset: 0x108) Transmit Pointer Register */
  __IO uint32_t  PWM_TCR;                      /**< \brief (Pwm Offset: 0x10C) Transmit Counter Register */
  __I  uint32_t  Reserved6[2];
  __IO uint32_t  PWM_TNPR;                     /**< \brief (Pwm Offset: 0x118) Transmit Next Pointer Register */
  __IO uint32_t  PWM_TNCR;                     /**< \brief (Pwm Offset: 0x11C) Transmit Next Counter Register */
  __O  uint32_t  PWM_PTCR;                     /**< \brief (Pwm Offset: 0x120) Transfer Control Register */
  __I  uint32_t  PWM_PTSR;                     /**< \brief (Pwm Offset: 0x124) Transfer Status Register */
  __I  uint32_t  Reserved7[2];
       PwmCmp    PWM_CMP[PWMCMP_NUMBER];       /**< \brief (Pwm Offset: 0x130) 0 .. 7 */
  __I  uint32_t  Reserved8[20];
       PwmCh_num PWM_CH_NUM[PWMCH_NUM_NUMBER]; /**< \brief (Pwm Offset: 0x200) ch_num = 0 .. 3 */
} Pwm;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- PWM_CLK : (PWM Offset: 0x00) PWM Clock Register -------- */
#define PWM_CLK_DIVA_Pos 0
#define PWM_CLK_DIVA_Msk (0xffu << PWM_CLK_DIVA_Pos) /**< \brief (PWM_CLK) CLKA Divide Factor */
#define PWM_CLK_DIVA(value) ((PWM_CLK_DIVA_Msk & ((value) << PWM_CLK_DIVA_Pos)))
#define   PWM_CLK_DIVA_CLKA_POFF (0x0u << 0) /**< \brief (PWM_CLK) CLKA clock is turned off */
#define   PWM_CLK_DIVA_PREA (0x1u << 0) /**< \brief (PWM_CLK) CLKA clock is clock selected by PREA */
#define PWM_CLK_PREA_Pos 8
#define PWM_CLK_PREA_Msk (0xfu << PWM_CLK_PREA_Pos) /**< \brief (PWM_CLK) CLKA Source Clock Selection */
#define PWM_CLK_PREA(value) ((PWM_CLK_PREA_Msk & ((value) << PWM_CLK_PREA_Pos)))
#define   PWM_CLK_PREA_CLK (0x0u << 8) /**< \brief (PWM_CLK) Peripheral clock */
#define   PWM_CLK_PREA_CLK_DIV2 (0x1u << 8) /**< \brief (PWM_CLK) Peripheral clock/2 */
#define   PWM_CLK_PREA_CLK_DIV4 (0x2u << 8) /**< \brief (PWM_CLK) Peripheral clock/4 */
#define   PWM_CLK_PREA_CLK_DIV8 (0x3u << 8) /**< \brief (PWM_CLK) Peripheral clock/8 */
#define   PWM_CLK_PREA_CLK_DIV16 (0x4u << 8) /**< \brief (PWM_CLK) Peripheral clock/16 */
#define   PWM_CLK_PREA_CLK_DIV32 (0x5u << 8) /**< \brief (PWM_CLK) Peripheral clock/32 */
#define   PWM_CLK_PREA_CLK_DIV64 (0x6u << 8) /**< \brief (PWM_CLK) Peripheral clock/64 */
#define   PWM_CLK_PREA_CLK_DIV128 (0x7u << 8) /**< \brief (PWM_CLK) Peripheral clock/128 */
#define   PWM_CLK_PREA_CLK_DIV256 (0x8u << 8) /**< \brief (PWM_CLK) Peripheral clock/256 */
#define   PWM_CLK_PREA_CLK_DIV512 (0x9u << 8) /**< \brief (PWM_CLK) Peripheral clock/512 */
#define   PWM_CLK_PREA_CLK_DIV1024 (0xAu << 8) /**< \brief (PWM_CLK) Peripheral clock/1024 */
#define PWM_CLK_DIVB_Pos 16
#define PWM_CLK_DIVB_Msk (0xffu << PWM_CLK_DIVB_Pos) /**< \brief (PWM_CLK) CLKB Divide Factor */
#define PWM_CLK_DIVB(value) ((PWM_CLK_DIVB_Msk & ((value) << PWM_CLK_DIVB_Pos)))
#define   PWM_CLK_DIVB_CLKB_POFF (0x0u << 16) /**< \brief (PWM_CLK) CLKB clock is turned off */
#define   PWM_CLK_DIVB_PREB (0x1u << 16) /**< \brief (PWM_CLK) CLKB clock is clock selected by PREB */
#define PWM_CLK_PREB_Pos 24
#define PWM_CLK_PREB_Msk (0xfu << PWM_CLK_PREB_Pos) /**< \brief (PWM_CLK) CLKB Source Clock Selection */
#define PWM_CLK_PREB(value) ((PWM_CLK_PREB_Msk & ((value) << PWM_CLK_PREB_Pos)))
#define   PWM_CLK_PREB_CLK (0x0u << 24) /**< \brief (PWM_CLK) Peripheral clock */
#define   PWM_CLK_PREB_CLK_DIV2 (0x1u << 24) /**< \brief (PWM_CLK) Peripheral clock/2 */
#define   PWM_CLK_PREB_CLK_DIV4 (0x2u << 24) /**< \brief (PWM_CLK) Peripheral clock/4 */
#define   PWM_CLK_PREB_CLK_DIV8 (0x3u << 24) /**< \brief (PWM_CLK) Peripheral clock/8 */
#define   PWM_CLK_PREB_CLK_DIV16 (0x4u << 24) /**< \brief (PWM_CLK) Peripheral clock/16 */
#define   PWM_CLK_PREB_CLK_DIV32 (0x5u << 24) /**< \brief (PWM_CLK) Peripheral clock/32 */
#define   PWM_CLK_PREB_CLK_DIV64 (0x6u << 24) /**< \brief (PWM_CLK) Peripheral clock/64 */
#define   PWM_CLK_PREB_CLK_DIV128 (0x7u << 24) /**< \brief (PWM_CLK) Peripheral clock/128 */
#define   PWM_CLK_PREB_CLK_DIV256 (0x8u << 24) /**< \brief (PWM_CLK) Peripheral clock/256 */
#define   PWM_CLK_PREB_CLK_DIV512 (0x9u << 24) /**< \brief (PWM_CLK) Peripheral clock/512 */
#define   PWM_CLK_PREB_CLK_DIV1024 (0xAu << 24) /**< \brief (PWM_CLK) Peripheral clock/1024 */
/* -------- PWM_ENA : (PWM Offset: 0x04) PWM Enable Register -------- */
#define PWM_ENA_CHID0 (0x1u << 0) /**< \brief (PWM_ENA) Channel ID */
#define PWM_ENA_CHID1 (0x1u << 1) /**< \brief (PWM_ENA) Channel ID */
#define PWM_ENA_CHID2 (0x1u << 2) /**< \brief (PWM_ENA) Channel ID */
#define PWM_ENA_CHID3 (0x1u << 3) /**< \brief (PWM_ENA) Channel ID */
/* -------- PWM_DIS : (PWM Offset: 0x08) PWM Disable Register -------- */
#define PWM_DIS_CHID0 (0x1u << 0) /**< \brief (PWM_DIS) Channel ID */
#define PWM_DIS_CHID1 (0x1u << 1) /**< \brief (PWM_DIS) Channel ID */
#define PWM_DIS_CHID2 (0x1u << 2) /**< \brief (PWM_DIS) Channel ID */
#define PWM_DIS_CHID3 (0x1u << 3) /**< \brief (PWM_DIS) Channel ID */
/* -------- PWM_SR : (PWM Offset: 0x0C) PWM Status Register -------- */
#define PWM_SR_CHID0 (0x1u << 0) /**< \brief (PWM_SR) Channel ID */
#define PWM_SR_CHID1 (0x1u << 1) /**< \brief (PWM_SR) Channel ID */
#define PWM_SR_CHID2 (0x1u << 2) /**< \brief (PWM_SR) Channel ID */
#define PWM_SR_CHID3 (0x1u << 3) /**< \brief (PWM_SR) Channel ID */
/* -------- PWM_IER1 : (PWM Offset: 0x10) PWM Interrupt Enable Register 1 -------- */
#define PWM_IER1_CHID0 (0x1u << 0) /**< \brief (PWM_IER1) Counter Event on Channel 0 Interrupt Enable */
#define PWM_IER1_CHID1 (0x1u << 1) /**< \brief (PWM_IER1) Counter Event on Channel 1 Interrupt Enable */
#define PWM_IER1_CHID2 (0x1u << 2) /**< \brief (PWM_IER1) Counter Event on Channel 2 Interrupt Enable */
#define PWM_IER1_CHID3 (0x1u << 3) /**< \brief (PWM_IER1) Counter Event on Channel 3 Interrupt Enable */
#define PWM_IER1_FCHID0 (0x1u << 16) /**< \brief (PWM_IER1) Fault Protection Trigger on Channel 0 Interrupt Enable */
#define PWM_IER1_FCHID1 (0x1u << 17) /**< \brief (PWM_IER1) Fault Protection Trigger on Channel 1 Interrupt Enable */
#define PWM_IER1_FCHID2 (0x1u << 18) /**< \brief (PWM_IER1) Fault Protection Trigger on Channel 2 Interrupt Enable */
#define PWM_IER1_FCHID3 (0x1u << 19) /**< \brief (PWM_IER1) Fault Protection Trigger on Channel 3 Interrupt Enable */
/* -------- PWM_IDR1 : (PWM Offset: 0x14) PWM Interrupt Disable Register 1 -------- */
#define PWM_IDR1_CHID0 (0x1u << 0) /**< \brief (PWM_IDR1) Counter Event on Channel 0 Interrupt Disable */
#define PWM_IDR1_CHID1 (0x1u << 1) /**< \brief (PWM_IDR1) Counter Event on Channel 1 Interrupt Disable */
#define PWM_IDR1_CHID2 (0x1u << 2) /**< \brief (PWM_IDR1) Counter Event on Channel 2 Interrupt Disable */
#define PWM_IDR1_CHID3 (0x1u << 3) /**< \brief (PWM_IDR1) Counter Event on Channel 3 Interrupt Disable */
#define PWM_IDR1_FCHID0 (0x1u << 16) /**< \brief (PWM_IDR1) Fault Protection Trigger on Channel 0 Interrupt Disable */
#define PWM_IDR1_FCHID1 (0x1u << 17) /**< \brief (PWM_IDR1) Fault Protection Trigger on Channel 1 Interrupt Disable */
#define PWM_IDR1_FCHID2 (0x1u << 18) /**< \brief (PWM_IDR1) Fault Protection Trigger on Channel 2 Interrupt Disable */
#define PWM_IDR1_FCHID3 (0x1u << 19) /**< \brief (PWM_IDR1) Fault Protection Trigger on Channel 3 Interrupt Disable */
/* -------- PWM_IMR1 : (PWM Offset: 0x18) PWM Interrupt Mask Register 1 -------- */
#define PWM_IMR1_CHID0 (0x1u << 0) /**< \brief (PWM_IMR1) Counter Event on Channel 0 Interrupt Mask */
#define PWM_IMR1_CHID1 (0x1u << 1) /**< \brief (PWM_IMR1) Counter Event on Channel 1 Interrupt Mask */
#define PWM_IMR1_CHID2 (0x1u << 2) /**< \brief (PWM_IMR1) Counter Event on Channel 2 Interrupt Mask */
#define PWM_IMR1_CHID3 (0x1u << 3) /**< \brief (PWM_IMR1) Counter Event on Channel 3 Interrupt Mask */
#define PWM_IMR1_FCHID0 (0x1u << 16) /**< \brief (PWM_IMR1) Fault Protection Trigger on Channel 0 Interrupt Mask */
#define PWM_IMR1_FCHID1 (0x1u << 17) /**< \brief (PWM_IMR1) Fault Protection Trigger on Channel 1 Interrupt Mask */
#define PWM_IMR1_FCHID2 (0x1u << 18) /**< \brief (PWM_IMR1) Fault Protection Trigger on Channel 2 Interrupt Mask */
#define PWM_IMR1_FCHID3 (0x1u << 19) /**< \brief (PWM_IMR1) Fault Protection Trigger on Channel 3 Interrupt Mask */
/* -------- PWM_ISR1 : (PWM Offset: 0x1C) PWM Interrupt Status Register 1 -------- */
#define PWM_ISR1_CHID0 (0x1u << 0) /**< \brief (PWM_ISR1) Counter Event on Channel 0 */
#define PWM_ISR1_CHID1 (0x1u << 1) /**< \brief (PWM_ISR1) Counter Event on Channel 1 */
#define PWM_ISR1_CHID2 (0x1u << 2) /**< \brief (PWM_ISR1) Counter Event on Channel 2 */
#define PWM_ISR1_CHID3 (0x1u << 3) /**< \brief (PWM_ISR1) Counter Event on Channel 3 */
#define PWM_ISR1_FCHID0 (0x1u << 16) /**< \brief (PWM_ISR1) Fault Protection Trigger on Channel 0 */
#define PWM_ISR1_FCHID1 (0x1u << 17) /**< \brief (PWM_ISR1) Fault Protection Trigger on Channel 1 */
#define PWM_ISR1_FCHID2 (0x1u << 18) /**< \brief (PWM_ISR1) Fault Protection Trigger on Channel 2 */
#define PWM_ISR1_FCHID3 (0x1u << 19) /**< \brief (PWM_ISR1) Fault Protection Trigger on Channel 3 */
/* -------- PWM_SCM : (PWM Offset: 0x20) PWM Sync Channels Mode Register -------- */
#define PWM_SCM_SYNC0 (0x1u << 0) /**< \brief (PWM_SCM) Synchronous Channel 0 */
#define PWM_SCM_SYNC1 (0x1u << 1) /**< \brief (PWM_SCM) Synchronous Channel 1 */
#define PWM_SCM_SYNC2 (0x1u << 2) /**< \brief (PWM_SCM) Synchronous Channel 2 */
#define PWM_SCM_SYNC3 (0x1u << 3) /**< \brief (PWM_SCM) Synchronous Channel 3 */
#define PWM_SCM_UPDM_Pos 16
#define PWM_SCM_UPDM_Msk (0x3u << PWM_SCM_UPDM_Pos) /**< \brief (PWM_SCM) Synchronous Channels Update Mode */
#define PWM_SCM_UPDM(value) ((PWM_SCM_UPDM_Msk & ((value) << PWM_SCM_UPDM_Pos)))
#define   PWM_SCM_UPDM_MODE0 (0x0u << 16) /**< \brief (PWM_SCM) Manual write of double buffer registers and manual update of synchronous channels */
#define   PWM_SCM_UPDM_MODE1 (0x1u << 16) /**< \brief (PWM_SCM) Manual write of double buffer registers and automatic update of synchronous channels */
#define   PWM_SCM_UPDM_MODE2 (0x2u << 16) /**< \brief (PWM_SCM) Automatic write of duty-cycle update registers by the Peripheral DMA Controller and automatic update of synchronous channels */
#define PWM_SCM_PTRM (0x1u << 20) /**< \brief (PWM_SCM) Peripheral DMA Controller Transfer Request Mode */
#define PWM_SCM_PTRCS_Pos 21
#define PWM_SCM_PTRCS_Msk (0x7u << PWM_SCM_PTRCS_Pos) /**< \brief (PWM_SCM) Peripheral DMA Controller Transfer Request Comparison Selection */
#define PWM_SCM_PTRCS(value) ((PWM_SCM_PTRCS_Msk & ((value) << PWM_SCM_PTRCS_Pos)))
/* -------- PWM_SCUC : (PWM Offset: 0x28) PWM Sync Channels Update Control Register -------- */
#define PWM_SCUC_UPDULOCK (0x1u << 0) /**< \brief (PWM_SCUC) Synchronous Channels Update Unlock */
/* -------- PWM_SCUP : (PWM Offset: 0x2C) PWM Sync Channels Update Period Register -------- */
#define PWM_SCUP_UPR_Pos 0
#define PWM_SCUP_UPR_Msk (0xfu << PWM_SCUP_UPR_Pos) /**< \brief (PWM_SCUP) Update Period */
#define PWM_SCUP_UPR(value) ((PWM_SCUP_UPR_Msk & ((value) << PWM_SCUP_UPR_Pos)))
#define PWM_SCUP_UPRCNT_Pos 4
#define PWM_SCUP_UPRCNT_Msk (0xfu << PWM_SCUP_UPRCNT_Pos) /**< \brief (PWM_SCUP) Update Period Counter */
#define PWM_SCUP_UPRCNT(value) ((PWM_SCUP_UPRCNT_Msk & ((value) << PWM_SCUP_UPRCNT_Pos)))
/* -------- PWM_SCUPUPD : (PWM Offset: 0x30) PWM Sync Channels Update Period Update Register -------- */
#define PWM_SCUPUPD_UPRUPD_Pos 0
#define PWM_SCUPUPD_UPRUPD_Msk (0xfu << PWM_SCUPUPD_UPRUPD_Pos) /**< \brief (PWM_SCUPUPD) Update Period Update */
#define PWM_SCUPUPD_UPRUPD(value) ((PWM_SCUPUPD_UPRUPD_Msk & ((value) << PWM_SCUPUPD_UPRUPD_Pos)))
/* -------- PWM_IER2 : (PWM Offset: 0x34) PWM Interrupt Enable Register 2 -------- */
#define PWM_IER2_WRDY (0x1u << 0) /**< \brief (PWM_IER2) Write Ready for Synchronous Channels Update Interrupt Enable */
#define PWM_IER2_ENDTX (0x1u << 1) /**< \brief (PWM_IER2) PDC End of TX Buffer Interrupt Enable */
#define PWM_IER2_TXBUFE (0x1u << 2) /**< \brief (PWM_IER2) PDC TX Buffer Empty Interrupt Enable */
#define PWM_IER2_UNRE (0x1u << 3) /**< \brief (PWM_IER2) Synchronous Channels Update Underrun Error Interrupt Enable */
#define PWM_IER2_CMPM0 (0x1u << 8) /**< \brief (PWM_IER2) Comparison 0 Match Interrupt Enable */
#define PWM_IER2_CMPM1 (0x1u << 9) /**< \brief (PWM_IER2) Comparison 1 Match Interrupt Enable */
#define PWM_IER2_CMPM2 (0x1u << 10) /**< \brief (PWM_IER2) Comparison 2 Match Interrupt Enable */
#define PWM_IER2_CMPM3 (0x1u << 11) /**< \brief (PWM_IER2) Comparison 3 Match Interrupt Enable */
#define PWM_IER2_CMPM4 (0x1u << 12) /**< \brief (PWM_IER2) Comparison 4 Match Interrupt Enable */
#define PWM_IER2_CMPM5 (0x1u << 13) /**< \brief (PWM_IER2) Comparison 5 Match Interrupt Enable */
#define PWM_IER2_CMPM6 (0x1u << 14) /**< \brief (PWM_IER2) Comparison 6 Match Interrupt Enable */
#define PWM_IER2_CMPM7 (0x1u << 15) /**< \brief (PWM_IER2) Comparison 7 Match Interrupt Enable */
#define PWM_IER2_CMPU0 (0x1u << 16) /**< \brief (PWM_IER2) Comparison 0 Update Interrupt Enable */
#define PWM_IER2_CMPU1 (0x1u << 17) /**< \brief (PWM_IER2) Comparison 1 Update Interrupt Enable */
#define PWM_IER2_CMPU2 (0x1u << 18) /**< \brief (PWM_IER2) Comparison 2 Update Interrupt Enable */
#define PWM_IER2_CMPU3 (0x1u << 19) /**< \brief (PWM_IER2) Comparison 3 Update Interrupt Enable */
#define PWM_IER2_CMPU4 (0x1u << 20) /**< \brief (PWM_IER2) Comparison 4 Update Interrupt Enable */
#define PWM_IER2_CMPU5 (0x1u << 21) /**< \brief (PWM_IER2) Comparison 5 Update Interrupt Enable */
#define PWM_IER2_CMPU6 (0x1u << 22) /**< \brief (PWM_IER2) Comparison 6 Update Interrupt Enable */
#define PWM_IER2_CMPU7 (0x1u << 23) /**< \brief (PWM_IER2) Comparison 7 Update Interrupt Enable */
/* -------- PWM_IDR2 : (PWM Offset: 0x38) PWM Interrupt Disable Register 2 -------- */
#define PWM_IDR2_WRDY (0x1u << 0) /**< \brief (PWM_IDR2) Write Ready for Synchronous Channels Update Interrupt Disable */
#define PWM_IDR2_ENDTX (0x1u << 1) /**< \brief (PWM_IDR2) PDC End of TX Buffer Interrupt Disable */
#define PWM_IDR2_TXBUFE (0x1u << 2) /**< \brief (PWM_IDR2) PDC TX Buffer Empty Interrupt Disable */
#define PWM_IDR2_UNRE (0x1u << 3) /**< \brief (PWM_IDR2) Synchronous Channels Update Underrun Error Interrupt Disable */
#define PWM_IDR2_CMPM0 (0x1u << 8) /**< \brief (PWM_IDR2) Comparison 0 Match Interrupt Disable */
#define PWM_IDR2_CMPM1 (0x1u << 9) /**< \brief (PWM_IDR2) Comparison 1 Match Interrupt Disable */
#define PWM_IDR2_CMPM2 (0x1u << 10) /**< \brief (PWM_IDR2) Comparison 2 Match Interrupt Disable */
#define PWM_IDR2_CMPM3 (0x1u << 11) /**< \brief (PWM_IDR2) Comparison 3 Match Interrupt Disable */
#define PWM_IDR2_CMPM4 (0x1u << 12) /**< \brief (PWM_IDR2) Comparison 4 Match Interrupt Disable */
#define PWM_IDR2_CMPM5 (0x1u << 13) /**< \brief (PWM_IDR2) Comparison 5 Match Interrupt Disable */
#define PWM_IDR2_CMPM6 (0x1u << 14) /**< \brief (PWM_IDR2) Comparison 6 Match Interrupt Disable */
#define PWM_IDR2_CMPM7 (0x1u << 15) /**< \brief (PWM_IDR2) Comparison 7 Match Interrupt Disable */
#define PWM_IDR2_CMPU0 (0x1u << 16) /**< \brief (PWM_IDR2) Comparison 0 Update Interrupt Disable */
#define PWM_IDR2_CMPU1 (0x1u << 17) /**< \brief (PWM_IDR2) Comparison 1 Update Interrupt Disable */
#define PWM_IDR2_CMPU2 (0x1u << 18) /**< \brief (PWM_IDR2) Comparison 2 Update Interrupt Disable */
#define PWM_IDR2_CMPU3 (0x1u << 19) /**< \brief (PWM_IDR2) Comparison 3 Update Interrupt Disable */
#define PWM_IDR2_CMPU4 (0x1u << 20) /**< \brief (PWM_IDR2) Comparison 4 Update Interrupt Disable */
#define PWM_IDR2_CMPU5 (0x1u << 21) /**< \brief (PWM_IDR2) Comparison 5 Update Interrupt Disable */
#define PWM_IDR2_CMPU6 (0x1u << 22) /**< \brief (PWM_IDR2) Comparison 6 Update Interrupt Disable */
#define PWM_IDR2_CMPU7 (0x1u << 23) /**< \brief (PWM_IDR2) Comparison 7 Update Interrupt Disable */
/* -------- PWM_IMR2 : (PWM Offset: 0x3C) PWM Interrupt Mask Register 2 -------- */
#define PWM_IMR2_WRDY (0x1u << 0) /**< \brief (PWM_IMR2) Write Ready for Synchronous Channels Update Interrupt Mask */
#define PWM_IMR2_ENDTX (0x1u << 1) /**< \brief (PWM_IMR2) PDC End of TX Buffer Interrupt Mask */
#define PWM_IMR2_TXBUFE (0x1u << 2) /**< \brief (PWM_IMR2) PDC TX Buffer Empty Interrupt Mask */
#define PWM_IMR2_UNRE (0x1u << 3) /**< \brief (PWM_IMR2) Synchronous Channels Update Underrun Error Interrupt Mask */
#define PWM_IMR2_CMPM0 (0x1u << 8) /**< \brief (PWM_IMR2) Comparison 0 Match Interrupt Mask */
#define PWM_IMR2_CMPM1 (0x1u << 9) /**< \brief (PWM_IMR2) Comparison 1 Match Interrupt Mask */
#define PWM_IMR2_CMPM2 (0x1u << 10) /**< \brief (PWM_IMR2) Comparison 2 Match Interrupt Mask */
#define PWM_IMR2_CMPM3 (0x1u << 11) /**< \brief (PWM_IMR2) Comparison 3 Match Interrupt Mask */
#define PWM_IMR2_CMPM4 (0x1u << 12) /**< \brief (PWM_IMR2) Comparison 4 Match Interrupt Mask */
#define PWM_IMR2_CMPM5 (0x1u << 13) /**< \brief (PWM_IMR2) Comparison 5 Match Interrupt Mask */
#define PWM_IMR2_CMPM6 (0x1u << 14) /**< \brief (PWM_IMR2) Comparison 6 Match Interrupt Mask */
#define PWM_IMR2_CMPM7 (0x1u << 15) /**< \brief (PWM_IMR2) Comparison 7 Match Interrupt Mask */
#define PWM_IMR2_CMPU0 (0x1u << 16) /**< \brief (PWM_IMR2) Comparison 0 Update Interrupt Mask */
#define PWM_IMR2_CMPU1 (0x1u << 17) /**< \brief (PWM_IMR2) Comparison 1 Update Interrupt Mask */
#define PWM_IMR2_CMPU2 (0x1u << 18) /**< \brief (PWM_IMR2) Comparison 2 Update Interrupt Mask */
#define PWM_IMR2_CMPU3 (0x1u << 19) /**< \brief (PWM_IMR2) Comparison 3 Update Interrupt Mask */
#define PWM_IMR2_CMPU4 (0x1u << 20) /**< \brief (PWM_IMR2) Comparison 4 Update Interrupt Mask */
#define PWM_IMR2_CMPU5 (0x1u << 21) /**< \brief (PWM_IMR2) Comparison 5 Update Interrupt Mask */
#define PWM_IMR2_CMPU6 (0x1u << 22) /**< \brief (PWM_IMR2) Comparison 6 Update Interrupt Mask */
#define PWM_IMR2_CMPU7 (0x1u << 23) /**< \brief (PWM_IMR2) Comparison 7 Update Interrupt Mask */
/* -------- PWM_ISR2 : (PWM Offset: 0x40) PWM Interrupt Status Register 2 -------- */
#define PWM_ISR2_WRDY (0x1u << 0) /**< \brief (PWM_ISR2) Write Ready for Synchronous Channels Update */
#define PWM_ISR2_ENDTX (0x1u << 1) /**< \brief (PWM_ISR2) PDC End of TX Buffer */
#define PWM_ISR2_TXBUFE (0x1u << 2) /**< \brief (PWM_ISR2) PDC TX Buffer Empty */
#define PWM_ISR2_UNRE (0x1u << 3) /**< \brief (PWM_ISR2) Synchronous Channels Update Underrun Error */
#define PWM_ISR2_CMPM0 (0x1u << 8) /**< \brief (PWM_ISR2) Comparison 0 Match */
#define PWM_ISR2_CMPM1 (0x1u << 9) /**< \brief (PWM_ISR2) Comparison 1 Match */
#define PWM_ISR2_CMPM2 (0x1u << 10) /**< \brief (PWM_ISR2) Comparison 2 Match */
#define PWM_ISR2_CMPM3 (0x1u << 11) /**< \brief (PWM_ISR2) Comparison 3 Match */
#define PWM_ISR2_CMPM4 (0x1u << 12) /**< \brief (PWM_ISR2) Comparison 4 Match */
#define PWM_ISR2_CMPM5 (0x1u << 13) /**< \brief (PWM_ISR2) Comparison 5 Match */
#define PWM_ISR2_CMPM6 (0x1u << 14) /**< \brief (PWM_ISR2) Comparison 6 Match */
#define PWM_ISR2_CMPM7 (0x1u << 15) /**< \brief (PWM_ISR2) Comparison 7 Match */
#define PWM_ISR2_CMPU0 (0x1u << 16) /**< \brief (PWM_ISR2) Comparison 0 Update */
#define PWM_ISR2_CMPU1 (0x1u << 17) /**< \brief (PWM_ISR2) Comparison 1 Update */
#define PWM_ISR2_CMPU2 (0x1u << 18) /**< \brief (PWM_ISR2) Comparison 2 Update */
#define PWM_ISR2_CMPU3 (0x1u << 19) /**< \brief (PWM_ISR2) Comparison 3 Update */
#define PWM_ISR2_CMPU4 (0x1u << 20) /**< \brief (PWM_ISR2) Comparison 4 Update */
#define PWM_ISR2_CMPU5 (0x1u << 21) /**< \brief (PWM_ISR2) Comparison 5 Update */
#define PWM_ISR2_CMPU6 (0x1u << 22) /**< \brief (PWM_ISR2) Comparison 6 Update */
#define PWM_ISR2_CMPU7 (0x1u << 23) /**< \brief (PWM_ISR2) Comparison 7 Update */
/* -------- PWM_OOV : (PWM Offset: 0x44) PWM Output Override Value Register -------- */
#define PWM_OOV_OOVH0 (0x1u << 0) /**< \brief (PWM_OOV) Output Override Value for PWMH output of the channel 0 */
#define PWM_OOV_OOVH1 (0x1u << 1) /**< \brief (PWM_OOV) Output Override Value for PWMH output of the channel 1 */
#define PWM_OOV_OOVH2 (0x1u << 2) /**< \brief (PWM_OOV) Output Override Value for PWMH output of the channel 2 */
#define PWM_OOV_OOVH3 (0x1u << 3) /**< \brief (PWM_OOV) Output Override Value for PWMH output of the channel 3 */
#define PWM_OOV_OOVL0 (0x1u << 16) /**< \brief (PWM_OOV) Output Override Value for PWML output of the channel 0 */
#define PWM_OOV_OOVL1 (0x1u << 17) /**< \brief (PWM_OOV) Output Override Value for PWML output of the channel 1 */
#define PWM_OOV_OOVL2 (0x1u << 18) /**< \brief (PWM_OOV) Output Override Value for PWML output of the channel 2 */
#define PWM_OOV_OOVL3 (0x1u << 19) /**< \brief (PWM_OOV) Output Override Value for PWML output of the channel 3 */
/* -------- PWM_OS : (PWM Offset: 0x48) PWM Output Selection Register -------- */
#define PWM_OS_OSH0 (0x1u << 0) /**< \brief (PWM_OS) Output Selection for PWMH output of the channel 0 */
#define PWM_OS_OSH1 (0x1u << 1) /**< \brief (PWM_OS) Output Selection for PWMH output of the channel 1 */
#define PWM_OS_OSH2 (0x1u << 2) /**< \brief (PWM_OS) Output Selection for PWMH output of the channel 2 */
#define PWM_OS_OSH3 (0x1u << 3) /**< \brief (PWM_OS) Output Selection for PWMH output of the channel 3 */
#define PWM_OS_OSL0 (0x1u << 16) /**< \brief (PWM_OS) Output Selection for PWML output of the channel 0 */
#define PWM_OS_OSL1 (0x1u << 17) /**< \brief (PWM_OS) Output Selection for PWML output of the channel 1 */
#define PWM_OS_OSL2 (0x1u << 18) /**< \brief (PWM_OS) Output Selection for PWML output of the channel 2 */
#define PWM_OS_OSL3 (0x1u << 19) /**< \brief (PWM_OS) Output Selection for PWML output of the channel 3 */
/* -------- PWM_OSS : (PWM Offset: 0x4C) PWM Output Selection Set Register -------- */
#define PWM_OSS_OSSH0 (0x1u << 0) /**< \brief (PWM_OSS) Output Selection Set for PWMH output of the channel 0 */
#define PWM_OSS_OSSH1 (0x1u << 1) /**< \brief (PWM_OSS) Output Selection Set for PWMH output of the channel 1 */
#define PWM_OSS_OSSH2 (0x1u << 2) /**< \brief (PWM_OSS) Output Selection Set for PWMH output of the channel 2 */
#define PWM_OSS_OSSH3 (0x1u << 3) /**< \brief (PWM_OSS) Output Selection Set for PWMH output of the channel 3 */
#define PWM_OSS_OSSL0 (0x1u << 16) /**< \brief (PWM_OSS) Output Selection Set for PWML output of the channel 0 */
#define PWM_OSS_OSSL1 (0x1u << 17) /**< \brief (PWM_OSS) Output Selection Set for PWML output of the channel 1 */
#define PWM_OSS_OSSL2 (0x1u << 18) /**< \brief (PWM_OSS) Output Selection Set for PWML output of the channel 2 */
#define PWM_OSS_OSSL3 (0x1u << 19) /**< \brief (PWM_OSS) Output Selection Set for PWML output of the channel 3 */
/* -------- PWM_OSC : (PWM Offset: 0x50) PWM Output Selection Clear Register -------- */
#define PWM_OSC_OSCH0 (0x1u << 0) /**< \brief (PWM_OSC) Output Selection Clear for PWMH output of the channel 0 */
#define PWM_OSC_OSCH1 (0x1u << 1) /**< \brief (PWM_OSC) Output Selection Clear for PWMH output of the channel 1 */
#define PWM_OSC_OSCH2 (0x1u << 2) /**< \brief (PWM_OSC) Output Selection Clear for PWMH output of the channel 2 */
#define PWM_OSC_OSCH3 (0x1u << 3) /**< \brief (PWM_OSC) Output Selection Clear for PWMH output of the channel 3 */
#define PWM_OSC_OSCL0 (0x1u << 16) /**< \brief (PWM_OSC) Output Selection Clear for PWML output of the channel 0 */
#define PWM_OSC_OSCL1 (0x1u << 17) /**< \brief (PWM_OSC) Output Selection Clear for PWML output of the channel 1 */
#define PWM_OSC_OSCL2 (0x1u << 18) /**< \brief (PWM_OSC) Output Selection Clear for PWML output of the channel 2 */
#define PWM_OSC_OSCL3 (0x1u << 19) /**< \brief (PWM_OSC) Output Selection Clear for PWML output of the channel 3 */
/* -------- PWM_OSSUPD : (PWM Offset: 0x54) PWM Output Selection Set Update Register -------- */
#define PWM_OSSUPD_OSSUPH0 (0x1u << 0) /**< \brief (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 0 */
#define PWM_OSSUPD_OSSUPH1 (0x1u << 1) /**< \brief (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 1 */
#define PWM_OSSUPD_OSSUPH2 (0x1u << 2) /**< \brief (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 2 */
#define PWM_OSSUPD_OSSUPH3 (0x1u << 3) /**< \brief (PWM_OSSUPD) Output Selection Set for PWMH output of the channel 3 */
#define PWM_OSSUPD_OSSUPL0 (0x1u << 16) /**< \brief (PWM_OSSUPD) Output Selection Set for PWML output of the channel 0 */
#define PWM_OSSUPD_OSSUPL1 (0x1u << 17) /**< \brief (PWM_OSSUPD) Output Selection Set for PWML output of the channel 1 */
#define PWM_OSSUPD_OSSUPL2 (0x1u << 18) /**< \brief (PWM_OSSUPD) Output Selection Set for PWML output of the channel 2 */
#define PWM_OSSUPD_OSSUPL3 (0x1u << 19) /**< \brief (PWM_OSSUPD) Output Selection Set for PWML output of the channel 3 */
/* -------- PWM_OSCUPD : (PWM Offset: 0x58) PWM Output Selection Clear Update Register -------- */
#define PWM_OSCUPD_OSCUPH0 (0x1u << 0) /**< \brief (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 0 */
#define PWM_OSCUPD_OSCUPH1 (0x1u << 1) /**< \brief (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 1 */
#define PWM_OSCUPD_OSCUPH2 (0x1u << 2) /**< \brief (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 2 */
#define PWM_OSCUPD_OSCUPH3 (0x1u << 3) /**< \brief (PWM_OSCUPD) Output Selection Clear for PWMH output of the channel 3 */
#define PWM_OSCUPD_OSCUPL0 (0x1u << 16) /**< \brief (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 0 */
#define PWM_OSCUPD_OSCUPL1 (0x1u << 17) /**< \brief (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 1 */
#define PWM_OSCUPD_OSCUPL2 (0x1u << 18) /**< \brief (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 2 */
#define PWM_OSCUPD_OSCUPL3 (0x1u << 19) /**< \brief (PWM_OSCUPD) Output Selection Clear for PWML output of the channel 3 */
/* -------- PWM_FMR : (PWM Offset: 0x5C) PWM Fault Mode Register -------- */
#define PWM_FMR_FPOL_Pos 0
#define PWM_FMR_FPOL_Msk (0xffu << PWM_FMR_FPOL_Pos) /**< \brief (PWM_FMR) Fault Polarity */
#define PWM_FMR_FPOL(value) ((PWM_FMR_FPOL_Msk & ((value) << PWM_FMR_FPOL_Pos)))
#define PWM_FMR_FMOD_Pos 8
#define PWM_FMR_FMOD_Msk (0xffu << PWM_FMR_FMOD_Pos) /**< \brief (PWM_FMR) Fault Activation Mode */
#define PWM_FMR_FMOD(value) ((PWM_FMR_FMOD_Msk & ((value) << PWM_FMR_FMOD_Pos)))
#define PWM_FMR_FFIL_Pos 16
#define PWM_FMR_FFIL_Msk (0xffu << PWM_FMR_FFIL_Pos) /**< \brief (PWM_FMR) Fault Filtering */
#define PWM_FMR_FFIL(value) ((PWM_FMR_FFIL_Msk & ((value) << PWM_FMR_FFIL_Pos)))
/* -------- PWM_FSR : (PWM Offset: 0x60) PWM Fault Status Register -------- */
#define PWM_FSR_FIV_Pos 0
#define PWM_FSR_FIV_Msk (0xffu << PWM_FSR_FIV_Pos) /**< \brief (PWM_FSR) Fault Input Value */
#define PWM_FSR_FS_Pos 8
#define PWM_FSR_FS_Msk (0xffu << PWM_FSR_FS_Pos) /**< \brief (PWM_FSR) Fault Status */
/* -------- PWM_FCR : (PWM Offset: 0x64) PWM Fault Clear Register -------- */
#define PWM_FCR_FCLR_Pos 0
#define PWM_FCR_FCLR_Msk (0xffu << PWM_FCR_FCLR_Pos) /**< \brief (PWM_FCR) Fault Clear */
#define PWM_FCR_FCLR(value) ((PWM_FCR_FCLR_Msk & ((value) << PWM_FCR_FCLR_Pos)))
/* -------- PWM_FPV : (PWM Offset: 0x68) PWM Fault Protection Value Register -------- */
#define PWM_FPV_FPVH0 (0x1u << 0) /**< \brief (PWM_FPV) Fault Protection Value for PWMH output on channel 0 */
#define PWM_FPV_FPVH1 (0x1u << 1) /**< \brief (PWM_FPV) Fault Protection Value for PWMH output on channel 1 */
#define PWM_FPV_FPVH2 (0x1u << 2) /**< \brief (PWM_FPV) Fault Protection Value for PWMH output on channel 2 */
#define PWM_FPV_FPVH3 (0x1u << 3) /**< \brief (PWM_FPV) Fault Protection Value for PWMH output on channel 3 */
#define PWM_FPV_FPVL0 (0x1u << 16) /**< \brief (PWM_FPV) Fault Protection Value for PWML output on channel 0 */
#define PWM_FPV_FPVL1 (0x1u << 17) /**< \brief (PWM_FPV) Fault Protection Value for PWML output on channel 1 */
#define PWM_FPV_FPVL2 (0x1u << 18) /**< \brief (PWM_FPV) Fault Protection Value for PWML output on channel 2 */
#define PWM_FPV_FPVL3 (0x1u << 19) /**< \brief (PWM_FPV) Fault Protection Value for PWML output on channel 3 */
/* -------- PWM_FPE : (PWM Offset: 0x6C) PWM Fault Protection Enable Register -------- */
#define PWM_FPE_FPE0_Pos 0
#define PWM_FPE_FPE0_Msk (0xffu << PWM_FPE_FPE0_Pos) /**< \brief (PWM_FPE) Fault Protection Enable for channel 0 */
#define PWM_FPE_FPE0(value) ((PWM_FPE_FPE0_Msk & ((value) << PWM_FPE_FPE0_Pos)))
#define PWM_FPE_FPE1_Pos 8
#define PWM_FPE_FPE1_Msk (0xffu << PWM_FPE_FPE1_Pos) /**< \brief (PWM_FPE) Fault Protection Enable for channel 1 */
#define PWM_FPE_FPE1(value) ((PWM_FPE_FPE1_Msk & ((value) << PWM_FPE_FPE1_Pos)))
#define PWM_FPE_FPE2_Pos 16
#define PWM_FPE_FPE2_Msk (0xffu << PWM_FPE_FPE2_Pos) /**< \brief (PWM_FPE) Fault Protection Enable for channel 2 */
#define PWM_FPE_FPE2(value) ((PWM_FPE_FPE2_Msk & ((value) << PWM_FPE_FPE2_Pos)))
#define PWM_FPE_FPE3_Pos 24
#define PWM_FPE_FPE3_Msk (0xffu << PWM_FPE_FPE3_Pos) /**< \brief (PWM_FPE) Fault Protection Enable for channel 3 */
#define PWM_FPE_FPE3(value) ((PWM_FPE_FPE3_Msk & ((value) << PWM_FPE_FPE3_Pos)))
/* -------- PWM_ELMR[2] : (PWM Offset: 0x7C) PWM Event Line 0 Mode Register -------- */
#define PWM_ELMR_CSEL0 (0x1u << 0) /**< \brief (PWM_ELMR[2]) Comparison 0 Selection */
#define PWM_ELMR_CSEL1 (0x1u << 1) /**< \brief (PWM_ELMR[2]) Comparison 1 Selection */
#define PWM_ELMR_CSEL2 (0x1u << 2) /**< \brief (PWM_ELMR[2]) Comparison 2 Selection */
#define PWM_ELMR_CSEL3 (0x1u << 3) /**< \brief (PWM_ELMR[2]) Comparison 3 Selection */
#define PWM_ELMR_CSEL4 (0x1u << 4) /**< \brief (PWM_ELMR[2]) Comparison 4 Selection */
#define PWM_ELMR_CSEL5 (0x1u << 5) /**< \brief (PWM_ELMR[2]) Comparison 5 Selection */
#define PWM_ELMR_CSEL6 (0x1u << 6) /**< \brief (PWM_ELMR[2]) Comparison 6 Selection */
#define PWM_ELMR_CSEL7 (0x1u << 7) /**< \brief (PWM_ELMR[2]) Comparison 7 Selection */
/* -------- PWM_SMMR : (PWM Offset: 0xB0) PWM Stepper Motor Mode Register -------- */
#define PWM_SMMR_GCEN0 (0x1u << 0) /**< \brief (PWM_SMMR) Gray Count ENable */
#define PWM_SMMR_GCEN1 (0x1u << 1) /**< \brief (PWM_SMMR) Gray Count ENable */
#define PWM_SMMR_DOWN0 (0x1u << 16) /**< \brief (PWM_SMMR) DOWN Count */
#define PWM_SMMR_DOWN1 (0x1u << 17) /**< \brief (PWM_SMMR) DOWN Count */
/* -------- PWM_WPCR : (PWM Offset: 0xE4) PWM Write Protection Control Register -------- */
#define PWM_WPCR_WPCMD_Pos 0
#define PWM_WPCR_WPCMD_Msk (0x3u << PWM_WPCR_WPCMD_Pos) /**< \brief (PWM_WPCR) Write Protection Command */
#define PWM_WPCR_WPCMD(value) ((PWM_WPCR_WPCMD_Msk & ((value) << PWM_WPCR_WPCMD_Pos)))
#define   PWM_WPCR_WPCMD_DISABLE_SW_PROT (0x0u << 0) /**< \brief (PWM_WPCR) Disables the software write protection of the register groups of which the bit WPRGx is at '1'. */
#define   PWM_WPCR_WPCMD_ENABLE_SW_PROT (0x1u << 0) /**< \brief (PWM_WPCR) Enables the software write protection of the register groups of which the bit WPRGx is at '1'. */
#define   PWM_WPCR_WPCMD_ENABLE_HW_PROT (0x2u << 0) /**< \brief (PWM_WPCR) Enables the hardware write protection of the register groups of which the bit WPRGx is at '1'. Only a hardware reset of the PWM controller can disable the hardware write protection. Moreover, to meet security requirements, the PIO lines associated with the PWM can not be configured through the PIO interface. */
#define PWM_WPCR_WPRG0 (0x1u << 2) /**< \brief (PWM_WPCR) Write Protection Register Group 0 */
#define PWM_WPCR_WPRG1 (0x1u << 3) /**< \brief (PWM_WPCR) Write Protection Register Group 1 */
#define PWM_WPCR_WPRG2 (0x1u << 4) /**< \brief (PWM_WPCR) Write Protection Register Group 2 */
#define PWM_WPCR_WPRG3 (0x1u << 5) /**< \brief (PWM_WPCR) Write Protection Register Group 3 */
#define PWM_WPCR_WPRG4 (0x1u << 6) /**< \brief (PWM_WPCR) Write Protection Register Group 4 */
#define PWM_WPCR_WPRG5 (0x1u << 7) /**< \brief (PWM_WPCR) Write Protection Register Group 5 */
#define PWM_WPCR_WPKEY_Pos 8
#define PWM_WPCR_WPKEY_Msk (0xffffffu << PWM_WPCR_WPKEY_Pos) /**< \brief (PWM_WPCR) Write Protection Key */
#define PWM_WPCR_WPKEY(value) ((PWM_WPCR_WPKEY_Msk & ((value) << PWM_WPCR_WPKEY_Pos)))
#define   PWM_WPCR_WPKEY_PASSWD (0x50574Du << 8) /**< \brief (PWM_WPCR) Writing any other value in this field aborts the write operation of the WPCMD field.Always reads as 0 */
/* -------- PWM_WPSR : (PWM Offset: 0xE8) PWM Write Protection Status Register -------- */
#define PWM_WPSR_WPSWS0 (0x1u << 0) /**< \brief (PWM_WPSR) Write Protect SW Status */
#define PWM_WPSR_WPSWS1 (0x1u << 1) /**< \brief (PWM_WPSR) Write Protect SW Status */
#define PWM_WPSR_WPSWS2 (0x1u << 2) /**< \brief (PWM_WPSR) Write Protect SW Status */
#define PWM_WPSR_WPSWS3 (0x1u << 3) /**< \brief (PWM_WPSR) Write Protect SW Status */
#define PWM_WPSR_WPSWS4 (0x1u << 4) /**< \brief (PWM_WPSR) Write Protect SW Status */
#define PWM_WPSR_WPSWS5 (0x1u << 5) /**< \brief (PWM_WPSR) Write Protect SW Status */
#define PWM_WPSR_WPVS (0x1u << 7) /**< \brief (PWM_WPSR) Write Protect Violation Status */
#define PWM_WPSR_WPHWS0 (0x1u << 8) /**< \brief (PWM_WPSR) Write Protect HW Status */
#define PWM_WPSR_WPHWS1 (0x1u << 9) /**< \brief (PWM_WPSR) Write Protect HW Status */
#define PWM_WPSR_WPHWS2 (0x1u << 10) /**< \brief (PWM_WPSR) Write Protect HW Status */
#define PWM_WPSR_WPHWS3 (0x1u << 11) /**< \brief (PWM_WPSR) Write Protect HW Status */
#define PWM_WPSR_WPHWS4 (0x1u << 12) /**< \brief (PWM_WPSR) Write Protect HW Status */
#define PWM_WPSR_WPHWS5 (0x1u << 13) /**< \brief (PWM_WPSR) Write Protect HW Status */
#define PWM_WPSR_WPVSRC_Pos 16
#define PWM_WPSR_WPVSRC_Msk (0xffffu << PWM_WPSR_WPVSRC_Pos) /**< \brief (PWM_WPSR) Write Protect Violation Source */
/* -------- PWM_TPR : (PWM Offset: 0x108) Transmit Pointer Register -------- */
#define PWM_TPR_TXPTR_Pos 0
#define PWM_TPR_TXPTR_Msk (0xffffffffu << PWM_TPR_TXPTR_Pos) /**< \brief (PWM_TPR) Transmit Counter Register */
#define PWM_TPR_TXPTR(value) ((PWM_TPR_TXPTR_Msk & ((value) << PWM_TPR_TXPTR_Pos)))
/* -------- PWM_TCR : (PWM Offset: 0x10C) Transmit Counter Register -------- */
#define PWM_TCR_TXCTR_Pos 0
#define PWM_TCR_TXCTR_Msk (0xffffu << PWM_TCR_TXCTR_Pos) /**< \brief (PWM_TCR) Transmit Counter Register */
#define PWM_TCR_TXCTR(value) ((PWM_TCR_TXCTR_Msk & ((value) << PWM_TCR_TXCTR_Pos)))
/* -------- PWM_TNPR : (PWM Offset: 0x118) Transmit Next Pointer Register -------- */
#define PWM_TNPR_TXNPTR_Pos 0
#define PWM_TNPR_TXNPTR_Msk (0xffffffffu << PWM_TNPR_TXNPTR_Pos) /**< \brief (PWM_TNPR) Transmit Next Pointer */
#define PWM_TNPR_TXNPTR(value) ((PWM_TNPR_TXNPTR_Msk & ((value) << PWM_TNPR_TXNPTR_Pos)))
/* -------- PWM_TNCR : (PWM Offset: 0x11C) Transmit Next Counter Register -------- */
#define PWM_TNCR_TXNCTR_Pos 0
#define PWM_TNCR_TXNCTR_Msk (0xffffu << PWM_TNCR_TXNCTR_Pos) /**< \brief (PWM_TNCR) Transmit Counter Next */
#define PWM_TNCR_TXNCTR(value) ((PWM_TNCR_TXNCTR_Msk & ((value) << PWM_TNCR_TXNCTR_Pos)))
/* -------- PWM_PTCR : (PWM Offset: 0x120) Transfer Control Register -------- */
#define PWM_PTCR_RXTEN (0x1u << 0) /**< \brief (PWM_PTCR) Receiver Transfer Enable */
#define PWM_PTCR_RXTDIS (0x1u << 1) /**< \brief (PWM_PTCR) Receiver Transfer Disable */
#define PWM_PTCR_TXTEN (0x1u << 8) /**< \brief (PWM_PTCR) Transmitter Transfer Enable */
#define PWM_PTCR_TXTDIS (0x1u << 9) /**< \brief (PWM_PTCR) Transmitter Transfer Disable */
/* -------- PWM_PTSR : (PWM Offset: 0x124) Transfer Status Register -------- */
#define PWM_PTSR_RXTEN (0x1u << 0) /**< \brief (PWM_PTSR) Receiver Transfer Enable */
#define PWM_PTSR_TXTEN (0x1u << 8) /**< \brief (PWM_PTSR) Transmitter Transfer Enable */
/* -------- PWM_CMPV : (PWM Offset: N/A) PWM Comparison 0 Value Register -------- */
#define PWM_CMPV_CV_Pos 0
#define PWM_CMPV_CV_Msk (0xffffffu << PWM_CMPV_CV_Pos) /**< \brief (PWM_CMPV) Comparison x Value */
#define PWM_CMPV_CV(value) ((PWM_CMPV_CV_Msk & ((value) << PWM_CMPV_CV_Pos)))
#define PWM_CMPV_CVM (0x1u << 24) /**< \brief (PWM_CMPV) Comparison x Value Mode */
/* -------- PWM_CMPVUPD : (PWM Offset: N/A) PWM Comparison 0 Value Update Register -------- */
#define PWM_CMPVUPD_CVUPD_Pos 0
#define PWM_CMPVUPD_CVUPD_Msk (0xffffffu << PWM_CMPVUPD_CVUPD_Pos) /**< \brief (PWM_CMPVUPD) Comparison x Value Update */
#define PWM_CMPVUPD_CVUPD(value) ((PWM_CMPVUPD_CVUPD_Msk & ((value) << PWM_CMPVUPD_CVUPD_Pos)))
#define PWM_CMPVUPD_CVMUPD (0x1u << 24) /**< \brief (PWM_CMPVUPD) Comparison x Value Mode Update */
/* -------- PWM_CMPM : (PWM Offset: N/A) PWM Comparison 0 Mode Register -------- */
#define PWM_CMPM_CEN (0x1u << 0) /**< \brief (PWM_CMPM) Comparison x Enable */
#define PWM_CMPM_CTR_Pos 4
#define PWM_CMPM_CTR_Msk (0xfu << PWM_CMPM_CTR_Pos) /**< \brief (PWM_CMPM) Comparison x Trigger */
#define PWM_CMPM_CTR(value) ((PWM_CMPM_CTR_Msk & ((value) << PWM_CMPM_CTR_Pos)))
#define PWM_CMPM_CPR_Pos 8
#define PWM_CMPM_CPR_Msk (0xfu << PWM_CMPM_CPR_Pos) /**< \brief (PWM_CMPM) Comparison x Period */
#define PWM_CMPM_CPR(value) ((PWM_CMPM_CPR_Msk & ((value) << PWM_CMPM_CPR_Pos)))
#define PWM_CMPM_CPRCNT_Pos 12
#define PWM_CMPM_CPRCNT_Msk (0xfu << PWM_CMPM_CPRCNT_Pos) /**< \brief (PWM_CMPM) Comparison x Period Counter */
#define PWM_CMPM_CPRCNT(value) ((PWM_CMPM_CPRCNT_Msk & ((value) << PWM_CMPM_CPRCNT_Pos)))
#define PWM_CMPM_CUPR_Pos 16
#define PWM_CMPM_CUPR_Msk (0xfu << PWM_CMPM_CUPR_Pos) /**< \brief (PWM_CMPM) Comparison x Update Period */
#define PWM_CMPM_CUPR(value) ((PWM_CMPM_CUPR_Msk & ((value) << PWM_CMPM_CUPR_Pos)))
#define PWM_CMPM_CUPRCNT_Pos 20
#define PWM_CMPM_CUPRCNT_Msk (0xfu << PWM_CMPM_CUPRCNT_Pos) /**< \brief (PWM_CMPM) Comparison x Update Period Counter */
#define PWM_CMPM_CUPRCNT(value) ((PWM_CMPM_CUPRCNT_Msk & ((value) << PWM_CMPM_CUPRCNT_Pos)))
/* -------- PWM_CMPMUPD : (PWM Offset: N/A) PWM Comparison 0 Mode Update Register -------- */
#define PWM_CMPMUPD_CENUPD (0x1u << 0) /**< \brief (PWM_CMPMUPD) Comparison x Enable Update */
#define PWM_CMPMUPD_CTRUPD_Pos 4
#define PWM_CMPMUPD_CTRUPD_Msk (0xfu << PWM_CMPMUPD_CTRUPD_Pos) /**< \brief (PWM_CMPMUPD) Comparison x Trigger Update */
#define PWM_CMPMUPD_CTRUPD(value) ((PWM_CMPMUPD_CTRUPD_Msk & ((value) << PWM_CMPMUPD_CTRUPD_Pos)))
#define PWM_CMPMUPD_CPRUPD_Pos 8
#define PWM_CMPMUPD_CPRUPD_Msk (0xfu << PWM_CMPMUPD_CPRUPD_Pos) /**< \brief (PWM_CMPMUPD) Comparison x Period Update */
#define PWM_CMPMUPD_CPRUPD(value) ((PWM_CMPMUPD_CPRUPD_Msk & ((value) << PWM_CMPMUPD_CPRUPD_Pos)))
#define PWM_CMPMUPD_CUPRUPD_Pos 16
#define PWM_CMPMUPD_CUPRUPD_Msk (0xfu << PWM_CMPMUPD_CUPRUPD_Pos) /**< \brief (PWM_CMPMUPD) Comparison x Update Period Update */
#define PWM_CMPMUPD_CUPRUPD(value) ((PWM_CMPMUPD_CUPRUPD_Msk & ((value) << PWM_CMPMUPD_CUPRUPD_Pos)))
/* -------- PWM_CMR : (PWM Offset: N/A) PWM Channel Mode Register -------- */
#define PWM_CMR_CPRE_Pos 0
#define PWM_CMR_CPRE_Msk (0xfu << PWM_CMR_CPRE_Pos) /**< \brief (PWM_CMR) Channel Pre-scaler */
#define PWM_CMR_CPRE(value) ((PWM_CMR_CPRE_Msk & ((value) << PWM_CMR_CPRE_Pos)))
#define   PWM_CMR_CPRE_MCK (0x0u << 0) /**< \brief (PWM_CMR) Peripheral clock */
#define   PWM_CMR_CPRE_MCK_DIV_2 (0x1u << 0) /**< \brief (PWM_CMR) Peripheral clock/2 */
#define   PWM_CMR_CPRE_MCK_DIV_4 (0x2u << 0) /**< \brief (PWM_CMR) Peripheral clock/4 */
#define   PWM_CMR_CPRE_MCK_DIV_8 (0x3u << 0) /**< \brief (PWM_CMR) Peripheral clock/8 */
#define   PWM_CMR_CPRE_MCK_DIV_16 (0x4u << 0) /**< \brief (PWM_CMR) Peripheral clock/16 */
#define   PWM_CMR_CPRE_MCK_DIV_32 (0x5u << 0) /**< \brief (PWM_CMR) Peripheral clock/32 */
#define   PWM_CMR_CPRE_MCK_DIV_64 (0x6u << 0) /**< \brief (PWM_CMR) Peripheral clock/64 */
#define   PWM_CMR_CPRE_MCK_DIV_128 (0x7u << 0) /**< \brief (PWM_CMR) Peripheral clock/128 */
#define   PWM_CMR_CPRE_MCK_DIV_256 (0x8u << 0) /**< \brief (PWM_CMR) Peripheral clock/256 */
#define   PWM_CMR_CPRE_MCK_DIV_512 (0x9u << 0) /**< \brief (PWM_CMR) Peripheral clock/512 */
#define   PWM_CMR_CPRE_MCK_DIV_1024 (0xAu << 0) /**< \brief (PWM_CMR) Peripheral clock/1024 */
#define   PWM_CMR_CPRE_CLKA (0xBu << 0) /**< \brief (PWM_CMR) Clock A */
#define   PWM_CMR_CPRE_CLKB (0xCu << 0) /**< \brief (PWM_CMR) Clock B */
#define PWM_CMR_CALG (0x1u << 8) /**< \brief (PWM_CMR) Channel Alignment */
#define PWM_CMR_CPOL (0x1u << 9) /**< \brief (PWM_CMR) Channel Polarity */
#define PWM_CMR_CES (0x1u << 10) /**< \brief (PWM_CMR) Counter Event Selection */
#define PWM_CMR_DTE (0x1u << 16) /**< \brief (PWM_CMR) Dead-Time Generator Enable */
#define PWM_CMR_DTHI (0x1u << 17) /**< \brief (PWM_CMR) Dead-Time PWMHx Output Inverted */
#define PWM_CMR_DTLI (0x1u << 18) /**< \brief (PWM_CMR) Dead-Time PWMLx Output Inverted */
/* -------- PWM_CDTY : (PWM Offset: N/A) PWM Channel Duty Cycle Register -------- */
#define PWM_CDTY_CDTY_Pos 0
#define PWM_CDTY_CDTY_Msk (0xffffffu << PWM_CDTY_CDTY_Pos) /**< \brief (PWM_CDTY) Channel Duty-Cycle */
#define PWM_CDTY_CDTY(value) ((PWM_CDTY_CDTY_Msk & ((value) << PWM_CDTY_CDTY_Pos)))
/* -------- PWM_CDTYUPD : (PWM Offset: N/A) PWM Channel Duty Cycle Update Register -------- */
#define PWM_CDTYUPD_CDTYUPD_Pos 0
#define PWM_CDTYUPD_CDTYUPD_Msk (0xffffffu << PWM_CDTYUPD_CDTYUPD_Pos) /**< \brief (PWM_CDTYUPD) Channel Duty-Cycle Update */
#define PWM_CDTYUPD_CDTYUPD(value) ((PWM_CDTYUPD_CDTYUPD_Msk & ((value) << PWM_CDTYUPD_CDTYUPD_Pos)))
/* -------- PWM_CPRD : (PWM Offset: N/A) PWM Channel Period Register -------- */
#define PWM_CPRD_CPRD_Pos 0
#define PWM_CPRD_CPRD_Msk (0xffffffu << PWM_CPRD_CPRD_Pos) /**< \brief (PWM_CPRD) Channel Period */
#define PWM_CPRD_CPRD(value) ((PWM_CPRD_CPRD_Msk & ((value) << PWM_CPRD_CPRD_Pos)))
/* -------- PWM_CPRDUPD : (PWM Offset: N/A) PWM Channel Period Update Register -------- */
#define PWM_CPRDUPD_CPRDUPD_Pos 0
#define PWM_CPRDUPD_CPRDUPD_Msk (0xffffffu << PWM_CPRDUPD_CPRDUPD_Pos) /**< \brief (PWM_CPRDUPD) Channel Period Update */
#define PWM_CPRDUPD_CPRDUPD(value) ((PWM_CPRDUPD_CPRDUPD_Msk & ((value) << PWM_CPRDUPD_CPRDUPD_Pos)))
/* -------- PWM_CCNT : (PWM Offset: N/A) PWM Channel Counter Register -------- */
#define PWM_CCNT_CNT_Pos 0
#define PWM_CCNT_CNT_Msk (0xffffffu << PWM_CCNT_CNT_Pos) /**< \brief (PWM_CCNT) Channel Counter Register */
/* -------- PWM_DT : (PWM Offset: N/A) PWM Channel Dead Time Register -------- */
#define PWM_DT_DTH_Pos 0
#define PWM_DT_DTH_Msk (0xffffu << PWM_DT_DTH_Pos) /**< \brief (PWM_DT) Dead-Time Value for PWMHx Output */
#define PWM_DT_DTH(value) ((PWM_DT_DTH_Msk & ((value) << PWM_DT_DTH_Pos)))
#define PWM_DT_DTL_Pos 16
#define PWM_DT_DTL_Msk (0xffffu << PWM_DT_DTL_Pos) /**< \brief (PWM_DT) Dead-Time Value for PWMLx Output */
#define PWM_DT_DTL(value) ((PWM_DT_DTL_Msk & ((value) << PWM_DT_DTL_Pos)))
/* -------- PWM_DTUPD : (PWM Offset: N/A) PWM Channel Dead Time Update Register -------- */
#define PWM_DTUPD_DTHUPD_Pos 0
#define PWM_DTUPD_DTHUPD_Msk (0xffffu << PWM_DTUPD_DTHUPD_Pos) /**< \brief (PWM_DTUPD) Dead-Time Value Update for PWMHx Output */
#define PWM_DTUPD_DTHUPD(value) ((PWM_DTUPD_DTHUPD_Msk & ((value) << PWM_DTUPD_DTHUPD_Pos)))
#define PWM_DTUPD_DTLUPD_Pos 16
#define PWM_DTUPD_DTLUPD_Msk (0xffffu << PWM_DTUPD_DTLUPD_Pos) /**< \brief (PWM_DTUPD) Dead-Time Value Update for PWMLx Output */
#define PWM_DTUPD_DTLUPD(value) ((PWM_DTUPD_DTLUPD_Msk & ((value) << PWM_DTUPD_DTLUPD_Pos)))

/*@}*/


#endif /* _SAM4S_PWM_COMPONENT_ */
