/*
 * Copyright 2024,2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_util
 * @brief Header file for SCMI Utility Macros.
 *
 * Contains various utility macros and macros used for protocol and
 * transport "registration".
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_UTIL_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_UTIL_H_

/**
 * @brief Helper macros and utilities for SCMI drivers
 * @defgroup scmi_util Utilities
 * @ingroup scmi_interface
 * @{
 */

/**
 * @brief Build protocol name from its ID
 *
 * Given a protocol ID, this macro builds the protocol
 * name. This is done by concatenating the scmi_protocol_
 * construct with the given protocol ID.
 *
 * @param proto Protocol ID in decimal format
 *
 * @return protocol name
 */
#define SCMI_PROTOCOL_NAME(proto) CONCAT(scmi_protocol_, proto)

#ifdef CONFIG_ARM_SCMI_MAILBOX_TRANSPORT
/**
 * @brief Devicetree compatible string for the Mailbox transport.
 */
#define DT_SCMI_TRANSPORT_COMPATIBLE arm_scmi
#elif CONFIG_ARM_SCMI_SMC_TRANSPORT
/**
 * @brief Devicetree compatible string for the SMC transport.
 */
#define DT_SCMI_TRANSPORT_COMPATIBLE arm_scmi_smc
#else
#error "Transport needs to define COMPATIBLE macro"
#endif

/**
 * @brief Check if a node is the base SCMI transport node.
 *
 * This macro determines if the given node_id corresponds to the primary
 * SCMI transport layer (the base node), by verifying if the node has
 * a compatible string matching the current transport's definition
 * (DT_SCMI_TRANSPORT_COMPATIBLE).
 *
 * @param node_id Protocol node identifier
 * @return 1 if the node is the base transport node, 0 otherwise.
 */
#define DT_SCMI_TRANSPORT_PROTO_IS_BASE(node_id) \
	DT_NODE_HAS_COMPAT(node_id, DT_SCMI_TRANSPORT_COMPATIBLE)

/**
 * @brief Get the protocol ID from the protocol DT node
 *
 * Map a DT protocol node to its corresponding protocol ID.
 * Base protocol requires special handling since it shares
 * the same DT node with the transport.
 *
 * @param node_id Protocol node identifier
 * @return protocol ID
 */
#define DT_SCMI_TRANSPORT_PROTOCOL_ID(node_id)					\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_IS_BASE(node_id),			\
		    (SCMI_PROTOCOL_BASE),					\
		    (DT_REG_ADDR_RAW(node_id)))

#ifdef CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS

#ifdef CONFIG_ARM_SCMI_MAILBOX_TRANSPORT
/** @brief Check if a protocol node has an associated channel
 *
 * This macro, when applied to a protocol node, checks if
 * the node has a dedicated static channel allocated to it.
 * This definition is specific to the mailbox driver and
 * each new transport layer driver should define its own
 * version of this macro based on the devicetree properties
 * that indicate the presence of a dedicated channel.
 *
 * @param node_id Protocol node identifier
 * @param idx channel index. Should be 0 for TX channels and 1 for
 * RX channels
 */
#define DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, idx)\
	DT_PROP_HAS_IDX(node_id, shmem, idx)
#elif CONFIG_ARM_SCMI_SMC_TRANSPORT
/** @brief Check if a protocol node has an associated channel
 *
 * For SMC transport, all protocols share the base channel.
 * Only the base node may describe channels. This allows future extensions
 * where the base node may describe an optional RX channel via
 * a second `shmem` phandle, while keeping per-protocol channels unsupported.
 *
 * @param node_id Protocol node identifier
 * @param idx channel index
 */
#define DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, idx)		\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_IS_BASE(node_id),	\
		    (DT_PROP_HAS_IDX(node_id, shmem, idx)),	\
		    (0))
#else
#error "Transport with static channels needs to define HAS_CHAN macro"
#endif

/** @cond INTERNAL_HIDDEN */
/* builds the symbol name of the static channel for a (proto, idx) pair */
#define SCMI_TRANSPORT_CHAN_NAME(proto, idx) CONCAT(scmi_channel_, proto, _, idx)
/** @endcond */

/** @cond INTERNAL_HIDDEN */

/*
 * Declare a TX SCMI channel.
 *
 * Given a node_id for a protocol, this macro declares the SCMI
 * TX channel statically bound to said protocol via the "extern"
 * qualifier. This is useful when the transport layer driver
 * supports static channels since all channel structures are
 * defined inside the transport layer driver. Internal helper used
 * by DT_SCMI_TRANSPORT_CHANNELS_DECLARE(); not meant to be used directly.
 */
