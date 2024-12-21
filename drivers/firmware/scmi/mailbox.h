/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_FIRMWARE_SCMI_MAILBOX_H_
#define _ZEPHYR_DRIVERS_FIRMWARE_SCMI_MAILBOX_H_

#include <zephyr/drivers/firmware/scmi/transport.h>
#include <zephyr/drivers/firmware/scmi/util.h>
#include <zephyr/drivers/firmware/scmi/shmem.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT arm_scmi

/* get a `struct device` for a protocol's shared memory area */
#define _SCMI_MBOX_SHMEM_BY_IDX(node_id, idx)					\
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, shmem, idx),			\
		    (DEVICE_DT_GET(DT_PROP_BY_IDX(node_id, shmem, idx))),	\
		    (NULL))

/* get the name of mailbox channel's private data */
#define _SCMI_MBOX_CHAN_NAME(proto, idx)\
	CONCAT(SCMI_TRANSPORT_CHAN_NAME(proto, idx), _, priv)

/* fetch a mailbox channel's doorbell */
#define _SCMI_MBOX_CHAN_DBELL(node_id, name)			\
	COND_CODE_1(DT_PROP_HAS_NAME(node_id, mboxes, name),	\
		    (MBOX_DT_SPEC_GET(node_id, name)),		\
		    ({ }))

/* define private data for a protocol TX channel */
#define _SCMI_MBOX_CHAN_DEFINE_PRIV_TX(node_id, proto)			\
	static struct scmi_mbox_channel _SCMI_MBOX_CHAN_NAME(proto, 0) =\
	{								\
		.shmem = _SCMI_MBOX_SHMEM_BY_IDX(node_id, 0),		\
		.tx = _SCMI_MBOX_CHAN_DBELL(node_id, tx),		\
		.tx_reply = _SCMI_MBOX_CHAN_DBELL(node_id, tx_reply),	\
	}

/*
 * Define a mailbox channel. This does two things:
 *	1) Define the mandatory `struct scmi_channel` structure
 *	2) Define the mailbox-specific private data for said
 *	channel (i.e: a struct scmi_mbox_channel)
 */
#define _SCMI_MBOX_CHAN_DEFINE(node_id, proto, idx)				\
	_SCMI_MBOX_CHAN_DEFINE_PRIV_TX(node_id, proto);				\
	DT_SCMI_TRANSPORT_CHAN_DEFINE(node_id, idx, proto,			\
				    &(_SCMI_MBOX_CHAN_NAME(proto, idx)));	\

/*
 * Optionally define a mailbox channel for a protocol. This is optional
 * because a protocol might not have a dedicated channel.
 */
#define _SCMI_MBOX_CHAN_DEFINE_OPTIONAL(node_id, proto, idx)		\
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, shmem, idx),		\
		    (_SCMI_MBOX_CHAN_DEFINE(node_id, proto, idx)),	\
		    ())

/* define a TX channel for a protocol node. This is preferred over
 * _SCMI_MBOX_CHAN_DEFINE_OPTIONAL() since support for RX channels
 * might be added later on. This macro is supposed to also define
 * the RX channel
 */
#define SCMI_MBOX_PROTO_CHAN_DEFINE(node_id)\
	_SCMI_MBOX_CHAN_DEFINE_OPTIONAL(node_id, DT_REG_ADDR(node_id), 0)

/* define and validate base protocol TX channel */
#define DT_INST_SCMI_MBOX_BASE_CHAN_DEFINE(inst)			\
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, mboxes) != 1 ||		\
		     (DT_INST_PROP_HAS_IDX(inst, shmem, 0) &&		\
		     DT_INST_PROP_HAS_NAME(inst, mboxes, tx)),		\
		     "bad bidirectional channel description");		\
									\
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, mboxes) != 2 ||		\
		     (DT_INST_PROP_HAS_NAME(inst, mboxes, tx) &&	\
		     DT_INST_PROP_HAS_NAME(inst, mboxes, tx_reply)),	\
		     "bad unidirectional channel description");		\
									\
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, shmem) == 1,		\
		     "bad SHMEM count");				\
									\
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, mboxes) <= 2,		\
		     "bad mbox count");					\
									\
	_SCMI_MBOX_CHAN_DEFINE(DT_INST(inst, DT_DRV_COMPAT), SCMI_PROTOCOL_BASE, 0)

/*
 * Define the mailbox-based transport layer. What this does is:
 *
 *	1) Goes through all protocol nodes (children of the `scmi` node)
 *	and creates a `struct scmi_channel` and its associated
 *	`struct scmi_mbox_channel` if the protocol has a dedicated channel.
 *
 *	2) Creates aforementioned structures for the base protocol
 *	(identified by the `scmi` node)
 *
 *	3) "registers" the driver via `DT_INST_SCMI_TRANSPORT_DEFINE()`.
 */
#define DT_INST_SCMI_MAILBOX_DEFINE(inst, level, prio, api)			\
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SCMI_MBOX_PROTO_CHAN_DEFINE)	\
	DT_INST_SCMI_MBOX_BASE_CHAN_DEFINE(inst)				\
	DT_INST_SCMI_TRANSPORT_DEFINE(inst, NULL, NULL, NULL, level, prio, api)

struct scmi_mbox_channel {
	/* SHMEM area bound to the channel */
	const struct device *shmem;
	/* TX dbell */
	struct mbox_dt_spec tx;
	/* TX reply dbell */
	struct mbox_dt_spec tx_reply;
};

#endif /* _ZEPHYR_DRIVERS_FIRMWARE_SCMI_MAILBOX_H_ */
