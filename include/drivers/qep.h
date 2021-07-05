/**
 * @file
 *
 * @brief Public APIs for the QEP drivers.
 */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DRIVERS_QEP_H
#define __DRIVERS_QEP_H

/**
 * @brief QEP Interface
 * @defgroup qep_interface QEP Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <device.h>

/* Macros for reporting direction */
#define QEP_DIRECTION_CLOCKWISE (0)
#define QEP_DIRECTION_COUNTER_CLOCKWISE (1)
#define QEP_DIRECTION_UNKNOWN (2)

/* Enumeration of events reported by QEP in callback function. */
typedef enum {
	/** Watchdog timeout occurred. */
	QEP_EVENT_WDT_TIMEOUT,

	/** Position counter reset when counting up. */
	QEP_EVENT_CTR_RST_UP,

	/** Position counter reset when counting down.*/
	QEP_EVENT_CTR_RST_DN,

	/** Direction change detected. */
	QEP_EVENT_DIR_CHANGE,

	/** Phase error detected. */
	QEP_EVENT_PHASE_ERR,

	/** Capture done for given count. */
	QEP_EVENT_EDGE_CAP_DONE,

	/** Capture cancelled. */
	QEP_EVENT_EDGE_CAP_CANCELLED,

	/** Unknown event detected. */
	QEP_EVENT_UNKNOWN
} qep_event_t;

/* Callback for qep  provided when starting qep decode or edge capture mode
 * parameters :
 * struct device *dev - Device pointer in callback
 * void * param - User provided parameter when callback is called.
 * qep_event_t event - qep event for the callback
 * length -Valid in capture mode , indicates the number of edges that have
 * been captured
 */
typedef  void (*qep_callback_t)(struct device *dev, void *param,
				qep_event_t event, uint32_t length);

/* Enumeration for selecting qep mode of operation
 * QEP Decoder mode - For getting position count and direction.
 * QEP Edge Capture mode - For capturing edge timestamps.
 */
enum qep_mode {
	/** QEP Decoder mode. */
	QEP_MODE_QEP_DECODER,

	/** QEP Edge capture mode. */
	QEP_MODE_EDGE_CAPTURE
};

/* Position counter reset mode selection. */
enum qep_pos_ctr_rst {
	/** Reset position count after reaching the programmed max count.*/
	QEP_POS_CTR_RST_ON_MAX_CNT,

	/** Reset position count on index  event */
	QEP_POS_CTR_RST_ON_IDX_EVT
};

/* Edge type to be used as trigger for QEP / Capture events. */
enum qep_edge_type {
	/** User rising edge for events */
	QEP_EDGE_SELECT_RISING,

	/** Use falling edge for events */
	QEP_EDGE_SELECT_FALLING
};

/* Index gate select - when position counter reset is based on index
 * event.
 */
enum qep_idx_gate_sel {
	QEP_PH_A_LOW_PH_B_LOW,
	QEP_PH_A_LOW_PH_B_HIGH,
	QEP_PH_A_HIGH_PH_B_LOW,
	QEP_PH_A_HIGH_PH_B_HIGH
};

/* Edge capture configuration in edge capture mode.*/
enum qep_edge_cap_sel {
	/** Trigger event only on rising or falling edge.*/
	QEP_EDGE_CAP_SINGLE,

	/** Trigger event on both rising and falling edge.*/
	QEP_EDGE_CAP_DOUBLE
};

/* Configuration structure for QEP module */

struct qep_config {
	/**  Mode of operation */
	enum qep_mode mode;

	/**  Select swapping of inputs phase A and Phase B */
	bool swap_a_b_input;

	/**  Counter reset mode select. 1 pulse = 4 counts.*/
	enum qep_pos_ctr_rst pos_ctr_rst;

	/** Pulse per rev in quadrature decoder mode */
	uint32_t pulses_per_rev;

	/**  Rising or falling edge */
	enum qep_edge_type edge_type;

	/**  Noise filter widht in nanoseconds */
	uint32_t filter_width_ns;

	/**  Enable or disable filter */
	bool filter_en;

	/** Watchdog timeout in microseconds, to detect stalls when operating in
	 * quadrature decoder mode.
	 */
	uint32_t wdt_timeout_us;

	/** Enable or disable watchdog timeouts when in QEP decode mode */
	bool wdt_en;

	/** Index gating select. */
	enum qep_idx_gate_sel index_gating;

	/** Select single or double edges to be captured when in capture mode.*/
	enum qep_edge_cap_sel cap_edges;
};

