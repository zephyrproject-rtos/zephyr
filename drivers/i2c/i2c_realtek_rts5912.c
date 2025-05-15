/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <reg/reg_gpio.h>
#include "i2c_realtek_rts5912.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_rts5912, CONFIG_I2C_LOG_LEVEL);

BUILD_ASSERT(CONFIG_I2C_RTS5912_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY,
	     "The I2C Realtek RTS5912 driver must be initialized after the I2C DW driver");

/* i2c_dw has define the DT_DRV_COMPAT at i2c_dw.h
 * so, need to undefine and define our own DT_DRV_COMPAT
 */
#ifdef DT_DRV_COMPAT
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT realtek_rts5912_i2c
#endif

#define RECOVERY_TIME 30 /* in ms */

struct i2c_rts5912_config {
	const struct device *clk_dev;
	struct rts5912_sccon_subsys sccon_cfg;
	const struct device *dw_i2c_dev;
	int scl_gpio;
	int sda_gpio;
};

static inline uint32_t get_regs(const struct device *dev)
{
	return (uint32_t)DEVICE_MMIO_GET(dev);
}

static int i2c_rts5912_recover_bus(const struct device *dev)
{
	static volatile GPIO_Type *pinctrl_base =
		(volatile GPIO_Type *)(DT_REG_ADDR(DT_NODELABEL(pinctrl)));
	struct i2c_rts5912_config const *config = dev->config;

	volatile uint32_t *GPIO_SDA = (volatile uint32_t *)&(pinctrl_base->GCR[config->sda_gpio]);
	volatile uint32_t *GPIO_SCL = (volatile uint32_t *)&(pinctrl_base->GCR[config->scl_gpio]);
	uint32_t GPIO_SDA_TEMP = *GPIO_SDA;
	uint32_t GPIO_SCL_TEMP = *GPIO_SCL;

	/* DW configure data */
	struct device const *dw_i2c_dev = config->dw_i2c_dev;
	struct i2c_dw_dev_config *bus = dw_i2c_dev->data;
	uint32_t reg_base = get_regs(dw_i2c_dev);

	uint32_t value;
	uint32_t start;
	int ret = 0;

	LOG_DBG("starting bus recover");
	LOG_DBG("sda_gpio=%d, GPIO_SDA=0x%08x", config->sda_gpio, *GPIO_SDA);
	LOG_DBG("scl_gpio=%d, GPIO_SCL=0x%08x", config->scl_gpio, *GPIO_SCL);

	/* disable all interrupt mask */
	write_intr_mask(DW_DISABLE_ALL_I2C_INT, reg_base);
	/* enable controller to make sure function works */
	set_bit_enable_en(reg_base);

	if (bus->state & I2C_DW_SDA_STUCK) {
		/*
		 * initiate the SDA Recovery Mechanism
		 * (that is, send at most 9 SCL clocks and STOP to release the
		 * SDA line) and then this bit gets auto clear
		 */
		LOG_DBG("CLK Recovery Start");
		/* initiate the Master Clock Reset */
		start = k_uptime_get_32();
		set_bit_enable_clk_reset(reg_base);
		while (test_bit_enable_clk_reset(reg_base) &&
		       (k_uptime_get_32() - start < RECOVERY_TIME)) {
			;
		}
		/* check if SCL bus clk is not reset */
		if (test_bit_enable_clk_reset(reg_base)) {
			LOG_ERR("ERROR: CLK recovery Fail");
			ret = -1;
		} else {
			LOG_DBG("CLK Recovery Success");
		}

		LOG_DBG("SDA Recovery Start");
		start = k_uptime_get_32();
		set_bit_enable_sdarecov(reg_base);
		while (test_bit_enable_sdarecov(reg_base) &&
		       (k_uptime_get_32() - start < RECOVERY_TIME)) {
			;
		}
		/* Check if bus is not clear */
		if (test_bit_status_sdanotrecov(reg_base)) {
			LOG_ERR("ERROR: SDA Recovery Fail");
			ret = -1;
		} else {
			LOG_DBG("SDA Recovery Success");
		}
	} else if (bus->state & I2C_DW_SCL_STUCK) {
		/* the controller initiates the transfer abort */
		LOG_DBG("ABORT transfer");
		start = k_uptime_get_32();
		set_bit_enable_abort(reg_base);
		while (test_bit_enable_abort(reg_base) &&
		       (k_uptime_get_32() - start < RECOVERY_TIME)) {
			;
		}
		/* check if Controller is not abort */
		if (test_bit_enable_abort(reg_base)) {
			LOG_ERR("ERROR: ABORT Fail!");
			ret = -1;
		} else {
			LOG_DBG("ABORT success");
		}
	}
	value = read_clr_intr(reg_base);
	value = read_clr_tx_abrt(reg_base);
	/* disable controller */
	clear_bit_enable_en(reg_base);
	/* set SCL line to GPIO input mode */
	*GPIO_SCL = 0x8002;
	k_busy_wait(500);
	/* check does SCL line released to high level */
	if ((*GPIO_SCL & GPIO_GCR_PINSTS_Msk) == 0) {
		LOG_ERR("SCL still in Low! scl_gpio=%d, GPIO_SCL=0x%08x", config->scl_gpio,
			*GPIO_SCL);
		*GPIO_SCL = GPIO_SCL_TEMP;
		return -1;
	}
	/* set high level to scl and sda line */
	*GPIO_SCL = 0x28003;
	*GPIO_SDA = 0x28003;
	k_busy_wait(10);

	/* send a ACK */
	*GPIO_SDA = 0x8003;
	k_busy_wait(10);
	*GPIO_SCL = 0x8003;
	k_busy_wait(10);
	*GPIO_SDA = 0x28003;
	k_busy_wait(10);
	/* send dummy clock */
	for (int i = 0; i < 9; i++) {
		*GPIO_SCL = 0x00028003;
		k_busy_wait(50);
		*GPIO_SCL = 0x00008003;
		k_busy_wait(50);
	}
	/* send a stop bit */
	*GPIO_SDA = 0x8003;
	k_busy_wait(10);
	*GPIO_SCL = 0x28003;
	k_busy_wait(10);
	*GPIO_SDA = 0x28003;
	k_busy_wait(10);

	/* set GPIO functoin to I2C */
	*GPIO_SCL = GPIO_SCL_TEMP;
	*GPIO_SDA = GPIO_SDA_TEMP;
	LOG_DBG("SCL=0x%08x, SDA=0x%08x", *GPIO_SCL, *GPIO_SDA);

	/* enable controller */
	set_bit_enable_en(reg_base);

	start = k_uptime_get_32();
	set_bit_enable_abort(reg_base);
	while (test_bit_enable_abort(reg_base) && (k_uptime_get_32() - start < RECOVERY_TIME)) {
		;
	}
	if (test_bit_enable_abort(reg_base)) {
		LOG_ERR("ERROR: ABORT Fail!");
		ret = -1;
	} else {
		LOG_DBG("ABORT success");
	}
	/* disable controller */
	clear_bit_enable_en(reg_base);

	if (ret) {
		LOG_ERR("ERROR: Bus Recover Fail, a slave device may be faulty or require a power "
			"reset");
	} else {
		LOG_DBG("BUS Recover success");
	}
	return ret;
}

