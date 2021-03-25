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
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_mchp, CONFIG_I2C_LOG_LEVEL);

#define SPEED_100KHZ_BUS    0
#define SPEED_400KHZ_BUS    1
#define SPEED_1MHZ_BUS      2

#define EC_OWN_I2C_ADDR		0x7F
#define RESET_WAIT_US		20

/* I2C timeout is  10 ms (WAIT_INTERVAL * WAIT_COUNT) */
#define WAIT_INTERVAL		50
#define WAIT_COUNT		200

/* I2C Read/Write bit pos */
#define I2C_READ_WRITE_POS  0

/* I2C recover SCL low retries */
#define I2C_RECOVER_SCL_LOW_RETRIES 3
/* I2C recover SDA low retries */
#define I2C_RECOVER_SDA_LOW_RETRIES 10

/* I2C get lines values */
#define I2C_SCL_HI BIT(0)
#define I2C_SDA_HI BIT(1)
#define I2C_SCL_SDA_HI (I2C_SCL_HI | I2C_SDA_HI)

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
	uint8_t sda_pos;
	uint8_t scl_pos;
	const char *sda_gpio_label;
	const char *scl_gpio_label;
	void (*irq_config_func)(void);
};

struct i2c_xec_data {
	uint8_t started;
	uint8_t pending_stop;
	uint8_t speed_id;
	const struct device *sda_gpio;
	const struct device *scl_gpio;
	struct i2c_slave_config *target_cfg;
	bool target_attached;
	bool target_read;
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

static int i2c_xec_reset_config(const struct device *dev);


#ifdef DEBUG_I2C_XEC_ITM
#else

static void ITM_send_str(uint8_t chan, const char *s)
{
	(void)chan;
	(void)s;
}
#endif

/*
 * AHB = 48 MHz. I2C BAUD clock is 16 MHz.
 * One AHB access takes minimum 3 AHB clocks.
 */
static void xec_i2c_baud_clk_delay(uint32_t ba, uint32_t nbc)
{
	while (nbc--) {
		REG8(ba + MCHP_I2C_SMB_BLOCK_REV_OFS) =
			REG8(ba + MCHP_I2C_SMB_BLOCK_ID_OFS);
	}
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

static int recover_from_error(const struct device *dev)
{
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	data->pending_stop = 0;
	data->target_read = 0;

	return i2c_xec_reset_config(dev);
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

static void clear_data_flags(const struct device *dev)
{
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	data->started = 0;
	data->pending_stop = 0;
}

static int i2c_xec_reset_config(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	uint32_t ba = config->base_addr;

	/* Assert RESET and clr others */
	MCHP_I2C_SMB_CFG(ba) = MCHP_I2C_SMB_CFG_RESET;
	xec_i2c_baud_clk_delay(ba, 5);
	/* clear reset, set filter enable, select port */
	MCHP_I2C_SMB_CFG(ba) = 0;

	/*
	 * Controller implements two peripheral addresses for itself.
	 * It always monitors whether an external controller issues START
	 * plus target address. We should write valid peripheral addresses
	 * that do not match any peripheral on the bus.
	 * An alternative is to use the default 0 value which is the
	 * general call address and disable the general call match
	 * enable in the configuration register.
	 */
	MCHP_I2C_SMB_OWN_ADDR(ba) = 0;
#ifdef CONFIG_I2C_SLAVE
	if (data->target_cfg) {
		MCHP_I2C_SMB_OWN_ADDR(ba) = data->target_cfg->address;
	}
#endif
	/* Port number and filter enable MUST be written before enabling */
	MCHP_I2C_SMB_CFG(ba) |= BIT(14);
	MCHP_I2C_SMB_CFG(ba) |= MCHP_I2C_SMB_CFG_FEN;
	MCHP_I2C_SMB_CFG(ba) |= (config->port_sel &
				MCHP_I2C_SMB_CFG_PORT_SEL_MASK);

	/* PIN=1 to clear all status except NBB and synchronize */
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN;

	/*
	 * Before enabling the controller program the desired bus clock,
	 * repeated start hold time, data timing, and timeout scaling
	 * registers.
	 */
	MCHP_I2C_SMB_BUS_CLK(ba) = xec_cfg_params[data->speed_id].bus_clk;
	MCHP_I2C_SMB_RSHT(ba) =
		xec_cfg_params[data->speed_id].start_hold_time;
	MCHP_I2C_SMB_DATA_TM(ba) = xec_cfg_params[data->speed_id].data_timing;
	MCHP_I2C_SMB_TMTSC(ba) = xec_cfg_params[data->speed_id].timeout_scale;

	/* Enable controller */
	MCHP_I2C_SMB_CFG(ba) |= MCHP_I2C_SMB_CFG_ENAB;

	/*
	 * PIN=1 clears all status except NBB
	 * ESO=1 enables output drivers
	 * ACK=1 enable ACK generation when data/address is clocked in.
	 */
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				   MCHP_I2C_SMB_CTRL_ESO |
				   MCHP_I2C_SMB_CTRL_ACK;

	data->started = 0;
	data->pending_stop = 0;

	/* wait for NBB=1, BER, or timeout */
	return wait_bus_free(dev);
}

#ifdef CONFIG_I2C_SLAVE
/*
 * Restart I2C controller as target for ACK of address match.
 * Setting PIN clears all status in I2C.Status register except NBB.
 */
static void restart_target(uint32_t ba)
{
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				   MCHP_I2C_SMB_CTRL_ESO |
				   MCHP_I2C_SMB_CTRL_ACK |
				   MCHP_I2C_SMB_CTRL_ENI;
}

/*
 * Configure I2C controller acting as target to NACK the next received byte.
 * NOTE: Firmware must re-enable ACK generation before the start of the next
 * transaction otherwise the controller will NACK its target addresses.
 */
static void target_config_for_nack(uint32_t ba)
{
	MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
				   MCHP_I2C_SMB_CTRL_ESO |
				   MCHP_I2C_SMB_CTRL_ENI;
}
#endif

/*
 * Wait for I2C.Status PIN 1 -> 0(active) with timeout.
 * returns:
 *  0: PIN == 0 I2C finished with ACK.
 * -EBUSY Bus Error
 * -ETIMEOUT PIN always 1
 * -EIO target device NACK's address or data.
 */
static int wait_completion(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	int ret;
	int counter = 0;
	uint32_t ba = config->base_addr;
	uint8_t status;

	/* Wait for transaction to be completed */
	status = MCHP_I2C_SMB_STS_RO(ba);
	while (status & MCHP_I2C_SMB_STS_PIN) {
		ret = xec_spin_yield(&counter);

		if (status & MCHP_I2C_SMB_STS_BER) {
			recover_from_error(dev);
			return -EBUSY;
		}

		if (ret < 0) {
			if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_PIN) {
				recover_from_error(dev);
				return ret;
			}
		}
		status = MCHP_I2C_SMB_STS_RO(ba);
	}

	/* Check for bus error */
	if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_BER) {
		recover_from_error(dev);
		return -EBUSY;
	}

	/* Check if target responded with ACK/NACK */
	if (MCHP_I2C_SMB_STS_RO(ba) & MCHP_I2C_SMB_STS_LRB_AD0) {
		return -EIO;
	}

	return 0;
}

static int send_stop_and_wait(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	uint32_t ba = config->base_addr;

	MCHP_I2C_SMB_CTRL_WO(ba) = (MCHP_I2C_SMB_CTRL_PIN |
				    MCHP_I2C_SMB_CTRL_ESO |
				    MCHP_I2C_SMB_CTRL_STO |
				    MCHP_I2C_SMB_CTRL_ACK);

	clear_data_flags(dev);

	return wait_bus_free(dev);
}

/*
 * Use DT information to get the port's SDA and SCL pin states
 * by calling the GPIO driver.
 */
static uint32_t get_lines(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	uint32_t i2c_lines = 0;
	gpio_port_value_t sda = 0, scl = 0;

	if (gpio_port_get_raw(data->sda_gpio, &sda)) {
		return i2c_lines;
	}

	if (sda & BIT(config->sda_pos)) {
		i2c_lines |= I2C_SDA_HI;
	}

	/* both pins on same GPIO port? */
	if (data->sda_gpio == data->scl_gpio) {
		scl = sda;
	} else {
		if (gpio_port_get_raw(data->scl_gpio, &scl)) {
			return i2c_lines;
		}
	}

	if (scl & BIT(config->scl_pos)) {
		i2c_lines |= I2C_SCL_HI;
	}

	return i2c_lines;
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

	return i2c_xec_reset_config(dev);
}

/*
 * Attempt to recover the I2C bus.
 * If SCL is driven low by an external device all we can do
 * is poll a few time and hope the external device will release SCL.
 * If SDA is driven low by an external device we will generate 9
 * clocks and attempt to generate a STOP. Sometimes this can trigger
 * the stuck device to release SDA.
 *
 * We can make use of the controller's bit-bang mode because we aren't
 * performing a real I2C transaction.
 * Switch to big-bang undriven mode: our drives tri-stated.
 * Get SDA and SCL pin states.
 * If SCL is low
 *   Retry SCL every 5 us up to I2C_RECOVER_SCL_LOW_RETRIES times.
 *   If SCL remains low reset controller and return -EBUSY
 * If SDA is low
 *  Retry driving 9 clocks at ~100 KHz and generate a STOP up
 *  to I2C_RECOVER_SDA_LOW_RETRIES
 *  If SDA remains low reset controller and return -EBUSY
 * reset controller
 * return success
 */
static int i2c_xec_recover_bus(const struct device *dev)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);

	uint32_t ba = config->base_addr;
	uint32_t cfg;
	int count;

	data->started = 0;
	data->pending_stop = 0;
	data->target_read = 0;

	/* Make sure our controller is idle before attempting bus recovery */
	cfg = MCHP_I2C_SMB_CFG(ba);
	MCHP_I2C_SMB_CFG(ba) = MCHP_I2C_SMB_CFG_RESET;
	xec_i2c_baud_clk_delay(ba, 16);
	MCHP_I2C_SMB_CFG(ba) =
		(config->port_sel & MCHP_I2C_SMB_CFG_PORT_SEL_MASK);
	MCHP_I2C_SMB_CFG(ba) |= MCHP_I2C_SMB_CFG_FEN;
	MCHP_I2C_SMB_CFG(ba) |= MCHP_I2C_SMB_CFG_ENAB;

	/* Enable bit-bang mode. */
	MCHP_I2C_SMB_BB_CTRL(ba) = BIT(0);

	count = I2C_RECOVER_SCL_LOW_RETRIES;
	while (!(MCHP_I2C_SMB_BB_CTRL(ba) & BIT(5))) {
		/* SCL is low. All we can do is wait for it to go high */
		if (count-- == 0) {
			/* reset controller which disables bit-bang mode */
			i2c_xec_reset_config(dev);
			return -EBUSY;
		}
		k_busy_wait(RESET_WAIT_US);
	}

	count = I2C_RECOVER_SDA_LOW_RETRIES;
	while (!(MCHP_I2C_SMB_BB_CTRL(ba) & BIT(6))) {
		/* SDA is low. Drive clocks until SDA is released */
		if (count-- == 0) {
			i2c_xec_reset_config(dev);
			return -EBUSY;
		}

		for (int i = 0; i < 9; i++) {
			/* drive SCL low */
			MCHP_I2C_SMB_BB_CTRL(ba) = BIT(0) | BIT(1);
			k_busy_wait(5);
			MCHP_I2C_SMB_BB_CTRL(ba) = BIT(0); /* release SCL */
			k_busy_wait(5);
		}

		/* try to generate STOP: SCL=High then SDA rising edge */
		MCHP_I2C_SMB_BB_CTRL(ba) = BIT(0) | BIT(2); /* SDA low */
		k_busy_wait(5);
		MCHP_I2C_SMB_BB_CTRL(ba) = BIT(0); /* release SDA */
		k_busy_wait(5);
	}

	/* Disable bit-bang mode */
	MCHP_I2C_SMB_BB_CTRL(ba) = 0U;

	/* reset controller */
	return i2c_xec_reset_config(dev);
}

