/*
 * Copyright (c) 2023 Vestas Wind Systems A/S
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_MCAN_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_MCAN_H_

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/*
 * The Bosch M_CAN register definitions correspond to those found in the Bosch M_CAN Controller Area
 * Network User's Manual, Revision 3.3.0.
 */

/* Core Release register */
#define CAN_MCAN_CREL         0x000
#define CAN_MCAN_CREL_REL     GENMASK(31, 28)
#define CAN_MCAN_CREL_STEP    GENMASK(27, 24)
#define CAN_MCAN_CREL_SUBSTEP GENMASK(23, 20)
#define CAN_MCAN_CREL_YEAR    GENMASK(19, 16)
#define CAN_MCAN_CREL_MON     GENMASK(15, 8)
#define CAN_MCAN_CREL_DAY     GENMASK(7, 0)

/* Endian register */
#define CAN_MCAN_ENDN     0x004
#define CAN_MCAN_ENDN_ETV GENMASK(31, 0)

/* Customer register */
#define CAN_MCAN_CUST      0x008
#define CAN_MCAN_CUST_CUST GENMASK(31, 0)

/* Data Bit Timing & Prescaler register */
#define CAN_MCAN_DBTP        0x00C
#define CAN_MCAN_DBTP_TDC    BIT(23)
#define CAN_MCAN_DBTP_DBRP   GENMASK(20, 16)
#define CAN_MCAN_DBTP_DTSEG1 GENMASK(12, 8)
#define CAN_MCAN_DBTP_DTSEG2 GENMASK(7, 4)
#define CAN_MCAN_DBTP_DSJW   GENMASK(3, 0)

/* Test register */
#define CAN_MCAN_TEST       0x010
#define CAN_MCAN_TEST_SVAL  BIT(21)
#define CAN_MCAN_TEST_TXBNS GENMASK(20, 16)
#define CAN_MCAN_TEST_PVAL  BIT(13)
#define CAN_MCAN_TEST_TXBNP GENMASK(12, 8)
#define CAN_MCAN_TEST_RX    BIT(7)
#define CAN_MCAN_TEST_TX    GENMASK(6, 5)
#define CAN_MCAN_TEST_LBCK  BIT(4)

/* RAM Watchdog register */
#define CAN_MCAN_RWD     0x014
#define CAN_MCAN_RWD_WDV GENMASK(15, 8)
#define CAN_MCAN_RWD_WDC GENMASK(7, 0)

/* CC Control register */
#define CAN_MCAN_CCCR      0x018
#define CAN_MCAN_CCCR_NISO BIT(15)
#define CAN_MCAN_CCCR_TXP  BIT(14)
#define CAN_MCAN_CCCR_EFBI BIT(13)
#define CAN_MCAN_CCCR_PXHD BIT(12)
#define CAN_MCAN_CCCR_WMM  BIT(11)
#define CAN_MCAN_CCCR_UTSU BIT(10)
#define CAN_MCAN_CCCR_BRSE BIT(9)
#define CAN_MCAN_CCCR_FDOE BIT(8)
#define CAN_MCAN_CCCR_TEST BIT(7)
#define CAN_MCAN_CCCR_DAR  BIT(6)
#define CAN_MCAN_CCCR_MON  BIT(5)
#define CAN_MCAN_CCCR_CSR  BIT(4)
#define CAN_MCAN_CCCR_CSA  BIT(3)
#define CAN_MCAN_CCCR_ASM  BIT(2)
#define CAN_MCAN_CCCR_CCE  BIT(1)
#define CAN_MCAN_CCCR_INIT BIT(0)

/* Nominal Bit Timing & Prescaler register */
#define CAN_MCAN_NBTP        0x01C
#define CAN_MCAN_NBTP_NSJW   GENMASK(31, 25)
#define CAN_MCAN_NBTP_NBRP   GENMASK(24, 16)
#define CAN_MCAN_NBTP_NTSEG1 GENMASK(15, 8)
#define CAN_MCAN_NBTP_NTSEG2 GENMASK(6, 0)

/* Timestamp Counter Configuration register */
#define CAN_MCAN_TSCC     0x020
#define CAN_MCAN_TSCC_TCP GENMASK(19, 16)
#define CAN_MCAN_TSCC_TSS GENMASK(1, 0)

/* Timestamp Counter Value register */
#define CAN_MCAN_TSCV     0x024
#define CAN_MCAN_TSCV_TSC GENMASK(15, 0)

/* Timeout Counter Configuration register */
#define CAN_MCAN_TOCC      0x028
#define CAN_MCAN_TOCC_TOP  GENMASK(31, 16)
#define CAN_MCAN_TOCC_TOS  GENMASK(2, 1)
#define CAN_MCAN_TOCC_ETOC BIT(1)

/* Timeout Counter Value register */
#define CAN_MCAN_TOCV     0x02C
#define CAN_MCAN_TOCV_TOC GENMASK(15, 0)

/* Error Counter register */
#define CAN_MCAN_ECR     0x040
#define CAN_MCAN_ECR_CEL GENMASK(23, 16)
#define CAN_MCAN_ECR_RP  BIT(15)
#define CAN_MCAN_ECR_REC GENMASK(14, 8)
#define CAN_MCAN_ECR_TEC GENMASK(7, 0)

/* Protocol Status register */
#define CAN_MCAN_PSR      0x044
#define CAN_MCAN_PSR_TDCV GENMASK(22, 16)
#define CAN_MCAN_PSR_PXE  BIT(14)
#define CAN_MCAN_PSR_RFDF BIT(13)
#define CAN_MCAN_PSR_RBRS BIT(12)
#define CAN_MCAN_PSR_RESI BIT(11)
#define CAN_MCAN_PSR_DLEC GENMASK(10, 8)
#define CAN_MCAN_PSR_BO   BIT(7)
#define CAN_MCAN_PSR_EW   BIT(6)
#define CAN_MCAN_PSR_EP   BIT(5)
#define CAN_MCAN_PSR_ACT  GENMASK(4, 3)
#define CAN_MCAN_PSR_LEC  GENMASK(2, 0)

enum can_mcan_psr_lec {
	CAN_MCAN_PSR_LEC_NO_ERROR    = 0,
	CAN_MCAN_PSR_LEC_STUFF_ERROR = 1,
	CAN_MCAN_PSR_LEC_FORM_ERROR  = 2,
	CAN_MCAN_PSR_LEC_ACK_ERROR   = 3,
	CAN_MCAN_PSR_LEC_BIT1_ERROR  = 4,
	CAN_MCAN_PSR_LEC_BIT0_ERROR  = 5,
	CAN_MCAN_PSR_LEC_CRC_ERROR   = 6,
	CAN_MCAN_PSR_LEC_NO_CHANGE   = 7
};

/* Transmitter Delay Compensation register */
#define CAN_MCAN_TDCR      0x048
#define CAN_MCAN_TDCR_TDCO GENMASK(14, 8)
#define CAN_MCAN_TDCR_TDCF GENMASK(6, 0)

/* Interrupt register */
#define CAN_MCAN_IR      0x050
#define CAN_MCAN_IR_ARA  BIT(29)
#define CAN_MCAN_IR_PED  BIT(28)
#define CAN_MCAN_IR_PEA  BIT(27)
#define CAN_MCAN_IR_WDI  BIT(26)
#define CAN_MCAN_IR_BO   BIT(25)
#define CAN_MCAN_IR_EW   BIT(24)
#define CAN_MCAN_IR_EP   BIT(23)
#define CAN_MCAN_IR_ELO  BIT(22)
#define CAN_MCAN_IR_BEU  BIT(21)
#define CAN_MCAN_IR_BEC  BIT(20)
#define CAN_MCAN_IR_DRX  BIT(19)
#define CAN_MCAN_IR_TOO  BIT(18)
#define CAN_MCAN_IR_MRAF BIT(17)
#define CAN_MCAN_IR_TSW  BIT(16)
#define CAN_MCAN_IR_TEFL BIT(15)
#define CAN_MCAN_IR_TEFF BIT(14)
#define CAN_MCAN_IR_TEFW BIT(13)
#define CAN_MCAN_IR_TEFN BIT(12)
#define CAN_MCAN_IR_TFE  BIT(11)
#define CAN_MCAN_IR_TCF  BIT(10)
#define CAN_MCAN_IR_TC   BIT(9)
#define CAN_MCAN_IR_HPM  BIT(8)
#define CAN_MCAN_IR_RF1L BIT(7)
#define CAN_MCAN_IR_RF1F BIT(6)
#define CAN_MCAN_IR_RF1W BIT(5)
#define CAN_MCAN_IR_RF1N BIT(4)
#define CAN_MCAN_IR_RF0L BIT(3)
#define CAN_MCAN_IR_RF0F BIT(2)
#define CAN_MCAN_IR_RF0W BIT(1)
#define CAN_MCAN_IR_RF0N BIT(0)

