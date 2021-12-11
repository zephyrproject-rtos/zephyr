/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_H_

#include <zephyr/types.h>
#include <device.h>
#include <string.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CAN Interface
 * @defgroup can_interface CAN Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name CAN frame definitions
 * @{
 */

/**
 * @brief Bit mask for a standard (11-bit) CAN identifier.
 */
#define CAN_STD_ID_MASK 0x7FFU
/**
 * @brief Maximum value for a standard (11-bit) CAN identifier.
 */
#define CAN_MAX_STD_ID  CAN_STD_ID_MASK
/**
 * @brief Bit mask for an extended (29-bit) CAN identifier.
 */
#define CAN_EXT_ID_MASK 0x1FFFFFFFU
/**
 * @brief Maximum value for an extended (29-bit) CAN identifier.
 */
#define CAN_MAX_EXT_ID  CAN_EXT_ID_MASK
/**
 * @brief Maximum data length code for CAN 2.0A/2.0B.
 */
#define CAN_MAX_DLC     8U
/**
 * @brief Maximum data length code for CAN-FD.
 */
#define CANFD_MAX_DLC   CONFIG_CANFD_MAX_DLC

/**
 * @cond INTERNAL_HIDDEN
 * Internally calculated maximum data length
 */
#ifndef CONFIG_CANFD_MAX_DLC
#define CAN_MAX_DLEN    8U
#else
#if CONFIG_CANFD_MAX_DLC <= 8
#define CAN_MAX_DLEN    CONFIG_CANFD_MAX_DLC
#elif CONFIG_CANFD_MAX_DLC <= 12
#define CAN_MAX_DLEN    (CONFIG_CANFD_MAX_DLC + (CONFIG_CANFD_MAX_DLC - 8U) * 4U)
#elif CONFIG_CANFD_MAX_DLC == 13
#define CAN_MAX_DLEN    32U
#elif CONFIG_CANFD_MAX_DLC == 14
#define CAN_MAX_DLEN    48U
#elif CONFIG_CANFD_MAX_DLC == 15
#define CAN_MAX_DLEN    64U
#endif
#endif /* CONFIG_CANFD_MAX_DLC */

/**
 * Allow including drivers/can.h even if CONFIG_CAN is not selected.
 */
#ifndef CONFIG_CAN_WORKQ_FRAMES_BUF_CNT
#define CONFIG_CAN_WORKQ_FRAMES_BUF_CNT 4
#endif
/** @endcond */

/** @} */

/**
 * @brief Defines the mode of the CAN controller
 */
enum can_mode {
	/** Normal mode. */
	CAN_NORMAL_MODE,
	/** Controller is not allowed to send dominant bits. */
	CAN_SILENT_MODE,
	/** Controller is in loopback mode (receives own frames). */
	CAN_LOOPBACK_MODE,
	/** Combination of loopback and silent modes. */
	CAN_SILENT_LOOPBACK_MODE
};

/**
 * @brief Defines the state of the CAN bus
 */
enum can_state {
	/** Error-active state. */
	CAN_ERROR_ACTIVE,
	/** Error-passive state. */
	CAN_ERROR_PASSIVE,
	/** Bus-off state. */
	CAN_BUS_OFF,
	/** Bus state unknown. */
	CAN_BUS_UNKNOWN
};

/**
 * @brief Defines if the CAN frame has a standard (11-bit) or extended (29-bit)
 * CAN identifier
 */
enum can_ide {
	/** Standard (11-bit) CAN identifier. */
	CAN_STANDARD_IDENTIFIER,
	/** Extended (29-bit) CAN identifier. */
	CAN_EXTENDED_IDENTIFIER
};

/**
 * @brief Defines if the CAN frame is a data frame or a Remote Transmission Request (RTR) frame
 */
enum can_rtr {
	/** Data frame. */
	CAN_DATAFRAME,
	/** Remote Transmission Request (RTR) frame. */
	CAN_REMOTEREQUEST
};

/**
 * @brief CAN frame structure
 */
struct zcan_frame {
	/** Standard (11-bit) or extended (29-bit) CAN identifier. */
	uint32_t id      : 29;
	/** Frame is in the CAN-FD frame format if set to true. */
	uint32_t fd      : 1;
	/** Remote Transmission Request (RTR) flag. Use @a can_rtr enum for assignment. */
	uint32_t rtr     : 1;
	/** CAN identifier type (standard or extended). Use @a can_ide enum for assignment. */
	uint32_t id_type : 1;
	/** Data Length Code (DLC) indicating data length in bytes. */
	uint8_t dlc;
	/** Baud Rate Switch (BRS). Only valid for CAN-FD. */
	uint8_t brs : 1;
	/** @cond INTERNAL_HIDDEN */
	uint8_t res : 7; /* reserved/padding. */
	/** @endcond */
#if defined(CONFIG_CAN_RX_TIMESTAMP) || defined(__DOXYGEN__)
	/** Captured value of the free-running timer in the CAN controller when
	 * this frame was received. The timer is incremented every bit time and
	 * captured at the start of frame bit (SOF).
	 *
	 * @note @kconfig{CONFIG_CAN_RX_TIMESTAMP} must be selected for this
	 * field to be available.
	 */
	uint16_t timestamp;
#else
	/** @cond INTERNAL_HIDDEN */
	uint8_t res0;  /* reserved/padding. */
	uint8_t res1;  /* reserved/padding. */
	/** @endcond */
#endif
	/** The frame payload data. */
	union {
		uint8_t data[CAN_MAX_DLEN];
		uint32_t data_32[ceiling_fraction(CAN_MAX_DLEN, sizeof(uint32_t))];
	};
};

/**
 * @brief CAN filter structure
 */
struct zcan_filter {
	/** CAN identifier to match. */
	uint32_t id           : 29;
	/** @cond INTERNAL_HIDDEN */
	uint32_t res0         : 1;
	/** @endcond */
	/** Match data frame or Remote Transmission Request (RTR) frame. */
	uint32_t rtr          : 1;
	/** Standard or extended CAN identifier. Use @a can_ide enum for assignment. */
	uint32_t id_type      : 1;
	/** CAN identifier matching mask. If a bit in this mask is 0, the value
	 * of the corresponding bit in the ``id`` field is ignored by the filter.
	 */
	uint32_t id_mask      : 29;
	/** @cond INTERNAL_HIDDEN */
	uint32_t res1         : 1;
	/** @endcond */
	/** Data frame/Remote Transmission Request (RTR) bit matching mask. If
	 * this bit is 0, the value of the ``rtr`` field is ignored by the
	 * filter.
	 */
	uint32_t rtr_mask     : 1;
	/** @cond INTERNAL_HIDDEN */
	uint32_t res2         : 1;
	/** @endcond */
};

/**
 * @brief CAN controller error counters
 */
struct can_bus_err_cnt {
	/** Value of the CAN controller transmit error counter. */
	uint8_t tx_err_cnt;
	/** Value of the CAN controller receive error counter. */
	uint8_t rx_err_cnt;
};

/**
 * @brief CAN bus timing structure
 *
 * This struct is used to pass bus timing values to the configuration and
 * bitrate calculation functions.
 *
 * The propagation segment represents the time of the signal propagation. Phase
 * segment 1 and phase segment 2 define the sampling point. The ``prop_seg`` and
 * ``phase_seg1`` values affect the sampling point in the same way and some
 * controllers only have a register for the sum of those two. The sync segment
 * always has a length of 1 time quantum (see below).
 *
 * @code{.unparsed}
 *
 * +---------+----------+------------+------------+
 * |sync_seg | prop_seg | phase_seg1 | phase_seg2 |
 * +---------+----------+------------+------------+
 *                                   ^
 *                             Sampling-Point
 *
 * @endcode
 *
 * 1 time quantum (tq) has the length of 1/(core_clock / prescaler). The bitrate
 * is defined by the core clock divided by the prescaler and the sum of the
 * segments:
 *
 *   br = (core_clock / prescaler) / (1 + prop_seg + phase_seg1 + phase_seg2)
 *
 * The Synchronization Jump Width (SJW) defines the amount of time quanta the
 * sample point can be moved. The sample point is moved when resynchronization
 * is needed.
 */
struct can_timing {
	/** Synchronisation jump width. */
	uint16_t sjw;
	/** Propagation segment. */
	uint16_t prop_seg;
	/** Phase segment 1. */
	uint16_t phase_seg1;
	/** Phase segment 2. */
	uint16_t phase_seg2;
	/** Prescaler value. */
	uint16_t prescaler;
};

/**
 * @typedef can_tx_callback_t
 * @brief Defines the application callback handler function signature
 *
 * @param error_flags Status of the performed send operation. See the list of
 *                    return values for @a can_send() for value descriptions.
 * @param user_data   User data provided when the frame was sent.
 */
typedef void (*can_tx_callback_t)(uint32_t error_flags, void *user_data);

/**
 * @typedef can_rx_callback_t
 * @brief Defines the application callback handler function signature for receiving.
 *
 * @param frame     Received frame.
 * @param user_data User data provided when the filter was attached.
 */
typedef void (*can_rx_callback_t)(struct zcan_frame *frame, void *user_data);

/**
 * @typedef can_state_change_isr_t
 * @brief Defines the state change Interrupt Service Routine (ISR) handler function signature
 *
 * @param state   State of the CAN controller.
 * @param err_cnt CAN controller error counter values.
 */
typedef void (*can_state_change_isr_t)(enum can_state state, struct can_bus_err_cnt err_cnt);

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/**
 * @brief CAN frame buffer structure
 *
 * Used internally by @a zcan_work struct
 */
struct can_frame_buffer {
	struct zcan_frame buf[CONFIG_CAN_WORKQ_FRAMES_BUF_CNT];
	uint16_t head;
	uint16_t tail;
};

/**
 * @brief CAN work structure
 *
 * Used to attach a work queue to a filter.
 */
struct zcan_work {
	struct k_work work_item;
	struct k_work_q *work_queue;
	struct can_frame_buffer buf;
	can_rx_callback_t cb;
	void *cb_arg;
};

/**
 * @typedef can_set_timing_t
 * @brief Callback API upon setting CAN bus timing
 * See @a can_set_timing() for argument description
 */
typedef int (*can_set_timing_t)(const struct device *dev,
				const struct can_timing *timing,
				const struct can_timing *timing_data);

/**
 * @typedef can_set_mode_t
 * @brief Callback API upon setting CAN controller mode
 * See @a can_set_mode() for argument description
 */
typedef int (*can_set_mode_t)(const struct device *dev, enum can_mode mode);

/**
 * @typedef can_send_t
 * @brief Callback API upon sending a CAN frame
 * See @a can_send() for argument description
 */
typedef int (*can_send_t)(const struct device *dev,
			  const struct zcan_frame *frame,
			  k_timeout_t timeout, can_tx_callback_t callback_isr,
			  void *user_data);

/**
 * @typedef can_attach_isr_t
 * @brief Callback API upon attaching an RX filter ISR
 * See @a can_attach_isr() for argument description
 */
typedef int (*can_attach_isr_t)(const struct device *dev,
				can_rx_callback_t isr,
				void *user_data,
				const struct zcan_filter *filter);

/**
 * @typedef can_detach_t
 * @brief Callback API upon detaching an RX filter
 * See @a can_detach() for argument description
 */
typedef void (*can_detach_t)(const struct device *dev, int filter_id);

/**
 * @typedef can_recover_t
 * @brief Callback API upon recovering the CAN bus
 * See @a can_recover() for argument description
 */
typedef int (*can_recover_t)(const struct device *dev, k_timeout_t timeout);

/**
 * @typedef can_get_state_t
 * @brief Callback API upon get the CAN controller state
 * See @a can_get_state() for argument description
 */
typedef enum can_state (*can_get_state_t)(const struct device *dev,
					  struct can_bus_err_cnt *err_cnt);

/**
 * @typedef can_register_state_change_isr_t
 * @brief Callback API upon registering a state change ISR
 * See @a can_register_state_change_isr() for argument description
 */
typedef void(*can_register_state_change_isr_t)(const struct device *dev,
					       can_state_change_isr_t isr);

/**
 * @typedef can_get_core_clock_t
 * @brief Callback API upon getting the CAN core clock rate
 * See @a can_get_core_clock() for argument description
 */
typedef int (*can_get_core_clock_t)(const struct device *dev, uint32_t *rate);

/**
 * @typedef can_get_max_filters_t
 * @brief Callback API upon getting the maximum number of concurrent CAN RX filters
 * See @a can_get_max_filters() for argument description
 */
typedef int (*can_get_max_filters_t)(const struct device *dev, enum can_ide id_type);

__subsystem struct can_driver_api {
	can_set_mode_t set_mode;
	can_set_timing_t set_timing;
	can_send_t send;
	can_attach_isr_t attach_isr;
	can_detach_t detach;
#if !defined(CONFIG_CAN_AUTO_BUS_OFF_RECOVERY) || defined(__DOXYGEN__)
	can_recover_t recover;
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	can_get_state_t get_state;
	can_register_state_change_isr_t register_state_change_isr;
	can_get_core_clock_t get_core_clock;
	can_get_max_filters_t get_max_filters;
	/* Min values for the timing registers */
	struct can_timing timing_min;
	/* Max values for the timing registers */
	struct can_timing timing_max;
#if defined(CONFIG_CAN_FD_MODE) || defined(__DOXYGEN__)
	/* Min values for the timing registers during the data phase */
	struct can_timing timing_min_data;
	/* Max values for the timing registers during the data phase */
	struct can_timing timing_max_data;
#endif /* CONFIG_CAN_FD_MODE */
};

/** @endcond */

/**
 * @name CAN controller configuration
 *
 * @{
 */

/**
 * @brief Get the CAN core clock rate
 *
 * Returns the CAN core clock rate. One time quantum is 1/(core clock rate).
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param[out] rate CAN core clock rate in Hz.
 *
 * @return 0 on success, or a negative error code on error
 */
__syscall int can_get_core_clock(const struct device *dev, uint32_t *rate);

static inline int z_impl_can_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->get_core_clock(dev, rate);
}