#define DT_SCMI_TRANSPORT_TX_CHAN_DECLARE(node_id)				\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 0),		\
		    (extern struct scmi_channel					\
		     SCMI_TRANSPORT_CHAN_NAME(					\
			DT_SCMI_TRANSPORT_PROTOCOL_ID(node_id), 0);),		\
		    (extern struct scmi_channel					\
		     SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 0);))		\
/** @endcond */

/**
 * @brief Declare RX SCMI channel for a protocol node (optional)
 *
 * Declares (via @c extern) the RX channel symbol used by a protocol.
 *
 * Resolution order:
 * 1) If the protocol node provides a dedicated RX channel (index 1 in its transport
 *    description), declare that channel symbol.
 * 2) Otherwise, fall back to the RX channel described on the SCMI node (base protocol
 *    transport description), if present.
 * 3) If neither exists, no declaration is emitted and the RX channel is treated as
 *    absent.
 *
 * Note:
 * Some transports do not provide an RX channel. Also, the base protocol is
 * represented by the SCMI node itself (compatible "arm,scmi" or others), so relying
 * unconditionally on @c DT_PARENT(node_id) for transport properties can be incorrect.
 * The fallback therefore checks the SCMI node transport description using the
 * transport-provided DT_SCMI_TRANSPORT_PROTO_HAS_CHAN() macro.
 *
 * @param node_id Protocol node identifier
 */
#define DT_SCMI_TRANSPORT_RX_CHAN_DECLARE_BASE_RAW(node_id)				\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 1),			\
		    (extern struct scmi_channel						\
		     SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 1);),			\
		    ())

#define DT_SCMI_TRANSPORT_RX_CHAN_DECLARE_BASE(node_id)					\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_IS_BASE(node_id),				\
		    (DT_SCMI_TRANSPORT_RX_CHAN_DECLARE_BASE_RAW(node_id)),		\
		    (DT_SCMI_TRANSPORT_RX_CHAN_DECLARE_BASE_RAW(DT_PARENT(node_id))))	\

#define DT_SCMI_TRANSPORT_RX_CHAN_DECLARE(node_id)					\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 1),			\
		    (extern struct scmi_channel						\
		     SCMI_TRANSPORT_CHAN_NAME(DT_REG_ADDR_RAW(node_id), 1);),		\
		    (DT_SCMI_TRANSPORT_RX_CHAN_DECLARE_BASE(node_id)))

/**
 * @brief Declare SCMI TX/RX channels
 *
 * Given a node_id for a protocol, this macro declares the
 * SCMI TX and RX channels statically bound to said protocol via
 * the "extern" qualifier.
 *
 * @param node_id protocol node identifier
 */
#define DT_SCMI_TRANSPORT_CHANNELS_DECLARE(node_id)				\
	DT_SCMI_TRANSPORT_TX_CHAN_DECLARE(node_id)				\
	DT_SCMI_TRANSPORT_RX_CHAN_DECLARE(node_id)				\

/**
 * @brief Declare SCMI TX/RX channels using node instance number
 *
 * Same as DT_SCMI_TRANSPORT_CHANNELS_DECLARE() but uses the
 * protocol's node instance number and the @c DT_DRV_COMPAT macro.
 *
 * @param protocol node instance number
 */
#define DT_INST_SCMI_TRANSPORT_CHANNELS_DECLARE(inst)				\
	DT_SCMI_TRANSPORT_CHANNELS_DECLARE(DT_INST(inst, DT_DRV_COMPAT))

/**
 * @brief Get a reference to a protocol's SCMI TX channel
 *
 * Given a node_id for a protocol, this macro returns a
 * reference to an SCMI TX channel statically bound to said
 * protocol.
 *
 * @param node_id Protocol node identifier
 *
 * @return Reference to the struct scmi_channel of the TX channel
 * bound to the protocol identifier by node_id
 */
#define DT_SCMI_TRANSPORT_TX_CHAN(node_id)					\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 0),		\
		    (&SCMI_TRANSPORT_CHAN_NAME(					\
			DT_SCMI_TRANSPORT_PROTOCOL_ID(node_id), 0)),		\
		    (&SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 0)))

/**
 * @brief Get a reference to a protocol's SCMI RX channel
 *
 * Given a node_id for a protocol, this macro returns a
 * reference to an SCMI RX channel statically bound to said
 * protocol.
 *
 * @param node_id protocol node identifier
 *
 * @return reference to the struct scmi_channel of the RX channel
 * bound to the protocol identifier by node_id
 */
#define DT_SCMI_TRANSPORT_RX_CHAN_BASE_RAW(node_id)				\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 1),		\
		    (&SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 1)),		\
		    (NULL))

