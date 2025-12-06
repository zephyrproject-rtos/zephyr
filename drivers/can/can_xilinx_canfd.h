/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CAN_XILINX_CANFD_H_
#define ZEPHYR_DRIVERS_CAN_XILINX_CANFD_H_

#include <zephyr/drivers/can.h>
#include <zephyr/sys/util.h>

#define XLNX_CANFD_BUS_SPEED_MIN	1000
#define XLNX_CANFD_BUS_SPEED_MAX	8000000

#define XCANFD_SRR_OFFSET			0x00 /* Software reset register */
#define XCANFD_MSR_OFFSET			0x04 /* Mode select register */
#define XCANFD_BRPR_OFFSET			0x08 /* Baud rate prescaler register */
#define XCANFD_BTR_OFFSET			0x0C /* Bit timing register */
#define XCANFD_ECR_OFFSET			0x10 /* Error counter register */
#define XCANFD_ESR_OFFSET			0x14 /* Error status register */
#define XCANFD_SR_OFFSET			0x18 /* Status register */
#define XCANFD_ISR_OFFSET			0x1C /* Interrupt status register */
#define XCANFD_IER_OFFSET			0x20 /* Interrupt enable register */
#define XCANFD_ICR_OFFSET			0x24 /* Interrupt clear register */
#define XCANFD_F_BRPR_OFFSET		0x088 /* Data Phase Baud Rate Prescaler register */
#define XCANFD_F_BTR_OFFSET			0x08C /* Data Phase Bit Timing register */
#define XCANFD_TRR_OFFSET			0x0090 /* TX Buffer Ready Request register */
#define XCANFD_FSR_OFFSET			0x00E8 /* RX FIFO Status register */
#define XCANFD_AFR_OFFSET			0x00E0 /* Acceptance Filter register */
#define XCANFD_TXMSG_BASE_OFFSET	0x0100 /* TX Message Space register */
#define XCANFD_RXMSG_BASE_OFFSET	0x1100 /* RX Message Space register */
#define XCANFD_RXMSG_2_BASE_OFFSET	0x2100 /* RX Message Space register */

#define XCANFD_FRAME_ID_ADDR(frame_base)	((frame_base) + 0x00)
#define XCANFD_FRAME_DLC_ADDR(frame_base)	((frame_base) + 0x04)
#define XCANFD_FRAME_DW1_ADDR(frame_base)	((frame_base) + 0x08)
#define XCANFD_FRAME_DW2_ADDR(frame_base)	((frame_base) + 0x0C)
#define XCANFD_FRAME_DW_ADDR(frame_base)	((frame_base) + 0x08)

#define XCANFD_CANFD_FRAME_SIZE		0x48
#define XCANFD_TXMSG_FRAME_ADDR(n)	(XCANFD_TXMSG_BASE_OFFSET + \
		XCANFD_CANFD_FRAME_SIZE * (n))
#define XCANFD_RXMSG_FRAME_ADDR(n)	(XCANFD_RXMSG_BASE_OFFSET + \
		XCANFD_CANFD_FRAME_SIZE * (n))
#define XCANFD_RXMSG_2_FRAME_ADDR(n)	(XCANFD_RXMSG_2_BASE_OFFSET + \
		XCANFD_CANFD_FRAME_SIZE * (n))

/* TX mailbox definitions for this driver on CAN FD HW */
#define XCANFD_MAX_TX_MAILBOXES		32

#define XCANFD_SRR_CEN_MASK			BIT(1) /* CAN enable mask */
#define XCANFD_SRR_RESET_MASK			BIT(0) /* Soft Reset mask */
#define XCANFD_MSR_LBACK_MASK			BIT(1) /* Loop back mode mask */
#define XCANFD_MSR_SLEEP_MASK			BIT(0) /* Sleep mode mask */
#define XCANFD_BRPR_BRP_MASK			GENMASK(7, 0) /* Baud rate prescaler mask */
#define XCANFD_BTR_SJW_MASK			GENMASK(8, 7) /* Synchronous jump width mask */
#define XCANFD_BTR_TS2_MASK			GENMASK(6, 4) /* Time segment 2 mask */
#define XCANFD_BTR_TS1_MASK			GENMASK(3, 0) /* Time segment 1 mask */
#define XCANFD_BTR_SJW_MASK_CANFD		GENMASK(19, 16) /* Synchronous jump width mask */
#define XCANFD_BTR_TS2_MASK_CANFD		GENMASK(11, 8) /* Time segment 2 mask */
#define XCANFD_BTR_TS1_MASK_CANFD		GENMASK(5, 0) /* Time segment 1 mask */
#define XCANFD_ECR_REC_MASK			GENMASK(15, 8) /* Receive error counter mask */
#define XCANFD_ECR_TEC_MASK			GENMASK(7, 0) /* Transmit error counter mask */
#define XCANFD_SR_CONFIG_MASK		BIT(0) /* Configuration mode mask */

/* Additional MSR register bits for mode configuration */
#define XCANFD_MSR_SNOOP_MASK		BIT(2) /* Snoop mode (listen-only) */