/**
 * @brief Calculate timing parameters from bitrate and sample point
 *
 * Calculate the timing parameters from a given bitrate in bits/s and the
 * sampling point in permill (1/1000) of the entire bit time. The bitrate must
 * alway match perfectly. If no result can be reached for the given parameters,
 * -EINVAL is returned.
 *
 * @note The requested ``sample_pnt`` will not always be matched perfectly. The
 * algorithm calculates the best possible match.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param[out] res   Result is written into the @a can_timing struct provided.
 * @param bitrate    Target bitrate in bits/s.
 * @param sample_pnt Sampling point in permill of the entire bit time.
 *
 * @retval 0 or positive sample point error on success.
 * @retval -EINVAL if there is no solution for the desired values.
 * @retval -EIO if @a can_get_core_clock() is not available.
 */
int can_calc_timing(const struct device *dev, struct can_timing *res,
		    uint32_t bitrate, uint16_t sample_pnt);

#if defined(CONFIG_CAN_FD_MODE) || defined(__DOXYGEN__)
/**
 * @brief Calculate timing parameters for the data phase
 *
 * Same as @a can_calc_timing() but with the maximum and minimum values from the
 * data phase.
 *
 * @note @kconfig{CONFIG_CAN_FD_MODE} must be selected for this function to be
 * available.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param[out] res   Result is written into the @a can_timing struct provided.
 * @param bitrate    Target bitrate for the data phase in bits/s
 * @param sample_pnt Sampling point for the data phase in permille of the entire bit time.
 *
 * @retval 0 or positive sample point error on success.
 * @retval -EINVAL if there is no solution for the desired values.
 * @retval -EIO if @a can_get_core_clock() is not available.
 */
