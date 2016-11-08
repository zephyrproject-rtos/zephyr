/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file I2C/TWI Controller driver for Atmel SAM3 family processor
 *
 * Notes on this driver:
 * 1. The controller does not have a documented way to
 *    issue RESTART when changing transfer direction as master.
 *
 *    Datasheet said about using the internal address register
 *    (IADR) to write 3 bytes before reading. This limits
 *    the number of bytes to write before a read. Also,
 *    this was documented under 7-bit addressing, and nothing
 *    about this with 10-bit addressing.
 *
 *    Experiments show that STOP has to be issued or the controller
 *    hangs forever. This was tested with reading and writing
 *    the Fujitsu I2C-based FRAM MB85RC256V.
 */

#include <errno.h>

#include <nanokernel.h>

#include <board.h>
#include <i2c.h>
#include <sys_clock.h>

#include <misc/util.h>

#include "i2c_atmel_sam3.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#include <misc/sys_log.h>

/* for use with dev_data->state */
#define STATE_READY		0
#define STATE_BUSY		(1 << 0)
#define STATE_TX		(1 << 1)
#define STATE_RX		(1 << 2)

/* return values for internal functions */
#define RET_OK			0
#define RET_ERR			1
#define RET_NACK		2


typedef void (*config_func_t)(struct device *port);


struct i2c_sam3_dev_config {
	volatile struct __twi	*port;

	config_func_t		config_func;
};

struct i2c_sam3_dev_data {
	device_sync_call_t	sync;
	union dev_config	dev_config;

	volatile uint32_t	state;

	uint8_t			*xfr_buf;
	uint32_t		xfr_len;
	uint32_t		xfr_flags;
};


/**
 * Calculate clock dividers for TWI controllers.
 *
 * @param dev Device struct
 * @return Value used for TWI_CWGR register.
 */
static uint32_t clk_div_calc(struct device *dev)
{
#if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 84000000)

	/* Use pre-calculated clock dividers when the SoC is running at
	 * 84 MHz. This saves execution time and ROM space.
	 */
	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;

	switch ((dev_data->dev_config.bits.speed)) {
	case I2C_SPEED_STANDARD:
		/* CKDIV = 1
		 * CHDIV = CLDIV = 208 = 0xD0
		 */
		return 0x0001D0D0;
	case I2C_SPEED_FAST:
		/* CKDIV = 0
		 * CHDIV = 101 = 0x65
		 * CLDIV = 106 = 0x6A
		 */
		return 0x0000656A;
	default:
		/* Return 0 as error */
		return 0;
	}

#else /* !(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 84000000) */

	/* Need to calcualte the clock dividers if the SoC is running at
	 * other frequencies.
	 */

	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;
	uint32_t i2c_clk;
	uint32_t cldiv, chdiv, ckdiv;
	uint32_t i2c_h_min_time, i2c_l_min_time;
	uint32_t cldiv_min, chdiv_min;
	uint32_t mck;

	/* The T(low) and T(high) are used to calculate CLDIV and CHDIV.
	 * Since we treat both clock low and clock high to have same period,
	 * the I2C clock frequency used for calculation has to be doubled.
	 *
	 * The I2C spec has the following minimum timing requirement:
	 * Standard Speed: High 4000ns, Low 4700ns
	 * Fast Speed: High 600ns, Low 1300ns
	 *
	 * So use these to calculate chdiv_min and cldiv_min.
	 */
	switch ((dev_data->dev_config.bits.speed)) {
	case I2C_SPEED_STANDARD:
		i2c_clk = 100000 * 2;
		i2c_h_min_time = 4000;
		i2c_l_min_time = 4700;
		break;
	case I2C_SPEED_FAST:
		i2c_clk = 400000 * 2;
		i2c_h_min_time = 600;
		i2c_l_min_time = 1300;
		break;
	default:
		/* Return 0 as error */
		return 0;
	}

	/* Calculate CLDIV (which will be used for CHDIV also) */
	cldiv = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / i2c_clk - 4;

	/* Calculate minimum CHDIV and CLDIV */

	/* Make 1/mck be in micro second */
	mck = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
	      / MSEC_PER_SEC / USEC_PER_MSEC;

	/* The +1 is to make sure we don't go under the minimum
	 * after the division. In other words, force rounding up.
	 */
	cldiv_min = (i2c_l_min_time * mck / 1000) - 4 + 1;
	chdiv_min = (i2c_h_min_time * mck / 1000) - 4 + 1;

	ckdiv = 0;
	while (cldiv > 255) {
		ckdiv++;

		/* Math is there to round up.
		 * Rounding up makes the SCL periods longer,
		 * which makes clock slower.
		 * This is fine as faster clock may cause
		 * issues.
		 */
		cldiv = (cldiv >> 1) + (cldiv & 0x01);

		cldiv_min = (cldiv_min >> 1) + (cldiv_min & 0x01);
		chdiv_min = (chdiv_min >> 1) + (chdiv_min & 0x01);
	}

	chdiv = cldiv;

	/* Make sure we are above minimum requirements */
	cldiv = max(cldiv, cldiv_min);
	chdiv = max(chdiv, chdiv_min);

	return ((ckdiv << TWI_CWGR_CKDIV_POS) + (chdiv << TWI_CWGR_CHDIV_POS)
		+ (cldiv << TWI_CWGR_CLDIV_POS));

#endif /* CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 84000000 */
}