/* Interrupt Enable register */
#define CAN_MCAN_IE       0x054
#define CAN_MCAN_IE_ARAE  BIT(29)
#define CAN_MCAN_IE_PEDE  BIT(28)
#define CAN_MCAN_IE_PEAE  BIT(27)
#define CAN_MCAN_IE_WDIE  BIT(26)
#define CAN_MCAN_IE_BOE   BIT(25)
#define CAN_MCAN_IE_EWE   BIT(24)
#define CAN_MCAN_IE_EPE   BIT(23)
#define CAN_MCAN_IE_ELOE  BIT(22)
#define CAN_MCAN_IE_BEUE  BIT(21)
#define CAN_MCAN_IE_BECE  BIT(20)
#define CAN_MCAN_IE_DRXE  BIT(19)
#define CAN_MCAN_IE_TOOE  BIT(18)
#define CAN_MCAN_IE_MRAFE BIT(17)
#define CAN_MCAN_IE_TSWE  BIT(16)
#define CAN_MCAN_IE_TEFLE BIT(15)
#define CAN_MCAN_IE_TEFFE BIT(14)
#define CAN_MCAN_IE_TEFWE BIT(13)
#define CAN_MCAN_IE_TEFNE BIT(12)
#define CAN_MCAN_IE_TFEE  BIT(11)
#define CAN_MCAN_IE_TCFE  BIT(10)
#define CAN_MCAN_IE_TCE   BIT(9)
#define CAN_MCAN_IE_HPME  BIT(8)
#define CAN_MCAN_IE_RF1LE BIT(7)
#define CAN_MCAN_IE_RF1FE BIT(6)
#define CAN_MCAN_IE_RF1WE BIT(5)
#define CAN_MCAN_IE_RF1NE BIT(4)
#define CAN_MCAN_IE_RF0LE BIT(3)
#define CAN_MCAN_IE_RF0FE BIT(2)
#define CAN_MCAN_IE_RF0WE BIT(1)
#define CAN_MCAN_IE_RF0NE BIT(0)

/* Interrupt Line Select register */
#define CAN_MCAN_ILS       0x058
#define CAN_MCAN_ILS_ARAL  BIT(29)
#define CAN_MCAN_ILS_PEDL  BIT(28)
#define CAN_MCAN_ILS_PEAL  BIT(27)
#define CAN_MCAN_ILS_WDIL  BIT(26)
#define CAN_MCAN_ILS_BOL   BIT(25)
#define CAN_MCAN_ILS_EWL   BIT(24)
#define CAN_MCAN_ILS_EPL   BIT(23)
#define CAN_MCAN_ILS_ELOL  BIT(22)
#define CAN_MCAN_ILS_BEUL  BIT(21)
#define CAN_MCAN_ILS_BECL  BIT(20)
#define CAN_MCAN_ILS_DRXL  BIT(19)
#define CAN_MCAN_ILS_TOOL  BIT(18)
#define CAN_MCAN_ILS_MRAFL BIT(17)
#define CAN_MCAN_ILS_TSWL  BIT(16)
#define CAN_MCAN_ILS_TEFLL BIT(15)
#define CAN_MCAN_ILS_TEFFL BIT(14)
#define CAN_MCAN_ILS_TEFWL BIT(13)
#define CAN_MCAN_ILS_TEFNL BIT(12)
#define CAN_MCAN_ILS_TFEL  BIT(11)
#define CAN_MCAN_ILS_TCFL  BIT(10)
#define CAN_MCAN_ILS_TCL   BIT(9)
#define CAN_MCAN_ILS_HPML  BIT(8)
#define CAN_MCAN_ILS_RF1LL BIT(7)
#define CAN_MCAN_ILS_RF1FL BIT(6)
#define CAN_MCAN_ILS_RF1WL BIT(5)
#define CAN_MCAN_ILS_RF1NL BIT(4)
#define CAN_MCAN_ILS_RF0LL BIT(3)
#define CAN_MCAN_ILS_RF0FL BIT(2)
#define CAN_MCAN_ILS_RF0WL BIT(1)
#define CAN_MCAN_ILS_RF0NL BIT(0)

/* Interrupt Line Enable register */
#define CAN_MCAN_ILE       0x05C
#define CAN_MCAN_ILE_EINT1 BIT(1)
#define CAN_MCAN_ILE_EINT0 BIT(0)

/* Global filter configuration register */
#define CAN_MCAN_GFC      0x080
#define CAN_MCAN_GFC_ANFS GENMASK(5, 4)
#define CAN_MCAN_GFC_ANFE GENMASK(3, 2)
#define CAN_MCAN_GFC_RRFS BIT(1)
#define CAN_MCAN_GFC_RRFE BIT(0)

/* Standard ID Filter Configuration register */
#define CAN_MCAN_SIDFC       0x084
#define CAN_MCAN_SIDFC_LSS   GENMASK(23, 16)
#define CAN_MCAN_SIDFC_FLSSA GENMASK(15, 2)

/* Extended ID Filter Configuration register */
#define CAN_MCAN_XIDFC       0x088
#define CAN_MCAN_XIDFC_LSS   GENMASK(22, 16)
#define CAN_MCAN_XIDFC_FLESA GENMASK(15, 2)

/* Extended ID AND Mask register */
#define CAN_MCAN_XIDAM      0x090
#define CAN_MCAN_XIDAM_EIDM GENMASK(28, 0)

/* High Priority Message Status register */
#define CAN_MCAN_HPMS      0x094
#define CAN_MCAN_HPMS_FLST BIT(15)
#define CAN_MCAN_HPMS_FIDX GENMASK(14, 8)
#define CAN_MCAN_HPMS_MSI  GENMASK(7, 6)
#define CAN_MCAN_HPMS_BIDX GENMASK(5, 0)

/* New Data 1 register */
#define CAN_MCAN_NDAT1    0x098
#define CAN_MCAN_NDAT1_ND GENMASK(31, 0)

/* New Data 2 register */
#define CAN_MCAN_NDAT2    0x09C
#define CAN_MCAN_NDAT2_ND GENMASK(31, 0)

/* Rx FIFO 0 Configuration register */
#define CAN_MCAN_RXF0C      0x0A0
#define CAN_MCAN_RXF0C_F0OM BIT(31)
#define CAN_MCAN_RXF0C_F0WM GENMASK(30, 24)
#define CAN_MCAN_RXF0C_F0S  GENMASK(22, 16)
#define CAN_MCAN_RXF0C_F0SA GENMASK(15, 2)

/* Rx FIFO 0 Status register */
#define CAN_MCAN_RXF0S      0x0A4
#define CAN_MCAN_RXF0S_RF0L BIT(25)
#define CAN_MCAN_RXF0S_F0F  BIT(24)
#define CAN_MCAN_RXF0S_F0PI GENMASK(21, 16)
#define CAN_MCAN_RXF0S_F0GI GENMASK(13, 8)
#define CAN_MCAN_RXF0S_F0FL GENMASK(6, 0)

/* Rx FIFO 0 Acknowledge register */
#define CAN_MCAN_RXF0A      0x0A8
#define CAN_MCAN_RXF0A_F0AI GENMASK(5, 0)

/* Rx Buffer Configuration register */
#define CAN_MCAN_RXBC      0x0AC
#define CAN_MCAN_RXBC_RBSA GENMASK(15, 2)

/* Rx FIFO 1 Configuration register */
#define CAN_MCAN_RXF1C      0x0B0
#define CAN_MCAN_RXF1C_F1OM BIT(31)
#define CAN_MCAN_RXF1C_F1WM GENMASK(30, 24)
#define CAN_MCAN_RXF1C_F1S  GENMASK(22, 16)
#define CAN_MCAN_RXF1C_F1SA GENMASK(15, 2)

/* Rx FIFO 1 Status register */
#define CAN_MCAN_RXF1S      0x0B4
#define CAN_MCAN_RXF1S_RF1L BIT(25)
#define CAN_MCAN_RXF1S_F1F  BIT(24)
#define CAN_MCAN_RXF1S_F1PI GENMASK(21, 16)
#define CAN_MCAN_RXF1S_F1GI GENMASK(13, 8)
#define CAN_MCAN_RXF1S_F1FL GENMASK(6, 0)

/* Rx FIFO 1 Acknowledge register */
#define CAN_MCAN_RXF1A      0x0B8
#define CAN_MCAN_RXF1A_F1AI GENMASK(5, 0)

