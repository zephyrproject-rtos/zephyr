/*
 * Copyright (c) 2025 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lowrisc_opentitan_i2c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(i2c_opentitan, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#define I2C_INTR_STATE_REG_OFFSET        0x00
#define I2C_CTRL_REG_OFFSET              0x10
#define I2C_STATUS_REG_OFFSET            0x14
#define I2C_RDATA_REG_OFFSET             0x18
#define I2C_FDATA_REG_OFFSET             0x1C
#define I2C_FIFO_CTRL_REG_OFFSET         0x20
#define I2C_TIMING0_REG_OFFSET           0x3C
#define I2C_TIMING1_REG_OFFSET           0x40
#define I2C_TIMING2_REG_OFFSET           0x44
#define I2C_TIMING3_REG_OFFSET           0x48
#define I2C_TIMING4_REG_OFFSET           0x4C
#define I2C_CONTROLLER_EVENTS_REG_OFFSET 0x78

#define I2C_INTR_STATE_RX_OVERFLOW_BIT      BIT(3)
#define I2C_INTR_STATE_CONTROLLER_HALT_BIT  BIT(4)
#define I2C_INTR_STATE_SCL_INTERFERENCE_BIT BIT(5)
#define I2C_INTR_STATE_SDA_INTERFERENCE_BIT BIT(6)
#define I2C_INTR_STATE_STRETCH_TIMEOUT_BIT  BIT(7)
#define I2C_INTR_STATE_SDA_UNSTABLE_BIT     BIT(8)
#define I2C_INTR_STATE_ERROR_MASK                                                                  \
	(I2C_INTR_STATE_RX_OVERFLOW_BIT | I2C_INTR_STATE_CONTROLLER_HALT_BIT |                     \
	 I2C_INTR_STATE_SCL_INTERFERENCE_BIT | I2C_INTR_STATE_SDA_INTERFERENCE_BIT |               \
	 I2C_INTR_STATE_STRETCH_TIMEOUT_BIT | I2C_INTR_STATE_SDA_UNSTABLE_BIT)

#define I2C_CTRL_ENABLEHOST_BIT BIT(0)

#define I2C_STATUS_FMTFULL_BIT  BIT(0)
#define I2C_STATUS_FMTEMPTY_BIT BIT(2)
#define I2C_STATUS_HOSTIDLE_BIT BIT(3)
#define I2C_STATUS_RXEMPTY_BIT  BIT(5)

#define I2C_FDATA_START_BIT BIT(8)
#define I2C_FDATA_STOP_BIT  BIT(9)
#define I2C_FDATA_READB_BIT BIT(10)
#define I2C_FDATA_RCONT_BIT BIT(11)
/* Reads are chunked to the RX FIFO depth because the controller drops bytes once it is full. */
#define I2C_RX_FIFO_DEPTH   64

#define I2C_FIFO_CTRL_RXRST_BIT  BIT(0)
#define I2C_FIFO_CTRL_FMTRST_BIT BIT(1)
#define I2C_FIFO_CTRL_ACQRST_BIT BIT(7)
#define I2C_FIFO_CTRL_TXRST_BIT  BIT(8)

/* NACK, UNHANDLED_NACK_TIMEOUT, BUS_TIMEOUT and ARBITRATION_LOST, all write-1-to-clear. */
#define I2C_CONTROLLER_EVENTS_MASK BIT_MASK(4)

/* Each timing register packs two cycle counts into its low and high halfwords. */
#define I2C_TIMING_REG(hi, lo) (((uint32_t)(hi) << 16) | (uint32_t)(lo))

/* Minimum data hold time, in input clock cycles. */
#define I2C_THD_DAT_CYCLES 1

/* Returns number of clock cycles needed for ns to elapse @ clk_khz rate, in 32-bit math. */
#define CYCLES_FROM_NS(ns, clk_khz) DIV_ROUND_UP((ns) * (clk_khz), USEC_PER_SEC)

/* Keeps CYCLES_FROM_NS and the 13-bit timing register fields from overflowing. */
#define I2C_OPENTITAN_MAX_CLOCK_HZ MHZ(400)

