/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _REALTEK_RTS5912_REG_I2C_H
#define _REALTEK_RTS5912_REG_I2C_H

/**
 * @brief I2C Controller (I2C)
 */

#ifdef __cplusplus
extern "C" {
#endif

/*  IC_CON bits */
union ic_con_register {
	uint32_t raw;
	struct {
		uint32_t master_mode: 1 __packed;
		uint32_t speed: 2 __packed;
		uint32_t addr_slave_10bit: 1 __packed;
		uint32_t addr_master_10bit: 1 __packed;
		uint32_t restart_en: 1 __packed;
		uint32_t slave_disable: 1 __packed;
		uint32_t stop_det: 1 __packed;
		uint32_t tx_empty_ctl: 1 __packed;
		uint32_t rx_fifo_full: 1 __packed;
		uint32_t stop_det_mstactive: 1 __packed;
		uint32_t bus_clear: 1 __packed;
	} bits;
};

/* IC_DATA_CMD bits */
#define IC_DATA_CMD_DAT_MASK 0xFF
#define IC_DATA_CMD_CMD      BIT(8)
#define IC_DATA_CMD_STOP     BIT(9)
#define IC_DATA_CMD_RESTART  BIT(10)

/* DesignWare Interrupt bits positions */
#define RTS5912_INTR_STAT_RX_UNDER      BIT(0)
#define RTS5912_INTR_STAT_RX_OVER       BIT(1)
#define RTS5912_INTR_STAT_RX_FULL       BIT(2)
#define RTS5912_INTR_STAT_TX_OVER       BIT(3)
#define RTS5912_INTR_STAT_TX_EMPTY      BIT(4)
#define RTS5912_INTR_STAT_RD_REQ        BIT(5)
#define RTS5912_INTR_STAT_TX_ABRT       BIT(6)
#define RTS5912_INTR_STAT_RX_DONE       BIT(7)
#define RTS5912_INTR_STAT_ACTIVITY      BIT(8)
#define RTS5912_INTR_STAT_STOP_DET      BIT(9)
#define RTS5912_INTR_STAT_START_DET     BIT(10)
#define RTS5912_INTR_STAT_GEN_CALL      BIT(11)
#define RTS5912_INTR_STAT_RESTART_DET   BIT(12)
#define RTS5912_INTR_STAT_MST_ON_HOLD   BIT(13)
#define RTS5912_INTR_STAT_SCL_STUCK_LOW BIT(14)

#define RTS5912_INTR_MASK_RX_UNDER    BIT(0)
#define RTS5912_INTR_MASK_RX_OVER     BIT(1)
#define RTS5912_INTR_MASK_RX_FULL     BIT(2)
#define RTS5912_INTR_MASK_TX_OVER     BIT(3)
#define RTS5912_INTR_MASK_TX_EMPTY    BIT(4)
#define RTS5912_INTR_MASK_RD_REQ      BIT(5)
#define RTS5912_INTR_MASK_TX_ABRT     BIT(6)
#define RTS5912_INTR_MASK_RX_DONE     BIT(7)
#define RTS5912_INTR_MASK_ACTIVITY    BIT(8)
#define RTS5912_INTR_MASK_STOP_DET    BIT(9)
#define RTS5912_INTR_MASK_START_DET   BIT(10)
#define RTS5912_INTR_MASK_GEN_CALL    BIT(11)
#define RTS5912_INTR_MASK_RESTART_DET BIT(12)
#define RTS5912_INTR_MASK_MST_ON_HOLD BIT(13)
#define RTS5912_INTR_MASK_RESET       0x000008ff

union ic_interrupt_register {
	uint32_t raw;
	struct {
		uint32_t rx_under: 1 __packed;
		uint32_t rx_over: 1 __packed;
		uint32_t rx_full: 1 __packed;
		uint32_t tx_over: 1 __packed;
		uint32_t tx_empty: 1 __packed;
		uint32_t rd_req: 1 __packed;
		uint32_t tx_abrt: 1 __packed;
		uint32_t rx_done: 1 __packed;
		uint32_t activity: 1 __packed;
		uint32_t stop_det: 1 __packed;
		uint32_t start_det: 1 __packed;
		uint32_t gen_call: 1 __packed;
		uint32_t restart_det: 1 __packed;
		uint32_t mst_on_hold: 1 __packed;
		uint32_t scl_stuck_low: 1 __packed;
		uint32_t reserved: 2 __packed;
	} bits;
};

union ic_txabrt_register {
	uint32_t raw;
	struct {
		uint32_t ADDR7BNACK: 1 __packed;
		uint32_t ADDR10BNACK1: 1 __packed;
		uint32_t ADDR10BNACK2: 1 __packed;
		uint32_t TXDATANACK: 1 __packed;
		uint32_t GCALLNACK: 1 __packed;
		uint32_t GCALLREAD: 1 __packed;
		uint32_t HSACKDET: 1 __packed;
		uint32_t SBYTEACKET: 1 __packed;
		uint32_t HSNORSTRT: 1 __packed;
		uint32_t SBYTENORSTRT: 1 __packed;
		uint32_t ADDR10BRDNORSTRT: 1 __packed;
		uint32_t MASTERIDS: 1 __packed;
		uint32_t ARBLOST: 1 __packed;
		uint32_t SLVFLUSHTXFIFO: 1 __packed;
		uint32_t SLVARBLOST: 1 __packed;
		uint32_t SLVRDINTX: 1 __packed;
		uint32_t USRABRT: 1 __packed;
		uint32_t SDASTUCKLOW: 1 __packed;
		uint32_t reserved: 2 __packed;
	} bits;
};

/* IC_TAR */
union ic_tar_register {
	uint32_t raw;
	struct {
		uint32_t ic_tar: 10 __packed;
		uint32_t gc_or_start: 1 __packed;
		uint32_t special: 1 __packed;
		uint32_t ic_10bitaddr_master: 1 __packed;
		uint32_t reserved: 3 __packed;
	} bits;
};

/* IC_COMP_PARAM_1 */
union ic_comp_param_1_register {
	uint32_t raw;
	struct {
		uint32_t apb_data_width: 2 __packed;
		uint32_t max_speed_mode: 2 __packed;
		uint32_t hc_count_values: 1 __packed;
		uint32_t intr_io: 1 __packed;
		uint32_t has_dma: 1 __packed;
		uint32_t add_encoded_params: 1 __packed;
		uint32_t rx_buffer_depth: 8 __packed;
		uint32_t tx_buffer_depth: 8 __packed;
		uint32_t reserved: 7 __packed;
	} bits;
};

#define RTS5912_IC_REG_CON           (0x00)
#define RTS5912_IC_REG_TAR           (0x04)
#define RTS5912_IC_REG_SAR           (0x08)
#define RTS5912_IC_REG_DATA_CMD      (0x10)
#define RTS5912_IC_REG_SS_SCL_HCNT   (0x14)
#define RTS5912_IC_REG_SS_SCL_LCNT   (0x18)
#define RTS5912_IC_REG_FS_SCL_HCNT   (0x1C)
#define RTS5912_IC_REG_FS_SCL_LCNT   (0x20)
#define RTS5912_IC_REG_HS_SCL_HCNT   (0x24)
#define RTS5912_IC_REG_HS_SCL_LCNT   (0x28)
#define RTS5912_IC_REG_INTR_STAT     (0x2C)
#define RTS5912_IC_REG_INTR_MASK     (0x30)
#define RTS5912_IC_REG_RAWINTR_MASK  (0x34)
#define RTS5912_IC_REG_RX_TL         (0x38)
#define RTS5912_IC_REG_TX_TL         (0x3C)
#define RTS5912_IC_REG_CLR_INTR      (0x40)
#define RTS5912_IC_REG_CLR_RX_UNDER  (0x44)
#define RTS5912_IC_REG_CLR_RX_OVER   (0x48)
#define RTS5912_IC_REG_CLR_TX_OVER   (0x4c)
#define RTS5912_IC_REG_CLR_RD_REQ    (0x50)
#define RTS5912_IC_REG_CLR_TX_ABRT   (0x54)
#define RTS5912_IC_REG_CLR_RX_DONE   (0x58)
#define RTS5912_IC_REG_CLR_ACTIVITY  (0x5c)
#define RTS5912_IC_REG_CLR_STOP_DET  (0x60)
#define RTS5912_IC_REG_CLR_START_DET (0x64)
#define RTS5912_IC_REG_CLR_GEN_CALL  (0x68)
#define RTS5912_IC_REG_ENABLE        (0x6C)
#define RTS5912_IC_REG_STATUS        (0x70)
#define RTS5912_IC_REG_TXFLR         (0x74)
#define RTS5912_IC_REG_RXFLR         (0x78)
#define RTS5912_IC_REG_SDAHOLD       (0x7c)
#define RTS5912_IC_REG_TXABRTSRC     (0x80)
#define RTS5912_IC_REG_DMA_CR        (0x88)
#define RTS5912_IC_REG_TDLR          (0x8C)
#define RTS5912_IC_REG_RDLR          (0x90)
#define RTS5912_IC_REG_FS_SPKLEN     (0xA0)
#define RTS5912_IC_REG_HS_SPKLEN     (0xA4)
#define RTS5912_IC_REG_SCL_TIMEOUT   (0xAC)
#define RTS5912_IC_REG_SDA_TIMEOUT   (0xB0)
#define RTS5912_IC_REG_COMP_PARAM_1  (0xF4)
#define RTS5912_IC_REG_COMP_TYPE     (0xFC)

#define IDMA_REG_INTR_STS    0xAE8
#define IDMA_TX_RX_CHAN_MASK 0x3

/* CON Bit */
#define RTS5912_IC_CON_MASTER_MODE_BIT (0)

/* DMA control bits */
#define RTS5912_IC_DMA_RX_ENABLE BIT(0)
#define RTS5912_IC_DMA_TX_ENABLE BIT(1)
#define RTS5912_IC_DMA_ENABLE    (BIT(0) | BIT(1))

#ifdef __cplusplus
}
#endif

#endif /* _REALTEK_RTS5912_REG_I2C_H */
