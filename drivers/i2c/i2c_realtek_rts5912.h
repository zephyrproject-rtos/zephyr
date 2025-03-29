/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_I2C_I2C_REALTEK_RTS5912_H_
#define ZEPHYR_DRIVERS_I2C_I2C_REALTEK_RTS5912_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <stdbool.h>

#define DT_DRV_COMPAT realtek_rts5912_i2c

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_RTS5912_MAGIC_KEY 0x44570140

typedef void (*i2c_isr_cb_t)(const struct device *port);

#define IC_ACTIVITY   (1 << 0)
#define IC_ENABLE_BIT (1 << 0)

/* dev->state values from IC_DATA_CMD Data transfer mode settings (bit 8) */
#define I2C_RTS5912_STATE_READY (0)
#define I2C_RTS5912_CMD_SEND    (1 << 0)
#define I2C_RTS5912_CMD_RECV    (1 << 1)
#define I2C_RTS5912_CMD_ERROR   (1 << 2)
#define I2C_RTS5912_BUSY        (1 << 3)
#define I2C_RTS5912_SCL_STUCK   (1 << 4)
#define I2C_RTS5912_TX_ABRT     (1 << 5)
#define I2C_RTS5912_SDA_STUCK   (1 << 6)
#define I2C_RTS5912_NACK        (1 << 7)

#define I2C_RTS5912_ERR_MASK                                                                       \
	(I2C_RTS5912_CMD_ERROR | I2C_RTS5912_SCL_STUCK | I2C_RTS5912_SDA_STUCK | I2C_RTS5912_NACK)

#define I2C_RTS5912_STUCK_ERR_MASK (I2C_RTS5912_SCL_STUCK | I2C_RTS5912_SDA_STUCK)

#define RTS5912_ENABLE_TX_INT_I2C_MASTER                                                           \
	(RTS5912_INTR_STAT_TX_OVER | RTS5912_INTR_STAT_TX_EMPTY | RTS5912_INTR_STAT_TX_ABRT |      \
	 RTS5912_INTR_STAT_STOP_DET | RTS5912_INTR_STAT_SCL_STUCK_LOW)
#define RTS5912_ENABLE_RX_INT_I2C_MASTER                                                           \
	(RTS5912_INTR_STAT_RX_UNDER | RTS5912_INTR_STAT_RX_OVER | RTS5912_INTR_STAT_RX_FULL |      \
	 RTS5912_INTR_STAT_STOP_DET)

#define RTS5912_ENABLE_TX_INT_I2C_SLAVE                                                            \
	(RTS5912_INTR_STAT_RD_REQ | RTS5912_INTR_STAT_TX_ABRT | RTS5912_INTR_STAT_STOP_DET)
#define RTS5912_ENABLE_RX_INT_I2C_SLAVE (RTS5912_INTR_STAT_RX_FULL | RTS5912_INTR_STAT_STOP_DET)

#define RTS5912_DISABLE_ALL_I2C_INT 0x00000000

/* IC_CON Low count and high count default values */
/* TODO verify values for high and fast speed */
/* 100KHz */
#define I2C_STD_HCNT (CONFIG_I2C_RTS5912_CLOCK_SPEED * 4)
#define I2C_STD_LCNT (CONFIG_I2C_RTS5912_CLOCK_SPEED * 5)
/* 400KHz */
#define I2C_FS_HCNT  (CONFIG_I2C_RTS5912_CLOCK_SPEED)
#define I2C_FS_LCNT  ((CONFIG_I2C_RTS5912_CLOCK_SPEED * 5) / 4)
/* 1MHz */
#define I2C_FSP_HCNT ((CONFIG_I2C_RTS5912_CLOCK_SPEED * 4) / 10)
#define I2C_FSP_LCNT ((CONFIG_I2C_RTS5912_CLOCK_SPEED * 5) / 10)
/* 3.4MHz */
#define I2C_HS_HCNT  ((CONFIG_I2C_RTS5912_CLOCK_SPEED * 6) / 8)
#define I2C_HS_LCNT  ((CONFIG_I2C_RTS5912_CLOCK_SPEED * 7) / 8)

/*
 * DesignWare speed values don't directly translate from the Zephyr speed
 * selections in include/i2c.h so here we do a little translation
 */
#define I2C_RTS5912_SPEED_STANDARD  0x1
#define I2C_RTS5912_SPEED_FAST      0x2
#define I2C_RTS5912_SPEED_FAST_PLUS 0x2
#define I2C_RTS5912_SPEED_HIGH      0x3

/*
 * These values have been randomly selected.  It would be good to test different
 * watermark levels for performance capabilities
 */
#define I2C_RTS5912_TX_WATERMARK 2
#define I2C_RTS5912_RX_WATERMARK 7

struct i2c_rts5912_rom_config {
	DEVICE_MMIO_ROM;
	i2c_isr_cb_t config_func;
	uint32_t bitrate;

	const struct device *clk_dev;
	struct rts5912_sccon_subsys sccon_cfg;
	const struct pinctrl_dev_config *pcfg;
};

struct i2c_rts5912_dev_config {
	DEVICE_MMIO_RAM;
	struct k_sem device_sync_sem;
	struct k_sem bus_sem;
	uint32_t app_config;

	uint8_t *xfr_buf;
	uint32_t xfr_len;
	uint32_t rx_pending;

	uint8_t cnt_req;

	uint16_t hcnt;
	uint16_t lcnt;

	volatile uint8_t state; /* last direction of transfer */
	volatile uint8_t last_state;
	uint8_t request_bytes;
	uint8_t xfr_flags;
	bool support_hs_mode;
#ifdef CONFIG_I2C_CALLBACK
	uint16_t addr;
	uint32_t msg;
	struct i2c_msg *msgs;
	uint32_t msg_left;
	i2c_callback_t cb;
	void *userdata;
#endif /* CONFIG_I2C_CALLBACK */

	struct i2c_target_config *slave_cfg;
	int need_setup;
	int sda_gpio;
	int scl_gpio;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_REALTEK_RTS5912_H_ */