int can_calc_timing_data(const struct device *dev, struct can_timing *res,
			 uint32_t bitrate, uint16_t sample_pnt);
#endif /* CONFIG_CAN_FD_MODE */

/**
 * @brief Fill in the prescaler value for a given bitrate and timing
 *
 * Fill the prescaler value in the timing struct. The sjw, prop_seg, phase_seg1
 * and phase_seg2 must be given.
 *
 * The returned bitrate error is reminder of the devision of the clock rate by
 * the bitrate times the timing segments.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param timing  Result is written into the can_timing struct provided.
 * @param bitrate Target bitrate.
 *
 * @retval 0 or positive bitrate error.
 * @retval Negative error code on error.
 */
int can_calc_prescaler(const struct device *dev, struct can_timing *timing,
		       uint32_t bitrate);

/** Synchronization Jump Width (SJW) value to indicate that the SJW should not
 * be changed by the timing calculation.
 */
#define CAN_SJW_NO_CHANGE 0

/**
 * @brief Configure the bus timing of a CAN controller.
 *
 * If the sjw equals CAN_SJW_NO_CHANGE, the sjw parameter is not changed.
 *
 * @note The parameter ``timing_data`` is only relevant for CAN-FD. If the
 * controller does not support CAN-FD or if @kconfig{CONFIG_CAN_FD_MODE} is not
 * selected, the value of this parameter is ignored.
 *
 * @param dev         Pointer to the device structure for the driver instance.
 * @param timing      Bus timings.
 * @param timing_data Bus timings for data phase (CAN-FD only).
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error, failed to configure device.
 */
