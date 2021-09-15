/**
 * @file
 *
 * @brief Generic low-level inter-processor mailbox communication API.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_IPM_H_
#define ZEPHYR_INCLUDE_DRIVERS_IPM_H_

/**
 * @brief IPM Interface
 * @defgroup ipm_interface IPM Interface
 * @ingroup io_interfaces
 * @{
 *
 *                      CPU #1        |
 * +----------+                       |        +----------+
 * |          +---TX9----+   +--------+--RX8---+          |
 * |  dev A   |          |   |        |        |  CPU #2  |
 * |          <---RX8--+ |   | +------+--TX9--->          |
 * +----------+        | |   | |      |        +----------+
 *                  +--+-v---v-+--+   |
 *                  |             |   |
 *                  |   IPM dev   |   |
 *                  |             |   |
 *                  +--+-^---^--+-+   |
 * +----------+        | |   |  |     |        +----------+
 * |          <---RX2--+ |   |  +-----+--TX3--->          |
 * |  dev B   |          |   |        |        |  CPU #3  |
 * |          +---TX3----+   +--------+--RX2---+          |
 * +----------+                       |        +----------+
 *                                    |
 *
 * An IPM device is a peripheral capable of passing signals (and data depending
 * on the peripheral) between CPUs and clusters in the system. Each IPM
 * instance is providing one or more channels, each one targeting one other CPU
 * cluster (multiple channels can target the same cluster).
 *
 * For example in the plot the device 'dev A' is using the TX channel 9 to
 * signal (or exchange data with) the CPU #2 and it's expecting data or signals
 * on the RX channel 8. Thus it can send the message through the channel 9, and
 * it can register a callback on the channel 8 of the IPM device.
 *
 * While a message is sent through a selected channel it might be accompanied
 * with optional ID number, if given hardware supports such accompanying
 * number. Allowed range of allowed ID values for given IPM instance is
 * hardware dependent.
 *
 * IPM driver users can determine number of supported channels and max value of
 * ID using functions available in the driver API.
 */

#include <kernel.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Message struct (to hold id, data and its size).
 */
struct ipm_msg {
	 /**
	  * Message identifier. Values are constrained by @a
	  * ipm_max_data_size_get since many boards only allow for a subset of
	  * bits in a 32-bit register to store the ID.
	  */
	uint32_t id;

	/* Pointer to the data sent in the message. */
	const void *data;

	/* Size of the data. */
	size_t size;
};

/**
 * @typedef ipm_callback_t
 * @brief Callback API for incoming IPM messages
 *
 * These callbacks execute in interrupt context. Therefore, use only
 * interrupt-safe APIS. Registration of callbacks is done via
 * @a ipm_register_callback
 *
 * @param ipmdev Driver instance
 * @param user_data Pointer to some private data provided at registration
 *        time.
 * @param id Message type identifier.
 * @param data Message data pointer. The correct amount of data to read out
 *        must be inferred using the message id/upper level protocol.
 */
typedef void (*ipm_callback_t)(const struct device *ipmdev, void *user_data,
			       uint32_t id, volatile void *data);

/**
 * @typedef ipm_send_t
 * @brief Callback API to send IPM messages
 *
 * See @a ipm_send() for argument definitions.
 */
typedef int (*ipm_send_t)(const struct device *ipmdev, int wait, uint32_t channel,
			  struct ipm_msg *msg);

/**
 * @typedef ipm_max_data_size_get_t
 * @brief Callback API to get maximum data size
 *
 * See @a ipm_max_data_size_get() for argument definitions.
 */
typedef int (*ipm_max_data_size_get_t)(const struct device *ipmdev);

/**
 * @typedef ipm_max_id_val_get_t
 * @brief Callback API to get the ID's maximum value
 *
 * See @a ipm_max_id_val_get() for argument definitions.
 */
typedef uint32_t (*ipm_max_id_val_get_t)(const struct device *ipmdev);

/**
 * @typedef ipm_register_callback_t
 * @brief Callback API upon registration
 *
 * See @a ipm_register_callback() for argument definitions.
 */
typedef void (*ipm_register_callback_t)(const struct device *dev,
					ipm_callback_t cb,
					uint32_t channel,
					void *user_data);