static int i2c_xec_poll_write(const struct device *dev, struct i2c_msg msg,
			      uint16_t addr)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	uint32_t ba = config->base_addr;
	int ret = 0;
	uint32_t lines;

	if (data->started == 0) {
		/* Check clock and data lines */
		lines = get_lines(dev);
		if (lines != I2C_SCL_SDA_HI) {
			return -EBUSY;
		}

		/* Wait until bus is free */
		ret = wait_bus_free(dev);
		if (ret) {
			clear_data_flags(dev);
			return ret;
		}

		/* Load target address */
		MCHP_I2C_SMB_DATA(ba) = (addr & ~BIT(0));

		/*
		 * Send start and target address. Enable ACK generation
		 * by this controller when it clocks in data from the target.
		 */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
					   MCHP_I2C_SMB_CTRL_ESO |
					   MCHP_I2C_SMB_CTRL_STA |
					   MCHP_I2C_SMB_CTRL_ACK;

		ret = wait_completion(dev);
		if (ret) {
			send_stop_and_wait(dev);
			return ret;
		}

		data->started = 1U;
	} else if (msg.flags & I2C_MSG_RESTART) {
		/*
		 * MCHP I2C spec requires repeated start sequence is:
		 * write to control register then data register.
		 */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO |
					   MCHP_I2C_SMB_CTRL_STA |
					   MCHP_I2C_SMB_CTRL_ACK;
		/* trigger start, address transmit, ACK/NACK capture */
		MCHP_I2C_SMB_DATA(ba) = (addr | BIT(0));

		ret = wait_completion(dev);
		if (ret) {
			send_stop_and_wait(dev);
			return ret;
		}

	}

	/* Send bytes */
	for (int i = 0U; i < msg.len; i++) {
		MCHP_I2C_SMB_DATA(ba) = msg.buf[i];
		ret = wait_completion(dev);
		if (ret) {
			send_stop_and_wait(dev);
			return ret;
		}

		/* Handle stop bit for last byte to write */
		if (i == (msg.len - 1)) {
			if (msg.flags & I2C_MSG_STOP) {
				/* Send stop and ack bits */
				ret = send_stop_and_wait(dev);
				break;
			} else {
				data->pending_stop = 1;
			}
		}
	}

	return ret;
}

