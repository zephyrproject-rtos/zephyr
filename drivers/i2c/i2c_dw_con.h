/* i2c_dw_con.h - DesignWare I2C IC_CON helpers */

/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_DW_CON_H_
#define ZEPHYR_DRIVERS_I2C_I2C_DW_CON_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/toolchain.h>

/* IC_CON bits */
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

static inline union ic_con_register i2c_dw_target_mode_con(bool addr_10bit)
{
	union ic_con_register ic_con = { .raw = 0U };

	ic_con.bits.addr_slave_10bit = addr_10bit ? 1U : 0U;
	ic_con.bits.rx_fifo_full = 1U;
	ic_con.bits.restart_en = 1U;
	ic_con.bits.stop_det = 1U;

	return ic_con;
}

#endif /* ZEPHYR_DRIVERS_I2C_I2C_DW_CON_H_ */
