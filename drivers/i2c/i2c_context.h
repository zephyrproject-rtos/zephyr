/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_CONTEXT_H
#define ZEPHYR_DRIVERS_I2C_CONTEXT_H

/* check if device.h was already included before i2c.h */
#ifdef ZEPHYR_INCLUDE_DEVICE_H
#error Only include i2c_context.h in drivers
#endif

#include <stdint.h>
#include <device_structs.h>

#if defined(CONFIG_I2C_STATS) || defined(__DOXYGEN__)
#include <stats/stats.h>

/** @cond INTERNAL_HIDDEN */

STATS_SECT_START(i2c)
STATS_SECT_ENTRY32(bytes_read)
STATS_SECT_ENTRY32(bytes_written)
STATS_SECT_ENTRY32(message_count)
STATS_SECT_ENTRY32(transfer_call_count)
STATS_SECT_END;

STATS_NAME_START(i2c)
STATS_NAME(i2c, bytes_read)
STATS_NAME(i2c, bytes_written)
STATS_NAME(i2c, message_count)
STATS_NAME(i2c, transfer_call_count)
STATS_NAME_END(i2c);

#endif
/** @endcond */


struct i2c_msg;

/**
 * @brief I2C specific device state which allows for i2c device class common data
 */
struct i2c_device_state {
	struct device_state devstate;
	int magic;
#if defined(CONFIG_I2C_STATS) || defined(__DOXYGEN__)
	struct stats_i2c stats;
#endif
};

/**
 * @brief Define a statically allocated and section assigned i2c device state
 */
#define Z_DEVICE_STATE_TYPE struct i2c_device_state


/* When needed initialize the stats part of device state */
#ifdef CONFIG_I2C_STATS
#define Z_I2C_STATS_INIT(dev, state)					\
	do {								\
		stats_init(&state->stats.s_hdr, STATS_SIZE_32, 4,	\
			   STATS_NAME_INIT_PARMS(i2c));			\
		stats_register(dev->name, &(state->stats.s_hdr));	\
	} while (0)
#else
#define Z_I2C_STATS_INIT(dev, state)
#endif


/**
 * @brief Define an i2c device init wrapper function
 *
 * This does device instance specific initialization of common data (such as stats)
 * and calls the given init_fn
 */
#define Z_DEVICE_INIT_WRAPPER(dev_name, init_fn) UTIL_CAT(dev_name, _init)

#define Z_DEVICE_INIT_WRAPPER_DEFINE(dev_name, init_fn)			\
	static int Z_DEVICE_INIT_WRAPPER(dev_name, _init)(const struct device *dev)	\
	{								\
		struct i2c_device_state *state =			\
			CONTAINER_OF(dev->state, struct i2c_device_state, devstate); \
		state->magic = Z_I2C_MAGIC;				\
		Z_I2C_STATS_INIT(dev, state);				\
		return init_fn(dev);					\
	}

#include <device.h>
#include <drivers/i2c.h>

#endif /* I2C_CONTEXT_H */
