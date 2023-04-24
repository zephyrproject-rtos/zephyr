/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_CAN_MCAN_PRIV_H_
#define ZEPHYR_DRIVERS_CAN_CAN_MCAN_PRIV_H_

#include <zephyr/drivers/can.h>

/*
 * Register Masks are taken from the stm32cube library and extended for
 * full M_CAN IPs
 * Copyright (c) 2019 STMicroelectronics.
 */
/****************  Bit definition for CAN_MCAN_CREL register  *****************/
/* Timestamp Day */
#define CAN_MCAN_CREL_DAY_POS	  (0U)
#define CAN_MCAN_CREL_DAY_MSK	  (0xFFUL << CAN_MCAN_CREL_DAY_POS)
#define CAN_MCAN_CREL_DAY	  CAN_MCAN_CREL_DAY_MSK
/* Timestamp Month */
#define CAN_MCAN_CREL_MON_POS	  (8U)
#define CAN_MCAN_CREL_MON_MSK	  (0xFFUL << CAN_MCAN_CREL_MON_POS)
#define CAN_MCAN_CREL_MON	  CAN_MCAN_CREL_MON_MSK
/* Timestamp Year */
#define CAN_MCAN_CREL_YEAR_POS	  (16U)
#define CAN_MCAN_CREL_YEAR_MSK	  (0xFUL << CAN_MCAN_CREL_YEAR_POS)
#define CAN_MCAN_CREL_YEAR	  CAN_MCAN_CREL_YEAR_MSK
/* Substep of Core release */
#define CAN_MCAN_CREL_SUBSTEP_POS (20U)
#define CAN_MCAN_CREL_SUBSTEP_MSK (0xFUL << CAN_MCAN_CREL_SUBSTEP_POS)
#define CAN_MCAN_CREL_SUBSTEP	  CAN_MCAN_CREL_SUBSTEP_MSK
/* Step of Core release */
#define CAN_MCAN_CREL_STEP_POS	  (24U)
#define CAN_MCAN_CREL_STEP_MSK	  (0xFUL << CAN_MCAN_CREL_STEP_POS)
#define CAN_MCAN_CREL_STEP	  CAN_MCAN_CREL_STEP_MSK
/* Core release */
#define CAN_MCAN_CREL_REL_POS	  (28U)
#define CAN_MCAN_CREL_REL_MSK	  (0xFUL << CAN_MCAN_CREL_REL_POS)
#define CAN_MCAN_CREL_REL	  CAN_MCAN_CREL_REL_MSK

/****************  Bit definition for CAN_MCAN_ENDN register  *****************/
/* Endianness Test Value */
#define CAN_MCAN_ENDN_ETV_POS (0U)
#define CAN_MCAN_ENDN_ETV_MSK (0xFFFFFFFFUL << CAN_MCAN_ENDN_ETV_POS)
#define CAN_MCAN_ENDN_ETV     CAN_MCAN_ENDN_ETV_MSK

/***************  Bit definition for CAN_MCAN_DBTP register  ******************/
/* Synchronization Jump Width */
#define CAN_MCAN_DBTP_DSJW_POS	 (0U)
#define CAN_MCAN_DBTP_DSJW_MSK	 (0xFUL << CAN_MCAN_DBTP_DSJW_POS)
#define CAN_MCAN_DBTP_DSJW	 CAN_MCAN_DBTP_DSJW_MSK
/* Data time segment after sample point */
#define CAN_MCAN_DBTP_DTSEG2_POS (4U)
#define CAN_MCAN_DBTP_DTSEG2_MSK (0xFUL << CAN_MCAN_DBTP_DTSEG2_POS)
#define CAN_MCAN_DBTP_DTSEG2	 CAN_MCAN_DBTP_DTSEG2_MSK
/* Data time segment before sample point */
#define CAN_MCAN_DBTP_DTSEG1_POS (8U)
#define CAN_MCAN_DBTP_DTSEG1_MSK (0x1FUL << CAN_MCAN_DBTP_DTSEG1_POS)
#define CAN_MCAN_DBTP_DTSEG1	 CAN_MCAN_DBTP_DTSEG1_MSK
/* Data BIt Rate Prescaler */
#define CAN_MCAN_DBTP_DBRP_POS	 (16U)
#define CAN_MCAN_DBTP_DBRP_MSK	 (0x1FUL << CAN_MCAN_DBTP_DBRP_POS)
#define CAN_MCAN_DBTP_DBRP	 CAN_MCAN_DBTP_DBRP_MSK
/* Transceiver Delay Compensation */
#define CAN_MCAN_DBTP_TDC_POS	 (23U)
#define CAN_MCAN_DBTP_TDC_MSK	 (0x1UL << CAN_MCAN_DBTP_TDC_POS)
#define CAN_MCAN_DBTP_TDC	 CAN_MCAN_DBTP_TDC_MSK

/***************  Bit definition for CAN_MCAN_TEST register  *****************/
/* Loop Back mode */
#define CAN_MCAN_TEST_LBCK_POS	(4U)
#define CAN_MCAN_TEST_LBCK_MSK	(0x1UL << CAN_MCAN_TEST_LBCK_POS)
#define CAN_MCAN_TEST_LBCK	CAN_MCAN_TEST_LBCK_MSK
/* Control of Transmit Pin */
#define CAN_MCAN_TEST_TX_POS	(5U)
#define CAN_MCAN_TEST_TX_MSK	(0x3UL << CAN_MCAN_TEST_TX_POS)
#define CAN_MCAN_TEST_TX	CAN_MCAN_TEST_TX_MSK
/* Receive Pin */
#define CAN_MCAN_TEST_RX_POS	(7U)
#define CAN_MCAN_TEST_RX_MSK	(0x1UL << CAN_MCAN_TEST_RX_POS)
#define CAN_MCAN_TEST_RX	CAN_MCAN_TEST_RX_MSK
/* Does not exist on STM32 begin */
/* Tx Buffer Number Prepared */
#define CAN_MCAN_TEST_TXBNP_POS (8U)
#define CAN_MCAN_TEST_TXBNP_MSK (0x1FUL << CAN_MCAN_TEST_TXBNP_POS)
#define CAN_MCAN_TEST_TXBNP	CAN_MCAN_TEST_RX_MSK
/* Prepared Valid */
#define CAN_MCAN_TEST_PVAL_POS	(13U)
#define CAN_MCAN_TEST_PVAL_MSK	(0x1UL << CAN_MCAN_TEST_PVAL_POS)
#define CAN_MCAN_TEST_PVAL	CAN_MCAN_TEST_RX_MSK
/* Tx Buffer Number Started */
#define CAN_MCAN_TEST_TXBNS_POS (16U)
#define CAN_MCAN_TEST_TXBNS_MSK (0x1FUL << CAN_MCAN_TEST_TXBNS_POS)
#define CAN_MCAN_TEST_TXBNS	CAN_MCAN_TEST_RX_MSK
/* Started Valid */
#define CAN_MCAN_TEST_SVAL_POS	(12U)
#define CAN_MCAN_TEST_SVAL_MSK	(0x1UL << CAN_MCAN_TEST_SVAL_POS)
#define CAN_MCAN_TEST_SVAL	CAN_MCAN_TEST_RX_MSK
/* Does not exist on STM32 end */

/***************  Bit definition for CAN_MCAN_RWD register  ******************/
/* Watchdog configuration */
#define CAN_MCAN_RWD_WDC_POS (0U)
#define CAN_MCAN_RWD_WDC_MSK (0xFFUL << CAN_MCAN_RWD_WDC_POS)
#define CAN_MCAN_RWD_WDC     CAN_MCAN_RWD_WDC_MSK
/* Watchdog value */
#define CAN_MCAN_RWD_WDV_POS (8U)
#define CAN_MCAN_RWD_WDV_MSK (0xFFUL << CAN_MCAN_RWD_WDV_POS)
#define CAN_MCAN_RWD_WDV     CAN_MCAN_RWD_WDV_MSK

/***************  Bit definition for CAN_MCAN_CCCR register  ******************/
/* Initialization */
#define CAN_MCAN_CCCR_INIT_POS (0U)
#define CAN_MCAN_CCCR_INIT_MSK (0x1UL << CAN_MCAN_CCCR_INIT_POS)
#define CAN_MCAN_CCCR_INIT     CAN_MCAN_CCCR_INIT_MSK
/* Configuration Change Enable */
#define CAN_MCAN_CCCR_CCE_POS  (1U)
#define CAN_MCAN_CCCR_CCE_MSK  (0x1UL << CAN_MCAN_CCCR_CCE_POS)
#define CAN_MCAN_CCCR_CCE      CAN_MCAN_CCCR_CCE_MSK
/* ASM Restricted Operation Mode */
#define CAN_MCAN_CCCR_ASM_POS  (2U)
#define CAN_MCAN_CCCR_ASM_MSK  (0x1UL << CAN_MCAN_CCCR_ASM_POS)
#define CAN_MCAN_CCCR_ASM      CAN_MCAN_CCCR_ASM_MSK
/* Clock Stop Acknowledge */
#define CAN_MCAN_CCCR_CSA_POS  (3U)
#define CAN_MCAN_CCCR_CSA_MSK  (0x1UL << CAN_MCAN_CCCR_CSA_POS)
#define CAN_MCAN_CCCR_CSA      CAN_MCAN_CCCR_CSA_MSK
/* Clock Stop Request */
#define CAN_MCAN_CCCR_CSR_POS  (4U)
#define CAN_MCAN_CCCR_CSR_MSK  (0x1UL << CAN_MCAN_CCCR_CSR_POS)
#define CAN_MCAN_CCCR_CSR      CAN_MCAN_CCCR_CSR_MSK
/* Bus Monitoring Mode */
#define CAN_MCAN_CCCR_MON_POS  (5U)
#define CAN_MCAN_CCCR_MON_MSK  (0x1UL << CAN_MCAN_CCCR_MON_POS)
#define CAN_MCAN_CCCR_MON      CAN_MCAN_CCCR_MON_MSK
/* Disable Automatic Retransmission */
#define CAN_MCAN_CCCR_DAR_POS  (6U)
#define CAN_MCAN_CCCR_DAR_MSK  (0x1UL << CAN_MCAN_CCCR_DAR_POS)
#define CAN_MCAN_CCCR_DAR      CAN_MCAN_CCCR_DAR_MSK
/* Test Mode Enable */
#define CAN_MCAN_CCCR_TEST_POS (7U)
#define CAN_MCAN_CCCR_TEST_MSK (0x1UL << CAN_MCAN_CCCR_TEST_POS)
#define CAN_MCAN_CCCR_TEST     CAN_MCAN_CCCR_TEST_MSK
/* FD Operation Enable */
#define CAN_MCAN_CCCR_FDOE_POS (8U)
#define CAN_MCAN_CCCR_FDOE_MSK (0x1UL << CAN_MCAN_CCCR_FDOE_POS)
#define CAN_MCAN_CCCR_FDOE     CAN_MCAN_CCCR_FDOE_MSK
/* FDCAN Bit Rate Switching */
#define CAN_MCAN_CCCR_BRSE_POS (9U)
#define CAN_MCAN_CCCR_BRSE_MSK (0x1UL << CAN_MCAN_CCCR_BRSE_POS)
#define CAN_MCAN_CCCR_BRSE     CAN_MCAN_CCCR_BRSE_MSK
/* does not exist on stm32 begin*/
/* Use Timestamping Uni */
#define CAN_MCAN_CCCR_UTSU_POS (10U)
#define CAN_MCAN_CCCR_UTSU_MSK (0x1UL << CAN_MCAN_CCCR_UTSU_POS)
#define CAN_MCAN_CCCR_UTSU     CAN_MCAN_CCCR_UTSU_MSK
/* FDCAN Wide Message Marker */
#define CAN_MCAN_CCCR_WMM_POS  (11U)
#define CAN_MCAN_CCCR_WMM_MSK  (0x1UL << CAN_MCAN_CCCR_WMM_POS)
#define CAN_MCAN_CCCR_WMM      CAN_MCAN_CCCR_WMM_MSK
/* end */
/* Protocol Exception Handling Disable */
#define CAN_MCAN_CCCR_PXHD_POS (12U)
#define CAN_MCAN_CCCR_PXHD_MSK (0x1UL << CAN_MCAN_CCCR_PXHD_POS)
#define CAN_MCAN_CCCR_PXHD     CAN_MCAN_CCCR_PXHD_MSK
/* Edge Filtering during Bus Integration */
#define CAN_MCAN_CCCR_EFBI_POS (13U)
#define CAN_MCAN_CCCR_EFBI_MSK (0x1UL << CAN_MCAN_CCCR_EFBI_POS)
#define CAN_MCAN_CCCR_EFBI     CAN_MCAN_CCCR_EFBI_MSK
/* Two CAN bit times Pause */
#define CAN_MCAN_CCCR_TXP_POS  (14U)
#define CAN_MCAN_CCCR_TXP_MSK  (0x1UL << CAN_MCAN_CCCR_TXP_POS)
#define CAN_MCAN_CCCR_TXP      CAN_MCAN_CCCR_TXP_MSK
/* Non ISO Operation */
#define CAN_MCAN_CCCR_NISO_POS (15U)
#define CAN_MCAN_CCCR_NISO_MSK (0x1UL << CAN_MCAN_CCCR_NISO_POS)
#define CAN_MCAN_CCCR_NISO     CAN_MCAN_CCCR_NISO_MSK

/***************  Bit definition for CAN_MCAN_NBTP register  ******************/
/* Nominal Time segment after sample point */
#define CAN_MCAN_NBTP_NTSEG2_POS (0U)
#define CAN_MCAN_NBTP_NTSEG2_MSK (0x7FUL << CAN_MCAN_NBTP_NTSEG2_POS)
#define CAN_MCAN_NBTP_NTSEG2	 CAN_MCAN_NBTP_NTSEG2_MSK
/* Nominal Time segment before sample point */
#define CAN_MCAN_NBTP_NTSEG1_POS (8U)
#define CAN_MCAN_NBTP_NTSEG1_MSK (0xFFUL << CAN_MCAN_NBTP_NTSEG1_POS)
#define CAN_MCAN_NBTP_NTSEG1	 CAN_MCAN_NBTP_NTSEG1_MSK
/* Bit Rate Prescaler */
#define CAN_MCAN_NBTP_NBRP_POS	 (16U)
#define CAN_MCAN_NBTP_NBRP_MSK	 (0x1FFUL << CAN_MCAN_NBTP_NBRP_POS)
#define CAN_MCAN_NBTP_NBRP	 CAN_MCAN_NBTP_NBRP_MSK
/* Nominal (Re)Synchronization Jump Width */
#define CAN_MCAN_NBTP_NSJW_POS	 (25U)
#define CAN_MCAN_NBTP_NSJW_MSK	 (0x7FUL << CAN_MCAN_NBTP_NSJW_POS)
#define CAN_MCAN_NBTP_NSJW	 CAN_MCAN_NBTP_NSJW_MSK