static int i2c_sam3_runtime_configure(struct device *dev, uint32_t config)
{
	const struct i2c_sam3_dev_config * const cfg = dev->config->config_info;
	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;
	uint32_t reg;
	uint32_t clk;

	dev_data->dev_config.raw = config;
	reg = 0;

	/* Currently support master mode only */
	if (dev_data->dev_config.bits.is_slave_read) {
		return -EINVAL;
	}

	/* Calculate clock dividers */
	clk = clk_div_calc(dev);
	if (!clk) {
		return -EINVAL;
	}

	/* Disable controller first before changing anything */
	cfg->port->cr = TWI_CR_MSDIS | TWI_CR_SVDIS;

	/* Setup clock wavefore generator */
	cfg->port->cwgr = clk;

	return 0;
}

static void i2c_sam3_isr(void *arg)
{
	struct device * const dev = (struct device *)arg;
	const struct i2c_sam3_dev_config *const cfg = dev->config->config_info;
	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;

	/* Disable all interrupts so they can be processed
	 * before ISR is called again.
	 */
	cfg->port->idr = TWI_IRQ_DISABLE;

	device_sync_call_complete(&dev_data->sync);
}

/* This should be used ONLY IF <bits> are the only bits of concern.
 * This is because reading from status register will clear certain
 * bits, and thus status might be ignored afterwards.
 */
static inline void sr_bits_set_wait(struct device *dev, uint32_t bits)
{
	const struct i2c_sam3_dev_config *const cfg = dev->config->config_info;

	while (!(cfg->port->sr & bits)) {
		/* loop till <bits> are set */
	};
}

/* Clear the status registers from previous transfers */
static inline void status_reg_clear(struct device *dev)
{
	const struct i2c_sam3_dev_config *const cfg = dev->config->config_info;
	uint32_t stat_reg;

	do {
		stat_reg = cfg->port->sr;

		/* ignore these */
		stat_reg &= ~(TWI_IRQ_PDC | TWI_IRQ_TXRDY | TWI_IRQ_TXCOMP
				| TWI_IRQ_SVREAD);

		if (stat_reg & TWI_IRQ_OVRE) {
			continue;
		}

		if (stat_reg & TWI_IRQ_NACK) {
			continue;
		}

		if (stat_reg & TWI_IRQ_RXRDY) {
			stat_reg = cfg->port->rhr;
		}
	} while (stat_reg);
}