/* Rx Buffer/FIFO Element Size Configuration register */
#define CAN_MCAN_RXESC      0x0BC
#define CAN_MCAN_RXESC_RBDS GENMASK(10, 8)
#define CAN_MCAN_RXESC_F1DS GENMASK(6, 4)
#define CAN_MCAN_RXESC_F0DS GENMASK(2, 0)

/* Tx Buffer Configuration register */
#define CAN_MCAN_TXBC      0x0C0
#define CAN_MCAN_TXBC_TFQM BIT(30)
#define CAN_MCAN_TXBC_TFQS GENMASK(29, 24)
#define CAN_MCAN_TXBC_NDTB GENMASK(21, 16)
#define CAN_MCAN_TXBC_TBSA GENMASK(15, 2)

/* Tx FIFO/Queue Status register */
#define CAN_MCAN_TXFQS       0x0C4
#define CAN_MCAN_TXFQS_TFQF  BIT(21)
#define CAN_MCAN_TXFQS_TFQPI GENMASK(20, 16)
#define CAN_MCAN_TXFQS_TFGI  GENMASK(12, 8)
#define CAN_MCAN_TXFQS_TFFL  GENMASK(5, 0)

/* Tx Buffer Element Size Configuration register */
#define CAN_MCAN_TXESC      0x0C8
#define CAN_MCAN_TXESC_TBDS GENMASK(2, 0)

/* Tx Buffer Request Pending register */
#define CAN_MCAN_TXBRP     0x0CC
#define CAN_MCAN_TXBRP_TRP GENMASK(31, 0)

/* Tx Buffer Add Request register */
#define CAN_MCAN_TXBAR    0x0D0
#define CAN_MCAN_TXBAR_AR GENMASK(31, 0)

/* Tx Buffer Cancellation Request register */
#define CAN_MCAN_TXBCR    0x0D4
#define CAN_MCAN_TXBCR_CR GENMASK(31, 0)

/* Tx Buffer Transmission Occurred register */
#define CAN_MCAN_TXBTO    0x0D8
#define CAN_MCAN_TXBTO_TO GENMASK(31, 0)

/* Tx Buffer Cancellation Finished register */
#define CAN_MCAN_TXBCF    0x0DC
#define CAN_MCAN_TXBCF_CF GENMASK(31, 0)

/* Tx Buffer Transmission Interrupt Enable register */
#define CAN_MCAN_TXBTIE     0x0E0
#define CAN_MCAN_TXBTIE_TIE GENMASK(31, 0)

/* Tx Buffer Cancellation Finished Interrupt Enable register */
#define CAN_MCAN_TXBCIE      0x0E4
#define CAN_MCAN_TXBCIE_CFIE GENMASK(31, 0)

/* Tx Event FIFO Configuration register */
#define CAN_MCAN_TXEFC      0x0F0
#define CAN_MCAN_TXEFC_EFWM GENMASK(29, 24)
#define CAN_MCAN_TXEFC_EFS  GENMASK(21, 16)
#define CAN_MCAN_TXEFC_EFSA GENMASK(15, 2)

/* Tx Event FIFO Status register */
#define CAN_MCAN_TXEFS      0x0F4
#define CAN_MCAN_TXEFS_TEFL BIT(25)
#define CAN_MCAN_TXEFS_EFF  BIT(24)
#define CAN_MCAN_TXEFS_EFPI GENMASK(20, 16)
#define CAN_MCAN_TXEFS_EFGI GENMASK(12, 8)
#define CAN_MCAN_TXEFS_EFFL GENMASK(5, 0)

/* Tx Event FIFO Acknowledge register */
#define CAN_MCAN_TXEFA      0x0F8
#define CAN_MCAN_TXEFA_EFAI GENMASK(4, 0)

/**
 * @name Indexes for the cells in the devicetree bosch,mram-cfg property
 * @anchor CAN_MCAN_MRAM_CFG

 * These match the description of the cells in the bosch,m_can-base devicetree binding.
 *
 * @{
 */
/** offset cell index */
#define CAN_MCAN_MRAM_CFG_OFFSET        0
/** std-filter-elements cell index */
#define CAN_MCAN_MRAM_CFG_STD_FILTER    1
/** ext-filter-elements cell index */
#define CAN_MCAN_MRAM_CFG_EXT_FILTER    2
/** rx-fifo0-elements cell index */
#define CAN_MCAN_MRAM_CFG_RX_FIFO0      3
/** rx-fifo1-elements cell index */
#define CAN_MCAN_MRAM_CFG_RX_FIFO1      4
/** rx-buffer-elements cell index */
#define CAN_MCAN_MRAM_CFG_RX_BUFFER     5
/** tx-event-fifo-elements cell index */
#define CAN_MCAN_MRAM_CFG_TX_EVENT_FIFO 6
/** tx-buffer-elements cell index */
#define CAN_MCAN_MRAM_CFG_TX_BUFFER     7
/** Total number of cells in bosch,mram-cfg property */
#define CAN_MCAN_MRAM_CFG_NUM_CELLS     8

/** @} */

/**
 * @brief Get the Bosch M_CAN Message RAM offset
 *
 * @param node_id node identifier
 * @return the Message RAM offset in bytes
 */
#define CAN_MCAN_DT_MRAM_OFFSET(node_id)                                                           \
	DT_PROP_BY_IDX(node_id, bosch_mram_cfg, CAN_MCAN_MRAM_CFG_OFFSET)

/**
 * @brief Get the number of standard (11-bit) filter elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the number of standard (11-bit) filter elements
 */
#define CAN_MCAN_DT_MRAM_STD_FILTER_ELEMENTS(node_id)                                              \
	DT_PROP_BY_IDX(node_id, bosch_mram_cfg, CAN_MCAN_MRAM_CFG_STD_FILTER)

/**
 * @brief Get the number of extended (29-bit) filter elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the number of extended (29-bit) filter elements
 */
#define CAN_MCAN_DT_MRAM_EXT_FILTER_ELEMENTS(node_id)                                              \
	DT_PROP_BY_IDX(node_id, bosch_mram_cfg, CAN_MCAN_MRAM_CFG_EXT_FILTER)

/**
 * @brief Get the number of Rx FIFO 0 elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the number of Rx FIFO 0 elements
 */
#define CAN_MCAN_DT_MRAM_RX_FIFO0_ELEMENTS(node_id)                                                \
	DT_PROP_BY_IDX(node_id, bosch_mram_cfg, CAN_MCAN_MRAM_CFG_RX_FIFO0)

/**
 * @brief Get the number of Rx FIFO 1 elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the number of Rx FIFO 1 elements
 */
#define CAN_MCAN_DT_MRAM_RX_FIFO1_ELEMENTS(node_id)                                                \
	DT_PROP_BY_IDX(node_id, bosch_mram_cfg, CAN_MCAN_MRAM_CFG_RX_FIFO1)

/**
 * @brief Get the number of Rx Buffer elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the number of Rx Buffer elements
 */
#define CAN_MCAN_DT_MRAM_RX_BUFFER_ELEMENTS(node_id)                                               \
	DT_PROP_BY_IDX(node_id, bosch_mram_cfg, CAN_MCAN_MRAM_CFG_RX_BUFFER)

/**
 * @brief Get the number of Tx Event FIFO elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the number of Tx Event FIFO elements
 */
#define CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_ELEMENTS(node_id)                                           \
	DT_PROP_BY_IDX(node_id, bosch_mram_cfg, CAN_MCAN_MRAM_CFG_TX_EVENT_FIFO)

/**
 * @brief Get the number of Tx Buffer elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the number of Tx Buffer elements
 */
#define CAN_MCAN_DT_MRAM_TX_BUFFER_ELEMENTS(node_id)                                               \
	DT_PROP_BY_IDX(node_id, bosch_mram_cfg, CAN_MCAN_MRAM_CFG_TX_BUFFER)

/**
 * @brief Get the base offset of standard (11-bit) filter elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the base offset of standard (11-bit) filter elements in bytes
 */
#define CAN_MCAN_DT_MRAM_STD_FILTER_OFFSET(node_id) (0U)

/**
 * @brief Get the base offset of extended (29-bit) filter elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the base offset of extended (29-bit) filter elements in bytes
 */
#define CAN_MCAN_DT_MRAM_EXT_FILTER_OFFSET(node_id)                                                \
	(CAN_MCAN_DT_MRAM_STD_FILTER_OFFSET(node_id) +                                             \
	 CAN_MCAN_DT_MRAM_STD_FILTER_ELEMENTS(node_id) * sizeof(struct can_mcan_std_filter))

/**
 * @brief Get the base offset of Rx FIFO 0 elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the base offset of Rx FIFO 0 elements in bytes
 */