/* Bus time per byte, counting the ACK bit and some inter-byte slack. */
#define I2C_BITS_PER_BYTE 10

struct i2c_opentitan_cfg {
	uint32_t base;
	uint32_t f_sys;
	uint32_t f_bus;
};

struct i2c_opentitan_data {
	struct k_mutex lock;
	uint32_t dev_config;
	/* Nominal bus time for one byte at the configured speed. */
	uint32_t byte_usec;
};

/* Bus timing requirements in nanoseconds, from table 10 of the I2C specification (rev. 6). */
struct i2c_opentitan_timing {
	uint16_t thigh;
	uint16_t tlow;
	uint16_t tf;
	uint16_t tr;
	uint16_t thd_sta;
	uint16_t tsu_sta;
	uint16_t tsu_dat;
	uint16_t tbuf;
	uint16_t tsto;
	uint16_t period;
};

static const struct i2c_opentitan_timing i2c_opentitan_timing_standard = {
	.thigh = 4000,
	.tlow = 4700,
	.tf = 300,
	.tr = 1000,
	.thd_sta = 4000,
	.tsu_sta = 4700,
	.tsu_dat = 250,
	.tbuf = 4700,
	.tsto = 4000,
	.period = 10000,
};

static const struct i2c_opentitan_timing i2c_opentitan_timing_fast = {
	.thigh = 600,
	.tlow = 1300,
	.tf = 300,
	.tr = 300,
	.thd_sta = 600,
	.tsu_sta = 600,
	.tsu_dat = 100,
	.tbuf = 1300,
	.tsto = 600,
	.period = 2500,
};

static const struct i2c_opentitan_timing i2c_opentitan_timing_fast_plus = {
	.thigh = 260,
	.tlow = 500,
	.tf = 120,
	.tr = 120,
	.thd_sta = 260,
	.tsu_sta = 260,
	.tsu_dat = 50,
	.tbuf = 500,
	.tsto = 260,
	.period = 1000,
};

static const struct i2c_opentitan_timing *i2c_opentitan_timing_for_speed(uint32_t speed)
{
	switch (speed) {
	case I2C_SPEED_STANDARD:
		return &i2c_opentitan_timing_standard;
	case I2C_SPEED_FAST:
		return &i2c_opentitan_timing_fast;
	case I2C_SPEED_FAST_PLUS:
		return &i2c_opentitan_timing_fast_plus;
	default:
		return NULL;
	}
}

static void i2c_opentitan_reset_fifos(const struct device *dev)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;

	sys_write32(I2C_FIFO_CTRL_RXRST_BIT | I2C_FIFO_CTRL_FMTRST_BIT | I2C_FIFO_CTRL_ACQRST_BIT |
			    I2C_FIFO_CTRL_TXRST_BIT,
		    cfg->base + I2C_FIFO_CTRL_REG_OFFSET);
}

/* Clears latched errors; the controller stays halted until every CONTROLLER_EVENTS bit is clear. */
static void i2c_opentitan_clear_errors(const struct device *dev)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;

	sys_write32(I2C_CONTROLLER_EVENTS_MASK, cfg->base + I2C_CONTROLLER_EVENTS_REG_OFFSET);
	sys_write32(I2C_INTR_STATE_ERROR_MASK, cfg->base + I2C_INTR_STATE_REG_OFFSET);
}

static bool i2c_opentitan_error(const struct device *dev)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;

	return sys_read32(cfg->base + I2C_INTR_STATE_REG_OFFSET) & I2C_INTR_STATE_ERROR_MASK;
}

static bool i2c_opentitan_status_matches(const struct device *dev, uint32_t bit, bool set)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;
	bool is_set = sys_read32(cfg->base + I2C_STATUS_REG_OFFSET) & bit;

	return is_set == set;
}

static bool i2c_opentitan_status_matches_or_error(const struct device *dev, uint32_t bit, bool set)
{
	return i2c_opentitan_status_matches(dev, bit, set) || i2c_opentitan_error(dev);
}

