/* i2c_dw_registers.h - array access for I2C Design Ware registers */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_I2C_I2C_DW_REGISTERS_H_
#define ZEPHYR_DRIVERS_I2C_I2C_DW_REGISTERS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*  IC_CON bits */
union ic_con_register {
	uint32_t                raw;
	struct {
		uint32_t         master_mode : 1 __packed;
		uint32_t         speed : 2 __packed;
		uint32_t         addr_slave_10bit : 1 __packed;
		uint32_t         addr_master_10bit : 1 __packed;
		uint32_t         restart_en : 1 __packed;
		uint32_t         slave_disable : 1 __packed;
		uint32_t         stop_det : 1 __packed;
		uint32_t         tx_empty_ctl : 1 __packed;
		uint32_t         rx_fifo_full : 1 __packed;
	} bits;
};

/* IC_DATA_CMD bits */
#define IC_DATA_CMD_DAT_MASK		0xFF
#define IC_DATA_CMD_CMD			BIT(8)
#define IC_DATA_CMD_STOP		BIT(9)
#define IC_DATA_CMD_RESTART		BIT(10)

/* DesignWare Interrupt bits positions */
#define DW_INTR_STAT_RX_UNDER		BIT(0)
#define DW_INTR_STAT_RX_OVER		BIT(1)
#define DW_INTR_STAT_RX_FULL		BIT(2)
#define DW_INTR_STAT_TX_OVER		BIT(3)
#define DW_INTR_STAT_TX_EMPTY		BIT(4)
#define DW_INTR_STAT_RD_REQ			BIT(5)
#define DW_INTR_STAT_TX_ABRT		BIT(6)
#define DW_INTR_STAT_RX_DONE		BIT(7)
#define DW_INTR_STAT_ACTIVITY		BIT(8)
#define DW_INTR_STAT_STOP_DET		BIT(9)
#define DW_INTR_STAT_START_DET		BIT(10)
#define DW_INTR_STAT_GEN_CALL		BIT(11)
#define DW_INTR_STAT_RESTART_DET	BIT(12)
#define DW_INTR_STAT_MST_ON_HOLD	BIT(13)

#define DW_INTR_MASK_RX_UNDER		BIT(0)
#define DW_INTR_MASK_RX_OVER		BIT(1)
#define DW_INTR_MASK_RX_FULL		BIT(2)
#define DW_INTR_MASK_TX_OVER		BIT(3)
#define DW_INTR_MASK_TX_EMPTY		BIT(4)
#define DW_INTR_MASK_RD_REQ			BIT(5)
#define DW_INTR_MASK_TX_ABRT		BIT(6)
#define DW_INTR_MASK_RX_DONE		BIT(7)
#define DW_INTR_MASK_ACTIVITY		BIT(8)
#define DW_INTR_MASK_STOP_DET		BIT(9)
#define DW_INTR_MASK_START_DET		BIT(10)
#define DW_INTR_MASK_GEN_CALL		BIT(11)
#define DW_INTR_MASK_RESTART_DET	BIT(12)
#define DW_INTR_MASK_MST_ON_HOLD	BIT(13)
#define DW_INTR_MASK_RESET		0x000008ff

union ic_interrupt_register {
	uint32_t			raw;
	struct {
		uint32_t	rx_under : 1 __packed;
		uint32_t	rx_over : 1 __packed;
		uint32_t	rx_full : 1 __packed;
		uint32_t	tx_over : 1 __packed;
		uint32_t	tx_empty : 1 __packed;
		uint32_t	rd_req : 1 __packed;
		uint32_t	tx_abrt : 1 __packed;
		uint32_t	rx_done : 1 __packed;
		uint32_t	activity : 1 __packed;
		uint32_t	stop_det : 1 __packed;
		uint32_t	start_det : 1 __packed;
		uint32_t	gen_call : 1 __packed;
		uint32_t	restart_det : 1 __packed;
		uint32_t	mst_on_hold : 1 __packed;
		uint32_t	reserved : 2 __packed;
	} bits;
};

/* IC_TAR */
union ic_tar_register {
	uint32_t		raw;
	struct {
		uint32_t	ic_tar : 10 __packed;
		uint32_t	gc_or_start : 1 __packed;
		uint32_t	special : 1 __packed;
		uint32_t	ic_10bitaddr_master : 1 __packed;
		uint32_t	reserved : 3 __packed;
	} bits;
};

/* IC_COMP_PARAM_1 */
union ic_comp_param_1_register {
	uint32_t		raw;
	struct {
		uint32_t	apb_data_width : 2 __packed;
		uint32_t	max_speed_mode : 2 __packed;
		uint32_t	hc_count_values : 1 __packed;
		uint32_t	intr_io : 1 __packed;
		uint32_t	has_dma : 1 __packed;
		uint32_t	add_encoded_params : 1 __packed;
		uint32_t	rx_buffer_depth : 8 __packed;
		uint32_t	tx_buffer_depth : 8 __packed;
		uint32_t	reserved : 7 __packed;
	} bits;
};

