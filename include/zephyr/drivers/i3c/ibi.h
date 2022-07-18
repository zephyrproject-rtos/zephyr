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

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_IBI_H_ */
