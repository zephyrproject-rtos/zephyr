/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_i2c

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_mchp, CONFIG_I2C_LOG_LEVEL);

#define SPEED_100KHZ_BUS    0
#define SPEED_400KHZ_BUS    1
#define SPEED_1MHZ_BUS      2

#define EC_OWN_I2C_ADDR		0x7F
#define RESET_WAIT_US		20
#define BUS_IDLE_US_DFLT	5

/* I2C timeout is 10 ms (WAIT_INTERVAL * WAIT_COUNT) */
#define WAIT_INTERVAL		50
#define WAIT_COUNT		200

/* Line High Timeout is 2.5 ms (WAIT_LINE_HIGH_USEC * WAIT_LINE_HIGH_COUNT) */
#define WAIT_LINE_HIGH_USEC	25
#define WAIT_LINE_HIGH_COUNT	100

/* I2C Read/Write bit pos */
#define I2C_READ_WRITE_POS  0

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
	uint8_t girq_id;
	uint8_t girq_bit;
	uint8_t pcr_idx;
	uint8_t pcr_bitpos;
	struct gpio_dt_spec sda_gpio;
	struct gpio_dt_spec scl_gpio;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

struct i2c_xec_data {
	uint32_t pending_stop;
	uint32_t error_seen;
	uint32_t timeout_seen;
	uint32_t previously_in_read;
	uint32_t speed_id;
	struct i2c_target_config *slave_cfg;
	bool slave_attached;
	bool slave_read;
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

