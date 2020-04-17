/**
 * @file
 *
 * @brief Public APIs for the CAN drivers.
 */

/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_H_

/**
 * @brief CAN Interface
 * @defgroup can_interface CAN Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <device.h>
#include <string.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_EX_ID      (1 << 31)
#define CAN_MAX_STD_ID (0x7FF)
#define CAN_STD_ID_MASK CAN_MAX_STD_ID
#define CAN_EXT_ID_MASK (0x1FFFFFFF)
#define CAN_MAX_DLC    (8)
#ifdef CONFIG_CAN_DL
#define CAN_MAX_DLEN   CONFIG_CAN_DL
#else
#define CAN_MAX_DLEN   (8)
#endif

/* CAN_TX_* are the error flags from tx_callback and send.*/
/** send successfully */
#define CAN_TX_OK       (0)
/** general send error */
#define CAN_TX_ERR      (-2)
/** bus arbitration lost during sending */
#define CAN_TX_ARB_LOST (-3)
/** controller is in bus off state */
#define CAN_TX_BUS_OFF  (-4)
/** unexpected error */
#define CAN_TX_UNKNOWN  (-5)

/** invalid parameter */
#define CAN_TX_EINVAL   (-22)

/** attach_* failed because there is no unused filter left*/
#define CAN_NO_FREE_FILTER (-1)

/** operation timed out*/
#define CAN_TIMEOUT (-1)

/**
 * @brief Statically define and initialize a can message queue.
 *
 * The message queue's ring buffer contains space for @a size messages.
 *
 * @param name Name of the message queue.
 * @param size Number of can messages.
 */
#define CAN_DEFINE_MSGQ(name, size) \
	K_MSGQ_DEFINE(name, sizeof(struct zcan_frame), size, 4)

/**
 * @brief can_ide enum
 * Define if the message has a standard (11bit) or extended (29bit)
 * identifier
 */
enum can_ide {
	CAN_STANDARD_IDENTIFIER,
	CAN_EXTENDED_IDENTIFIER
};

/**
 * @brief can_rtr enum
 * Define if the message is a data or remote frame
 */
enum can_rtr {
	CAN_DATAFRAME,
	CAN_REMOTEREQUEST
};

/**
 * @brief can_mode enum
 * Defines the mode of the can controller
 */
enum can_mode {
	/*Normal mode*/
	CAN_NORMAL_MODE,
	/*Controller is not allowed to send dominant bits*/
	CAN_SILENT_MODE,
	/*Controller is in loopback mode (receive own messages)*/
	CAN_LOOPBACK_MODE,
	/*Combination of loopback and silent*/
	CAN_SILENT_LOOPBACK_MODE,
	/*Value that can be passed to can_config without changing the mode*/
	CAN_NO_MODE_CHANGE,
};

/**
 * @brief can_state enum
 * Defines the possible states of the CAN bus
 */
enum can_state {
	CAN_ERROR_ACTIVE,
	CAN_ERROR_PASSIVE,
	CAN_BUS_OFF,
	CAN_BUS_UNKNOWN
};

/*
 * Controller Area Network Identifier structure for Linux compatibility.
 *
 * The fields in this type are:
 *
 * bit 0-28	: CAN identifier (11/29 bit)
 * bit 29	: error message frame flag (0 = data frame, 1 = error message)
 * bit 30	: remote transmission request flag (1 = rtr frame)
 * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
 */
typedef u32_t canid_t;

/**
 * @brief CAN frame structure that is compatible with Linux. This is mainly
 * used by Socket CAN code.
 *
 * @details Used to pass CAN messages from userspace to the socket CAN and vice
 * versa.
 */
struct can_frame {
	/** 32 bit CAN_ID + EFF/RTR/ERR flags */
	canid_t can_id;

	/** The length of the message */
	u8_t can_dlc;

	/** @cond INTERNAL_HIDDEN */
	u8_t pad;   /* padding */
	u8_t res0;  /* reserved / padding */
	u8_t res1;  /* reserved / padding */
	/** @endcond */

	/** The message data */
	u8_t data[CAN_MAX_DLEN];
};