static inline void transfer_setup(struct device *dev, uint16_t slave_address)
{
	const struct i2c_sam3_dev_config *const cfg = dev->config->config_info;
	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;
	uint32_t mmr;
	uint32_t iadr;

	/* Set slave address */
	if (dev_data->dev_config.bits.use_10_bit_addr) {
		/* 10-bit slave addressing:
		 * first two bits goes to MMR/DADR, other 8 to IADR.
		 *
		 * 0x78 is the 0b11110xx bit prefix.
		 */
		mmr = 0x78 | ((slave_address >> 8) & 0x03);
		mmr <<= TWI_MMR_DADR_POS;
		mmr |= TWI_MMR_IADRSZ_1_BYTE;

		iadr = slave_address & 0xFF;
	} else {
		/* 7-bit slave addressing */
		mmr = (slave_address << TWI_MMR_DADR_POS) & TWI_MMR_DADR_MASK;

		iadr = 0;
	}

	cfg->port->mmr = mmr;
	cfg->port->iadr = iadr;
}

static inline int msg_write(struct device *dev)
{
	const struct i2c_sam3_dev_config *const cfg = dev->config->config_info;
	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;

	/* To write to slave */
	cfg->port->mmr &= ~TWI_MMR_MREAD;

	/* Setup PDC to do DMA transfer */
	cfg->port->pdc.ptcr = PDC_PTCR_TXTDIS | PDC_PTCR_RXTDIS;
	cfg->port->pdc.tpr = (uint32_t)dev_data->xfr_buf;
	cfg->port->pdc.tcr = dev_data->xfr_len;

	/* Enable TX related interrupts.
	 * TXRDY is used by PDC so we don't want to interfere.
	 */
	cfg->port->ier = TWI_IRQ_ENDTX | TWI_IRQ_NACK;

	/* Start DMA transfer for TX */
	cfg->port->pdc.ptcr = PDC_PTCR_TXTEN;

	/* Wait till transfer is done or error occurs */
	device_sync_call_wait(&dev_data->sync);

	/* Check for error */
	if (cfg->port->sr & TWI_IRQ_NACK) {
		return RET_NACK;
	}

	/* STOP if needed */
	if (dev_data->xfr_flags & I2C_MSG_STOP) {
		cfg->port->cr = TWI_CR_STOP;

		/* Wait for TXCOMP if sending STOP.
		 * The transfer is done and the controller just needs to
		 * 'send' the STOP bit. So wait should be very short.
		 */
		sr_bits_set_wait(dev, TWI_IRQ_TXCOMP);
	} else {
		/* If no STOP, just wait for TX buffer to clear.
		 * At this point, this should take no time.
		 */
		sr_bits_set_wait(dev, TWI_IRQ_TXRDY);
	}

	/* Disable PDC */
	cfg->port->pdc.ptcr = PDC_PTCR_TXTDIS;

	return RET_OK;
}

