/* ipi.h - Generic low-level inter-processor interrupt communication API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INCipih
#define __INCipih

#include <nanokernel.h>
#include <device.h>

/**
 * Callback API for incoming IPI messages
 *
 * These callbacks are executed in interrupt context, and only interrupt-
 * safe APIS may be used. Registration of callbacks is done via
 * ipi_register_callback()
 *
 * @param context Arbitrary context pointer provided at registration time
 * @param id Message type identifier
 * @param data Message data pointer. The correct amount of data to read out
 * needs to be inferred by the message id/upper level protocol
 */
typedef void (*ipi_callback_t)(void *context, uint32_t id, volatile void *data);

typedef int (*ipi_send_t)(struct device *ipidev, int wait, uint32_t id,
			  const void *data, int size);
typedef int (*ipi_max_data_size_get_t)(struct device *ipidev);
typedef uint32_t (*ipi_max_id_val_get_t)(struct device *ipidev);
typedef void (*ipi_register_callback_t)(struct device *port, ipi_callback_t cb,
					void *cb_context);
typedef int (*ipi_set_enabled_t)(struct device *ipidev, int enable);

struct ipi_driver_api {
	ipi_send_t send;
	ipi_register_callback_t register_callback;
	ipi_max_data_size_get_t max_data_size_get;
	ipi_max_id_val_get_t max_id_val_get;
	ipi_set_enabled_t set_enabled;
};

/**
 * Try to send a message over the IPI device
 *
 * @param ipidev Driver instance
 * @param wait If nonzero, busy-wait for remote to consume the message. The
 *	       message is considered consumed once the remote interrupt handler
 *	       finishes. If there is deferred processing on the remote side,
 *	       or you would like to queue outgoing messages and wait on an
 *	       event/semaphore, you can implement that in a high-level driver
 * @param id Message identifier. Values are constrained by
 *	     ipi_max_data_size_get() since many platforms only allow for a
 *	     subset of bits in a 32-bit register to store the ID.
 * @param data Pointer to data to send in the message
 * @param size size of the data pointed to
 *
 * There are constraints on how much data can be sent or the maximum value
 * of id, use the ipi_max_data_size_get() and ipi_max_id_val_get() functions
 * to determine these.
 *
 * The size parameter is only used on the sending side to know how much data
 * to put in the message registers, it is not passed along to the receiving
 * side. How much data to read back should be dictated by the upper-level
 * protocol.
 *
 * @return -EBUSY if the remote hasn't yet read the last data sent
 *	   -EMSGSIZE If the supplied data size is unsupported by the driver
 *	   -EINVAL   Bad parameter, such as a too-large id value, or
 *		     the device isn't an outbound IPI channel
 *	   0 Success
 */
static inline int ipi_send(struct device *ipidev, int wait, uint32_t id,
			   void *data, int size)
{
	struct ipi_driver_api *api;

	api = (struct ipi_driver_api *) ipidev->driver_api;
	return api->send(ipidev, wait, id, data, size);
}

/**
 * Register a callback function for incoming messages
 *
 * @param ipidev Driver instance
 * @param cb Callback function to execute on incoming message interrupts
 * @param context Application-specific context pointer which will be passed
 *        to the callback function when executed.
 */
static inline void ipi_register_callback(struct device *ipidev,
					 ipi_callback_t cb, void *context)
{
	struct ipi_driver_api *api;

	api = (struct ipi_driver_api *) ipidev->driver_api;
	api->register_callback(ipidev, cb, context);
}

/**
 * Return the maxmimum number of bytes that can be sent in an outbound message
 *
 * IPI implementations vary in how much data can be sent in a single message
 * since the data payload is typically stored in registers.
 *
 * @param ipidev Driver instance pointer
 *
 * @return Max size of a message in bytes
 */
static inline int ipi_max_data_size_get(struct device *ipidev)
{
	struct ipi_driver_api *api;

	api = (struct ipi_driver_api *) ipidev->driver_api;
	return api->max_data_size_get(ipidev);
}


/**
 * Return the maximum id value that can be sent in an outbound message
 *
 * Many IPI implementations store the id in a register which may have some bits
 * reserved for other use
 *
 * @param ipidev Driver instance pointer
 *
 * @return Maximum value of a message id
 */
static inline uint32_t ipi_max_id_val_get(struct device *ipidev)
{
	struct ipi_driver_api *api;

	api = (struct ipi_driver_api *) ipidev->driver_api;
	return api->max_id_val_get(ipidev);
}

/**
 * For inbound channels, enable interrupts/callbacks
 *
 * @param ipidev Driver instance pointer
 * @param enable 0=disable nonzero=enable
 *
 * @return 0 on success, -EINVAL if this isn't an inbound channel
 */
static inline int ipi_set_enabled(struct device *ipidev, int enable)
{
	struct ipi_driver_api *api;

	api = (struct ipi_driver_api *) ipidev->driver_api;
	return api->set_enabled(ipidev, enable);
}


#endif /* __INCipih */
