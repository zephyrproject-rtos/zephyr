/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_i2c

#include <drivers/clock_control.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <drivers/i2c.h>

#define SPEED_100KHZ_BUS    0
#define SPEED_400KHZ_BUS    1
#define SPEED_1MHZ_BUS      2

#define EC_OWN_I2C_ADDR		0x7F
#define RESET_WAIT_US		20

/* I2C timeout is  10 ms (WAIT_INTERVAL * WAIT_COUNT) */
#define WAIT_INTERVAL		50
#define WAIT_COUNT		200

struct xec_speed_cfg {
	uint32_t bus_clk;
	uint32_t data_timing;
	uint32_t start_hold_time;
	uint32_t config;
	uint32_t timeout_scale;
};

struct i2c_xec_config {
	uint32_t port_sel;
	uint32_t base_addr;
};

struct i2c_xec_data {
	uint32_t pending_stop;
	uint32_t speed_id;
};

/* Recommended programming values based on 16MHz
 * i2c_baud_clk_period/bus_clk_period - 2 = (low_period + hi_period)
 * bus_clk_reg (16MHz/100KHz -2) = 0x4F + 0x4F
 *             (16MHz/400KHz -2) = 0x0F + 0x17
 *             (16MHz/1MHz -2) = 0x05 + 0x09
 */
static const struct xec_speed_cfg xec_cfg_params[] = {
	[SPEED_100KHZ_BUS] = {
		.bus_clk            = 0x00004F4F,
		.data_timing        = 0x0C4D5006,
		.start_hold_time    = 0x0000004D,
		.config             = 0x01FC01ED,
		.timeout_scale      = 0x4B9CC2C7,
	},
	[SPEED_400KHZ_BUS] = {
		.bus_clk            = 0x00000F17,
		.data_timing        = 0x040A0A06,
		.start_hold_time    = 0x0000000A,
		.config             = 0x01000050,
		.timeout_scale      = 0x159CC2C7,
	},
	[SPEED_1MHZ_BUS] = {
		.bus_clk            = 0x00000509,
		.data_timing        = 0x04060601,
		.start_hold_time    = 0x00000006,
		.config             = 0x10000050,
		.timeout_scale      = 0x089CC2C7,
	},
};

static void i2c_xec_reset_config(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	uint32_t ba = config->base_addr;

	/* Assert RESET and clr others */
	MCHP_I2C_SMB_CFG(ba) = MCHP_I2C_SMB_CFG_RESET;

	k_busy_wait(RESET_WAIT_US);

	/* Bus reset */
	MCHP_I2C_SMB_CFG(ba) = 0;

	/* Write 0x80. i.e Assert PIN bit, ESO = 0 and Interrupts
	 * disabled (ENI)
	 */
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN;

	/* Enable controller and I2C filters */
	MCHP_I2C_SMB_CFG(ba) = MCHP_I2C_SMB_CFG_GC_EN |
				MCHP_I2C_SMB_CFG_ENAB |
				MCHP_I2C_SMB_CFG_FEN |
				(config->port_sel &
				 MCHP_I2C_SMB_CFG_PORT_SEL_MASK);

	/* Configure bus clock register, Data Timing register,
	 * Repeated Start Hold Time register,
	 * and Timeout Scaling register
	 */
	MCHP_I2C_SMB_BUS_CLK(ba) = xec_cfg_params[data->speed_id].bus_clk;
	MCHP_I2C_SMB_DATA_TM(ba) = xec_cfg_params[data->speed_id].data_timing;
	MCHP_I2C_SMB_RSHT(ba) =
		xec_cfg_params[data->speed_id].start_hold_time;
	MCHP_I2C_SMB_TMTSC(ba) = xec_cfg_params[data->speed_id].timeout_scale;

	/* Set own address */
	MCHP_I2C_SMB_OWN_ADDR(ba) = EC_OWN_I2C_ADDR;

	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				   MCHP_I2C_SMB_CTRL_ESO |
				   MCHP_I2C_SMB_CTRL_ACK;

	k_busy_wait(RESET_WAIT_US);
}

static int xec_spin_yield(int *counter)
{
	*counter = *counter + 1;

	if (*counter > WAIT_COUNT) {
		return -ETIMEDOUT;
	}

	k_busy_wait(WAIT_INTERVAL);

	return 0;
}