/**
 * @brief CAN filter that is compatible with Linux. This is mainly used by
 * Socket CAN code.
 *
 * @details A filter matches, when "received_can_id & mask == can_id & mask"
 */
struct can_filter {
	canid_t can_id;
	canid_t can_mask;
};

/**
 * @brief CAN message structure
 *
 * Used to pass can messages from userspace to the driver and
 * from driver to userspace
 *
 */
struct zcan_frame {
	union {
		struct {
			/** 29-bit Frame identifier.*/
			u32_t ext_id  : 29;
			/** Frame is in the CAN-FD frame format */
			u32_t fd      : 1;
			/** Set the message to a transmission request instead of
			 * data frame. Use can_rtr enum for assignment.
			 */
			u32_t rtr     : 1;
			/** Indicates the identifier type (standard or extended).
			 * Use can_ide enum for assignment.
			 */
			u32_t id_type : 1;
		};
		struct {
			/** 11-bit Frame identifier.*/
			u32_t std_id  : 11;
		};
	};

	/** The length of the message (max. 8) in bytes.*/
	u8_t dlc;
	/** @cond INTERNAL_HIDDEN */
	u8_t pad;   /* padding for alignment.*/
	/** @endcond */
#if defined(CONFIG_CAN_RX_TIMESTAMP)
	/** Timer value of the CAN free running timer.
	 * The timer is incremented every bit time and captured at the start
	 * of frame bit (SOF).
	 */
	u16_t timestamp;
#else
	/** @cond INTERNAL_HIDDEN */
	u8_t res0;  /* reserved / padding */
	u8_t res1;  /* reserved / padding */
	/** @endcond */
#endif
	/** The frame payload data.*/
	union {
		u8_t data[CAN_MAX_DLEN];
		u32_t data_32[ROUND_UP(CAN_MAX_DLEN / sizeof(u32_t),
				       sizeof(u32_t))];
	};
};

/**
 * @brief CAN filter structure
 *
 * Used to pass can identifier filter information to the driver.
 * rtr_mask and *_id_mask are used to mask bits of the rtr and id fields.
 * If the mask bit is 0, the value of the corresponding bit in the id or rtr
 * field don't care for the filter matching.
 *
 */
struct zcan_filter {
	union {
		struct {
			/** target state of the 29-bit identifier */
			u32_t ext_id   : 29;
			/** @cond INTERNAL_HIDDEN */
			/*Currently unused bit */
			u32_t res_id   : 1;
			/** @endcond */
			/** Indicates the identifier type (standard or extended)
			 *  use can_ide enum for assignment
			 */
			u32_t id_type      : 1;
			/** target state of the rtr bit */
			u32_t rtr          : 1;
		};
		struct {
			/** target state of the 11-bit identifier */
			u32_t std_id   : 11;
		};
	};
	union {
		struct {
			/** 29-bit identifier mask*/
			u32_t ext_id_mask  : 29;
			/** @cond INTERNAL_HIDDEN */
			/* Currently unused bits */
			u32_t res_mask     : 2;
			/** @endcond */
			/** rtr bit mask */
			u32_t rtr_mask     : 1;
		};
		struct {
			/** 11-bit identifier mask*/
			u32_t std_id_mask  : 11;
		};
	};
};

/**
 * @brief can bus error count structure
 *
 * Used to pass the bus error counters to userspace
 */
struct can_bus_err_cnt {
	u8_t tx_err_cnt;
	u8_t rx_err_cnt;
};


/**
 * @brief canbus timings
 *
 * Used to pass bus timing values to the config and bitrate calculator function.
 *
 * The propagation segment represents the time of the signal propagation.
 * Phase segment 1 and phase segment 2 define the sampling point.
 * prop_seg and phase_seg1 affect the sampling-point in the same way and some
 * controllers only have a register for the sum of those two. The sync segment
 * always has a length of 1 tq
 * +---------+----------+------------+------------+
 * |sync_seg | prop_seg | phase_seg1 | phase_seg2 |
 * +---------+----------+------------+------------+
 *                                   ^
 *                             Sampling-Point
 * 1 tq (time quantum) has the length of 1/(core_clock / prescaler)
 * The bitrate is defined by the core clock divided by the prescaler and the
 * sum of the segments.
 * br = (core_clock / prescaler) / (1 + prop_seg + phase_seg1 + phase_seg2)
 * The resynchronization jump width (SJW) defines the amount of time quantum
 * the sample point can be moved.
 * The sample point is moved when resynchronization is needed.
 */