/***************  Bit definition for CAN_MCAN_TSCC register  ******************/
/* Timestamp Select */
#define CAN_MCAN_TSCC_TSS_POS (0U)
#define CAN_MCAN_TSCC_TSS_MSK (0x3UL << CAN_MCAN_TSCC_TSS_POS)
#define CAN_MCAN_TSCC_TSS     CAN_MCAN_TSCC_TSS_MSK
/* Timestamp Counter Prescaler */
#define CAN_MCAN_TSCC_TCP_POS (16U)
#define CAN_MCAN_TSCC_TCP_MSK (0xFUL << CAN_MCAN_TSCC_TCP_POS)
#define CAN_MCAN_TSCC_TCP     CAN_MCAN_TSCC_TCP_MSK
/***************  Bit definition for CAN_MCAN_TSCV register  ******************/
/* Timestamp Counter */
#define CAN_MCAN_TSCV_TSC_POS (0U)
#define CAN_MCAN_TSCV_TSC_MSK (0xFFFFUL << CAN_MCAN_TSCV_TSC_POS)
#define CAN_MCAN_TSCV_TSC     CAN_MCAN_TSCV_TSC_MSK

/***************  Bit definition for CAN_MCAN_TOCC register  ******************/
/* Enable Timeout Counter */
#define CAN_MCAN_TOCC_ETOC_POS (0U)
#define CAN_MCAN_TOCC_ETOC_MSK (0x1UL << CAN_MCAN_TOCC_ETOC_POS)
#define CAN_MCAN_TOCC_ETOC     CAN_MCAN_TOCC_ETOC_MSK
/* Timeout Select */
#define CAN_MCAN_TOCC_TOS_POS  (1U)
#define CAN_MCAN_TOCC_TOS_MSK  (0x3UL << CAN_MCAN_TOCC_TOS_POS)
#define CAN_MCAN_TOCC_TOS      CAN_MCAN_TOCC_TOS_MSK
/* Timeout Period */
#define CAN_MCAN_TOCC_TOP_POS  (16U)
#define CAN_MCAN_TOCC_TOP_MSK  (0xFFFFUL << CAN_MCAN_TOCC_TOP_POS)
#define CAN_MCAN_TOCC_TOP      CAN_MCAN_TOCC_TOP_MSK

/***************  Bit definition for CAN_MCAN_TOCV register  ******************/
/* Timeout Counter */
#define CAN_MCAN_TOCV_TOC_POS (0U)
#define CAN_MCAN_TOCV_TOC_MSK (0xFFFFUL << CAN_MCAN_TOCV_TOC_POS)
#define CAN_MCAN_TOCV_TOC     CAN_MCAN_TOCV_TOC_MSK

/***************  Bit definition for CAN_MCAN_ECR register  *******************/
/* Transmit Error Counter */
#define CAN_MCAN_ECR_TEC_POS (0U)
#define CAN_MCAN_ECR_TEC_MSK (0xFFUL << CAN_MCAN_ECR_TEC_POS)
#define CAN_MCAN_ECR_TEC     CAN_MCAN_ECR_TEC_MSK
/* Receive Error Counter */
#define CAN_MCAN_ECR_REC_POS (8U)
#define CAN_MCAN_ECR_REC_MSK (0x7FUL << CAN_MCAN_ECR_REC_POS)
#define CAN_MCAN_ECR_REC     CAN_MCAN_ECR_REC_MSK
/* Receive Error Passive */
#define CAN_MCAN_ECR_RP_POS  (15U)
#define CAN_MCAN_ECR_RP_MSK  (0x1UL << CAN_MCAN_ECR_RP_POS)
#define CAN_MCAN_ECR_RP	     CAN_MCAN_ECR_RP_MSK
/* CAN Error Logging */
#define CAN_MCAN_ECR_CEL_POS (16U)
#define CAN_MCAN_ECR_CEL_MSK (0xFFUL << CAN_MCAN_ECR_CEL_POS)
#define CAN_MCAN_ECR_CEL     CAN_MCAN_ECR_CEL_MSK

/***************  Bit definition for CAN_MCAN_PSR register  *******************/
/* Last Error Code */
#define CAN_MCAN_PSR_LEC_POS  (0U)
#define CAN_MCAN_PSR_LEC_MSK  (0x7UL << CAN_MCAN_PSR_LEC_POS)
#define CAN_MCAN_PSR_LEC      CAN_MCAN_PSR_LEC_MSK
/* Activity */
#define CAN_MCAN_PSR_ACT_POS  (3U)
#define CAN_MCAN_PSR_ACT_MSK  (0x3UL << CAN_MCAN_PSR_ACT_POS)
#define CAN_MCAN_PSR_ACT      CAN_MCAN_PSR_ACT_MSK
/* Error Passive */
#define CAN_MCAN_PSR_EP_POS   (5U)
#define CAN_MCAN_PSR_EP_MSK   (0x1UL << CAN_MCAN_PSR_EP_POS)
#define CAN_MCAN_PSR_EP	      CAN_MCAN_PSR_EP_MSK
/* Warning Status */
#define CAN_MCAN_PSR_EW_POS   (6U)
#define CAN_MCAN_PSR_EW_MSK   (0x1UL << CAN_MCAN_PSR_EW_POS)
#define CAN_MCAN_PSR_EW	      CAN_MCAN_PSR_EW_MSK
/* Bus_Off Status */
#define CAN_MCAN_PSR_BO_POS   (7U)
#define CAN_MCAN_PSR_BO_MSK   (0x1UL << CAN_MCAN_PSR_BO_POS)
#define CAN_MCAN_PSR_BO	      CAN_MCAN_PSR_BO_MSK
/* Data Last Error Code */
#define CAN_MCAN_PSR_DLEC_POS (8U)
#define CAN_MCAN_PSR_DLEC_MSK (0x7UL << CAN_MCAN_PSR_DLEC_POS)
#define CAN_MCAN_PSR_DLEC     CAN_MCAN_PSR_DLEC_MSK
/* ESI flag of last received FDCAN Message */
#define CAN_MCAN_PSR_RESI_POS (11U)
#define CAN_MCAN_PSR_RESI_MSK (0x1UL << CAN_MCAN_PSR_RESI_POS)
#define CAN_MCAN_PSR_RESI     CAN_MCAN_PSR_RESI_MSK
/* BRS flag of last received FDCAN Message */
#define CAN_MCAN_PSR_RBRS_POS (12U)
#define CAN_MCAN_PSR_RBRS_MSK (0x1UL << CAN_MCAN_PSR_RBRS_POS)
#define CAN_MCAN_PSR_RBRS     CAN_MCAN_PSR_RBRS_MSK
/* Received FDCAN Message */
#define CAN_MCAN_PSR_REDL_POS (13U)
#define CAN_MCAN_PSR_REDL_MSK (0x1UL << CAN_MCAN_PSR_REDL_POS)
#define CAN_MCAN_PSR_REDL     CAN_MCAN_PSR_REDL_MSK
/* Protocol Exception Event */
#define CAN_MCAN_PSR_PXE_POS  (14U)
#define CAN_MCAN_PSR_PXE_MSK  (0x1UL << CAN_MCAN_PSR_PXE_POS)
#define CAN_MCAN_PSR_PXE      CAN_MCAN_PSR_PXE_MSK
/* Transmitter Delay Compensation Value */
#define CAN_MCAN_PSR_TDCV_POS (16U)
#define CAN_MCAN_PSR_TDCV_MSK (0x7FUL << CAN_MCAN_PSR_TDCV_POS)
#define CAN_MCAN_PSR_TDCV     CAN_MCAN_PSR_TDCV_MSK

/***************  Bit definition for CAN_MCAN_TDCR register  ******************/
/* Transmitter Delay Compensation Filter */
#define CAN_MCAN_TDCR_TDCF_POS (0U)
#define CAN_MCAN_TDCR_TDCF_MSK (0x7FUL << CAN_MCAN_TDCR_TDCF_POS)
#define CAN_MCAN_TDCR_TDCF     CAN_MCAN_TDCR_TDCF_MSK
/* Transmitter Delay Compensation Offset */
#define CAN_MCAN_TDCR_TDCO_POS (8U)
#define CAN_MCAN_TDCR_TDCO_MSK (0x7FUL << CAN_MCAN_TDCR_TDCO_POS)
#define CAN_MCAN_TDCR_TDCO     CAN_MCAN_TDCR_TDCO_MSK

/***************  Bit definition for CAN_MCAN_IR register  ********************/
#ifdef CONFIG_CAN_STM32FD
/* Rx FIFO 0 New Message */
#define CAN_MCAN_IR_RF0N_POS (0U)
#define CAN_MCAN_IR_RF0N_MSK (0x1UL << CAN_MCAN_IR_RF0N_POS)
#define CAN_MCAN_IR_RF0N     CAN_MCAN_IR_RF0N_MSK
/* Rx FIFO 0 Full */
#define CAN_MCAN_IR_RF0F_POS (1U)
#define CAN_MCAN_IR_RF0F_MSK (0x1UL << CAN_MCAN_IR_RF0F_POS)
#define CAN_MCAN_IR_RF0F     CAN_MCAN_IR_RF0F_MSK
/* Rx FIFO 0 Message Lost */
#define CAN_MCAN_IR_RF0L_POS (2U)
#define CAN_MCAN_IR_RF0L_MSK (0x1UL << CAN_MCAN_IR_RF0L_POS)
#define CAN_MCAN_IR_RF0L     CAN_MCAN_IR_RF0L_MSK
/* Rx FIFO 1 New Message */
#define CAN_MCAN_IR_RF1N_POS (3U)
#define CAN_MCAN_IR_RF1N_MSK (0x1UL << CAN_MCAN_IR_RF1N_POS)
#define CAN_MCAN_IR_RF1N     CAN_MCAN_IR_RF1N_MSK
/* Rx FIFO 1 Full */
#define CAN_MCAN_IR_RF1F_POS (4U)
#define CAN_MCAN_IR_RF1F_MSK (0x1UL << CAN_MCAN_IR_RF1F_POS)
#define CAN_MCAN_IR_RF1F     CAN_MCAN_IR_RF1F_MSK
/* Rx FIFO 1 Message Lost */
#define CAN_MCAN_IR_RF1L_POS (5U)
#define CAN_MCAN_IR_RF1L_MSK (0x1UL << CAN_MCAN_IR_RF1L_POS)
#define CAN_MCAN_IR_RF1L     CAN_MCAN_IR_RF1L_MSK
/* High Priority Message */
#define CAN_MCAN_IR_HPM_POS  (6U)
#define CAN_MCAN_IR_HPM_MSK  (0x1UL << CAN_MCAN_IR_HPM_POS)
#define CAN_MCAN_IR_HPM	     CAN_MCAN_IR_HPM_MSK
/* Transmission Completed     */
#define CAN_MCAN_IR_TC_POS   (7U)
#define CAN_MCAN_IR_TC_MSK   (0x1UL << CAN_MCAN_IR_TC_POS)
#define CAN_MCAN_IR_TC	     CAN_MCAN_IR_TC_MSK
/* Transmission Cancellation Finished */
#define CAN_MCAN_IR_TCF_POS  (8U)
#define CAN_MCAN_IR_TCF_MSK  (0x1UL << CAN_MCAN_IR_TCF_POS)
#define CAN_MCAN_IR_TCF	     CAN_MCAN_IR_TCF_MSK
/* Tx FIFO Empty */
#define CAN_MCAN_IR_TFE_POS  (9U)
#define CAN_MCAN_IR_TFE_MSK  (0x1UL << CAN_MCAN_IR_TFE_POS)
#define CAN_MCAN_IR_TFE	     CAN_MCAN_IR_TFE_MSK
/* Tx Event FIFO New Entry */
#define CAN_MCAN_IR_TEFN_POS (10U)
#define CAN_MCAN_IR_TEFN_MSK (0x1UL << CAN_MCAN_IR_TEFN_POS)
#define CAN_MCAN_IR_TEFN     CAN_MCAN_IR_TEFN_MSK
/* Tx Event FIFO Full */
#define CAN_MCAN_IR_TEFF_POS (11U)
#define CAN_MCAN_IR_TEFF_MSK (0x1UL << CAN_MCAN_IR_TEFF_POS)
#define CAN_MCAN_IR_TEFF     CAN_MCAN_IR_TEFF_MSK
/* Tx Event FIFO Element Lost */
#define CAN_MCAN_IR_TEFL_POS (12U)
#define CAN_MCAN_IR_TEFL_MSK (0x1UL << CAN_MCAN_IR_TEFL_POS)
#define CAN_MCAN_IR_TEFL     CAN_MCAN_IR_TEFL_MSK
/* Timestamp Wraparound */
#define CAN_MCAN_IR_TSW_POS  (13U)
#define CAN_MCAN_IR_TSW_MSK  (0x1UL << CAN_MCAN_IR_TSW_POS)
#define CAN_MCAN_IR_TSW	     CAN_MCAN_IR_TSW_MSK
/* Message RAM Access Failure */
#define CAN_MCAN_IR_MRAF_POS (14U)
#define CAN_MCAN_IR_MRAF_MSK (0x1UL << CAN_MCAN_IR_MRAF_POS)
#define CAN_MCAN_IR_MRAF     CAN_MCAN_IR_MRAF_MSK
/* Timeout Occurred */
#define CAN_MCAN_IR_TOO_POS  (15U)
#define CAN_MCAN_IR_TOO_MSK  (0x1UL << CAN_MCAN_IR_TOO_POS)
#define CAN_MCAN_IR_TOO	     CAN_MCAN_IR_TOO_MSK
/* Error Logging Overflow */
#define CAN_MCAN_IR_ELO_POS  (16U)
#define CAN_MCAN_IR_ELO_MSK  (0x1UL << CAN_MCAN_IR_ELO_POS)
#define CAN_MCAN_IR_ELO	     CAN_MCAN_IR_ELO_MSK
/* Error Passive */
#define CAN_MCAN_IR_EP_POS   (17U)
#define CAN_MCAN_IR_EP_MSK   (0x1UL << CAN_MCAN_IR_EP_POS)
#define CAN_MCAN_IR_EP	     CAN_MCAN_IR_EP_MSK
/* Warning Status */
#define CAN_MCAN_IR_EW_POS   (18U)
#define CAN_MCAN_IR_EW_MSK   (0x1UL << CAN_MCAN_IR_EW_POS)
#define CAN_MCAN_IR_EW	     CAN_MCAN_IR_EW_MSK
/* Bus_Off Status */
#define CAN_MCAN_IR_BO_POS   (19U)
#define CAN_MCAN_IR_BO_MSK   (0x1UL << CAN_MCAN_IR_BO_POS)
#define CAN_MCAN_IR_BO	     CAN_MCAN_IR_BO_MSK
/* Watchdog Interrupt */
#define CAN_MCAN_IR_WDI_POS  (20U)
#define CAN_MCAN_IR_WDI_MSK  (0x1UL << CAN_MCAN_IR_WDI_POS)
#define CAN_MCAN_IR_WDI	     CAN_MCAN_IR_WDI_MSK
/* Protocol Error in Arbitration Phase */
#define CAN_MCAN_IR_PEA_POS  (21U)
#define CAN_MCAN_IR_PEA_MSK  (0x1UL << CAN_MCAN_IR_PEA_POS)
#define CAN_MCAN_IR_PEA	     CAN_MCAN_IR_PEA_MSK
/* Protocol Error in Data Phase */
#define CAN_MCAN_IR_PED_POS  (22U)
#define CAN_MCAN_IR_PED_MSK  (0x1UL << CAN_MCAN_IR_PED_POS)
#define CAN_MCAN_IR_PED	     CAN_MCAN_IR_PED_MSK
/* Access to Reserved Address */
#define CAN_MCAN_IR_ARA_POS  (23U)
#define CAN_MCAN_IR_ARA_MSK  (0x1UL << CAN_MCAN_IR_ARA_POS)
#define CAN_MCAN_IR_ARA	     CAN_MCAN_IR_ARA_MSK

