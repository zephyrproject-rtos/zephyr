/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software driven 'bit-banging' library for I2C
 *
 * This code implements the I2C single controller protocol in software by directly
 * manipulating the levels of the SCL and SDA lines of an I2C bus. It supports
 * the Standard-mode and Fast-mode speeds and doesn't support optional
 * protocol feature like 10-bit addresses or clock stretching.
 *
 * Timings and protocol are based Rev. 7 of the I2C specification:
 * https://www.nxp.com/docs/en/user-guide/UM10204.pdf
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include "i2c_bitbang.h"

/*
 * Indexes into delay table for each part of I2C timing waveform we are
 * interested in. In practice, for Standard and Fast modes, there are only two
 * different numerical values (T_LOW and T_HIGH) so we alias the others to
 * these. (Actually, we're simplifying a little, T_SU_STA could be T_HIGH on
 * Fast mode)
 */
#define T_LOW		0
#define T_HIGH		1
#define T_SU_STA	T_LOW
#define T_HD_STA	T_HIGH
#define T_SU_STP	T_HIGH
#define T_BUF		T_LOW

#define NS_TO_SYS_CLOCK_HW_CYCLES(ns) \
	((uint64_t)sys_clock_hw_cycles_per_sec() * (ns) / NSEC_PER_SEC + 1)

int i2c_bitbang_configure(struct i2c_bitbang *context, uint32_t dev_config)
{
	/* Check for features we don't support */
	if (I2C_ADDR_10_BITS & dev_config) {
		return -ENOTSUP;
	}

	/* Setup speed to use */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		context->delays[T_LOW]  = NS_TO_SYS_CLOCK_HW_CYCLES(4700);
		context->delays[T_HIGH] = NS_TO_SYS_CLOCK_HW_CYCLES(4000);
		break;
	case I2C_SPEED_FAST:
		context->delays[T_LOW]  = NS_TO_SYS_CLOCK_HW_CYCLES(1300);
		context->delays[T_HIGH] = NS_TO_SYS_CLOCK_HW_CYCLES(600);
		break;
	default:
		return -ENOTSUP;
	}

	context->dev_config = dev_config;

	return 0;
}

int i2c_bitbang_get_config(struct i2c_bitbang *context, uint32_t *config)
{
	if (context->dev_config == 0) {
		return -EIO;
	}

	*config = context->dev_config;

	return 0;
}

static int i2c_set_scl(struct i2c_bitbang *context, int state)
{
	context->io->set_scl(context->io_context, state);
#ifdef CONFIG_I2C_GPIO_CLOCK_STRETCHING
	if (state == 1) {
		/* Wait for target to release the clock */
		if (!WAIT_FOR(context->io->get_scl(context->io_context) != 0,
			      CONFIG_I2C_GPIO_CLOCK_STRETCHING_TIMEOUT_US,
			      ;)) {
			return -ETIMEDOUT;
		}
	}
#endif
	return 0;
}

static void i2c_release_lines(struct i2c_bitbang *context)
{
	/* A held SCL cannot produce STOP. Release both lines without waiting again. */
	context->io->set_scl(context->io_context, 1);
	context->io->set_sda(context->io_context, 1);
}

static void i2c_set_sda(struct i2c_bitbang *context, int state)
{
	context->io->set_sda(context->io_context, state);
}

static int i2c_get_sda(struct i2c_bitbang *context)
{
	return context->io->get_sda(context->io_context);
}

static void i2c_delay(unsigned int cycles_to_wait)
{
	uint32_t start = k_cycle_get_32();

	/* Wait until the given number of cycles have passed */
	while (k_cycle_get_32() - start < cycles_to_wait) {
	}
}

static int i2c_start(struct i2c_bitbang *context)
{
	int ret;
	if (!i2c_get_sda(context)) {
		/*
		 * SDA is already low, so we need to do something to make it
		 * high. Try pulsing clock low to get target to release SDA.
		 */
		i2c_set_scl(context, 0);
		i2c_delay(context->delays[T_LOW]);
		ret = i2c_set_scl(context, 1);
		if (ret < 0) {
			return ret;
		}
		i2c_delay(context->delays[T_SU_STA]);
	}
	i2c_set_sda(context, 0);
	i2c_delay(context->delays[T_HD_STA]);

	i2c_set_scl(context, 0);
	i2c_delay(context->delays[T_LOW]);
	return 0;
}

static int i2c_repeated_start(struct i2c_bitbang *context)
{
	int ret;
	i2c_set_sda(context, 1);
	ret = i2c_set_scl(context, 1);
	if (ret < 0) {
		return ret;
	}
	i2c_delay(context->delays[T_HIGH]);

	i2c_delay(context->delays[T_SU_STA]);
	return i2c_start(context);
}

static int i2c_stop(struct i2c_bitbang *context)
{
	int ret;
	i2c_set_sda(context, 0);
	i2c_delay(context->delays[T_LOW]);

	ret = i2c_set_scl(context, 1);
	if (ret < 0) {
		return ret;
	}
	i2c_delay(context->delays[T_HIGH]);

	i2c_delay(context->delays[T_SU_STP]);
	i2c_set_sda(context, 1);
	i2c_delay(context->delays[T_BUF]); /* In case we start again too soon */
	return 0;
}

static int i2c_write_bit(struct i2c_bitbang *context, int bit)
{
	int ret;
	/* SDA hold time is zero, so no need for a delay here */
	i2c_set_sda(context, bit);
	ret = i2c_set_scl(context, 1);
	if (ret < 0) {
		return ret;
	}
	i2c_delay(context->delays[T_HIGH]);
	i2c_set_scl(context, 0);
	i2c_delay(context->delays[T_LOW]);
	return 0;
}