static int i2c_xec_poll_read(const struct device *dev, struct i2c_msg msg,
			     uint16_t addr)
{
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	uint32_t ba = config->base_addr;
	uint8_t byte;
	int ret;
	uint32_t lines, nb;
	uint8_t *pb;

	if (msg.len == 0) {
		return 0;
	}

	/* if (!(msg.flags & I2C_MSG_RESTART)) { */
	if (data->started == 0) {
		/* Check clock and data lines */
		lines = get_lines(dev);
		if (lines != I2C_SCL_SDA_HI) {
			return -EBUSY;
		}

		/* Wait until bus is free */
		ret = wait_bus_free(dev);
		if (ret) {
			clear_data_flags(dev);
			return ret;
		}

		/* Load target address into data register */
		MCHP_I2C_SMB_DATA(ba) = (addr | BIT(0));
		/* trigger start, address transmit, ACK/NACK capture */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_PIN |
					   MCHP_I2C_SMB_CTRL_ESO |
					   MCHP_I2C_SMB_CTRL_STA |
					   MCHP_I2C_SMB_CTRL_ACK;

	} else if (msg.flags & I2C_MSG_RESTART) {
		/*
		 * MCHP I2C spec requires repeated start sequence is:
		 * write to control register then data register.
		 */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO |
					   MCHP_I2C_SMB_CTRL_STA |
					   MCHP_I2C_SMB_CTRL_ACK;
		/* trigger start, address transmit, ACK/NACK capture */
		MCHP_I2C_SMB_DATA(ba) = (addr | BIT(0));
	}

	ret = wait_completion(dev);
	if (ret) {
		send_stop_and_wait(dev);
		return ret;
	}

	if (msg.len == 1) {
		/* Prepare controller to NACK on last data */
		MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO;
	}

	/*
	 * The HW clocked in the target address.
	 * We read and discard it causing HW to
	 * generate clocks for first data byte.
	 * This is why we must clear ACK above
	 * if the request is to read one byte.
	 */
	byte = MCHP_I2C_SMB_DATA(ba);
	ret = wait_completion(dev);
	if (ret) {
		send_stop_and_wait(dev);
		return ret;
	}

	ret = 0;
	pb = msg.buf;
	nb = msg.len;
	while (nb) {
		byte = MCHP_I2C_SMB_STS_RO(ba);

		if (nb == 2) {
			/* prepare HW to NACK next byte clocked in */
			MCHP_I2C_SMB_CTRL_WO(ba) = MCHP_I2C_SMB_CTRL_ESO;
		} else if (nb == 1) {
			MCHP_I2C_SMB_CTRL_WO(ba) = (MCHP_I2C_SMB_CTRL_PIN
						    | MCHP_I2C_SMB_CTRL_ESO
						    | MCHP_I2C_SMB_CTRL_STO
						    | MCHP_I2C_SMB_CTRL_ACK);
			clear_data_flags(dev);
			/*
			 * read last byte previously clocked in
			 * and generate STOP instead of clocks.
			 */
			*pb = MCHP_I2C_SMB_DATA(ba);

			/* wait for NBB 0 -> 1 */
			ret = wait_bus_free(dev);
			break;
		}

		/* read data in buffer and trigger clocks for next byte */
		*pb = MCHP_I2C_SMB_DATA(ba);
		ret = wait_completion(dev);
		if (ret) {
			send_stop_and_wait(dev);
			break;
		}

		pb++;
		nb--;
	}

	return ret;
}

