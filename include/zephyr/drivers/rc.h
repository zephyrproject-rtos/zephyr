/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RC_H_

/**
 * @file
 * @ingroup rc_driver_interface
 * @brief Remote control (IR) driver API.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Remote control driver interface
 * @defgroup rc_driver_interface Remote Control Drivers
 * @ingroup rc_interface
 * @{
 */

/** @brief Driver roles, combined in rc_caps::roles. */
enum rc_driver_role {
	/** Receiver reporting raw pulse and space durations */
	RC_ROLE_RAW_RX = BIT(0),
	/** Transmitter of raw pulse and space durations */
	RC_ROLE_RAW_TX = BIT(1),
};

/** @brief Driver capabilities. */
struct rc_caps {
	/** rc_driver_role bits */
	uint32_t roles;
	/** Receive timing resolution in ns */
	uint32_t rx_resolution_ns;
	/** Minimum TX carrier frequency in Hz, 0 if fixed or none */
	uint32_t carrier_min_hz;
	/** Maximum TX carrier frequency in Hz, 0 if fixed or none */
	uint32_t carrier_max_hz;
	/** Maximum raw words per transmission, 0 if unbounded */
	uint32_t max_tx_words;
	/** Minimum receiver timeout in us */
	uint32_t rx_timeout_min_us;
	/** Maximum receiver timeout in us */
	uint32_t rx_timeout_max_us;
};

/** @brief Remote control driver API. */
__subsystem struct rc_driver_api {
	/** Get capabilities. Mandatory. */
	int (*get_caps)(const struct device *dev, struct rc_caps *caps);
	/** Start or stop reception. Mandatory for receivers. */
	int (*rx_enable)(const struct device *dev, bool enable);
	/** Set receiver timeout in us. Optional. */
	int (*set_rx_timeout)(const struct device *dev, uint32_t us);
	/** Set TX carrier frequency and duty cycle. Optional. */
	int (*set_carrier)(const struct device *dev, uint32_t hz, uint8_t duty_pct);
	/** Transmit raw words, blocking until the last word is sent. Mandatory for transmitters. */
	int (*tx)(const struct device *dev, const uint32_t *words, size_t count);
};

/**
 * @brief Common rc state of a device, the first member of the driver data.
 */
struct rc_dev_data {
	/** @cond INTERNAL_HIDDEN */
	struct rc_caps caps;
	ATOMIC_DEFINE(protocols, RC_PROTOCOL_MAX);
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */
int z_rc_device_init(const struct device *dev, int (*init_fn)(const struct device *dev),
		     const uint16_t *protocols, size_t num_protocols);

#define Z_RC_INIT_NAME(node_id)      _CONCAT(z_rc_init_, DT_DEP_ORD(node_id))
#define Z_RC_PROTOCOLS_NAME(node_id) _CONCAT(z_rc_protocols_, DT_DEP_ORD(node_id))

#define Z_RC_PROTOCOLS_DEFINE(node_id)                                                             \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, protocols),                                           \
		   (static const uint16_t Z_RC_PROTOCOLS_NAME(node_id)[] =                         \
			    DT_PROP(node_id, protocols);))

#define Z_RC_PROTOCOLS_ARGS(node_id)                                                               \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, protocols),                                          \
		    (Z_RC_PROTOCOLS_NAME(node_id), ARRAY_SIZE(Z_RC_PROTOCOLS_NAME(node_id))),      \
		    (NULL, 0))
/** @endcond */

/**
 * @brief Define an rc device from a devicetree node.
 *
 * @param node_id Devicetree node identifier.
 * @param init_fn Driver init function, may be NULL.
 * @param pm PM device resources reference, may be NULL.
 * @param data Driver data.
 * @param config Driver config, may be NULL.
 * @param level Initialization level.
 * @param prio Initialization priority.
 * @param api Pointer to the struct rc_driver_api of the driver.
 */
#define RC_DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level, prio, api)                  \
	Z_RC_PROTOCOLS_DEFINE(node_id)                                                             \
	static int _CONCAT(z_rc_init_, DT_DEP_ORD(node_id))(const struct device *dev)              \
	{                                                                                          \
		return z_rc_device_init(dev, init_fn, Z_RC_PROTOCOLS_ARGS(node_id));               \
	}                                                                                          \
	DEVICE_DT_DEFINE(node_id, Z_RC_INIT_NAME(node_id), pm, data, config, level, prio, api)

/**
 * @brief Like RC_DEVICE_DT_DEFINE() for an instance of a DT_DRV_COMPAT compatible.
 *
 * @param inst Instance number.
 * @param ... Other parameters as expected by RC_DEVICE_DT_DEFINE().
 */
#define RC_DEVICE_DT_INST_DEFINE(inst, ...) RC_DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Start or stop reception.
 *
 * @param dev rc device.
 * @param enable true to start, false to stop.
 *
 * @retval 0 Success.
 * @retval -ENOSYS Device cannot receive.
 * @retval <0 Driver error.
 */
int rc_rx_enable(const struct device *dev, bool enable);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RC_H_ */