__syscall int can_set_timing(const struct device *dev,
			     const struct can_timing *timing,
			     const struct can_timing *timing_data);

static inline int z_impl_can_set_timing(const struct device *dev,
					const struct can_timing *timing,
					const struct can_timing *timing_data)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->set_timing(dev, timing, timing_data);
}

/**
 * @brief Set the CAN controller to the given operation mode
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param mode Operation mode.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error, failed to configure device.
 */
__syscall int can_set_mode(const struct device *dev, enum can_mode mode);

static inline int z_impl_can_set_mode(const struct device *dev, enum can_mode mode)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->set_mode(dev, mode);
}

/**
 * @brief Set the bitrate of the CAN controller
 *
 * The sample point is set to the CiA DS 301 recommended value of 87.5%.
 *
 * @note The parameter ``bitrate_data`` is only relevant for CAN-FD. If the
 * controller does not support CAN-FD or if @kconfig{CONFIG_CAN_FD_MODE} is not
 * selected, the value of this parameter is ignored.

 * @param dev          Pointer to the device structure for the driver instance.
 * @param bitrate      Desired arbitration phase bitrate.
 * @param bitrate_data Desired data phase bitrate.
 *
 * @retval 0 If successful.
 * @retval -EINVAL bitrate cannot be met.
 * @retval -EIO General input/output error, failed to set bitrate.
 */