/* Polls until the STATUS bit has the requested value, a bus error is flagged, or time runs out. */
static int i2c_opentitan_wait_status(const struct device *dev, uint32_t bit, bool set,
				     uint32_t timeout_usec)
{
	if (!WAIT_FOR(i2c_opentitan_status_matches_or_error(dev, bit, set), timeout_usec, NULL)) {
		LOG_ERR("Timeout waiting for STATUS bit 0x%x to be %d", bit, set);
		return -ETIMEDOUT;
	}

	return i2c_opentitan_error(dev) ? -EIO : 0;
}

/* Ends the transaction in progress and returns the controller to an idle, enabled state. */
static void i2c_opentitan_recover(const struct device *dev)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;

	LOG_ERR("Transfer failed: INTR_STATE 0x%x CONTROLLER_EVENTS 0x%x",
		sys_read32(cfg->base + I2C_INTR_STATE_REG_OFFSET),
		sys_read32(cfg->base + I2C_CONTROLLER_EVENTS_REG_OFFSET));

	/* Clearing ENABLEHOST makes the controller issue a STOP, even while halted on a NACK. */
	sys_write32(0, cfg->base + I2C_CTRL_REG_OFFSET);
	if (!WAIT_FOR(i2c_opentitan_status_matches(dev, I2C_STATUS_HOSTIDLE_BIT, true),
		      CONFIG_I2C_OPENTITAN_TIMEOUT_US, NULL)) {
		LOG_WRN("Controller did not return to idle");
	}

	i2c_opentitan_reset_fifos(dev);
	i2c_opentitan_clear_errors(dev);
	sys_write32(I2C_CTRL_ENABLEHOST_BIT, cfg->base + I2C_CTRL_REG_OFFSET);
}

static int i2c_opentitan_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;
	struct i2c_opentitan_data *data = dev->data;
	const struct i2c_opentitan_timing *t =
		i2c_opentitan_timing_for_speed(I2C_SPEED_GET(dev_config));
	uint32_t sys_khz = cfg->f_sys / KHZ(1);
	uint32_t thigh, tlow, tf, tr, period;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("I2C only supports operation as controller");
		return -ENOTSUP;
	}

	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("I2C driver does not support 10-bit addresses");
		return -ENOTSUP;
	}

	if (t == NULL) {
		LOG_ERR("Unsupported I2C speed requested");
		return -ENOTSUP;
	}

	thigh = CYCLES_FROM_NS(t->thigh, sys_khz);
	tlow = CYCLES_FROM_NS(t->tlow, sys_khz);
	tf = CYCLES_FROM_NS(t->tf, sys_khz);
	tr = CYCLES_FROM_NS(t->tr, sys_khz);
	period = CYCLES_FROM_NS(t->period, sys_khz);

	/* Stretch the high period so the SCL period does not exceed the bus speed. */
	if (period > thigh + tlow + tf + tr) {
		thigh += period - (thigh + tlow + tf + tr);
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Disable the I2C controller during configuration */
	sys_write32(0, cfg->base + I2C_CTRL_REG_OFFSET);

	sys_write32(I2C_TIMING_REG(tlow, thigh), cfg->base + I2C_TIMING0_REG_OFFSET);
	sys_write32(I2C_TIMING_REG(tf, tr), cfg->base + I2C_TIMING1_REG_OFFSET);
	sys_write32(I2C_TIMING_REG(CYCLES_FROM_NS(t->thd_sta, sys_khz),
				   CYCLES_FROM_NS(t->tsu_sta, sys_khz)),
		    cfg->base + I2C_TIMING2_REG_OFFSET);
	sys_write32(I2C_TIMING_REG(I2C_THD_DAT_CYCLES, CYCLES_FROM_NS(t->tsu_dat, sys_khz)),
		    cfg->base + I2C_TIMING3_REG_OFFSET);
	sys_write32(
		I2C_TIMING_REG(CYCLES_FROM_NS(t->tbuf, sys_khz), CYCLES_FROM_NS(t->tsto, sys_khz)),
		cfg->base + I2C_TIMING4_REG_OFFSET);

	i2c_opentitan_reset_fifos(dev);
	i2c_opentitan_clear_errors(dev);

	sys_write32(I2C_CTRL_ENABLEHOST_BIT, cfg->base + I2C_CTRL_REG_OFFSET);
	data->dev_config = dev_config;
	data->byte_usec = DIV_ROUND_UP(I2C_BITS_PER_BYTE * t->period, NSEC_PER_USEC);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int i2c_opentitan_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_opentitan_data *data = dev->data;

	*dev_config = data->dev_config;

	return 0;
}