static int i2c_xec_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t addr)
{
	int ret = 0;

#ifdef CONFIG_I2C_SLAVE
	struct i2c_xec_data *data = dev->data;

	if (data->target_attached) {
		LOG_ERR("Device is registered as target");
		return -EBUSY;
	}
#endif

	addr <<= 1;
	for (int i = 0U; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_xec_poll_write(dev, msgs[i], addr);
			if (ret) {
				LOG_ERR("Write error: %d", ret);
				return ret;
			}
		} else {
			ret = i2c_xec_poll_read(dev, msgs[i], addr);
			if (ret) {
				LOG_ERR("Read error: %d", ret);
				return ret;
			}
		}
	}

	return 0;
}

static void i2c_xec_bus_isr(void *arg)
{
#ifdef CONFIG_I2C_SLAVE
	struct device *dev = (struct device *)arg;
	const struct i2c_xec_config *config =
		(const struct i2c_xec_config *const) (dev->config);
	struct i2c_xec_data *data = dev->data;
	const struct i2c_slave_callbacks *target_cb =
		data->target_cfg->callbacks;
	uint32_t ba = config->base_addr;

	int ret;
	uint32_t status;
	uint32_t compl_status;
	uint8_t val;

	uint8_t dummy = 0U;

	/* Get current status */
	status = MCHP_I2C_SMB_STS_RO(ba);
	compl_status = MCHP_I2C_SMB_CMPL(ba) & MCHP_I2C_SMB_CMPL_RW1C_MASK;

	/* Idle interrupt enabled and active? */
	if (MCHP_I2C_SMB_CFG(ba) & compl_status & BIT(29)) {
		MCHP_I2C_SMB_CFG_B3(ba) = 0U; /* disable Idle interrupt */
		if (status & MCHP_I2C_SMB_STS_NBB) {
			restart_target(ba);
			goto clear_iag;
		}
	}

	if (!data->target_attached) {
		goto clear_iag;
	}

	/* Bus Error */
	if (status & MCHP_I2C_SMB_STS_BER) {
		if (target_cb->stop) {
			target_cb->stop(data->target_cfg);
		}
		restart_target(ba);
		goto clear_iag;
	}

	/* External stop */
	if (status & MCHP_I2C_SMB_STS_EXT_STOP) {
		if (target_cb->stop) {
			target_cb->stop(data->target_cfg);
		}
		restart_target(ba);
		goto clear_iag;
	}

	/* Address byte handling */
	if (status & MCHP_I2C_SMB_STS_AAS) {
		if (status & MCHP_I2C_SMB_STS_PIN) {
			goto clear_iag;
		}

		uint8_t slv_data = MCHP_I2C_SMB_DATA(ba);

		if (slv_data & BIT(I2C_READ_WRITE_POS)) {
			/* target transmitter mode */
			data->target_read = true;
			val = dummy;
			if (target_cb->read_requested) {
				target_cb->read_requested(
					data->target_cfg, &val);

				/* Application target transmit handler
				 * does not have data to send. In
				 * target transmit mode the external
				 * Controller is ACK's data we send.
				 * All we can do is keep sending dummy
				 * data. We assume read_requested does
				 * not modify the value pointed to by val
				 * if it has not data(returns error).
				 */
			}
			/*
			 * Writing I2CData causes this HW to release SCL
			 * ending clock stretching. The external Controller
			 * senses SCL released and begins generating clocks
			 * and capturing data driven by this controller
			 * on SDA. External Controller ACK's data until it
			 * wants no more then it will NACK.
			 */
			MCHP_I2C_SMB_DATA(ba) = val;
			goto clear_iag; /* Exit ISR */
		} else {
			/* target receiver mode */
			data->target_read = false;
			if (target_cb->write_requested) {
				ret = target_cb->write_requested(
							data->target_cfg);
				if (ret) {
					/*
					 * Application handler can't accept
					 * data. Configure HW to NACK next
					 * data transmitted by external
					 * Controller.
					 * !!! TODO We must re-program our HW
					 * for address ACK before next
					 * transaction is begun !!!
					 */
					target_config_for_nack(ba);
				}
			}
			goto clear_iag; /* Exit ISR */
		}
	}

	if (data->target_read) { /* Target transmitter mode */

		/* Master has Nacked, then just write a dummy byte */
		status = MCHP_I2C_SMB_STS_RO(ba);
		if (status & MCHP_I2C_SMB_STS_LRB_AD0) {

			/*
			 * ISSUE: HW will not detect external STOP in
			 * target transmit mode. Enable IDLE interrupt
			 * to catch PIN 0 -> 1 and NBB 0 -> 1.
			 */
			MCHP_I2C_SMB_CFG(ba) |= MCHP_I2C_SMB_CFG_ENIDI;

			/*
			 * dummy write causes this controller's PIN status
			 * to de-assert 0 -> 1. Data is not transmitted.
			 * SCL is not driven low by this controller.
			 */
			MCHP_I2C_SMB_DATA(ba) = dummy;

			status = MCHP_I2C_SMB_STS_RO(ba);

		} else {
			val = dummy;
			if (target_cb->read_processed) {
				target_cb->read_processed(
					data->target_cfg, &val);
			}
			MCHP_I2C_SMB_DATA(ba) = val;
		}
	} else { /* target receiver mode */
		/*
		 * Reading the I2CData register causes this target to release
		 * SCL. The external Controller senses SCL released generates
		 * clocks for transmitting the next data byte.
		 * Reading I2C Data register causes PIN status 0 -> 1.
		 */
		val = MCHP_I2C_SMB_DATA(ba);
		if (target_cb->write_received) {
			/*
			 * Call back returns error if we should NACK
			 * next byte.
			 */
			ret = target_cb->write_received(data->target_cfg, val);
			if (ret) {
				/*
				 * Configure HW to NACK next byte. It will not
				 * generate clocks for another byte of data
				 */
				target_config_for_nack(ba);
			}
		}
	}

clear_iag:
	MCHP_I2C_SMB_CMPL(ba) = compl_status;
	MCHP_GIRQ_SRC(config->girq_id) = BIT(config->girq_bit);
#endif
}