static inline int can_set_bitrate(const struct device *dev,
				  uint32_t bitrate,
				  uint32_t bitrate_data)
{
	struct can_timing timing;
#ifdef CONFIG_CAN_FD_MODE
	struct can_timing timing_data;
#endif
	int ret;

	ret = can_calc_timing(dev, &timing, bitrate, 875);
	if (ret < 0) {
		return -EINVAL;
	}

	timing.sjw = CAN_SJW_NO_CHANGE;

#ifdef CONFIG_CAN_FD_MODE
	ret = can_calc_timing_data(dev, &timing_data, bitrate_data, 875);
	if (ret < 0) {
		return -EINVAL;
	}

	timing_data.sjw = CAN_SJW_NO_CHANGE;

	return can_set_timing(dev, &timing, &timing_data);
#else /* CONFIG_CAN_FD_MODE */
	return can_set_timing(dev, &timing, NULL);
#endif /* !CONFIG_CAN_FD_MODE */
}

/** @} */

/**
 * @name Transmitting CAN frames
 *
 * @{
 */

/**
 * @brief Transmit a CAN frame on the CAN bus
 *
 * Transmit a CAN fram on the CAN bus with optional timeout and completion
 * callback function.
 *
 * @see can_write() for a simplified API wrapper.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param frame     CAN frame to transmit.
 * @param timeout   Timeout waiting for a empty TX mailbox or ``K_FOREVER``.
 * @param callback  Optional callback for when the frame was sent or a
 *                  transmission error occurred. If ``NULL``, this function is
 *                  blocking until frame is sent. The callback must be ``NULL``
 *                  if called from user mode.
 * @param user_data User data to pass to callback function.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid parameter was passed to the function.
 * @retval -ENETDOWN if the CAN controller is in bus-off state.
 * @retval -EBUSY if CAN bus arbitration was lost.
 * @retval -EIO if a general transmit error occurred.
 * @retval -EAGAIN on timeout.
 */
__syscall int can_send(const struct device *dev, const struct zcan_frame *frame,
		       k_timeout_t timeout, can_tx_callback_t callback,
		       void *user_data);

static inline int z_impl_can_send(const struct device *dev, const struct zcan_frame *frame,
				  k_timeout_t timeout, can_tx_callback_t callback,
				  void *user_data)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->send(dev, frame, timeout, callback, user_data);
}

/**
 * @brief Wrapper function for writing data to the CAN bus.
 *
 * Simple wrapper function for @a can_send() without the need for filling in a
 * @a zcan_frame struct. This function blocks until the data is sent or a
 * timeout occurs.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param data    Pointer to the data to write.
 * @param length  Number of bytes to write (max. 8).
 * @param id      CAN identifier used for writing.
 * @param rtr     Write as data frame or Remote Transmission Request (RTR) frame.
 * @param timeout Timeout waiting for an empty TX mailbox or ``K_FOREVER``.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if an invalid parameter was passed to the function.
 * @retval -ENETDOWN if the CAN controller is in bus-off state.
 * @retval -EBUSY if CAN bus arbitration was lost.
 * @retval -EIO if a general transmit error occurred.
 * @retval -EAGAIN on timeout.
 */
static inline int can_write(const struct device *dev, const uint8_t *data, uint8_t length,
			    uint32_t id, enum can_rtr rtr, k_timeout_t timeout)
{
	struct zcan_frame frame;

	if (length > 8) {
		return -EINVAL;
	}

	frame.id = id;

	if (id > CAN_MAX_STD_ID) {
		frame.id_type = CAN_EXTENDED_IDENTIFIER;
	} else {
		frame.id_type = CAN_STANDARD_IDENTIFIER;
	}

	frame.dlc = length;
	frame.rtr = rtr;
	memcpy(frame.data, data, length);

	return can_send(dev, &frame, timeout, NULL, NULL);
}

/** @} */

/**
 * @name Receiving CAN frames
 *
 * @{
 */

/**
 * @brief Attach a callback function with a given CAN filter
 *
 * Attach a callback to CAN identifiers specified by a filter. Whenever a frame
 * matching the filter is received by the CAN controller, the callback function
 * is called in interrupt context.
 *
 * If a frame matches more than one attached filter, the priority of the match
 * is hardware dependent.
 *
 * The same callback function can be attached to more than one filter.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param callback  This function is called by the CAN controller driver whenever
 *                  a frame matching the filter is received.
 * @param user_data User data to pass to callback function.
 * @param filter    Pointer to a @a zcan_filter structure defining the filter.
 *
 * @retval filter_id on success.
 * @retval -ENOSPC if there are no free filters.
 */