#define DT_SCMI_TRANSPORT_RX_CHAN_BASE(node_id)					\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_IS_BASE(node_id),			\
		    (DT_SCMI_TRANSPORT_RX_CHAN_BASE_RAW(node_id)),		\
		    (DT_SCMI_TRANSPORT_RX_CHAN_BASE_RAW(DT_PARENT(node_id))))

#define DT_SCMI_TRANSPORT_RX_CHAN(node_id)					\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 1),		\
		    (&SCMI_TRANSPORT_CHAN_NAME(DT_REG_ADDR_RAW(node_id), 1)),	\
		    (DT_SCMI_TRANSPORT_RX_CHAN_BASE(node_id)))


/**
 * @brief Define an SCMI channel for a protocol
 *
 * This macro defines a struct scmi_channel for a given protocol.
 * This should be used by the transport layer driver to statically
 * define SCMI channels for the protocols.
 *
 * @param node_id Protocol node identifier
 * @param idx channel index. Should be 0 for TX channels and 1
 * for RX channels
 * @param proto Protocol ID in decimal format
 * @param pdata Pointer to the channel's private data
 */
#define DT_SCMI_TRANSPORT_CHAN_DEFINE(node_id, idx, proto, pdata)		\
	struct scmi_channel SCMI_TRANSPORT_CHAN_NAME(proto, idx) =		\
	{									\
		.data = pdata,							\
	}

/** @cond INTERNAL_HIDDEN */

/*
 * Define an SCMI protocol's data.
 *
 * Each SCMI protocol is identified by a struct scmi_protocol
 * placed in a linker section called scmi_protocol. Each protocol
 * driver is required to use this macro for "registration". Using
 * this macro directly is highly discouraged and users should opt
 * for macros such as DT_SCMI_PROTOCOL_DEFINE_NODEV() or
 * DT_SCMI_PROTOCOL_DEFINE(), which also takes care of the static
 * channel declaration (if applicable).
 *
 * node_id	- protocol node identifier
 * proto	- protocol ID in decimal format
 * pdata	- protocol private data
 * version_val	- protocol version supported by the driver
 * pevents	- protocol registered events
 */
#define DT_SCMI_PROTOCOL_DATA_DEFINE(node_id, proto, pdata, version_val, pevents)	\
	STRUCT_SECTION_ITERABLE(scmi_protocol, SCMI_PROTOCOL_NAME(proto)) =		\
	{										\
		.id = proto,								\
		.tx = DT_SCMI_TRANSPORT_TX_CHAN(node_id),				\
		.rx = DT_SCMI_TRANSPORT_RX_CHAN(node_id),				\
		.data = pdata,								\
		.version = version_val,							\
		.events = pevents							\
	}
/** @endcond */

#else /* CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS */

/** @cond INTERNAL_HIDDEN */
/* no-op variant used when the transport does not support static channels */
#define DT_SCMI_TRANSPORT_CHANNELS_DECLARE(node_id)

/* variant used when the transport does not support static channels */
#define DT_SCMI_PROTOCOL_DATA_DEFINE(node_id, proto, pdata)			\
	STRUCT_SECTION_ITERABLE(scmi_protocol, SCMI_PROTOCOL_NAME(proto)) =	\
	{									\
		.id = proto,							\
		.data = pdata,							\
	}
/** @endcond */

#endif /* CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS */

/**
 * @brief Define an SCMI transport driver
 *
 * This is merely a wrapper over DEVICE_DT_INST_DEFINE(), but is
 * required since transport layer drivers are not allowed to place
 * their own init() function in the init section. Instead, transport
 * layer drivers place the scmi_core_transport_init() function in the
 * init section, which, in turn, will call the transport layer driver
 * init() function. This is required because the SCMI core needs to
 * perform channel binding and setup during the transport layer driver's
 * initialization.
 */
#define DT_INST_SCMI_TRANSPORT_DEFINE(inst, pm, data, config, level, prio, api)	\
	DEVICE_DT_INST_DEFINE(inst, &scmi_core_transport_init,			\
			      pm, data, config, level, prio, api)

/**
 * @brief Define an SCMI protocol
 *
 * This macro performs three important functions:
 *
 * 1. It defines a `struct scmi_protocol`, which is
 *    needed by all protocol drivers to work with the SCMI API.
 *
 * 2. It declares the static channels bound to the protocol.
 *    This is only applicable if the transport layer driver
 *    supports static channels.
 *
 * 3. It creates a `struct device` a sets the `data` field
 *    to the newly defined `struct scmi_protocol`. This is
 *    needed because the protocol driver needs to work with the
 *    SCMI API **and** the subsystem API.
 *
 * @param node_id Protocol node identifier
 * @param init_fn Pointer to protocol's initialization function
 * @param api Pointer to protocol's subsystem API
 * @param pm Pointer to the protocol's power management resources
 * @param data Pointer to protocol's private data
 * @param config Pointer to protocol's private constant data
 * @param level Protocol initialization level
 * @param prio Protocol's priority within its initialization level
 * @param version_val Protocol version supported by the driver
 */
