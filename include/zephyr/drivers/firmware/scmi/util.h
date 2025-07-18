/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM SCMI utility header
 *
 * Contains various utility macros and macros used for protocol and
 * transport "registration".
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_UTIL_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_UTIL_H_

/**
 * @brief Build protocol name from its ID
 *
 * Given a protocol ID, this macro builds the protocol
 * name. This is done by concatenating the scmi_protocol_
 * construct with the given protocol ID.
 *
 * @param proto protocol ID in decimal format
 *
 * @return protocol name
 */
#define SCMI_PROTOCOL_NAME(proto) CONCAT(scmi_protocol_, proto)

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
 * @param node_id protocol node identifier
 * @idx channel index. Should be 0 for TX channels and 1 for
 * RX channels
 */
#define DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, idx)\
	DT_INST_NODE_HAS_PROP(idx, shmems)
#else /* CONFIG_ARM_SCMI_MAILBOX_TRANSPORT */
#error "Transport with static channels needs to define HAS_CHAN macro"
#endif /* CONFIG_ARM_SCMI_MAILBOX_TRANSPORT */

#define SCMI_TRANSPORT_CHAN_NAME(proto, idx) CONCAT(scmi_channel_, proto, _, idx)

/**
 * @brief Declare a TX SCMI channel
 *
 * Given a node_id for a protocol, this macro declares the SCMI
 * TX channel statically bound to said protocol via the "extern"
 * qualifier. This is useful when the transport layer driver
 * supports static channels since all channel structures are
 * defined inside the transport layer driver.
 *
 * @param node_id protocol node identifier
 */
#define DT_SCMI_TRANSPORT_TX_CHAN_DECLARE(node_id)				\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 0),		\
		    (extern struct scmi_channel					\
		     SCMI_TRANSPORT_CHAN_NAME(DT_REG_ADDR_RAW(node_id), 0);),	\
		    (extern struct scmi_channel					\
		     SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 0);))		\

#define DT_SCMI_TRANSPORT_RX_CHAN_DECLARE(node_id)				\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 1),		\
		    (extern struct scmi_channel					\
		     SCMI_TRANSPORT_CHAN_NAME(DT_REG_ADDR_RAW(node_id), 1);),	\
		    (extern struct scmi_channel					\
		     SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 1);))		\

/**
 * @brief Declare SCMI TX/RX channels
 *
 * Given a node_id for a protocol, this macro declares the
 * SCMI TX and RX channels statically bound to said protocol via
 * the "extern" qualifier. Since RX channels are currently not
 * supported, this is equivalent to DT_SCMI_TRANSPORT_TX_CHAN_DECLARE().
 * Despite this, users should opt for this macro instead of the TX-specific
 * one.
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
 * protocol's node instance number and the DT_DRV_COMPAT macro.
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
 * @param node_id protocol node identifier
 *
 * @return reference to the struct scmi_channel of the TX channel
 * bound to the protocol identifier by node_id
 */
#define DT_SCMI_TRANSPORT_TX_CHAN(node_id)					\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 0),		\
		    (&SCMI_TRANSPORT_CHAN_NAME(DT_REG_ADDR_RAW(node_id), 0)),	\
		    (&SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 0)))

#define DT_SCMI_TRANSPORT_RX_CHAN(node_id)					\
	COND_CODE_1(DT_SCMI_TRANSPORT_PROTO_HAS_CHAN(node_id, 1),		\
		    (&SCMI_TRANSPORT_CHAN_NAME(DT_REG_ADDR_RAW(node_id), 1)),	\
		    (&SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 1)))
/**
 * @brief Define an SCMI channel for a protocol
 *
 * This macro defines a struct scmi_channel for a given protocol.
 * This should be used by the transport layer driver to statically
 * define SCMI channels for the protocols.
 *
 * @param node_id protocol node identifier
 * @param idx channel index. Should be 0 for TX channels and 1
 * for RX channels
 * @param proto protocol ID in decimal format
 */
#define DT_SCMI_TRANSPORT_CHAN_DEFINE(node_id, idx, proto, pdata)		\
	struct scmi_channel SCMI_TRANSPORT_CHAN_NAME(proto, idx) =		\
	{									\
		.data = pdata,							\
	}

/**
 * @brief Define an SCMI protocol's data
 *
 * Each SCMI protocol is identified by a struct scmi_protocol
 * placed in a linker section called scmi_protocol. Each protocol
 * driver is required to use this macro for "registration". Using
 * this macro directly is higly discouraged and users should opt
 * for macros such as DT_SCMI_PROTOCOL_DEFINE_NODEV() or
 * DT_SCMI_PROTOCOL_DEFINE(), which also takes care of the static
 * channel declaration (if applicable).
 *
 * @param node_id protocol node identifier
 * @param proto protocol ID in decimal format
 * @param pdata protocol private data
 */