static inline int msg_read(struct device *dev)
{
	const struct i2c_sam3_dev_config *const cfg = dev->config->config_info;
	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;
	uint32_t stat_reg;
	uint32_t ctrl_reg;
	uint32_t last_len;

	/* To read from slave */
	cfg->port->mmr |= TWI_MMR_MREAD;

	/* START bit in control register needs to be set to start
	 * reading from slave. If the previous message is also read,
	 * there is no need to set the START bit again.
	 */
	ctrl_reg = 0;
	if (dev_data->xfr_flags & I2C_MSG_RESTART) {
		ctrl_reg = TWI_CR_START;
	}
	/* If there is only one byte to read, need to send STOP also. */
	if ((dev_data->xfr_len == 1)
	    && (dev_data->xfr_flags & I2C_MSG_STOP)) {
		ctrl_reg |= TWI_CR_STOP;
		dev_data->xfr_flags &= ~I2C_MSG_STOP;
	}
	cfg->port->cr = ctrl_reg;

	/* Note that this is entirely possible to do the last byte without
	 * going through DMA. But that requires another block of code to
	 * setup the transfer and test for RXRDY bit (and other). So do it
	 * this way to save a few bytes of code space.
	 */
	while (dev_data->xfr_len > 0) {
		/* Setup PDC to do DMA transfer. */
		cfg->port->pdc.ptcr = PDC_PTCR_TXTDIS | PDC_PTCR_RXTDIS;
		cfg->port->pdc.rpr = (uint32_t)dev_data->xfr_buf;

		/* Note that we need to set the STOP bit before reading
		 * last byte from RHR. So we need to process the last byte
		 * differently.
		 */
		if (dev_data->xfr_len > 1) {
			last_len = dev_data->xfr_len - 1;
		} else {
			last_len = 1;

			/* Set STOP bit for last byte.
			 * The extra check here is to prevent setting
			 * TWI_CR_STOP twice, when the message length
			 * is 1, as it is already set above.
			 */
			if (dev_data->xfr_flags & I2C_MSG_STOP) {
				cfg->port->cr = TWI_CR_STOP;
			}
		}
		cfg->port->pdc.rcr = last_len;

		/* Start DMA transfer for RX */
		cfg->port->pdc.ptcr = PDC_PTCR_RXTEN;

		/* Enable RX related interrupts
		 * RXRDY is used by PDC so we don't want to interfere.
		 */
		cfg->port->ier = TWI_IRQ_ENDRX | TWI_IRQ_NACK | TWI_IRQ_OVRE;

		/* Wait till transfer is done or error occurs */
		device_sync_call_wait(&dev_data->sync);

		/* Check for errors */
		stat_reg = cfg->port->sr;
		if (stat_reg & TWI_IRQ_NACK) {
			return RET_NACK;
		}

		if (stat_reg & TWI_IRQ_OVRE) {
			return RET_ERR;
		}

		/* no more bytes to send */
		if (dev_data->xfr_len == 0) {
			break;
		}

		dev_data->xfr_buf += last_len;
		dev_data->xfr_len -= last_len;
	}

	/* Disable PDC */
	cfg->port->pdc.ptcr = PDC_PTCR_RXTDIS;

	/* TXCOMP is kind of misleading here. This bit is set when THR/RHR
	 * and all shift registers are empty, and STOP (or NACK) is detected.
	 * So we wait here.
	 */
	sr_bits_set_wait(dev, TWI_IRQ_TXCOMP);

	return RET_OK;
}

