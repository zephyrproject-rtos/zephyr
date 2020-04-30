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

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_EX_ID      (1 << 31)
#define CAN_MAX_STD_ID (0x7FF)
#define CAN_STD_ID_MASK CAN_MAX_STD_ID
#define CAN_EXT_ID_MASK (0x1FFFFFFF)
#define CAN_MAX_DLC    (8)
#define CAN_MAX_DLEN    8

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
	CAN_SILENT_LOOPBACK_MODE
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
typedef uint32_t canid_t;

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
	uint8_t can_dlc;

	/** @cond INTERNAL_HIDDEN */
	uint8_t pad;   /* padding */
	uint8_t res0;  /* reserved / padding */
	uint8_t res1;  /* reserved / padding */
	/** @endcond */

	/** The message data */
	uint8_t data[CAN_MAX_DLEN];
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
	/** Indicates the identifier type (standard or extended)
	 * use can_ide enum for assignment
	 */
	uint32_t id_type : 1;
	/** Set the message to a transmission request instead of data frame
	 * use can_rtr enum for assignment
	 */
	uint32_t rtr     : 1;
	/** Message identifier*/
	union {
		uint32_t std_id  : 11;
		uint32_t ext_id  : 29;
	};
	/** The length of the message (max. 8) in byte */
	uint8_t dlc;
	/** The message data*/
	union {
		uint8_t data[8];
		uint32_t data_32[2];
	};
#if defined(CONFIG_CAN_RX_TIMESTAMP)
	/** Timer value of the CAN free running timer.
	 * The timer is incremented every bit time and captured at the start
	 * of frame bit (SOF).
	 */
	uint16_t timestamp;
#endif
} __packed;

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
	/** Indicates the identifier type (standard or extended)
	 * use can_ide enum for assignment
	 */
	uint32_t id_type      : 1;
	/** target state of the rtr bit */
	uint32_t rtr          : 1;
	/** target state of the identifier */
	union {
		uint32_t std_id   : 11;
		uint32_t ext_id   : 29;
	};
	/** rtr bit mask */
	uint32_t rtr_mask     : 1;
	/** identifier mask*/
	union {
		uint32_t std_id_mask  : 11;
		uint32_t ext_id_mask  : 29;
	};
} __packed;

/**
 * @brief can bus error count structure
 *
 * Used to pass the bus error counters to userspace
 */
struct can_bus_err_cnt {
	uint8_t tx_err_cnt;
	uint8_t rx_err_cnt;
};

/**
 * @typedef can_tx_callback_t
 * @brief Define the application callback handler function signature
 *
 * @param error_flags status of the performed send operation
 * @param arg argument that was passed when the message was sent
 */
typedef void (*can_tx_callback_t)(uint32_t error_flags, void *arg);

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

typedef int (*can_configure_t)(const struct device *dev, enum can_mode mode,
				uint32_t bitrate);

typedef int (*can_send_t)(const struct device *dev,
			  const struct zcan_frame *msg,
			  k_timeout_t timeout, can_tx_callback_t callback_isr,
			  void *callback_arg);


typedef int (*can_attach_msgq_t)(const struct device *dev,
				 struct k_msgq *msg_q,
				 const struct zcan_filter *filter);

typedef int (*can_attach_isr_t)(const struct device *dev,
				can_rx_callback_t isr,
				void *callback_arg,
				const struct zcan_filter *filter);

typedef void (*can_detach_t)(const struct device *dev, int filter_id);

typedef int (*can_recover_t)(const struct device *dev, k_timeout_t timeout);

typedef enum can_state (*can_get_state_t)(const struct device *dev,
					  struct can_bus_err_cnt *err_cnt);

typedef void(*can_register_state_change_isr_t)(const struct device *dev,
					       can_state_change_isr_t isr);