struct can_timing {
	/** Synchronisation jup width*/
	u16_t sjw;
	/** Propagation Segment */
	u16_t prop_seg;
	/** Phase Segment 1 */
	u16_t phase_seg1;
	/** Phase Segment 2 */
	u16_t phase_seg2;
	/** Prescaler value*/
	u16_t prescaler;
};

/**
 * @typedef can_tx_callback_t
 * @brief Define the application callback handler function signature
 *
 * @param error_flags status of the performed send operation
 * @param arg argument that was passed when the message was sent
 */
typedef void (*can_tx_callback_t)(u32_t error_flags, void *arg);

/**
 * @typedef can_rx_callback_t
 * @brief Define the application callback handler function signature
 *        for receiving.
 *
 * @param msg received message
 * @param arg argument that was passed when the filter was attached
 */
typedef void (*can_rx_callback_t)(struct zcan_frame *msg, void *arg);

/**
 * @typedef can_state_change_isr_t
 * @brief Defines the state change isr handler function signature
 *
 * @param state state of the node
 * @param err_cnt struct with the error counter values
 */
typedef void(*can_state_change_isr_t)(enum can_state state,
					  struct can_bus_err_cnt err_cnt);

typedef int (*can_configure_t)(struct device *dev, enum can_mode mode,
			       struct can_timing *timing,
			       struct can_timing *timing_data);

#ifdef CONFIG_CAN_FD_MODE
typedef int (*can_send_t)(struct device *dev, const struct zcan_frame *msg,
			  bool brs, s32_t timeout,
			  can_tx_callback_t callback_isr, void *callback_arg);
#else
typedef int (*can_send_t)(struct device *dev, const struct zcan_frame *msg,
			  k_timeout_t timeout, can_tx_callback_t callback_isr,
			  void *callback_arg);
#endif /* CONFIG_CAN_FD_MODE */

typedef int (*can_attach_msgq_t)(struct device *dev, struct k_msgq *msg_q,
				 const struct zcan_filter *filter);

typedef int (*can_attach_isr_t)(struct device *dev, can_rx_callback_t isr,
				void *callback_arg,
				const struct zcan_filter *filter);

typedef void (*can_detach_t)(struct device *dev, int filter_id);

typedef int (*can_recover_t)(struct device *dev, k_timeout_t timeout);

typedef enum can_state (*can_get_state_t)(struct device *dev,
					  struct can_bus_err_cnt *err_cnt);

typedef void(*can_register_state_change_isr_t)(struct device *dev,
					       can_state_change_isr_t isr);

typedef u32_t (*can_get_core_clock_t)(struct device *dev);