static int i2c_rts5912_initialize(const struct device *dev)
{
	const struct i2c_rts5912_config *const config = dev->config;
	int ret = 0;

	/* Register our recovery routine with the DW I2C driver. */
	if (!device_is_ready(config->dw_i2c_dev)) {
		LOG_ERR("DW i2c not ready");
		return -ENODEV;
	}
	i2c_dw_register_recover_bus_cb(config->dw_i2c_dev, i2c_rts5912_recover_bus, dev);

	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("clock source not ready");
		return -ENODEV;
	}
	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t)&config->sccon_cfg);
	if (ret != 0) {
		LOG_ERR("enable i2c[%s] clock source power fail", dev->name);
		return ret;
	}

	uint32_t reg_base = get_regs(config->dw_i2c_dev);
	/* clear enable register */
	clear_bit_enable_en(reg_base);
	/* disable block mode */
	clear_bit_enable_block(reg_base);

	return ret;
}

#define DEV_CONFIG_CLK_DEV_INIT(n)                                                                 \
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                          \
	.sccon_cfg = {                                                                             \
		.clk_grp = DT_INST_CLOCKS_CELL(n, clk_grp),                                        \
		.clk_idx = DT_INST_CLOCKS_CELL(n, clk_idx),                                        \
	}

#define I2C_DEVICE_INIT_RTS5912(n)                                                                 \
	static const struct i2c_rts5912_config i2c_rts5912_##n##_config = {                        \
		DEV_CONFIG_CLK_DEV_INIT(n),                                                        \
		.dw_i2c_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, dw_i2c_dev)),                       \
		.sda_gpio = DT_INST_PROP(n, sda_gpio_pin),                                         \
		.scl_gpio = DT_INST_PROP(n, scl_gpio_pin)};                                        \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_rts5912_initialize, NULL, NULL,                           \
				  &i2c_rts5912_##n##_config, POST_KERNEL,                          \
				  CONFIG_I2C_RTS5912_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_RTS5912)
