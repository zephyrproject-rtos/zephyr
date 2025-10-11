/**
 * @file can_xilinx_canfd.h
 * @brief Xilinx CAN-FD driver header
 */

#ifndef __XLNX_CANFD_H__
#define __XLNX_CANFD_H__

#include <zephyr/drivers/can.h>
#include <zephyr/sys/util.h>

#define XLNX_CANFD_BUS_SPEED_MIN	1000
#define XLNX_CANFD_BUS_SPEED_MAX	8000000

#define XCANFD_SRR_OFFSET		0x00 /* Software reset register */
#define XCANFD_MSR_OFFSET		0x04 /* Mode select register */
#define XCANFD_BRPR_OFFSET		0x08 /* Baud rate prescaler register */
#define XCANFD_BTR_OFFSET		0x0C /* Bit timing register */
#define XCANFD_ECR_OFFSET		0x10 /* Error counter register */
#define XCANFD_ESR_OFFSET		0x14 /* Error status register */
#define XCANFD_SR_OFFSET		0x18 /* Status register */
#define XCANFD_ISR_OFFSET		0x1C /* Interrupt status register */
#define XCANFD_IER_OFFSET		0x20 /* Interrupt enable register */
#define XCANFD_ICR_OFFSET		0x24 /* Interrupt clear register */
#define XCANFD_F_BRPR_OFFSET		0x088 /* Data Phase Baud Rate Prescaler register */
#define XCANFD_F_BTR_OFFSET		0x08C /* Data Phase Bit Timing register */
#define XCANFD_TRR_OFFSET		0x0090 /* TX Buffer Ready Request register */
#define XCANFD_FSR_OFFSET		0x00E8 /* RX FIFO Status register */
#define XCANFD_AFR_OFFSET		0x00E0 /* Acceptance Filter register */
#define XCANFD_TXMSG_BASE_OFFSET	0x0100 /* TX Message Space register */
#define XCANFD_RXMSG_BASE_OFFSET	0x1100 /* RX Message Space register */
#define XCANFD_RXMSG_2_BASE_OFFSET	0x2100 /* RX Message Space register */

#define XCANFD_FRAME_ID_OFFSET(frame_base)	((frame_base) + 0x00)
#define XCANFD_FRAME_DLC_OFFSET(frame_base)	((frame_base) + 0x04)
#define XCANFD_FRAME_DW1_OFFSET(frame_base)	((frame_base) + 0x08)
#define XCANFD_FRAME_DW2_OFFSET(frame_base)	((frame_base) + 0x0C)
#define XCANFD_FRAME_DW_OFFSET(frame_base)	((frame_base) + 0x08)

#define XCANFD_CANFD_FRAME_SIZE		0x48
#define XCANFD_TXMSG_FRAME_OFFSET(n)	(XCANFD_TXMSG_BASE_OFFSET + \
		XCANFD_CANFD_FRAME_SIZE * (n))
#define XCANFD_RXMSG_FRAME_OFFSET(n)	(XCANFD_RXMSG_BASE_OFFSET + \
		XCANFD_CANFD_FRAME_SIZE * (n))
#define XCANFD_RXMSG_2_FRAME_OFFSET(n)	(XCANFD_RXMSG_2_BASE_OFFSET + \
		XCANFD_CANFD_FRAME_SIZE * (n))

/* the single TX mailbox used by this driver on CAN FD HW */
#define XCANFD_TX_MAILBOX_IDX		0