static inline int can_attach_isr(const struct device *dev, can_rx_callback_t callback,
				 void *user_data, const struct zcan_filter *filter)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->attach_isr(dev, callback, user_data, filter);
}

/**
 * @brief Attach a CAN work queue with a given CAN filter
 *
 * Attach a work queue to CAN identifiers specified by a filter. Whenever a
 * frame matching the filter is received by the CAN controller, the frame is
 * pushed to the buffer of the @a zcan_work structure and the work element is
 * put in the workqueue.
 *
 * If a frame matches more than one attached filter, the priority of the match
 * is hardware dependent.
 *
 * The same CAN work queue can be attached to more than one filter.
 *
 * @note The work queue must be initialized before and the caller must have
 * appropriate permissions on it.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param work_q    Pointer to the already initialized @a zcan_work queue.
 * @param work      Pointer to a @a zcan_work structure, which will be initialized.
 * @param callback  This function is called by the work queue whenever a frame
 *                  matching the filter is received.
 * @param user_data User data to pass to callback function.
 * @param filter    Pointer to a @a zcan_filter structure defining the filter.
 *
 * @retval filter_id on success.
 * @retval -ENOSPC if there are no free filters.
 */
int can_attach_workq(const struct device *dev, struct k_work_q  *work_q,
		     struct zcan_work *work, can_rx_callback_t callback, void *user_data,
		     const struct zcan_filter *filter);

/**
 * @brief Statically define and initialize a CAN RX message queue.
 *
 * The message queue's ring buffer contains space for @a size CAN frames.
 *
 * @param name Name of the message queue.
 * @param size Number of CAN frames.
 */
#define CAN_DEFINE_MSGQ(name, size) \
	K_MSGQ_DEFINE(name, sizeof(struct zcan_frame), size, 4)

/**
 * @brief Attach a message queue with a given filter
 *
 * Attach a message queue to CAN identifiers specified by a filter. Whenever a
 * frame matching the filter is received by the CAN controller, the frame is
 * pushed to the message queue.
 *
 * If a frame matches more than one attached filter, the priority of the match
 * is hardware dependent.
 *
 * A message queue can be attached to more than one filter.
 *
 * @note The message queue must me initialized before, and the caller must have
 * appropriate permissions on it.
 *
 * @param dev    Pointer to the device structure for the driver instance.
 * @param msg_q  Pointer to the already initialized @a k_msgq struct.
 * @param filter Pointer to a @a zcan_filter structure defining the filter.
 *
 * @retval filter_id on success.
 * @retval -ENOSPC if there are no free filters.
 */
__syscall int can_attach_msgq(const struct device *dev, struct k_msgq *msg_q,
			      const struct zcan_filter *filter);

/**
 * @brief Detach an ISR, CAN work queue, or CAN message queue RX filter
 *
 * This routine detaches an CAN RX filter based on the filter ID returned by @a
 * can_attach_isr(), @a can_attach_workq(), or @a can_attach_msgq().
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param filter_id Filter ID
 */
__syscall void can_detach(const struct device *dev, int filter_id);

static inline void z_impl_can_detach(const struct device *dev, int filter_id)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->detach(dev, filter_id);
}

/**
 * @brief Get maximum number of RX filters
 *
 * Get the maximum number of concurrent RX filters for the CAN controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id_type CAN identifier type (standard or extended).
 *
 * @retval Positive number of maximum concurrent filters.
 * @retval -EIO General input/output error.
 * @retval -ENOSYS If this function is not implemented by the driver.
 */
__syscall int can_get_max_filters(const struct device *dev, enum can_ide id_type);

static inline int z_impl_can_get_max_filters(const struct device *dev, enum can_ide id_type)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	if (api->get_max_filters == NULL) {
		return -ENOSYS;
	}

	return api->get_max_filters(dev, id_type);
}

/** @} */

/**
 * @name CAN bus error reporting and handling
 *
 * @{
 */

/**
 * @brief Get current CAN controller state
 *
 * Returns the current state and optionally the error counter values of the CAN
 * controller.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param[out] err_cnt Pointer to the err_cnt destination structure or NULL.
 *
 * @retval  state
 */
__syscall enum can_state can_get_state(const struct device *dev,
				       struct can_bus_err_cnt *err_cnt);

static inline enum can_state z_impl_can_get_state(const struct device *dev,
						  struct can_bus_err_cnt *err_cnt)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->get_state(dev, err_cnt);
}