#define DT_SCMI_PROTOCOL_DEFINE(node_id, init_fn, pm, data, config,		\
				level, prio, api, version_val, events)		\
	DT_SCMI_TRANSPORT_CHANNELS_DECLARE(node_id)				\
	DT_SCMI_PROTOCOL_DATA_DEFINE(node_id,					\
				     DT_SCMI_TRANSPORT_PROTOCOL_ID(node_id),	\
				     data, version_val, events);		\
	DEVICE_DT_DEFINE(node_id, init_fn, pm,					\
			 &SCMI_PROTOCOL_NAME(					\
				DT_SCMI_TRANSPORT_PROTOCOL_ID(node_id)),	\
			 config, level, prio, api)

/**
 * @brief Just like DT_SCMI_PROTOCOL_DEFINE(), but uses an instance
 * of a @c DT_DRV_COMPAT compatible instead of a node identifier
 *
 * @param inst Instance number
 * @param init_fn Pointer to protocol's initialization function
 * @param api Pointer to protocol's subsystem API
 * @param pm Pointer to the protocol's power management resources
 * @param data Pointer to protocol's private data
 * @param config Pointer to protocol's private constant data
 * @param level Protocol initialization level
 * @param prio Protocol's priority within its initialization level
 * @param version Protocol version supported by the driver
 */
#define DT_INST_SCMI_PROTOCOL_DEFINE(inst, init_fn, pm, data, config,		\
				     level, prio, api, version, events)		\
	DT_SCMI_PROTOCOL_DEFINE(DT_INST(inst, DT_DRV_COMPAT), init_fn, pm,	\
				data, config, level, prio, api, version, events)

/**
 * @brief Define an SCMI protocol with no device
 *
 * Variant of DT_SCMI_PROTOCOL_DEFINE(), but no `struct device` is
 * created and no initialization function is called during system
 * initialization. This is useful for protocols that are not really
 * part of a subsystem with an API (e.g: pinctrl).
 *
 * @param node_id Protocol node identifier
 * @param data Protocol private data
 * @param version Protocol version supported by the driver
 */
#define DT_SCMI_PROTOCOL_DEFINE_NODEV(node_id, data, version, events)		\
	DT_SCMI_TRANSPORT_CHANNELS_DECLARE(node_id)				\
	DT_SCMI_PROTOCOL_DATA_DEFINE(node_id,					\
				     DT_SCMI_TRANSPORT_PROTOCOL_ID(node_id),	\
				     data, version, events)

/**
 * @brief Create an SCMI message field
 *
 * Data might not necessarily be encoded in the first
 * x bits of an SCMI message parameter/return value.
 * This comes in handy when building said parameters/
 * return values.
 *
 * @param x Value to encode
 * @param mask Value to perform bitwise-and with `x`
 * @param shift Value to left-shift masked `x`
 *
 * @return the encoded message field
 */
#define SCMI_FIELD_MAKE(x, mask, shift)\
	(((uint32_t)(x) & (mask)) << (shift))

/**
 * @brief SCMI protocol IDs
 *
 * Each SCMI protocol is identified by an ID. Each
 * of these IDs needs to be in decimal since they
 * might be used to build protocol and static channel
 * names.
 */
#define SCMI_PROTOCOL_BASE 16           /**< Base protocol */
#define SCMI_PROTOCOL_POWER_DOMAIN 17   /**< Power domain management protocol */
#define SCMI_PROTOCOL_SYSTEM 18         /**< System power management protocol */
#define SCMI_PROTOCOL_PERF 19           /**< Performance domain management protocol */
#define SCMI_PROTOCOL_CLOCK 20          /**< Clock management protocol */
#define SCMI_PROTOCOL_SENSOR 21         /**< Sensor management protocol */
#define SCMI_PROTOCOL_RESET_DOMAIN 22   /**< Reset domain management protocol */
#define SCMI_PROTOCOL_VOLTAGE_DOMAIN 23 /**< Voltage domain management protocol */
#define SCMI_PROTOCOL_PCAP_MONITOR 24   /**< Power capping and monitoring protocol */
#define SCMI_PROTOCOL_PINCTRL 25        /**< Pin control protocol */

/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_UTIL_H_ */