#ifndef CONFIG_CAN_WORKQ_FRAMES_BUF_CNT
#define CONFIG_CAN_WORKQ_FRAMES_BUF_CNT 4
#endif
struct can_frame_buffer {
	struct zcan_frame buf[CONFIG_CAN_WORKQ_FRAMES_BUF_CNT];
	u16_t head;
	u16_t tail;
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

__subsystem struct can_driver_api {
	can_configure_t configure;
	can_send_t send;
	can_attach_isr_t attach_isr;
	can_detach_t detach;
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	can_recover_t recover;
#endif
	can_get_state_t get_state;
	can_register_state_change_isr_t register_state_change_isr;
	can_get_core_clock_t get_core_clock;
	/* Min values for the timing registers */
	struct can_timing timing_min;
	/* Max values for the timing registers */
	struct can_timing timing_max;
#ifdef CONFIG_CAN_FD_MODE
	/* Min values for the timing registers during the data phase */
	struct can_timing timing_min_data;
	/* Max values for the timing registers during the data phase */
	struct can_timing timing_max_data;
#endif
};

/**
 * @brief Convert the DLC to the number of bytes
 *
 * This function converts a the Data Length Code to the number of bytes.
 *
 * @param dlc The Data Length Code
 *
 * @retval Number of bytes
 */

static inline u8_t can_dlc_to_bytes(u8_t dlc)
{
	static const u8_t dlc_table[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12,
					 16, 20, 24, 32, 48, 64};

	return dlc > 0x0F ? 64 : dlc_table[dlc];
}

/**
 * @brief Convert a number of bytes to the DLC
 *
 * This function converts a number of bytes to the Data Length Code
 *
 * @param num_bytes The number of bytes
 *
 * @retval The DLC
 */

static inline u8_t can_bytes_to_dlc(u8_t num_bytes)
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

#ifdef CONFIG_CAN_FD_MODE

/**
 * @brief Perform data transfer to CAN bus with flexible datarate option.
 *
 * This routine provides a generic interface to perform data transfer
 * to the can bus with options for CAN-FD
 * *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param frame        Frame to transfer.
 * @param brs          Baudrate Switch. If true, switch to fast br in dataphase.
 * @param timeout      Waiting for empty tx mailbox timeout in ms or K_FOREVER.
 * @param callback_isr Is called when message was sent or a transmission error
 *                     occurred. If NULL, this function is blocking until
 *                     message is sent. This must be NULL if called from user
 *                     mode.
 * @param callback_arg This will be passed whenever the isr is called.
 *
 * @retval 0 If successful.
 * @retval CAN_TX_* on failure.
 */
__syscall int can_fd_send(struct device *dev, const struct zcan_frame *frame,
		       bool brs, s32_t timeout,
		       can_tx_callback_t callback_isr,
		       void *callback_arg);

static inline int z_impl_can_fd_send(struct device *dev,
				     const struct zcan_frame *frame,
				     bool brs, s32_t timeout,
				     can_tx_callback_t callback_isr,
				     void *callback_arg)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->send(dev, frame, timeout, brs, callback_isr, callback_arg);
}
#endif /* CONFIG_CAN_FD_MODE */

/**
 * @brief Perform data transfer to CAN bus.
 *
 * This routine provides a generic interface to perform data transfer
 * to the can bus. Use can_write() for simple write.
 * *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param msg          Message to transfer.
 * @param timeout      Waiting for empty tx mailbox timeout or K_FOREVER.
 * @param callback_isr Is called when message was sent or a transmission error
 *                     occurred. If NULL, this function is blocking until
 *                     message is sent. This must be NULL if called from user
 *                     mode.
 * @param callback_arg This will be passed whenever the isr is called.
 *
 * @retval 0 If successful.
 * @retval CAN_TX_* on failure.
 */
__syscall int can_send(struct device *dev, const struct zcan_frame *frame,
		       k_timeout_t timeout, can_tx_callback_t callback_isr,
		       void *callback_arg);

static inline int z_impl_can_send(struct device *dev,
				 const struct zcan_frame *frame,
				 k_timeout_t timeout, can_tx_callback_t callback_isr,
				 void *callback_arg)
{
#ifdef CONFIG_CAN_FD_MODE
	return can_fd_send(dev, frame, false, false, timeout, callback_isr,
			   callback_arg);
#else
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->send(dev, frame, timeout, callback_isr, callback_arg);
#endif
}

/*
 * Derived can APIs -- all implemented in terms of can_send()
 */

/**
 * @brief Write a set amount of data to the can bus.
 *
 * This routine writes a set amount of data synchronously.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param data Data to send.
 * @param length Number of bytes to write (max. 8).
 * @param id  Identifier of the can message.
 * @param rtr Send remote transmission request or data frame
 * @param timeout Waiting for empty tx mailbox timeout or K_FOREVER
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -EINVAL if length > 8.
 */
static inline int can_write(struct device *dev, const u8_t *data, u8_t length,
			    u32_t id, enum can_rtr rtr, k_timeout_t timeout)
{
	struct zcan_frame msg;

	if (length > 8) {
		return -EINVAL;
	}

	if (id > CAN_MAX_STD_ID) {
		msg.id_type = CAN_EXTENDED_IDENTIFIER;
		msg.ext_id = id & CAN_EXT_ID_MASK;
	} else {
		msg.id_type = CAN_STANDARD_IDENTIFIER;
		msg.std_id = id;
	}

	msg.dlc = length;
	msg.rtr = rtr;
	memcpy(msg.data, data, length);

	return can_send(dev, &msg, timeout, NULL, NULL);
}

