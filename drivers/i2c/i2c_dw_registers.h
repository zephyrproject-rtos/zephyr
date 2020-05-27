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
#define IC_CON_TX_INTR_MODE			(1 << 8)
#define IC_CON_STOP_DET_IFADDR			(1 << 7)
#define IC_CON_SLAVE_DISABLE			(1 << 6)
#define IC_CON_RESTART_EN			(1 << 5)
#define IC_CON_10BIT_ADDR_MASTER		(1 << 4)
#define IC_CON_10BIT_ADDR_SLAVE			(1 << 3)
#define IC_CON_SPEED_MASK			(0x3 << 1)
#define IC_CON_MASTER_MODE			(1 << 0)

union ic_con_register {
	uint16_t                raw;
	struct {
		uint16_t         master_mode : 1 __packed;
		uint16_t         speed : 2 __packed;
		uint16_t         addr_slave_10bit : 1 __packed;
		uint16_t         addr_master_10bit : 1 __packed;
		uint16_t         restart_en : 1 __packed;
		uint16_t         slave_disable : 1 __packed;
		uint16_t         stop_det : 1 __packed;
		uint16_t         tx_empty_ctl : 1 __packed;
		uint16_t         rx_fifo_full : 1 __packed;
	} bits;
};


/* IC_DATA_CMD bits */
#define IC_DATA_CMD_DAT_MASK		0xFF
#define IC_DATA_CMD_CMD			(1 << 8)
#define IC_DATA_CMD_STOP		(1 << 9)
#define IC_DATA_CMD_RESTART		(1 << 10)

union ic_data_cmd_register {
	uint16_t                raw;
	struct {
	       uint16_t         dat : 8 __packed;
	       uint16_t         cmd : 1 __packed;
	       uint16_t         stop : 1 __packed;
	       uint16_t         restart : 1 __packed;
	       uint16_t         reserved : 4 __packed;
	} bits;
};

/* IC_ENABLE register bits */
#define IC_ENABLE_ENABLE		(1 << 0)
#define IC_ENABLE_ABORT			(1 << 1)

union ic_enable_register {
	uint16_t		raw;
	struct {
		uint16_t	enable : 1 __packed;
		uint16_t	abort : 1 __packed;
		uint16_t	reserved : 13 __packed;
	} bits;
};


/* DesignWare Interrupt bits positions */
#define DW_INTR_STAT_RX_UNDER		(1 << 0)
#define DW_INTR_STAT_RX_OVER		(1 << 1)
#define DW_INTR_STAT_RX_FULL		(1 << 2)
#define DW_INTR_STAT_TX_OVER		(1 << 3)
#define DW_INTR_STAT_TX_EMPTY		(1 << 4)
#define DW_INTR_STAT_RD_REQ		(1 << 5)
#define DW_INTR_STAT_TX_ABRT		(1 << 6)
#define DW_INTR_STAT_RX_DONE		(1 << 7)
#define DW_INTR_STAT_ACTIVITY		(1 << 8)
#define DW_INTR_STAT_STOP_DET		(1 << 9)
#define DW_INTR_STAT_START_DET		(1 << 10)
#define DW_INTR_STAT_GEN_CALL		(1 << 11)
#define DW_INTR_STAT_RESTART_DET	(1 << 12)
#define DW_INTR_STAT_MST_ON_HOLD	(1 << 13)


union ic_interrupt_register {
	uint16_t			raw;
	struct {
		uint16_t	rx_under : 1 __packed;
		uint16_t	rx_over : 1 __packed;
		uint16_t	rx_full : 1 __packed;
		uint16_t	tx_over : 1 __packed;
		uint16_t	tx_empty : 1 __packed;
		uint16_t	rd_req : 1 __packed;
		uint16_t	tx_abrt : 1 __packed;
		uint16_t	rx_done : 1 __packed;
		uint16_t	activity : 1 __packed;
		uint16_t	stop_det : 1 __packed;
		uint16_t	start_det : 1 __packed;
		uint16_t	gen_call : 1 __packed;
		uint16_t	restart_det : 1 __packed;
		uint16_t	mst_on_hold : 1 __packed;
		uint16_t	reserved : 2 __packed;
	} bits;
};


/* IC_TAR */
union ic_tar_register {
	uint16_t		raw;
	struct {
		uint16_t	ic_tar : 10 __packed;
		uint16_t	gc_or_start : 1 __packed;
		uint16_t	special : 1 __packed;
		uint16_t	ic_10bitaddr_master : 1 __packed;
		uint16_t	reserved : 3 __packed;
	} bits;
};


/* IC_SAR */
union ic_sar_register {
	uint16_t		raw;
	struct {
		uint16_t	ic_sar : 10 __packed;
		uint16_t	reserved : 6 __packed;
	} bits;
};


/* IC_STATUS */
union ic_status_register {
	uint32_t		raw;
	struct {
		uint32_t	activity : 1 __packed;
		uint32_t	tfnf : 1 __packed;
		uint32_t	tfe : 1 __packed;
		uint32_t	rfne : 1 __packed;
		uint32_t	rff : 1 __packed;
		uint32_t	mst_activity : 1 __packed;
		uint32_t	slv_activity : 1 __packed;
		uint32_t	reserved : 24 __packed;
	} bits;
};