__subsystem struct qep_driver_api {
	/** Start qep decoding. */
	int (*start_decode)(const struct device *dev, qep_callback_t cb,
			    void *cb_param);

	/** Stop qep decoding. */
	int (*stop_decode)(const struct device *dev);

	/** Get direction based on last reset event. */
	int (*get_direction)(const struct device *dev, uint32_t *p_direction);

	/** Get posotion count. */
	int (*get_position_count)(const struct device *dev,
				  uint32_t *p_current_count);

	/** Start edge capture when configured fore edge capture mode. */
	int (*start_capture)(const struct device *dev, uint64_t *buffer,
			     uint32_t count, qep_callback_t cb,
			     void *cb_param);

	/** Stop edge capture. */
	int (*stop_capture)(const struct device *dev);

	/** Enable QEP  decode event. */
	int (*enable_event)(const struct device *dev, qep_event_t event);

	/** Disable  QEP decode event. */
	int (*disable_event)(const struct device *dev, qep_event_t event);

	/** Get phase error status. */
	int (*get_phase_err_status)(const struct device *dev,
				    uint32_t *p_phase_error);

	/** Configure qep device. */
	int (*config_device)(const struct device *dev,
			     struct qep_config *config);
};

/**
 * @brief Configure QEP device.
 *
 * This routine configures the QEP device for various parmeters such as
 * operating mode, edge selection, filter configuration, watchdog
 * timeout etc.
 *
 * This function needs to be called before any operation on the QEP device
 * can be started.
 *
 * @param dev QEP device structure.
 * @param config Ponter to QEP configuration structure.
 *
 * @retval -EINVAL if the parameters in the configuration struct are
 *          invalid
 * @retval 0 if configuration is successful
 * @retval -ENTOSUP if this API is not supported.
 */
__syscall int qep_config_device(const struct device *dev,
				struct qep_config *config);

static inline int  z_impl_qep_config_device(const struct device *dev,
					    struct qep_config *config)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->config_device) {
		return api->config_device(dev, config);
	}
	return -ENOTSUP;
}

/**
 * @brief Start QEP decoding.
 *
 * This routine starts the QEP decode operation when configured for QEP
 * decode mode.
 *
 * Different events of the QEP dcode mode are notified through the user
 * provide callback. The events notified are  counter reset events,
 * watchdog timeouts direction change events.
 * This function should not be called in interrupt context.
 *
 * @param dev QEP device structure.
 * @param cb pointer to user callback of type qep_callback_t
 * @param cb_param  user parameter passed as argument in callback.
 *
 * @retval -EPERM if the operation is not permitted in current
 *          configuration.
 * @retval -EBUSY if a decode/capture  operaton on the device is
 *          in progress.
 * @retval 0 if decode was started successfully.
 * @retval -ENTOSUP if this API is not supported.
 */

__syscall int qep_start_decode(const struct device *dev, qep_callback_t cb,
			       void *cb_param);

static inline int z_impl_qep_start_decode(const struct device *dev,
					  qep_callback_t cb,
					  void *cb_param)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->start_decode) {
		return api->start_decode(dev, cb, cb_param);
	}
	return -ENOTSUP;
}

/**
 * @brief Stop QEP decoding.
 *
 * This routine stop an ongoing QEP decode operation.
 *
 * @param dev QEP device structure.
 *
 * @retval -EPERM if the operation is not permitted in current
 *          configuration.
 * @retval 0 if qep decoding was stopped successfully.
 * @retval -ENTOSUP if this API is not supported.
 */
__syscall int qep_stop_decode(const struct device *dev);

static inline int z_impl_qep_stop_decode(const struct device *dev)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->stop_decode) {
		return api->stop_decode(dev);
	}
	return -ENOTSUP;
}

/**
 * @brief Get direction of rotation.
 *
 * This routine provides direction of rotation in QEP mode based on the
 * last position counter reset event. The direction of rotation is
 * clockwise, counter clockwise or unknown if no counter reset event has
 * occurred. This function should not be called in interrupt context.
 *
 * @param dev QEP device structure.
 * @param p_direction pointer to uint32_t populated with direction by
 *        the call.
 * @retval -EPERM if the operation is not permitted in current
 *          configuration.
 * @retval 0 if direction was read successfully.
 * @retval -ENTOSUP if this API is not supported.
 */

__syscall int qep_get_direction(const struct device *dev,
				uint32_t *p_direction);

static inline int z_impl_qep_get_direction(const struct device *dev,
					   uint32_t *p_direction)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->get_direction) {
		return api->get_direction(dev, p_direction);
	}
	return -ENOTSUP;

}

/**
 * @brief Get position count.
 *
 * This routine provides current position counter value. The position count
 * is updated using the pulses per revolution field provided during the
 * device configuration.
 * This function should not be called in interrupt context.
 * The counter value counts down from value =(4* pulses per revoution)
 * when moving in counter clockwise direction.
 * After crossing zero, the position counter is reloaded with
 * 4*pulses_per_rev, when counter reset based on max count is selected.
 * Otherwise, counter reload happens when index event is detected.
 *
 * When moving in clockwise direction, the position counter counts up
 * from 0 to 4*pulses_per_rev . After crossing the maximum value of
 * 4*pulses_per_rev, the counter is reset to zero, when position counter
 * reset on max count is selected. Otherwise counter reset happens when
 * index event is detected.
 *
 * @param dev QEP device structure.
 * @param p_current_count pointer to uint32_t populated with current
 *        position
 * count by the call.
 *
 * @retval -EPERM if the operation is not permitted in current
 *          configuration.
 * @retval 0 if qep position count was read  successfully.
 * @retval -ENTOSUP if this API is not supported.
 */