#define XCANFD_SRR_CEN_MASK		0x00000002 /* CAN enable mask */
#define XCANFD_SRR_RESET_MASK		0x00000001 /* Soft Reset mask */
#define XCANFD_MSR_LBACK_MASK		0x00000002 /* Loop back mode mask */
#define XCANFD_MSR_SLEEP_MASK		0x00000001 /* Sleep mode mask */
#define XCANFD_BRPR_BRP_MASK		0x000000FF /* Baud rate prescaler mask */
#define XCANFD_BRPR_TDCO_MASK		GENMASK(12, 8) /* TDCO mask */
#define XCANFD_2_BRPR_TDCO_MASK		GENMASK(13, 8) /* TDCO for CANFD 2.0 mask */
#define XCANFD_BTR_SJW_MASK		0x00000180 /* Synchronous jump width mask */
#define XCANFD_BTR_TS2_MASK		0x00000070 /* Time segment 2 mask */
#define XCANFD_BTR_TS1_MASK		0x0000000F /* Time segment 1 mask */
#define XCANFD_BTR_SJW_MASK_CANFD	0x000F0000 /* Synchronous jump width mask */
#define XCANFD_BTR_TS2_MASK_CANFD	0x00000F00 /* Time segment 2 mask */
#define XCANFD_BTR_TS1_MASK_CANFD	0x0000003F /* Time segment 1 mask */
#define XCANFD_ECR_REC_MASK		0x0000FF00 /* Receive error counter mask */
#define XCANFD_ECR_TEC_MASK		0x000000FF /* Transmit error counter mask */
#define XCANFD_ESR_ACKER_MASK		0x00000010 /* ACK error mask */
#define XCANFD_ESR_BERR_MASK		0x00000008 /* Bit error mask */
#define XCANFD_ESR_STER_MASK		0x00000004 /* Stuff error mask */
#define XCANFD_ESR_FMER_MASK		0x00000002 /* Form error mask */
#define XCANFD_ESR_CRCER_MASK		0x00000001 /* CRC error mask */
#define XCANFD_SR_TDCV_MASK		GENMASK(22, 16) /* TDCV Value mask */
#define XCANFD_SR_TXFLL_MASK		0x00000400 /* TX FIFO is full mask */
#define XCANFD_SR_ESTAT_MASK		0x00000180 /* Error status mask */
#define XCANFD_SR_ERRWRN_MASK		0x00000040 /* Error warning mask */
#define XCANFD_SR_NORMAL_MASK		0x00000008 /* Normal mode mask */
#define XCANFD_SR_LBACK_MASK		0x00000002 /* Loop back mode mask */
#define XCANFD_SR_CONFIG_MASK		0x00000001 /* Configuration mode mask */

/* Additional MSR register bits for mode configuration */
#define XCANFD_MSR_SNOOP_MASK		0x00000004 /* Snoop mode (listen-only) */

#define XCANFD_BRPR_TDC_ENABLE_MASK	0x00010000 /* TDC Enable for CAN-FD mask */
#define XCANFD_IXR_RXMNF_MASK		0x00020000 /* RX match not finished mask */
#define XCANFD_IXR_TXFEMP_MASK		0x00004000 /* TX FIFO Empty mask */
#define XCANFD_IXR_WKUP_MASK		0x00000800 /* Wake up interrupt mask */
#define XCANFD_IXR_SLP_MASK		0x00000400 /* Sleep interrupt mask */
#define XCANFD_IXR_BSOFF_MASK		0x00000200 /* Bus off interrupt mask */
#define XCANFD_IXR_ERROR_MASK		0x00000100 /* Error interrupt mask */
#define XCANFD_IXR_RXNEMP_MASK		0x00000080 /* RX FIFO NotEmpty mask */
#define XCANFD_IXR_RXOFLW_MASK		0x00000040 /* RX FIFO Overflow mask */
#define XCANFD_IXR_RXOK_MASK		0x00000010 /* Message received mask */
#define XCANFD_IXR_TXFLL_MASK		0x00000004 /* Tx FIFO Full mask */
#define XCANFD_IXR_TXOK_MASK		0x00000002 /* TX successful mask */
#define XCANFD_IXR_ARBLST_MASK		0x00000001 /* Arbitration lost mask */
#define XCANFD_IXR_E2BERX_MASK		BIT(23) /* RX FIFO two bit ECC error mask */
#define XCANFD_IXR_E1BERX_MASK		BIT(22) /* RX FIFO one bit ECC error mask */
#define XCANFD_IXR_E2BETXOL_MASK	BIT(21) /* TXOL FIFO two bit ECC error mask */
#define XCANFD_IXR_E1BETXOL_MASK	BIT(20) /* TXOL FIFO One bit ECC error mask */
#define XCANFD_IXR_E2BETXTL_MASK	BIT(19) /* TXTL FIFO Two bit ECC error mask */
#define XCANFD_IXR_E1BETXTL_MASK	BIT(18) /* TXTL FIFO One bit ECC error mask */
#define XCANFD_IXR_ECC_MASK		(XCANFD_IXR_E2BERX_MASK | \
		XCANFD_IXR_E1BERX_MASK | \
		XCANFD_IXR_E2BETXOL_MASK | \
		XCANFD_IXR_E1BETXOL_MASK | \
		XCANFD_IXR_E2BETXTL_MASK | \
		XCANFD_IXR_E1BETXTL_MASK) /* ECC mask */

