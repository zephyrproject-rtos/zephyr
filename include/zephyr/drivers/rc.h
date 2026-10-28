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
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/spsc_lockfree.h>
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

/** @cond INTERNAL_HIDDEN */
struct rc_raw_ring {
	struct spsc _spsc;
	uint32_t *const buffer;
};
/** @endcond */

/**
 * @brief Common rc state of a device, the first member of the driver data.
 */
struct rc_dev_data {
	/** @cond INTERNAL_HIDDEN */
	const struct device *dev;
	struct rc_caps caps;
	ATOMIC_DEFINE(protocols, RC_PROTOCOL_MAX);
	ATOMIC_DEFINE(reset_pending, RC_PROTOCOL_MAX);
	struct rc_raw_ring *ring;
	uint8_t *decoder_state;
	struct k_spinlock lock;
	atomic_t flags;
	uint64_t last_edge;
	uint64_t last_store;
	atomic_t rx_timeout_us;
	struct k_work rx_work;
	struct k_work_delayable timeout_work;
	struct k_spinlock evt_lock;
	struct rtio_iodev_sqe *evt_head;
	struct rtio_iodev_sqe *evt_tail;
	struct rc_event key;
	const struct rc_protocol *key_decoder;
	bool key_pressed;
	int64_t keyup_deadline;
	struct k_work_delayable keyup_work;
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */
struct z_rc_init_cfg {
	const uint16_t *protocols;
	size_t num_protocols;
	uint32_t rx_timeout_us;
	struct rc_raw_ring *ring;
	uint8_t *decoder_state;
};

int z_rc_device_init(const struct device *dev, int (*init_fn)(const struct device *dev),
		     const struct z_rc_init_cfg *cfg);

#define Z_RC_NAME(prefix, node_id)   _CONCAT(prefix, DT_DEP_ORD(node_id))
#define Z_RC_INIT_NAME(node_id)      Z_RC_NAME(z_rc_init_, node_id)
#define Z_RC_PROTOCOLS_NAME(node_id) Z_RC_NAME(z_rc_protocols_, node_id)

#define Z_RC_PROTOCOLS_DEFINE(node_id)                                                             \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, protocols),                                           \
		   (static const uint16_t Z_RC_PROTOCOLS_NAME(node_id)[] =                         \
			    DT_PROP(node_id, protocols);))

#define Z_RC_PROTOCOLS_PTR(node_id)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, protocols), (Z_RC_PROTOCOLS_NAME(node_id)), (NULL))

#define Z_RC_PROTOCOLS_NUM(node_id)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, protocols),                                          \
		    (ARRAY_SIZE(Z_RC_PROTOCOLS_NAME(node_id))), (0))

#define Z_RC_STATE_DEFINE(node_id)                                                                 \
	BUILD_ASSERT(IS_POWER_OF_TWO(CONFIG_RC_RAW_BUFFER_SIZE));                                  \
	static uint32_t Z_RC_NAME(z_rc_raw_buf_, node_id)[CONFIG_RC_RAW_BUFFER_SIZE];              \
	static struct rc_raw_ring Z_RC_NAME(z_rc_raw_ring_, node_id) =                             \
		SPSC_INITIALIZER(CONFIG_RC_RAW_BUFFER_SIZE, Z_RC_NAME(z_rc_raw_buf_, node_id));    \
	static uint8_t Z_RC_NAME(z_rc_decoder_state_, node_id)[CONFIG_RC_DECODER_STATE_SIZE]       \
		__aligned(8);                                                                      \
	static const struct z_rc_init_cfg Z_RC_NAME(z_rc_init_cfg_, node_id) = {                   \
		.protocols = Z_RC_PROTOCOLS_PTR(node_id),                                          \
		.num_protocols = Z_RC_PROTOCOLS_NUM(node_id),                                      \
		.rx_timeout_us = DT_PROP_OR(node_id, rx_timeout_us, 0),                            \
		.ring = &Z_RC_NAME(z_rc_raw_ring_, node_id),                                       \
		.decoder_state = Z_RC_NAME(z_rc_decoder_state_, node_id),                          \
	}
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
	Z_RC_STATE_DEFINE(node_id);                                                                \
	static int _CONCAT(z_rc_init_, DT_DEP_ORD(node_id))(const struct device *dev)              \
	{                                                                                          \
		return z_rc_device_init(dev, init_fn, &Z_RC_NAME(z_rc_init_cfg_, node_id));        \
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

/**
 * @brief Store one raw word received by a driver.
 *
 * ISR safe.
 *
 * @param dev rc device.
 * @param word Raw word, see RC_RAW_PULSE_US() and RC_RAW_SPACE_US().
 *
 * @retval 0 Success.
 * @retval -ENOMEM Buffer full, word dropped.
 */
int rc_raw_store(const struct device *dev, uint32_t word);

/**
 * @brief Store the interval ending at a signal edge.
 *
 * ISR safe.
 *
 * @param dev rc device.
 * @param pulse_start true if the edge starts a pulse, so the interval
 *        ending at it was a space.
 *
 * @retval 0 Success.
 * @retval -ENOMEM Buffer full, word dropped.
 */
int rc_raw_store_edge(const struct device *dev, bool pulse_start);

/**
 * @brief Report a receive overflow detected by the driver.
 *
 * ISR safe.
 *
 * @param dev rc device.
 */
void rc_raw_overflow(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RC_H_ */