/**
 * @brief Recover from bus-off state
 *
 * Recover the CAN controller from bus-off state to error-active state.
 *
 * @note @kconfig{CONFIG_CAN_AUTO_BUS_OFF_RECOVERY} must be deselected for this
 * function to be available.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param timeout Timeout for waiting for the recovery or ``K_FOREVER``.
 *
 * @retval 0 on success.
 * @retval -EAGAIN on timeout.
 */
#if !defined(CONFIG_CAN_AUTO_BUS_OFF_RECOVERY) || defined(__DOXYGEN__)
__syscall int can_recover(const struct device *dev, k_timeout_t timeout);

static inline int z_impl_can_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->recover(dev, timeout);
}
#else /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
/* This implementation prevents inking errors for auto recovery */
static inline int z_impl_can_recover(const struct device *dev, k_timeout_t timeout)
{
	return 0;
}
#endif /* !CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

/**
 * @brief Register an ISR callback for the CAN controller state change interrupt
 *
 * Only one callback can be registered per controller. Calling this function
 * again overrides any previously registered callback.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param isr ISR callback function.
 */
static inline void can_register_state_change_isr(const struct device *dev,
						 can_state_change_isr_t isr)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->register_state_change_isr(dev, isr);
}

/** @} */

/**
 * @name CAN utility functions
 *
 * @{
 */

/**
 * @brief Convert from Data Length Code (DLC) to the number of data bytes
 *
 * @param dlc Data Length Code (DLC).
 *
 * @retval Number of bytes.
 */
static inline uint8_t can_dlc_to_bytes(uint8_t dlc)
{
	static const uint8_t dlc_table[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12,
					    16, 20, 24, 32, 48, 64};

	return dlc > 0x0F ? 64 : dlc_table[dlc];
}

/**
 * @brief Convert from number of bytes to Data Length Code (DLC)
 *
 * @param num_bytes Number of bytes.
 *
 * @retval Data Length Code (DLC).
 */
static inline uint8_t can_bytes_to_dlc(uint8_t num_bytes)
{
	return num_bytes <= 8  ? num_bytes :
	       num_bytes <= 12 ? 9 :
	       num_bytes <= 16 ? 10 :
	       num_bytes <= 20 ? 11 :
	       num_bytes <= 24 ? 12 :
	       num_bytes <= 32 ? 13 :
	       num_bytes <= 48 ? 14 :
	       15;
}

/** @} */

/**
 * @name Linux SocketCAN compatibility
 *
 * The following structures and functions provide compatibility with the CAN
 * frame and CAN filter formats used by Linux SocketCAN.
 *
 * @{
 */

/**
 * CAN Identifier structure for Linux SocketCAN compatibility.
 *
 * The fields in this type are:
 *
 * @code{.unparsed}
 *
 * +------+--------------------------------------------------------------+
 * | Bits | Description                                                  |
 * +======+==============================================================+
 * | 0-28 | CAN identifier (11/29 bit)                                   |
 * +------+--------------------------------------------------------------+
 * |  29  | Error message frame flag (0 = data frame, 1 = error message) |
 * +------+--------------------------------------------------------------+
 * |  30  | Remote transmission request flag (1 = RTR frame)             |
 * +------+--------------------------------------------------------------+
 * |  31  | Frame format flag (0 = standard 11 bit, 1 = extended 29 bit) |
 * +------+--------------------------------------------------------------+
 *
 * @endcode
 */
typedef uint32_t canid_t;

/**
 * @brief CAN frame for Linux SocketCAN compatibility.
 */
struct can_frame {
	/** 32-bit CAN ID + EFF/RTR/ERR flags. */
	canid_t can_id;

	/** The data length code (DLC). */
	uint8_t can_dlc;

	/** @cond INTERNAL_HIDDEN */
	uint8_t pad;   /* padding. */
	uint8_t res0;  /* reserved/padding. */
	uint8_t res1;  /* reserved/padding. */
	/** @endcond */

	/** The payload data. */
	uint8_t data[CAN_MAX_DLEN];
};

/**
 * @brief CAN filter for Linux SocketCAN compatibility.
 *
 * A filter is considered a match when `received_can_id & mask == can_id & can_mask`.
 */
struct can_filter {
	/** The CAN identifier to match. */
	canid_t can_id;
	/** The mask applied to @a can_id for matching. */
	canid_t can_mask;
};

/**
 * @brief Translate a @a can_frame struct to a @a zcan_frame struct.
 *
 * @param frame  Pointer to can_frame struct.
 * @param zframe Pointer to zcan_frame struct.
 */