/**
 * @brief Attach a CAN work queue to a single or group of identifiers.
 *
 * This routine attaches a work queue to identifiers specified by a filter.
 * Whenever the filter matches, the message is pushed to the buffer
 * of the zcan_work structure and the work element is put to the workqueue.
 * If a message passes more than one filter the priority of the match
 * is hardware dependent.
 * A CAN work queue can be attached to more than one filter.
 * The work queue must be initialized before and the caller must have
 * appropriate permissions on it.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param work_q       Pointer to the already initialized work queue.
 * @param work         Pointer to a zcan_work. The work will be initialized.
 * @param callback     This function is called by workq whenever a message arrives.
 * @param callback_arg Is passed to the callback when called.
 * @param filter       Pointer to a zcan_filter structure defining the id
 *                     filtering.
 *
 * @retval filter_id on success.
 * @retval CAN_NO_FREE_FILTER if there is no filter left.
 */
int can_attach_workq(struct device *dev, struct k_work_q  *work_q,
		     struct zcan_work *work,
		     can_rx_callback_t callback, void *callback_arg,
		     const struct zcan_filter *filter);

/**
 * @brief Attach a message queue to a single or group of identifiers.
 *
 * This routine attaches a message queue to identifiers specified by
 * a filter. Whenever the filter matches, the message is pushed to the queue
 * If a message passes more than one filter the priority of the match
 * is hardware dependent.
 * A message queue can be attached to more than one filter.
 * The message queue must me initialized before, and the caller must have
 * appropriate permissions on it.
 *
 * @param dev    Pointer to the device structure for the driver instance.
 * @param msg_q  Pointer to the already initialized message queue.
 * @param filter Pointer to a zcan_filter structure defining the id
 *               filtering.
 *
 * @retval filter_id on success.
 * @retval CAN_NO_FREE_FILTER if there is no filter left.
 */
__syscall int can_attach_msgq(struct device *dev, struct k_msgq *msg_q,
			      const struct zcan_filter *filter);

/**
 * @brief Attach an isr callback function to a single or group of identifiers.
 *
 * This routine attaches an isr callback to identifiers specified by
 * a filter. Whenever the filter matches, the callback function is called
 * with isr context.
 * If a message passes more than one filter the priority of the match
 * is hardware dependent.
 * A callback function can be attached to more than one filter.
 * *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param isr          Callback function pointer.
 * @param callback_arg This will be passed whenever the isr is called.
 * @param filter       Pointer to a zcan_filter structure defining the id
 *                     filtering.
 *
 * @retval filter_id on success.
 * @retval CAN_NO_FREE_FILTER if there is no filter left.
 */
static inline int can_attach_isr(struct device *dev,
				       can_rx_callback_t isr,
				       void *callback_arg,
				       const struct zcan_filter *filter)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->attach_isr(dev, isr, callback_arg, filter);
}

/**
 * @brief Detach an isr or message queue from the identifier filtering.
 *
 * This routine detaches an isr callback  or message queue from the identifier
 * filtering.
 * *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param filter_id filter id returned by can_attach_isr or can_attach_msgq.
 *
 * @retval none
 */
__syscall void can_detach(struct device *dev, int filter_id);

static inline void z_impl_can_detach(struct device *dev, int filter_id)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->detach(dev, filter_id);
}

/**
 * @brief Read the core clock value
 *
 * Returns the core clock value. One time quantum is 1/core clock.
 * *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval Core clock value
 * @retval -EIO on error
 */
__syscall u32_t can_get_core_clock(struct device *dev);

static inline u32_t z_impl_can_get_core_clock(struct device *dev)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->get_core_clock(dev);
}

