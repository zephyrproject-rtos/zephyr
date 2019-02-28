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

#ifndef ZEPHYR_INCLUDE_CAN_H_
#define ZEPHYR_INCLUDE_CAN_H_

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
	/** Indicates the identifier type (standard or extended)
	 * use can_ide enum for assignment
	 */
	u32_t id_type : 1;
	/** Set the message to a transmission request instead of data frame
	 * use can_rtr enum for assignment
	 */
	u32_t rtr     : 1;
	/** Message identifier*/
	union {
		u32_t std_id  : 11;
		u32_t ext_id  : 29;
	};
	/** The length of the message (max. 8) in byte */
	u8_t dlc;
	/** The message data*/
	union {
		u8_t data[8];
		u32_t data_32[2];
	};
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
	u32_t id_type      : 1;
	/** target state of the rtr bit */
	u32_t rtr          : 1;
	/** target state of the identifier */
	union {
		u32_t std_id   : 11;
		u32_t ext_id   : 29;
	};
	/** rtr bit mask */
	u32_t rtr_mask     : 1;
	/** identifier mask*/
	union {
		u32_t std_id_mask  : 11;
		u32_t ext_id_mask  : 29;
	};
} __packed;

/**
 * @typedef can_tx_callback_t
 * @brief Define the application callback handler function signature
 *
 * @param error_flags status of the performed send operation
 */
typedef void (*can_tx_callback_t)(u32_t error_flags);

/**
 * @typedef can_rx_callback_t
 * @brief Define the application callback handler function signature
 *        for receiving.
 *
 * @param received message
 */
typedef void (*can_rx_callback_t)(struct zcan_frame *msg);

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
typedef int (*can_configure_t)(struct device *dev, enum can_mode mode,
				u32_t bitrate);

/**
 * @brief Perform data transfer to CAN bus.
 *
 * This routine provides a generic interface to perform data transfer
 * to the can bus. Use can_write() for simple write.
 * *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param msg          Message to transfer.
 * @param timeout      Waiting for empty tx mailbox timeout in ms or K_FOREVER.
 * @param callback_isr Is called when message was sent or a transmission error
 *                     occurred. If null, this function is blocking until
 *                     message is sent.
 *
 * @retval 0 If successful.
 * @retval CAN_TX_* on failure.
 */
typedef int (*can_send_t)(struct device *dev, const struct zcan_frame *msg,
			  s32_t timeout, can_tx_callback_t callback_isr);


/**
 * @brief Attach a message queue to a single or group of identifiers.
 *
 * This routine attaches a message queue to identifiers specified by
 * a filter. Whenever the filter matches, the message is pushed to the queue
 * If a message passes more than one filter the priority of the match
 * is hardware dependent.
 * A message queue can be attached to more than one filter.
 * The message queue must me initialized before.
 * *
 * @param dev    Pointer to the device structure for the driver instance.
 * @param msgq   Pointer to the already initialized message queue.
 * @param filter Pointer to a zcan_filter structure defining the id
 *               filtering.
 *
 * @retval filter id on success.
 * @retval CAN_NO_FREE_FILTER if there is no filter left.
 */
typedef int (*can_attach_msgq_t)(struct device *dev, struct k_msgq *msg_q,
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
 * @param dev    Pointer to the device structure for the driver instance.
 * @param isr    Callback function pointer.
 * @param filter Pointer to a zcan_filter structure defining the id
 *               filtering.
 *
 * @retval filter id on success.
 * @retval CAN_NO_FREE_FILTER if there is no filter left.
 */
typedef int (*can_attach_isr_t)(struct device *dev, can_rx_callback_t isr,
				const struct zcan_filter *filter);

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
typedef void (*can_detach_t)(struct device *dev, int filter_id);

struct can_driver_api {
	can_configure_t configure;
	can_send_t send;
	can_attach_isr_t attach_isr;
	can_attach_msgq_t attach_msgq;
	can_detach_t detach;
};


__syscall int can_send(struct device *dev, const struct zcan_frame *msg,
		       s32_t timeout, can_tx_callback_t callback_isr);

static inline int _impl_can_send(struct device *dev,
				 const struct zcan_frame *msg,
				 s32_t timeout, can_tx_callback_t callback_isr)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->send(dev, msg, timeout, callback_isr);
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
 * @param timeout Waiting for empty tx mailbox timeout in ms or K_FOREVER
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -EINVAL if length > 8.
 */
static inline int can_write(struct device *dev, const u8_t *data, u8_t length,
			    u32_t id, enum can_rtr rtr, s32_t timeout)
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

	return can_send(dev, &msg, timeout, NULL);
}


__syscall int can_attach_msgq(struct device *dev, struct k_msgq *msg_q,
			      const struct zcan_filter *filter);

static inline int _impl_can_attach_msgq(struct device *dev,
					struct k_msgq *msg_q,
					const struct zcan_filter *filter)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->attach_msgq(dev, msg_q, filter);
}


__syscall int can_attach_isr(struct device *dev, can_rx_callback_t isr,
			     const struct zcan_filter *filter);
static inline int _impl_can_attach_isr(struct device *dev,
				       can_rx_callback_t isr,
				       const struct zcan_filter *filter)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->attach_isr(dev, isr, filter);
}


__syscall void can_detach(struct device *dev, int filter_id);

static inline void _impl_can_detach(struct device *dev, int filter_id)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->detach(dev, filter_id);
}


__syscall int can_configure(struct device *dev, enum can_mode mode,
			    u32_t bitrate);

static inline int _impl_can_configure(struct device *dev, enum can_mode mode,
				      u32_t bitrate)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->configure(dev, mode, bitrate);
}

/**
 * @brief Converter that translates between can_frame and zcan_frame structs.
 *
 * @param frame Pointer to can_frame struct.
 * @param zframe Pointer to zcan_frame struct.
 */
static inline void can_copy_frame_to_zframe(struct can_frame *frame,
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
static inline void can_copy_zframe_to_frame(struct zcan_frame *zframe,
					    struct can_frame *frame)
{
	frame->can_id = (zframe->id_type << 31) | (zframe->rtr << 30) |
		zframe->ext_id;
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
void can_copy_filter_to_zfilter(struct can_filter *filter,
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
void can_copy_zfilter_to_filter(struct zcan_filter *zfilter,
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

#endif /* ZEPHYR_INCLUDE_CAN_H_ */