#else /* CONFIG_CAN_STM32FD */

/* Rx FIFO 0 New Message */
#define CAN_MCAN_IR_RF0N_POS (0U)
#define CAN_MCAN_IR_RF0N_MSK (0x1UL << CAN_MCAN_IR_RF0N_POS)
#define CAN_MCAN_IR_RF0N     CAN_MCAN_IR_RF0N_MSK
/* Rx FIFO 0 Watermark Reached*/
#define CAN_MCAN_IR_RF0W_POS (1U)
#define CAN_MCAN_IR_RF0W_MSK (0x1UL << CAN_MCAN_IR_RF0W_POS)
#define CAN_MCAN_IR_RF0W     CAN_MCAN_IR_RF0W_MSK
/* Rx FIFO 0 Full */
#define CAN_MCAN_IR_RF0F_POS (2U)
#define CAN_MCAN_IR_RF0F_MSK (0x1UL << CAN_MCAN_IR_RF0F_POS)
#define CAN_MCAN_IR_RF0F     CAN_MCAN_IR_RF0F_MSK
/* Rx FIFO 0 Message Lost */
#define CAN_MCAN_IR_RF0L_POS (3U)
#define CAN_MCAN_IR_RF0L_MSK (0x1UL << CAN_MCAN_IR_RF0L_POS)
#define CAN_MCAN_IR_RF0L     CAN_MCAN_IR_RF0L_MSK
/* Rx FIFO 1 New Message */
#define CAN_MCAN_IR_RF1N_POS (4U)
#define CAN_MCAN_IR_RF1N_MSK (0x1UL << CAN_MCAN_IR_RF1N_POS)
#define CAN_MCAN_IR_RF1N     CAN_MCAN_IR_RF1N_MSK
/* Rx FIFO 1 Watermark Reached*/
#define CAN_MCAN_IR_RF1W_POS (5U)
#define CAN_MCAN_IR_RF1W_MSK (0x1UL << CAN_MCAN_IR_RF1W_POS)
#define CAN_MCAN_IR_RF1W     CAN_MCAN_IR_RF1W_MSK
/* Rx FIFO 1 Full */
#define CAN_MCAN_IR_RF1F_POS (6U)
#define CAN_MCAN_IR_RF1F_MSK (0x1UL << CAN_MCAN_IR_RF1F_POS)
#define CAN_MCAN_IR_RF1F     CAN_MCAN_IR_RF1F_MSK
/* Rx FIFO 1 Message Lost */
#define CAN_MCAN_IR_RF1L_POS (7U)
#define CAN_MCAN_IR_RF1L_MSK (0x1UL << CAN_MCAN_IR_RF1L_POS)
#define CAN_MCAN_IR_RF1L     CAN_MCAN_IR_RF1L_MSK
/* High Priority Message */
#define CAN_MCAN_IR_HPM_POS  (8U)
#define CAN_MCAN_IR_HPM_MSK  (0x1UL << CAN_MCAN_IR_HPM_POS)
#define CAN_MCAN_IR_HPM	     CAN_MCAN_IR_HPM_MSK
/* Transmission Completed */
#define CAN_MCAN_IR_TC_POS   (9U)
#define CAN_MCAN_IR_TC_MSK   (0x1UL << CAN_MCAN_IR_TC_POS)
#define CAN_MCAN_IR_TC	     CAN_MCAN_IR_TC_MSK
/* Transmission Cancellation Finished */
#define CAN_MCAN_IR_TCF_POS  (10U)
#define CAN_MCAN_IR_TCF_MSK  (0x1UL << CAN_MCAN_IR_TCF_POS)
#define CAN_MCAN_IR_TCF	     CAN_MCAN_IR_TCF_MSK
/* Tx FIFO Empty */
#define CAN_MCAN_IR_TFE_POS  (11U)
#define CAN_MCAN_IR_TFE_MSK  (0x1UL << CAN_MCAN_IR_TFE_POS)
#define CAN_MCAN_IR_TFE	     CAN_MCAN_IR_TFE_MSK
/* Tx Event FIFO New Entry */
#define CAN_MCAN_IR_TEFN_POS (12U)
#define CAN_MCAN_IR_TEFN_MSK (0x1UL << CAN_MCAN_IR_TEFN_POS)
#define CAN_MCAN_IR_TEFN     CAN_MCAN_IR_TEFN_MSK
/* Tx Event FIFO Watermark */
#define CAN_MCAN_IR_TEFW_POS (13U)
#define CAN_MCAN_IR_TEFW_MSK (0x1UL << CAN_MCAN_IR_TEFW_POS)
#define CAN_MCAN_IR_TEFW     CAN_MCAN_IR_TEFW_MSK
/* Tx Event FIFO Full */
#define CAN_MCAN_IR_TEFF_POS (14U)
#define CAN_MCAN_IR_TEFF_MSK (0x1UL << CAN_MCAN_IR_TEFF_POS)
#define CAN_MCAN_IR_TEFF     CAN_MCAN_IR_TEFF_MSK
/* Tx Event FIFO Element Lost */
#define CAN_MCAN_IR_TEFL_POS (15U)
#define CAN_MCAN_IR_TEFL_MSK (0x1UL << CAN_MCAN_IR_TEFL_POS)
#define CAN_MCAN_IR_TEFL     CAN_MCAN_IR_TEFL_MSK
/* Timestamp Wraparound */
#define CAN_MCAN_IR_TSW_POS  (16U)
#define CAN_MCAN_IR_TSW_MSK  (0x1UL << CAN_MCAN_IR_TSW_POS)
#define CAN_MCAN_IR_TSW	     CAN_MCAN_IR_TSW_MSK
/* Message RAM Access Failure */
#define CAN_MCAN_IR_MRAF_POS (17U)
#define CAN_MCAN_IR_MRAF_MSK (0x1UL << CAN_MCAN_IR_MRAF_POS)
#define CAN_MCAN_IR_MRAF     CAN_MCAN_IR_MRAF_MSK
/* Timeout Occurred */
#define CAN_MCAN_IR_TOO_POS  (18U)
#define CAN_MCAN_IR_TOO_MSK  (0x1UL << CAN_MCAN_IR_TOO_POS)
#define CAN_MCAN_IR_TOO	     CAN_MCAN_IR_TOO_MSK
/* Message stored to Dedicated Rx Buffer */
#define CAN_MCAN_IR_DRX_POS  (19U)
#define CAN_MCAN_IR_DRX_MSK  (0x1UL << CAN_MCAN_IR_DRX_POS)
#define CAN_MCAN_IR_DRX	     CAN_MCAN_IR_DRX_MSK
/* Bit Error Corrected */
#define CAN_MCAN_IR_BEC_POS  (20U)
#define CAN_MCAN_IR_BEC_MSK  (0x1UL << CAN_MCAN_IR_BEC_POS)
#define CAN_MCAN_IR_BEC	     CAN_MCAN_IR_BEC_MSK
/* Bit Error Uncorrected */
#define CAN_MCAN_IR_BEU_POS  (21U)
#define CAN_MCAN_IR_BEU_MSK  (0x1UL << CAN_MCAN_IR_BEU_POS)
#define CAN_MCAN_IR_BEU	     CAN_MCAN_IR_BEU_MSK
/* Error Logging Overflow */
#define CAN_MCAN_IR_ELO_POS  (22U)
#define CAN_MCAN_IR_ELO_MSK  (0x1UL << CAN_MCAN_IR_ELO_POS)
#define CAN_MCAN_IR_ELO	     CAN_MCAN_IR_ELO_MSK
/* Error Passive*/
#define CAN_MCAN_IR_EP_POS   (23U)
#define CAN_MCAN_IR_EP_MSK   (0x1UL << CAN_MCAN_IR_EP_POS)
#define CAN_MCAN_IR_EP	     CAN_MCAN_IR_EP_MSK
/* Warning Status*/
#define CAN_MCAN_IR_EW_POS   (24U)
#define CAN_MCAN_IR_EW_MSK   (0x1UL << CAN_MCAN_IR_EW_POS)
#define CAN_MCAN_IR_EW	     CAN_MCAN_IR_EW_MSK
/* Bus_Off Status*/
#define CAN_MCAN_IR_BO_POS   (25U)
#define CAN_MCAN_IR_BO_MSK   (0x1UL << CAN_MCAN_IR_BO_POS)
#define CAN_MCAN_IR_BO	     CAN_MCAN_IR_BO_MSK
/* Watchdog Interrupt */
#define CAN_MCAN_IR_WDI_POS  (26U)
#define CAN_MCAN_IR_WDI_MSK  (0x1UL << CAN_MCAN_IR_WDI_POS)
#define CAN_MCAN_IR_WDI	     CAN_MCAN_IR_WDI_MSK
/* Protocol Error in Arbitration Phase */
#define CAN_MCAN_IR_PEA_POS  (27U)
#define CAN_MCAN_IR_PEA_MSK  (0x1UL << CAN_MCAN_IR_PEA_POS)
#define CAN_MCAN_IR_PEA	     CAN_MCAN_IR_PEA_MSK
/* Protocol Error in Data Phase */
#define CAN_MCAN_IR_PED_POS  (28U)
#define CAN_MCAN_IR_PED_MSK  (0x1UL << CAN_MCAN_IR_PED_POS)
#define CAN_MCAN_IR_PED	     CAN_MCAN_IR_PED_MSK
/* Access to Reserved Address */
#define CAN_MCAN_IR_ARA_POS  (29U)
#define CAN_MCAN_IR_ARA_MSK  (0x1UL << CAN_MCAN_IR_ARA_POS)
#define CAN_MCAN_IR_ARA	     CAN_MCAN_IR_ARA_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_IE register  ********************/
#ifdef CONFIG_CAN_STM32FD
/* Rx FIFO 0 New Message Enable */
#define CAN_MCAN_IE_RF0N_POS (0U)
#define CAN_MCAN_IE_RF0N_MSK (0x1UL << CAN_MCAN_IE_RF0N_POS)
#define CAN_MCAN_IE_RF0N     CAN_MCAN_IE_RF0N_MSK
/* Rx FIFO 0 Full Enable */
#define CAN_MCAN_IE_RF0F_POS (1U)
#define CAN_MCAN_IE_RF0F_MSK (0x1UL << CAN_MCAN_IE_RF0F_POS)
#define CAN_MCAN_IE_RF0F     CAN_MCAN_IE_RF0F_MSK
/* Rx FIFO 0 Message Lost Enable */
#define CAN_MCAN_IE_RF0L_POS (2U)
#define CAN_MCAN_IE_RF0L_MSK (0x1UL << CAN_MCAN_IE_RF0L_POS)
#define CAN_MCAN_IE_RF0L     CAN_MCAN_IE_RF0L_MSK
/* Rx FIFO 1 New Message Enable */
#define CAN_MCAN_IE_RF1N_POS (3U)
#define CAN_MCAN_IE_RF1N_MSK (0x1UL << CAN_MCAN_IE_RF1N_POS)
#define CAN_MCAN_IE_RF1N     CAN_MCAN_IE_RF1N_MSK
/* Rx FIFO 1 Full Enable */
#define CAN_MCAN_IE_RF1F_POS (4U)
#define CAN_MCAN_IE_RF1F_MSK (0x1UL << CAN_MCAN_IE_RF1F_POS)
#define CAN_MCAN_IE_RF1F     CAN_MCAN_IE_RF1F_MSK
/* Rx FIFO 1 Message Lost Enable */
#define CAN_MCAN_IE_RF1L_POS (5U)
#define CAN_MCAN_IE_RF1L_MSK (0x1UL << CAN_MCAN_IE_RF1L_POS)
#define CAN_MCAN_IE_RF1L     CAN_MCAN_IE_RF1L_MSK
/* High Priority Message Enable */
#define CAN_MCAN_IE_HPM_POS  (6U)
#define CAN_MCAN_IE_HPM_MSK  (0x1UL << CAN_MCAN_IE_HPM_POS)
#define CAN_MCAN_IE_HPM	     CAN_MCAN_IE_HPM_MSK
/* Transmission Completed Enable */
#define CAN_MCAN_IE_TC_POS   (7U)
#define CAN_MCAN_IE_TC_MSK   (0x1UL << CAN_MCAN_IE_TC_POS)
#define CAN_MCAN_IE_TC	     CAN_MCAN_IE_TC_MSK
/* Transmission Cancellation Finished Enable*/
#define CAN_MCAN_IE_TCF_POS  (8U)
#define CAN_MCAN_IE_TCF_MSK  (0x1UL << CAN_MCAN_IE_TCF_POS)
#define CAN_MCAN_IE_TCF	     CAN_MCAN_IE_TCF_MSK
/* Tx FIFO Empty Enable */
#define CAN_MCAN_IE_TFE_POS  (9U)
#define CAN_MCAN_IE_TFE_MSK  (0x1UL << CAN_MCAN_IE_TFE_POS)
#define CAN_MCAN_IE_TFE	     CAN_MCAN_IE_TFE_MSK
/* Tx Event FIFO New Entry Enable */
#define CAN_MCAN_IE_TEFN_POS (10U)
#define CAN_MCAN_IE_TEFN_MSK (0x1UL << CAN_MCAN_IE_TEFN_POS)
#define CAN_MCAN_IE_TEFN     CAN_MCAN_IE_TEFN_MSK
/* Tx Event FIFO Full Enable */
#define CAN_MCAN_IE_TEFF_POS (11U)
#define CAN_MCAN_IE_TEFF_MSK (0x1UL << CAN_MCAN_IE_TEFF_POS)
#define CAN_MCAN_IE_TEFF     CAN_MCAN_IE_TEFF_MSK
/* Tx Event FIFO Element Lost Enable */
#define CAN_MCAN_IE_TEFL_POS (12U)
#define CAN_MCAN_IE_TEFL_MSK (0x1UL << CAN_MCAN_IE_TEFL_POS)
#define CAN_MCAN_IE_TEFL     CAN_MCAN_IE_TEFL_MSK
/* Timestamp Wraparound Enable */
#define CAN_MCAN_IE_TSW_POS  (13U)
#define CAN_MCAN_IE_TSW_MSK  (0x1UL << CAN_MCAN_IE_TSW_POS)
#define CAN_MCAN_IE_TSW	     CAN_MCAN_IE_TSW_MSK
/* Message RAM Access Failure Enable */
#define CAN_MCAN_IE_MRAF_POS (14U)
#define CAN_MCAN_IE_MRAF_MSK (0x1UL << CAN_MCAN_IE_MRAF_POS)
#define CAN_MCAN_IE_MRAF     CAN_MCAN_IE_MRAF_MSK
/* Timeout Occurred Enable */
#define CAN_MCAN_IE_TOO_POS  (15U)
#define CAN_MCAN_IE_TOO_MSK  (0x1UL << CAN_MCAN_IE_TOO_POS)
#define CAN_MCAN_IE_TOO	     CAN_MCAN_IE_TOO_MSK
/* Error Logging Overflow Enable */
#define CAN_MCAN_IE_ELO_POS  (16U)
#define CAN_MCAN_IE_ELO_MSK  (0x1UL << CAN_MCAN_IE_ELO_POS)
#define CAN_MCAN_IE_ELO	     CAN_MCAN_IE_ELO_MSK
/* Error Passive Enable */
#define CAN_MCAN_IE_EP_POS   (17U)
#define CAN_MCAN_IE_EP_MSK   (0x1UL << CAN_MCAN_IE_EP_POS)
#define CAN_MCAN_IE_EP	     CAN_MCAN_IE_EP_MSK
/* Warning Status Enable */
#define CAN_MCAN_IE_EW_POS   (18U)
#define CAN_MCAN_IE_EW_MSK   (0x1UL << CAN_MCAN_IE_EW_POS)
#define CAN_MCAN_IE_EW	     CAN_MCAN_IE_EW_MSK
/* Bus_Off Status Enable */
#define CAN_MCAN_IE_BO_POS   (19U)
#define CAN_MCAN_IE_BO_MSK   (0x1UL << CAN_MCAN_IE_BO_POS)
#define CAN_MCAN_IE_BO	     CAN_MCAN_IE_BO_MSK
/* Watchdog Interrupt Enable  */
#define CAN_MCAN_IE_WDI_POS  (20U)
#define CAN_MCAN_IE_WDI_MSK  (0x1UL << CAN_MCAN_IE_WDI_POS)
#define CAN_MCAN_IE_WDI	     CAN_MCAN_IE_WDIE_MSK
/* Protocol Error in Arbitration Phase Enable */
#define CAN_MCAN_IE_PEA_POS  (21U)
#define CAN_MCAN_IE_PEA_MSK  (0x1UL << CAN_MCAN_IE_PEA_POS)
#define CAN_MCAN_IE_PEA	     CAN_MCAN_IE_PEA_MSK
/* Protocol Error in Data Phase Enable */
#define CAN_MCAN_IE_PED_POS  (22U)
#define CAN_MCAN_IE_PED_MSK  (0x1UL << CAN_MCAN_IE_PED_POS)
#define CAN_MCAN_IE_PED	     CAN_MCAN_IE_PED_MSK
/* Access to Reserved Address Enable */
#define CAN_MCAN_IE_ARA_POS  (23U)
#define CAN_MCAN_IE_ARA_MSK  (0x1UL << CAN_MCAN_IE_ARA_POS)
#define CAN_MCAN_IE_ARA	     CAN_MCAN_IE_ARA_MSK