/**
 * @brief Calculate timing parameters from bitrate and sample point
 *
 * Calculate the timing parameters from a given bitrate in bits/s and the
 * sampling point in permill (1/1000) of the entire bit time.
 * The bitrate must alway match perfectly. If no result can be given for the,
 * give parameters, -EINVAL is returned.
 * The sample_pnt does not always match perfectly. The algorithm tries to find
 * the best match possible.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param res        Result is written into the can_timing struct provided.
 * @param bitrate    Target bitrate in bits/s
 * @param sample_pnt Sampling point in permill of the entire bit time.
 *
 * @retval Positive sample point error on success
 * @retval -EINVAL if there is no solution for the desired values
 */
int can_calc_timing(struct device *dev, struct can_timing *res,
		    u32_t bitrate, u16_t sample_pnt);

#ifdef CONFIG_CAN_FD_MODE
/**
 * @brief Calculate timing parameters for the data pase
 *
 * Same as can_calc_timing but with the max and min values from the data phase.
 *
 * @param dev        Pointer to the device structure for the driver instance.
 * @param res        Result is written into the can_timing struct provided.
 * @param bitrate    Target bitrate for the datapase in bits/s
 * @param sample_pnt Sampling point datapase in permill of the entire bit time.
 *
 * @retval Positive sample point error on success
 * @retval -EINVAL if there is no solution for the desired values
 */
int can_calc_timing_data(struct device *dev, struct can_timing *res,
			 u32_t bitrate, u16_t sample_pnt);
#endif

/**
 * @brief Fill in the prescaler value for a given bitrate and timing
 *
 * Fill the prescaler value in the timing struct.
 * sjw, prop_seg, phase_seg1 and phase_seg2 must be given.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param timing  Result is written into the can_timing struct provided.
 * @param bitrate Target bitrate.
 *
 * @retval bitrate error
 */
int can_calc_prescaler(struct device *dev, struct can_timing *timing,
		       u32_t bitrate);

/**
 * @brief Configure operation of a host controller.
 *
 * The timing structs can either be valid timings or NULL. If NULL is passed,
 * the bus timing does not change. timing_data is only relevant for CAN-FD
 * If the controller does not support CAN-FD or the FD mode is not enabled,
 * this parameter is ignored.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode   Operation mode
 * @param timing Bus timings
 * @param timing Bus timings for the dataphase (CAN-FD only)
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 */
__syscall int can_configure(struct device *dev, enum can_mode mode,
			    struct can_timing *timing,
			    struct can_timing *timing_data);

static inline int z_impl_can_configure(struct device *dev, enum can_mode mode,
				       struct can_timing *timing,
				       struct can_timing *timing_data)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->configure(dev, mode, timing, timing_data);
}

/**
 * @brief Set the controller to the given mode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode   Operation mode
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 */
static inline int can_set_mode(struct device *dev, enum can_mode mode)
{
	return can_configure(dev, mode, NULL, NULL);
}

/**
 * @brief Set the bitrate of the controller
 *
 * The sample point is set to the CiA DS 301 reccommended value of 87.5%
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param bitrate Desired bitrate
 *
 * @retval 0 If successful.
 * @retval -EINVAL bitrate cannot be reached.
 * @retval -EIO General input / output error, failed to configure device.
 */
static inline int can_set_bitrate(struct device *dev, u32_t bitrate)
{
	struct can_timing timing = {.sjw = 1};
	int ret;

	ret = can_calc_timing(dev, &timing, bitrate, 875);
	if (ret < 0) {
		return -EINVAL;
	}

	return can_configure(dev, CAN_NO_MODE_CHANGE, &timing, NULL);
}

#ifdef CONFIG_CAN_FD_MODE
/**
 * @brief Set the dataphase bitrate of the controller
 *
 * The sample point is set to the CiA DS 301 reccommended value of 87.5%
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param bitrate Desired dataphase bitrate
 *
 * @retval 0 If successful.
 * @retval -EINVAL bitrate cannot be reached.
 * @retval -EIO General input / output error, failed to configure device.
 */
static inline int can_set_bitrate_data(struct device *dev, u32_t bitrate)
{
	struct can_timing timing = {.sjw = 1};
	int ret;

	ret = can_calc_timing_data(dev, &timing, bitrate, 875);
	if (ret < 0) {
		return -EINVAL;
	}

	return can_configure(dev, CAN_NO_MODE_CHANGE, NULL, &timing);
}

