/*
 * Copyright 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_IBI_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_IBI_H_

/**
 * @brief I3C In-Band Interrupts
 * @defgroup i3c_ibi I3C In-Band Interrupts
 * @ingroup i3c_interface
 * @{
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#ifndef CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE
#define CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct i3c_device_desc;

/**
 * @brief IBI Types.
 */
enum i3c_ibi_type {
	/** Target interrupt */
	I3C_IBI_TARGET_INTR,

	/** Controller Role Request */
	I3C_IBI_CONTROLLER_ROLE_REQUEST,

	/** Hot Join Request */
	I3C_IBI_HOTJOIN,

	I3C_IBI_TYPE_MAX = I3C_IBI_HOTJOIN,

	/**
	 * Not an actual IBI type, but simply used by
	 * the IBI workq for generic callbacks.
	 */
	I3C_IBI_WORKQUEUE_CB,
};

/**
 * @brief Struct for IBI request.
 */
struct i3c_ibi {
	/** Type of IBI. */
	enum i3c_ibi_type	ibi_type;

	/** Pointer to payload of IBI. */
	uint8_t			*payload;

	/** Length in bytes of the IBI payload. */
	uint8_t			payload_len;
};

/**
 * @brief Structure of payload buffer for IBI.
 *
 * This is used for the IBI callback.
 */
struct i3c_ibi_payload {
	/**
	 * Length of available data in the payload buffer.
	 */
	uint8_t payload_len;

	/**
	 * Pointer to byte array as payload buffer.
	 */
	uint8_t payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
};

/**
 * @brief Node about a queued IBI.
 */
struct i3c_ibi_work {
	sys_snode_t node;

	/**
	 * k_work struct.
	 */
	struct k_work work;

	/**
	 * IBI type.
	 */
	enum i3c_ibi_type type;

	union {
		/**
		 * Use for @see I3C_IBI_HOTJOIN.
		 */
		const struct device *controller;

		/**
		 * Use for @see I3C_IBI_TARGET_INTR,
		 * and @see I3C_IBI_CONTROLLER_ROLE_REQUEST.
		 */
		struct i3c_device_desc *target;
	};

	union {
		/**
		 * IBI payload.
		 */
		struct i3c_ibi_payload payload;

		/**
		 * Generic workqueue callback when
		 * type is I3C_IBI_WORKQUEUE_CB.
		 */
		k_work_handler_t work_cb;
	};
};

/**
 * @brief Function called when In-Band Interrupt received from target device.
 *
 * This function is invoked by the controller when the controller
 * receives an In-Band Interrupt from the target device.
 *
 * A success return shall cause the controller to ACK the next byte
 * received.  An error return shall cause the controller to NACK the
 * next byte received.
 *
 * @param target the device description structure associated with the
 *               device to which the operation is addressed.
 * @param payload Payload associated with the IBI. NULL if there is
 *                no payload.
 *
 * @return 0 if the IBI is accepted, or a negative error code.
 */
typedef int (*i3c_target_ibi_cb_t)(struct i3c_device_desc *target,
				   struct i3c_ibi_payload *payload);


/**
 * @brief Queue an IBI work item for future processing.
 *
 * This queues up an IBI work item in the IBI workqueue
 * for future processing.
 *
 * Note that this will copy the @p ibi_work struct into
 * internal structure. If there is not enough space to
 * copy the @p ibi_work struct, this returns -ENOMEM.
 *
 * @param ibi_work Pointer to the IBI work item struct.
 *
 * @retval 0 If work item is successfully queued.
 * @retval -ENOMEM If no more free internal node to
 *                 store IBI work item.
 * @retval Others @see k_work_submit_to_queue
 */
int i3c_ibi_work_enqueue(struct i3c_ibi_work *ibi_work);

/**
 * @brief Queue a target interrupt IBI for future processing.
 *
 * This queues up a target interrupt IBI in the IBI workqueue
 * for future processing.
 *
 * @param target Pointer to target device descriptor.
 * @param payload Pointer to IBI payload byte array.
 * @param payload_len Length of payload byte array.
 *
 * @retval 0 If work item is successfully queued.
 * @retval -ENOMEM If no more free internal node to
 *                 store IBI work item.
 * @retval Others @see k_work_submit_to_queue
 */
int i3c_ibi_work_enqueue_target_irq(struct i3c_device_desc *target,
				    uint8_t *payload, size_t payload_len);

/**
 * @brief Queue a hot join IBI for future processing.
 *
 * This queues up a hot join IBI in the IBI workqueue
 * for future processing.
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @retval 0 If work item is successfully queued.
 * @retval -ENOMEM If no more free internal node to
 *                 store IBI work item.
 * @retval Others @see k_work_submit_to_queue
 */
int i3c_ibi_work_enqueue_hotjoin(const struct device *dev);

/**
 * @brief Queue a generic callback for future processing.
 *
 * This queues up a generic callback in the IBI workqueue
 * for future processing.
 *
 * @param dev Pointer to controller device driver instance.
 * @param work_cb Callback function.
 *
 * @retval 0 If work item is successfully queued.
 * @retval -ENOMEM If no more free internal node to
 *                 store IBI work item.
 * @retval Others @see k_work_submit_to_queue
 */
int i3c_ibi_work_enqueue_cb(const struct device *dev,
			    k_work_handler_t work_cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_IBI_H_ */