union ic_comp_param_1_register {
	uint32_t			raw;
	struct {
		uint32_t		apb_data_width : 2 __packed;
		uint32_t		max_speed_mode : 2 __packed;
		uint32_t		hc_count_values : 1 __packed;
		uint32_t		intr_io : 1 __packed;
		uint32_t		has_dma : 1 __packed;
		uint32_t		add_encoded_params : 1 __packed;
		uint32_t		rx_buffer_depth : 8 __packed;
		uint32_t		tx_buffer_depth : 8 __packed;
		uint32_t		reserved : 7 __packed;
	} bits;
};


/*
 * instantiate this like:
 *  volatile struct i2c_dw_registers *regs = (struct i2c_dw_regs *)0x80000000;
 *
 *  If this is being set as a global, use the following change to avoid the
 *  base pointer from being reloaded after function calls:
 *  volatile struct i2c_dw_regs* const *regs = (struct i2c_dw_regs*)0x80000000;
 *
 *  This will allow access to the registers like so:
 *  x = regs->ctrlreg;
 *  regs->ctrlreg = newval;
 */
struct i2c_dw_registers {
	union ic_con_register ic_con;	/* offset 0x00 */
	uint16_t dummy1;
	union ic_tar_register ic_tar;	/* offset 0x04 */
	uint16_t dummy2;
	union ic_sar_register ic_sar;	/* offset 0x08 */
	uint16_t dummy3;
	uint16_t ic_hs_maddr;		/* offset 0x0C */
	uint16_t dummy4;
	union ic_data_cmd_register ic_data_cmd;		/* offset 0x10 */
	uint16_t dummy5;
	uint16_t ic_ss_scl_hcnt;	/* offset 0x14 */
	uint16_t dummy6;
	uint16_t ic_ss_scl_lcnt;	/* offset 0x18 */
	uint16_t dummy7;
	uint16_t ic_fs_scl_hcnt;	/* offset 0x1C */
	uint16_t dummy8;
	uint16_t ic_fs_scl_lcnt;	/* offset 0x20 */
	uint16_t dummy9;
	uint16_t ic_hs_scl_hcnt;	/* offset 0x24 */
	uint16_t dummy10;
	uint16_t ic_hs_scl_lcnt;	/* offset 0x28 */
	uint16_t dummy11;
	union ic_interrupt_register ic_intr_stat;	/* offset 0x2C */
	uint16_t dummy12;
	union ic_interrupt_register ic_intr_mask;	/* offset 0x30 */
	uint16_t dummy13;
	union ic_interrupt_register ic_raw_intr_stat;	/* offset 0x34 */
	uint16_t dummy14;
	uint16_t ic_rx_tl;		/* offset 0x38 */
	uint16_t dummy15;
	uint16_t ic_tx_tl;		/* offset 0x3C */
	uint16_t dummy16;
	uint16_t ic_clr_intr;		/* offset 0x40 */
	uint16_t dummy17;
	uint16_t ic_clr_rx_under;	/* offset 0x44 */
	uint16_t dummy18;
	uint16_t ic_clr_rx_over;	/* offset 0x48 */
	uint16_t dummy19;
	uint16_t ic_clr_tx_over;	/* offset 0x4C */
	uint16_t dummy20;
	uint16_t ic_clr_rd_req;		/* offset 0x50 */
	uint16_t dummy21;
	uint16_t ic_clr_tx_abrt;	/* offset 0x54 */
	uint16_t dummy22;
	uint16_t ic_clr_rx_done;	/* offset 0x58 */
	uint16_t dummy23;
	uint16_t ic_clr_activity;	/* offset 0x5C */
	uint16_t dummy24;
	uint16_t ic_clr_stop_det;	/* offset 0x60 */
	uint16_t dummy25;
	uint16_t ic_clr_start_det;	/* offset 0x64 */
	uint16_t dummy26;
	uint16_t ic_clr_gen_call;	/* offset 0x68 */
	uint16_t dummy27;
	union ic_enable_register ic_enable;	/* offset 0x6c */
	uint16_t dummy28;
	union ic_status_register ic_status;	/* offset 0x70 */
	uint32_t ic_txflr;		/* offset 0x74 */
	uint32_t ic_rxflr;		/* offset 0x78 */
	uint32_t ic_sda_hold;		/* offset 0x7C */
	uint32_t ic_tx_abrt_source;	/* offset 0x80 */
	uint32_t ic_slv_data_nack_only; /* offset 0x84 */
	uint32_t ic_dma_cr;		/* offset 0x88 */
	uint32_t ic_dma_tdlr;		/* offset 0x8C */
	uint32_t ic_dma_rdlr;		/* offset 0x90 */
	uint32_t ic_sda_setup;		/* offset 0x94 */
	uint32_t ic_ack_general_call;	/* offset 0x98 */
	uint32_t ic_enable_status;	/* offset 0x9C */
	uint32_t ic_fs_spklen;		/* offset 0xA0 */
	uint32_t ic_hs_spklen;		/* offset 0xA4 */
	uint16_t ic_clr_restart_det;	/* offset 0xA8 */
	uint8_t	 filler[74];
	union ic_comp_param_1_register ic_comp_param_1;	/* offset 0xF4 */
	uint32_t ic_comp_version;	/* offset 0xF8 */
	uint32_t ic_comp_type;		/* offset 0xFC */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_DW_REGISTERS_H_ */
