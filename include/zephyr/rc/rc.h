/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RC_RC_H_
#define ZEPHYR_INCLUDE_RC_RC_H_

/**
 * @file
 * @ingroup rc_interface
 * @brief Remote control (IR) subsystem API.
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/dt-bindings/rc/rc-proto.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Defined in <zephyr/drivers/rc.h> */
struct rc_caps;

/**
 * @brief Remote control (IR) interface
 * @defgroup rc_interface Remote Control
 * @since 4.6
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name Event flags
 * @{
 */

/** Toggle bit of the frame is set (same value as LIRC_SCANCODE_FLAG_TOGGLE) */
#define RC_EVENT_FLAG_TOGGLE  BIT(0)
/** Frame repeats the held key (same value as LIRC_SCANCODE_FLAG_REPEAT) */
#define RC_EVENT_FLAG_REPEAT  BIT(1)
/** Held key released */
#define RC_EVENT_FLAG_RELEASE BIT(2)

/** @} */

/** @brief Decoded remote control event. */
struct rc_event {
	/** Uptime in ns when the frame was decoded */
	uint64_t timestamp_ns;
	/** RC_EVENT_FLAG_* bits */
	uint16_t flags;
	/** Protocol, one of RC_PROTO_* */
	uint16_t proto;
	/** Key code from the keymap, 0 if none */
	uint32_t keycode;
	/** Scancode in the protocol's Linux layout */
	uint32_t scancode;
};

/** @brief Scancode transmit request. */
struct rc_tx_req {
	/** Protocol, one of RC_PROTO_* */
	uint16_t proto;
	/** Number of repeats after the first frame */
	uint16_t repeats;
	/** Scancode in the protocol's Linux layout */
	uint32_t scancode;
};

/**
 * @name Raw words
 * @{
 */

/** Maximum duration in us of one raw word */
#define RC_RAW_DURATION_MASK GENMASK(23, 0)
/** Word is a pulse (carrier on); cleared for a space */
#define RC_RAW_PULSE         BIT(24)
/** Word marks receiver timeout (end of frame) */
#define RC_RAW_TIMEOUT       BIT(25)
/** Word marks a receive buffer overflow */
#define RC_RAW_OVERFLOW      BIT(26)

/** Pulse of @p us microseconds, clamped to RC_RAW_DURATION_MASK */
#define RC_RAW_PULSE_US(us) (MIN((uint32_t)(us), RC_RAW_DURATION_MASK) | RC_RAW_PULSE)
/** Space of @p us microseconds, clamped to RC_RAW_DURATION_MASK */
#define RC_RAW_SPACE_US(us) MIN((uint32_t)(us), RC_RAW_DURATION_MASK)

/** @} */

/**
 * @brief Get the duration of a raw word.
 *
 * @param word Raw word.
 *
 * @return Duration in us.
 */
static inline uint32_t rc_raw_duration_us(uint32_t word)
{
	return word & RC_RAW_DURATION_MASK;
}

/**
 * @brief Check whether a raw word is a pulse.
 *
 * @param word Raw word.
 *
 * @retval true Word is a pulse.
 * @retval false Word is a space.
 */
static inline bool rc_raw_is_pulse(uint32_t word)
{
	return (word & RC_RAW_PULSE) != 0U;
}

/**
 * @brief Get the capabilities of an rc device.
 *
 * @param dev rc device.
 * @param caps Filled with the capabilities.
 *
 * @retval 0 Success.
 */
int rc_get_caps(const struct device *dev, struct rc_caps *caps);

/**
 * @brief Set the receiver timeout.
 *
 * @param dev rc device.
 * @param us Timeout in us.
 *
 * @retval 0 Success.
 * @retval -EINVAL Timeout out of range.
 * @retval <0 Driver error.
 */
int rc_set_rx_timeout(const struct device *dev, uint32_t us);

/**
 * @brief Enable decoding of a protocol on a device.
 *
 * @param dev rc device.
 * @param proto Protocol, one of RC_PROTO_*.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP No decoder for @p proto is built.
 */
int rc_protocol_enable(const struct device *dev, uint16_t proto);

/**
 * @brief Disable decoding of a protocol on a device.
 *
 * @param dev rc device.
 * @param proto Protocol, one of RC_PROTO_*.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP No decoder for @p proto is built.
 */
int rc_protocol_disable(const struct device *dev, uint16_t proto);

/**
 * @brief Check whether decoding of a protocol is enabled on a device.
 *
 * @param dev rc device.
 * @param proto Protocol, one of RC_PROTO_*.
 *
 * @retval 1 Enabled.
 * @retval 0 Disabled.
 * @retval -ENOTSUP No decoder for @p proto is built.
 */
int rc_protocol_is_enabled(const struct device *dev, uint16_t proto);

/**
 * @brief Callback for rc events.
 *
 * @param dev rc device of the event.
 * @param evt Event, valid during the call only.
 * @param user_data User data of the callback definition.
 */
typedef void (*rc_callback_t)(const struct device *dev, const struct rc_event *evt,
			      void *user_data);