#else /* CONFIG_CAN_STM32FD */

/* Rx FIFO 0 New Message */
#define CAN_MCAN_IE_RF0N_POS (0U)
#define CAN_MCAN_IE_RF0N_MSK (0x1UL << CAN_MCAN_IE_RF0N_POS)
#define CAN_MCAN_IE_RF0N     CAN_MCAN_IE_RF0N_MSK
/* Rx FIFO 0 Watermark Reached*/
#define CAN_MCAN_IE_RF0W_POS (1U)
#define CAN_MCAN_IE_RF0W_MSK (0x1UL << CAN_MCAN_IE_RF0W_POS)
#define CAN_MCAN_IE_RF0W     CAN_MCAN_IE_RF0W_MSK
/* Rx FIFO 0 Full */
#define CAN_MCAN_IE_RF0F_POS (2U)
#define CAN_MCAN_IE_RF0F_MSK (0x1UL << CAN_MCAN_IE_RF0F_POS)
#define CAN_MCAN_IE_RF0F     CAN_MCAN_IE_RF0F_MSK
/* Rx FIFO 0 Message Lost */
#define CAN_MCAN_IE_RF0L_POS (3U)
#define CAN_MCAN_IE_RF0L_MSK (0x1UL << CAN_MCAN_IE_RF0L_POS)
#define CAN_MCAN_IE_RF0L     CAN_MCAN_IE_RF0L_MSK
/* Rx FIFO 1 New Message */
#define CAN_MCAN_IE_RF1N_POS (4U)
#define CAN_MCAN_IE_RF1N_MSK (0x1UL << CAN_MCAN_IE_RF1N_POS)
#define CAN_MCAN_IE_RF1N     CAN_MCAN_IE_RF1N_MSK
/* Rx FIFO 1 Watermark Reached*/
#define CAN_MCAN_IE_RF1W_POS (5U)
#define CAN_MCAN_IE_RF1W_MSK (0x1UL << CAN_MCAN_IE_RF1W_POS)
#define CAN_MCAN_IE_RF1W     CAN_MCAN_IE_RF1W_MSK
/* Rx FIFO 1 Full */
#define CAN_MCAN_IE_RF1F_POS (6U)
#define CAN_MCAN_IE_RF1F_MSK (0x1UL << CAN_MCAN_IE_RF1F_POS)
#define CAN_MCAN_IE_RF1F     CAN_MCAN_IE_RF1F_MSK
/* Rx FIFO 1 Message Lost */
#define CAN_MCAN_IE_RF1L_POS (7U)
#define CAN_MCAN_IE_RF1L_MSK (0x1UL << CAN_MCAN_IE_RF1L_POS)
#define CAN_MCAN_IE_RF1L     CAN_MCAN_IE_RF1L_MSK
/* High Priority Message */
#define CAN_MCAN_IE_HPM_POS  (8U)
#define CAN_MCAN_IE_HPM_MSK  (0x1UL << CAN_MCAN_IE_HPM_POS)
#define CAN_MCAN_IE_HPM	     CAN_MCAN_IE_HPM_MSK
/* Transmission Completed */
#define CAN_MCAN_IE_TC_POS   (9U)
#define CAN_MCAN_IE_TC_MSK   (0x1UL << CAN_MCAN_IE_TC_POS)
#define CAN_MCAN_IE_TC	     CAN_MCAN_IE_TC_MSK
/* Transmission Cancellation Finished */
#define CAN_MCAN_IE_TCF_POS  (10U)
#define CAN_MCAN_IE_TCF_MSK  (0x1UL << CAN_MCAN_IE_TCF_POS)
#define CAN_MCAN_IE_TCF	     CAN_MCAN_IE_TCF_MSK
/* Tx FIFO Empty */
#define CAN_MCAN_IE_TFE_POS  (11U)
#define CAN_MCAN_IE_TFE_MSK  (0x1UL << CAN_MCAN_IE_TFE_POS)
#define CAN_MCAN_IE_TFE	     CAN_MCAN_IE_TFE_MSK
/* Tx Event FIFO New Entry */
#define CAN_MCAN_IE_TEFN_POS (12U)
#define CAN_MCAN_IE_TEFN_MSK (0x1UL << CAN_MCAN_IE_TEFN_POS)
#define CAN_MCAN_IE_TEFN     CAN_MCAN_IE_TEFN_MSK
/* Tx Event FIFO Watermark */
#define CAN_MCAN_IE_TEFW_POS (13U)
#define CAN_MCAN_IE_TEFW_MSK (0x1UL << CAN_MCAN_IE_TEFW_POS)
#define CAN_MCAN_IE_TEFW     CAN_MCAN_IE_TEFW_MSK
/* Tx Event FIFO Full */
#define CAN_MCAN_IE_TEFF_POS (14U)
#define CAN_MCAN_IE_TEFF_MSK (0x1UL << CAN_MCAN_IE_TEFF_POS)
#define CAN_MCAN_IE_TEFF     CAN_MCAN_IE_TEFF_MSK
/* Tx Event FIFO Element Lost */
#define CAN_MCAN_IE_TEFL_POS (15U)
#define CAN_MCAN_IE_TEFL_MSK (0x1UL << CAN_MCAN_IE_TEFL_POS)
#define CAN_MCAN_IE_TEFL     CAN_MCAN_IE_TEFL_MSK
/* Timestamp Wraparound */
#define CAN_MCAN_IE_TSW_POS  (16U)
#define CAN_MCAN_IE_TSW_MSK  (0x1UL << CAN_MCAN_IE_TSW_POS)
#define CAN_MCAN_IE_TSW	     CAN_MCAN_IE_TSW_MSK
/* Message RAM Access Failure */
#define CAN_MCAN_IE_MRAF_POS (17U)
#define CAN_MCAN_IE_MRAF_MSK (0x1UL << CAN_MCAN_IE_MRAF_POS)
#define CAN_MCAN_IE_MRAF     CAN_MCAN_IE_MRAF_MSK
/* Timeout Occurred */
#define CAN_MCAN_IE_TOO_POS  (18U)
#define CAN_MCAN_IE_TOO_MSK  (0x1UL << CAN_MCAN_IE_TOO_POS)
#define CAN_MCAN_IE_TOO	     CAN_MCAN_IE_TOO_MSK
/* Message stored to Dedicated Rx Buffer */
#define CAN_MCAN_IE_DRX_POS  (19U)
#define CAN_MCAN_IE_DRX_MSK  (0x1UL << CAN_MCAN_IE_DRX_POS)
#define CAN_MCAN_IE_DRX	     CAN_MCAN_IE_DRX_MSK
/* Bit Error Corrected */
#define CAN_MCAN_IE_BEC_POS  (20U)
#define CAN_MCAN_IE_BEC_MSK  (0x1UL << CAN_MCAN_IE_BEC_POS)
#define CAN_MCAN_IE_BEC	     CAN_MCAN_IE_BEC_MSK
/* Bit Error Uncorrected */
#define CAN_MCAN_IE_BEU_POS  (21U)
#define CAN_MCAN_IE_BEU_MSK  (0x1UL << CAN_MCAN_IE_BEU_POS)
#define CAN_MCAN_IE_BEU	     CAN_MCAN_IE_BEU_MSK
/* Error Logging Overflow */
#define CAN_MCAN_IE_ELO_POS  (22U)
#define CAN_MCAN_IE_ELO_MSK  (0x1UL << CAN_MCAN_IE_ELO_POS)
#define CAN_MCAN_IE_ELO	     CAN_MCAN_IE_ELO_MSK
/* Error Passive*/
#define CAN_MCAN_IE_EP_POS   (23U)
#define CAN_MCAN_IE_EP_MSK   (0x1UL << CAN_MCAN_IE_EP_POS)
#define CAN_MCAN_IE_EP	     CAN_MCAN_IE_EP_MSK
/* Warning Status*/
#define CAN_MCAN_IE_EW_POS   (24U)
#define CAN_MCAN_IE_EW_MSK   (0x1UL << CAN_MCAN_IE_EW_POS)
#define CAN_MCAN_IE_EW	     CAN_MCAN_IE_EW_MSK
/* Bus_Off Status*/
#define CAN_MCAN_IE_BO_POS   (25U)
#define CAN_MCAN_IE_BO_MSK   (0x1UL << CAN_MCAN_IE_BO_POS)
#define CAN_MCAN_IE_BO	     CAN_MCAN_IE_BO_MSK
/* Watchdog Interrupt */
#define CAN_MCAN_IE_WDI_POS  (26U)
#define CAN_MCAN_IE_WDI_MSK  (0x1UL << CAN_MCAN_IE_WDI_POS)
#define CAN_MCAN_IE_WDI	     CAN_MCAN_IE_WDI_MSK
/* Protocol Error in Arbitration Phase */
#define CAN_MCAN_IE_PEA_POS  (27U)
#define CAN_MCAN_IE_PEA_MSK  (0x1UL << CAN_MCAN_IE_PEA_POS)
#define CAN_MCAN_IE_PEA	     CAN_MCAN_IE_PEA_MSK
/* Protocol Error in Data Phase */
#define CAN_MCAN_IE_PED_POS  (28U)
#define CAN_MCAN_IE_PED_MSK  (0x1UL << CAN_MCAN_IE_PED_POS)
#define CAN_MCAN_IE_PED	     CAN_MCAN_IE_PED_MSK
/* Access to Reserved Address */
#define CAN_MCAN_IE_ARA_POS  (29U)
#define CAN_MCAN_IE_ARA_MSK  (0x1UL << CAN_MCAN_IE_ARA_POS)
#define CAN_MCAN_IE_ARA	     CAN_MCAN_IE_ARA_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_ILS register  *******************/
#ifdef CONFIG_CAN_STM32FD
/* Rx FIFO 0 */
#define CAN_MCAN_ILS_RXFIFO0_POS (0U)
#define CAN_MCAN_ILS_RXFIFO0_MSK (0x1UL << CAN_MCAN_ILS_RXFIFO0_POS)
#define CAN_MCAN_ILS_RXFIFO0	 CAN_MCAN_ILS_RXFIFO0_MSK
/* Rx FIFO 1 */
#define CAN_MCAN_ILS_RXFIFO1_POS (1U)
#define CAN_MCAN_ILS_RXFIFO1_MSK (0x1UL << CAN_MCAN_ILS_RXFIFO1_POS)
#define CAN_MCAN_ILS_RXFIFO1	 CAN_MCAN_ILS_RXFIFO1_MSK
/* Transmission Cancellation Finished */
#define CAN_MCAN_ILS_SMSG_POS	 (2U)
#define CAN_MCAN_ILS_SMSG_MSK	 (0x1UL << CAN_MCAN_ILS_SMSG_POS)
#define CAN_MCAN_ILS_SMSG	 CAN_MCAN_ILS_SMSG_MSK
/* Tx Event FIFO Element Lost */
#define CAN_MCAN_ILS_TFERR_POS	 (3U)
#define CAN_MCAN_ILS_TFERR_MSK	 (0x1UL << CAN_MCAN_ILS_TFERR_POS)
#define CAN_MCAN_ILS_TFERR	 CAN_MCAN_ILS_TFERR_MSK
/* Timeout Occurred */
#define CAN_MCAN_ILS_MISC_POS	 (4U)
#define CAN_MCAN_ILS_MISC_MSK	 (0x1UL << CAN_MCAN_ILS_MISC_POS)
#define CAN_MCAN_ILS_MISC	 CAN_MCAN_ILS_MISC_MSK
/* Error Passive Error Logging Overflow */
#define CAN_MCAN_ILS_BERR_POS	 (5U)
#define CAN_MCAN_ILS_BERR_MSK	 (0x1UL << CAN_MCAN_ILS_BERR_POS)
#define CAN_MCAN_ILS_BERR	 CAN_MCAN_ILS_BERR_MSK
/* Access to Reserved Address Line */
#define CAN_MCAN_ILS_PERR_POS	 (6U)
#define CAN_MCAN_ILS_PERR_MSK	 (0x1UL << CAN_MCAN_ILS_PERR_POS)
#define CAN_MCAN_ILS_PERR	 CAN_MCAN_ILS_PERR_MSK

