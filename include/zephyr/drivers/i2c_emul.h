/**
 * @file
 *
 * @brief Public APIs for the I2C emulation drivers.
 */

/*
 * Copyright 2020 Google LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_I2C_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_I2C_EMUL_H_

/**
 * @brief I2C Emulation Interface
 * @defgroup i2c_emul_interface I2C Emulation Interface
 * @ingroup io_emulators
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>

#ifdef __cplusplus
extern "C" {
#endif

struct i2c_msg;
struct i2c_emul_api;

/** Node in a linked list of emulators for I2C devices */
struct i2c_emul {
	sys_snode_t node;

	/** Parent emulator */
	const struct emul *parent;

	/* API provided for this device */
	const struct i2c_emul_api *api;

	/* I2C address of the emulated device */
	uint16_t addr;
};

/**
 * Passes I2C messages to the emulator. The emulator updates the data with what
 * was read back.
 *
 * @param emul Emulator instance
 * @param msgs Array of messages to transfer. For 'read' messages, this function
 *	updates the 'buf' member with the data that was read
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C target device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
typedef int (*i2c_emul_transfer_t)(struct i2c_emul *emul, struct i2c_msg *msgs,
				   int num_msgs, int addr);

/**
 * Register an emulated device on the controller
 *
 * @param dev Device that will use the emulator
 * @param name User-friendly name for this emulator
 * @param emul I2C emulator to use
 * @return 0 indicating success (always)
 */
int i2c_emul_register(const struct device *dev, const char *name,
		      struct i2c_emul *emul);

/** Definition of the emulator API */
struct i2c_emul_api {
	i2c_emul_transfer_t transfer;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_I2C_EMUL_H_ */