	/* Assert RESET */
	z_mchp_xec_pcr_periph_reset(config->pcr_idx, config->pcr_bitpos);
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

#ifdef CONFIG_I2C_TARGET
static void restart_slave(uint32_t ba)
{
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				   MCHP_I2C_SMB_CTRL_ESO |
				   MCHP_I2C_SMB_CTRL_ACK |
				   MCHP_I2C_SMB_CTRL_ENI;
}
#endif

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

/*
 * Wait with timeout for I2C controller to finish transmit/receive of one
 * byte(address or data).
 * When transmit/receive operation is started the I2C PIN status is 1. Upon
 * normal completion I2C PIN status asserts(0).
 * We loop checking I2C status for the following events:
 * Bus Error:
 *      Reset controller and return -EBUSY
 * Lost Arbitration:
 *      Return -EPERM. We lost bus to another controller. No reset.
 * PIN == 0: I2C Status LRB is valid and contains ACK/NACK data on 9th clock.
 *      ACK return 0 (success)
 *      NACK Issue STOP, wait for bus minimum idle time, return -EIO.
 * Timeout:
 *      Reset controller and return -ETIMEDOUT
 *
 * NOTE: After generating a STOP the controller will not generate a START until
 * Bus Minimum Idle time has expired.
 */
static int wait_completion(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	int ret;
	int counter = 0;
	uint32_t ba = config->base_addr;

	while (1) {
		uint8_t status = MCHP_I2C_SMB_STS_RO(ba);

		/* Is bus error ? */
		if (status & MCHP_I2C_SMB_STS_BER) {
			recover_from_error(dev);
			return -EBUSY;
		}

		/* Is Lost arbitration ? */
		status = MCHP_I2C_SMB_STS_RO(ba);
		if (status & MCHP_I2C_SMB_STS_LAB) {
			recover_from_error(dev);
			return -EPERM;
		}

		status = MCHP_I2C_SMB_STS_RO(ba);
		/* PIN -> 0 indicates I2C is done */
		if (!(status & MCHP_I2C_SMB_STS_PIN)) {
			/* PIN == 0. LRB contains state of 9th bit */
			if (status & MCHP_I2C_SMB_STS_LRB_AD0) { /* NACK? */
				/* Send STOP */
				MCHP_I2C_SMB_CTRL_WO(ba) =
						MCHP_I2C_SMB_CTRL_PIN |
						MCHP_I2C_SMB_CTRL_ESO |
						MCHP_I2C_SMB_CTRL_STO |
						MCHP_I2C_SMB_CTRL_ACK;
				k_busy_wait(BUS_IDLE_US_DFLT);
				return -EIO;
			}
			break; /* success: ACK */
		}

		ret = xec_spin_yield(&counter);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/*
 * Call GPIO driver to read state of pins.
 * Return boolean true if both lines HIGH else return boolean false
 */
static bool check_lines_high(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const)(dev->config);
	gpio_port_value_t sda = 0, scl = 0;

	if (gpio_port_get_raw(config->sda_gpio.port, &sda)) {
		LOG_ERR("gpio_port_get_raw for %s SDA failed", dev->name);
		return false;
	}

	/* both pins could be on same GPIO group */
	if (config->sda_gpio.port == config->scl_gpio.port) {
		scl = sda;
	} else {
		if (gpio_port_get_raw(config->scl_gpio.port, &scl)) {
			LOG_ERR("gpio_port_get_raw for %s SCL failed",
				dev->name);
			return false;
		}
	}

	return (sda & BIT(config->sda_gpio.pin)) && (scl & BIT(config->scl_gpio.pin));

}

static int i2c_xec_configure(const struct device *dev,
			     uint32_t dev_config_raw)
{
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	if (!(dev_config_raw & I2C_MODE_CONTROLLER)) {
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
	uint8_t i2c_timer = 0, byte;
	int ret;

	if (data->timeout_seen == 1) {
		/* Wait to see if the slave has released the CLK */
		ret = wait_completion(dev);
		if (ret) {
			data->timeout_seen = 1;
			LOG_ERR("%s: %s wait_completion failure %d\n",
				__func__, dev->name, ret);
			return ret;
		}
		data->timeout_seen = 0;

		/* If we are here, it means the slave has finally released
		 * the CLK. The master needs to end that transaction
		 * gracefully by sending a STOP on the bus.
		 */
		LOG_DBG("%s: %s Force Stop", __func__, dev->name);
		MCHP_I2C_SMB_CTRL_WO(ba) =
					MCHP_I2C_SMB_CTRL_PIN |
					MCHP_I2C_SMB_CTRL_ESO |
					MCHP_I2C_SMB_CTRL_STO |
					MCHP_I2C_SMB_CTRL_ACK;
		k_busy_wait(BUS_IDLE_US_DFLT);
		data->pending_stop = 0;

		/* If the timeout had occurred while the master was reading
		 * something from the slave, that read needs to be completed
		 * to clear the bus.
		 */
		if (data->previously_in_read == 1) {
			data->previously_in_read = 0;
			byte = MCHP_I2C_SMB_DATA(ba);
		}
		return -EBUSY;
	}

	if ((data->pending_stop == 0) || (data->error_seen == 1)) {
		/* Wait till clock and data lines are HIGH */
		while (check_lines_high(dev) == false) {
			if (i2c_timer >= WAIT_LINE_HIGH_COUNT) {
				LOG_DBG("%s: %s not high",
					__func__, dev->name);
				data->error_seen = 1;
				return -EBUSY;
			}
			k_busy_wait(WAIT_LINE_HIGH_USEC);
			i2c_timer++;
		}

		if (data->error_seen) {
			LOG_DBG("%s: Recovering %s previously in error",
				__func__, dev->name);
			data->error_seen = 0;
			recover_from_error(dev);
		}

		/* Wait until bus is free */
		ret = wait_bus_free(dev);
		if (ret) {
			data->error_seen = 1;
			LOG_DBG("%s: %s wait_bus_free failure %d",
				__func__, dev->name, ret);
			return ret;
		}

		/* Send slave address */
		MCHP_I2C_SMB_DATA(ba) = (addr & ~BIT(0));

		/* Send start and ack bits */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				MCHP_I2C_SMB_CTRL_ESO | MCHP_I2C_SMB_CTRL_STA |
				MCHP_I2C_SMB_CTRL_ACK;

		ret = wait_completion(dev);
		switch (ret) {
		case 0:	/* Success */
			break;

		case -EIO:
			LOG_WRN("%s: No Addr ACK from Slave 0x%x on %s",
				__func__, addr >> 1, dev->name);
			return ret;

		default:
			data->error_seen = 1;
			LOG_ERR("%s: %s wait_comp error %d for addr send",
				__func__, dev->name, ret);
			return ret;
		}
	}

	/* Send bytes */
	for (int i = 0U; i < msg.len; i++) {
		MCHP_I2C_SMB_DATA(ba) = msg.buf[i];
		ret = wait_completion(dev);

		switch (ret) {
		case 0:	/* Success */
			break;

		case -EIO:
			LOG_ERR("%s: No Data ACK from Slave 0x%x on %s",
				__func__, addr >> 1, dev->name);
			return ret;

		case -ETIMEDOUT:
			data->timeout_seen = 1;
			LOG_ERR("%s: Clk stretch Timeout - Slave 0x%x on %s",
				__func__, addr >> 1, dev->name);
			return ret;

		default:
			data->error_seen = 1;
			LOG_ERR("%s: %s wait_completion error %d for data send",
				__func__, dev->name, ret);
			return ret;
		}
	}

	/* Handle stop bit for last byte to write */
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
	uint8_t byte, ctrl, i2c_timer = 0;
	int ret;

	if (data->timeout_seen == 1) {
		/* Wait to see if the slave has released the CLK */
		ret = wait_completion(dev);
		if (ret) {
			data->timeout_seen = 1;
			LOG_ERR("%s: %s wait_completion failure %d\n",
				__func__, dev->name, ret);
			return ret;
		}
		data->timeout_seen = 0;

		/* If we are here, it means the slave has finally released
		 * the CLK. The master needs to end that transaction
		 * gracefully by sending a STOP on the bus.
		 */
		LOG_DBG("%s: %s Force Stop", __func__, dev->name);
		MCHP_I2C_SMB_CTRL_WO(ba) =
					MCHP_I2C_SMB_CTRL_PIN |
					MCHP_I2C_SMB_CTRL_ESO |
					MCHP_I2C_SMB_CTRL_STO |
					MCHP_I2C_SMB_CTRL_ACK;
		k_busy_wait(BUS_IDLE_US_DFLT);
		return -EBUSY;
	}

	if (!(msg.flags & I2C_MSG_RESTART) || (data->error_seen == 1)) {
		/* Wait till clock and data lines are HIGH */
		while (check_lines_high(dev) == false) {
			if (i2c_timer >= WAIT_LINE_HIGH_COUNT) {
				LOG_DBG("%s: %s not high",
					__func__, dev->name);
				data->error_seen = 1;
				return -EBUSY;
			}
			k_busy_wait(WAIT_LINE_HIGH_USEC);
			i2c_timer++;
		}

		if (data->error_seen) {
			LOG_DBG("%s: Recovering %s previously in error",
				__func__, dev->name);
			data->error_seen = 0;
			recover_from_error(dev);
		}

		/* Wait until bus is free */
		ret = wait_bus_free(dev);
		if (ret) {
			data->error_seen = 1;
			LOG_DBG("%s: %s wait_bus_free failure %d",
				__func__, dev->name, ret);
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
	switch (ret) {
	case 0:	/* Success */
		break;

	case -EIO:
		data->error_seen = 1;
		LOG_WRN("%s: No Addr ACK from Slave 0x%x on %s",
			__func__, addr >> 1, dev->name);
		return ret;

	case -ETIMEDOUT:
		data->previously_in_read = 1;
		data->timeout_seen = 1;
		LOG_ERR("%s: Clk stretch Timeout - Slave 0x%x on %s",
			__func__, addr >> 1, dev->name);
		return ret;

	default:
		data->error_seen = 1;
		LOG_ERR("%s: %s wait_completion error %d for address send",
			__func__, dev->name, ret);
		return ret;
	}

	if (msg.len == 1) {
		/* Send NACK for last transaction */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO;
	}

	/* Read dummy byte */
	byte = MCHP_I2C_SMB_DATA(ba);

	for (int i = 0U; i < msg.len; i++) {
		ret = wait_completion(dev);
		switch (ret) {
		case 0:	/* Success */
			break;

		case -EIO:
			LOG_ERR("%s: No Data ACK from Slave 0x%x on %s",
				__func__, addr >> 1, dev->name);
			return ret;

		case -ETIMEDOUT:
			data->previously_in_read = 1;
			data->timeout_seen = 1;
			LOG_ERR("%s: Clk stretch Timeout - Slave 0x%x on %s",
				__func__, addr >> 1, dev->name);
			return ret;

		default:
			data->error_seen = 1;
			LOG_ERR("%s: %s wait_completion error %d for data send",
				__func__, dev->name, ret);
			return ret;
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

#ifdef CONFIG_I2C_TARGET
	struct i2c_xec_data *data = dev->data;

	if (data->slave_attached) {
		LOG_ERR("%s Device is registered as slave", dev->name);
		return -EBUSY;
	}
#endif

	addr <<= 1;
	for (int i = 0U; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_xec_poll_write(dev, msgs[i], addr);
			if (ret) {
				LOG_ERR("%s Write error: %d", dev->name, ret);
				return ret;
			}
		} else {
			ret = i2c_xec_poll_read(dev, msgs[i], addr);
			if (ret) {
				LOG_ERR("%s Read error: %d", dev->name, ret);
				return ret;
			}
		}
	}

	return 0;
}

static void i2c_xec_bus_isr(const struct device *dev)
{
#ifdef CONFIG_I2C_TARGET
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data = dev->data;
	const struct i2c_target_callbacks *slave_cb = data->slave_cfg->callbacks;
	uint32_t ba = config->base_addr;

	uint32_t status;
	uint8_t val;

	uint8_t dummy = 0U;

	if (!data->slave_attached) {
		return;
	}

	/* Get current status */
	status = MCHP_I2C_SMB_STS_RO(ba);

	/* Bus Error */
	if (status & MCHP_I2C_SMB_STS_BER) {
		if (slave_cb->stop) {
			slave_cb->stop(data->slave_cfg);
		}
		restart_slave(ba);
		goto clear_iag;
	}

	/* External stop */
	if (status & MCHP_I2C_SMB_STS_EXT_STOP) {
		if (slave_cb->stop) {
			slave_cb->stop(data->slave_cfg);
		}
		dummy = MCHP_I2C_SMB_DATA(ba);
		restart_slave(ba);
		goto clear_iag;
	}

	/* Address byte handling */
	if (status & MCHP_I2C_SMB_STS_AAS) {
		uint8_t slv_data = MCHP_I2C_SMB_DATA(ba);

		if (!(slv_data & BIT(I2C_READ_WRITE_POS))) {
			/* Slave receive  */
			data->slave_read = false;
			if (slave_cb->write_requested) {
				slave_cb->write_requested(data->slave_cfg);
			}
			goto clear_iag;
		} else {
			/* Slave transmit */
			data->slave_read = true;
			if (slave_cb->read_requested) {
				slave_cb->read_requested(data->slave_cfg, &val);
			}
			MCHP_I2C_SMB_DATA(ba) = val;
			goto clear_iag;
		}
	}

	/* Slave transmit */
	if (data->slave_read) {
		/* Master has Nacked, then just write a dummy byte */
		if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_LRB_AD0) {
			MCHP_I2C_SMB_DATA(ba) = dummy;
		} else {
			if (slave_cb->read_processed) {
				slave_cb->read_processed(data->slave_cfg, &val);
			}
			MCHP_I2C_SMB_DATA(ba) = val;
		}
	} else {
		val = MCHP_I2C_SMB_DATA(ba);
		/* TODO NACK Master */
		if (slave_cb->write_received) {
			slave_cb->write_received(data->slave_cfg, val);
		}
	}

clear_iag:
	MCHP_GIRQ_SRC(config->girq_id) = BIT(config->girq_bit);
#endif
}

#ifdef CONFIG_I2C_TARGET
static int i2c_xec_target_register(const struct device *dev,
				  struct i2c_target_config *config)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data = dev->data;
	uint32_t ba = cfg->base_addr;
	int ret;
	int counter = 0;

	if (!config) {
		return -EINVAL;
	}

	if (data->slave_attached) {
		return -EBUSY;
	}

	/* Wait for any outstanding transactions to complete so that
	 * the bus is free
	 */
	while (!(MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_NBB)) {
		ret = xec_spin_yield(&counter);

		if (ret < 0) {
			return ret;
		}
	}

	data->slave_cfg = config;

	/* Set own address */
	MCHP_I2C_SMB_OWN_ADDR(ba) = data->slave_cfg->address;
	restart_slave(ba);

	data->slave_attached = true;

	/* Clear before enabling girq bit */
	MCHP_GIRQ_SRC(cfg->girq_id) = BIT(cfg->girq_bit);
	MCHP_GIRQ_ENSET(cfg->girq_id) = BIT(cfg->girq_bit);

	return 0;
}

static int i2c_xec_target_unregister(const struct device *dev,
				    struct i2c_target_config *config)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data = dev->data;

	if (!data->slave_attached) {
		return -EINVAL;
	}

	data->slave_attached = false;

	MCHP_GIRQ_ENCLR(cfg->girq_id) = BIT(cfg->girq_bit);

	return 0;
}
#endif

static const struct i2c_driver_api i2c_xec_driver_api = {
	.configure = i2c_xec_configure,
	.transfer = i2c_xec_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_xec_target_register,
	.target_unregister = i2c_xec_target_unregister,
#endif
};

static int i2c_xec_init(const struct device *dev)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	int ret;

	data->pending_stop = 0;
	data->slave_attached = false;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC I2C pinctrl setup failed (%d)", ret);
		return ret;
	}

	if (!gpio_is_ready_dt(&cfg->sda_gpio)) {
		LOG_ERR("%s GPIO device is not ready for SDA GPIO", dev->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->scl_gpio)) {
		LOG_ERR("%s GPIO device is not ready for SCL GPIO", dev->name);
		return -ENODEV;
	}

	/* Default configuration */
	ret = i2c_xec_configure(dev,
				I2C_MODE_CONTROLLER |
				I2C_SPEED_SET(I2C_SPEED_STANDARD));
	if (ret) {
		LOG_ERR("%s configure failed %d", dev->name, ret);
		return ret;
	}

#ifdef CONFIG_I2C_TARGET
	const struct i2c_xec_config *config =
	(const struct i2c_xec_config *const) (dev->config);

	config->irq_config_func();
#endif
	return 0;
}

#define I2C_XEC_DEVICE(n)						\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static void i2c_xec_irq_config_func_##n(void);			\
									\
	static struct i2c_xec_data i2c_xec_data_##n;			\
	static const struct i2c_xec_config i2c_xec_config_##n = {	\
		.base_addr =						\
			DT_INST_REG_ADDR(n),				\
		.port_sel = DT_INST_PROP(n, port_sel),			\
		.girq_id = DT_INST_PROP(n, girq),			\
		.girq_bit = DT_INST_PROP(n, girq_bit),			\
		.pcr_idx = DT_INST_PROP_BY_IDX(n, pcrs, 0),		\
		.pcr_bitpos = DT_INST_PROP_BY_IDX(n, pcrs, 1),		\
		.sda_gpio = GPIO_DT_SPEC_INST_GET(n, sda_gpios),	\
		.scl_gpio = GPIO_DT_SPEC_INST_GET(n, scl_gpios),	\
		.irq_config_func = i2c_xec_irq_config_func_##n,		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_xec_init, NULL,	\
		&i2c_xec_data_##n, &i2c_xec_config_##n,			\
		POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,			\
		&i2c_xec_driver_api);					\
									\
	static void i2c_xec_irq_config_func_##n(void)			\
	{                                                               \
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    i2c_xec_bus_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_XEC_DEVICE)
