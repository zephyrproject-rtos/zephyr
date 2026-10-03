/*
 * SPDX-FileCopyrightText: Copyright (c) 2017 BayLibre, SAS
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the I2C target library.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_TARGET_REGSET_TARGET_LIB_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_TARGET_REGSET_TARGET_LIB_H_

#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Register set library internal APIs.
 */
struct regset_target_lib_api {
	/**
	 * @brief Callback invoked when a single register is read.
	 *
	 * Called after the register value has been fetched from the buffer.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param offset Address offset of the register being read.
	 * @param reg Pointer to the register in the buffer.
	 * @param val Pointer to the value to be returned to the controller.
	 */
	void (*read)(const struct device *dev, off_t offset, uint8_t *reg, uint8_t *val);
	/**
	 * @brief Callback invoked when a single register is written.
	 *
	 * Called instead of the default buffer write when provided.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param offset Address offset of the register being written.
	 * @param reg Pointer to the register in the buffer.
	 * @param val Value being written to the register.
	 */
	void (*write)(const struct device *dev, off_t offset, uint8_t *reg, uint8_t val);
	/**
	 * @brief Callback invoked when the register set content has changed.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 */
	void (*changed)(const struct device *dev);
};

/**
 * @brief Register set library config
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct regset_target_lib_data {
	/** @cond INTERNAL_HIDDEN */
	const struct device *dev;
	struct i2c_target_config config;
	uint32_t buffer_idx;
	uint32_t idx_write_cnt;
	bool changed;
	/** @endcond */
};

/**
 * @brief Register set library data
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct regset_target_lib_config {
	/** @cond INTERNAL_HIDDEN */
	const struct regset_target_lib_api *api;
	struct i2c_dt_spec bus;
	uint32_t buffer_size;
	uint8_t *buffer;
	uint8_t address_width;
	bool auto_register;
	/** @endcond */
};

/**
 * @brief Get size of the register set
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return Size of register set in bytes
 */
size_t regset_target_lib_get_size(const struct device *dev);

/**
 * @brief Read data from the register set
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Address offset to read from.
 * @param data Buffer to store read data.
 * @param len Number of bytes to read.
 *
 * @return 0 on success, negative errno code on failure.
 */
int regset_target_lib_read_data(const struct device *dev, off_t offset,
				void *data, size_t len);

/**
 * @brief Write data to the register set
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param offset Address offset to write data to.
 * @param data Buffer with data to write.
 * @param len Number of bytes to write.
 *
 * @return 0 on success, negative errno code on failure.
 */
int regset_target_lib_write_data(const struct device *dev, off_t offset,
				 const void *data, size_t len);

/**
 * @brief Change the address of register set target at runtime
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param addr New address to assign to the register set target device
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -EIO General input / output error during i2c_taget_register
 * @retval -ENOSYS If target mode is not implemented
 */
int regset_target_lib_set_addr(const struct device *dev, uint8_t addr);

/**
 * @brief Generig register set library i2c target API
 */
extern const struct i2c_target_driver_api regset_target_api;

/**
 * @brief Initialize the register set data structures
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 on success, negative errno code on failure.
 */
int regset_target_lib_init(const struct device *dev);

/**
 * @brief Validate the offset of the regset data structures.
 *
 * @param config Name of the config structure.
 * @param data Name of the data structure.
 */
#define REGSET_TARGET_LIB_STRUCT_CHECK(config, data)				\
	BUILD_ASSERT(offsetof(config, regset_cfg) == 0,				\
		     "struct regset_target_lib_config must be placed first");	\
	BUILD_ASSERT(offsetof(data, regset_data) == 0,				\
		     "struct regset_target_lib_data must be placed first")

/**
 * @brief Validate the node properties from devicetree
 */
#define REGSET_TARGET_LIB_DT_BUILD_ASSERT(node_id)						\
	BUILD_ASSERT(DT_PROP(node_id, size) <= (1 << DT_PROP_OR(node_id, address_width, 8)),	\
		     "size must be <= than 2^address_width");

/**
 * @brief Validate the node properties from a devicetree instance
 */
#define REGSET_TARGET_LIB_DT_INST_BUILD_ASSERT(inst)		\
	REGSET_TARGET_LIB_DT_BUILD_ASSERT(DT_DRV_INST(inst))

/**
 * @brief Define the common data structure from devicetree
 *
 * @param node_id The devicetree node identifier.
 * @param _api Pointer to a @ref regset_target_lib_api structure.
 */
#define REGSET_TARGET_LIB_DT_CONFIG_INIT(node_id, _api)		\
{								\
	.api = _api,						\
	.bus = I2C_DT_SPEC_GET(node_id),			\
	.buffer_size = DT_PROP(node_id, size),			\
	.buffer = (uint8_t[DT_PROP(node_id, size)]) {},		\
	.address_width = DT_PROP_OR(node_id, address_width, 8),	\
	.auto_register = DT_PROP(node_id, auto_register),	\
}

/**
 * @brief Define the common data structure from a devicetree instance
 *
 * @param inst Instance.
 * @param _api Pointer to a @ref regset_target_lib_api structure.
 */
#define REGSET_TARGET_LIB_DT_INST_CONFIG_INIT(inst, _api) \
	REGSET_TARGET_LIB_DT_CONFIG_INIT(DT_DRV_INST(inst), _api)

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_TARGET_REGSET_TARGET_LIB_H_ */