#else /* CONFIG_CAN_STM32FD */
/* Rx FIFO 0 New Message */
#define CAN_MCAN_ILS_RF0N_POS (0U)
#define CAN_MCAN_ILS_RF0N_MSK (0x1UL << CAN_MCAN_ILS_RF0N_POS)
#define CAN_MCAN_ILS_RF0N     CAN_MCAN_ILS_RF0N_MSK
/* Rx FIFO 0 Watermark Reached*/
#define CAN_MCAN_ILS_RF0W_POS (1U)
#define CAN_MCAN_ILS_RF0W_MSK (0x1UL << CAN_MCAN_ILS_RF0W_POS)
#define CAN_MCAN_ILS_RF0W     CAN_MCAN_ILS_RF0W_MSK
/* Rx FIFO 0 Full */
#define CAN_MCAN_ILS_RF0F_POS (2U)
#define CAN_MCAN_ILS_RF0F_MSK (0x1UL << CAN_MCAN_ILS_RF0F_POS)
#define CAN_MCAN_ILS_RF0F     CAN_MCAN_ILS_RF0F_MSK
/* Rx FIFO 0 Message Lost */
#define CAN_MCAN_ILS_RF0L_POS (3U)
#define CAN_MCAN_ILS_RF0L_MSK (0x1UL << CAN_MCAN_ILS_RF0L_POS)
#define CAN_MCAN_ILS_RF0L     CAN_MCAN_ILS_RF0L_MSK
/* Rx FIFO 1 New Message */
#define CAN_MCAN_ILS_RF1N_POS (4U)
#define CAN_MCAN_ILS_RF1N_MSK (0x1UL << CAN_MCAN_ILS_RF1N_POS)
#define CAN_MCAN_ILS_RF1N     CAN_MCAN_ILS_RF1N_MSK
/* Rx FIFO 1 Watermark Reached*/
#define CAN_MCAN_ILS_RF1W_POS (5U)
#define CAN_MCAN_ILS_RF1W_MSK (0x1UL << CAN_MCAN_ILS_RF1W_POS)
#define CAN_MCAN_ILS_RF1W     CAN_MCAN_ILS_RF1W_MSK
/* Rx FIFO 1 Full */
#define CAN_MCAN_ILS_RF1F_POS (6U)
#define CAN_MCAN_ILS_RF1F_MSK (0x1UL << CAN_MCAN_ILS_RF1F_POS)
#define CAN_MCAN_ILS_RF1F     CAN_MCAN_ILS_RF1F_MSK
/* Rx FIFO 1 Message Lost */
#define CAN_MCAN_ILS_RF1L_POS (7U)
#define CAN_MCAN_ILS_RF1L_MSK (0x1UL << CAN_MCAN_ILS_RF1L_POS)
#define CAN_MCAN_ILS_RF1L     CAN_MCAN_ILS_RF1L_MSK
/* High Priority Message */
#define CAN_MCAN_ILS_HPM_POS  (8U)
#define CAN_MCAN_ILS_HPM_MSK  (0x1UL << CAN_MCAN_ILS_HPM_POS)
#define CAN_MCAN_ILS_HPM      CAN_MCAN_ILS_HPM_MSK
/* Transmission Completed */
#define CAN_MCAN_ILS_TC_POS   (9U)
#define CAN_MCAN_ILS_TC_MSK   (0x1UL << CAN_MCAN_ILS_TC_POS)
#define CAN_MCAN_ILS_TC	      CAN_MCAN_ILS_TC_MSK
/* Transmission Cancellation Finished */
#define CAN_MCAN_ILS_TCF_POS  (10U)
#define CAN_MCAN_ILS_TCF_MSK  (0x1UL << CAN_MCAN_ILS_TCF_POS)
#define CAN_MCAN_ILS_TCF      CAN_MCAN_ILS_TCF_MSK
/* Tx FIFO Empty */
#define CAN_MCAN_ILS_TFE_POS  (11U)
#define CAN_MCAN_ILS_TFE_MSK  (0x1UL << CAN_MCAN_ILS_TFE_POS)
#define CAN_MCAN_ILS_TFE      CAN_MCAN_ILS_TFE_MSK
/* Tx Event FIFO New Entry */
#define CAN_MCAN_ILS_TEFN_POS (12U)
#define CAN_MCAN_ILS_TEFN_MSK (0x1UL << CAN_MCAN_ILS_TEFN_POS)
#define CAN_MCAN_ILS_TEFN     CAN_MCAN_ILS_TEFN_MSK
/* Tx Event FIFO Watermark */
#define CAN_MCAN_ILS_TEFW_POS (13U)
#define CAN_MCAN_ILS_TEFW_MSK (0x1UL << CAN_MCAN_ILS_TEFW_POS)
#define CAN_MCAN_ILS_TEFW     CAN_MCAN_ILS_TEFW_MSK
/* Tx Event FIFO Full */
#define CAN_MCAN_ILS_TEFF_POS (14U)
#define CAN_MCAN_ILS_TEFF_MSK (0x1UL << CAN_MCAN_ILS_TEFF_POS)
#define CAN_MCAN_ILS_TEFF     CAN_MCAN_ILS_TEFF_MSK
/* Tx Event FIFO Element Lost */
#define CAN_MCAN_ILS_TEFL_POS (15U)
#define CAN_MCAN_ILS_TEFL_MSK (0x1UL << CAN_MCAN_ILS_TEFL_POS)
#define CAN_MCAN_ILS_TEFL     CAN_MCAN_ILS_TEFL_MSK
/* Timestamp Wraparound */
#define CAN_MCAN_ILS_TSW_POS  (16U)
#define CAN_MCAN_ILS_TSW_MSK  (0x1UL << CAN_MCAN_ILS_TSW_POS)
#define CAN_MCAN_ILS_TSW      CAN_MCAN_ILS_TSW_MSK
/* Message RAM Access Failure */
#define CAN_MCAN_ILS_MRAF_POS (17U)
#define CAN_MCAN_ILS_MRAF_MSK (0x1UL << CAN_MCAN_ILS_MRAF_POS)
#define CAN_MCAN_ILS_MRAF     CAN_MCAN_ILS_MRAF_MSK
/* Timeout Occurred */
#define CAN_MCAN_ILS_TOO_POS  (18U)
#define CAN_MCAN_ILS_TOO_MSK  (0x1UL << CAN_MCAN_ILS_TOO_POS)
#define CAN_MCAN_ILS_TOO      CAN_MCAN_ILS_TOO_MSK
/* Message stored to Dedicated Rx Buffer */
#define CAN_MCAN_ILS_DRX_POS  (19U)
#define CAN_MCAN_ILS_DRX_MSK  (0x1UL << CAN_MCAN_ILS_DRX_POS)
#define CAN_MCAN_ILS_DRX      CAN_MCAN_ILS_DRX_MSK
/* Bit Error Corrected */
#define CAN_MCAN_ILS_BEC_POS  (20U)
#define CAN_MCAN_ILS_BEC_MSK  (0x1UL << CAN_MCAN_ILS_BEC_POS)
#define CAN_MCAN_ILS_BEC      CAN_MCAN_ILS_BEC_MSK
/* Bit Error Uncorrected */
#define CAN_MCAN_ILS_BEU_POS  (21U)
#define CAN_MCAN_ILS_BEU_MSK  (0x1UL << CAN_MCAN_ILS_BEU_POS)
#define CAN_MCAN_ILS_BEU      CAN_MCAN_ILS_BEU_MSK
/* Error Logging Overflow */
#define CAN_MCAN_ILS_ELO_POS  (22U)
#define CAN_MCAN_ILS_ELO_MSK  (0x1UL << CAN_MCAN_ILS_ELO_POS)
#define CAN_MCAN_ILS_ELO      CAN_MCAN_ILS_ELO_MSK
/* Error Passive*/
#define CAN_MCAN_ILS_EP_POS   (23U)
#define CAN_MCAN_ILS_EP_MSK   (0x1UL << CAN_MCAN_ILS_EP_POS)
#define CAN_MCAN_ILS_EP	      CAN_MCAN_ILS_EP_MSK
/* Warning Status*/
#define CAN_MCAN_ILS_EW_POS   (24U)
#define CAN_MCAN_ILS_EW_MSK   (0x1UL << CAN_MCAN_ILS_EW_POS)
#define CAN_MCAN_ILS_EW	      CAN_MCAN_ILS_EW_MSK
/* Bus_Off Status*/
#define CAN_MCAN_ILS_BO_POS   (25U)
#define CAN_MCAN_ILS_BO_MSK   (0x1UL << CAN_MCAN_ILS_BO_POS)
#define CAN_MCAN_ILS_BO	      CAN_MCAN_ILS_BO_MSK
/* Watchdog Interrupt */
#define CAN_MCAN_ILS_WDI_POS  (26U)
#define CAN_MCAN_ILS_WDI_MSK  (0x1UL << CAN_MCAN_ILS_WDI_POS)
#define CAN_MCAN_ILS_WDI      CAN_MCAN_ILS_WDI_MSK
/* Protocol Error in Arbitration Phase */
#define CAN_MCAN_ILS_PEA_POS  (27U)
#define CAN_MCAN_ILS_PEA_MSK  (0x1UL << CAN_MCAN_ILS_PEA_POS)
#define CAN_MCAN_ILS_PEA      CAN_MCAN_ILS_PEA_MSK
/* Protocol Error in Data Phase */
#define CAN_MCAN_ILS_PED_POS  (28U)
#define CAN_MCAN_ILS_PED_MSK  (0x1UL << CAN_MCAN_ILS_PED_POS)
#define CAN_MCAN_ILS_PED      CAN_MCAN_ILS_PED_MSK
/* Access to Reserved Address */
#define CAN_MCAN_ILS_ARA_POS  (29U)
#define CAN_MCAN_ILS_ARA_MSK  (0x1UL << CAN_MCAN_ILS_ARA_POS)
#define CAN_MCAN_ILS_ARA      CAN_MCAN_IL_ARA_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_ILE register  *******************/
/* Enable Interrupt Line 0 */
#define CAN_MCAN_ILE_EINT0_POS (0U)
#define CAN_MCAN_ILE_EINT0_MSK (0x1UL << CAN_MCAN_ILE_EINT0_POS)
#define CAN_MCAN_ILE_EINT0     CAN_MCAN_ILE_EINT0_MSK
/* Enable Interrupt Line 1 */
#define CAN_MCAN_ILE_EINT1_POS (1U)
#define CAN_MCAN_ILE_EINT1_MSK (0x1UL << CAN_MCAN_ILE_EINT1_POS)
#define CAN_MCAN_ILE_EINT1     CAN_MCAN_ILE_EINT1_MSK

/***************  Bit definition for CAN_MCAN_RXGFC register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Reject Remote Frames Extended */
#define CAN_MCAN_RXGFC_RRFE_POS (0U)
#define CAN_MCAN_RXGFC_RRFE_MSK (0x1UL << CAN_MCAN_RXGFC_RRFE_POS)
#define CAN_MCAN_RXGFC_RRFE	CAN_MCAN_RXGFC_RRFE_MSK
/* Reject Remote Frames Standard */
#define CAN_MCAN_RXGFC_RRFS_POS (1U)
#define CAN_MCAN_RXGFC_RRFS_MSK (0x1UL << CAN_MCAN_RXGFC_RRFS_POS)
#define CAN_MCAN_RXGFC_RRFS	CAN_MCAN_RXGFC_RRFS_MSK
/* Accept Non-matching Frames Extended */
#define CAN_MCAN_RXGFC_ANFE_POS (2U)
#define CAN_MCAN_RXGFC_ANFE_MSK (0x3UL << CAN_MCAN_RXGFC_ANFE_POS)
#define CAN_MCAN_RXGFC_ANFE	CAN_MCAN_RXGFC_ANFE_MSK
/* Accept Non-matching Frames Standard */
#define CAN_MCAN_RXGFC_ANFS_POS (4U)
#define CAN_MCAN_RXGFC_ANFS_MSK (0x3UL << CAN_MCAN_RXGFC_ANFS_POS)
#define CAN_MCAN_RXGFC_ANFS	CAN_MCAN_RXGFC_ANFS_MSK
/* FIFO 1 operation mode */
#define CAN_MCAN_RXGFC_F1OM_POS (8U)
#define CAN_MCAN_RXGFC_F1OM_MSK (0x1UL << CAN_MCAN_RXGFC_F1OM_POS)
#define CAN_MCAN_RXGFC_F1OM	CAN_MCAN_RXGFC_F1OM_MSK
/* FIFO 0 operation mode */
#define CAN_MCAN_RXGFC_F0OM_POS (9U)
#define CAN_MCAN_RXGFC_F0OM_MSK (0x1UL << CAN_MCAN_RXGFC_F0OM_POS)
#define CAN_MCAN_RXGFC_F0OM	CAN_MCAN_RXGFC_F0OM_MSK
/* List Size Standard */
#define CAN_MCAN_RXGFC_LSS_POS	(16U)
#define CAN_MCAN_RXGFC_LSS_MSK	(0x1FUL << CAN_MCAN_RXGFC_LSS_POS)
#define CAN_MCAN_RXGFC_LSS	CAN_MCAN_RXGFC_LSS_MSK
/* List Size Extended */
#define CAN_MCAN_RXGFC_LSE_POS	(24U)
#define CAN_MCAN_RXGFC_LSE_MSK	(0xFUL << CAN_MCAN_RXGFC_LSE_POS)
#define CAN_MCAN_RXGFC_LSE	CAN_MCAN_RXGFC_LSE_MSK