__syscall int qep_get_position_count(const struct device *dev,
				     uint32_t *p_current_count);

static inline int z_impl_qep_get_position_count(const struct device *dev,
						uint32_t *p_current_count)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->get_position_count) {
		return api->get_position_count(dev, p_current_count);
	}
	return -ENOTSUP;

}

/**
 * @brief Start edge capture.
 *
 * This function starts edge capture when the QEP device is configured for
 * edge capture for the specified counts. The timestamps for edge capture
 * are in nanoseconds.
 * Different events of the edge capture  mode are notified through the user
 * provided callback. The events notified are capture completion and
 * capture cancelled events.
 *
 * This function is asynchronous. Callback is called after completion
 * of the capture. Also ,this function should not be called in interrupt
 * context.
 *
 * @param dev QEP device structure.
 * @param buffer pointer to uint64_t to hold timestamps for edges.
 * @param count number of edges to capture.
 * @param cb pointer to user callback of type qep_callback_t
 * @param cb_param  user parameter passed as argument in callback.
 *
 * @retval -EPERM if the operation is not permitted in current
 *          configuration.
 * @retval -EBUSY if a decode/capture  operaton on the device is in
 *          progress.
 * @retval 0 if edge capture  was started successfully.
 * @retval -ENTOSUP if this API is not supported.
 */

__syscall int qep_start_capture(const struct device *dev, uint64_t *buffer,
				uint32_t count, qep_callback_t cb,
				void *cb_param);

static inline int z_impl_qep_start_capture(const struct device *dev,
					   uint64_t *buffer,
					   uint32_t count,
					   qep_callback_t cb,
					   void *cb_param)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->start_capture) {
		return api->start_capture(dev, buffer, count, cb, cb_param);
	}
	return -ENOTSUP;
}

/**
 * @brief Stop edge capture.
 *
 * This function stops an ongoing  edge capture. On calling this function,
 * user provided callback is called with event as
 * QEP_EVENT_EDGE_CAP_CANCELLED.
 *
 * @param dev QEP device structure.
 *
 * @retval -EPERM if the operation is not permitted in current
 *          configuration.
 * @retval 0 if edge capture was stopped successfully.
 * @retval -ENTOSUP if this API is not supported.
 */

__syscall int qep_stop_capture(const struct device *dev);

static inline int z_impl_qep_stop_capture(const struct device *dev)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->stop_capture) {
		return api->stop_capture(dev);
	}
	return -ENOTSUP;
}

/**
 * @brief Enable QEP event.
 *
 * This function enables specified event in QEP decode mode.
 * This function should not be called in interrupt context.
 *
 * @param dev QEP device structure.
 * @param of type qep_event_t
 *
 * @retval -EPERM if the operation is not permitted in current
 *          configuration.
 * @retval 0 if decode was started successfully.
 * @retval -ENTOSUP if this API is not supported.
 */

__syscall int qep_enable_event(const struct device *dev,
			       qep_event_t event);

static inline int z_impl_qep_enable_event(const struct device *dev,
					  qep_event_t event)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->enable_event) {
		return api->enable_event(dev, event);
	}
	return -ENOTSUP;
}

/**
 * @brief Disable QEP event.
 *
 * This function disables specified event in QEP decode mode.
 * This function should not be called in interrupt context.
 *
 * @param dev QEP device structure.
 * @param of type qep_event_t
 *
 * @retval -EPERM if the operation is not permitted in current
 *          configuration.
 * @retval 0 if decode was started successfully.
 * @retval -ENTOSUP if this API is not supported.
 */

__syscall int qep_disable_event(const struct device *dev,
				qep_event_t event);

static inline int z_impl_qep_disable_event(const struct device *dev,
					   qep_event_t event)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->disable_event) {
		return api->disable_event(dev, event);
	}
	return -ENOTSUP;
}

/**
 * @brief Get phase error status.
 *
 * This function qeueries the QEP controller for to determine if a phase
 * errors exists between inputs Phase A and Phase B.
 * This function should not be called in interrupt context.
 *
 *
 * @param dev QEP device structure.
 * @param p_phase_err ponter to uint32_t that would hold the phase error
 * status.
 *
 * @retval -EPERM if the operation is not permitted in current
 * configuration.
 * @retval 0 if decode was started successfully.
 * @retval -ENTOSUP if this API is not supported.
 */

__syscall int qep_get_phase_err_status(const struct device *dev,
				       uint32_t *p_phase_err);

static inline int z_impl_qep_get_phase_err_status(const struct device *dev,
						  uint32_t *p_phase_err)
{
	const struct qep_driver_api *api =
		(const struct qep_driver_api *)dev->api;

	if (api->get_phase_err_status) {
		return api->get_phase_err_status(dev, p_phase_err);
	}
	return -ENOTSUP;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#include <syscalls/qep.h>

#endif /* __DRIVERS_QEP_H */