#define DW_IC_REG_CON				(0x00)
#define DW_IC_REG_TAR				(0x04)
#define DW_IC_REG_SAR				(0x08)
#define DW_IC_REG_DATA_CMD			(0x10)
#define DW_IC_REG_SS_SCL_HCNT		(0x14)
#define DW_IC_REG_SS_SCL_LCNT		(0x18)
#define DW_IC_REG_FS_SCL_HCNT		(0x1C)
#define DW_IC_REG_FS_SCL_LCNT		(0x20)
#define DW_IC_REG_HS_SCL_HCNT		(0x24)
#define DW_IC_REG_HS_SCL_LCNT		(0x28)
#define DW_IC_REG_INTR_STAT			(0x2C)
#define DW_IC_REG_INTR_MASK			(0x30)
#define DW_IC_REG_RX_TL				(0x38)
#define DW_IC_REG_TX_TL				(0x3C)
#define DW_IC_REG_CLR_INTR			(0x40)
#define DW_IC_REG_CLR_RX_UNDER		(0x44)
#define DW_IC_REG_CLR_RX_OVER		(0x48)
#define DW_IC_REG_CLR_TX_OVER		(0x4c)
#define DW_IC_REG_CLR_RD_REQ		(0x50)
#define DW_IC_REG_CLR_TX_ABRT		(0x54)
#define DW_IC_REG_CLR_RX_DONE		(0x58)
#define DW_IC_REG_CLR_ACTIVITY		(0x5c)
#define DW_IC_REG_CLR_STOP_DET		(0x60)
#define DW_IC_REG_CLR_START_DET		(0x64)
#define DW_IC_REG_CLR_GEN_CALL		(0x68)
#define DW_IC_REG_ENABLE			(0x6C)
#define DW_IC_REG_STATUS			(0x70)
#define DW_IC_REG_TXFLR				(0x74)
#define DW_IC_REG_RXFLR				(0x78)
#define DW_IC_REG_DMA_CR                        (0x88)
#define DW_IC_REG_TDLR                          (0x8C)
#define DW_IC_REG_RDLR                          (0x90)
#define DW_IC_REG_FS_SPKLEN			(0xA0)
#define DW_IC_REG_HS_SPKLEN			(0xA4)
#define DW_IC_REG_COMP_PARAM_1		(0xF4)
#define DW_IC_REG_COMP_TYPE			(0xFC)

#define IDMA_REG_INTR_STS               0xAE8
#define IDMA_TX_RX_CHAN_MASK            0x3

/* CON Bit */
#define DW_IC_CON_MASTER_MODE_BIT		(0)

/* DMA control bits */
#define DW_IC_DMA_RX_ENABLE                    BIT(0)
#define DW_IC_DMA_TX_ENABLE                    BIT(1)
#define DW_IC_DMA_ENABLE                       (BIT(0) | BIT(1))

DEFINE_TEST_BIT_OP(con_master_mode, DW_IC_REG_CON, DW_IC_CON_MASTER_MODE_BIT)
DEFINE_MM_REG_WRITE(con, DW_IC_REG_CON, 32)
DEFINE_MM_REG_READ(con, DW_IC_REG_CON, 32)

DEFINE_MM_REG_WRITE(cmd_data, DW_IC_REG_DATA_CMD, 32)
DEFINE_MM_REG_READ(cmd_data, DW_IC_REG_DATA_CMD, 32)

DEFINE_MM_REG_WRITE(ss_scl_hcnt, DW_IC_REG_SS_SCL_HCNT, 32)
DEFINE_MM_REG_WRITE(ss_scl_lcnt, DW_IC_REG_SS_SCL_LCNT, 32)

DEFINE_MM_REG_WRITE(fs_scl_hcnt, DW_IC_REG_FS_SCL_HCNT, 32)
DEFINE_MM_REG_WRITE(fs_scl_lcnt, DW_IC_REG_FS_SCL_LCNT, 32)

DEFINE_MM_REG_WRITE(hs_scl_hcnt, DW_IC_REG_HS_SCL_HCNT, 32)
DEFINE_MM_REG_WRITE(hs_scl_lcnt, DW_IC_REG_HS_SCL_LCNT, 32)



DEFINE_MM_REG_READ(intr_stat, DW_IC_REG_INTR_STAT, 32)
#define DW_IC_INTR_STAT_TX_ABRT_BIT		(6)
DEFINE_TEST_BIT_OP(intr_stat_tx_abrt, DW_IC_REG_INTR_STAT, DW_IC_INTR_STAT_TX_ABRT_BIT)