#define XCANFD_BRPR_TDC_ENABLE_MASK	BIT(16) /* TDC Enable for CAN-FD mask */
#define XCANFD_IXR_TXFEMP_MASK		BIT(14) /* TX FIFO Empty mask */
#define XCANFD_IXR_WKUP_MASK		BIT(11) /* Wake up interrupt mask */
#define XCANFD_IXR_SLP_MASK			BIT(10) /* Sleep interrupt mask */
#define XCANFD_IXR_BSOFF_MASK		BIT(9) /* Bus off interrupt mask */
#define XCANFD_IXR_ERROR_MASK		BIT(8) /* Error interrupt mask */
#define XCANFD_IXR_RXNEMP_MASK		BIT(7) /* RX FIFO NotEmpty mask */
#define XCANFD_IXR_RXOK_MASK		BIT(4) /* Message received mask */
#define XCANFD_IXR_TXOK_MASK		BIT(1) /* TX successful mask */
#define XCANFD_IXR_ARBLST_MASK		BIT(0) /* Arbitration lost mask */

#define XCANFD_IDR_ID1_MASK			GENMASK(31, 21) /* Standard msg identifier mask */
#define XCANFD_IDR_SRR_MASK			BIT(20) /* Substitute remote TXreq mask */
#define XCANFD_IDR_IDE_MASK			BIT(19) /* Identifier extension mask */
#define XCANFD_IDR_ID2_MASK			GENMASK(18, 1) /* Extended message ident mask */
#define XCANFD_IDR_RTR_MASK			BIT(0) /* Remote TX request mask */
#define XCANFD_DLCR_DLC_MASK		GENMASK(31, 28) /* Data length code mask */
#define XCANFD_2_FSR_FL_MASK		GENMASK(14, 8) /* RX Fill Level mask */
#define XCANFD_FSR_IRI_MASK			BIT(7) /* RX Increment Read Index mask */
#define XCANFD_2_FSR_RI_MASK		GENMASK(5, 0) /* RX Read Index mask */
#define XCANFD_DLCR_EDL_MASK		BIT(27) /* EDL mask in DLC */
#define XCANFD_DLCR_BRS_MASK		BIT(26) /* BRS mask in DLC */
#define XCANFD_DLCR_ESI_MASK		BIT(25) /* ESI mask in DLC */

/* CAN register bit shift - XCANFD_<REG>_<BIT>_SHIFT */
#define XCANFD_BTR_SJW_SHIFT		7 /* Synchronous jump width */
#define XCANFD_BTR_TS2_SHIFT		4 /* Time segment 2 */
#define XCANFD_BTR_SJW_SHIFT_CANFD	16 /* Synchronous jump width */
#define XCANFD_BTR_TS2_SHIFT_CANFD	8 /* Time segment 2 */
#define XCANFD_IDR_ID1_SHIFT		21 /* Standard Messg Identifier */
#define XCANFD_IDR_ID2_SHIFT		1 /* Extended Message Identifier */
#define XCANFD_DLCR_DLC_SHIFT		28 /* Data length code */
#define XCANFD_ECR_REC_SHIFT		8 /* Rx Error Count */

#define XCANFD_AFMR_ADDR(n)		(0xA00U + ((n) * 0x8U))  /* Acceptance Filter Mask */
#define XCANFD_AFIDR_ADDR(n)		(0xA04U + ((n) * 0x8U))  /* Acceptance Filter ID Register */
#define XCANFD_MAX_FILTERS		32U
#define MIN_FILTER_INDEX		1U
#define MAX_FILTER_INDEX		32U

/* Mask Register (AF_FMSK) bit fields */
#define XCANFD_AFR_UAF_ALL_MASK		GENMASK(31, 0) /* All acceptance filters */
#define XCANFD_AFMR_AMID_SHIFT		21U	/* Standard Message ID Mask (31:21) */
#define XCANFD_AFMR_AMID_MASK		GENMASK(31, 21)
#define XCANFD_AFMR_AMSRR_SHIFT		20U	/* Substitute RTR Mask (20) */
#define XCANFD_AFMR_AMSRR_MASK		BIT(20)
#define XCANFD_AFMR_AMIDE_MASK		BIT(19)
#define XCANFD_AFMR_AMID_EXT_SHIFT	1U	/* Extended Message ID Mask (18:1) */
#define XCANFD_AFMR_AMID_EXT_MASK	GENMASK(18, 1)
#define XCANFD_AFMR_AMRTR_MASK		BIT(0)

/* ID Register (AF_FID) bit fields */
#define XCANFD_AFIDR_AIID_SHIFT		21U	/* Standard Message ID (31:21) */
#define XCANFD_AFIDR_AIID_MASK		GENMASK(31, 21)
#define XCANFD_AFIDR_AIIDE_MASK		BIT(19)
#define XCANFD_AFIDR_AIID_EXT_SHIFT	1U	/* Extended Message ID (18:1) */
#define XCANFD_AFIDR_AIID_EXT_MASK	GENMASK(18, 1)
#define XCANFD_AFIDR_AIRTR_MASK		BIT(0)

/* CAN frame length constants */
#define XCANFD_DW_BYTES				4
#define XCANFD_TIMING_SJW_MIN		0x1
#define XCANFD_TIMING_PROP_SEG_MIN	0x00
#define XCANFD_TIMING_PHASE_SEG1_MIN	0x04
#define XCANFD_TIMING_PHASE_SEG2_MIN	0x02
#define XCANFD_TIMING_PRESCALER_MIN	0x01

#define XCANFD_TIMING_SJW_MAX		0x4
#define XCANFD_TIMING_PROP_SEG_MAX	0x00
#define XCANFD_TIMING_PHASE_SEG1_MAX	0x10
#define XCANFD_TIMING_PHASE_SEG2_MAX	0x08
#define XCANFD_TIMING_PRESCALER_MAX	0x400

#endif /* ZEPHYR_DRIVERS_CAN_XILINX_CANFD_H_ */
