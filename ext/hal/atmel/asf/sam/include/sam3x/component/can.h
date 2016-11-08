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

#ifndef _SAM3XA_CAN_COMPONENT_
#define _SAM3XA_CAN_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Controller Area Network */
/* ============================================================================= */
/** \addtogroup SAM3XA_CAN Controller Area Network */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief CanMb hardware registers */
typedef struct {
  __IO uint32_t CAN_MMR;  /**< \brief (CanMb Offset: 0x0) Mailbox Mode Register */
  __IO uint32_t CAN_MAM;  /**< \brief (CanMb Offset: 0x4) Mailbox Acceptance Mask Register */
  __IO uint32_t CAN_MID;  /**< \brief (CanMb Offset: 0x8) Mailbox ID Register */
  __I  uint32_t CAN_MFID; /**< \brief (CanMb Offset: 0xC) Mailbox Family ID Register */
  __I  uint32_t CAN_MSR;  /**< \brief (CanMb Offset: 0x10) Mailbox Status Register */
  __IO uint32_t CAN_MDL;  /**< \brief (CanMb Offset: 0x14) Mailbox Data Low Register */
  __IO uint32_t CAN_MDH;  /**< \brief (CanMb Offset: 0x18) Mailbox Data High Register */
  __O  uint32_t CAN_MCR;  /**< \brief (CanMb Offset: 0x1C) Mailbox Control Register */
} CanMb;
/** \brief Can hardware registers */
#define CANMB_NUMBER 8
typedef struct {
  __IO uint32_t CAN_MR;               /**< \brief (Can Offset: 0x0000) Mode Register */
  __O  uint32_t CAN_IER;              /**< \brief (Can Offset: 0x0004) Interrupt Enable Register */
  __O  uint32_t CAN_IDR;              /**< \brief (Can Offset: 0x0008) Interrupt Disable Register */
  __I  uint32_t CAN_IMR;              /**< \brief (Can Offset: 0x000C) Interrupt Mask Register */
  __I  uint32_t CAN_SR;               /**< \brief (Can Offset: 0x0010) Status Register */
  __IO uint32_t CAN_BR;               /**< \brief (Can Offset: 0x0014) Baudrate Register */
  __I  uint32_t CAN_TIM;              /**< \brief (Can Offset: 0x0018) Timer Register */
  __I  uint32_t CAN_TIMESTP;          /**< \brief (Can Offset: 0x001C) Timestamp Register */
  __I  uint32_t CAN_ECR;              /**< \brief (Can Offset: 0x0020) Error Counter Register */
  __O  uint32_t CAN_TCR;              /**< \brief (Can Offset: 0x0024) Transfer Command Register */
  __O  uint32_t CAN_ACR;              /**< \brief (Can Offset: 0x0028) Abort Command Register */
  __I  uint32_t Reserved1[46];
  __IO uint32_t CAN_WPMR;             /**< \brief (Can Offset: 0x00E4) Write Protect Mode Register */
  __I  uint32_t CAN_WPSR;             /**< \brief (Can Offset: 0x00E8) Write Protect Status Register */
  __I  uint32_t Reserved2[69];
       CanMb    CAN_MB[CANMB_NUMBER]; /**< \brief (Can Offset: 0x200) MB = 0 .. 7 */
} Can;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_MR : (CAN Offset: 0x0000) Mode Register -------- */
#define CAN_MR_CANEN (0x1u << 0) /**< \brief (CAN_MR) CAN Controller Enable */
#define CAN_MR_LPM (0x1u << 1) /**< \brief (CAN_MR) Disable/Enable Low Power Mode */
#define CAN_MR_ABM (0x1u << 2) /**< \brief (CAN_MR) Disable/Enable Autobaud/Listen mode */
#define CAN_MR_OVL (0x1u << 3) /**< \brief (CAN_MR) Disable/Enable Overload Frame */
#define CAN_MR_TEOF (0x1u << 4) /**< \brief (CAN_MR) Timestamp messages at each end of Frame */
#define CAN_MR_TTM (0x1u << 5) /**< \brief (CAN_MR) Disable/Enable Time Triggered Mode */
#define CAN_MR_TIMFRZ (0x1u << 6) /**< \brief (CAN_MR) Enable Timer Freeze */
#define CAN_MR_DRPT (0x1u << 7) /**< \brief (CAN_MR) Disable Repeat */
#define CAN_MR_RXSYNC_Pos 24
#define CAN_MR_RXSYNC_Msk (0x7u << CAN_MR_RXSYNC_Pos) /**< \brief (CAN_MR) Reception Synchronization Stage (not readable) */
#define   CAN_MR_RXSYNC_DOUBLE_PP (0x0u << 24) /**< \brief (CAN_MR) Rx Signal with Double Synchro Stages (2 Positive Edges) */
#define   CAN_MR_RXSYNC_DOUBLE_PN (0x1u << 24) /**< \brief (CAN_MR) Rx Signal with Double Synchro Stages (One Positive Edge and One Negative Edge) */
#define   CAN_MR_RXSYNC_SINGLE_P (0x2u << 24) /**< \brief (CAN_MR) Rx Signal with Single Synchro Stage (Positive Edge) */
#define   CAN_MR_RXSYNC_NONE (0x3u << 24) /**< \brief (CAN_MR) Rx Signal with No Synchro Stage */
/* -------- CAN_IER : (CAN Offset: 0x0004) Interrupt Enable Register -------- */
#define CAN_IER_MB0 (0x1u << 0) /**< \brief (CAN_IER) Mailbox 0 Interrupt Enable */
#define CAN_IER_MB1 (0x1u << 1) /**< \brief (CAN_IER) Mailbox 1 Interrupt Enable */
#define CAN_IER_MB2 (0x1u << 2) /**< \brief (CAN_IER) Mailbox 2 Interrupt Enable */
#define CAN_IER_MB3 (0x1u << 3) /**< \brief (CAN_IER) Mailbox 3 Interrupt Enable */
#define CAN_IER_MB4 (0x1u << 4) /**< \brief (CAN_IER) Mailbox 4 Interrupt Enable */
#define CAN_IER_MB5 (0x1u << 5) /**< \brief (CAN_IER) Mailbox 5 Interrupt Enable */
#define CAN_IER_MB6 (0x1u << 6) /**< \brief (CAN_IER) Mailbox 6 Interrupt Enable */
#define CAN_IER_MB7 (0x1u << 7) /**< \brief (CAN_IER) Mailbox 7 Interrupt Enable */
#define CAN_IER_ERRA (0x1u << 16) /**< \brief (CAN_IER) Error Active Mode Interrupt Enable */
#define CAN_IER_WARN (0x1u << 17) /**< \brief (CAN_IER) Warning Limit Interrupt Enable */
#define CAN_IER_ERRP (0x1u << 18) /**< \brief (CAN_IER) Error Passive Mode Interrupt Enable */
#define CAN_IER_BOFF (0x1u << 19) /**< \brief (CAN_IER) Bus Off Mode Interrupt Enable */
#define CAN_IER_SLEEP (0x1u << 20) /**< \brief (CAN_IER) Sleep Interrupt Enable */
#define CAN_IER_WAKEUP (0x1u << 21) /**< \brief (CAN_IER) Wakeup Interrupt Enable */
#define CAN_IER_TOVF (0x1u << 22) /**< \brief (CAN_IER) Timer Overflow Interrupt Enable */
#define CAN_IER_TSTP (0x1u << 23) /**< \brief (CAN_IER) TimeStamp Interrupt Enable */
#define CAN_IER_CERR (0x1u << 24) /**< \brief (CAN_IER) CRC Error Interrupt Enable */
#define CAN_IER_SERR (0x1u << 25) /**< \brief (CAN_IER) Stuffing Error Interrupt Enable */
#define CAN_IER_AERR (0x1u << 26) /**< \brief (CAN_IER) Acknowledgment Error Interrupt Enable */
#define CAN_IER_FERR (0x1u << 27) /**< \brief (CAN_IER) Form Error Interrupt Enable */
#define CAN_IER_BERR (0x1u << 28) /**< \brief (CAN_IER) Bit Error Interrupt Enable */
/* -------- CAN_IDR : (CAN Offset: 0x0008) Interrupt Disable Register -------- */
#define CAN_IDR_MB0 (0x1u << 0) /**< \brief (CAN_IDR) Mailbox 0 Interrupt Disable */
#define CAN_IDR_MB1 (0x1u << 1) /**< \brief (CAN_IDR) Mailbox 1 Interrupt Disable */
#define CAN_IDR_MB2 (0x1u << 2) /**< \brief (CAN_IDR) Mailbox 2 Interrupt Disable */
#define CAN_IDR_MB3 (0x1u << 3) /**< \brief (CAN_IDR) Mailbox 3 Interrupt Disable */
#define CAN_IDR_MB4 (0x1u << 4) /**< \brief (CAN_IDR) Mailbox 4 Interrupt Disable */
#define CAN_IDR_MB5 (0x1u << 5) /**< \brief (CAN_IDR) Mailbox 5 Interrupt Disable */
#define CAN_IDR_MB6 (0x1u << 6) /**< \brief (CAN_IDR) Mailbox 6 Interrupt Disable */
#define CAN_IDR_MB7 (0x1u << 7) /**< \brief (CAN_IDR) Mailbox 7 Interrupt Disable */
#define CAN_IDR_ERRA (0x1u << 16) /**< \brief (CAN_IDR) Error Active Mode Interrupt Disable */
#define CAN_IDR_WARN (0x1u << 17) /**< \brief (CAN_IDR) Warning Limit Interrupt Disable */
#define CAN_IDR_ERRP (0x1u << 18) /**< \brief (CAN_IDR) Error Passive Mode Interrupt Disable */
#define CAN_IDR_BOFF (0x1u << 19) /**< \brief (CAN_IDR) Bus Off Mode Interrupt Disable */
#define CAN_IDR_SLEEP (0x1u << 20) /**< \brief (CAN_IDR) Sleep Interrupt Disable */
#define CAN_IDR_WAKEUP (0x1u << 21) /**< \brief (CAN_IDR) Wakeup Interrupt Disable */
#define CAN_IDR_TOVF (0x1u << 22) /**< \brief (CAN_IDR) Timer Overflow Interrupt */
#define CAN_IDR_TSTP (0x1u << 23) /**< \brief (CAN_IDR) TimeStamp Interrupt Disable */
#define CAN_IDR_CERR (0x1u << 24) /**< \brief (CAN_IDR) CRC Error Interrupt Disable */
#define CAN_IDR_SERR (0x1u << 25) /**< \brief (CAN_IDR) Stuffing Error Interrupt Disable */
#define CAN_IDR_AERR (0x1u << 26) /**< \brief (CAN_IDR) Acknowledgment Error Interrupt Disable */
#define CAN_IDR_FERR (0x1u << 27) /**< \brief (CAN_IDR) Form Error Interrupt Disable */
#define CAN_IDR_BERR (0x1u << 28) /**< \brief (CAN_IDR) Bit Error Interrupt Disable */
/* -------- CAN_IMR : (CAN Offset: 0x000C) Interrupt Mask Register -------- */
#define CAN_IMR_MB0 (0x1u << 0) /**< \brief (CAN_IMR) Mailbox 0 Interrupt Mask */
#define CAN_IMR_MB1 (0x1u << 1) /**< \brief (CAN_IMR) Mailbox 1 Interrupt Mask */
#define CAN_IMR_MB2 (0x1u << 2) /**< \brief (CAN_IMR) Mailbox 2 Interrupt Mask */
#define CAN_IMR_MB3 (0x1u << 3) /**< \brief (CAN_IMR) Mailbox 3 Interrupt Mask */
#define CAN_IMR_MB4 (0x1u << 4) /**< \brief (CAN_IMR) Mailbox 4 Interrupt Mask */
#define CAN_IMR_MB5 (0x1u << 5) /**< \brief (CAN_IMR) Mailbox 5 Interrupt Mask */
#define CAN_IMR_MB6 (0x1u << 6) /**< \brief (CAN_IMR) Mailbox 6 Interrupt Mask */
#define CAN_IMR_MB7 (0x1u << 7) /**< \brief (CAN_IMR) Mailbox 7 Interrupt Mask */
#define CAN_IMR_ERRA (0x1u << 16) /**< \brief (CAN_IMR) Error Active Mode Interrupt Mask */
#define CAN_IMR_WARN (0x1u << 17) /**< \brief (CAN_IMR) Warning Limit Interrupt Mask */
#define CAN_IMR_ERRP (0x1u << 18) /**< \brief (CAN_IMR) Error Passive Mode Interrupt Mask */
#define CAN_IMR_BOFF (0x1u << 19) /**< \brief (CAN_IMR) Bus Off Mode Interrupt Mask */
#define CAN_IMR_SLEEP (0x1u << 20) /**< \brief (CAN_IMR) Sleep Interrupt Mask */
#define CAN_IMR_WAKEUP (0x1u << 21) /**< \brief (CAN_IMR) Wakeup Interrupt Mask */
#define CAN_IMR_TOVF (0x1u << 22) /**< \brief (CAN_IMR) Timer Overflow Interrupt Mask */
#define CAN_IMR_TSTP (0x1u << 23) /**< \brief (CAN_IMR) Timestamp Interrupt Mask */
#define CAN_IMR_CERR (0x1u << 24) /**< \brief (CAN_IMR) CRC Error Interrupt Mask */
#define CAN_IMR_SERR (0x1u << 25) /**< \brief (CAN_IMR) Stuffing Error Interrupt Mask */
#define CAN_IMR_AERR (0x1u << 26) /**< \brief (CAN_IMR) Acknowledgment Error Interrupt Mask */
#define CAN_IMR_FERR (0x1u << 27) /**< \brief (CAN_IMR) Form Error Interrupt Mask */
#define CAN_IMR_BERR (0x1u << 28) /**< \brief (CAN_IMR) Bit Error Interrupt Mask */
/* -------- CAN_SR : (CAN Offset: 0x0010) Status Register -------- */
#define CAN_SR_MB0 (0x1u << 0) /**< \brief (CAN_SR) Mailbox 0 Event */
#define CAN_SR_MB1 (0x1u << 1) /**< \brief (CAN_SR) Mailbox 1 Event */
#define CAN_SR_MB2 (0x1u << 2) /**< \brief (CAN_SR) Mailbox 2 Event */
#define CAN_SR_MB3 (0x1u << 3) /**< \brief (CAN_SR) Mailbox 3 Event */
#define CAN_SR_MB4 (0x1u << 4) /**< \brief (CAN_SR) Mailbox 4 Event */
#define CAN_SR_MB5 (0x1u << 5) /**< \brief (CAN_SR) Mailbox 5 Event */
#define CAN_SR_MB6 (0x1u << 6) /**< \brief (CAN_SR) Mailbox 6 Event */
#define CAN_SR_MB7 (0x1u << 7) /**< \brief (CAN_SR) Mailbox 7 Event */
#define CAN_SR_ERRA (0x1u << 16) /**< \brief (CAN_SR) Error Active Mode */
#define CAN_SR_WARN (0x1u << 17) /**< \brief (CAN_SR) Warning Limit */
#define CAN_SR_ERRP (0x1u << 18) /**< \brief (CAN_SR) Error Passive Mode */
#define CAN_SR_BOFF (0x1u << 19) /**< \brief (CAN_SR) Bus Off Mode */
#define CAN_SR_SLEEP (0x1u << 20) /**< \brief (CAN_SR) CAN controller in Low power Mode */
#define CAN_SR_WAKEUP (0x1u << 21) /**< \brief (CAN_SR) CAN controller is not in Low power Mode */
#define CAN_SR_TOVF (0x1u << 22) /**< \brief (CAN_SR) Timer Overflow */
#define CAN_SR_TSTP (0x1u << 23) /**< \brief (CAN_SR)  */
#define CAN_SR_CERR (0x1u << 24) /**< \brief (CAN_SR) Mailbox CRC Error */
#define CAN_SR_SERR (0x1u << 25) /**< \brief (CAN_SR) Mailbox Stuffing Error */
#define CAN_SR_AERR (0x1u << 26) /**< \brief (CAN_SR) Acknowledgment Error */
#define CAN_SR_FERR (0x1u << 27) /**< \brief (CAN_SR) Form Error */
#define CAN_SR_BERR (0x1u << 28) /**< \brief (CAN_SR) Bit Error */
#define CAN_SR_RBSY (0x1u << 29) /**< \brief (CAN_SR) Receiver busy */
#define CAN_SR_TBSY (0x1u << 30) /**< \brief (CAN_SR) Transmitter busy */
#define CAN_SR_OVLSY (0x1u << 31) /**< \brief (CAN_SR) Overload busy */
/* -------- CAN_BR : (CAN Offset: 0x0014) Baudrate Register -------- */
#define CAN_BR_PHASE2_Pos 0
#define CAN_BR_PHASE2_Msk (0x7u << CAN_BR_PHASE2_Pos) /**< \brief (CAN_BR) Phase 2 segment */
#define CAN_BR_PHASE2(value) ((CAN_BR_PHASE2_Msk & ((value) << CAN_BR_PHASE2_Pos)))
#define CAN_BR_PHASE1_Pos 4
#define CAN_BR_PHASE1_Msk (0x7u << CAN_BR_PHASE1_Pos) /**< \brief (CAN_BR) Phase 1 segment */
#define CAN_BR_PHASE1(value) ((CAN_BR_PHASE1_Msk & ((value) << CAN_BR_PHASE1_Pos)))
#define CAN_BR_PROPAG_Pos 8
#define CAN_BR_PROPAG_Msk (0x7u << CAN_BR_PROPAG_Pos) /**< \brief (CAN_BR) Programming time segment */
#define CAN_BR_PROPAG(value) ((CAN_BR_PROPAG_Msk & ((value) << CAN_BR_PROPAG_Pos)))
#define CAN_BR_SJW_Pos 12
#define CAN_BR_SJW_Msk (0x3u << CAN_BR_SJW_Pos) /**< \brief (CAN_BR) Re-synchronization jump width */
#define CAN_BR_SJW(value) ((CAN_BR_SJW_Msk & ((value) << CAN_BR_SJW_Pos)))
#define CAN_BR_BRP_Pos 16
#define CAN_BR_BRP_Msk (0x7fu << CAN_BR_BRP_Pos) /**< \brief (CAN_BR) Baudrate Prescaler. */
#define CAN_BR_BRP(value) ((CAN_BR_BRP_Msk & ((value) << CAN_BR_BRP_Pos)))
#define CAN_BR_SMP (0x1u << 24) /**< \brief (CAN_BR) Sampling Mode */
#define   CAN_BR_SMP_ONCE (0x0u << 24) /**< \brief (CAN_BR) The incoming bit stream is sampled once at sample point. */
#define   CAN_BR_SMP_THREE (0x1u << 24) /**< \brief (CAN_BR) The incoming bit stream is sampled three times with a period of a MCK clock period, centered on sample point. */
/* -------- CAN_TIM : (CAN Offset: 0x0018) Timer Register -------- */
#define CAN_TIM_TIMER_Pos 0
#define CAN_TIM_TIMER_Msk (0xffffu << CAN_TIM_TIMER_Pos) /**< \brief (CAN_TIM) Timer */
/* -------- CAN_TIMESTP : (CAN Offset: 0x001C) Timestamp Register -------- */
#define CAN_TIMESTP_MTIMESTAMP_Pos 0
#define CAN_TIMESTP_MTIMESTAMP_Msk (0xffffu << CAN_TIMESTP_MTIMESTAMP_Pos) /**< \brief (CAN_TIMESTP) Timestamp */
/* -------- CAN_ECR : (CAN Offset: 0x0020) Error Counter Register -------- */
#define CAN_ECR_REC_Pos 0
#define CAN_ECR_REC_Msk (0xffu << CAN_ECR_REC_Pos) /**< \brief (CAN_ECR) Receive Error Counter */
#define CAN_ECR_TEC_Pos 16
#define CAN_ECR_TEC_Msk (0xffu << CAN_ECR_TEC_Pos) /**< \brief (CAN_ECR) Transmit Error Counter */
/* -------- CAN_TCR : (CAN Offset: 0x0024) Transfer Command Register -------- */
#define CAN_TCR_MB0 (0x1u << 0) /**< \brief (CAN_TCR) Transfer Request for Mailbox 0 */
#define CAN_TCR_MB1 (0x1u << 1) /**< \brief (CAN_TCR) Transfer Request for Mailbox 1 */
#define CAN_TCR_MB2 (0x1u << 2) /**< \brief (CAN_TCR) Transfer Request for Mailbox 2 */
#define CAN_TCR_MB3 (0x1u << 3) /**< \brief (CAN_TCR) Transfer Request for Mailbox 3 */
#define CAN_TCR_MB4 (0x1u << 4) /**< \brief (CAN_TCR) Transfer Request for Mailbox 4 */
#define CAN_TCR_MB5 (0x1u << 5) /**< \brief (CAN_TCR) Transfer Request for Mailbox 5 */
#define CAN_TCR_MB6 (0x1u << 6) /**< \brief (CAN_TCR) Transfer Request for Mailbox 6 */
#define CAN_TCR_MB7 (0x1u << 7) /**< \brief (CAN_TCR) Transfer Request for Mailbox 7 */
#define CAN_TCR_TIMRST (0x1u << 31) /**< \brief (CAN_TCR) Timer Reset */
/* -------- CAN_ACR : (CAN Offset: 0x0028) Abort Command Register -------- */
#define CAN_ACR_MB0 (0x1u << 0) /**< \brief (CAN_ACR) Abort Request for Mailbox 0 */
#define CAN_ACR_MB1 (0x1u << 1) /**< \brief (CAN_ACR) Abort Request for Mailbox 1 */
#define CAN_ACR_MB2 (0x1u << 2) /**< \brief (CAN_ACR) Abort Request for Mailbox 2 */
#define CAN_ACR_MB3 (0x1u << 3) /**< \brief (CAN_ACR) Abort Request for Mailbox 3 */
#define CAN_ACR_MB4 (0x1u << 4) /**< \brief (CAN_ACR) Abort Request for Mailbox 4 */
#define CAN_ACR_MB5 (0x1u << 5) /**< \brief (CAN_ACR) Abort Request for Mailbox 5 */
#define CAN_ACR_MB6 (0x1u << 6) /**< \brief (CAN_ACR) Abort Request for Mailbox 6 */
#define CAN_ACR_MB7 (0x1u << 7) /**< \brief (CAN_ACR) Abort Request for Mailbox 7 */
/* -------- CAN_WPMR : (CAN Offset: 0x00E4) Write Protect Mode Register -------- */
#define CAN_WPMR_WPEN (0x1u << 0) /**< \brief (CAN_WPMR) Write Protection Enable */
#define CAN_WPMR_WPKEY_Pos 8
#define CAN_WPMR_WPKEY_Msk (0xffffffu << CAN_WPMR_WPKEY_Pos) /**< \brief (CAN_WPMR) SPI Write Protection Key Password */
#define CAN_WPMR_WPKEY(value) ((CAN_WPMR_WPKEY_Msk & ((value) << CAN_WPMR_WPKEY_Pos)))
/* -------- CAN_WPSR : (CAN Offset: 0x00E8) Write Protect Status Register -------- */
#define CAN_WPSR_WPVS (0x1u << 0) /**< \brief (CAN_WPSR) Write Protection Violation Status */
#define CAN_WPSR_WPVSRC_Pos 8
#define CAN_WPSR_WPVSRC_Msk (0xffu << CAN_WPSR_WPVSRC_Pos) /**< \brief (CAN_WPSR) Write Protection Violation Source */
/* -------- CAN_MMR : (CAN Offset: N/A) Mailbox Mode Register -------- */
#define CAN_MMR_MTIMEMARK_Pos 0
#define CAN_MMR_MTIMEMARK_Msk (0xffffu << CAN_MMR_MTIMEMARK_Pos) /**< \brief (CAN_MMR) Mailbox Timemark */
#define CAN_MMR_MTIMEMARK(value) ((CAN_MMR_MTIMEMARK_Msk & ((value) << CAN_MMR_MTIMEMARK_Pos)))
#define CAN_MMR_PRIOR_Pos 16
#define CAN_MMR_PRIOR_Msk (0xfu << CAN_MMR_PRIOR_Pos) /**< \brief (CAN_MMR) Mailbox Priority */
#define CAN_MMR_PRIOR(value) ((CAN_MMR_PRIOR_Msk & ((value) << CAN_MMR_PRIOR_Pos)))
#define CAN_MMR_MOT_Pos 24
#define CAN_MMR_MOT_Msk (0x7u << CAN_MMR_MOT_Pos) /**< \brief (CAN_MMR) Mailbox Object Type */
#define   CAN_MMR_MOT_MB_DISABLED (0x0u << 24) /**< \brief (CAN_MMR) Mailbox is disabled. This prevents receiving or transmitting any messages with this mailbox. */
#define   CAN_MMR_MOT_MB_RX (0x1u << 24) /**< \brief (CAN_MMR) Reception Mailbox. Mailbox is configured for reception. If a message is received while the mailbox data register is full, it is discarded. */
#define   CAN_MMR_MOT_MB_RX_OVERWRITE (0x2u << 24) /**< \brief (CAN_MMR) Reception mailbox with overwrite. Mailbox is configured for reception. If a message is received while the mailbox is full, it overwrites the previous message. */
#define   CAN_MMR_MOT_MB_TX (0x3u << 24) /**< \brief (CAN_MMR) Transmit mailbox. Mailbox is configured for transmission. */
#define   CAN_MMR_MOT_MB_CONSUMER (0x4u << 24) /**< \brief (CAN_MMR) Consumer Mailbox. Mailbox is configured in reception but behaves as a Transmit Mailbox, i.e., it sends a remote frame and waits for an answer. */
#define   CAN_MMR_MOT_MB_PRODUCER (0x5u << 24) /**< \brief (CAN_MMR) Producer Mailbox. Mailbox is configured in transmission but also behaves like a reception mailbox, i.e., it waits to receive a Remote Frame before sending its contents. */
/* -------- CAN_MAM : (CAN Offset: N/A) Mailbox Acceptance Mask Register -------- */
#define CAN_MAM_MIDvB_Pos 0
#define CAN_MAM_MIDvB_Msk (0x3ffffu << CAN_MAM_MIDvB_Pos) /**< \brief (CAN_MAM) Complementary bits for identifier in extended frame mode */
#define CAN_MAM_MIDvB(value) ((CAN_MAM_MIDvB_Msk & ((value) << CAN_MAM_MIDvB_Pos)))
#define CAN_MAM_MIDvA_Pos 18
#define CAN_MAM_MIDvA_Msk (0x7ffu << CAN_MAM_MIDvA_Pos) /**< \brief (CAN_MAM) Identifier for standard frame mode */
#define CAN_MAM_MIDvA(value) ((CAN_MAM_MIDvA_Msk & ((value) << CAN_MAM_MIDvA_Pos)))
#define CAN_MAM_MIDE (0x1u << 29) /**< \brief (CAN_MAM) Identifier Version */
/* -------- CAN_MID : (CAN Offset: N/A) Mailbox ID Register -------- */
#define CAN_MID_MIDvB_Pos 0
#define CAN_MID_MIDvB_Msk (0x3ffffu << CAN_MID_MIDvB_Pos) /**< \brief (CAN_MID) Complementary bits for identifier in extended frame mode */
#define CAN_MID_MIDvB(value) ((CAN_MID_MIDvB_Msk & ((value) << CAN_MID_MIDvB_Pos)))
#define CAN_MID_MIDvA_Pos 18
#define CAN_MID_MIDvA_Msk (0x7ffu << CAN_MID_MIDvA_Pos) /**< \brief (CAN_MID) Identifier for standard frame mode */
#define CAN_MID_MIDvA(value) ((CAN_MID_MIDvA_Msk & ((value) << CAN_MID_MIDvA_Pos)))
#define CAN_MID_MIDE (0x1u << 29) /**< \brief (CAN_MID) Identifier Version */
/* -------- CAN_MFID : (CAN Offset: N/A) Mailbox Family ID Register -------- */
#define CAN_MFID_MFID_Pos 0
#define CAN_MFID_MFID_Msk (0x1fffffffu << CAN_MFID_MFID_Pos) /**< \brief (CAN_MFID) Family ID */
/* -------- CAN_MSR : (CAN Offset: N/A) Mailbox Status Register -------- */
#define CAN_MSR_MTIMESTAMP_Pos 0
#define CAN_MSR_MTIMESTAMP_Msk (0xffffu << CAN_MSR_MTIMESTAMP_Pos) /**< \brief (CAN_MSR) Timer value */
#define CAN_MSR_MDLC_Pos 16
#define CAN_MSR_MDLC_Msk (0xfu << CAN_MSR_MDLC_Pos) /**< \brief (CAN_MSR) Mailbox Data Length Code */
#define CAN_MSR_MRTR (0x1u << 20) /**< \brief (CAN_MSR) Mailbox Remote Transmission Request */
#define CAN_MSR_MABT (0x1u << 22) /**< \brief (CAN_MSR) Mailbox Message Abort */
#define CAN_MSR_MRDY (0x1u << 23) /**< \brief (CAN_MSR) Mailbox Ready */
#define CAN_MSR_MMI (0x1u << 24) /**< \brief (CAN_MSR) Mailbox Message Ignored */
/* -------- CAN_MDL : (CAN Offset: N/A) Mailbox Data Low Register -------- */
#define CAN_MDL_MDL_Pos 0
#define CAN_MDL_MDL_Msk (0xffffffffu << CAN_MDL_MDL_Pos) /**< \brief (CAN_MDL) Message Data Low Value */
#define CAN_MDL_MDL(value) ((CAN_MDL_MDL_Msk & ((value) << CAN_MDL_MDL_Pos)))
/* -------- CAN_MDH : (CAN Offset: N/A) Mailbox Data High Register -------- */
#define CAN_MDH_MDH_Pos 0
#define CAN_MDH_MDH_Msk (0xffffffffu << CAN_MDH_MDH_Pos) /**< \brief (CAN_MDH) Message Data High Value */
#define CAN_MDH_MDH(value) ((CAN_MDH_MDH_Msk & ((value) << CAN_MDH_MDH_Pos)))
/* -------- CAN_MCR : (CAN Offset: N/A) Mailbox Control Register -------- */
#define CAN_MCR_MDLC_Pos 16
#define CAN_MCR_MDLC_Msk (0xfu << CAN_MCR_MDLC_Pos) /**< \brief (CAN_MCR) Mailbox Data Length Code */
#define CAN_MCR_MDLC(value) ((CAN_MCR_MDLC_Msk & ((value) << CAN_MCR_MDLC_Pos)))
#define CAN_MCR_MRTR (0x1u << 20) /**< \brief (CAN_MCR) Mailbox Remote Transmission Request */
#define CAN_MCR_MACR (0x1u << 22) /**< \brief (CAN_MCR) Abort Request for Mailbox x */
#define CAN_MCR_MTCR (0x1u << 23) /**< \brief (CAN_MCR) Mailbox Transfer Command */

/*@}*/


#endif /* _SAM3XA_CAN_COMPONENT_ */