/** @brief rc callback definition, see RC_CALLBACK_DEFINE(). */
struct rc_callback {
	/** rc device, or NULL for all rc devices */
	const struct device *dev;
	/** Callback function */
	rc_callback_t callback;
	/** User data passed to the callback */
	void *user_data;
};

/**
 * @brief Register a callback for rc events, with an explicit name.
 *
 * @param _dev rc device, or NULL for the events of all rc devices.
 * @param _callback Callback function, see rc_callback_t.
 * @param _user_data User data passed to the callback.
 * @param _name Name of the callback definition.
 */
#define RC_CALLBACK_DEFINE_NAMED(_dev, _callback, _user_data, _name)                               \
	static const STRUCT_SECTION_ITERABLE(rc_callback, _rc_callback__##_name) = {               \
		.dev = _dev,                                                                       \
		.callback = _callback,                                                             \
		.user_data = _user_data,                                                           \
	}

/**
 * @brief Register a callback for rc events.
 *
 * @param _dev rc device, or NULL for the events of all rc devices.
 * @param _callback Callback function, see rc_callback_t.
 * @param _user_data User data passed to the callback.
 */
#define RC_CALLBACK_DEFINE(_dev, _callback, _user_data)                                            \
	RC_CALLBACK_DEFINE_NAMED(_dev, _callback, _user_data, _callback)

/** @cond INTERNAL_HIDDEN */
struct rc_iodev_config {
	const struct device *dev;
};

extern const struct rtio_iodev_api rc_event_iodev_api;

struct rc_raw_iodev_data {
	struct rc_iodev_config config;
	struct k_spinlock lock;
	struct rtio_iodev_sqe *head;
	struct rtio_iodev_sqe *tail;
	bool active;
	bool missed;
};

extern const struct rtio_iodev_api rc_raw_iodev_api;
/** @endcond */

/**
 * @brief Define an RTIO iodev streaming the events of an rc device.
 *
 * @param name Name of the iodev.
 * @param node_id Devicetree node identifier of the rc device.
 */
#define RC_DT_EVENT_IODEV(name, node_id)                                                           \
	static struct rc_iodev_config _CONCAT(name, _config) = {                                   \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	};                                                                                         \
	RTIO_IODEV_DEFINE(name, &rc_event_iodev_api, &_CONCAT(name, _config))

/**
 * @brief Define an RTIO iodev streaming the raw words of an rc device.
 *
 * @param name Name of the iodev.
 * @param node_id Devicetree node identifier of the rc device.
 */
#define RC_DT_RAW_IODEV(name, node_id)                                                             \
	static STRUCT_SECTION_ITERABLE(rc_raw_iodev_data, _CONCAT(name, _data)) = {                \
		.config = {.dev = DEVICE_DT_GET(node_id)},                                         \
	};                                                                                         \
	RTIO_IODEV_DEFINE(name, &rc_raw_iodev_api, &_CONCAT(name, _data))

/**
 * @brief Start streaming events into an RTIO context.
 *
 * @param iodev Iodev defined with RC_DT_EVENT_IODEV() or RC_DT_RAW_IODEV().
 * @param ctx RTIO context with a memory pool.
 * @param userdata Userdata of the completions.
 * @param handle Filled with the submission handle, may be NULL.
 *
 * @retval 0 Success.
 * @retval -ENOMEM No free submission in @p ctx.
 */
static inline int rc_stream(const struct rtio_iodev *iodev, struct rtio *ctx, void *userdata,
			    rtio_sqe_handle_t *handle)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		struct rtio_sqe sqe;
		int ret;

		rtio_sqe_prep_read_multishot(&sqe, iodev, RTIO_PRIO_NORM, userdata);
		ret = rtio_sqe_copy_in_get_handles(ctx, &sqe, handle, 1);
		if (ret != 0) {
			return ret;
		}
	} else {
		struct rtio_sqe *sqe = rtio_sqe_acquire(ctx);

		if (sqe == NULL) {
			return -ENOMEM;
		}
		if (handle != NULL) {
			*handle = rtio_sqe_handle(ctx, sqe);
		}
		rtio_sqe_prep_read_multishot(sqe, iodev, RTIO_PRIO_NORM, userdata);
	}

	return rtio_submit(ctx, 0);
}

/**
 * @brief Wait for the next event of an rc device.
 *
 * Not callable from an ISR or the rc work queues.
 *
 * @param dev rc device.
 * @param evt Filled with the event.
 * @param timeout Time to wait for an event.
 *
 * @retval 0 Success.
 * @retval -EAGAIN No event within @p timeout.
 * @retval -ENOMEM CONFIG_RC_RECEIVE_SYNC_MAX callers already waiting, counted
 *         over all rc devices.
 * @retval -ENODEV Device not ready.
 * @retval -EDEADLK Called from the rc receive work queue.
 */
int rc_receive_sync(const struct device *dev, struct rc_event *evt, k_timeout_t timeout);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RC_RC_H_ */