#ifdef CONFIG_I2C_SLAVE
static int i2c_xec_target_register(const struct device *dev,
				   struct i2c_slave_config *config)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data = dev->data;
	uint32_t ba = cfg->base_addr;
	int ret;
	int counter = 0;

	if (!config) {
		return -EINVAL;
	}

	if (data->target_attached) {
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

	data->target_cfg = config;

	ret = i2c_xec_reset_config(dev);
	if (ret) {
		return ret;
	}

	restart_target(ba);

	data->target_attached = true;

	/* Clear before enabling girq bit */
	MCHP_GIRQ_SRC(cfg->girq_id) = BIT(cfg->girq_bit);
	MCHP_GIRQ_ENSET(cfg->girq_id) = BIT(cfg->girq_bit);

	return 0;
}

static int i2c_xec_target_unregister(const struct device *dev,
				     struct i2c_slave_config *config)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data = dev->data;

	if (!data->target_attached) {
		return -EINVAL;
	}

	data->target_cfg = NULL;
	data->target_attached = false;

	MCHP_GIRQ_ENCLR(cfg->girq_id) = BIT(cfg->girq_bit);

	return 0;
}
#endif

static const struct i2c_driver_api i2c_xec_driver_api = {
	.configure = i2c_xec_configure,
	.transfer = i2c_xec_transfer,
	.recover_bus = i2c_xec_recover_bus,
#ifdef CONFIG_I2C_SLAVE
	.slave_register = i2c_xec_target_register,
	.slave_unregister = i2c_xec_target_unregister,
#endif
};