#ifndef CONFIG_CAN_WORKQ_FRAMES_BUF_CNT
#define CONFIG_CAN_WORKQ_FRAMES_BUF_CNT 4
#endif
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

};

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
__syscall int can_send(const struct device *dev, const struct zcan_frame *msg,
		       k_timeout_t timeout, can_tx_callback_t callback_isr,
		       void *callback_arg);

static inline int z_impl_can_send(const struct device *dev,
				  const struct zcan_frame *msg,
				  k_timeout_t timeout,
				  can_tx_callback_t callback_isr,
				  void *callback_arg)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->api;

	return api->send(dev, msg, timeout, callback_isr, callback_arg);
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
static inline int can_write(const struct device *dev, const uint8_t *data,
			    uint8_t length,
			    uint32_t id, enum can_rtr rtr, k_timeout_t timeout)
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
int can_attach_workq(const struct device *dev, struct k_work_q  *work_q,
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
__syscall int can_attach_msgq(const struct device *dev, struct k_msgq *msg_q,
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
static inline int can_attach_isr(const struct device *dev,
				       can_rx_callback_t isr,
				       void *callback_arg,
				       const struct zcan_filter *filter)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->api;

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
__syscall void can_detach(const struct device *dev, int filter_id);

static inline void z_impl_can_detach(const struct device *dev, int filter_id)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->api;

	return api->detach(dev, filter_id);
}

/**
 * @brief Configure operation of a host controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode Operation mode
 * @param bitrate bus-speed in Baud/s
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 */
__syscall int can_configure(const struct device *dev, enum can_mode mode,
			    uint32_t bitrate);

static inline int z_impl_can_configure(const struct device *dev,
				       enum can_mode mode,
				       uint32_t bitrate)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->api;

	return api->configure(dev, mode, bitrate);
}

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
__syscall enum can_state can_get_state(const struct device *dev,
				       struct can_bus_err_cnt *err_cnt);

static inline
enum can_state z_impl_can_get_state(const struct device *dev,
				    struct can_bus_err_cnt *err_cnt)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->api;

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
__syscall int can_recover(const struct device *dev, k_timeout_t timeout);

static inline int z_impl_can_recover(const struct device *dev,
				     k_timeout_t timeout)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->api;

	return api->recover(dev, timeout);
}
#else
/* This implementation prevents inking errors for auto recovery */
static inline int z_impl_can_recover(const struct device *dev,
				     k_timeout_t timeout)
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
void can_register_state_change_isr(const struct device *dev,
				   can_state_change_isr_t isr)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)dev->api;

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
	zframe->id_type = (frame->can_id & BIT(31)) >> 31;
	zframe->rtr = (frame->can_id & BIT(30)) >> 30;
	zframe->ext_id = frame->can_id & BIT_MASK(29);
	zframe->dlc = frame->can_dlc;
	memcpy(zframe->data, frame->data, sizeof(zframe->data));
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
	frame->can_id = (zframe->id_type << 31) | (zframe->rtr << 30) |
		(zframe->id_type == CAN_STANDARD_IDENTIFIER ? zframe->std_id :
				    zframe->ext_id);
	frame->can_dlc = zframe->dlc;
	memcpy(frame->data, zframe->data, sizeof(frame->data));
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
	zfilter->id_type = (filter->can_id & BIT(31)) >> 31;
	zfilter->rtr = (filter->can_id & BIT(30)) >> 30;
	zfilter->ext_id = filter->can_id & BIT_MASK(29);
	zfilter->rtr_mask = (filter->can_mask & BIT(30)) >> 30;
	zfilter->ext_id_mask = filter->can_mask & BIT_MASK(29);
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
	filter->can_id = (zfilter->id_type << 31) |
		(zfilter->rtr << 30) | zfilter->ext_id;
	filter->can_mask = (zfilter->rtr_mask << 30) |
		(zfilter->id_type << 31) | zfilter->ext_id_mask;
}

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#include <syscalls/can.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_H_ */