static int i2c_read_bit(struct i2c_bitbang *context, bool *bit)
{
	int ret;

	/* SDA hold time is zero, so no need for a delay here */
	i2c_set_sda(context, 1); /* Stop driving low, so target has control */

	ret = i2c_set_scl(context, 1);
	if (ret < 0) {
		return ret;
	}
	i2c_delay(context->delays[T_HIGH]);

	*bit = i2c_get_sda(context);

	i2c_set_scl(context, 0);
	i2c_delay(context->delays[T_LOW]);
	return 0;
}

static int i2c_write_byte(struct i2c_bitbang *context, uint8_t byte)
{
	uint8_t mask = 1 << 7;
	bool nack;
	int ret;

	do {
		ret = i2c_write_bit(context, byte & mask);
		if (ret < 0) {
			return ret;
		}
	} while (mask >>= 1);

	ret = i2c_read_bit(context, &nack);
	return ret < 0 ? ret : (nack ? -EIO : 0);
}

static int i2c_read_byte(struct i2c_bitbang *context, uint8_t *value)
{
	unsigned int byte = 1U;
	bool bit;
	int ret;

	do {
		byte <<= 1;
		ret = i2c_read_bit(context, &bit);
		if (ret < 0) {
			return ret;
		}
		byte |= bit;
	} while (!(byte & (1 << 8)));

	*value = byte;
	return 0;
}

int i2c_bitbang_transfer(struct i2c_bitbang *context,
			   struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t target_address)
{
	uint8_t *buf, *buf_end;
	unsigned int flags;
	int result = -EIO;
	int stop_result;

	/* We want an initial Start condition */
	flags = I2C_MSG_RESTART;

	/* Make sure we're in a good state so target recognises the Start */
	result = i2c_set_scl(context, 1);
	if (result < 0) {
		goto abort;
	}
	flags |= I2C_MSG_STOP;

	do {
		/* Stop flag from previous message? */
		if (flags & I2C_MSG_STOP) {
			result = i2c_stop(context);
			if (result < 0) {
				goto abort;
			}
		}

		/* Forget old flags except start flag */
		flags &= I2C_MSG_RESTART;

		/* Start condition? */
		if (flags & I2C_MSG_RESTART) {
			result = i2c_start(context);
			if (result < 0) {
				goto abort;
			}
		} else if (msgs->flags & I2C_MSG_RESTART) {
			result = i2c_repeated_start(context);
			if (result < 0) {
				goto abort;
			}
		}

		/* Get flags for new message */
		flags |= msgs->flags;

		/* Send address after any Start condition */
		if (flags & I2C_MSG_RESTART) {
			unsigned int byte0 = target_address << 1;

			byte0 |= (flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
			result = i2c_write_byte(context, byte0);
			if (result < 0) {
				goto finish;
			}
			flags &= ~I2C_MSG_RESTART;
		}

		/* Transfer data */
		buf = msgs->buf;
		buf_end = buf + msgs->len;
		if ((flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			/* Read */
			while (buf < buf_end) {
				result = i2c_read_byte(context, buf);
				if (result < 0) {
					goto abort;
				}
				buf++;
				/* ACK the byte, except for the last one */
				result = i2c_write_bit(context, buf == buf_end);
				if (result < 0) {
					goto abort;
				}
			}
		} else {
			/* Write */
			while (buf < buf_end) {
				result = i2c_write_byte(context, *buf++);
				if (result < 0) {
					goto finish;
				}
			}
		}

		/* Next message */
		msgs++;
		num_msgs--;
	} while (num_msgs);

	/* Complete without error */
	result = 0;
finish:
	if (result == -ETIMEDOUT) {
		goto abort;
	}
	stop_result = i2c_stop(context);
	if (stop_result < 0) {
		if (result == 0) {
			result = stop_result;
		}
		goto abort;
	}

	return result;
abort:
	i2c_release_lines(context);
	return result;
}

int i2c_bitbang_recover_bus(struct i2c_bitbang *context)
{
	int i;
	int ret;

	/*
	 * The I2C-bus specification and user manual (NXP UM10204
	 * rev. 6, section 3.1.16) suggests the controller emit 9 SCL
	 * clock pulses to recover the bus.
	 *
	 * The Linux kernel I2C bitbang recovery functionality issues
	 * a START condition followed by 9 STOP conditions.
	 *
	 * Other I2C target devices (e.g. Microchip ATSHA204a) suggest
	 * issuing a START condition followed by 9 SCL clock pulses
	 * with SDA held high/floating, a REPEATED START condition,
	 * and a STOP condition.
	 *
	 * The latter is what is implemented here.
	 */

	/* Start condition */
	ret = i2c_start(context);
	if (ret < 0) {
		goto abort;
	}

	/* 9 cycles of SCL with SDA held high */
	for (i = 0; i < 9; i++) {
		ret = i2c_write_bit(context, 1);
		if (ret < 0) {
			goto abort;
		}
	}

	/* Another start condition followed by a stop condition */
	ret = i2c_repeated_start(context);
	if (ret < 0) {
		goto abort;
	}
	ret = i2c_stop(context);
	if (ret < 0) {
		goto abort;
	}

	/* Check if bus is clear */
	if (i2c_get_sda(context)) {
		return 0;
	} else {
		return -EBUSY;
	}
abort:
	i2c_release_lines(context);
	return ret;
}

void i2c_bitbang_init(struct i2c_bitbang *context,
			const struct i2c_bitbang_io *io, void *io_context)
{
	context->io = io;
	context->io_context = io_context;
	i2c_bitbang_configure(context, I2C_SPEED_STANDARD << I2C_SPEED_SHIFT);
}
