/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software driven 'bit-banging' library for I2C
 *
 * This code implements the I2C single master protocol in software by directly
 * manipulating the levels of the SCL and SDA lines of an I2C bus. It supports
 * the Standard-mode and Fast-mode speeds and doesn't support optional
 * protocol feature like 10-bit addresses or clock stretching.
 *
 * Timings and protocol are based Rev. 6 of the I2C specification:
 * http://www.nxp.com/documents/user_manual/UM10204.pdf
 */

#include <errno.h>
#include <kernel.h>
#include <drivers/i2c.h>
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
	((u64_t)sys_clock_hw_cycles_per_sec() * (ns) / NSEC_PER_SEC + 1)

int i2c_bitbang_configure(struct i2c_bitbang *context, u32_t dev_config)
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

	return 0;
}

static void i2c_set_scl(struct i2c_bitbang *context, int state)
{
	context->io->set_scl(context->io_context, state);
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
	u32_t start = k_cycle_get_32();

	/* Wait until the given number of cycles have passed */
	while (k_cycle_get_32() - start < cycles_to_wait) {
	}
}

static void i2c_start(struct i2c_bitbang *context)
{
	if (!i2c_get_sda(context)) {
		/*
		 * SDA is already low, so we need to do something to make it
		 * high. Try pulsing clock low to get slave to release SDA.
		 */
		i2c_set_scl(context, 0);
		i2c_delay(context->delays[T_LOW]);
		i2c_set_scl(context, 1);
		i2c_delay(context->delays[T_SU_STA]);
	}
	i2c_set_sda(context, 0);
	i2c_delay(context->delays[T_HD_STA]);
}

static void i2c_repeated_start(struct i2c_bitbang *context)
{
	i2c_delay(context->delays[T_SU_STA]);
	i2c_start(context);
}

static void i2c_stop(struct i2c_bitbang *context)
{
	if (i2c_get_sda(context)) {
		/*
		 * SDA is already high, so we need to make it low so that
		 * we can create a rising edge. This means we're effectively
		 * doing a START.
		 */
		i2c_delay(context->delays[T_SU_STA]);
		i2c_set_sda(context, 0);
		i2c_delay(context->delays[T_HD_STA]);
	}
	i2c_delay(context->delays[T_SU_STP]);
	i2c_set_sda(context, 1);
	i2c_delay(context->delays[T_BUF]); /* In case we start again too soon */
}

static void i2c_write_bit(struct i2c_bitbang *context, int bit)
{
	i2c_set_scl(context, 0);
	/* SDA hold time is zero, so no need for a delay here */
	i2c_set_sda(context, bit);
	i2c_delay(context->delays[T_LOW]);
	i2c_set_scl(context, 1);
	i2c_delay(context->delays[T_HIGH]);
}

static bool i2c_read_bit(struct i2c_bitbang *context)
{
	bool bit;

	i2c_set_scl(context, 0);
	/* SDA hold time is zero, so no need for a delay here */
	i2c_set_sda(context, 1); /* Stop driving low, so slave has control */
	i2c_delay(context->delays[T_LOW]);
	bit = i2c_get_sda(context);
	i2c_set_scl(context, 1);
	i2c_delay(context->delays[T_HIGH]);
	return bit;
}

static bool i2c_write_byte(struct i2c_bitbang *context, u8_t byte)
{
	u8_t mask = 1 << 7;

	do {
		i2c_write_bit(context, byte & mask);
	} while (mask >>= 1);

	/* Return inverted ACK bit, i.e. 'true' for ACK, 'false' for NACK */
	return !i2c_read_bit(context);
}

static u8_t i2c_read_byte(struct i2c_bitbang *context)
{
	unsigned int byte = 1U;

	do {
		byte <<= 1;
		byte |= i2c_read_bit(context);
	} while (!(byte & (1 << 8)));

	return byte;
}

int i2c_bitbang_transfer(struct i2c_bitbang *context,
			   struct i2c_msg *msgs, u8_t num_msgs,
			   u16_t slave_address)
{
	u8_t *buf, *buf_end;
	unsigned int flags;
	int result = -EIO;

	if (!num_msgs) {
		return 0;
	}

	/* We want an initial Start condition */
	flags = I2C_MSG_RESTART;

	/* Make sure we're in a good state so slave recognises the Start */
	i2c_set_scl(context, 1);
	flags |= I2C_MSG_STOP;

	do {
		/* Stop flag from previous message? */
		if (flags & I2C_MSG_STOP) {
			i2c_stop(context);
		}

		/* Forget old flags except start flag */
		flags &= I2C_MSG_RESTART;

		/* Start condition? */
		if (flags & I2C_MSG_RESTART) {
			i2c_start(context);
		} else if (msgs->flags & I2C_MSG_RESTART) {
			i2c_repeated_start(context);
		}

		/* Get flags for new message */
		flags |= msgs->flags;

		/* Send address after any Start condition */
		if (flags & I2C_MSG_RESTART) {
			unsigned int byte0 = slave_address << 1;

			byte0 |= (flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
			if (!i2c_write_byte(context, byte0)) {
				goto finish; /* No ACK received */
			}
			flags &= ~I2C_MSG_RESTART;
		}

		/* Transfer data */
		buf = msgs->buf;
		buf_end = buf + msgs->len;
		if ((flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			/* Read */
			while (buf < buf_end) {
				*buf++ = i2c_read_byte(context);
				/* ACK the byte, except for the last one */
				i2c_write_bit(context, buf == buf_end);
			}
		} else {
			/* Write */
			while (buf < buf_end) {
				if (!i2c_write_byte(context, *buf++)) {
					goto finish; /* No ACK received */
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
	i2c_stop(context);

	return result;
}

void i2c_bitbang_init(struct i2c_bitbang *context,
			const struct i2c_bitbang_io *io, void *io_context)
{
	context->io = io;
	context->io_context = io_context;
	i2c_bitbang_configure(context, I2C_SPEED_STANDARD << I2C_SPEED_SHIFT);
}