static int i2c_xec_init(const struct device *dev)
{
	const struct i2c_xec_config *cfg = dev->config;
	struct i2c_xec_data *data =
		(struct i2c_xec_data *const) (dev->data);
	int ret;

	ITM_send_str(0, "INIT ");

	data->target_cfg = NULL;
	data->started = 0;
	data->pending_stop = 0;
	data->target_attached = false;

	data->sda_gpio = device_get_binding(cfg->sda_gpio_label);
	if (!data->sda_gpio) {
		LOG_ERR("i2c configure failed to bind SDA GPIO");
		return -ENXIO;
	}

	data->scl_gpio = device_get_binding(cfg->scl_gpio_label);
	if (!data->scl_gpio) {
		LOG_ERR("i2c configure failed to bind SCL GPIO");
		return -ENXIO;
	}

	/* Default configuration */
	ret = i2c_xec_configure(dev,
				I2C_MODE_MASTER |
				I2C_SPEED_SET(I2C_SPEED_STANDARD));
	if (ret) {
		LOG_ERR("i2c configure failed %d", ret);
		return ret;
	}

#ifdef CONFIG_I2C_SLAVE
	const struct i2c_xec_config *config =
	(const struct i2c_xec_config *const) (dev->config);

	config->irq_config_func();
#endif
	return 0;
}

#define I2C_XEC_DEVICE(n)						\
	static void i2c_xec_irq_config_func_##n(void);			\
									\
	static struct i2c_xec_data i2c_xec_data_##n;			\
	static const struct i2c_xec_config i2c_xec_config_##n = {	\
		.base_addr =						\
			DT_INST_REG_ADDR(n),				\
		.port_sel = DT_INST_PROP(n, port_sel),			\
		.girq_id = DT_INST_PROP(n, girq),			\
		.girq_bit = DT_INST_PROP(n, girq_bit),			\
		.sda_pos = DT_INST_GPIO_PIN(n, sda_gpios),		\
		.scl_pos = DT_INST_GPIO_PIN(n, scl_gpios),		\
		.sda_gpio_label = DT_INST_GPIO_LABEL(n, sda_gpios),	\
		.scl_gpio_label = DT_INST_GPIO_LABEL(n, scl_gpios),	\
		.irq_config_func = i2c_xec_irq_config_func_##n,		\
	};								\
	DEVICE_DT_INST_DEFINE(n, &i2c_xec_init, device_pm_control_nop,	\
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