#define CAN_MCAN_DT_MRAM_RX_FIFO0_OFFSET(node_id)                                                  \
	(CAN_MCAN_DT_MRAM_EXT_FILTER_OFFSET(node_id) +                                             \
	 CAN_MCAN_DT_MRAM_EXT_FILTER_ELEMENTS(node_id) * sizeof(struct can_mcan_ext_filter))

/**
 * @brief Get the base offset of Rx FIFO 1 elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the base offset of Rx FIFO 1 elements in bytes
 */
#define CAN_MCAN_DT_MRAM_RX_FIFO1_OFFSET(node_id)                                                  \
	(CAN_MCAN_DT_MRAM_RX_FIFO0_OFFSET(node_id) +                                               \
	 CAN_MCAN_DT_MRAM_RX_FIFO0_ELEMENTS(node_id) * sizeof(struct can_mcan_rx_fifo))

/**
 * @brief Get the base offset of Rx Buffer elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the base offset of Rx Buffer elements in bytes
 */
#define CAN_MCAN_DT_MRAM_RX_BUFFER_OFFSET(node_id)                                                 \
	(CAN_MCAN_DT_MRAM_RX_FIFO1_OFFSET(node_id) +                                               \
	 CAN_MCAN_DT_MRAM_RX_FIFO1_ELEMENTS(node_id) * sizeof(struct can_mcan_rx_fifo))

/**
 * @brief Get the base offset of Tx Event FIFO elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the base offset of Tx Event FIFO elements in bytes
 */
#define CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_OFFSET(node_id)                                             \
	(CAN_MCAN_DT_MRAM_RX_BUFFER_OFFSET(node_id) +                                              \
	 CAN_MCAN_DT_MRAM_RX_BUFFER_ELEMENTS(node_id) * sizeof(struct can_mcan_rx_fifo))

/**
 * @brief Get the base offset of Tx Buffer elements in Bosch M_CAN Message RAM
 *
 * @param node_id node identifier
 * @return the base offset of Tx Buffer elements in bytes
 */
#define CAN_MCAN_DT_MRAM_TX_BUFFER_OFFSET(node_id)                                                 \
	(CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_OFFSET(node_id) +                                          \
	 CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_ELEMENTS(node_id) * sizeof(struct can_mcan_tx_event_fifo))

/**
 * @brief Get the Bosch M_CAN register base address
 *
 * For devicetree nodes with just one register block, this macro returns the base address of that
 * register block.
 *
 * If a devicetree node has more than one register block, this macros returns the base address of
 * the register block named "m_can".
 *
 * @param node_id node identifier
 * @return the Bosch M_CAN register base address
 */
#define CAN_MCAN_DT_MCAN_ADDR(node_id)                                                             \
	COND_CODE_1(DT_NUM_REGS(node_id), ((mm_reg_t)DT_REG_ADDR(node_id)),                        \
		    ((mm_reg_t)DT_REG_ADDR_BY_NAME(node_id, m_can)))

/**
 * @brief Get the Bosch M_CAN Message RAM base address
 *
 * For devicetree nodes with dedicated Message RAM area defined via devicetree, this macro returns
 * the base address of the Message RAM.
 *
 * @param node_id node identifier
 * @return the Bosch M_CAN Message RAM base address (MRBA)
 */
#define CAN_MCAN_DT_MRBA(node_id)                                                                  \
	(mem_addr_t)DT_REG_ADDR_BY_NAME(node_id, message_ram)

/**
 * @brief Get the Bosch M_CAN Message RAM address
 *
 * For devicetree nodes with dedicated Message RAM area defined via devicetree, this macro returns
 * the address of the Message RAM, taking in the Message RAM offset into account.
 *
 * @param node_id node identifier
 * @return the Bosch M_CAN Message RAM address
 */
#define CAN_MCAN_DT_MRAM_ADDR(node_id)                                                             \
	(mem_addr_t)(CAN_MCAN_DT_MRBA(node_id) + CAN_MCAN_DT_MRAM_OFFSET(node_id))

/**
 * @brief Get the Bosch M_CAN Message RAM size
 *
 * For devicetree nodes with dedicated Message RAM area defined via devicetree, this macro returns
 * the size of the Message RAM, taking in the Message RAM offset into account.
 *
 * @param node_id node identifier
 * @return the Bosch M_CAN Message RAM base address
 * @see CAN_MCAN_DT_MRAM_ELEMENTS_SIZE()
 */
#define CAN_MCAN_DT_MRAM_SIZE(node_id)                                                             \
	(mem_addr_t)(DT_REG_SIZE_BY_NAME(node_id, message_ram) - CAN_MCAN_DT_MRAM_OFFSET(node_id))

/**
 * @brief Get the total size of all Bosch M_CAN Message RAM elements
 *
 * @param node_id node identifier
 * @return the total size of all Message RAM elements in bytes
 * @see CAN_MCAN_DT_MRAM_SIZE()
 */
#define CAN_MCAN_DT_MRAM_ELEMENTS_SIZE(node_id)                                                    \
	(CAN_MCAN_DT_MRAM_TX_BUFFER_OFFSET(node_id) +                                              \
	 CAN_MCAN_DT_MRAM_TX_BUFFER_ELEMENTS(node_id) * sizeof(struct can_mcan_tx_buffer))

/**
 * @brief Define a RAM buffer for Bosch M_CAN Message RAM
 *
 * For devicetree nodes without dedicated Message RAM area, this macro defines a suitable RAM buffer
 * to hold the Message RAM elements. Since this buffer cannot be shared between multiple Bosch M_CAN
 * instances, the Message RAM offset must be set to 0x0.
 *
 * @param node_id node identifier
 * @param _name buffer variable name
 */
#define CAN_MCAN_DT_MRAM_DEFINE(node_id, _name)                                                    \
	BUILD_ASSERT(CAN_MCAN_DT_MRAM_OFFSET(node_id) == 0, "offset must be 0");                   \
	static char __nocache_noinit __aligned(4) _name[CAN_MCAN_DT_MRAM_ELEMENTS_SIZE(node_id)];

/**
 * @brief Assert that the Message RAM configuration meets the Bosch M_CAN IP core restrictions
 *
 * @param node_id node identifier
 */