/**
 * @typedef ipm_set_enabled_t
 * @brief Callback API upon enablement of interrupts
 *
 * See @a ipm_set_enabled() for argument definitions.
 */
typedef int (*ipm_set_enabled_t)(const struct device *ipmdev, int enable);

/**
 * @typedef ipm_max_channels_get_t
 * @brief Callback API to get maximum number of channels
 *
 * See @a ipm_max_channels_get() for argument definitions.
 */
typedef uint32_t (*ipm_max_channels_get_t)(const struct device *ipmdev);

/**
 * @typedef ipm_configure_channel_t
 * @brief Configure the channel in the IPM device
 *
 * See @a ipm_configure_channel() for argument definitions.
 */
typedef int (*ipm_configure_channel_t)(const struct device *ipmdev, uint32_t channel,
				       void *conf);

__subsystem struct ipm_driver_api {
	ipm_send_t send;
	ipm_register_callback_t register_callback;
	ipm_max_data_size_get_t max_data_size_get;
	ipm_max_id_val_get_t max_id_val_get;
	ipm_max_channels_get_t max_channels_get;
	ipm_set_enabled_t set_enabled;
	ipm_configure_channel_t configure_channel;
};

/**
 * @brief Try to send a message over the IPM device.
 *
 * A message is considered consumed once the remote interrupt handler
 * finishes. If there is deferred processing on the remote side,
 * or if outgoing messages must be queued and wait on an
 * event/semaphore, a high-level driver can implement that.
 *
 * There are constraints on how much data can be sent or the maximum value
 * of id. Use the @a ipm_max_data_size_get and @a ipm_max_id_val_get routines
 * to determine them.
 *
 * The @a size parameter is used only on the sending side to determine
 * the amount of data to put in the message registers. It is not passed along
 * to the receiving side. The upper-level protocol dictates the amount of
 * data read back.
 *
 * @param ipmdev Driver instance
 * @param wait If nonzero, busy-wait for remote to consume the message. The
 *	       message is considered consumed once the remote interrupt handler
 *	       finishes. If there is deferred processing on the remote side,
 *	       or you would like to queue outgoing messages and wait on an
 *	       event/semaphore, you can implement that in a high-level driver
 * @param channel Channel to use
 * @param msg Pointer to the message struct
 *
 * @retval -EBUSY    If the remote hasn't yet read the last data sent.
 * @retval -EMSGSIZE If the supplied data size is unsupported by the driver.
 * @retval -EINVAL   If there was a bad parameter, such as: too-large id value.
 *                   or the device isn't an outbound IPM channel.
 * @retval 0         On success.
 */
__syscall int ipm_send(const struct device *ipmdev, int wait, uint32_t channel,
		       struct ipm_msg *msg);

static inline int z_impl_ipm_send(const struct device *ipmdev, int wait, uint32_t channel,
				  struct ipm_msg *msg)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	return api->send(ipmdev, wait, channel, msg);
}

/**
 * @brief Register a callback function for incoming messages.
 *
 * @param ipmdev Driver instance pointer.
 * @param cb Callback function to execute on incoming message interrupts.
 * @param channel Channel to register the callback on.
 * @param user_data Application-specific data pointer which will be passed
 *        to the callback function when executed.
 */
static inline void ipm_register_callback(const struct device *ipmdev,
					 ipm_callback_t cb, uint32_t channel,
					 void *user_data)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	api->register_callback(ipmdev, cb, channel, user_data);
}

/**
 * @brief Return the maximum number of bytes possible in an outbound message.
 *
 * IPM implementations vary on the amount of data that can be sent in a
 * single message since the data payload is typically stored in registers.
 *
 * @param ipmdev Driver instance pointer.
 *
 * @return Maximum possible size of a message in bytes.
 */
__syscall int ipm_max_data_size_get(const struct device *ipmdev);

static inline int z_impl_ipm_max_data_size_get(const struct device *ipmdev)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	return api->max_data_size_get(ipmdev);
}


/**
 * @brief Return the maximum id value possible in an outbound message.
 *
 * Many IPM implementations store the message's ID in a register with
 * some bits reserved for other uses.
 *
 * @param ipmdev Driver instance pointer.
 *
 * @return Maximum possible value of a message ID.
 */
