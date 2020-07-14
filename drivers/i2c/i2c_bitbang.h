/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Functions for setting and getting the state of the I2C lines.
 *
 * These need to be implemented by the user of this library.
 */
struct i2c_bitbang_io {
	/* Set the state of the SCL line (zero/non-zero value) */
	void (*set_scl)(void *io_context, int state);
	/* Set the state of the SDA line (zero/non-zero value) */
	void (*set_sda)(void *io_context, int state);
	/* Return the state of the SDA line (zero/non-zero value) */
	int (*get_sda)(void *io_context);
};

/**
 * @brief Instance data for i2c_bitbang
 *
 * A driver or other code wishing to use the i2c_bitbang library should
 * create one of these structures then use it via the library APIs.
 * Structure members are private, and shouldn't be accessed directly.
 */
struct i2c_bitbang {
	const struct i2c_bitbang_io	*io;
	void				*io_context;
	uint32_t				delays[2];
};

/**
 * @brief Initialize an i2c_bitbang instance
 *
 * @param bitbang	The instance to initialize
 * @param io		Functions to use for controlling I2C bus lines
 * @param io_context	Context pointer to pass to i/o functions when then are
 *			called.
 */
void i2c_bitbang_init(struct i2c_bitbang *bitbang,
			const struct i2c_bitbang_io *io, void *io_context);

/**
 * Implementation of the functionality required by the 'configure' function
 * in struct i2c_driver_api.
 */
int i2c_bitbang_configure(struct i2c_bitbang *bitbang, uint32_t dev_config);

/**
 * Implementation of the functionality required by the 'recover_bus'
 * function in struct i2c_driver_api.
 */
int i2c_bitbang_recover_bus(struct i2c_bitbang *bitbang);

/**
 * Implementation of the functionality required by the 'transfer' function
 * in struct i2c_driver_api.
 */
int i2c_bitbang_transfer(struct i2c_bitbang *bitbang,
			   struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t slave_address);