/**
 * @brief Set the bitrate of the CAN-FD controller
 *
 * The sample point is set to the CiA DS 301 reccommended value of 87.5%
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param bitrate      Desired arbitration phase bitrate
 * @param bitrate_data Desired data phase bitrate
 *
 * @retval 0 If successful.
 * @retval -EINVAL bitrate cannot be reached.
 * @retval -EIO General input / output error, failed to configure device.
 */
static inline int can_set_bitrate_fd(struct device *dev, u32_t bitrate,
				     u32_t bitrate_data)
{
	struct can_timing timing;
	struct can_timing timing_data;
	int ret;

	ret = can_calc_timing_data(dev, &timing, bitrate, 875);
	if (ret < 0) {
		return -EINVAL;
	}

	ret = can_calc_timing_data(dev, &timing_data, bitrate, 875);
	if (ret < 0) {
		return -EINVAL;
	}

	return can_configure(dev, CAN_NO_MODE_CHANGE, &timing, &timing_data);
}
#endif /* CONFIG_CAN_FD_MODE */

/**
 * @brief Get current state
 *
 * Returns the actual state of the CAN controller.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param err_cnt Pointer to the err_cnt destination structure or NULL.
 *
 * @retval  state
 */
__syscall enum can_state can_get_state(struct device *dev,
				       struct can_bus_err_cnt *err_cnt);

static inline
enum can_state z_impl_can_get_state(struct device *dev,
				    struct can_bus_err_cnt *err_cnt)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->get_state(dev, err_cnt);
}

/**
 * @brief Recover from bus-off state
 *
 * Recover the CAN controller from bus-off state to error-active state.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param timeout Timeout for waiting for the recovery or K_FOREVER.
 *
 * @retval 0 on success.
 * @retval CAN_TIMEOUT on timeout.
 */
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
__syscall int can_recover(struct device *dev, k_timeout_t timeout);

static inline int z_impl_can_recover(struct device *dev, k_timeout_t timeout)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->recover(dev, timeout);
}
#else
/* This implementation prevents inking errors for auto recovery */
static inline int z_impl_can_recover(struct device *dev, k_timeout_t timeout)
{
	return 0;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

/**
 * @brief Register an ISR callback for state change interrupt
 *
 * Only one callback can be registered per controller.
 * Calling this function again, overrides the previous call.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param isr Pointer to ISR
 */
static inline
void can_register_state_change_isr(struct device *dev,
				   can_state_change_isr_t isr)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->driver_api;

	return api->register_state_change_isr(dev, isr);
}

/**
 * @brief Converter that translates between can_frame and zcan_frame structs.
 *
 * @param frame Pointer to can_frame struct.
 * @param zframe Pointer to zcan_frame struct.
 */
static inline void can_copy_frame_to_zframe(const struct can_frame *frame,
					    struct zcan_frame *zframe)
{
	memcpy(zframe, frame, sizeof(struct can_frame));
}

/**
 * @brief Converter that translates between zcan_frame and can_frame structs.
 *
 * @param zframe Pointer to zcan_frame struct.
 * @param frame Pointer to can_frame struct.
 */
static inline void can_copy_zframe_to_frame(const struct zcan_frame *zframe,
					    struct can_frame *frame)
{
	memcpy(frame, zframe, sizeof(struct zcan_frame));
}

/**
 * @brief Converter that translates between can_filter and zcan_frame_filter
 * structs.
 *
 * @param filter Pointer to can_filter struct.
 * @param zfilter Pointer to zcan_frame_filter struct.
 */
static inline
void can_copy_filter_to_zfilter(const struct can_filter *filter,
				struct zcan_filter *zfilter)
{
	memcpy(zfilter, filter, sizeof(struct zcan_filter));
}

/**
 * @brief Converter that translates between zcan_filter and can_filter
 * structs.
 *
 * @param zfilter Pointer to zcan_filter struct.
 * @param filter Pointer to can_filter struct.
 */
static inline
void can_copy_zfilter_to_filter(const struct zcan_filter *zfilter,
				struct can_filter *filter)
{
	memcpy(filter, zfilter, sizeof(struct can_filter));
}

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#include <syscalls/can.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_H_ */