/* Queues one FMT FIFO entry, waiting for space if the FIFO is full. */
static int i2c_opentitan_push_fmt(const struct device *dev, uint32_t fmt)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;
	const struct i2c_opentitan_data *data = dev->data;
	int rc;

	rc = i2c_opentitan_wait_status(dev, I2C_STATUS_FMTFULL_BIT, false,
				       CONFIG_I2C_OPENTITAN_TIMEOUT_US + data->byte_usec);
	if (rc != 0) {
		return rc;
	}
	sys_write32(fmt, cfg->base + I2C_FDATA_REG_OFFSET);

	return 0;
}

static int i2c_opentitan_write_bytes(const struct device *dev, const uint8_t *buf, uint32_t len,
				     bool stop)
{
	int rc;

	for (uint32_t i = 0; i < len; i++) {
		uint32_t fmt = buf[i];

		if (stop && i == len - 1) {
			fmt |= I2C_FDATA_STOP_BIT;
		}
		rc = i2c_opentitan_push_fmt(dev, fmt);
		if (rc != 0) {
			return rc;
		}
	}

	return 0;
}

/* Reads len bytes; rcont makes the controller ACK the final byte so a later read can continue. */
static int i2c_opentitan_read_bytes(const struct device *dev, uint8_t *buf, uint32_t len, bool stop,
				    bool rcont)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;
	const struct i2c_opentitan_data *data = dev->data;
	uint32_t offset = 0;
	int rc;

	while (offset < len) {
		uint32_t chunk = MIN(len - offset, I2C_RX_FIFO_DEPTH);
		bool last = (offset + chunk) == len;
		uint32_t fmt = chunk | I2C_FDATA_READB_BIT;

		if (!last || rcont) {
			fmt |= I2C_FDATA_RCONT_BIT;
		} else if (stop) {
			fmt |= I2C_FDATA_STOP_BIT;
		}
		rc = i2c_opentitan_push_fmt(dev, fmt);
		if (rc != 0) {
			return rc;
		}

		for (uint32_t i = 0; i < chunk; i++) {
			rc = i2c_opentitan_wait_status(dev, I2C_STATUS_RXEMPTY_BIT, false,
						       CONFIG_I2C_OPENTITAN_TIMEOUT_US +
							       data->byte_usec);
			if (rc != 0) {
				return rc;
			}
			buf[offset + i] = sys_read32(cfg->base + I2C_RDATA_REG_OFFSET);
		}
		offset += chunk;
	}

	return 0;
}

/* A message needs a START and address unless it continues the previous message's transaction. */
static bool i2c_opentitan_msg_starts(const struct i2c_msg *prev, const struct i2c_msg *msg)
{
	return prev == NULL || (prev->flags & I2C_MSG_STOP) || (msg->flags & I2C_MSG_RESTART) ||
	       ((prev->flags ^ msg->flags) & I2C_MSG_RW_MASK);
}

static int i2c_opentitan_check_msgs(const struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr)
{
	if (addr > 0x7F) {
		LOG_ERR("Invalid 7-bit address 0x%x", addr);
		return -EINVAL;
	}

	for (uint8_t i = 0; i < num_msgs; i++) {
		const struct i2c_msg *msg = &msgs[i];
		const struct i2c_msg *prev = (i > 0) ? &msgs[i - 1] : NULL;

		if (msg->flags & I2C_MSG_ADDR_10_BITS) {
			LOG_ERR("I2C driver does not support 10-bit addresses");
			return -ENOTSUP;
		}

		/* The hardware cannot issue a bare STOP, and a STOP right after a read address can
		 * hang if the target starts driving data.
		 */
		if (msg->len == 0 &&
		    ((msg->flags & I2C_MSG_READ) || !i2c_opentitan_msg_starts(prev, msg))) {
			LOG_ERR("Empty message must be a write that addresses the target");
			return -EINVAL;
		}
	}

	return 0;
}