static void cleanup_registers(uint32_t ba)
{
	uint32_t cfg = MCHP_I2C_SMB_CFG(ba);

	cfg |= MCHP_I2C_SMB_CFG_FLUSH_MXBUF_WO;
	MCHP_I2C_SMB_CFG(ba) = cfg;
	cfg &= ~MCHP_I2C_SMB_CFG_FLUSH_MXBUF_WO;

	cfg |= MCHP_I2C_SMB_CFG_FLUSH_MRBUF_WO;
	MCHP_I2C_SMB_CFG(ba) = cfg;
	cfg &= ~MCHP_I2C_SMB_CFG_FLUSH_MRBUF_WO;

	cfg |= MCHP_I2C_SMB_CFG_FLUSH_SXBUF_WO;
	MCHP_I2C_SMB_CFG(ba) = cfg;
	cfg &= ~MCHP_I2C_SMB_CFG_FLUSH_SXBUF_WO;

	cfg |= MCHP_I2C_SMB_CFG_FLUSH_SRBUF_WO;
	MCHP_I2C_SMB_CFG(ba) = cfg;
	cfg &= ~MCHP_I2C_SMB_CFG_FLUSH_SRBUF_WO;
}

static void recover_from_error(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	uint32_t ba = config->base_addr;

	cleanup_registers(ba);
	i2c_xec_reset_config(dev);
}

static int wait_bus_free(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	int ret;
	int counter = 0;
	uint32_t ba = config->base_addr;

	while (!(MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_NBB)) {
		ret = xec_spin_yield(&counter);

		if (ret < 0) {
			return ret;
		}
	}

	/* Check for bus error */
	if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_BER) {
		recover_from_error(dev);
		return -EBUSY;
	}

	return 0;
}

static int wait_completion(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	int ret;
	int counter = 0;
	uint32_t ba = config->base_addr;

	/* Wait for transaction to be completed */
	while (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_PIN) {
		ret = xec_spin_yield(&counter);

		if (ret < 0) {
			if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_PIN) {
				recover_from_error(dev);
				return ret;
			}
		}
	}

	/* Check if Slave send ACK/NACK */
	if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_LRB_AD0) {
		recover_from_error(dev);
		return -EIO;
	}

	/* Check for bus error */
	if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_BER) {
		recover_from_error(dev);
		return -EBUSY;
	}

	return 0;
}

static bool check_lines(uint32_t ba)
{
	return ((!(MCHP_I2C_SMB_BB_CTRL(ba) & MCHP_I2C_SMB_BB_CLKI_RO)) ||
		(!(MCHP_I2C_SMB_BB_CTRL(ba) & MCHP_I2C_SMB_BB_DATI_RO)));
}

static int i2c_xec_configure(const struct device *dev,
			     uint32_t dev_config_raw)
{
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	if (!(dev_config_raw & I2C_MODE_MASTER)) {
		return -ENOTSUP;
	}

	if (dev_config_raw & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		data->speed_id = SPEED_100KHZ_BUS;
		break;
	case I2C_SPEED_FAST:
		data->speed_id = SPEED_400KHZ_BUS;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->speed_id = SPEED_1MHZ_BUS;
		break;
	default:
		return -EINVAL;
	}

	i2c_xec_reset_config(dev);

	return 0;
}

static int i2c_xec_poll_write(const struct device *dev, struct i2c_msg msg,
			      uint16_t addr)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	uint32_t ba = config->base_addr;
	int ret;

	if (data->pending_stop == 0) {
		/* Check clock and data lines */
		if (check_lines(ba)) {
			return -EBUSY;
		}

		/* Wait until bus is free */
		ret = wait_bus_free(dev);
		if (ret) {
			return ret;
		}

		/* Send slave address */
		MCHP_I2C_SMB_DATA(ba) = (addr & ~BIT(0));

		/* Send start and ack bits */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				MCHP_I2C_SMB_CTRL_ESO | MCHP_I2C_SMB_CTRL_STA |
				MCHP_I2C_SMB_CTRL_ACK;

		ret = wait_completion(dev);
		if (ret) {
			return ret;
		}
	}

	/* Send bytes */
	for (int i = 0U; i < msg.len; i++) {
		MCHP_I2C_SMB_DATA(ba) = msg.buf[i];
		ret = wait_completion(dev);
		if (ret) {
			return ret;
		}

		/* Handle stop bit for last byte to write */
		if (i == (msg.len - 1)) {
			if (msg.flags & I2C_MSG_STOP) {
				/* Send stop and ack bits */
				MCHP_I2C_SMB_CTRL_WO(ba) =
						MCHP_I2C_SMB_CTRL_PIN |
						MCHP_I2C_SMB_CTRL_ESO |
						MCHP_I2C_SMB_CTRL_STO |
						MCHP_I2C_SMB_CTRL_ACK;
				data->pending_stop = 0;
			} else {
				data->pending_stop = 1;
			}
		}
	}

	return 0;
}