__syscall uint32_t ipm_max_id_val_get(const struct device *ipmdev);

static inline uint32_t z_impl_ipm_max_id_val_get(const struct device *ipmdev)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	return api->max_id_val_get(ipmdev);
}

/**
 * @brief Enable interrupts and callbacks for inbound channels.
 *
 * @param ipmdev Driver instance pointer.
 * @param enable Set to 0 to disable and to nonzero to enable.
 *
 * @retval 0       On success.
 * @retval -EINVAL If it isn't an inbound channel.
 */
__syscall int ipm_set_enabled(const struct device *ipmdev, int enable);

static inline int z_impl_ipm_set_enabled(const struct device *ipmdev,
					 int enable)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	return api->set_enabled(ipmdev, enable);
}

/**
 * @brief Return the maximum number of channels.
 *
 * Return the maximum number of channels supported by the hardware.
 *
 * @param ipmdev Driver instance pointer.
 *
 * @return Maximum possible number of supported channels.
 */
__syscall uint32_t ipm_max_channels_get(const struct device *ipmdev);

static inline uint32_t z_impl_ipm_max_channels_get(const struct device *ipmdev)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	if (api->max_channels_get == NULL) {
		/* At least 1 channel */
		return 1;
	}

	return api->max_channels_get(ipmdev);
}

/**
 * @brief Configure channels.
 *
 * Configure the device for the specified channels.
 *
 * @param ipmdev Driver instance pointer.
 * @param channel IPM device channel
 * @param conf Pointer to user-defined configuration parameter.
 *
 * @retval 0       On success.
 * @retval -EINVAL If it isn't a valid channel.
 */
__syscall int ipm_configure_channel(const struct device *ipmdev, uint32_t channel,
				    void *conf);

static inline int z_impl_ipm_configure_channel(const struct device *ipmdev,
					       uint32_t channel, void *conf)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	if (api->configure_channel == NULL) {
		return -ENOSYS;
	}

	return api->configure_channel(ipmdev, channel, conf);
}

/**
 * @brief Provides a type to hold IPM information specified in devicetree
 *
 * This type is sufficient to hold an IPM device pointer and the channel ID
 * to control IPM configuration which may be given in devicetree.
 */
struct ipm_dt_spec {
	const struct device *dev;
	uint32_t channel;
};

/**
 * @brief Structure initializer for ipm_dt_spec from devicetree
 *
 * This helper macro expands to a static initializer for a @p ipm_dt_spec by
 * reading the relevant device controller and channel number from the
 * devicetree.
 *
 * Example devicetree fragment:
 *
 *     ipm1: ipm-controller@... { ... };
 *
 *     n: node {
 *             mboxes = <&ipm1 8>,
 *                      <&ipm1 9>;
 *             mbox-names = "tx", "rx";
 *     };
 *
 * Example usage:
 *
 *     const struct ipm_dt_spec spec = IPM_DT_SPEC_GET(DT_NODELABEL(n), tx);
 *
 * @param node_id Devicetree node identifier for the IPM device whose
 *                struct ipm_dt_spec to create an initializer for
 * @param name lowercase-and-underscores name for the mboxes element
 */
#define IPM_DT_SPEC_GET(node_id, name)						\
	{									\
		.dev = DEVICE_DT_GET(IPM_DT_CTLR_BY_NAME(node_id, name)),	\
		.channel = IPM_DT_CHANNEL_BY_NAME(node_id, name),		\
	}

/**
 * @brief Get the node identifier for the IPM controller from a
 *        mboxes property by name
 *
 * Example devicetree fragment:
 *
 *     ipm1: ipm-controller@... { ... };
 *
 *     n: node {
 *             mboxes = <&ipm1 8>,
 *                      <&ipm1 9>;
 *             mbox-names = "tx", "rx";
 *     };
 *
 * Example usage:
 *
 *     IPM_DT_CTLR_BY_NAME(DT_NODELABEL(n), tx) // DT_NODELABEL(ipm1)
 *     IPM_DT_CTLR_BY_NAME(DT_NODELABEL(n), rx) // DT_NODELABEL(ipm1)
 *
 * @param node_id node identifier for a node with a mboxes property
 * @param name lowercase-and-underscores name of a mboxes element
 *             as defined by the node's mbox-names property
 *
 * @return the node identifier for the IPM controller in the named element
 *
 * @see DT_PHANDLE_BY_NAME()
 */