static int i2c_opentitan_transfer_msg(const struct device *dev, const struct i2c_msg *msg,
				      const struct i2c_msg *prev, const struct i2c_msg *next,
				      uint16_t addr)
{
	const struct i2c_opentitan_data *data = dev->data;
	bool read = msg->flags & I2C_MSG_READ;
	bool stop = msg->flags & I2C_MSG_STOP;
	bool start = i2c_opentitan_msg_starts(prev, msg);
	/* A following read without RESTART continues this read, so its last byte needs an ACK. */
	bool rcont = !stop && next != NULL && (next->flags & I2C_MSG_READ) &&
		     !(next->flags & I2C_MSG_RESTART);
	int rc;

	if (start) {
		uint32_t fmt = (addr << 1) | (read ? 1 : 0) | I2C_FDATA_START_BIT;

		/* An empty message has no data byte to carry the STOP. */
		if (stop && msg->len == 0) {
			fmt |= I2C_FDATA_STOP_BIT;
		}
		rc = i2c_opentitan_push_fmt(dev, fmt);
		if (rc != 0) {
			return rc;
		}
	}

	if (read) {
		rc = i2c_opentitan_read_bytes(dev, msg->buf, msg->len, stop, rcont);
	} else {
		rc = i2c_opentitan_write_bytes(dev, msg->buf, msg->len, stop);
	}
	if (rc != 0) {
		return rc;
	}

	/* Entries pop only after their ACK bit, so an empty FMT FIFO means the message is done. */
	return i2c_opentitan_wait_status(dev, I2C_STATUS_FMTEMPTY_BIT, true,
					 CONFIG_I2C_OPENTITAN_TIMEOUT_US +
						 (msg->len + 1) * data->byte_usec);
}

static int i2c_opentitan_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				  uint16_t addr)
{
	struct i2c_opentitan_data *data = dev->data;
	int rc;

	rc = i2c_opentitan_check_msgs(msgs, num_msgs, addr);
	if (rc != 0) {
		return rc;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (uint8_t i = 0; i < num_msgs && rc == 0; i++) {
		const struct i2c_msg *prev = (i > 0) ? &msgs[i - 1] : NULL;
		const struct i2c_msg *next = (i + 1 < num_msgs) ? &msgs[i + 1] : NULL;

		rc = i2c_opentitan_transfer_msg(dev, &msgs[i], prev, next, addr);
	}

	if (rc != 0) {
		i2c_opentitan_recover(dev);
	}

	k_mutex_unlock(&data->lock);

	return rc;
}

static int i2c_opentitan_init(const struct device *dev)
{
	const struct i2c_opentitan_cfg *cfg = dev->config;
	struct i2c_opentitan_data *data = dev->data;
	int rc;

	k_mutex_init(&data->lock);

	rc = i2c_opentitan_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(cfg->f_bus));
	if (rc != 0) {
		LOG_ERR("Failed to configure I2C on init");
	}

	return rc;
}

static DEVICE_API(i2c, i2c_opentitan_api) = {
	.configure = i2c_opentitan_configure,
	.get_config = i2c_opentitan_get_config,
	.transfer = i2c_opentitan_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_OPENTITAN_INIT(n)                                                                      \
	BUILD_ASSERT(DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency) <=                        \
		     I2C_OPENTITAN_MAX_CLOCK_HZ);                                                  \
	static const struct i2c_opentitan_cfg i2c_opentitan_cfg_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.f_sys = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),                      \
		.f_bus = DT_INST_PROP(n, clock_frequency),                                         \
	};                                                                                         \
	static struct i2c_opentitan_data i2c_opentitan_data_##n;                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_opentitan_init, NULL, &i2c_opentitan_data_##n,            \
				  &i2c_opentitan_cfg_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,   \
				  &i2c_opentitan_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_OPENTITAN_INIT)