static int i2c_xec_poll_read(const struct device *dev, struct i2c_msg msg,
			     uint16_t addr)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	uint32_t ba = config->base_addr;
	uint8_t byte, ctrl;
	int ret;

	if (!(msg.flags & I2C_MSG_RESTART)) {
		/* Check clock and data lines */
		if (check_lines(ba)) {
			return -EBUSY;
		}

		/* Wait until bus is free */
		ret = wait_bus_free(dev);
		if (ret) {
			return ret;
		}
	}

	/* MCHP I2C spec recommends that for repeated start to write to control
	 * register before writing to data register
	 */
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO |
		MCHP_I2C_SMB_CTRL_STA | MCHP_I2C_SMB_CTRL_ACK;

	/* Send slave address */
	MCHP_I2C_SMB_DATA(ba) = (addr | BIT(0));

	ret = wait_completion(dev);
	if (ret) {
		return ret;
	}

	if (msg.len == 1) {
		/* Send NACK for last transaction */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO;
	}

	/* Read dummy byte */
	byte = MCHP_I2C_SMB_DATA(ba);
	ret = wait_completion(dev);
	if (ret) {
		return ret;
	}

	for (int i = 0U; i < msg.len; i++) {
		while (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_PIN) {
			if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_BER) {
				recover_from_error(dev);
				return -EBUSY;
			}
		}

		if (i == (msg.len - 1)) {
			if (msg.flags & I2C_MSG_STOP) {
				/* Send stop and ack bits */
				ctrl = (MCHP_I2C_SMB_CTRL_PIN |
					MCHP_I2C_SMB_CTRL_ESO |
					MCHP_I2C_SMB_CTRL_STO |
					MCHP_I2C_SMB_CTRL_ACK);
				MCHP_I2C_SMB_CTRL_WO(ba) = ctrl;
				data->pending_stop = 0;
			}
		} else if (i == (msg.len - 2)) {
			/* Send NACK for last transaction */
			MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO;
		}
		msg.buf[i] = MCHP_I2C_SMB_DATA(ba);
	}

	return 0;
}

static int i2c_xec_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t addr)
{
	int ret = 0;

	addr <<= 1;
	for (int i = 0U; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_xec_poll_write(dev, msgs[i], addr);
			if (ret) {
				return ret;
			}
		} else {
			ret = i2c_xec_poll_read(dev, msgs[i], addr);
		}
	}

	return ret;
}

#if defined(CONFIG_I2C_SLAVE)
static int i2c_xec_slave_register(const struct device *dev)
{
	return -ENOTSUP;
}

static int i2c_xec_slave_unregister(const struct device *dev)
{
	return -ENOTSUP;
}
#endif

static int i2c_xec_init(const struct device *dev);

static const struct i2c_driver_api i2c_xec_driver_api = {
	.configure = i2c_xec_configure,
	.transfer = i2c_xec_transfer,
#if defined(CONFIG_I2C_SLAVE)
	.slave_register = i2c_xec_slave_register,
	.slave_unregister = i2c_xec_slave_unregister,
#endif
};

static int i2c_xec_init(const struct device *dev)
{
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	data->pending_stop = 0;
	return 0;
}

#define I2C_XEC_DEVICE(n)						\
	static struct i2c_xec_data i2c_xec_data_##n;			\
	static const struct i2c_xec_config i2c_xec_config_##n = {	\
		.base_addr =						\
			DT_INST_REG_ADDR(n),	\
		.port_sel = DT_INST_PROP(n, port_sel),	\
	};								\
	DEVICE_DT_INST_DEFINE(n, &i2c_xec_init, device_pm_control_nop,	\
		&i2c_xec_data_##n, &i2c_xec_config_##n,			\
		POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,			\
		&i2c_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_XEC_DEVICE)
