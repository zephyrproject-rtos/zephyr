/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I2C_NATIVE_LINUX_BOTTOM_H
#define I2C_NATIVE_LINUX_BOTTOM_H

#include <stdbool.h>
#include <stdint.h>

/* Messages of one transfer, at most I2C_RDWR_IOCTL_MAX_MSGS */
#define I2C_NATIVE_LINUX_MAX_MSGS 42

/* Mapped to I2C_M_* on the host side */
#define I2C_NATIVE_LINUX_M_RD  (1U << 0)
#define I2C_NATIVE_LINUX_M_TEN (1U << 1)

struct i2c_native_linux_msg {
	uint8_t *buf;
	uint16_t addr;
	uint16_t flags;
	uint16_t len;
};

/* Errors are negative nsi_errno.h values; EOPNOTSUPP for an SMBus only adapter */

/*
 * Open the i2c-dev node at <path> and start the host thread that runs its
 * transfers. Returns a handle, or NULL with the error in <err>.
 */
void *i2c_native_linux_open(const char *path, int *err);

/*
 * Hand <num> messages to the host thread: repeated starts between them, one
 * final stop. I2C_NATIVE_LINUX_IRQ is raised when the transfer is done.
 */
void i2c_native_linux_transfer_start(void *handle, const struct i2c_native_linux_msg *msgs,
				     int num);

/* True once per transfer when it is done, with its result in <result> */
bool i2c_native_linux_transfer_done(void *handle, int *result);

/* Stop the host thread and close the node */
void i2c_native_linux_close(void *handle);

#endif /* I2C_NATIVE_LINUX_BOTTOM_H */
