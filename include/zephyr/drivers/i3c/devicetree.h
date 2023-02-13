/*
 * Copyright 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_DEVICETREE_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_DEVICETREE_H_

/**
 * @brief I3C Devicetree related bits
 * @defgroup i3c_devicetree I3C Devicetree related bits
 * @ingroup i3c_interface
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure initializer for i3c_device_id from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * i3c_device_id</tt> by reading the relevant device data from devicetree.
 *
 * @param node_id Devicetree node identifier for the I3C device whose
 *                struct i3c_device_id to create an initializer for
 */
#define I3C_DEVICE_ID_DT(node_id)					\
	{								\
		.pid = ((uint64_t)DT_PROP_BY_IDX(node_id, reg, 1) << 32)\
		       | DT_PROP_BY_IDX(node_id, reg, 2),		\
	}

/**
 * @brief Structure initializer for i3c_device_id from devicetree instance
 *
 * This is equivalent to
 * <tt>I3C_DEVICE_ID_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define I3C_DEVICE_ID_DT_INST(inst)					\
	I3C_DEVICE_ID_DT(DT_DRV_INST(inst))

/**
 * @brief Structure initializer for i3c_device_desc from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * i3c_device_desc</tt> by reading the relevant bus and device data
 * from the devicetree.
 *
 * @param node_id Devicetree node identifier for the I3C device whose
 *                struct i3c_device_desc to create an initializer for
 */
#define I3C_DEVICE_DESC_DT(node_id)					\
	{								\
		.bus = DEVICE_DT_GET(DT_BUS(node_id)),			\
		.dev = DEVICE_DT_GET(node_id),				\
		.static_addr = DT_PROP_BY_IDX(node_id, reg, 0),		\
		.pid = ((uint64_t)DT_PROP_BY_IDX(node_id, reg, 1) << 32)\
		       | DT_PROP_BY_IDX(node_id, reg, 2),		\
		.init_dynamic_addr =					\
			DT_PROP_OR(node_id, assigned_address, 0),	\
	},

/**
 * @brief Structure initializer for i3c_device_desc from devicetree instance
 *
 * This is equivalent to
 * <tt>I3C_DEVICE_DESC_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define I3C_DEVICE_DESC_DT_INST(inst)					\
	I3C_DEVICE_DESC_DT(DT_DRV_INST(inst))

/**
 * @brief Structure initializer for i3c_device_desc from devicetree
 *
 * This is mainly used by <tt>I3C_DEVICE_ARRAY_DT()</tt> to only
 * create a struct if and only if it is an I3C device.
 */
#define I3C_DEVICE_DESC_DT_FILTERED(node_id)				\
	COND_CODE_0(DT_PROP_BY_IDX(node_id, reg, 1),			\
		    (), (I3C_DEVICE_DESC_DT(node_id)))

/**
 * @brief Array initializer for a list of i3c_device_desc from devicetree
 *
 * This is a helper macro to generate an array for a list of i3c_device_desc
 * from device tree.
 *
 * @param node_id Devicetree node identifier of the I3C controller
 */
#define I3C_DEVICE_ARRAY_DT(node_id)					\
	{								\
		DT_FOREACH_CHILD_STATUS_OKAY(				\
			node_id,					\
			I3C_DEVICE_DESC_DT_FILTERED)			\
	}

/**
 * @brief Array initializer for a list of i3c_device_desc from devicetree instance
 *
 * This is equivalent to
 * <tt>I3C_DEVICE_ARRAY_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number of the I3C controller
 */
#define I3C_DEVICE_ARRAY_DT_INST(inst)					\
	I3C_DEVICE_ARRAY_DT(DT_DRV_INST(inst))

/**
 * @brief Like DEVICE_DT_DEFINE() with I3C target device specifics.
 *
 * Defines a I3C target device which implements the I3C target device API.
 *
 * @param node_id The devicetree node identifier.
 *
 * @param init_fn Name of the init function of the driver.
 *
 * @param pm PM device resources reference (NULL if device does not use PM).
 *
 * @param data Pointer to the device's private data.
 *
 * @param config The address to the structure containing the
 *               configuration information for this instance of the driver.
 *
 * @param level The initialization level. See SYS_INIT() for
 *              details.
 *
 * @param prio Priority within the selected initialization level. See
 *             SYS_INIT() for details.
 *
 * @param api Provides an initial pointer to the API function struct
 *            used by the driver. Can be NULL.
 */
#define I3C_DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level,	\
			     prio, api, ...)				\
	DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level,	\
			 prio, api, __VA_ARGS__)

/**
 * @brief Like I3C_TARGET_DT_DEFINE() for an instance of a DT_DRV_COMPAT compatible
 *
 * @param inst instance number. This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to I3C_TARGET_DT_DEFINE().
 *
 * @param ... other parameters as expected by I3C_TARGET_DT_DEFINE().
 */
#define I3C_DEVICE_DT_INST_DEFINE(inst, ...)				\
	I3C_DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Structure initializer for i3c_i2c_device_desc from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * i3c_i2c_device_desc</tt> by reading the relevant bus and device data
 * from the devicetree.
 *
 * @param node_id Devicetree node identifier for the I3C device whose
 *                struct i3c_i2c_device_desc to create an initializer for
 */
#define I3C_I2C_DEVICE_DESC_DT(node_id)					\
	{								\
		.bus = DEVICE_DT_GET(DT_BUS(node_id)),			\
		.addr = DT_PROP_BY_IDX(node_id, reg, 0),		\
		.lvr = DT_PROP_BY_IDX(node_id, reg, 2),			\
	},

/**
 * @brief Structure initializer for i3c_i2c_device_desc from devicetree instance
 *
 * This is equivalent to
 * <tt>I3C_I2C_DEVICE_DESC_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define I3C_I2C_DEVICE_DESC_DT_INST(inst) \
	I3C_I2C_DEVICE_DESC_DT(DT_DRV_INST(inst))


/**
 * @brief Structure initializer for i3c_i2c_device_desc from devicetree
 *
 * This is mainly used by <tt>I3C_I2C_DEVICE_ARRAY_DT()</tt> to only
 * create a struct if and only if it is an I2C device.
 */
#define I3C_I2C_DEVICE_DESC_DT_FILTERED(node_id)			\
	COND_CODE_0(DT_PROP_BY_IDX(node_id, reg, 1),			\
		    (I3C_I2C_DEVICE_DESC_DT(node_id)), ())

/**
 * @brief Array initializer for a list of i3c_i2c_device_desc from devicetree
 *
 * This is a helper macro to generate an array for a list of
 * i3c_i2c_device_desc from device tree.
 *
 * @param node_id Devicetree node identifier of the I3C controller
 */
#define I3C_I2C_DEVICE_ARRAY_DT(node_id)				\
	{								\
		DT_FOREACH_CHILD_STATUS_OKAY(				\
			node_id,					\
			I3C_I2C_DEVICE_DESC_DT_FILTERED)		\
	}

/**
 * @brief Array initializer for a list of i3c_i2c_device_desc from devicetree instance
 *
 * This is equivalent to
 * <tt>I3C_I2C_DEVICE_ARRAY_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number of the I3C controller
 */
#define I3C_I2C_DEVICE_ARRAY_DT_INST(inst)				\
	I3C_I2C_DEVICE_ARRAY_DT(DT_DRV_INST(inst))

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_DEVICETREE_H_ */