#else /* CONFIG_CAN_STM32FD */

/* Reject Remote Frames Extended */
#define CAN_MCAN_GFC_RRFE_POS	 (0U)
#define CAN_MCAN_GFC_RRFE_MSK	 (0x1UL << CAN_MCAN_GFC_RRFE_POS)
#define CAN_MCAN_GFC_RRFE	 CAN_MCAN_GFC_RRFE_MSK
/* Reject Remote Frames Standard */
#define CAN_MCAN_GFC_RRFS_POS	 (1U)
#define CAN_MCAN_GFC_RRFS_MSK	 (0x1UL << CAN_MCAN_GFC_RRFS_POS)
#define CAN_MCAN_GFC_RRFS	 CAN_MCAN_GFC_RRFS_MSK
/* Accept Non-matching Frames Extended */
#define CAN_MCAN_GFC_ANFE_POS	 (2U)
#define CAN_MCAN_GFC_ANFE_MSK	 (0x3UL << CAN_MCAN_GFC_ANFE_POS)
#define CAN_MCAN_GFC_ANFE	 CAN_MCAN_GFC_ANFE_MSK
/* Accept Non-matching Frames Standard */
#define CAN_MCAN_GFC_ANFS_POS	 (4U)
#define CAN_MCAN_GFC_ANFS_MSK	 (0x3UL << CAN_MCAN_GFC_ANFS_POS)
#define CAN_MCAN_GFC_ANFS	 CAN_MCAN_GFC_ANFS_MSK

/*  Filter List Standard Start Address */
#define CAN_MCAN_SIDFC_FLSSA_POS (2U)
#define CAN_MCAN_SIDFC_FLSSA_MSK (0x3FFFUL << CAN_MCAN_SIDFC_FLSSA_POS)
#define CAN_MCAN_SIDFC_FLSSA	 CAN_MCAN_SIDFC_FLSSA_MSK
/* List Size Standard */
#define CAN_MCAN_SIDFC_LSS_POS	 (16U)
#define CAN_MCAN_SIDFC_LSS_MSK	 (0xFFUL << CAN_MCAN_SIDFC_LSS_POS)
#define CAN_MCAN_SIDFC_LSS	 CAN_MCAN_SIDFC_LSS_MSK

/*  Filter List Extended Start Address */
#define CAN_MCAN_XIDFC_FLESA_POS (2U)
#define CAN_MCAN_XIDFC_FLESA_MSK (0x3FFFUL << CAN_MCAN_XIDFC_FLESA_POS)
#define CAN_MCAN_XIDFC_FLESA	 CAN_MCAN_XIDFC_FLESA_MSK
/* List Size Extended */
#define CAN_MCAN_XIDFC_LSS_POS	 (16U)
#define CAN_MCAN_XIDFC_LSS_MSK	 (0x7FUL << CAN_MCAN_XIDFC_LSS_POS)
#define CAN_MCAN_XIDFC_LSS	 CAN_MCAN_XIDFC_LSS_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_XIDAM register  *****************/
/* Extended ID Mask */
#define CAN_MCAN_XIDAM_EIDM_POS (0U)
#define CAN_MCAN_XIDAM_EIDM_MSK (0x1FFFFFFFUL << CAN_MCAN_XIDAM_EIDM_POS)
#define CAN_MCAN_XIDAM_EIDM	CAN_MCAN_XIDAM_EIDM_MSK

/***************  Bit definition for CAN_MCAN_HPMS register  ******************/
#ifdef CONFIG_CAN_STM32FD
/* Buffer Index */
#define CAN_MCAN_HPMS_BIDX_POS (0U)
#define CAN_MCAN_HPMS_BIDX_MSK (0x7UL << CAN_MCAN_HPMS_BIDX_POS)
#define CAN_MCAN_HPMS_BIDX     CAN_MCAN_HPMS_BIDX_MSK
/* Message Storage Indicator */
#define CAN_MCAN_HPMS_MSI_POS  (6U)
#define CAN_MCAN_HPMS_MSI_MSK  (0x3UL << CAN_MCAN_HPMS_MSI_POS)
#define CAN_MCAN_HPMS_MSI      CAN_MCAN_HPMS_MSI_MSK
/* Filter Index */
#define CAN_MCAN_HPMS_FIDX_POS (8U)
#define CAN_MCAN_HPMS_FIDX_MSK (0x1FUL << CAN_MCAN_HPMS_FIDX_POS)
#define CAN_MCAN_HPMS_FIDX     CAN_MCAN_HPMS_FIDX_MSK
/* Filter List  */
#define CAN_MCAN_HPMS_FLST_POS (15U)
#define CAN_MCAN_HPMS_FLST_MSK (0x1UL << CAN_MCAN_HPMS_FLST_POS)
#define CAN_MCAN_HPMS_FLST     CAN_MCAN_HPMS_FLST_MSK

#else /* CONFIG_CAN_STM32FD */

/* Buffer Index */
#define CAN_MCAN_HPMS_BIDX_POS (0U)
#define CAN_MCAN_HPMS_BIDX_MSK (0x3FUL << CAN_MCAN_HPMS_BIDX_POS)
#define CAN_MCAN_HPMS_BIDX     CAN_MCAN_HPMS_BIDX_MSK
/* Message Storage Indicator */
#define CAN_MCAN_HPMS_MSI_POS  (6U)
#define CAN_MCAN_HPMS_MSI_MSK  (0x3UL << CAN_MCAN_HPMS_MSI_POS)
#define CAN_MCAN_HPMS_MSI      CAN_MCAN_HPMS_MSI_MSK
/* Filter Index */
#define CAN_MCAN_HPMS_FIDX_POS (8U)
#define CAN_MCAN_HPMS_FIDX_MSK (0x7FUL << CAN_MCAN_HPMS_FIDX_POS)
#define CAN_MCAN_HPMS_FIDX     CAN_MCAN_HPMS_FIDX_MSK
/* Filter List  */
#define CAN_MCAN_HPMS_FLST_POS (15U)
#define CAN_MCAN_HPMS_FLST_MSK (0x1UL << CAN_MCAN_HPMS_FLST_POS)
#define CAN_MCAN_HPMS_FLST     CAN_MCAN_HPMS_FLST_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_RXF0C register  *****************/
/* Rx FIFO 0 Start Address */
#define CAN_MCAN_RXF0C_F0SA_POS (2U)
#define CAN_MCAN_RXF0C_F0SA_MSK (0x3FFFUL << CAN_MCAN_RXF0C_F0SA_POS)
#define CAN_MCAN_RXF0C_F0SA	CAN_MCAN_RXF0C_F0SA_MSK
/* Rx FIFO 0 Size */
#define CAN_MCAN_RXF0C_F0S_POS	(16U)
#define CAN_MCAN_RXF0C_F0S_MSK	(0x7FUL << CAN_MCAN_RXF0C_F0S_POS)
#define CAN_MCAN_RXF0C_F0S	CAN_MCAN_RXF0C_F0S_MSK
/* Rx FIFO 0 Watermark */
#define CAN_MCAN_RXF0C_F0WM_POS (24)
#define CAN_MCAN_RXF0C_F0WM_MSK (0x7FUL << CAN_MCAN_RXF0C_F0WM_POS)
#define CAN_MCAN_RXF0C_F0WM	CAN_MCAN_RXF0C_F0WM_MSK
/* FIFO 0 Operation Mode */
#define CAN_MCAN_RXF0C_F0OM_POS (31)
#define CAN_MCAN_RXF0C_F0OM_MSK (0x1UL << CAN_MCAN_RXF0C_F0OM_POS)
#define CAN_MCAN_RXF0C_F0OM	CAN_MCAN_RXF0C_F0OM_MSK

/***************  Bit definition for CAN_MCAN_RXF0S register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Rx FIFO 0 Fill Level */
#define CAN_MCAN_RXF0S_F0FL_POS (0U)
#define CAN_MCAN_RXF0S_F0FL_MSK (0xFUL << CAN_MCAN_RXF0S_F0FL_POS)
#define CAN_MCAN_RXF0S_F0FL	CAN_MCAN_RXF0S_F0FL_MSK
/* Rx FIFO 0 Get Index */
#define CAN_MCAN_RXF0S_F0GI_POS (8U)
#define CAN_MCAN_RXF0S_F0GI_MSK (0x3UL << CAN_MCAN_RXF0S_F0GI_POS)
#define CAN_MCAN_RXF0S_F0GI	CAN_MCAN_RXF0S_F0GI_MSK
/* Rx FIFO 0 Put Index */
#define CAN_MCAN_RXF0S_F0PI_POS (16U)
#define CAN_MCAN_RXF0S_F0PI_MSK (0x3UL << CAN_MCAN_RXF0S_F0PI_POS)
#define CAN_MCAN_RXF0S_F0PI	CAN_MCAN_RXF0S_F0PI_MSK
/* Rx FIFO 0 Full */
#define CAN_MCAN_RXF0S_F0F_POS	(24U)
#define CAN_MCAN_RXF0S_F0F_MSK	(0x1UL << CAN_MCAN_RXF0S_F0F_POS)
#define CAN_MCAN_RXF0S_F0F	CAN_MCAN_RXF0S_F0F_MSK
/* Rx FIFO 0 Message Lost */
#define CAN_MCAN_RXF0S_RF0L_POS (25U)
#define CAN_MCAN_RXF0S_RF0L_MSK (0x1UL << CAN_MCAN_RXF0S_RF0L_POS)
#define CAN_MCAN_RXF0S_RF0L	CAN_MCAN_RXF0S_RF0L_MSK

#else /* CONFIG_CAN_STM32FD */

/* Rx FIFO 0 Fill Level */
#define CAN_MCAN_RXF0S_F0FL_POS (0U)
#define CAN_MCAN_RXF0S_F0FL_MSK (0x7FUL << CAN_MCAN_RXF0S_F0FL_POS)
#define CAN_MCAN_RXF0S_F0FL	CAN_MCAN_RXF0S_F0FL_MSK
/* Rx FIFO 0 Get Index */
#define CAN_MCAN_RXF0S_F0GI_POS (8U)
#define CAN_MCAN_RXF0S_F0GI_MSK (0x3FUL << CAN_MCAN_RXF0S_F0GI_POS)
#define CAN_MCAN_RXF0S_F0GI	CAN_MCAN_RXF0S_F0GI_MSK
/* Rx FIFO 0 Put Index */
#define CAN_MCAN_RXF0S_F0PI_POS (16U)
#define CAN_MCAN_RXF0S_F0PI_MSK (0x3FUL << CAN_MCAN_RXF0S_F0PI_POS)
#define CAN_MCAN_RXF0S_F0PI	CAN_MCAN_RXF0S_F0PI_MSK
/* Rx FIFO 0 Full */
#define CAN_MCAN_RXF0S_F0F_POS	(24U)
#define CAN_MCAN_RXF0S_F0F_MSK	(0x1UL << CAN_MCAN_RXF0S_F0F_POS)
#define CAN_MCAN_RXF0S_F0F	CAN_MCAN_RXF0S_F0F_MSK
/* Rx FIFO 0 Message Lost */
#define CAN_MCAN_RXF0S_RF0L_POS (25U)
#define CAN_MCAN_RXF0S_RF0L_MSK (0x1UL << CAN_MCAN_RXF0S_RF0L_POS)
#define CAN_MCAN_RXF0S_RF0L	CAN_MCAN_RXF0S_RF0L_MSK
#endif

/***************  Bit definition for CAN_MCAN_RXF0A register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Rx FIFO 0 Acknowledge Index */
#define CAN_MCAN_RXF0A_F0AI_POS (0U)
#define CAN_MCAN_RXF0A_F0AI_MSK (0x7UL << CAN_MCAN_RXF0A_F0AI_POS)
#define CAN_MCAN_RXF0A_F0AI	CAN_MCAN_RXF0A_F0AI_MSK
#else
/* Rx FIFO 0 Acknowledge Index */
#define CAN_MCAN_RXF0A_F0AI_POS (0U)
#define CAN_MCAN_RXF0A_F0AI_MSK (0x3FUL << CAN_MCAN_RXF0A_F0AI_POS)
#define CAN_MCAN_RXF0A_F0AI	CAN_MCAN_RXF0A_F0AI_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_RXBC register  ******************/
/*  Rx Buffer Start Address */
#define CAN_MCAN_RXBC_RBSA_POS (2U)
#define CAN_MCAN_RXBC_RBSA_MSK (0x3FFFUL << CAN_MCAN_RXBC_RBSA_POS)
#define CAN_MCAN_RXBC_RBSA     CAN_MCAN_RXBC_RBSA_MSK

/***************  Bit definition for CAN_MCAN_RXF1C register  *****************/
/* Rx FIFO 0 Start Address */
#define CAN_MCAN_RXF1C_F1SA_POS (2U)
#define CAN_MCAN_RXF1C_F1SA_MSK (0x3FFFUL << CAN_MCAN_RXF1C_F1SA_POS)
#define CAN_MCAN_RXF1C_F1SA	CAN_MCAN_RXF1C_F1SA_MSK
/* Rx FIFO 0 Size */
#define CAN_MCAN_RXF1C_F1S_POS	(16U)
#define CAN_MCAN_RXF1C_F1S_MSK	(0x7FUL << CAN_MCAN_RXF1C_F1S_POS)
#define CAN_MCAN_RXF1C_F1S	CAN_MCAN_RXF1C_F1S_MSK
/* Rx FIFO 0 Watermark */
#define CAN_MCAN_RXF1C_F1WM_POS (24)
#define CAN_MCAN_RXF1C_F1WM_MSK (0x7FUL << CAN_MCAN_RXF1C_F1WM_POS)
#define CAN_MCAN_RXF1C_F1WM	CAN_MCAN_RXF1C_F1WM_MSK
/* FIFO 0 Operation Mode */
#define CAN_MCAN_RXF1C_F1OM_POS (31)
#define CAN_MCAN_RXF1C_F1OM_MSK (0x1UL << CAN_MCAN_RXF1C_F1OM_POS)
#define CAN_MCAN_RXF1C_F1OM	CAN_MCAN_RXF1C_F1OM_MSK

