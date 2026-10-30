/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Fake i2c-dev adapters behind the host call hooks of the driver bottom
 * (I2C_NATIVE_LINUX_TEST_HOOKS). FAKE_I2C_DEV_PATH is a plain I2C adapter
 * with a register file at FAKE_I2C_DEV_ADDR (the first written byte sets the
 * auto-incrementing pointer), FAKE_I2C_DEV_SMBUS_PATH is SMBus only.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "fake_i2c_dev_bottom.h"

#define MAX_TRANSFERS 8

/* File descriptors returned for the fake adapters */
#define FAKE_FD  100
#define SMBUS_FD 101

struct recorded_transfer {
	int nmsgs;
	struct i2c_msg msgs[I2C_RDWR_IOCTL_MAX_MSGS];
};

static uint8_t regs[256];
static uint8_t reg_ptr;
static struct recorded_transfer transfers[MAX_TRANSFERS];
static int ntransfers;

int i2c_native_linux_host_open(const char *path)
{
	if (strcmp(path, FAKE_I2C_DEV_PATH) == 0) {
		return FAKE_FD;
	}

	if (strcmp(path, FAKE_I2C_DEV_SMBUS_PATH) == 0) {
		return SMBUS_FD;
	}

	errno = ENOENT;
	return -1;
}

int i2c_native_linux_host_close(int fd)
{
	return 0;
}

static int fake_funcs(int fd, unsigned long *funcs)
{
	*funcs = I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA;

	if (fd == FAKE_FD) {
		*funcs |= I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR;
	}

	return 0;
}

static int fake_rdwr(int fd, struct i2c_rdwr_ioctl_data *rdwr)
{
	struct i2c_msg *msg;
	bool read;
	uint32_t i;
	uint16_t b;

	if (fd != FAKE_FD) {
		errno = EOPNOTSUPP;
		return -1;
	}

	if (rdwr->nmsgs == 0U || rdwr->nmsgs > I2C_RDWR_IOCTL_MAX_MSGS) {
		errno = EINVAL;
		return -1;
	}

	if (ntransfers < MAX_TRANSFERS) {
		transfers[ntransfers].nmsgs = (int)rdwr->nmsgs;
		memcpy(transfers[ntransfers].msgs, rdwr->msgs, rdwr->nmsgs * sizeof(rdwr->msgs[0]));
		ntransfers++;
	}

	for (i = 0U; i < rdwr->nmsgs; i++) {
		if (rdwr->msgs[i].addr != FAKE_I2C_DEV_ADDR) {
			errno = ENXIO;
			return -1;
		}
	}

	for (i = 0U; i < rdwr->nmsgs; i++) {
		msg = &rdwr->msgs[i];
		read = (msg->flags & I2C_M_RD) != 0U;

		for (b = 0U; b < msg->len; b++) {
			if (read) {
				msg->buf[b] = regs[reg_ptr++];
			} else if (b == 0U) {
				reg_ptr = msg->buf[0];
			} else {
				regs[reg_ptr++] = msg->buf[b];
			}
		}
	}

	return (int)rdwr->nmsgs;
}

int i2c_native_linux_host_ioctl(int fd, unsigned long request, void *arg)
{
	if (fd != FAKE_FD && fd != SMBUS_FD) {
		errno = EBADF;
		return -1;
	}

	switch (request) {
	case I2C_FUNCS:
		return fake_funcs(fd, arg);
	case I2C_RDWR:
		return fake_rdwr(fd, arg);
	default:
		errno = EINVAL;
		return -1;
	}
}

void fake_i2c_dev_reset(void)
{
	ntransfers = 0;
}

int fake_i2c_dev_transfers(void)
{
	return ntransfers;
}

int fake_i2c_dev_msgs(int transfer)
{
	return transfers[transfer].nmsgs;
}

int fake_i2c_dev_msg_len(int transfer, int msg)
{
	return transfers[transfer].msgs[msg].len;
}

int fake_i2c_dev_msg_read(int transfer, int msg)
{
	return (transfers[transfer].msgs[msg].flags & I2C_M_RD) != 0U;
}

int fake_i2c_dev_msg_ten(int transfer, int msg)
{
	return (transfers[transfer].msgs[msg].flags & I2C_M_TEN) != 0U;
}