static inline void can_copy_frame_to_zframe(const struct can_frame *frame,
					    struct zcan_frame *zframe)
{
	zframe->id_type = (frame->can_id & BIT(31)) >> 31;
	zframe->rtr = (frame->can_id & BIT(30)) >> 30;
	zframe->id = frame->can_id & BIT_MASK(29);
	zframe->dlc = frame->can_dlc;
	memcpy(zframe->data, frame->data, sizeof(zframe->data));
}

/**
 * @brief Translate a @a zcan_frame struct to a @a can_frame struct.
 *
 * @param zframe Pointer to zcan_frame struct.
 * @param frame  Pointer to can_frame struct.
 */
static inline void can_copy_zframe_to_frame(const struct zcan_frame *zframe,
					    struct can_frame *frame)
{
	frame->can_id = (zframe->id_type << 31) | (zframe->rtr << 30) |	zframe->id;
	frame->can_dlc = zframe->dlc;
	memcpy(frame->data, zframe->data, sizeof(frame->data));
}

/**
 * @brief Translate a @a can_filter struct to a @a zcan_filter struct.
 *
 * @param filter  Pointer to can_filter struct.
 * @param zfilter Pointer to zcan_filter struct.
 */
static inline void can_copy_filter_to_zfilter(const struct can_filter *filter,
					      struct zcan_filter *zfilter)
{
	zfilter->id_type = (filter->can_id & BIT(31)) >> 31;
	zfilter->rtr = (filter->can_id & BIT(30)) >> 30;
	zfilter->id = filter->can_id & BIT_MASK(29);
	zfilter->rtr_mask = (filter->can_mask & BIT(30)) >> 30;
	zfilter->id_mask = filter->can_mask & BIT_MASK(29);
}

/**
 * @brief Translate a @a zcan_filter struct to a @a can_filter struct.
 *
 * @param zfilter Pointer to zcan_filter struct.
 * @param filter  Pointer to can_filter struct.
 */
static inline void can_copy_zfilter_to_filter(const struct zcan_filter *zfilter,
					      struct can_filter *filter)
{
	filter->can_id = (zfilter->id_type << 31) |
		(zfilter->rtr << 30) | zfilter->id;
	filter->can_mask = (zfilter->rtr_mask << 30) |
		(zfilter->id_type << 31) | zfilter->id_mask;
}

/** @} */

/**
 * @cond INTERNAL_HIDDEN
 * Deprecated APIs
 */

/**
 * @name CAN specific error codes
 *
 * The `CAN_TX_*` error codes are used for CAN specific error return codes from
 * @a can_send() and for `error_flags` values in @a can_tx_callback_t().
 *
 * `CAN_NO_FREE_FILTER` is returned by `can_attach_*()` if no free filters are
 * available. `CAN_TIMEOUT` indicates that @a can_recover() timed out.
 *
 * @warning These definitions are deprecated. Use the corresponding errno
 * definitions instead.
 *
 * @{
 */

/** Transmitted successfully. */
#define CAN_TX_OK          (0)          __DEPRECATED_MACRO
/** General transmit error. */
#define CAN_TX_ERR         (-EIO)       __DEPRECATED_MACRO
/** Bus arbitration lost during transmission. */
#define CAN_TX_ARB_LOST    (-EBUSY)     __DEPRECATED_MACRO
/** CAN controller is in bus off state. */
#define CAN_TX_BUS_OFF     (-ENETDOWN)  __DEPRECATED_MACRO
/** Unknown error. */
#define CAN_TX_UNKNOWN     (CAN_TX_ERR) __DEPRECATED_MACRO
/** Invalid parameter. */
#define CAN_TX_EINVAL      (-EINVAL)    __DEPRECATED_MACRO
/** No free filters available. */
#define CAN_NO_FREE_FILTER (-ENOSPC)    __DEPRECATED_MACRO
/** Operation timed out. */
#define CAN_TIMEOUT        (-EAGAIN)    __DEPRECATED_MACRO

/** @} */

/**
 * @brief Configure operation of a host controller.
 *
 * @warning This function is deprecated. Use @a can_set_bitrate() and @a
 * can_set_mode() instead.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode Operation mode.
 * @param bitrate bus-speed in Baud/s.
 *
 * @retval 0 If successful.
 * @retval -EIO General input/output error, failed to configure device.
 */
__deprecated static inline int can_configure(const struct device *dev, enum can_mode mode,
					     uint32_t bitrate)
{
	int err;

	if (bitrate > 0) {
		err = can_set_bitrate(dev, bitrate, bitrate);
		if (err != 0) {
			return err;
		}
	}

	return can_set_mode(dev, mode);
}

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/can.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_H_ */