#define IPM_DT_CTLR_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, mboxes, name)

/**
 * @brief Get a IPM channel value by name
 *
 * Example devicetree fragment:
 *
 *     ipm1: ipm@... {
 *             #mbox-cells = <1>;
 *     };
 *
 *     n: node {
 *		mboxes = <&ipm1 1>,
 *		         <&ipm1 6>;
 *		mbox-names = "tx", "rx";
 *     };
 *
 * Bindings fragment for the ipm compatible:
 *
 *     mbox-cells:
 *       - channel
 *
 * Example usage:
 *
 *     IPM_DT_CHANNEL_BY_NAME(DT_NODELABEL(n), tx) // 1
 *     IPM_DT_CHANNEL_BY_NAME(DT_NODELABEL(n), rx) // 6
 *
 * @param node_id node identifier for a node with a mboxes property
 * @param name lowercase-and-underscores name of a mboxes element
 *             as defined by the node's mbox-names property
 *
 * @return the channel value in the specifier at the named element or 0 if no
 *         channels are supported
 *
 * @see DT_PHA_BY_NAME()
 */
#define IPM_DT_CHANNEL_BY_NAME(node_id, name) \
	DT_PHA_BY_NAME_OR(node_id, mboxes, name, channel, 0)

/**
 * @brief Try to send a message over the IPM device.
 *
 * This is equivalent to:
 *
 *     ipm_send(spec->dev, wait, spec->channel, msg);
 *
 * @param spec IPM specification from devicetree.
 * @param wait If nonzero, busy-wait for remote to consume the message. The
 *	       message is considered consumed once the remote interrupt handler
 *	       finishes. If there is deferred processing on the remote side,
 *	       or you would like to queue outgoing messages and wait on an
 *	       event/semaphore, you can implement that in a high-level driver
 * @param msg Pointer to the message struct
 *
 * @return a value from ipm_send()
 */
static inline int ipm_send_dt(const struct ipm_dt_spec *spec, int wait, struct ipm_msg *msg)
{
	return ipm_send(spec->dev, wait, spec->channel, msg);
}

/**
 * @brief Register a callback function for incoming messages.
 *
 * This is equivalent to:
 *
 *     ipm_register_callback(spec->dev, cb, spec->channel, user_data);
 *
 * @param spec IPM specification from devicetree.
 * @param cb Callback function to execute on incoming message interrupts.
 * @param user_data Application-specific data pointer which will be passed
 *        to the callback function when executed.
 */
static inline void ipm_register_callback_dt(const struct ipm_dt_spec *spec,
					    ipm_callback_t cb, void *user_data)
{
	ipm_register_callback(spec->dev, cb, spec->channel, user_data);
}

/**
 * @brief Enable interrupts and callbacks for inbound channels.
 *
 * This is equivalent to:
 *
 *     ipm_set_enabled(spec->dev, enable);
 *
 * @param spec IPM specification from devicetree.
 * @param enable Set to 0 to disable and to nonzero to enable.
 *
 * @return a value from ipm_set_enable()
 */
static inline int ipm_set_enabled_dt(const struct ipm_dt_spec *spec, int enable)
{
	return ipm_set_enabled(spec->dev, enable);
}

/**
 * @brief Configure channel.
 *
 * Configure the device for the specified channel.
 *
 * This is equivalent to:
 *
 *     ipm_configure_channel(spec->dev, spec->channel, conf);
 *
 * @param spec IPM specification from devicetree.
 * @param conf Pointer to user-defined configuration parameter for the channel.
 *
 * @retval a value from ipm_configure_channel()
 */
static inline int ipm_configure_channel_dt(const struct ipm_dt_spec *spec, void *conf)
{
	return ipm_configure_channel(spec->dev, spec->channel, conf);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/ipm.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_IPM_H_ */