#define XCANFD_IDR_ID1_MASK		0xFFE00000 /* Standard msg identifier mask */
#define XCANFD_IDR_SRR_MASK		0x00100000 /* Substitute remote TXreq mask */
#define XCANFD_IDR_IDE_MASK		0x00080000 /* Identifier extension mask */
#define XCANFD_IDR_ID2_MASK		0x0007FFFE /* Extended message ident mask */
#define XCANFD_IDR_RTR_MASK		0x00000001 /* Remote TX request mask */
#define XCANFD_DLCR_DLC_MASK		0xF0000000 /* Data length code mask */
#define XCANFD_FSR_FL_MASK		0x00003F00 /* RX Fill Level mask */
#define XCANFD_2_FSR_FL_MASK		0x00007F00 /* RX Fill Level mask */
#define XCANFD_FSR_IRI_MASK		0x00000080 /* RX Increment Read Index mask */
#define XCANFD_FSR_RI_MASK		0x0000001F /* RX Read Index mask */
#define XCANFD_2_FSR_RI_MASK		0x0000003F /* RX Read Index mask */
#define XCANFD_DLCR_EDL_MASK		0x08000000 /* EDL mask in DLC */
#define XCANFD_DLCR_BRS_MASK		0x04000000 /* BRS mask in DLC */
#define XCANFD_DLCR_ESI_MASK		0x02000000 /* ESI mask in DLC */
#define XCANFD_ECC_CFG_REECRX_MASK	BIT(2) /* Reset RX FIFO ECC error counters mask */
#define XCANFD_ECC_CFG_REECTXOL_MASK	BIT(1) /* Reset TXOL FIFO ECC error counters mask */
#define XCANFD_ECC_CFG_REECTXTL_MASK	BIT(0) /* Reset TXTL FIFO ECC error counters mask */
#define XCANFD_ECC_1BIT_CNT_MASK	GENMASK(15, 0) /* FIFO ECC 1bit count mask */
#define XCANFD_ECC_2BIT_CNT_MASK	GENMASK(31, 16) /* FIFO ECC 2bit count mask */

/* Acceptance Filter Register (AFR) bit masks */
#define XCANFD_AFR_UAF_ALL_MASK		0xFFFFFFFF/* All acceptance filters */
#define XCANFD_AFR_UAF0_MASK		BIT(0) /* Acceptance filter 0 */
#define XCANFD_AFR_UAF1_MASK		BIT(1) /* Acceptance filter 1 */
#define XCANFD_AFR_UAF2_MASK		BIT(2) /* Acceptance filter 2 */
#define XCANFD_AFR_UAF3_MASK		BIT(3) /* Acceptance filter 3 */

/* CAN register bit shift - XCANFD_<REG>_<BIT>_SHIFT */
#define XCANFD_BRPR_TDC_ENABLE		BIT(16) /* Transmitter Delay Compensation (TDC) Enable */
#define XCANFD_BTR_SJW_SHIFT		7 /* Synchronous jump width */
#define XCANFD_BTR_TS2_SHIFT		4 /* Time segment 2 */
#define XCANFD_BTR_SJW_SHIFT_CANFD	16 /* Synchronous jump width */
#define XCANFD_BTR_TS2_SHIFT_CANFD	8 /* Time segment 2 */
#define XCANFD_IDR_ID1_SHIFT		21 /* Standard Messg Identifier */
#define XCANFD_IDR_ID2_SHIFT		1 /* Extended Message Identifier */
#define XCANFD_DLCR_DLC_SHIFT		28 /* Data length code */
#define XCANFD_ESR_REC_SHIFT		8 /* Rx Error Count */
#define XCANFD_ECR_REC_SHIFT		8 /* Rx Error Count */

/* CAN frame length constants */
#define XCANFD_FRAME_MAX_DATA_LEN	8
#define XCANFD_DW_BYTES			4
#define XCANFD_TIMEOUT			500
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

/* TX-FIFO-empty interrupt available */
#define XCANFD_FLAG_TXFEMP		0x0001
/* RX Match Not Finished interrupt available */
#define XCANFD_FLAG_RXMNF		0x0002
/* Extended acceptance filters with control at 0xE0 */
#define XCANFD_FLAG_EXT_FILTERS		0x0004
/* TX mailboxes instead of TX FIFO */
#define XCANFD_FLAG_TX_MAILBOXES	0x0008
/* RX FIFO with each buffer in separate registers at 0x1100
 * instead of the regular FIFO at 0x50
 */
#define XCANFD_FLAG_RX_FIFO_MULTI	0x0010
#define XCANFD_FLAG_CANFD_2		0x0020
#define MAX_BUFFER_INDEX		32
#define XCANFD_FIFO_DEPTH		4
#define XCANFD_TXFIFO_0_BASE_ID_OFFSET	0x0100U
#define XCANFD_TXFIFO_0_BASE_DLC_OFFSET	0x0104U
#define XCANFD_TXFIFO_0_BASE_DW0_OFFSET	0x0108U
#define XCANFD_MAX_FRAME_SIZE		72 /**< Maximum Frame size */
#define XCANFD_MAX_FRAME_SIZE		72 /**< Maximum Frame size */
#define CAN_MODE_CONFIG			0x00000001 /**< Configuration mode */
#define CAN_MODE_SLEEP			0x00000008 /**< Sleep mode */
#define MAX_FRAMES_PER_ISR		4

#endif /* __XLNX_CANFD_H__ */