DEFINE_MM_REG_WRITE(intr_mask, DW_IC_REG_INTR_MASK, 32)
#define DW_IC_INTR_MASK_TX_EMPTY_BIT		(4)
DEFINE_CLEAR_BIT_OP(intr_mask_tx_empty, DW_IC_REG_INTR_MASK, DW_IC_INTR_MASK_TX_EMPTY_BIT)
DEFINE_SET_BIT_OP(intr_mask_tx_empty, DW_IC_REG_INTR_MASK, DW_IC_INTR_MASK_TX_EMPTY_BIT)

DEFINE_MM_REG_WRITE(rx_tl, DW_IC_REG_RX_TL, 32)
DEFINE_MM_REG_WRITE(tx_tl, DW_IC_REG_TX_TL, 32)

DEFINE_MM_REG_READ(clr_intr, DW_IC_REG_CLR_INTR, 32)
DEFINE_MM_REG_READ(clr_stop_det, DW_IC_REG_CLR_STOP_DET, 32)
DEFINE_MM_REG_READ(clr_start_det, DW_IC_REG_CLR_START_DET, 32)
DEFINE_MM_REG_READ(clr_gen_call, DW_IC_REG_CLR_GEN_CALL, 32)
DEFINE_MM_REG_READ(clr_tx_abrt, DW_IC_REG_CLR_TX_ABRT, 32)
DEFINE_MM_REG_READ(clr_rx_under, DW_IC_REG_CLR_RX_UNDER, 32)
DEFINE_MM_REG_READ(clr_rx_over, DW_IC_REG_CLR_RX_OVER, 32)
DEFINE_MM_REG_READ(clr_tx_over, DW_IC_REG_CLR_TX_OVER, 32)
DEFINE_MM_REG_READ(clr_rx_done, DW_IC_REG_CLR_RX_DONE, 32)
DEFINE_MM_REG_READ(clr_rd_req, DW_IC_REG_CLR_RD_REQ, 32)
DEFINE_MM_REG_READ(clr_activity, DW_IC_REG_CLR_ACTIVITY, 32)

#define DW_IC_ENABLE_EN_BIT		(0)
DEFINE_CLEAR_BIT_OP(enable_en, DW_IC_REG_ENABLE, DW_IC_ENABLE_EN_BIT)
DEFINE_SET_BIT_OP(enable_en, DW_IC_REG_ENABLE, DW_IC_ENABLE_EN_BIT)


#define DW_IC_STATUS_ACTIVITY_BIT	(0)
#define DW_IC_STATUS_TFNT_BIT	(1)
#define DW_IC_STATUS_RFNE_BIT	(3)
DEFINE_TEST_BIT_OP(status_activity, DW_IC_REG_STATUS, DW_IC_STATUS_ACTIVITY_BIT)
DEFINE_TEST_BIT_OP(status_tfnt, DW_IC_REG_STATUS, DW_IC_STATUS_TFNT_BIT)
DEFINE_TEST_BIT_OP(status_rfne, DW_IC_REG_STATUS, DW_IC_STATUS_RFNE_BIT)

DEFINE_MM_REG_READ(txflr, DW_IC_REG_TXFLR, 32)
DEFINE_MM_REG_READ(rxflr, DW_IC_REG_RXFLR, 32)

DEFINE_MM_REG_READ(dma_cr, DW_IC_REG_DMA_CR, 32)
DEFINE_MM_REG_WRITE(dma_cr, DW_IC_REG_DMA_CR, 32)

DEFINE_MM_REG_READ(tdlr, DW_IC_REG_TDLR, 32)
DEFINE_MM_REG_WRITE(tdlr, DW_IC_REG_TDLR, 32)
DEFINE_MM_REG_READ(rdlr, DW_IC_REG_RDLR, 32)
DEFINE_MM_REG_WRITE(rdlr, DW_IC_REG_RDLR, 32)

DEFINE_MM_REG_READ(fs_spklen, DW_IC_REG_FS_SPKLEN, 32)
DEFINE_MM_REG_READ(hs_spklen, DW_IC_REG_HS_SPKLEN, 32)

DEFINE_MM_REG_READ(comp_param_1, DW_IC_REG_COMP_PARAM_1, 32)
DEFINE_MM_REG_READ(comp_type, DW_IC_REG_COMP_TYPE, 32)
DEFINE_MM_REG_READ(tar, DW_IC_REG_TAR, 32)
DEFINE_MM_REG_WRITE(tar, DW_IC_REG_TAR, 32)
DEFINE_MM_REG_WRITE(sar, DW_IC_REG_SAR, 32)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_DW_REGISTERS_H_ */