#define CAN_MCAN_DT_BUILD_ASSERT_MRAM_CFG(node_id)                                                 \
	BUILD_ASSERT(CAN_MCAN_DT_MRAM_STD_FILTER_ELEMENTS(node_id) <= 128,                         \
		     "Maximum Standard filter elements exceeded");                                 \
	BUILD_ASSERT(CAN_MCAN_DT_MRAM_EXT_FILTER_ELEMENTS(node_id) <= 64,                          \
		     "Maximum Extended filter elements exceeded");                                 \
	BUILD_ASSERT(CAN_MCAN_DT_MRAM_RX_FIFO0_ELEMENTS(node_id) <= 64,                            \
		     "Maximum Rx FIFO 0 elements exceeded");                                       \
	BUILD_ASSERT(CAN_MCAN_DT_MRAM_RX_FIFO1_ELEMENTS(node_id) <= 64,                            \
		     "Maximum Rx FIFO 1 elements exceeded");                                       \
	BUILD_ASSERT(CAN_MCAN_DT_MRAM_RX_BUFFER_ELEMENTS(node_id) <= 64,                           \
		     "Maximum Rx Buffer elements exceeded");                                       \
	BUILD_ASSERT(CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_ELEMENTS(node_id) <= 32,                       \
		     "Maximum Tx Buffer elements exceeded");                                       \
	BUILD_ASSERT(CAN_MCAN_DT_MRAM_TX_BUFFER_ELEMENTS(node_id) <= 32,                           \
		     "Maximum Tx Buffer elements exceeded");

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_OFFSET(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the Message RAM offset in bytes
 * @see CAN_MCAN_DT_MRAM_OFFSET()
 */
#define CAN_MCAN_DT_INST_MRAM_OFFSET(inst) CAN_MCAN_DT_MRAM_OFFSET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_STD_FILTER_ELEMENTS(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the number of standard (11-bit) elements
 * @see CAN_MCAN_DT_MRAM_STD_FILTER_ELEMENTS()
 */
#define CAN_MCAN_DT_INST_MRAM_STD_FILTER_ELEMENTS(inst)                                            \
	CAN_MCAN_DT_MRAM_STD_FILTER_ELEMENTS(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_EXT_FILTER_ELEMENTS(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the number of extended (29-bit) elements
 * @see CAN_MCAN_DT_MRAM_EXT_FILTER_ELEMENTS()
 */
#define CAN_MCAN_DT_INST_MRAM_EXT_FILTER_ELEMENTS(inst)                                            \
	CAN_MCAN_DT_MRAM_EXT_FILTER_ELEMENTS(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_RX_FIFO0_ELEMENTS(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the number of Rx FIFO 0 elements
 * @see CAN_MCAN_DT_MRAM_RX_FIFO0_ELEMENTS()
 */
#define CAN_MCAN_DT_INST_MRAM_RX_FIFO0_ELEMENTS(inst)                                              \
	CAN_MCAN_DT_MRAM_RX_FIFO0_ELEMENTS(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_RX_FIFO1_ELEMENTS(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the number of Rx FIFO 1 elements
 * @see CAN_MCAN_DT_MRAM_RX_FIFO1_ELEMENTS()
 */
#define CAN_MCAN_DT_INST_MRAM_RX_FIFO1_ELEMENTS(inst)                                              \
	CAN_MCAN_DT_MRAM_RX_FIFO1_ELEMENTS(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_RX_BUFFER_ELEMENTS(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the number of Rx Buffer elements
 * @see CAN_MCAN_DT_MRAM_RX_BUFFER_ELEMENTS()
 */
#define CAN_MCAN_DT_INST_MRAM_RX_BUFFER_ELEMENTS(inst)                                             \
	CAN_MCAN_DT_MRAM_RX_BUFFER_ELEMENTS(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_ELEMENTS(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the number of Tx Event FIFO elements
 * @see CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_ELEMENTS()
 */
#define CAN_MCAN_DT_INST_MRAM_TX_EVENT_FIFO_ELEMENTS(inst)                                         \
	CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_ELEMENTS(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_TX_BUFFER_ELEMENTS(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the number of Tx Buffer elements
 * @see CAN_MCAN_DT_MRAM_TX_BUFFER_ELEMENTS()
 */
#define CAN_MCAN_DT_INST_MRAM_TX_BUFFER_ELEMENTS(inst)                                             \
	CAN_MCAN_DT_MRAM_TX_BUFFER_ELEMENTS(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_STD_FILTER_OFFSET(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the base offset of standard (11-bit) filter elements in bytes
 * @see CAN_MCAN_DT_MRAM_STD_FILTER_OFFSET()
 */
#define CAN_MCAN_DT_INST_MRAM_STD_FILTER_OFFSET(inst)                                              \
	CAN_MCAN_DT_MRAM_STD_FILTER_OFFSET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_EXT_FILTER_OFFSET(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the base offset of extended (29-bit) filter elements in bytes
 * @see CAN_MCAN_DT_MRAM_EXT_FILTER_OFFSET()
 */
#define CAN_MCAN_DT_INST_MRAM_EXT_FILTER_OFFSET(inst)                                              \
	CAN_MCAN_DT_MRAM_EXT_FILTER_OFFSET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_RX_FIFO0_OFFSET(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the base offset of Rx FIFO 0 elements in bytes
 * @see CAN_MCAN_DT_MRAM_RX_FIFO0_OFFSET()
 */
#define CAN_MCAN_DT_INST_MRAM_RX_FIFO0_OFFSET(inst)                                                \
	CAN_MCAN_DT_MRAM_RX_FIFO0_OFFSET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_RX_FIFO1_OFFSET(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the base offset of Rx FIFO 1 elements in bytes
 * @see CAN_MCAN_DT_MRAM_RX_FIFO1_OFFSET()
 */
#define CAN_MCAN_DT_INST_MRAM_RX_FIFO1_OFFSET(inst)                                                \
	CAN_MCAN_DT_MRAM_RX_FIFO1_OFFSET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_RX_BUFFER_OFFSET(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the base offset of Rx Buffer elements in bytes
 * @see CAN_MCAN_DT_MRAM_RX_BUFFER_OFFSET()
 */
#define CAN_MCAN_DT_INST_MRAM_RX_BUFFER_OFFSET(inst)                                               \
	CAN_MCAN_DT_MRAM_RX_BUFFER_OFFSET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_OFFSET(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the base offset of Tx Event FIFO elements in bytes
 * @see CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_OFFSET()
 */
#define CAN_MCAN_DT_INST_MRAM_TX_EVENT_FIFO_OFFSET(inst)                                           \
	CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_OFFSET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_TX_BUFFER_OFFSET(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the base offset of Tx Buffer elements in bytes
 * @see CAN_MCAN_DT_MRAM_TX_BUFFER_OFFSET()
 */
#define CAN_MCAN_DT_INST_MRAM_TX_BUFFER_OFFSET(inst)                                               \
	CAN_MCAN_DT_MRAM_TX_BUFFER_OFFSET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MCAN_ADDR(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the Bosch M_CAN register base address
 * @see CAN_MCAN_DT_MRAM_ADDR()
 */
#define CAN_MCAN_DT_INST_MCAN_ADDR(inst) CAN_MCAN_DT_MCAN_ADDR(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRBA(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the Bosch M_CAN Message RAM Base Address (MRBA)
 * @see CAN_MCAN_DT_MRBA()
 */
#define CAN_MCAN_DT_INST_MRBA(inst) CAN_MCAN_DT_MRBA(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_ADDR(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the Bosch M_CAN Message RAM address
 * @see CAN_MCAN_DT_MRAM_ADDR()
 */
#define CAN_MCAN_DT_INST_MRAM_ADDR(inst) CAN_MCAN_DT_MRAM_ADDR(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_SIZE(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the Bosch M_CAN Message RAM size in bytes
 * @see CAN_MCAN_DT_MRAM_SIZE()
 */
#define CAN_MCAN_DT_INST_MRAM_SIZE(inst) CAN_MCAN_DT_MRAM_SIZE(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_ELEMENTS_SIZE(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @return the total size of all Message RAM elements in bytes
 * @see CAN_MCAN_DT_MRAM_ELEMENTS_SIZE()
 */
#define CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(inst) CAN_MCAN_DT_MRAM_ELEMENTS_SIZE(DT_DRV_INST(inst))

/**
 * @brief Equivalent to CAN_MCAN_DT_MRAM_DEFINE(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @param _name buffer variable name
 * @see CAN_MCAN_DT_MRAM_DEFINE()
 */
#define CAN_MCAN_DT_INST_MRAM_DEFINE(inst, _name) CAN_MCAN_DT_MRAM_DEFINE(DT_DRV_INST(inst), _name)

/**
 * @brief Bosch M_CAN specific static initializer for a minimum nominal @p can_timing struct
 */
#define CAN_MCAN_TIMING_MIN_INITIALIZER                                                            \
	{                                                                                          \
		.sjw = 1,                                                                          \
		.prop_seg = 0,                                                                     \
		.phase_seg1 = 2,                                                                   \
		.phase_seg2 = 2,                                                                   \
		.prescaler = 1                                                                     \
	}

/**
 * @brief Bosch M_CAN specific static initializer for a maximum nominal @p can_timing struct
 */
#define CAN_MCAN_TIMING_MAX_INITIALIZER                                                            \
	{                                                                                          \
		.sjw = 128,                                                                        \
		.prop_seg = 0,                                                                     \
		.phase_seg1 = 256,                                                                 \
		.phase_seg2 = 128,                                                                 \
		.prescaler = 512                                                                   \
	}

/**
 * @brief Bosch M_CAN specific static initializer for a minimum data phase @p can_timing struct
 */
#define CAN_MCAN_TIMING_DATA_MIN_INITIALIZER                                                       \
	{                                                                                          \
		.sjw = 1,                                                                          \
		.prop_seg = 0,                                                                     \
		.phase_seg1 = 1,                                                                   \
		.phase_seg2 = 1,                                                                   \
		.prescaler = 1                                                                     \
	}

/**
 * @brief Bosch M_CAN specific static initializer for a maximum data phase @p can_timing struct
 */
#define CAN_MCAN_TIMING_DATA_MAX_INITIALIZER                                                       \
	{                                                                                          \
		.sjw = 16,                                                                         \
		.prop_seg = 0,                                                                     \
		.phase_seg1 = 32,                                                                  \
		.phase_seg2 = 16,                                                                  \
		.prescaler = 32                                                                    \
	}

/**
 * @brief Equivalent to CAN_MCAN_DT_BUILD_ASSERT_MRAM_CFG(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @see CAN_MCAN_DT_BUILD_ASSERT_MRAM_CFG()
 */
#define CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(inst)                                               \
	CAN_MCAN_DT_BUILD_ASSERT_MRAM_CFG(DT_DRV_INST(inst))

/**
 * @brief Bosch M_CAN Rx Buffer and FIFO Element header
 *
 * See Bosch M_CAN Users Manual section 2.4.2 for details.
 */
struct can_mcan_rx_fifo_hdr {
	union {
		struct {
			uint32_t ext_id: 29;
			uint32_t rtr: 1;
			uint32_t xtd: 1;
			uint32_t esi: 1;
		};
		struct {
			uint32_t pad1: 18;
			uint32_t std_id: 11;
			uint32_t pad2: 3;
		};
	};
	uint32_t rxts: 16;
	uint32_t dlc: 4;
	uint32_t brs: 1;
	uint32_t fdf: 1;
	uint32_t res: 2;
	uint32_t fidx: 7;
	uint32_t anmf: 1;
} __packed __aligned(4);

/**
 * @brief Bosch M_CAN Rx Buffer and FIFO Element
 *
 * See Bosch M_CAN Users Manual section 2.4.2 for details.
 */
struct can_mcan_rx_fifo {
	struct can_mcan_rx_fifo_hdr hdr;
	union {
		uint8_t data[64];
		uint32_t data_32[16];
	};
} __packed __aligned(4);

/**
 * @brief Bosch M_CAN Tx Buffer Element header
 *
 * See Bosch M_CAN Users Manual section 2.4.3 for details.
 */
struct can_mcan_tx_buffer_hdr {
	union {
		struct {
			uint32_t ext_id: 29;
			uint32_t rtr: 1;
			uint32_t xtd: 1;
			uint32_t esi: 1;
		};
		struct {
			uint32_t pad1: 18;
			uint32_t std_id: 11;
			uint32_t pad2: 3;
		};
	};
	uint16_t res1;
	uint8_t dlc: 4;
	uint8_t brs: 1;
	uint8_t fdf: 1;
	uint8_t tsce: 1;
	uint8_t efc: 1;
	uint8_t mm;
} __packed __aligned(4);

/**
 * @brief Bosch M_CAN Tx Buffer Element
 *
 * See Bosch M_CAN Users Manual section 2.4.3 for details.
 */
struct can_mcan_tx_buffer {
	struct can_mcan_tx_buffer_hdr hdr;
	union {
		uint8_t data[64];
		uint32_t data_32[16];
	};
} __packed __aligned(4);

/**
 * @brief Bosch M_CAN Tx Event FIFO Element
 *
 * See Bosch M_CAN Users Manual section 2.4.4 for details.
 */
struct can_mcan_tx_event_fifo {
	union {
		struct {
			uint32_t ext_id: 29;
			uint32_t rtr: 1;
			uint32_t xtd: 1;
			uint32_t esi: 1;
		};
		struct {
			uint32_t pad1: 18;
			uint32_t std_id: 11;
			uint32_t pad2: 3;
		};
	};
	uint16_t txts;
	uint8_t dlc: 4;
	uint8_t brs: 1;
	uint8_t fdf: 1;
	uint8_t et: 2;
	uint8_t mm;
} __packed __aligned(4);

/* Bosch M_CAN Standard/Extended Filter Element Configuration (SFEC/EFEC) */
#define CAN_MCAN_XFEC_DISABLE    0x0
#define CAN_MCAN_XFEC_FIFO0      0x1
#define CAN_MCAN_XFEC_FIFO1      0x2
#define CAN_MCAN_XFEC_REJECT     0x3
#define CAN_MCAN_XFEC_PRIO       0x4
#define CAN_MCAN_XFEC_PRIO_FIFO0 0x5
#define CAN_MCAN_XFEC_PRIO_FIFO1 0x7

/* Bosch M_CAN Standard Filter Type (SFT) */
#define CAN_MCAN_SFT_RANGE    0x0
#define CAN_MCAN_SFT_DUAL     0x1
#define CAN_MCAN_SFT_CLASSIC  0x2
#define CAN_MCAN_SFT_DISABLED 0x3

/**
 * @brief Bosch M_CAN Standard Message ID Filter Element
 *
 * See Bosch M_CAN Users Manual section 2.4.5 for details.
 */
struct can_mcan_std_filter {
	uint32_t sfid2: 11;
	uint32_t res: 5;
	uint32_t sfid1: 11;
	uint32_t sfec: 3;
	uint32_t sft: 2;
} __packed __aligned(4);

/* Bosch M_CAN Extended Filter Type (EFT) */
#define CAN_MCAN_EFT_RANGE_XIDAM 0x0
#define CAN_MCAN_EFT_DUAL        0x1
#define CAN_MCAN_EFT_CLASSIC     0x2
#define CAN_MCAN_EFT_RANGE       0x3

/**
 * @brief Bosch M_CAN Extended Message ID Filter Element
 *
 * See Bosch M_CAN Users Manual section 2.4.6 for details.
 */
struct can_mcan_ext_filter {
	uint32_t efid1: 29;
	uint32_t efec: 3;
	uint32_t efid2: 29;
	uint32_t esync: 1;
	uint32_t eft: 2;
} __packed __aligned(4);

/**
 * @brief Bosch M_CAN driver internal data structure.
 */
struct can_mcan_data {
	struct can_driver_data common;
	struct k_mutex lock;
	struct k_sem tx_sem;
	struct k_mutex tx_mtx;
	void *custom;
} __aligned(4);

/**
 * @brief Bosch M_CAN driver front-end callback for reading a register value
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param[out] val Register value
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP Register not supported.
 * @retval -EIO General input/output error.
 */
typedef int (*can_mcan_read_reg_t)(const struct device *dev, uint16_t reg, uint32_t *val);

/**
 * @brief Bosch M_CAN driver front-end callback for writing a register value
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param val Register value
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP Register not supported.
 * @retval -EIO General input/output error.
 */
typedef int (*can_mcan_write_reg_t)(const struct device *dev, uint16_t reg, uint32_t val);

/**
 * @brief Bosch M_CAN driver front-end callback for reading from Message RAM
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param[out] dst Destination for the data read. The destination address must be 32-bit aligned.
 * @param len Number of bytes to read. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
typedef int (*can_mcan_read_mram_t)(const struct device *dev, uint16_t offset, void *dst,
				    size_t len);

/**
 * @brief Bosch M_CAN driver front-end callback for writing to Message RAM
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param src Source for the data to be written. The source address must be 32-bit aligned.
 * @param len Number of bytes to write. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
typedef int (*can_mcan_write_mram_t)(const struct device *dev, uint16_t offset, const void *src,
				     size_t len);

/**
 * @brief Bosch M_CAN driver front-end callback for clearing Message RAM
 *
 * Clear Message RAM by writing 0 to all words.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param len Number of bytes to clear. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
typedef int (*can_mcan_clear_mram_t)(const struct device *dev, uint16_t offset, size_t len);

/**
 * @brief Bosch M_CAN driver front-end operations.
 */
struct can_mcan_ops {
	can_mcan_read_reg_t read_reg;
	can_mcan_write_reg_t write_reg;
	can_mcan_read_mram_t read_mram;
	can_mcan_write_mram_t write_mram;
	can_mcan_clear_mram_t clear_mram;
};

/**
 * @brief Bosch M_CAN driver internal Tx callback structure.
 */
struct can_mcan_tx_callback {
	can_tx_callback_t function;
	void *user_data;
};

/**
 * @brief Bosch M_CAN driver internal Rx callback structure.
 */
struct can_mcan_rx_callback {
	can_rx_callback_t function;
	void *user_data;
};

/**
 * @brief Bosch M_CAN driver internal Tx + Rx callbacks structure.
 */
struct can_mcan_callbacks {
	struct can_mcan_tx_callback *tx;
	struct can_mcan_rx_callback *std;
	struct can_mcan_rx_callback *ext;
	uint8_t num_tx;
	uint8_t num_std;
	uint8_t num_ext;
};

/**
 * @brief Define Bosch M_CAN TX and RX callbacks
 *
 * This macro allows a Bosch M_CAN driver frontend using a fixed Message RAM configuration to limit
 * the required software resources (e.g. limit the number of the standard (11-bit) or extended
 * (29-bit) filters in use).
 *
 * Frontend drivers supporting dynamic Message RAM configuration should use @ref
 * CAN_MCAN_DT_CALLBACKS_DEFINE() or @ref CAN_MCAN_DT_INST_CALLBACKS_DEFINE() instead.
 *
 * @param _name callbacks variable name
 * @param _tx Number of Tx callbacks
 * @param _std Number of standard (11-bit) filter callbacks
 * @param _ext Number of extended (29-bit) filter callbacks
 * @see CAN_MCAN_DT_CALLBACKS_DEFINE()
 */
#define CAN_MCAN_CALLBACKS_DEFINE(_name, _tx, _std, _ext)                                          \
	static struct can_mcan_tx_callback _name##_tx_cbs[_tx];                                    \
	static struct can_mcan_rx_callback _name##_std_cbs[_std];                                  \
	static struct can_mcan_rx_callback _name##_ext_cbs[_ext];                                  \
	static const struct can_mcan_callbacks _name = {                                           \
		.tx = _name##_tx_cbs,                                                              \
		.std = _name##_std_cbs,                                                            \
		.ext = _name##_ext_cbs,                                                            \
		.num_tx = _tx,                                                                     \
		.num_std = _std,                                                                   \
		.num_ext = _ext,                                                                   \
	}

/**
 * @brief Define Bosch M_CAN TX and RX callbacks
 * @param node_id node identifier
 * @param _name callbacks variable name
 * @see CAN_MCAN_CALLBACKS_DEFINE()
 */
#define CAN_MCAN_DT_CALLBACKS_DEFINE(node_id, _name)                                               \
	CAN_MCAN_CALLBACKS_DEFINE(_name, CAN_MCAN_DT_MRAM_TX_BUFFER_ELEMENTS(node_id),             \
				  CAN_MCAN_DT_MRAM_STD_FILTER_ELEMENTS(node_id),                   \
				  CAN_MCAN_DT_MRAM_EXT_FILTER_ELEMENTS(node_id))

/**
 * @brief Equivalent to CAN_MCAN_DT_CALLBACKS_DEFINE(DT_DRV_INST(inst))
 * @param inst DT_DRV_COMPAT instance number
 * @param _name callbacks variable name
 * @see CAN_MCAN_DT_CALLBACKS_DEFINE()
 */
#define CAN_MCAN_DT_INST_CALLBACKS_DEFINE(inst, _name)                                             \
	CAN_MCAN_DT_CALLBACKS_DEFINE(DT_DRV_INST(inst), _name)

/**
 * @brief Bosch M_CAN driver internal configuration structure.
 */
struct can_mcan_config {
	const struct can_driver_config common;
	const struct can_mcan_ops *ops;
	const struct can_mcan_callbacks *callbacks;
	uint16_t mram_elements[CAN_MCAN_MRAM_CFG_NUM_CELLS];
	uint16_t mram_offsets[CAN_MCAN_MRAM_CFG_NUM_CELLS];
	size_t mram_size;
	const void *custom;
};

/**
 * @brief Get an array containing the number of elements in Bosch M_CAN Message RAM
 *
 * The order of the array entries is given by the @ref CAN_MCAN_MRAM_CFG definitions.
 *
 * @param node_id node identifier
 * @return array of number of elements
 */
#define CAN_MCAN_DT_MRAM_ELEMENTS_GET(node_id)                                                     \
	{                                                                                          \
		0, /* offset cell */                                                               \
			CAN_MCAN_DT_MRAM_STD_FILTER_ELEMENTS(node_id),                             \
			CAN_MCAN_DT_MRAM_EXT_FILTER_ELEMENTS(node_id),                             \
			CAN_MCAN_DT_MRAM_RX_FIFO0_ELEMENTS(node_id),                               \
			CAN_MCAN_DT_MRAM_RX_FIFO1_ELEMENTS(node_id),                               \
			CAN_MCAN_DT_MRAM_RX_BUFFER_ELEMENTS(node_id),                              \
			CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_ELEMENTS(node_id),                          \
			CAN_MCAN_DT_MRAM_TX_BUFFER_ELEMENTS(node_id)                               \
	}

/**
 * @brief Get an array containing the base offsets for element in Bosch M_CAN Message RAM
 *
 * The order of the array entries is given by the @ref CAN_MCAN_MRAM_CFG definitions.
 *
 * @param node_id node identifier
 * @return array of base offsets for elements
 */
#define CAN_MCAN_DT_MRAM_OFFSETS_GET(node_id)                                                      \
	{                                                                                          \
		0, /* offset cell */                                                               \
			CAN_MCAN_DT_MRAM_STD_FILTER_OFFSET(node_id),                               \
			CAN_MCAN_DT_MRAM_EXT_FILTER_OFFSET(node_id),                               \
			CAN_MCAN_DT_MRAM_RX_FIFO0_OFFSET(node_id),                                 \
			CAN_MCAN_DT_MRAM_RX_FIFO1_OFFSET(node_id),                                 \
			CAN_MCAN_DT_MRAM_RX_BUFFER_OFFSET(node_id),                                \
			CAN_MCAN_DT_MRAM_TX_EVENT_FIFO_OFFSET(node_id),                            \
			CAN_MCAN_DT_MRAM_TX_BUFFER_OFFSET(node_id)                                 \
	}

/**
 * @brief Static initializer for @p can_mcan_config struct
 *
 * @param node_id Devicetree node identifier
 * @param _custom Pointer to custom driver frontend configuration structure
 * @param _ops Pointer to front-end @a can_mcan_ops
 * @param _cbs Pointer to front-end @a can_mcan_callbacks
 */
#ifdef CONFIG_CAN_FD_MODE
#define CAN_MCAN_DT_CONFIG_GET(node_id, _custom, _ops, _cbs)                                       \
	{                                                                                          \
		.common = CAN_DT_DRIVER_CONFIG_GET(node_id, 0, 8000000),                           \
		.ops = _ops,                                                                       \
		.callbacks = _cbs,                                                                 \
		.mram_elements = CAN_MCAN_DT_MRAM_ELEMENTS_GET(node_id),                           \
		.mram_offsets = CAN_MCAN_DT_MRAM_OFFSETS_GET(node_id),                             \
		.mram_size = CAN_MCAN_DT_MRAM_ELEMENTS_SIZE(node_id),                              \
<		.custom = _custom,                                                                 \
	}
#else /* CONFIG_CAN_FD_MODE */
#define CAN_MCAN_DT_CONFIG_GET(node_id, _custom, _ops, _cbs)                                       \
	{                                                                                          \
		.common = CAN_DT_DRIVER_CONFIG_GET(node_id, 0, 1000000),                           \
		.ops = _ops,                                                                       \
		.callbacks = _cbs,                                                                 \
		.mram_elements = CAN_MCAN_DT_MRAM_ELEMENTS_GET(node_id),                           \
		.mram_offsets = CAN_MCAN_DT_MRAM_OFFSETS_GET(node_id),                             \
		.mram_size = CAN_MCAN_DT_MRAM_ELEMENTS_SIZE(node_id),                              \
		.custom = _custom,                                                                 \
	}
#endif /* !CONFIG_CAN_FD_MODE */

/**
 * @brief Static initializer for @p can_mcan_config struct from DT_DRV_COMPAT instance
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param _custom Pointer to custom driver frontend configuration structure
 * @param _ops Pointer to front-end @a can_mcan_ops
 * @param _cbs Pointer to front-end @a can_mcan_callbacks
 * @see CAN_MCAN_DT_CONFIG_GET()
 */
#define CAN_MCAN_DT_CONFIG_INST_GET(inst, _custom, _ops, _cbs)                                     \
	CAN_MCAN_DT_CONFIG_GET(DT_DRV_INST(inst), _custom, _ops, _cbs)

/**
 * @brief Initializer for a @a can_mcan_data struct
 * @param _custom Pointer to custom driver frontend data structure
 */
#define CAN_MCAN_DATA_INITIALIZER(_custom)                                                         \
	{                                                                                          \
		.custom = _custom,                                                                 \
	}

/**
 * @brief Bosch M_CAN driver front-end callback helper for reading a memory mapped register
 *
 * @param base Register base address
 * @param reg Register offset
 * @param[out] val Register value
 *
 * @retval 0 Memory mapped register read always succeeds.
 */
static inline int can_mcan_sys_read_reg(mm_reg_t base, uint16_t reg, uint32_t *val)
{
	*val = sys_read32(base + reg);

	return 0;
}

/**
 * @brief Bosch M_CAN driver front-end callback helper for writing a memory mapped register
 *
 * @param base Register base address
 * @param reg Register offset
 * @param val Register value
 *
 * @retval 0 Memory mapped register write always succeeds.
 */
static inline int can_mcan_sys_write_reg(mm_reg_t base, uint16_t reg, uint32_t val)
{
	sys_write32(val, base + reg);

	return 0;
}

/**
 * @brief Bosch M_CAN driver front-end callback helper for reading from memory mapped Message RAM
 *
 * @param base Base address of the Message RAM for the given Bosch M_CAN instance. The base address
 *        must be 32-bit aligned.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param[out] dst Destination for the data read. The destination address must be 32-bit aligned.
 * @param len Number of bytes to read. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
static inline int can_mcan_sys_read_mram(mem_addr_t base, uint16_t offset, void *dst, size_t len)
{
	volatile uint32_t *src32 = (volatile uint32_t *)(base + offset);
	uint32_t *dst32 = (uint32_t *)dst;
	size_t len32 = len / sizeof(uint32_t);

	__ASSERT(base % 4U == 0U, "base must be a multiple of 4");
	__ASSERT(offset % 4U == 0U, "offset must be a multiple of 4");
	__ASSERT(POINTER_TO_UINT(dst) % 4U == 0U, "dst must be 32-bit aligned");
	__ASSERT(len % 4U == 0U, "len must be a multiple of 4");

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	int err;

	err = sys_cache_data_invd_range((void *)(base + offset), len);
	if (err != 0) {
		return err;
	}
#endif /* !defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE) */

	while (len32-- > 0) {
		*dst32++ = *src32++;
	}

	return 0;
}

/**
 * @brief Bosch M_CAN driver front-end callback helper for writing to memory mapped Message RAM
 *
 * @param base Base address of the Message RAM for the given Bosch M_CAN instance. The base address
 *        must be 32-bit aligned.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param src Source for the data to be written. The source address must be 32-bit aligned.
 * @param len Number of bytes to write. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
static inline int can_mcan_sys_write_mram(mem_addr_t base, uint16_t offset, const void *src,
					  size_t len)
{
	volatile uint32_t *dst32 = (volatile uint32_t *)(base + offset);
	const uint32_t *src32 = (const uint32_t *)src;
	size_t len32 = len / sizeof(uint32_t);

	__ASSERT(base % 4U == 0U, "base must be a multiple of 4");
	__ASSERT(offset % 4U == 0U, "offset must be a multiple of 4");
	__ASSERT(POINTER_TO_UINT(src) % 4U == 0U, "src must be 32-bit aligned");
	__ASSERT(len % 4U == 0U, "len must be a multiple of 4");

	while (len32-- > 0) {
		*dst32++ = *src32++;
	}

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return sys_cache_data_flush_range((void *)(base + offset), len);
#else /* defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE) */
	return 0;
#endif /* !defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE) */
}

/**
 * @brief Bosch M_CAN driver front-end callback helper for clearing memory mapped Message RAM
 *
 * Clear Message RAM by writing 0 to all words.
 *
 * @param base Base address of the Message RAM for the given Bosch M_CAN instance. The base address
 *        must be 32-bit aligned.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param len Number of bytes to clear. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
static inline int can_mcan_sys_clear_mram(mem_addr_t base, uint16_t offset, size_t len)
{
	volatile uint32_t *dst32 = (volatile uint32_t *)(base + offset);
	size_t len32 = len / sizeof(uint32_t);

	__ASSERT(base % 4U == 0U, "base must be a multiple of 4");
	__ASSERT(offset % 4U == 0U, "offset must be a multiple of 4");
	__ASSERT(len % 4U == 0U, "len must be a multiple of 4");

	while (len32-- > 0) {
		*dst32++ = 0U;
	}

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return sys_cache_data_flush_range((void *)(base + offset), len);
#else /* defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE) */
	return 0;
#endif /* !defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE) */
}

/**
 * @brief Read a Bosch M_CAN register
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param[out] val Register value
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP Register not supported.
 * @retval -EIO General input/output error.
 */
int can_mcan_read_reg(const struct device *dev, uint16_t reg, uint32_t *val);

/**
 * @brief Write a Bosch M_CAN register
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param val Register value
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP Register not supported.
 * @retval -EIO General input/output error.
 */
int can_mcan_write_reg(const struct device *dev, uint16_t reg, uint32_t val);

/**
 * @brief Read from Bosch M_CAN Message RAM
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param[out] dst Destination for the data read.
 * @param len Number of bytes to read. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
static inline int can_mcan_read_mram(const struct device *dev, uint16_t offset, void *dst,
				     size_t len)
{
	const struct can_mcan_config *config = dev->config;

	return config->ops->read_mram(dev, offset, dst, len);
}

/**
 * @brief Write to Bosch M_CAN Message RAM
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param src Source for the data to be written
 * @param len Number of bytes to write. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
static inline int can_mcan_write_mram(const struct device *dev, uint16_t offset, const void *src,
				      size_t len)
{
	const struct can_mcan_config *config = dev->config;

	return config->ops->write_mram(dev, offset, src, len);
}

/**
 * @brief Clear Bosch M_CAN Message RAM
 *
 * Clear Message RAM by writing 0 to all words.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Offset from the start of the Message RAM for the given Bosch M_CAN instance. The
 *        offset must be 32-bit aligned.
 * @param len Number of bytes to clear. Must be a multiple of 4.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
static inline int can_mcan_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *config = dev->config;

	return config->ops->clear_mram(dev, offset, len);
}

/**
 * @brief Configure Bosch M_MCAN Message RAM start addresses.
 *
 * Bosch M_CAN driver front-end callback helper function for configuring the start addresses of the
 * Bosch M_CAN Rx FIFO0 (RXFOC), Rx FIFO1 (RXF1C), Rx Buffer (RXBCC), Tx Buffer (TXBC), and Tx Event
 * FIFO (TXEFC) in Message RAM.
 *
 * The start addresses (containing bits 15:2 since Bosch M_CAN message RAM is accessed as 32 bit
 * words) are calculated relative to the provided Message RAM Base Address (mrba).
 *
 * Some Bosch M_CAN implementations use a fixed Message RAM configuration, other use a fixed memory
 * area and relative addressing, others again have custom registers for configuring the Message
 * RAM. It is the responsibility of the front-end driver to call this function during driver
 * initialization as needed.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mrba Message RAM Base Address.
 * @param mram Message RAM Address.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error.
 */
int can_mcan_configure_mram(const struct device *dev, uintptr_t mrba, uintptr_t mram);

/**
 * @brief Bosch M_CAN driver initialization callback.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
int can_mcan_init(const struct device *dev);

/**
 * @brief Bosch M_CAN driver m_can_int0 IRQ handler.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void can_mcan_line_0_isr(const struct device *dev);

/**
 * @brief Bosch M_CAN driver m_can_int1 IRQ handler.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void can_mcan_line_1_isr(const struct device *dev);

/**
 * @brief Enable Bosch M_CAN configuration change.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void can_mcan_enable_configuration_change(const struct device *dev);

/**
 * @brief Bosch M_CAN driver callback API upon getting CAN controller capabilities
 * See @a can_get_capabilities() for argument description
 */
int can_mcan_get_capabilities(const struct device *dev, can_mode_t *cap);

/**
 * @brief Bosch M_CAN driver callback API upon starting CAN controller
 * See @a can_start() for argument description
 */
int can_mcan_start(const struct device *dev);

/**
 * @brief Bosch M_CAN driver callback API upon stopping CAN controller
 * See @a can_stop() for argument description
 */
int can_mcan_stop(const struct device *dev);

/**
 * @brief Bosch M_CAN driver callback API upon setting CAN controller mode
 * See @a can_set_mode() for argument description
 */
int can_mcan_set_mode(const struct device *dev, can_mode_t mode);

/**
 * @brief Bosch M_CAN driver callback API upon setting CAN bus timing
 * See @a can_set_timing() for argument description
 */
int can_mcan_set_timing(const struct device *dev, const struct can_timing *timing);

/**
 * @brief Bosch M_CAN driver callback API upon setting CAN bus data phase timing
 * See @a can_set_timing_data() for argument description
 */
int can_mcan_set_timing_data(const struct device *dev, const struct can_timing *timing_data);

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
/**
 * @brief Bosch M_CAN driver callback API upon recovering the CAN bus
 * See @a can_recover() for argument description
 */
int can_mcan_recover(const struct device *dev, k_timeout_t timeout);
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

int can_mcan_send(const struct device *dev, const struct can_frame *frame, k_timeout_t timeout,
		  can_tx_callback_t callback, void *user_data);

int can_mcan_get_max_filters(const struct device *dev, bool ide);

/**
 * @brief Bosch M_CAN driver callback API upon adding an RX filter
 * See @a can_add_rx_callback() for argument description
 */
int can_mcan_add_rx_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			   const struct can_filter *filter);

/**
 * @brief Bosch M_CAN driver callback API upon removing an RX filter
 * See @a can_remove_rx_filter() for argument description
 */
void can_mcan_remove_rx_filter(const struct device *dev, int filter_id);

/**
 * @brief Bosch M_CAN driver callback API upon getting the CAN controller state
 * See @a can_get_state() for argument description
 */
int can_mcan_get_state(const struct device *dev, enum can_state *state,
		       struct can_bus_err_cnt *err_cnt);

/**
 * @brief Bosch M_CAN driver callback API upon setting a state change callback
 * See @a can_set_state_change_callback() for argument description
 */
void can_mcan_set_state_change_callback(const struct device *dev,
					can_state_change_callback_t callback, void *user_data);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_MCAN_H_ */