/***************  Bit definition for CAN_MCAN_RXF1S register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Rx FIFO 1 Fill Level */
#define CAN_MCAN_RXF1S_F1FL_POS (0U)
#define CAN_MCAN_RXF1S_F1FL_MSK (0xFUL << CAN_MCAN_RXF1S_F1FL_POS)
#define CAN_MCAN_RXF1S_F1FL	CAN_MCAN_RXF1S_F1FL_MSK
/* Rx FIFO 1 Get Index */
#define CAN_MCAN_RXF1S_F1GI_POS (8U)
#define CAN_MCAN_RXF1S_F1GI_MSK (0x3UL << CAN_MCAN_RXF1S_F1GI_POS)
#define CAN_MCAN_RXF1S_F1GI	CAN_MCAN_RXF1S_F1GI_MSK
/* Rx FIFO 1 Put Index */
#define CAN_MCAN_RXF1S_F1PI_POS (16U)
#define CAN_MCAN_RXF1S_F1PI_MSK (0x3UL << CAN_MCAN_RXF1S_F1PI_POS)
#define CAN_MCAN_RXF1S_F1PI	CAN_MCAN_RXF1S_F1PI_MSK
/* Rx FIFO 1 Full */
#define CAN_MCAN_RXF1S_F1F_POS	(24U)
#define CAN_MCAN_RXF1S_F1F_MSK	(0x1UL << CAN_MCAN_RXF1S_F1F_POS)
#define CAN_MCAN_RXF1S_F1F	CAN_MCAN_RXF1S_F1F_MSK
/* Rx FIFO 1 Message Lost */
#define CAN_MCAN_RXF1S_RF1L_POS (25U)
#define CAN_MCAN_RXF1S_RF1L_MSK (0x1UL << CAN_MCAN_RXF1S_RF1L_POS)
#define CAN_MCAN_RXF1S_RF1L	CAN_MCAN_RXF1S_RF1L_MSK

#else /* CONFIG_CAN_STM32FD */

/* Rx FIFO 1 Fill Level */
#define CAN_MCAN_RXF1S_F1FL_POS (0U)
#define CAN_MCAN_RXF1S_F1FL_MSK (0x7FUL << CAN_MCAN_RXF1S_F1FL_POS)
#define CAN_MCAN_RXF1S_F1FL	CAN_MCAN_RXF1S_F1FL_MSK
/* Rx FIFO 1 Get Index */
#define CAN_MCAN_RXF1S_F1GI_POS (8U)
#define CAN_MCAN_RXF1S_F1GI_MSK (0x3FUL << CAN_MCAN_RXF1S_F1GI_POS)
#define CAN_MCAN_RXF1S_F1GI	CAN_MCAN_RXF1S_F1GI_MSK
/* Rx FIFO 1 Put Index */
#define CAN_MCAN_RXF1S_F1PI_POS (16U)
#define CAN_MCAN_RXF1S_F1PI_MSK (0x3FUL << CAN_MCAN_RXF1S_F1PI_POS)
#define CAN_MCAN_RXF1S_F1PI	CAN_MCAN_RXF1S_F1PI_MSK
/* Rx FIFO 1 Full */
#define CAN_MCAN_RXF1S_F1F_POS	(24U)
#define CAN_MCAN_RXF1S_F1F_MSK	(0x1UL << CAN_MCAN_RXF1S_F1F_POS)
#define CAN_MCAN_RXF1S_F1F	CAN_MCAN_RXF1S_F1F_MSK
/* Rx FIFO 1 Message Lost */
#define CAN_MCAN_RXF1S_RF1L_POS (25U)
#define CAN_MCAN_RXF1S_RF1L_MSK (0x1UL << CAN_MCAN_RXF1S_RF1L_POS)
#define CAN_MCAN_RXF1S_RF1L	CAN_MCAN_RXF1S_RF1L_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_RXF1A register  *****************/
/* Rx FIFO 1 Acknowledge Index */
#ifdef CONFIG_CAN_STM32FD
#define CAN_MCAN_RXF1A_F1AI_POS (0U)
#define CAN_MCAN_RXF1A_F1AI_MSK (0x7UL << CAN_MCAN_RXF1A_F1AI_POS)
#define CAN_MCAN_RXF1A_F1AI	CAN_MCAN_RXF1A_F1AI_MSK
#else
#define CAN_MCAN_RXF1A_F1AI_POS (0U)
#define CAN_MCAN_RXF1A_F1AI_MSK (0x3FUL << CAN_MCAN_RXF1A_F1AI_POS)
#define CAN_MCAN_RXF1A_F1AI	CAN_MCAN_RXF1A_F1AI_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_RXESC register  *****************/
/* Rx FIFO 0 Data Field Size */
#define CAN_MCAN_RXESC_F0DS_POS (0U)
#define CAN_MCAN_RXESC_F0DS_MSK (0x7UL << CAN_MCAN_RXESC_F0DS_POS)
#define CAN_MCAN_RXESC_F0DS	CAN_MCAN_RXESC_F0DS_MSK
/* Rx FIFO 1 Data Field Size */
#define CAN_MCAN_RXESC_F1DS_POS (4U)
#define CAN_MCAN_RXESC_F1DS_MSK (0x7UL << CAN_MCAN_RXESC_F1DS_POS)
#define CAN_MCAN_RXESC_F1DS	CAN_MCAN_RXESC_F1DS_MSK
/* Receive Buffer Data Field Size */
#define CAN_MCAN_RXESC_RBDS_POS (8U)
#define CAN_MCAN_RXESC_RBDS_MSK (0x7UL << CAN_MCAN_RXESC_RBDS_POS)
#define CAN_MCAN_RXESC_RBDS	CAN_MCAN_RXESC_RBDS_MSK

/***************  Bit definition for CAN_MCAN_TXBC register  ******************/
#ifdef CONFIG_CAN_STM32FD
/* Tx FIFO/Queue Mode */
#define CAN_MCAN_TXBC_TFQM_POS (24U)
#define CAN_MCAN_TXBC_TFQM_MSK (0x1UL << CAN_MCAN_TXBC_TFQM_POS)
#define CAN_MCAN_TXBC_TFQM     CAN_MCAN_TXBC_TFQM_MSK
#else
/* Tx Buffers Start Address */
#define CAN_MCAN_TXBC_TBSA_POS (2U)
#define CAN_MCAN_TXBC_TBSA_MSK (0x3FFFUL << CAN_MCAN_TXBC_TBSA_POS)
#define CAN_MCAN_TXBC_TBSA     CAN_MCAN_TXBC_TBSA_MSK
/* Number of Dedicated Transmit Buffers */
#define CAN_MCAN_TXBC_NDTB_POS (16U)
#define CAN_MCAN_TXBC_NDTB_MSK (0x3FUL << CAN_MCAN_TXBC_NDTB_POS)
#define CAN_MCAN_TXBC_NDTB     CAN_MCAN_TXBC_NDTB_MSK
/* Transmit FIFO/Queue Size */
#define CAN_MCAN_TXBC_TFQS_POS (24U)
#define CAN_MCAN_TXBC_TFQS_MSK (0x3FUL << CAN_MCAN_TXBC_TFQS_POS)
#define CAN_MCAN_TXBC_TFQS     CAN_MCAN_TXBC_TFQS_MSK
/* Tx FIFO/Queue Mode */
#define CAN_MCAN_TXBC_TFQM_POS (30U)
#define CAN_MCAN_TXBC_TFQM_MSK (0x3FUL << CAN_MCAN_TXBC_TFQM_POS)
#define CAN_MCAN_TXBC_TFQM     CAN_MCAN_TXBC_TFQM_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXFQS register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Tx FIFO Free Level */
#define CAN_MCAN_TXFQS_TFFL_POS	 (0U)
#define CAN_MCAN_TXFQS_TFFL_MSK	 (0x7UL << CAN_MCAN_TXFQS_TFFL_POS)
#define CAN_MCAN_TXFQS_TFFL	 CAN_MCAN_TXFQS_TFFL_MSK
/* Tx FIFO Get Index */
#define CAN_MCAN_TXFQS_TFGI_POS	 (8U)
#define CAN_MCAN_TXFQS_TFGI_MSK	 (0x3UL << CAN_MCAN_TXFQS_TFGI_POS)
#define CAN_MCAN_TXFQS_TFGI	 CAN_MCAN_TXFQS_TFGI_MSK
/* Tx FIFO/Queue Put Index */
#define CAN_MCAN_TXFQS_TFQPI_POS (16U)
#define CAN_MCAN_TXFQS_TFQPI_MSK (0x3UL << CAN_MCAN_TXFQS_TFQPI_POS)
#define CAN_MCAN_TXFQS_TFQPI	 CAN_MCAN_TXFQS_TFQPI_MSK
/* Tx FIFO/Queue Full */
#define CAN_MCAN_TXFQS_TFQF_POS	 (21U)
#define CAN_MCAN_TXFQS_TFQF_MSK	 (0x1UL << CAN_MCAN_TXFQS_TFQF_POS)
#define CAN_MCAN_TXFQS_TFQF	 CAN_MCAN_TXFQS_TFQF_MSK

#else /* CONFIG_CAN_STM32FD */

/* Tx FIFO Free Level */
#define CAN_MCAN_TXFQS_TFFL_POS	 (0U)
#define CAN_MCAN_TXFQS_TFFL_MSK	 (0x3FUL << CAN_MCAN_TXFQS_TFFL_POS)
#define CAN_MCAN_TXFQS_TFFL	 CAN_MCAN_TXFQS_TFFL_MSK
/* Tx FIFO Get Index */
#define CAN_MCAN_TXFQS_TFGI_POS	 (8U)
#define CAN_MCAN_TXFQS_TFGI_MSK	 (0x1FUL << CAN_MCAN_TXFQS_TFGI_POS)
#define CAN_MCAN_TXFQS_TFGI	 CAN_MCAN_TXFQS_TFGI_MSK
/* Tx FIFO/Queue Put Index */
#define CAN_MCAN_TXFQS_TFQPI_POS (16U)
#define CAN_MCAN_TXFQS_TFQPI_MSK (0x1FUL << CAN_MCAN_TXFQS_TFQPI_POS)
#define CAN_MCAN_TXFQS_TFQPI	 CAN_MCAN_TXFQS_TFQPI_MSK
/* Tx FIFO/Queue Full */
#define CAN_MCAN_TXFQS_TFQF_POS	 (21U)
#define CAN_MCAN_TXFQS_TFQF_MSK	 (0x1UL << CAN_MCAN_TXFQS_TFQF_POS)
#define CAN_MCAN_TXFQS_TFQF	 CAN_MCAN_TXFQS_TFQF_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXESC register  *****************/
/* Tx Buffer Data Field Size */
#define CAN_MCAN_TXESC_TBDS_POS (0U)
#define CAN_MCAN_TXESC_TBDS_MSK (0x7UL << CAN_MCAN_TXESC_TBDS_POS)
#define CAN_MCAN_TXESC_TBDS	CAN_MCAN_TXESC_TBDS_MSK

/***************  Bit definition for CAN_MCAN_TXBRP register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Transmission Request Pending */
#define CAN_MCAN_TXBRP_TRP_POS (0U)
#define CAN_MCAN_TXBRP_TRP_MSK (0x7UL << CAN_MCAN_TXBRP_TRP_POS)
#define CAN_MCAN_TXBRP_TRP     CAN_MCAN_TXBRP_TRP_MSK
#else
/* Transmission Request Pending */
#define CAN_MCAN_TXBRP_TRP_POS (0U)
#define CAN_MCAN_TXBRP_TRP_MSK (0xFFFFFFFFUL << CAN_MCAN_TXBRP_TRP_POS)
#define CAN_MCAN_TXBRP_TRP     CAN_MCAN_TXBRP_TRP_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXBAR register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Add Request */
#define CAN_MCAN_TXBAR_AR_POS (0U)
#define CAN_MCAN_TXBAR_AR_MSK (0x7UL << CAN_MCAN_TXBAR_AR_POS)
#define CAN_MCAN_TXBAR_AR     CAN_MCAN_TXBAR_AR_MSK
#else
/* Add Request */
#define CAN_MCAN_TXBAR_AR_POS (0U)
#define CAN_MCAN_TXBAR_AR_MSK (0xFFFFFFFFUL << CAN_MCAN_TXBAR_AR_POS)
#define CAN_MCAN_TXBAR_AR     CAN_MCAN_TXBAR_AR_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXBCR register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Cancellation Request */
#define CAN_MCAN_TXBCR_CR_POS (0U)
#define CAN_MCAN_TXBCR_CR_MSK (0x7UL << CAN_MCAN_TXBCR_CR_POS)
#define CAN_MCAN_TXBCR_CR     CAN_MCAN_TXBCR_CR_MSK
#else
/* Cancellation Request */
#define CAN_MCAN_TXBCR_CR_POS (0U)
#define CAN_MCAN_TXBCR_CR_MSK (0xFFFFFFFFUL << CAN_MCAN_TXBCR_CR_POS)
#define CAN_MCAN_TXBCR_CR     CAN_MCAN_TXBCR_CR_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXBTO register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Transmission Occurred */
#define CAN_MCAN_TXBTO_TO_POS (0U)
#define CAN_MCAN_TXBTO_TO_MSK (0x7UL << CAN_MCAN_TXBTO_TO_POS)
#define CAN_MCAN_TXBTO_TO     CAN_MCAN_TXBTO_TO_MSK
#else
/* Transmission Occurred */
#define CAN_MCAN_TXBTO_TO_POS (0U)
#define CAN_MCAN_TXBTO_TO_MSK (0xFFFFFFFFUL << CAN_MCAN_TXBTO_TO_POS)
#define CAN_MCAN_TXBTO_TO     CAN_MCAN_TXBTO_TO_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXBCF register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Cancellation Finished */
#define CAN_MCAN_TXBCF_CF_POS (0U)
#define CAN_MCAN_TXBCF_CF_MSK (0x7UL << CAN_MCAN_TXBCF_CF_POS)
#define CAN_MCAN_TXBCF_CF     CAN_MCAN_TXBCF_CF_MSK
#else
/* Cancellation Finished */
#define CAN_MCAN_TXBCF_CF_POS (0U)
#define CAN_MCAN_TXBCF_CF_MSK (0xFFFFFFFFUL << CAN_MCAN_TXBCF_CF_POS)
#define CAN_MCAN_TXBCF_CF     CAN_MCAN_TXBCF_CF_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXBTIE register  ****************/
#ifdef CONFIG_CAN_STM32FD
/* Transmission Interrupt Enable */
#define CAN_MCAN_TXBTIE_TIE_POS (0U)
#define CAN_MCAN_TXBTIE_TIE_MSK (0x7UL << CAN_MCAN_TXBTIE_TIE_POS)
#define CAN_MCAN_TXBTIE_TIE	CAN_MCAN_TXBTIE_TIE_MSK
#else
/* Transmission Interrupt Enable */
#define CAN_MCAN_TXBTIE_TIE_POS (0U)
#define CAN_MCAN_TXBTIE_TIE_MSK (0xFFFFFFFFUL << CAN_MCAN_TXBTIE_TIE_POS)
#define CAN_MCAN_TXBTIE_TIE	CAN_MCAN_TXBTIE_TIE_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_ TXBCIE register  ***************/
#ifdef CONFIG_CAN_STM32FD
/* Cancellation Finished Interrupt Enable */
#define CAN_MCAN_TXBCIE_CFIE_POS (0U)
#define CAN_MCAN_TXBCIE_CFIE_MSK (0x7UL << CAN_MCAN_TXBCIE_CFIE_POS)
#define CAN_MCAN_TXBCIE_CFIE	 CAN_MCAN_TXBCIE_CFIE_MSK
#else
/* Cancellation Finished Interrupt Enable */
#define CAN_MCAN_TXBCIE_CFIE_POS (0U)
#define CAN_MCAN_TXBCIE_CFIE_MSK (0xFFFFFFFFUL << CAN_MCAN_TXBCIE_CFIE_POS)
#define CAN_MCAN_TXBCIE_CFIE	 CAN_MCAN_TXBCIE_CFIE_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXEFC register  *****************/
/* Event FIFO Watermark */
#define CAN_MCAN_TXEFC_EFSA_POS (2U)
#define CAN_MCAN_TXEFC_EFSA_MSK (0x3FFFUL << CAN_MCAN_TXEFC_EFSA_POS)
#define CAN_MCAN_TXEFC_EFSA	CAN_MCAN_TXEFC_EFSA_MSK
/* Event FIFO Size */
#define CAN_MCAN_TXEFC_EFS_POS	(16U)
#define CAN_MCAN_TXEFC_EFS_MSK	(0x3FUL << CAN_MCAN_TXEFC_EFS_POS)
#define CAN_MCAN_TXEFC_EFS	CAN_MCAN_TXEFC_EFS_MSK
/* Event FIFO Start Address */
#define CAN_MCAN_TXEFC_EFWM_POS (24U)
#define CAN_MCAN_TXEFC_EFWM_MSK (0x3FUL << CAN_MCAN_TXEFC_EFWM_POS)
#define CAN_MCAN_TXEFC_EFWM	CAN_MCAN_TXEFC_EFWM_POS