#define DT_SCMI_PROTOCOL_DATA_DEFINE(node_id, proto, pdata)			\
	STRUCT_SECTION_ITERABLE(scmi_protocol, SCMI_PROTOCOL_NAME(proto)) =	\
	{									\
		.id = proto,							\
		.tx = DT_SCMI_TRANSPORT_TX_CHAN(node_id),			\
		.rx = DT_SCMI_TRANSPORT_RX_CHAN(node_id),			\
		.data = pdata,							\
	}

#else /* CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS */

#define DT_SCMI_TRANSPORT_CHANNELS_DECLARE(node_id)

#define DT_SCMI_PROTOCOL_DATA_DEFINE(node_id, proto, pdata)			\
	STRUCT_SECTION_ITERABLE(scmi_protocol, SCMI_PROTOCOL_NAME(proto)) =	\
	{									\
		.id = proto,							\
		.data = pdata,							\
	}

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
 *	1) It defines a `struct scmi_protocol`, which is
 *	needed by all protocol drivers to work with the SCMI API.
 *
 *	2) It declares the static channels bound to the protocol.
 *	This is only applicable if the transport layer driver
 *	supports static channels.
 *
 *	3) It creates a `struct device` a sets the `data` field
 *	to the newly defined `struct scmi_protocol`. This is
 *	needed because the protocol driver needs to work with the
 *	SCMI API **and** the subsystem API.
 *
 * @param node_id protocol node identifier
 * @param init_fn pointer to protocol's initialization function
 * @param api pointer to protocol's subsystem API
 * @param pm pointer to the protocol's power management resources
 * @param data pointer to protocol's private data
 * @param config pointer to protocol's private constant data
 * @param level protocol initialization level
 * @param prio protocol's priority within its initialization level
 */
#define DT_SCMI_PROTOCOL_DEFINE(node_id, init_fn, pm, data, config,		\
				level, prio, api)				\
	DT_SCMI_TRANSPORT_CHANNELS_DECLARE(node_id)				\
	DT_SCMI_PROTOCOL_DATA_DEFINE(node_id, DT_REG_ADDR_RAW(node_id), data);	\
	DEVICE_DT_DEFINE(node_id, init_fn, pm,					\
			 &SCMI_PROTOCOL_NAME(DT_REG_ADDR_RAW(node_id)),		\
					     config, level, prio, api)

/**
 * @brief Just like DT_SCMI_PROTOCOL_DEFINE(), but uses an instance
 * of a `DT_DRV_COMPAT` compatible instead of a node identifier
 *
 * @param inst instance number
 * @param init_fn pointer to protocol's initialization function
 * @param api pointer to protocol's subsystem API
 * @param pm pointer to the protocol's power management resources
 * @param data pointer to protocol's private data
 * @param config pointer to protocol's private constant data
 * @param level protocol initialization level
 * @param prio protocol's priority within its initialization level
 */
#define DT_INST_SCMI_PROTOCOL_DEFINE(inst, init_fn, pm, data, config,		\
				     level, prio, api)				\
	DT_SCMI_PROTOCOL_DEFINE(DT_INST(inst, DT_DRV_COMPAT), init_fn, pm,	\
				data, config, level, prio, api)

/**
 * @brief Define an SCMI protocol with no device
 *
 * Variant of DT_SCMI_PROTOCOL_DEFINE(), but no `struct device` is
 * created and no initialization function is called during system
 * initialization. This is useful for protocols that are not really
 * part of a subsystem with an API (e.g: pinctrl).
 *
 * @param node_id protocol node identifier
 * @param data protocol private data
 */
#define DT_SCMI_PROTOCOL_DEFINE_NODEV(node_id, data)				\
	DT_SCMI_TRANSPORT_CHANNELS_DECLARE(node_id)				\
	DT_SCMI_PROTOCOL_DATA_DEFINE(node_id, DT_REG_ADDR_RAW(node_id), data)

/**
 * @brief Create an SCMI message field
 *
 * Data might not necessarily be encoded in the first
 * x bits of an SCMI message parameter/return value.
 * This comes in handy when building said parameters/
 * return values.
 *
 * @param x value to encode
 * @param mask value to perform bitwise-and with `x`
 * @param shift value to left-shift masked `x`
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
#define SCMI_PROTOCOL_BASE 16
#define SCMI_PROTOCOL_POWER_DOMAIN 17
#define SCMI_PROTOCOL_SYSTEM 18
#define SCMI_PROTOCOL_PERF 19
#define SCMI_PROTOCOL_CLOCK 20
#define SCMI_PROTOCOL_SENSOR 21
#define SCMI_PROTOCOL_RESET_DOMAIN 22
#define SCMI_PROTOCOL_VOLTAGE_DOMAIN 23
#define SCMI_PROTOCOL_PCAP_MONITOR 24
#define SCMI_PROTOCOL_PINCTRL 25
#define SCMI_PROTOCOL_BBM	129

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_UTIL_H_ */