static int i2c_sam3_transfer(struct device *dev,
			     struct i2c_msg *msgs, uint8_t num_msgs,
			     uint16_t slave_address)
{
	const struct i2c_sam3_dev_config *const cfg = dev->config->config_info;
	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;
	struct i2c_msg *cur_msg = msgs;
	uint8_t msg_left = num_msgs;
	uint32_t pflags = 0;
	int ret = 0;
	int xfr_ret;

	/* Why bother processing no messages */
	if (!msgs || !num_msgs) {
		return -ENOTSUP;
	}

	/* Device is busy servicing another transfer */
	if (dev_data->state & STATE_BUSY) {
		return -EIO;
	}

	dev_data->state = STATE_BUSY;

	/* Need to clear status from previous transfers */
	status_reg_clear(dev);

	/* Enable master */
	cfg->port->cr = TWI_CR_MSEN | TWI_CR_SVDIS;

	transfer_setup(dev, slave_address);

	/* Process all messages one-by-one */
	while (msg_left > 0) {
		dev_data->xfr_buf = cur_msg->buf;
		dev_data->xfr_len = cur_msg->len;
		dev_data->xfr_flags = cur_msg->flags;

		/* Send STOP if this is the last message */
		if (msg_left == 1) {
			dev_data->xfr_flags |= I2C_MSG_STOP;
		}

		/* The controller does not have a documented way to
		 * issue RESTART when changing transfer direction as master.
		 *
		 * Datasheet said about using the internal address register
		 * (IADR) to write 3 bytes before reading. This limits
		 * the number of bytes to write before a read. Also,
		 * this was documented under 7-bit addressing, and nothing
		 * about this with 10-bit addressing.
		 *
		 * Experiments show that STOP has to be issued or
		 * the controller hangs forever.
		 */
		if (msg_left > 1) {
			if ((dev_data->xfr_flags & I2C_MSG_RW_MASK) !=
			    (cur_msg[1].flags & I2C_MSG_RW_MASK)) {
				dev_data->xfr_flags |= I2C_MSG_STOP;
			}
		}

		/* The RESTART flag is used to indicate whether to set
		 * the START bit in control register. This is used only
		 * when changing from write to read, as the START needs
		 * to be set to start receiving. This is also to avoid
		 * setting the START bit multiple time if we are doing
		 * multiple read messages in a roll.
		 */
		if ((dev_data->xfr_flags & I2C_MSG_RW_MASK) !=
		    (pflags & I2C_MSG_RW_MASK)) {
			dev_data->xfr_flags |= I2C_MSG_RESTART;
		}

		dev_data->state &= ~(STATE_TX | STATE_RX);

		if ((dev_data->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			dev_data->state |= STATE_TX;
			xfr_ret = msg_write(dev);
		} else {
			dev_data->state |= STATE_RX;
			xfr_ret = msg_read(dev);
		}

		if (xfr_ret == RET_NACK) {
			/* Disable PDC if NACK is received. */
			cfg->port->pdc.ptcr = PDC_PTCR_TXTDIS
					      | PDC_PTCR_RXTDIS;

			ret = -EIO;
			goto done;
		}

		if (xfr_ret == RET_ERR) {
			/* Error encountered:
			 * Reset the controller and configure it again.
			 */
			cfg->port->pdc.ptcr = PDC_PTCR_TXTDIS
					      | PDC_PTCR_RXTDIS;
			cfg->port->cr = TWI_CR_SWRST | TWI_CR_MSDIS
					| TWI_CR_SVDIS;

			i2c_sam3_runtime_configure(dev,
						   dev_data->dev_config.raw);

			ret = -EIO;
			goto done;
		}

		cur_msg++;
		msg_left--;
		pflags = cur_msg->flags;
	}

done:
	dev_data->state = STATE_READY;

	/* Disable master and slave after transfer is done */
	cfg->port->cr = TWI_CR_MSDIS | TWI_CR_SVDIS;

	return ret;
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_sam3_runtime_configure,
	.transfer = i2c_sam3_transfer,
};

static int i2c_sam3_init(struct device *dev)
{
	const struct i2c_sam3_dev_config * const cfg = dev->config->config_info;
	struct i2c_sam3_dev_data * const dev_data = dev->driver_data;

	device_sync_call_init(&dev_data->sync);

	/* Disable all interrupts */
	cfg->port->idr = TWI_IRQ_DISABLE;

	cfg->config_func(dev);

	if (i2c_sam3_runtime_configure(dev, dev_data->dev_config.raw)
	    != 0) {
		SYS_LOG_DBG("I2C: Cannot set default configuration 0x%x",
		    dev_data->dev_config.raw);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_I2C_0

static void config_func_0(struct device *port);

static const struct i2c_sam3_dev_config dev_config_0 = {
	.port = __TWI0,
	.config_func = config_func_0,
};

static struct i2c_sam3_dev_data dev_data_0 = {
	.dev_config.raw = CONFIG_I2C_0_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c_sam3_0, CONFIG_I2C_0_NAME, &i2c_sam3_init,
		    &dev_data_0, &dev_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

static void config_func_0(struct device *dev)
{
	/* Enable clock for TWI0 controller */
	__PMC->pcer0 = (1 << PID_TWI0);

	IRQ_CONNECT(IRQ_TWI0, CONFIG_I2C_0_IRQ_PRI,
		    i2c_sam3_isr, DEVICE_GET(i2c_sam3_0), 0);
	irq_enable(IRQ_TWI0);
}

#endif /* CONFIG_I2C_0 */

#ifdef CONFIG_I2C_1

static void config_func_1(struct device *port);

static const struct i2c_sam3_dev_config dev_config_1 = {
	.port = __TWI1,
	.config_func = config_func_1,
};

static struct i2c_sam3_dev_data dev_data_1 = {
	.dev_config.raw = CONFIG_I2C_1_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c_sam3_1, CONFIG_I2C_1_NAME, &i2c_sam3_init,
		    &dev_data_1, &dev_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

static void config_func_1(struct device *dev)
{
	/* Enable clock for TWI0 controller */
	__PMC->pcer0 = (1 << PID_TWI1);

	IRQ_CONNECT(IRQ_TWI1, CONFIG_I2C_1_IRQ_PRI,
		    i2c_sam3_isr, DEVICE_GET(i2c_sam3_1), 0);
	irq_enable(IRQ_TWI1);
}

#endif /* CONFIG_I2C_1 */