/***************  Bit definition for CAN_MCAN_TXEFS register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Event FIFO Fill Level */
#define CAN_MCAN_TXEFS_EFFL_POS (0U)
#define CAN_MCAN_TXEFS_EFFL_MSK (0x7UL << CAN_MCAN_TXEFS_EFFL_POS)
#define CAN_MCAN_TXEFS_EFFL	CAN_MCAN_TXEFS_EFFL_MSK
/* Event FIFO Get Index */
#define CAN_MCAN_TXEFS_EFGI_POS (8U)
#define CAN_MCAN_TXEFS_EFGI_MSK (0x3UL << CAN_MCAN_TXEFS_EFGI_POS)
#define CAN_MCAN_TXEFS_EFGI	CAN_MCAN_TXEFS_EFGI_MSK
/* Event FIFO Put Index */
#define CAN_MCAN_TXEFS_EFPI_POS (16U)
#define CAN_MCAN_TXEFS_EFPI_MSK (0x3UL << CAN_MCAN_TXEFS_EFPI_POS)
#define CAN_MCAN_TXEFS_EFPI	CAN_MCAN_TXEFS_EFPI_MSK
/* Event FIFO Full */
#define CAN_MCAN_TXEFS_EFF_POS	(24U)
#define CAN_MCAN_TXEFS_EFF_MSK	(0x1UL << CAN_MCAN_TXEFS_EFF_POS)
#define CAN_MCAN_TXEFS_EFF	CAN_MCAN_TXEFS_EFF_MSK
/* Tx Event FIFO Element Lost */
#define CAN_MCAN_TXEFS_TEFL_POS (25U)
#define CAN_MCAN_TXEFS_TEFL_MSK (0x1UL << CAN_MCAN_TXEFS_TEFL_POS)
#define CAN_MCAN_TXEFS_TEFL	CAN_MCAN_TXEFS_TEFL_MSK

#else /* CONFIG_CAN_STM32FD */
/* Event FIFO Fill Level */
#define CAN_MCAN_TXEFS_EFFL_POS (0U)
#define CAN_MCAN_TXEFS_EFFL_MSK (0x3FUL << CAN_MCAN_TXEFS_EFFL_POS)
#define CAN_MCAN_TXEFS_EFFL	CAN_MCAN_TXEFS_EFFL_MSK
/* Event FIFO Get Index */
#define CAN_MCAN_TXEFS_EFGI_POS (8U)
#define CAN_MCAN_TXEFS_EFGI_MSK (0x1FUL << CAN_MCAN_TXEFS_EFGI_POS)
#define CAN_MCAN_TXEFS_EFGI	CAN_MCAN_TXEFS_EFGI_MSK
#define CAN_MCAN_TXEFS_EFPI_POS (16U)
/* Event FIFO Put Index */
#define CAN_MCAN_TXEFS_EFPI_MSK (0x1FUL << CAN_MCAN_TXEFS_EFPI_POS)
#define CAN_MCAN_TXEFS_EFPI	CAN_MCAN_TXEFS_EFPI_MSK
/* Event FIFO Full */
#define CAN_MCAN_TXEFS_EFF_POS	(24U)
#define CAN_MCAN_TXEFS_EFF_MSK	(0x1UL << CAN_MCAN_TXEFS_EFF_POS)
#define CAN_MCAN_TXEFS_EFF	CAN_MCAN_TXEFS_EFF_MSK
/* Tx Event FIFO Element Lost */
#define CAN_MCAN_TXEFS_TEFL_POS (25U)
#define CAN_MCAN_TXEFS_TEFL_MSK (0x1UL << CAN_MCAN_TXEFS_TEFL_POS)
#define CAN_MCAN_TXEFS_TEFL	CAN_MCAN_TXEFS_TEFL_MSK

#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_TXEFA register  *****************/
#ifdef CONFIG_CAN_STM32FD
/* Event FIFO Acknowledge Index */
#define CAN_MCAN_TXEFA_EFAI_POS (0U)
#define CAN_MCAN_TXEFA_EFAI_MSK (0x3UL << CAN_MCAN_TXEFA_EFAI_POS)
#define CAN_MCAN_TXEFA_EFAI	CAN_MCAN_TXEFA_EFAI_MSK
#else
/* Event FIFO Acknowledge Index */
#define CAN_MCAN_TXEFA_EFAI_POS (0U)
#define CAN_MCAN_TXEFA_EFAI_MSK (0x1FUL << CAN_MCAN_TXEFA_EFAI_POS)
#define CAN_MCAN_TXEFA_EFAI	CAN_MCAN_TXEFA_EFAI_MSK
#endif /* CONFIG_CAN_STM32FD */

/***************  Bit definition for CAN_MCAN_MRBA register  *****************/
#ifdef CONFIG_CAN_MCUX_MCAN
/* Event FIFO Acknowledge Index */
#define CAN_MCAN_MRBA_BA_POS (16U)
#define CAN_MCAN_MRBA_BA_MSK (0xFFFFUL << CAN_MCAN_MRBA_BA_POS)
#define CAN_MCAN_MRBA_BA     CAN_MCAN_MRBA_BA_MSK
#endif /* CONFIG_CAN_MCUX_MCAN */

#ifdef CONFIG_CAN_STM32FD
struct can_mcan_reg {
	volatile uint32_t crel;	  /* Core Release Register */
	volatile uint32_t endn;	  /* Endian Register */
	volatile uint32_t cust;	  /* Customer Register */
	volatile uint32_t dbtp;	  /* Data Bit Timing & Prescaler Register */
	volatile uint32_t test;	  /* Test Register */
	volatile uint32_t rwd;	  /* RAM Watchdog */
	volatile uint32_t cccr;	  /* CC Control Register */
	volatile uint32_t nbtp;	  /* Nominal Bit Timing & Prescaler Register */
	volatile uint32_t tscc;	  /* Timestamp Counter Configuration */
	volatile uint32_t tscv;	  /* Timestamp Counter Value */
	volatile uint32_t tocc;	  /* Timeout Counter Configuration */
	volatile uint32_t tocv;	  /* Timeout Counter Value */
	uint32_t res1[4];	  /* Reserved (4) */
	volatile uint32_t ecr;	  /* Error Counter Register */
	volatile uint32_t psr;	  /* Protocol Status Register */
	volatile uint32_t tdcr;	  /* Transmitter Delay Compensation */
	uint32_t res2;		  /* Reserved (1) */
	volatile uint32_t ir;	  /* Interrupt Register */
	volatile uint32_t ie;	  /* Interrupt Enable */
	volatile uint32_t ils;	  /* Interrupt Line Select */
	volatile uint32_t ile;	  /* Interrupt Line Enable */
	uint32_t res3[8];	  /* Reserved (8) */
	volatile uint32_t rxgfc;  /* Global Filter Configuration */
	volatile uint32_t xidam;  /* Extended ID AND Mask */
	volatile uint32_t hpms;	  /* High Priority Message Status */
	uint32_t res4;		  /* Reserved (1) */
	volatile uint32_t rxf0s;  /* Rx FIFO 0 Status */
	volatile uint32_t rxf0a;  /* Rx FIFO 0 Acknowledge */
	volatile uint32_t rxf1s;  /* Rx FIFO 1 Status */
	volatile uint32_t rxf1a;  /* Rx FIFO 1 Acknowledge */
	uint32_t res5[8];	  /* Reserved (8) */
	volatile uint32_t txbc;	  /* Tx Buffer Configuration */
	volatile uint32_t txfqs;  /* Tx FIFO/Queue Status */
	volatile uint32_t txbrp;  /* Tx Buffer Request Pending */
	volatile uint32_t txbar;  /* Tx Buffer Add Request */
	volatile uint32_t txbcr;  /* Tx Buffer Cancellation */
	volatile uint32_t txbto;  /* Tx Buffer Transmission */
	volatile uint32_t txbcf;  /* Tx Buffer Cancellation Finished */
	volatile uint32_t txbtie; /* Tx Buffer Transmission Interrupt Enable */
	volatile uint32_t txcbie; /* Tx Buffer Cancellation Fi.Interrupt En. */
	volatile uint32_t txefs;  /* Tx Event FIFO Status */
	volatile uint32_t txefa;  /* Tx Event FIFO Acknowledge */
};
#else /* CONFIG_CAN_STM32FD */

struct can_mcan_reg {
	volatile uint32_t crel;	    /* Core Release Register */
	volatile uint32_t endn;	    /* Endian Register */
	volatile uint32_t cust;	    /* Customer Register */
	volatile uint32_t dbtp;	    /* Data Bit Timing & Prescaler Register */
	volatile uint32_t test;	    /* Test Register */
	volatile uint32_t rwd;	    /* RAM Watchdog */
	volatile uint32_t cccr;	    /* CC Control Register */
	volatile uint32_t nbtp;	    /* Nominal Bit Timing & Prescaler Register */
	volatile uint32_t tscc;	    /* Timestamp Counter Configuration */
	volatile uint32_t tscv;	    /* Timestamp Counter Value */
	volatile uint32_t tocc;	    /* Timeout Counter Configuration */
	volatile uint32_t tocv;	    /* Timeout Counter Value */
	uint32_t res1[4];	    /* Reserved (4) */
	volatile uint32_t ecr;	    /* Error Counter Register */
	volatile uint32_t psr;	    /* Protocol Status Register */
	volatile uint32_t tdcr;	    /* Transmitter Delay Compensation */
	uint32_t res2;		    /* Reserved (1) */
	volatile uint32_t ir;	    /* Interrupt Register */
	volatile uint32_t ie;	    /* Interrupt Enable */
	volatile uint32_t ils;	    /* Interrupt Line Select */
	volatile uint32_t ile;	    /* Interrupt Line Enable */
	uint32_t res3[8];	    /* Reserved (8) */
	volatile uint32_t gfc;	    /* Global Filter Configuration */
	volatile uint32_t sidfc;    /* Standard ID Filter Configuration */
	volatile uint32_t xidfc;    /* Extended ID Filter Configuration */
	volatile uint32_t res4;	    /* Reserved (1) */
	volatile uint32_t xidam;    /* Extended ID AND Mask */
	volatile uint32_t hpms;	    /* High Priority Message Status */
	volatile uint32_t ndata1;   /* New Data 1 */
	volatile uint32_t ndata2;   /* New Data 2 */
	volatile uint32_t rxf0c;    /* Rx FIFO 0 Configuration */
	volatile uint32_t rxf0s;    /* Rx FIFO 0 Status */
	volatile uint32_t rxf0a;    /* FIFO 0 Acknowledge */
	volatile uint32_t rxbc;	    /* Rx Buffer Configuration */
	volatile uint32_t rxf1c;    /* Rx FIFO 1 Configuration */
	volatile uint32_t rxf1s;    /* Rx FIFO 1 Status */
	volatile uint32_t rxf1a;    /* Rx FIFO 1 Acknowledge*/
	volatile uint32_t rxesc;    /* Rx Buffer / FIFO Element Size Config */
	volatile uint32_t txbc;	    /* Buffer Configuration */
	volatile uint32_t txfqs;    /* FIFO/Queue Status */
	volatile uint32_t txesc;    /* Tx Buffer Element Size Configuration */
	volatile uint32_t txbrp;    /* Buffer Request Pending */
	volatile uint32_t txbar;    /* Add Request */
	volatile uint32_t txbcr;    /* Buffer Cancellation Request */
	volatile uint32_t txbto;    /* Tx Buffer Transmission Occurred  */
	volatile uint32_t txbcf;    /* Tx Buffer Cancellation Finished */
	volatile uint32_t txbtie;   /* Tx Buffer Transmission Interrupt Enable */
	volatile uint32_t txbcie;   /* Tx Buffer Cancellation Fin. Interrupt En. */
	volatile uint32_t res5[2];  /* Reserved (2) */
	volatile uint32_t txefc;    /* Tx Event FIFO Configuration */
	volatile uint32_t txefs;    /* Tx Event FIFO Status */
	volatile uint32_t txefa;    /* Tx Event FIFO Acknowledge */
#ifdef CONFIG_CAN_MCUX_MCAN
	volatile uint32_t res6[65]; /* Reserved (65) */
	volatile uint32_t mrba;	    /* Message RAM Base Address */
#endif /* CONFIG_CAN_MCUX_MCAN */
};

#endif /* CONFIG_CAN_STM32FD */

#endif /* ZEPHYR_DRIVERS_CAN_CAN_MCAN_PRIV_H_*/
